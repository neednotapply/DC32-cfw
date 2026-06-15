#!/usr/bin/env python3
"""Stamp a linked fixed-address ELF image into the DCAPP container format."""

from __future__ import annotations

import argparse
import binascii
import hashlib
import re
import struct
import subprocess
from pathlib import Path


DCAPP_MAGIC = 0x50414344
DCAPP_ABI_VERSION = 4
DCAPP_HEADER_SIZE = 256
DCAPP_CONTRACT_MAGIC = 0x43444332
DCAPP_CONTRACT_HASH_WORDS = 4

SYMBOL_NAMES = {
    "__data_data",
    "__data_start",
    "__data_size",
    "__bss_start",
    "__bss_size",
    "dcAppEntry",
    "dcAppAbort",
    "dcAppRefreshDisplayOptions",
}


def parse_int(value: str) -> int:
    return int(value, 0)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--nm", required=True, type=Path)
    parser.add_argument("--elf", required=True, type=Path)
    parser.add_argument("--raw", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--runtime-id", required=True, type=parse_int)
    parser.add_argument("--flags", type=parse_int, default=0)
    parser.add_argument("--load-addr", required=True, type=parse_int)
    parser.add_argument("--app-ram-start", required=True, type=parse_int)
    parser.add_argument("--app-ram-size", required=True, type=parse_int)
    parser.add_argument("--contract-header", required=True, type=Path)
    return parser.parse_args()


def read_symbols(nm: Path, elf: Path) -> dict[str, int]:
    proc = subprocess.run(
        [str(nm), "-g", "--defined-only", str(elf)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=True,
    )
    symbols: dict[str, int] = {}
    for line in proc.stdout.splitlines():
        parts = line.strip().split(maxsplit=2)
        if len(parts) != 3:
            continue
        addr_s, _sym_type, name = parts
        if name in SYMBOL_NAMES:
            symbols[name] = int(addr_s, 16)
    return symbols


def required(symbols: dict[str, int], name: str) -> int:
    if name not in symbols:
        raise RuntimeError(f"missing required symbol: {name}")
    return symbols[name]


def thumb_offset(symbols: dict[str, int], name: str, load_addr: int) -> int:
    return (required(symbols, name) - load_addr) | 1


def optional_thumb_offset(symbols: dict[str, int], name: str, load_addr: int) -> int:
    if name not in symbols:
        return 0
    return (symbols[name] - load_addr) | 1


def read_contract_words(path: Path) -> list[int]:
    text = path.read_text(encoding="ascii")
    match = re.search(r"^#define\s+DCAPP_BUILD_CONTRACT_WORDS\s+(.+)$", text, re.MULTILINE)
    if not match:
        raise RuntimeError(f"{path} does not define DCAPP_BUILD_CONTRACT_WORDS")
    words = [int(part.rstrip("uU"), 0) for part in match.group(1).split(",")]
    if len(words) != DCAPP_CONTRACT_HASH_WORDS:
        raise RuntimeError(f"{path} has {len(words)} contract words, expected {DCAPP_CONTRACT_HASH_WORDS}")
    return words


def main() -> int:
    args = parse_args()
    symbols = read_symbols(args.nm, args.elf)
    contract_words = read_contract_words(args.contract_header)
    raw = bytearray(args.raw.read_bytes())
    if len(raw) < DCAPP_HEADER_SIZE:
        raise RuntimeError(f"{args.raw} is smaller than the DCAPP header")

    data_data = required(symbols, "__data_data")
    data_start = required(symbols, "__data_start")
    data_size = required(symbols, "__data_size")
    bss_start = required(symbols, "__bss_start")
    bss_size = required(symbols, "__bss_size")
    data_load_offset = data_data - args.load_addr if data_size else 0
    image_size = len(raw)
    payload = bytes(raw[DCAPP_HEADER_SIZE:image_size])
    build_id = hashlib.sha256(payload).digest()
    crc32 = binascii.crc32(payload) & 0xFFFFFFFF

    reserved = [0] * 39
    reserved[0] = DCAPP_CONTRACT_MAGIC
    reserved[1 : 1 + DCAPP_CONTRACT_HASH_WORDS] = contract_words

    header = struct.pack(
        "<IHH" + "I" * 14 + "32sI" + "I" * 39,
        DCAPP_MAGIC,
        DCAPP_HEADER_SIZE,
        DCAPP_ABI_VERSION,
        args.runtime_id,
        args.flags,
        args.load_addr,
        image_size,
        data_load_offset,
        data_start,
        data_size,
        bss_start,
        bss_size,
        thumb_offset(symbols, "dcAppEntry", args.load_addr),
        optional_thumb_offset(symbols, "dcAppAbort", args.load_addr),
        optional_thumb_offset(symbols, "dcAppRefreshDisplayOptions", args.load_addr),
        args.app_ram_start,
        args.app_ram_size,
        build_id,
        crc32,
        *reserved,
    )
    if len(header) != DCAPP_HEADER_SIZE:
        raise RuntimeError(f"internal header size error: {len(header)}")
    raw[:DCAPP_HEADER_SIZE] = header
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(raw)
    print(f"Wrote {args.output} ({image_size} bytes, crc32=0x{crc32:08x})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
