#!/usr/bin/env python3
"""Convert a flat RP2350 firmware binary to UF2.

This intentionally covers the small subset the badge build needs:
256-byte payload UF2 blocks, a family ID, and the optional RP2350
absolute metadata block that picotool's --abs-block emits for flash
downloads.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import struct


MAGIC0 = 0x0A324655
MAGIC1 = 0x9E5D5157
MAGIC_END = 0x0AB16F30
FLAG_FAMILY_ID = 0x00002000
FLAG_EXTENSION_TAGS = 0x00008000
PAYLOAD_SIZE = 256

FAMILIES = {
    "absolute": 0xE48BFF57,
    "rp2040": 0xE48BFF56,
    "data": 0xE48BFF58,
    "rp2350-arm-s": 0xE48BFF59,
    "rp2350-riscv": 0xE48BFF5A,
    "rp2350-arm-ns": 0xE48BFF5B,
}


def parse_u32(value: str) -> int:
    if value in FAMILIES:
        return FAMILIES[value]
    return int(value, 0)


def write_block(
    output,
    *,
    address: int,
    payload: bytes,
    block_number: int,
    total_blocks: int,
    family_id: int,
    flags: int = FLAG_FAMILY_ID,
) -> None:
    payload = payload[:PAYLOAD_SIZE].ljust(PAYLOAD_SIZE, b"\xff")
    header = struct.pack(
        "<IIIIIIII",
        MAGIC0,
        MAGIC1,
        flags,
        address,
        PAYLOAD_SIZE,
        block_number,
        total_blocks,
        family_id,
    )
    output.write(header)
    output.write(payload)
    output.write(b"\x00" * (512 - 32 - PAYLOAD_SIZE - 4))
    output.write(struct.pack("<I", MAGIC_END))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--base", type=parse_u32, default=0x10000000)
    parser.add_argument("--family", type=parse_u32, default=FAMILIES["rp2350-arm-s"])
    parser.add_argument("--abs-block", action="store_true")
    parser.add_argument("--abs-block-loc", type=parse_u32, default=0x10FFFF00)
    args = parser.parse_args()

    data = args.input.read_bytes()
    firmware_blocks = (len(data) + PAYLOAD_SIZE - 1) // PAYLOAD_SIZE

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("wb") as output:
        if args.abs_block:
            # Match picotool: the RP2350 absolute E10 block is not part of
            # the firmware-family block sequence used to detect completion.
            write_block(
                output,
                address=args.abs_block_loc,
                payload=b"\xef" * PAYLOAD_SIZE,
                block_number=0,
                total_blocks=2,
                family_id=FAMILIES["absolute"],
                flags=FLAG_FAMILY_ID | FLAG_EXTENSION_TAGS,
            )

        for block_number, offset in enumerate(range(0, len(data), PAYLOAD_SIZE)):
            write_block(
                output,
                address=args.base + offset,
                payload=data[offset : offset + PAYLOAD_SIZE],
                block_number=block_number,
                total_blocks=firmware_blocks,
                family_id=args.family,
            )


if __name__ == "__main__":
    main()
