#!/usr/bin/env python3
"""Static and artifact regression checks for the dedicated NES runtime."""

from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def require(text: str, needle: str, context: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing {context}: {needle}")


def reject(text: str, needle: str, context: str) -> None:
    if needle in text:
        raise AssertionError(f"unexpected {context}: {needle}")


def main() -> None:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    core = (ROOT / "src/nes/InfoNES.cpp").read_text(encoding="utf-8")
    rw = (ROOT / "src/nes/K6502_rw.h").read_text(encoding="utf-8")
    port = (ROOT / "src/nes/nes_port.cpp").read_text(encoding="utf-8")
    app = (ROOT / "src/apps/nes_app.c").read_text(encoding="utf-8")
    pico = (ROOT / "src/pico.h").read_text(encoding="utf-8")

    nes_sources = cmake.split("set(NES_RUNTIME_SOURCES", 1)[1].split(")", 1)[0]
    reject(nes_sources, "InfoNES_pAPU.cpp", "APU source in NES target")
    require(cmake, "NES_FASTCODE_ENABLED=1", "NES fast-code target definition")
    require(core, "FrameSkip = 1;", "half-rate renderer")
    require(core, "InfoNES_FrameBoundary();", "full-rate scheduler")
    require(core, "if (MapperNo == 4)", "mapper 4 direct HSync")
    reject(core, "MapperPPU(PATTBL", "no-op per-tile mapper callback")
    reject(core, "InfoNES_pAPU", "runtime APU calls")
    reject(rw, "pAPUSoundRegs", "APU event generation")
    require(rw, "only the existing frame IRQ is visible", "muted APU status contract")
    require(port, "mFrame = mPool + NES_SAVE_RAM_SIZE;", "cartridge SRAM frame buffer")
    require(port, "NES_SAVE_RAM_SIZE + NES_DISP_WIDTH * NES_SAFE_HEIGHT", "SRAM layout assertion")
    require(port, "p95=%u", "95th-percentile profiler output")
    require(port, "if (mLedsTick)", "LED animation service")
    require(app, "host ? host->ledsTick : 0", "host LED callback wiring")
    require(pico, 'section(".fastcode.', "SRAM code section")

    with tempfile.TemporaryDirectory() as temp:
        for kind in ("cpu", "ppu"):
            output = Path(temp) / f"nes_{kind}_perf.nes"
            subprocess.run(
                [sys.executable, str(ROOT / "tools/build_nes_perf_rom.py"), kind, str(output)],
                check=True,
            )
            data = output.read_bytes()
            if len(data) != 16 + 16 * 1024 + 8 * 1024:
                raise AssertionError(f"bad {kind} benchmark ROM size: {len(data)}")
            if data[:4] != b"NES\x1a" or data[4:6] != bytes((1, 1)):
                raise AssertionError(f"bad {kind} benchmark ROM header")

    print("NES port checks passed")


if __name__ == "__main__":
    main()
