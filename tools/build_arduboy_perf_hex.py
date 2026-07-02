#!/usr/bin/env python3
"""Build small deterministic Arduboy HEX workloads for badge profiling."""

from __future__ import annotations

import argparse
import json
import zipfile
from pathlib import Path


def reg_bits(dst: int, src: int) -> int:
    return ((src & 0x10) << 5) | ((dst & 0x1F) << 4) | (src & 0x0F)


class Avr:
    def __init__(self) -> None:
        self.words: list[int] = []

    def emit(self, *words: int) -> None:
        self.words.extend(word & 0xFFFF for word in words)

    def ldi(self, reg: int, value: int) -> None:
        assert 16 <= reg <= 31
        self.emit(0xE000 | ((value & 0xF0) << 4) | ((reg - 16) << 4) | (value & 0x0F))

    def out(self, io_addr: int, reg: int) -> None:
        self.emit(0xB800 | ((io_addr & 0x30) << 5) | ((reg & 0x1F) << 4) | (io_addr & 0x0F))

    def in_(self, reg: int, io_addr: int) -> None:
        self.emit(0xB000 | ((io_addr & 0x30) << 5) | ((reg & 0x1F) << 4) | (io_addr & 0x0F))

    def sts(self, addr: int, reg: int) -> None:
        self.emit(0x9200 | ((reg & 0x1F) << 4), addr)

    def lds(self, reg: int, addr: int) -> None:
        self.emit(0x9000 | ((reg & 0x1F) << 4), addr)

    def add(self, dst: int, src: int) -> None:
        self.emit(0x0C00 | reg_bits(dst, src))

    def adc(self, dst: int, src: int) -> None:
        self.emit(0x1C00 | reg_bits(dst, src))

    def sub(self, dst: int, src: int) -> None:
        self.emit(0x1800 | reg_bits(dst, src))

    def eor(self, dst: int, src: int) -> None:
        self.emit(0x2400 | reg_bits(dst, src))

    def inc(self, reg: int) -> None:
        self.emit(0x9403 | ((reg & 0x1F) << 4))

    def mov(self, dst: int, src: int) -> None:
        self.emit(0x2C00 | reg_bits(dst, src))

    def cpse(self, dst: int, src: int) -> None:
        self.emit(0x1000 | reg_bits(dst, src))

    def ld_z(self, reg: int) -> None:
        self.emit(0x8000 | ((reg & 0x1F) << 4))

    def st_z_inc(self, reg: int) -> None:
        self.emit(0x9201 | ((reg & 0x1F) << 4))

    def sbiw(self, reg: int, value: int) -> None:
        assert reg in (24, 26, 28, 30) and 0 <= value < 64
        pair = (reg - 24) // 2
        self.emit(0x9700 | ((value & 0x30) << 2) | (pair << 4) | (value & 0x0F))

    def sbrc(self, reg: int, bit: int) -> None:
        self.emit(0xFC00 | ((reg & 0x1F) << 4) | (bit & 7))

    def brne(self, target_word: int) -> None:
        offset = target_word - len(self.words) - 1
        if not -64 <= offset <= 63:
            raise ValueError("BRNE target out of range")
        self.emit(0xF401 | ((offset & 0x7F) << 3))

    def lsr(self, reg: int) -> None:
        self.emit(0x9406 | ((reg & 0x1F) << 4))

    def rjmp(self, target_word: int) -> None:
        offset = target_word - len(self.words) - 1
        if not -2048 <= offset <= 2047:
            raise ValueError("RJMP target out of range")
        self.emit(0xC000 | (offset & 0x0FFF))

    def bytes(self) -> bytes:
        result = bytearray()
        for word in self.words:
            result.extend((word & 0xFF, word >> 8))
        return bytes(result)


def initialize_display(code: Avr) -> None:
    # PORTD bit 6 is display CS and bit 4 is data/command.
    code.ldi(16, 0x50)
    code.out(0x0A, 16)  # DDRD
    code.ldi(16, 0x00)
    code.out(0x0B, 16)  # PORTD: command, selected
    code.ldi(16, 0x50)
    code.out(0x2C, 16)  # SPCR: enabled, master
    code.ldi(16, 0xAF)
    code.out(0x2E, 16)  # SSD1306 display on
    code.ldi(16, 0x10)
    code.out(0x0B, 16)  # PORTD: data, selected
    code.ldi(16, 0xFF)
    code.out(0x2E, 16)  # Make the boot watchdog see visible pixels.


