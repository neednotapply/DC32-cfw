#!/usr/bin/env python3
"""Build small redistributable NROM workloads for DC32 NES profiling."""

from __future__ import annotations

import argparse
from pathlib import Path


PRG_SIZE = 16 * 1024
CHR_SIZE = 8 * 1024
LOAD_ADDR = 0xC000


class Code:
    def __init__(self) -> None:
        self.data = bytearray()
        self.labels: dict[str, int] = {}
        self.fixups: list[tuple[int, str, str]] = []

    @property
    def pc(self) -> int:
        return LOAD_ADDR + len(self.data)

    def emit(self, *values: int) -> None:
        self.data.extend(value & 0xFF for value in values)

    def label(self, name: str) -> None:
        self.labels[name] = self.pc

    def absolute(self, opcode: int, label: str) -> None:
        self.emit(opcode, 0, 0)
        self.fixups.append((len(self.data) - 2, label, "abs"))

    def relative(self, opcode: int, label: str) -> None:
        self.emit(opcode, 0)
        self.fixups.append((len(self.data) - 1, label, "rel"))

    def finish(self) -> bytes:
        for offset, label, kind in self.fixups:
            target = self.labels[label]
            if kind == "abs":
                self.data[offset] = target & 0xFF
                self.data[offset + 1] = target >> 8
            else:
                origin_after_operand = LOAD_ADDR + offset + 1
                delta = target - origin_after_operand
                if not -128 <= delta <= 127:
                    raise ValueError(f"branch to {label} is out of range")
                self.data[offset] = delta & 0xFF
        return bytes(self.data)


def common_reset(code: Code) -> None:
    code.label("reset")
    code.emit(0x78, 0xD8)  # SEI; CLD
    code.emit(0xA2, 0xFF, 0x9A)  # LDX #$ff; TXS
    code.emit(0xE8)  # INX -> 0
    for address in (0x2000, 0x2001, 0x4010):
        code.emit(0x8E, address & 0xFF, address >> 8)  # STX abs


def cpu_workload() -> tuple[bytes, bytes]:
    code = Code()
    common_reset(code)
    code.emit(0xA9, 0x01, 0x85, 0x00)  # LDA #1; STA $00
    code.label("loop")
    code.emit(
        0xA5, 0x00,       # LDA $00
        0x69, 0x3D,       # ADC #$3d
        0x49, 0xA7,       # EOR #$a7
        0x85, 0x00,       # STA $00
        0xAA,             # TAX
        0x2A,             # ROL A
        0x65, 0x00,       # ADC $00
        0xCA,             # DEX
    )
    code.relative(0xD0, "loop")  # BNE
    code.absolute(0x4C, "loop")  # JMP
    code.label("nmi")
    code.emit(0x40)  # RTI
    return code.finish(), bytes(CHR_SIZE)


def ppu_workload() -> tuple[bytes, bytes]:
    code = Code()
    common_reset(code)
    code.label("wait_vblank")
    code.emit(0x2C, 0x02, 0x20)  # BIT $2002
    code.relative(0x10, "wait_vblank")  # BPL

    # Fill the nametable with tile zero.
    code.emit(0xA9, 0x20, 0x8D, 0x06, 0x20, 0xA9, 0x00, 0x8D, 0x06, 0x20)
    code.emit(0xA2, 0x04, 0xA0, 0x00, 0xA9, 0x00)  # four 256-byte pages
    code.label("name_inner")
    code.emit(0x8D, 0x07, 0x20, 0x88)  # STA $2007; DEY
    code.relative(0xD0, "name_inner")
    code.emit(0xCA)
    code.relative(0xD0, "name_inner")

    # Palette and OAM data are deliberately simple but exercise both layers.
    code.emit(0xA9, 0x3F, 0x8D, 0x06, 0x20, 0xA9, 0x00, 0x8D, 0x06, 0x20)
    code.emit(0xA2, 0x20)
    code.label("palette")
    code.emit(0x8A, 0x29, 0x3F, 0x8D, 0x07, 0x20, 0xCA)
    code.relative(0xD0, "palette")
    code.emit(0xA9, 0x80, 0x8D, 0x00, 0x20, 0xA9, 0x1E, 0x8D, 0x01, 0x20)
    code.label("main")
    code.absolute(0x4C, "main")

    code.label("nmi")
    code.emit(0xE6, 0x00, 0xA5, 0x00)  # INC $00; LDA $00
    code.emit(0x8D, 0x05, 0x20, 0xA9, 0x00, 0x8D, 0x05, 0x20)
    code.emit(0xA9, 0x02, 0x8D, 0x14, 0x40, 0x40)  # OAM DMA; RTI

    chr_data = bytearray(CHR_SIZE)
    for row in range(8):
        chr_data[row] = 0xAA if row & 1 else 0x55
        chr_data[row + 8] = 0x33 if row & 1 else 0xCC
    return code.finish(), bytes(chr_data)


def build_rom(kind: str) -> bytes:
    program, chr_data = cpu_workload() if kind == "cpu" else ppu_workload()
    prg = bytearray([0xEA] * PRG_SIZE)
    prg[: len(program)] = program
    reset = LOAD_ADDR
    # Locate RTI for NMI; the reset and IRQ vectors intentionally share reset/loop-safe code.
    nmi = LOAD_ADDR + program.rfind(b"\x40")
    prg[-6:] = bytes((nmi & 0xFF, nmi >> 8, reset & 0xFF, reset >> 8, reset & 0xFF, reset >> 8))
    header = b"NES\x1a" + bytes((1, 1, 0, 0)) + bytes(8)
    return header + prg + chr_data


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("kind", choices=("cpu", "ppu"))
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data = build_rom(args.kind)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)


if __name__ == "__main__":
    main()
