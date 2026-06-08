#!/usr/bin/env python3
"""Static and artifact checks for SD-loaded DCAPP binaries."""

from __future__ import annotations

import binascii
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER_FMT = "<IHH" + "I" * 14 + "32sI" + "I" * 39
HEADER_SIZE = 256
MAGIC = 0x50414344
ABI = 2
LOAD_ADDR = 0x10080000
APP_RAM_START = 0x2005F000
APP_RAM_SIZE = 0x14000
APP_RAM_END = APP_RAM_START + APP_RAM_SIZE
WORKSPACE_WRAM_SIZE = 0x8000
WORKSPACE_VRAM_SIZE = 0x4000
QSPI_APP_CACHE_SIZE = 0x40000
BADUSB_SCRATCH_MIN = 2048 + 2048 + 128
IMAGE_TRANSFER_MIN = 32768
PICOWARE_APP_SCRATCH_MIN = 4096
APPS = {
    "gb.DC32": 1,
    "nes.DC32": 2,
    "arduboy.DC32": 3,
    "ir.DC32": 100,
    "image.DC32": 101,
    "music.DC32": 102,
    "badusb.DC32": 103,
    "autoclicker.DC32": 104,
    "gamepad.DC32": 105,
    "pong.DC32": 200,
    "tetris.DC32": 201,
    "arkanoid.DC32": 202,
    "flappy.DC32": 203,
    "labyrinth.DC32": 204,
    "trex.DC32": 205,
    "starfield.DC32": 220,
    "spiro.DC32": 221,
    "cube.DC32": 222,
}
GAME_APPS = {"gb.DC32", "nes.DC32", "arduboy.DC32"}
PICOWARE_APPS = {
    "pong.DC32",
    "tetris.DC32",
    "arkanoid.DC32",
    "flappy.DC32",
    "labyrinth.DC32",
    "trex.DC32",
    "starfield.DC32",
    "spiro.DC32",
    "cube.DC32",
}


def expect(name: str, condition: bool) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def range_in_app_ram(addr: int, size: int) -> bool:
    return size == 0 or (APP_RAM_START <= addr <= APP_RAM_END and addr + size <= APP_RAM_END)


def align_up(value: int, align: int) -> int:
    return (value + align - 1) & ~(align - 1)


def active_scratch(data_addr: int, data_size: int, bss_addr: int, bss_size: int) -> tuple[int, int]:
    used_end = APP_RAM_START
    if data_size:
        used_end = max(used_end, data_addr + data_size)
    if bss_size:
        used_end = max(used_end, bss_addr + bss_size)
    scratch_start = align_up(used_end, 8)
    scratch_end = APP_RAM_END
    return scratch_start, max(0, scratch_end - scratch_start)


def active_wram_size(scratch_size: int) -> int:
    return min(scratch_size, WORKSPACE_WRAM_SIZE)


def active_vram_size(scratch_size: int) -> int:
    if scratch_size <= WORKSPACE_WRAM_SIZE:
        return 0
    return min(scratch_size - WORKSPACE_WRAM_SIZE, WORKSPACE_VRAM_SIZE)


def check_loader_source() -> None:
    text = (ROOT / "src" / "dcApp.c").read_text(encoding="utf-8")
    qspi_h = (ROOT / "src" / "qspi.h").read_text(encoding="utf-8")
    qspi_c = (ROOT / "src" / "qspi2350.c").read_text(encoding="utf-8")
    workspace = (ROOT / "src" / "toolWorkspace.c").read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    run_start = text.find("static enum DcAppResult dcAppRunLoadedById")
    run_entry = text.find("ret = entry(&mHostApi, args)", run_start)
    run_sync = text.find("dcAppPrvSyncExecutableCache();", run_start)
    verify_start = text.find("if (!dcAppPrvVerifyCachedImage(&hdr, runtime))")
    verify_sync = text.find("dcAppPrvSyncExecutableCache();", verify_start)
    flash_write_start = qspi_c.find("flashWrite")
    xip_mode_restore = qspi_c.find("flashPrvEnterXipMode();", flash_write_start)
    flash_write_sync = qspi_c.find("flashSyncExecutableRange(xipAddr, largerSize);", flash_write_start)

    expect("loader checks magic/header", "hdr->magic != DCAPP_MAGIC" in text and "hdr->headerSize != DCAPP_HEADER_SIZE" in text)
    expect("loader checks ABI version", "hdr->abiVersion != DCAPP_ABI_VERSION" in text)
    expect("loader checks image overflow", "hdr->imageSize > QSPI_APP_CACHE_SIZE" in text)
    expect("loader checks CRC", "crc != hdr.crc32" in text)
    expect("loader has cache-hit skip", "dcAppPrvCachedBuildMatches" in text and "buildId" in text)
    expect("loader erases stale cached image span", "dcAppPrvCachedEraseSpan" in text and "QSPI_APP_CACHE_SIZE" in text)
    expect("loader verifies flash image after write", "dcAppPrvVerifyCachedImage" in text and "App flash verify failed" in text)
    expect("loader verifies app cache through uncached XIP", "dcAppPrvCachedHeader" in text and "flashUncachedPtr(QSPI_APP_CACHE_START" in text)
    expect("loader syncs full app cache for execution", "flashSyncExecutableRange(QSPI_APP_CACHE_START, QSPI_APP_CACHE_SIZE)" in text)
    expect("loader syncs rewritten image after verify", verify_sync != -1)
    expect("loader syncs app cache before app entry", run_sync != -1 and run_entry != -1 and run_sync < run_entry)
    expect("loader clears active app context before load and run", text.count("dcAppPrvClearActiveAppContext();") >= 3)
    expect("QSPI exposes uncached XIP reads", "flashUncachedPtr" in qspi_h and "XIP_NOCACHE_NOALLOC_BASE" in qspi_c)
    expect("QSPI exposes XIP cache flush helper", "flashFlushXipCacheRange" in qspi_h and "XIP_MAINTENANCE_BASE" in qspi_c and "QSPI_XIP_MAINT_INVALIDATE" in qspi_c)
    expect("QSPI exposes executable sync helper", "flashSyncExecutableRange" in qspi_h and "SCB->ICIALLU" in qspi_c and "SCB->BPIALL" in qspi_c)
    expect("flash writes sync executable cache after XIP restore", xip_mode_restore != -1 and flash_write_sync != -1 and xip_mode_restore < flash_write_sync)
    expect("loader computes active app scratch", "dcAppPrvBeginActiveRamContext" in text and "dcAppGetActiveScratch" in text)
    expect("tool workspace remaps active app WRAM", "toolWorkspacePrvActiveAppSpan" in workspace and "dcAppGetActiveScratch(NULL)" in workspace)
    expect("app abort callback is linker-rooted", "-Wl,--undefined=dcAppAbort" in cmake)
    expect("app refresh callback is linker-rooted", "-Wl,--undefined=dcAppRefreshDisplayOptions" in cmake)
    expect("apps relink when host symbols change", "LINK_DEPENDS ${DCAPP_HOST_SYMBOLS}" in cmake)
    for filename, runtime in APPS.items():
        output = filename.removesuffix(".DC32")
        expect(f"{filename} has CMake dcapp target", f" {output} {runtime}" in cmake)
        expect(f"{filename} is in SD apps package", f'"{filename}"' in cmake or f'"{filename}"' in (ROOT / "tools" / "build_sd_zip.py").read_text(encoding="utf-8"))


