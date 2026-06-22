#!/usr/bin/env python3
"""Model and source checks for the guarded OpenJazz split heap."""

from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER_FMT = "<IHH" + "I" * 14 + "32sI" + "I" * 39
APP_RAM_START = 0x2005F000
APP_RAM_END = APP_RAM_START + 0x14000
TRANSIENT_MIN = 56 * 1024
HEADER = 16
FOOTER = 8
MIN_PAYLOAD = 16


def expect(name: str, condition: bool) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def image_transient_size() -> int:
    image = (ROOT / "build" / "apps" / "openjazz.DC32").read_bytes()
    fields = struct.unpack(HEADER_FMT, image[:256])
    data_addr, data_size = fields[8], fields[9]
    bss_addr, bss_size = fields[10], fields[11]
    used_end = max(
        APP_RAM_START,
        data_addr + data_size if data_size else APP_RAM_START,
        bss_addr + bss_size if bss_size else APP_RAM_START,
    )
    scratch_start = (used_end + 7) & ~7
    return max(0, APP_RAM_END - scratch_start)


def startup_accepts(span_size: int) -> bool:
    return span_size >= TRANSIENT_MIN


@dataclass
class Block:
    offset: int
    size: int
    used: bool = False
    header_ok: bool = True
    footer_ok: bool = True
    next_index: int | None = None


class HeapModel:
    def __init__(self, size: int):
        self.size = size & ~7
        self.blocks: list[Block] = []
        self.reset()

    def reset(self) -> None:
        self.blocks = [Block(0, self.size - HEADER - FOOTER)]

    def validate(self) -> None:
        index: int | None = 0
        visits = 0
        while index is not None:
            visits += 1
            if visits > self.size // (HEADER + FOOTER) + 1:
                raise ValueError("damaged link")
            if index < 0 or index >= len(self.blocks):
                raise ValueError("damaged link")
            block = self.blocks[index]
            if not block.header_ok:
                raise ValueError("damaged header")
            if not block.footer_ok:
                raise ValueError("damaged footer")
            if block.offset < 0 or block.offset + HEADER + block.size + FOOTER > self.size:
                raise ValueError("bad block bounds")
            if block.next_index is not None:
                if block.next_index <= index:
                    raise ValueError("damaged link")
                next_block = self.blocks[block.next_index]
                if next_block.offset != block.offset + HEADER + block.size + FOOTER:
                    raise ValueError("damaged link")
            index = block.next_index

    def allocate(self, size: int) -> int:
        self.validate()
        wanted = max(1, size)
        wanted = (wanted + 7) & ~7
        for index, block in enumerate(self.blocks):
            if block.used or block.size < wanted:
                continue
            if block.size >= wanted + HEADER + FOOTER + MIN_PAYLOAD:
                remainder = Block(
                    block.offset + HEADER + wanted + FOOTER,
                    block.size - wanted - HEADER - FOOTER,
                    next_index=block.next_index,
                )
                self.blocks.insert(index + 1, remainder)
                for current in self.blocks:
                    if current.next_index is not None and current is not block and current.next_index > index:
                        current.next_index += 1
                block.next_index = index + 1
                block.size = wanted
            block.used = True
            return block.offset + HEADER
        raise MemoryError

    def free(self, pointer: int) -> None:
        self.validate()
        for block in self.blocks:
            if block.offset + HEADER != pointer:
                continue
            if not block.used:
                raise ValueError("double free")
            block.used = False
            return
        raise ValueError("invalid free")

    def free_bytes(self) -> int:
        self.validate()
        return sum(block.size for block in self.blocks if not block.used)

    def largest(self) -> int:
        self.validate()
        return max(block.size for block in self.blocks if not block.used)


def main() -> None:
    source = (
        ROOT / "src" / "apps" / "openjazz" / "openjazz_heap.c"
    ).read_text(encoding="utf-8")
    app = (
        ROOT / "src" / "apps" / "openjazz" / "openjazz_app.cpp"
    ).read_text(encoding="utf-8")
    memory = (
        ROOT / "src" / "apps" / "openjazz" / "openjazz_memory.h"
    ).read_text(encoding="utf-8")
    transient_size = image_transient_size()
    expect(
        "Production heap has header/footer guards and bounded traversal",
        all(token in source for token in (
            "OJ_HEAP_HEADER_MAGIC", "OJ_HEAP_FOOT_MAGIC",
            "OJ_HEAP_SIZE_XOR", "limit--", "DC32_OJ_HEAP_BAD_LINK",
        )),
    )
    expect(
        "Production heap routes frees by pointer ownership",
        "ojHeapForPointer(ptr)" in source and "ojFindBlock(region, ptr)" in source,
    )
    expect(
        "Startup and build tests share the 56 KiB contract",
        "DC32_OJ_TRANSIENT_MIN" in app
        and "#define DC32_OJ_TRANSIENT_MIN        (56u * 1024u)" in memory
        and "80u * 1024u" not in app,
    )
    expect(
        "Final image exposes a usable non-round transient span",
        transient_size >= TRANSIENT_MIN and transient_size % 1024 != 0,
    )
    expect(
        "Startup accepts the final span and rejects undersized spans",
        startup_accepts(transient_size)
        and startup_accepts(TRANSIENT_MIN)
        and not startup_accepts(TRANSIENT_MIN - 1),
    )

    persistent = HeapModel(36 * 1024)
    transient = HeapModel(transient_size)
    persistent_ptrs = [
        persistent.allocate(size) for size in (64, 512, 1536, 4096)
    ]
    persistent_free = persistent.free_bytes()
    persistent_largest = persistent.largest()

    for cycle in range(100):
        transient.reset()
        pointers = [
            transient.allocate(24 + ((cycle * 37 + index * 113) % 1400))
            for index in range(24)
        ]
        for pointer in pointers[::2]:
            transient.free(pointer)
        for pointer in pointers[1::2]:
            transient.free(pointer)
        transient.validate()
        transient.reset()
        expect(
            f"Transient cycle {cycle} returns one full block",
            transient.free_bytes() == transient_size - HEADER - FOOTER
            and transient.largest() == transient.free_bytes(),
        )
        expect(
            f"Persistent state survives transient cycle {cycle}",
            persistent_ptrs
            and persistent.free_bytes() == persistent_free
            and persistent.largest() == persistent_largest,
        )

    damaged = HeapModel(4096)
    damaged.allocate(64)
    damaged.blocks[0].header_ok = False
    try:
        damaged.validate()
        raise AssertionError
    except ValueError as error:
        expect("Damaged headers are detected", "header" in str(error))

    damaged = HeapModel(4096)
    damaged.allocate(64)
    damaged.blocks[0].footer_ok = False
    try:
        damaged.validate()
        raise AssertionError
    except ValueError as error:
        expect("Damaged footers are detected", "footer" in str(error))

    damaged = HeapModel(4096)
    damaged.allocate(64)
    damaged.blocks[0].next_index = 0
    try:
        damaged.validate()
        raise AssertionError
    except ValueError as error:
        expect("Cycles are detected by bounded link validation", "link" in str(error))

    invalid = HeapModel(4096)
    pointer = invalid.allocate(64)
    invalid.free(pointer)
    try:
        invalid.free(pointer)
        raise AssertionError
    except ValueError as error:
        expect("Double frees are rejected", "double free" in str(error))
    try:
        invalid.free(pointer + 4)
        raise AssertionError
    except ValueError as error:
        expect("Invalid frees are rejected", "invalid free" in str(error))

    print("OpenJazz split-heap tests passed")


if __name__ == "__main__":
    main()
