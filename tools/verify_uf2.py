#!/usr/bin/env python3
"""Validate the badge UF2 artifact before CI uploads it."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys


MAGIC0 = 0x0A324655
MAGIC1 = 0x9E5D5157
MAGIC_END = 0x0AB16F30
FLAG_FAMILY_ID = 0x00002000
PAYLOAD_SIZE = 256

FAMILY_ABSOLUTE = 0xE48BFF57
FAMILY_RP2350_ARM_S = 0xE48BFF59
ABS_BLOCK_LOC = 0x10FFFF00


def fail(message: str) -> None:
    raise SystemExit(f"UF2 validation failed: {message}")


def parse_block(block: bytes, offset: int) -> tuple[int, int, int, int, int, bytes]:
    if len(block) != 512:
        fail(f"short block at byte offset {offset}")

    magic0, magic1, flags, target, payload_size, block_no, total_blocks, family = struct.unpack(
        "<IIIIIIII", block[:32]
    )
    (magic_end,) = struct.unpack("<I", block[-4:])

    if magic0 != MAGIC0 or magic1 != MAGIC1 or magic_end != MAGIC_END:
        fail(f"bad UF2 magic in block {block_no}")
    if (flags & FLAG_FAMILY_ID) == 0:
        fail(f"missing family ID flag in block {block_no}")
    if payload_size != PAYLOAD_SIZE:
        fail(f"unexpected payload size {payload_size} in block {block_no}")

    return target, block_no, total_blocks, family, flags, block[32 : 32 + payload_size]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("uf2", type=Path)
    parser.add_argument("--input-bin", type=Path, required=True)
    parser.add_argument("--base", type=lambda v: int(v, 0), default=0x10000000)
    args = parser.parse_args()

    uf2_data = args.uf2.read_bytes()
    bin_data = args.input_bin.read_bytes()
    if len(uf2_data) == 0 or len(uf2_data) % 512 != 0:
        fail("file size is not a non-zero multiple of 512 bytes")

    block_count = len(uf2_data) // 512
    firmware = bytearray()
    expected_firmware_addr = args.base

    for idx in range(block_count):
        target, block_no, total_blocks, family, _flags, payload = parse_block(
            uf2_data[idx * 512 : (idx + 1) * 512], idx * 512
        )

        if block_no != idx:
            fail(f"block number {block_no} does not match position {idx}")
        if total_blocks != block_count:
            fail(f"block {idx} total {total_blocks} does not match {block_count}")

        if idx == 0:
            if target != ABS_BLOCK_LOC or family != FAMILY_ABSOLUTE:
                fail("first block is not the required RP2350 absolute block")
            if payload != b"\xef" * PAYLOAD_SIZE:
                fail("absolute block payload does not match RP2350 flash workaround data")
            continue

        if family != FAMILY_RP2350_ARM_S:
            fail(f"block {idx} has unexpected family 0x{family:08x}")
        if target != expected_firmware_addr:
            fail(
                f"block {idx} target 0x{target:08x}, expected 0x{expected_firmware_addr:08x}"
            )

        firmware.extend(payload)
        expected_firmware_addr += PAYLOAD_SIZE

    firmware = firmware[: len(bin_data)]
    if firmware != bin_data:
        fail(
            "UF2 payload does not reconstruct the input BIN "
            f"(bin sha256={hashlib.sha256(bin_data).hexdigest()}, "
            f"uf2 sha256={hashlib.sha256(firmware).hexdigest()})"
        )

    print(
        "UF2 validation ok: "
        f"{block_count} blocks, {len(bin_data)} firmware bytes, "
        f"sha256={hashlib.sha256(uf2_data).hexdigest()}"
    )


if __name__ == "__main__":
    try:
        main()
    except BrokenPipeError:
        sys.exit(1)
