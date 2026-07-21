#!/usr/bin/env python3
"""Verify SDK-v1 container metadata and that apps only import fixed veneers."""

from __future__ import annotations

import argparse
import struct
import subprocess
import tempfile
from pathlib import Path

from gen_dcapp_abi_veneers import IMPORTS


HEADER = struct.Struct("<IHH" + "I" * 14 + "32sI156s")
MAGIC = 0x50414344
META_MAGIC = 0x314D4344
SDK_ABI = 1
VENEER_START = 0x10070000
VENEER_END = 0x10080000
VENEER_STRIDE = 12
UF2_BLOCK_SIZE = 512
UF2_MAGIC0 = 0x0A324655
UF2_MAGIC1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30
PACKAGED_APPS = (
    "gb", "nes", "arduboy", "ir", "image", "music", "badusb", "autoclicker", "gamepad",
    "lasertag", "raspyjack", "pwnagotchi", "pong", "tetris", "arkanoid", "flappy", "trex",
    "doom", "chips", "scorch", "pipe", "sokoban", "openjazz", "soccer", "starfield", "spiro",
    "cube", "dvd-bounce", "scroll-pattern",
)


def check_image(path: Path) -> None:
    raw = path.read_bytes()
    if len(raw) < HEADER.size:
        raise AssertionError(f"{path}: shorter than header")
    fields = HEADER.unpack(raw[:HEADER.size])
    magic, header_size, abi = fields[:3]
    metadata = fields[-1]
    meta_magic, schema, meta_abi, category, name_len = struct.unpack_from("<IHHBB", metadata)
    if (magic, header_size, abi) != (MAGIC, HEADER.size, SDK_ABI):
        raise AssertionError(f"{path}: invalid SDK-v1 header")
    if (meta_magic, schema, meta_abi) != (META_MAGIC, 1, SDK_ABI) or not 1 <= category <= 6 or not name_len:
        raise AssertionError(f"{path}: invalid SDK-v1 metadata")


def check_imports(binary: Path, nm: str) -> None:
    output = subprocess.check_output([nm, "--defined-only", str(binary)], text=True)
    bad: list[str] = []
    for line in output.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1].lower() == "a":
            address = int(parts[0], 16)
            if 0x10000000 <= address < VENEER_START:
                bad.append(parts[2])
            elif VENEER_END <= address < 0x10080000:
                bad.append(parts[2])
    if bad:
        raise AssertionError(f"{binary}: direct firmware imports: {', '.join(bad)}")


def read_uf2_bytes(path: Path) -> dict[int, int]:
    raw = path.read_bytes()
    if len(raw) % UF2_BLOCK_SIZE:
        raise AssertionError(f"{path}: incomplete UF2 block")
    payload: dict[int, int] = {}
    for offset in range(0, len(raw), UF2_BLOCK_SIZE):
        magic0, magic1, _flags, address, payload_size, _block, _total, _family = struct.unpack_from(
            "<8I", raw, offset)
        if (magic0, magic1) != (UF2_MAGIC0, UF2_MAGIC1):
            raise AssertionError(f"{path}: invalid UF2 header at block {offset // UF2_BLOCK_SIZE}")
        if payload_size > 256 or struct.unpack_from("<I", raw, offset + UF2_BLOCK_SIZE - 4)[0] != UF2_MAGIC_END:
            raise AssertionError(f"{path}: invalid UF2 payload at block {offset // UF2_BLOCK_SIZE}")
        for index, value in enumerate(raw[offset + 32:offset + 32 + payload_size]):
            previous = payload.setdefault(address + index, value)
            if previous != value:
                raise AssertionError(f"{path}: conflicting data at 0x{address + index:08x}")
    return payload


def check_firmware_veneers(elf: Path, uf2: Path, objcopy: str) -> None:
    with tempfile.TemporaryDirectory(prefix="dcapp-sdk-test-") as temp:
        section = Path(temp) / "dcapp_abi_veneer.bin"
        subprocess.run([objcopy, f"--dump-section=.dcapp_abi_veneer={section}", str(elf)], check=True)
        expected = section.read_bytes()
    expected_size = len(IMPORTS) * VENEER_STRIDE
    if len(expected) != expected_size:
        raise AssertionError(f"{elf}: veneer section is {len(expected)} bytes, expected {expected_size}")
    actual = read_uf2_bytes(uf2)
    missing = [VENEER_START + offset for offset in range(len(expected)) if VENEER_START + offset not in actual]
    if missing:
        raise AssertionError(f"{uf2}: missing SDK veneer bytes starting at 0x{missing[0]:08x}")
    for offset, value in enumerate(expected):
        address = VENEER_START + offset
        if actual[address] != value:
            raise AssertionError(f"{uf2}: SDK veneer differs from ELF at 0x{address:08x}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--apps-dir", required=True, type=Path)
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--nm", default="arm-none-eabi-nm")
    parser.add_argument("--objcopy", default="arm-none-eabi-objcopy")
    parser.add_argument("--firmware-elf", type=Path)
    parser.add_argument("--firmware-uf2", type=Path)
    args = parser.parse_args()
    for stem in PACKAGED_APPS:
        check_image(args.apps_dir / f"{stem}.DC32")
    for binary in sorted(args.build_dir.glob("dcapp_*")):
        stem = binary.name.removeprefix("dcapp_").replace("_", "-")
        if binary.is_file() and not binary.suffix and stem in PACKAGED_APPS:
            check_imports(binary, args.nm)
    check_firmware_veneers(args.firmware_elf or args.build_dir / "DC32-cfw",
                           args.firmware_uf2 or args.build_dir / "DC32-cfw.uf2",
                           args.objcopy)
    print("DCAPP SDK v1 verification passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