def check_artifacts() -> None:
    apps_dir = ROOT / "build" / "apps"

    expect("app artifacts directory exists", apps_dir.is_dir())
    for name, runtime in APPS.items():
        path = apps_dir / name
        expect(f"{name} exists", path.is_file())
        data = path.read_bytes()
        expect(f"{name} has a full header", len(data) >= HEADER_SIZE)
        fields = struct.unpack(HEADER_FMT, data[:HEADER_SIZE])
        magic, header_size, abi = fields[:3]
        runtime_id = fields[3]
        load_addr = fields[5]
        image_size = fields[6]
        data_load_offset = fields[7]
        data_addr = fields[8]
        data_size = fields[9]
        bss_addr = fields[10]
        bss_size = fields[11]
        entry_offset = fields[12]
        abort_offset = fields[13]
        refresh_offset = fields[14]
        app_ram_start = fields[15]
        app_ram_size = fields[16]
        crc32 = fields[18]
        scratch_start, scratch_size = active_scratch(data_addr, data_size, bss_addr, bss_size)

        expect(f"{name} magic", magic == MAGIC)
        expect(f"{name} header size", header_size == HEADER_SIZE)
        expect(f"{name} ABI", abi == ABI)
        expect(f"{name} runtime", runtime_id == runtime)
        expect(f"{name} load addr", load_addr == LOAD_ADDR)
        expect(f"{name} image size", image_size == len(data))
        expect(f"{name} image fits app cache", image_size <= QSPI_APP_CACHE_SIZE)
        expect(f"{name} entry offset", HEADER_SIZE <= (entry_offset & ~1) < image_size)
        expect(f"{name} app RAM descriptor", app_ram_start == APP_RAM_START and app_ram_size == APP_RAM_SIZE)
        expect(f"{name} data RAM range", range_in_app_ram(data_addr, data_size))
        expect(f"{name} bss RAM range", range_in_app_ram(bss_addr, bss_size))
        expect(f"{name} data load range", data_size == 0 or HEADER_SIZE <= data_load_offset <= image_size - data_size)
        expect(f"{name} active scratch starts after data", data_size == 0 or scratch_start >= data_addr + data_size)
        expect(f"{name} active scratch starts after bss", bss_size == 0 or scratch_start >= bss_addr + bss_size)
        expect(f"{name} active WRAM and VRAM do not overlap", active_wram_size(scratch_size) + active_vram_size(scratch_size) <= scratch_size)
        if name in GAME_APPS:
            expect(f"{name} abort offset", HEADER_SIZE <= (abort_offset & ~1) < image_size)
        if refresh_offset:
            expect(f"{name} refresh offset", HEADER_SIZE <= (refresh_offset & ~1) < image_size)
        if name == "badusb.DC32":
            expect("BadUSB active WRAM scratch", active_wram_size(scratch_size) >= BADUSB_SCRATCH_MIN)
        if name == "image.DC32":
            expect("Image active transfer scratch", active_wram_size(scratch_size) >= IMAGE_TRANSFER_MIN)
        if name in PICOWARE_APPS:
            expect(f"{name} Picoware app active WRAM scratch", active_wram_size(scratch_size) >= PICOWARE_APP_SCRATCH_MIN)
            expect(f"{name} abort offset", HEADER_SIZE <= (abort_offset & ~1) < image_size)
        expect(f"{name} CRC", (binascii.crc32(data[HEADER_SIZE:]) & 0xFFFFFFFF) == crc32)


def main() -> int:
    check_loader_source()
    check_artifacts()
    print("DCAPP static tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