def build(kind: str) -> bytes:
    code = Avr()
    initialize_display(code)

    if kind == "cpu":
        code.ldi(17, 0x13)
        code.ldi(18, 0x57)
        code.ldi(19, 0xA9)
        loop = len(code.words)
        code.add(17, 18)
        code.adc(18, 19)
        code.eor(19, 17)
        code.sub(17, 19)
        code.inc(18)
        code.lsr(19)
        code.rjmp(loop)
    elif kind == "spi":
        code.ldi(17, 0x01)
        loop = len(code.words)
        code.out(0x2E, 17)
        code.inc(17)
        code.rjmp(loop)
    elif kind == "timer":
        # Arduboy2 BeepPin1-compatible Timer3A CTC configuration.
        code.ldi(16, 0x40)
        code.sts(0x90, 16)  # TCCR3A: toggle speaker pin on compare
        code.ldi(16, 0x0A)
        code.sts(0x91, 16)  # TCCR3B: CTC, divide by 8
        code.ldi(16, 0x00)
        code.sts(0x99, 16)
        code.ldi(16, 0x40)
        code.sts(0x98, 16)  # OCR3A
        loop = len(code.words)
        code.lds(17, 0x94)  # TCNT3L forces observable lazy-timer updates.
        code.eor(18, 17)
        code.rjmp(loop)
    elif kind == "graphics":
        # Canonical Arduboy2 fillScreen loop: four stores per iteration.
        code.ldi(30, 0x00)
        code.ldi(31, 0x01)
        code.ldi(17, 0xA5)
        code.eor(18, 18)
        clear_loop = len(code.words)
        for _ in range(4):
            code.st_z_inc(17)
        code.inc(18)
        code.brne(clear_loop)

        # Canonical Arduboy2 paintScreen loop. X counts two ticks per byte.
        code.ldi(30, 0x00)
        code.ldi(31, 0x01)
        code.eor(24, 24)
        code.ldi(26, 0x00)
        code.ldi(27, 0x08)
        display_loop = len(code.words)
        code.ld_z(0)
        code.out(0x2E, 0)
        code.cpse(24, 1)
        code.mov(0, 1)
        delay_loop = len(code.words)
        code.sbiw(26, 1)
        code.sbrc(26, 0)
        code.rjmp(delay_loop)
        code.st_z_inc(0)
        code.brne(display_loop)
        done = len(code.words)
        code.rjmp(done)
    else:
        # Read the first byte at 0xffff00 from a 256-byte FX data image,
        # then send it to the OLED. This exercises wake, CS framing, address
        # transfer, MISO, and package data placement.
        code.ldi(16, 0x52)
        code.out(0x0A, 16)  # DDRD: OLED CS/DC and newer FX CS on PD1.
        code.out(0x0B, 16)  # Both devices deselected.
        code.ldi(16, 0x50)
        code.out(0x0B, 16)  # FX selected, OLED deselected.
        code.ldi(17, 0xAB)
        code.out(0x2E, 17)  # Release FX power-down.
        code.ldi(16, 0x52)
        code.out(0x0B, 16)
        code.ldi(16, 0x50)
        code.out(0x0B, 16)
        for value in (0x03, 0xFF, 0xFF, 0x00, 0x00):
            code.ldi(17, value)
            code.out(0x2E, 17)
        code.in_(18, 0x2E)
        code.ldi(16, 0x52)
        code.out(0x0B, 16)  # End FX command.
        code.ldi(16, 0x10)
        code.out(0x0B, 16)  # OLED selected in data mode.
        code.out(0x2E, 18)
        loop = len(code.words)
        code.rjmp(loop)

    return code.bytes()


def hex_record(address: int, record_type: int, data: bytes) -> str:
    body = bytes((len(data), address >> 8, address & 0xFF, record_type)) + data
    checksum = (-sum(body)) & 0xFF
    return ":" + (body + bytes((checksum,))).hex().upper()


def write_hex(path: Path, image: bytes) -> None:
    lines = [hex_record(addr, 0, image[addr : addr + 16]) for addr in range(0, len(image), 16)]
    lines.append(hex_record(0, 1, b""))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="ascii")


def write_fx_package(path: Path) -> None:
    image = build("fx")
    lines = [hex_record(addr, 0, image[addr : addr + 16]) for addr in range(0, len(image), 16)]
    lines.append(hex_record(0, 1, b""))
    fxdata = bytes((0xA5, 0x5A)) + bytes(254)
    path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as package:
        package.writestr("info.json", json.dumps({
            "schemaVersion": 4,
            "title": "DC32 FX benchmark",
            "binaries": [{
                "title": "DC32 FX benchmark",
                "filename": "arduboy_fx_perf.hex",
                "device": "ArduboyFX",
                "flashdata": "arduboy_fx_perf-data.bin",
            }],
        }))
        package.writestr("arduboy_fx_perf.hex", "\n".join(lines) + "\n")
        package.writestr("arduboy_fx_perf-data.bin", fxdata)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("kind", choices=("cpu", "spi", "timer", "graphics", "fx"))
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.kind == "fx" and args.output.suffix.lower() == ".arduboy":
        write_fx_package(args.output)
    else:
        write_hex(args.output, build(args.kind))


if __name__ == "__main__":
    main()
