#!/usr/bin/env python3
"""Static and artifact checks for SD-loaded DCAPP binaries."""

from __future__ import annotations

import binascii
import re
import subprocess
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER_FMT = "<IHH" + "I" * 14 + "32sI" + "I" * 39
HEADER_SIZE = 256
MAGIC = 0x50414344
ABI = 4
LOAD_ADDR = 0x10080000
DCAPP_IMAGE_FLAG_LARGE_XIP = 0x00000001
DCAPP_CONTRACT_MAGIC = 0x43444332
DCAPP_CONTRACT_HASH_WORDS = 4
APP_RAM_START = 0x2005F000
APP_RAM_SIZE = 0x14000
APP_RAM_END = APP_RAM_START + APP_RAM_SIZE
WORKSPACE_WRAM_SIZE = 0x8000
WORKSPACE_VRAM_SIZE = 0x4000
QSPI_APP_CACHE_SIZE = 0x40000
QSPI_APP_CACHE_START = 0x10080000
QSPI_SETTINGS_START = 0x100C0000
QSPI_SETTINGS_LEN = 0x1F000
QSPI_FILENAME_START = 0x100DF000
QSPI_FILENAME_MAXLEN = 0x1000
QSPI_RAM_COPY_START = 0x100E0000
QSPI_RAM_SIZE_MAX = 0x10000
QSPI_ROM_START = 0x10100000
QSPI_ROM_SIZE_MAX = 0x300000
DOOM_SHORTPTR_BASE = 0x20030000
DOOM_SHORTPTR_END = DOOM_SHORTPTR_BASE + 0x40000
DOOM_WHX_PATH = ROOT / "third_party" / "rp2040-doom" / "doom1.whx"
DOOM_WHX_LEGACY_VPATCH_COUNT = 384
DOOM_WHX_CURRENT_VPATCH_COUNT = 385
BADUSB_SCRATCH_MIN = 2048 + 2048 + 128
IMAGE_TRANSFER_MIN = 32768
PICOWARE_APP_SCRATCH_MIN = 4096
OPENJAZZ_ACTIVE_SCRATCH_MIN = 56 * 1024
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
    "doom.DC32": 206,
    "chips.DC32": 207,
    "scorch.DC32": 208,
    "pipe.DC32": 209,
    "cave.DC32": 210,
    "sokoban.DC32": 211,
    "openjazz.DC32": 212,
    "starfield.DC32": 220,
    "spiro.DC32": 221,
    "cube.DC32": 222,
}
RESERVED_PERIOD_IDS = {}
LARGE_XIP_APPS = {"chips.DC32", "scorch.DC32", "pipe.DC32", "cave.DC32"}
GAME_APPS = {"gb.DC32", "nes.DC32", "arduboy.DC32"}
PICOWARE_APPS = {
    "pong.DC32",
    "tetris.DC32",
    "flappy.DC32",
    "labyrinth.DC32",
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


def ranges_overlap(start_a: int, size_a: int, start_b: int, size_b: int) -> bool:
    return start_a < start_b + size_b and start_b < start_a + size_a


def build_contract_words() -> tuple[int, ...]:
    path = ROOT / "build" / "dcapp_build_contract.h"
    expect("DCAPP build contract header exists", path.is_file())
    match = re.search(r"^#define\s+DCAPP_BUILD_CONTRACT_WORDS\s+(.+)$", path.read_text(encoding="ascii"), re.MULTILINE)
    expect("DCAPP build contract words are generated", match is not None)
    words = tuple(int(part.rstrip("uU"), 0) for part in match.group(1).split(","))
    expect("DCAPP build contract word count", len(words) == DCAPP_CONTRACT_HASH_WORDS)
    return words


def doom_symbol_address(symbol: str) -> int:
    elf_path = ROOT / "build" / "dcapp_doom"
    expect("DOOM ELF exists", elf_path.is_file())
    try:
        result = subprocess.run(
            ["arm-none-eabi-nm", "-n", str(elf_path)],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
    except FileNotFoundError:
        raise SystemExit("FAIL: arm-none-eabi-nm is available for DOOM symbol checks")

    expect("arm-none-eabi-nm reads DOOM ELF", result.returncode == 0)
    for line in result.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[-1] == symbol:
            return int(parts[0], 16)
    raise SystemExit(f"FAIL: DOOM symbol {symbol} exists")


def check_doom_shortptr_symbols() -> None:
    thinkercap = doom_symbol_address("thinkercap")
    expect(
        "DOOM thinkercap stays in shortptr window",
        DOOM_SHORTPTR_BASE + 4 <= thinkercap < DOOM_SHORTPTR_END,
    )


def check_flash_layout() -> None:
    app_cache_end = QSPI_APP_CACHE_START + QSPI_APP_CACHE_SIZE
    regions = {
        "settings": (QSPI_SETTINGS_START, QSPI_SETTINGS_LEN),
        "filename": (QSPI_FILENAME_START, QSPI_FILENAME_MAXLEN),
        "RAM copy": (QSPI_RAM_COPY_START, QSPI_RAM_SIZE_MAX),
        "ROM staging": (QSPI_ROM_START, QSPI_ROM_SIZE_MAX),
    }

    for label, (start, size) in regions.items():
        expect(
            f"app cache does not overlap {label}",
            not ranges_overlap(QSPI_APP_CACHE_START, QSPI_APP_CACHE_SIZE, start, size),
        )
    expect("app cache ends at settings start", app_cache_end <= QSPI_SETTINGS_START)


def whx_lump_span(data: bytes, lump_num: int) -> tuple[int, int]:
    num_lumps = struct.unpack_from("<I", data, 4)[0]
    info_table_ofs = struct.unpack_from("<I", data, 8)[0]

    expect("doom1.whx lump range", lump_num < num_lumps)
    raw0 = struct.unpack_from("<I", data, info_table_ofs + lump_num * 4)[0]
    raw1 = struct.unpack_from("<I", data, info_table_ofs + (lump_num + 1) * 4)[0]
    offset = raw0 & 0x00FFFFFF
    size = ((raw1 - raw0) & 0x00FFFFFF) - (raw0 >> 30)
    expect("doom1.whx lump span fits", offset <= len(data) and size <= len(data) - offset)
    return offset, size


def whx_named_lump(data: bytes, name: bytes) -> tuple[int, int, int]:
    num_lumps = struct.unpack_from("<I", data, 4)[0]
    info_table_ofs = struct.unpack_from("<I", data, 8)[0]
    named_count = struct.unpack_from("<H", data, 34)[0]
    names_ofs = 36 + (num_lumps + 1) * 4

    expect("doom1.whx directory fits", info_table_ofs + (num_lumps + 1) * 4 <= len(data))
    expect("doom1.whx names fit", names_ofs + named_count * 12 <= len(data))
    for i in range(named_count):
        entry_ofs = names_ofs + i * 12
        entry_name = data[entry_ofs : entry_ofs + 10].split(b"\0", 1)[0].lower()
        if entry_name == name:
            lump_num = struct.unpack_from("<H", data, entry_ofs + 10)[0]
            expect("doom1.whx named lump range", lump_num < num_lumps)
            offset, size = whx_lump_span(data, lump_num)
            return lump_num, offset, size
    raise SystemExit(f"FAIL: doom1.whx missing {name.decode('ascii')}")


def doom_vpatch_index(name: str) -> int:
    text = (ROOT / "third_party" / "rp2040-doom" / "src" / "whddata.h").read_text(encoding="utf-8")
    start = text.index("#define VPATCH_LIST")
    end = text.index("#define NAMED_FLAT_LIST", start)
    entries: list[str] = []

    for raw_line in text[start:end].splitlines()[1:]:
        line = raw_line.split("\\", 1)[0].strip().rstrip(",").strip()

        if line == "VPATCH_NAME_INVALID":
            entries.append("INVALID")
        elif line.startswith("VPATCH_NAME(") and line.endswith(")"):
            entries.append(line.removeprefix("VPATCH_NAME(").removesuffix(")"))
    expect("doom vpatch enum parsed", len(entries) == DOOM_WHX_CURRENT_VPATCH_COUNT)
    return entries.index(name)


def whx_vpatch_has_palette_seed(data: bytes, lump_num: int) -> bool:
    offset, size = whx_lump_span(data, lump_num)

    expect("doom1.whx vpatch header fits", size >= 4)
    return bool(data[offset + 2] and (data[offset + 3] & 1))


def check_doom_whx() -> None:
    expect("doom1.whx exists", DOOM_WHX_PATH.is_file())
    data = DOOM_WHX_PATH.read_bytes()
    whx_size = struct.unpack_from("<I", data, 12)[0]
    source_name = data[20:34].split(b"\0", 1)[0]
    _, pstart_offset, pstart_size = whx_named_lump(data, b"p_start")
    pstart_entries = pstart_size // 2
    pstart = [
        struct.unpack_from("<H", data, pstart_offset + i * 2)[0]
        for i in range(pstart_entries)
    ]
    stbar_idx = doom_vpatch_index("STBAR")
    m_bright_idx = doom_vpatch_index("M_BRIGHT")
    stcfn033_idx = doom_vpatch_index("STCFN033")

    expect("doom1.whx magic", data[:4] == b"IWHX")
    expect("doom1.whx size field", whx_size == len(data))
    expect("doom1.whx source WAD metadata", source_name == b"DOOM1.WAD")
    expect("doom1.whx P_START entries are 16-bit", pstart_size % 2 == 0)
    expect("doom1.whx legacy vpatch count", pstart_entries == DOOM_WHX_LEGACY_VPATCH_COUNT)
    expect("doom vpatch gap is before STCFN033", m_bright_idx + 1 == stcfn033_idx)
    expect("doom1.whx keeps pre-gap vpatches aligned", whx_vpatch_has_palette_seed(data, pstart[stbar_idx]))
    expect("doom1.whx omits M_BRIGHT vpatch", whx_vpatch_has_palette_seed(data, pstart[stcfn033_idx - 1]))
    expect("doom1.whx STCFN033 raw slot is shifted", not whx_vpatch_has_palette_seed(data, pstart[stcfn033_idx]))


def check_loader_source() -> None:
    text = (ROOT / "src" / "dcApp.c").read_text(encoding="utf-8")
    dcapp_h = (ROOT / "src" / "dcApp.h").read_text(encoding="utf-8")
    qspi_h = (ROOT / "src" / "qspi.h").read_text(encoding="utf-8")
    qspi_c = (ROOT / "src" / "qspi2350.c").read_text(encoding="utf-8")
    workspace = (ROOT / "src" / "toolWorkspace.c").read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    sd_zip = (ROOT / "tools" / "build_sd_zip.py").read_text(encoding="utf-8")
    doom_platform = (ROOT / "src" / "apps" / "doom" / "doom_dc32_platform.c").read_text(encoding="utf-8")
    doom_app = (ROOT / "src" / "apps" / "doom" / "doom_app.c").read_text(encoding="utf-8")
    doom_sound = (ROOT / "src" / "apps" / "doom" / "doom_dc32_sound.c").read_text(encoding="utf-8")
    run_start = text.find("static enum DcAppResult dcAppRunLoadedById")
    run_entry = text.find("ret = entry(&mHostApi, args)", run_start)
    run_sync = text.find("dcAppPrvSyncExecutableImage(&mLoadedHeader);", run_start)
    verify_start = text.find("if (!dcAppPrvVerifyCachedImage(&hdr, runtime))")
    verify_sync = text.find("dcAppPrvSyncExecutableImage(&hdr);", verify_start)
    flash_write_start = qspi_c.find("flashWrite")
    xip_mode_restore = qspi_c.find("flashPrvEnterXipMode();", flash_write_start)
    flash_write_sync = qspi_c.find("flashSyncExecutableRange(xipAddr, largerSize);", flash_write_start)

    expect("loader checks magic/header", "hdr->magic != DCAPP_MAGIC" in text and "hdr->headerSize != DCAPP_HEADER_SIZE" in text)
    expect("loader checks ABI version", "hdr->abiVersion != DCAPP_ABI_VERSION" in text)
    expect("loader exposes LED tick host callback", ".ledsTick = badgeLedsTick" in text and "void (*ledsTick)(void)" in dcapp_h)
    expect("loader checks DCAPP build contract", "dcAppPrvHeaderHasCurrentBuildContract" in text and "DCAPP_CONTRACT_MAGIC" in text)
    expect("loader reports stale SD apps clearly", "does not match this firmware" in text and "Update /APPS" in text)
    expect("loader checks image overflow", "hdr->imageSize > imageLimit" in text)
    expect("loader rejects unknown image flags", "hdr->flags & ~DCAPP_IMAGE_FLAG_LARGE_XIP" in text)
    expect("loader defines large XIP flag", "DCAPP_IMAGE_FLAG_LARGE_XIP" in (ROOT / "src" / "dcApp.h").read_text(encoding="utf-8"))
    expect("loader validates large XIP load region", "hdr->loadAddr != QSPI_ROM_START" in text and "QSPI_ROM_SIZE_MAX" in text)
    expect("loader validates small app cache region", "hdr->loadAddr != QSPI_APP_CACHE_START" in text and "QSPI_APP_CACHE_SIZE" in text)
    expect("loader checks CRC", "crc != hdr.crc32" in text)
    expect("loader has cache-hit skip", "dcAppPrvCachedBuildMatches" in text and "buildId" in text)
    expect("loader erases stale cached image span", "dcAppPrvCachedEraseSpan" in text and "imageLimit" in text)
    expect("loader verifies flash image after write", "dcAppPrvVerifyCachedImage" in text and "App flash verify failed" in text)
    expect("loader verifies app cache through uncached XIP", "dcAppPrvCachedHeaderAt" in text and "flashUncachedPtr(loadAddr)" in text)
    expect("loader writes to declared load address", "flashWrite(hdr.loadAddr + pos" in text and "flashWrite(hdr.loadAddr, eraseSize" in text)
    expect("loader syncs executable image region", "dcAppPrvSyncExecutableImage" in text and "flashSyncExecutableRange(hdr->loadAddr, imageLimit)" in text)
    expect("loader syncs rewritten image after verify", verify_sync != -1)
    expect("loader syncs app cache before app entry", run_sync != -1 and run_entry != -1 and run_sync < run_entry)
    expect("loader clears active app context before load and run", text.count("dcAppPrvClearActiveAppContext();") >= 3)
    expect("loader stops PWM audio around DC app execution", text.count("audioPwmStop();") >= 2)
    expect("QSPI exposes uncached XIP reads", "flashUncachedPtr" in qspi_h and "XIP_NOCACHE_NOALLOC_BASE" in qspi_c)
    expect("QSPI exposes XIP cache flush helper", "flashFlushXipCacheRange" in qspi_h and "XIP_MAINTENANCE_BASE" in qspi_c and "QSPI_XIP_MAINT_INVALIDATE" in qspi_c)
    expect("QSPI exposes executable sync helper", "flashSyncExecutableRange" in qspi_h and "SCB->ICIALLU" in qspi_c and "SCB->BPIALL" in qspi_c)
    expect("QSPI program and erase waits are bounded", all(token in qspi_c for token in ("QSPI_PROGRAM_TIMEOUT_USEC", "QSPI_ERASE_TIMEOUT_USEC", "timer0_hw->timerawl", "flashPrvBusyWait(uint32_t timeoutUsec)")))
    expect("QSPI timeout resets the flash command state", all(token in qspi_c for token in ("flashPrvCommand(0x66)", "flashPrvCommand(0x99)", "flashPrvReset();", "return false;")))
    expect("QSPI write propagates failures after restoring XIP", "for (curAddr = addr, i = 0; ret && i < writeSz" in qspi_c and "flashPrvEnterXipMode();" in qspi_c and "return ret;" in qspi_c)
    expect("flash writes sync executable cache after XIP restore", xip_mode_restore != -1 and flash_write_sync != -1 and xip_mode_restore < flash_write_sync)
    expect("loader computes active app scratch", "dcAppPrvBeginActiveRamContext" in text and "dcAppGetActiveScratch" in text)
    expect("tool workspace remaps active app WRAM", "toolWorkspacePrvActiveAppSpan" in workspace and "dcAppGetActiveScratch(NULL)" in workspace)
    expect("app abort callback is linker-rooted", "-Wl,--undefined=dcAppAbort" in cmake)
    expect("app refresh callback is linker-rooted", "-Wl,--undefined=dcAppRefreshDisplayOptions" in cmake)
    expect("apps relink when host symbols change", "LINK_DEPENDS ${DCAPP_HOST_SYMBOLS}" in cmake)
    expect("large DCAPP linker exists", (ROOT / "src" / "linker_dcapp_large.lkr").read_text(encoding="utf-8").count("0x10100000") >= 1)
    expect("CMake can stamp large XIP DCAPPs", "LARGE_XIP" in cmake and "--flags ${DCAPP_TARGET_FLAGS}" in cmake and "DCAPP_LARGE_LINKER_SCRIPT" in cmake)
    make_dcapp = (ROOT / "tools" / "make_dcapp.py").read_text(encoding="utf-8")
    expect("make_dcapp stamps flags", "parser.add_argument(\"--flags\"" in make_dcapp and "args.flags" in make_dcapp)
    expect("make_dcapp stamps build contract", "--contract-header" in make_dcapp and "DCAPP_CONTRACT_MAGIC" in make_dcapp)
    expect("CMake generates DCAPP build contract", "gen_dcapp_build_contract.py" in cmake and "dcapp_build_contract" in cmake)
    for filename, runtime in APPS.items():
        output = filename.removesuffix(".DC32")
        expect(f"{filename} has CMake dcapp target", f" {output} {runtime}" in cmake)
        expect(f"{filename} is in SD apps package", f'"{filename}"' in cmake or f'"{filename}"' in sd_zip)
    ui_c = (ROOT / "src" / "ui.c").read_text(encoding="utf-8")
    for symbol, app_id in RESERVED_PERIOD_IDS.items():
        expect(f"{symbol} is assigned", f"{symbol} = {app_id}" in dcapp_h)
    for filename in ():
        expect(f"{filename} is not built", filename.removesuffix(".DC32") not in cmake)
        expect(f"{filename} is not packaged", f'"{filename}"' not in sd_zip)
    expect("Cave fake data is not packaged", "cave.dat" not in sd_zip and "CAVE_DATA_SOURCE" not in sd_zip)
    expect("Period port README is packaged", "README-period-ports.txt" in sd_zip)
    for label in ():
        expect(f"{label} is not menu-enabled before faithful port", label not in ui_c)
    expect("Chip's Challenge faithful port is menu-enabled", '"Chip\'s Challenge"' in ui_c and '"chips.DC32"' in sd_zip)
    expect("Chip's Challenge Tile World pack is packaged", '"APPS/chips-tworld.pak"' in sd_zip and "CHIPS_TWORLD_PACK_SOURCE" in sd_zip)
    expect("Scorched Earth faithful port is menu-enabled", '"Scorched Earth"' in ui_c and '"scorch.DC32"' in sd_zip)
    expect("Scorched Earth xscorch pack is packaged", '"APPS/scorch-xscorch.pak"' in sd_zip and "XSCORCH_PACK_SOURCE" in sd_zip)
    expect("Pipe Dream faithful port is menu-enabled", '"Pipe Dream"' in ui_c and '"pipe.DC32"' in sd_zip)
    expect("Pipe Dream PipeDreamer pack is packaged", '"APPS/pipe-pipedreamer.pak"' in sd_zip and "PIPEDREAMER_PACK_SOURCE" in sd_zip)
    expect("Cave Story faithful port is menu-enabled", '"Cave Story"' in ui_c and '"cave.DC32"' in sd_zip)
    expect("Cave Story user data is not packaged", '"APPS/cave.pak"' not in sd_zip and "cave.dat" not in sd_zip)
    expect("Sokoban faithful port is menu-enabled", '"Sokoban"' in ui_c and '"sokoban.DC32"' in sd_zip)
    expect("Sokoban XSokoban pack is packaged", '"APPS/sokoban-xsokoban.pak"' in sd_zip and "XSOKOBAN_PACK_SOURCE" in sd_zip)
    expect("DOOM WHX is packaged under APPS", '"APPS/doom1.whx"' in sd_zip and "rp2040-doom" in sd_zip)
    expect("DOOM shareware label is explicit", '"DOOM (shareware)"' in text and '"DOOM (shareware)"' in (ROOT / "src" / "ui.c").read_text(encoding="utf-8"))
    expect("DOOM converter is not packaged", "doom_whx_convert" not in sd_zip and "ROMS/DOOM" not in sd_zip)
    expect("DOOM accepts WHX M_BRIGHT vpatch gap", "DC32_WHD_OMITS_M_BRIGHT_VPATCH=1" in cmake)
    expect("DOOM repairs WHX M_BRIGHT vpatch gap", "VPATCH_M_BRIGHT" in cmake)
    expect("DOOM accepts current and legacy vpatch counts", "doom WHX vpatches %d expected %d/%d" in cmake)
    expect("DOOM trims local build paths from assertions", "-ffile-prefix-map=${CMAKE_BINARY_DIR}=build" in cmake)
    expect("DOOM no longer forces shareware demo mode", "DEMO1_ONLY=1" not in cmake)
    expect("DOOM opens APPS doom1.whx only", 'DOOM_DC32_WHX_PATH "/APPS/doom1.whx"' in (ROOT / "src" / "apps" / "doom" / "doom_dc32.h").read_text(encoding="utf-8") and "fatfsFileOpen(vol, DOOM_DC32_WHX_PATH" in doom_app)
    expect("DOOM does not scan ROMS/DOOM", "fatfsDirOpen" not in doom_app and "fatfsFileOpenWithLocator" not in doom_app and "ROMS/DOOM" not in doom_app)
    expect("DOOM requires shareware WHX source", "WHX source %s is not DOOM1.WAD" in doom_app)
    expect("DOOM staged identity includes size hash source", "doomDc32WhxSameIdentity" in doom_app and "hash" in doom_app and "sourceName" in doom_app)
    expect("DOOM rejects oversized WHX staging", "QSPI_ROM_SIZE_MAX" in doom_app and "exceeds" in doom_app)
    expect("DOOM requires P_START and E1M1", "WHX missing P_START" in doom_app and "WHX missing E1M1" in doom_app)
    expect("DOOM builds real intermission", "doom/wi_stuff.c" in cmake and "void WI_Start" not in (ROOT / "src" / "apps" / "doom" / "doom_dc32_stubs.c").read_text(encoding="utf-8"))
    expect("DOOM restores attract demo playback", "D_Dc32DeferDemoIfPresent" in cmake and "G_DeferedPlayDemo(name)" in cmake)
    expect("DOOM guards invalid demo sprite frames", "r_things_dc32.c" in cmake and "if (demoplayback)\\n            return;" in cmake and "psp->state->frame & FF_FRAMEMASK" in cmake)
    expect("DOOM guards missing demo screen pages", "pagename ? W_CheckNumForName(pagename) : -1" in cmake and "D_AdvanceDemo();" in cmake)
    expect("DOOM links real sound frontend", "i_sound.c" in cmake and "doom/s_sound.c" in cmake and "doom/sounds.c" in cmake)
    expect("DOOM ships with audio disabled", "DOOM_DC32_ENABLE_AUDIO=0" in cmake and "audio disabled in DC32 safe mode" in doom_sound)
    expect("DOOM keeps experimental audio behind guard", "#if DOOM_DC32_ENABLE_AUDIO" in doom_sound and "audioPwmPcmStart" in doom_sound and "audioPwmPcmWriteU8" in doom_sound)
    expect("DOOM guarded audio decodes WHX ADPCM SFX", "ADPCM_BLOCK_SIZE" in doom_sound and "data[0] != 0x03" in doom_sound and "data[1] != 0x80" in doom_sound)
    expect("DOOM disabled audio is not frame-pumped", "I_UpdateSound();" not in doom_platform and "doomDc32SoundStopAll();" in doom_app)
    expect("DOOM converts host ticks to microseconds", "doomDc32TicksToUsec(doomDc32HostTicks())" in doom_platform and "ticks % TICKS_PER_SECOND" in doom_platform)
    expect("DOOM converts host ticks to game tics", "doomDc32HostTicks() * TICRATE) / TICKS_PER_SECOND" in doom_platform)
    expect("DOOM converts host ticks to milliseconds", "doomDc32HostTicks() * 1000ull) / TICKS_PER_SECOND" in doom_platform)
    expect("DOOM maps Start to menu enter", "doomDc32PostKey(KEY_ENTER, (keys & KEY_BIT_START) != 0)" in doom_platform)
    expect("DOOM maps Select to menu escape", "doomDc32PostKey(KEY_ESCAPE, (keys & KEY_BIT_SEL) != 0)" in doom_platform)
    expect("DOOM maps B to use/open", "doomDc32PostKey(' ', (keys & KEY_BIT_B) != 0)" in doom_platform)
    expect("DOOM maps Start+Select to pause", "KEY_BIT_START | KEY_BIT_SEL" in doom_platform and "doomDc32PostKey(KEY_PAUSE, pauseDown)" in doom_platform)
    expect("DOOM B no longer maps to run", "doomDc32PostKey(KEY_RSHIFT, (keys & KEY_BIT_B)" not in doom_platform)
    expect("DOOM disables missing WHX brightness menu", "BRIGHTNESS_MENU 0 // DC32 WHX omits M_BRIGHT" in cmake)
    expect("DOOM guards invalid shared vpatch palettes", "vpatch_shared_palette(patch) >= NUM_SHARED_PALETTES" in cmake)
    expect("DOOM recovers corrupt patch decoder cache", "dc32_reset_patch_decoder_cache" in cmake and "header->size == 0" in cmake and "offset_or_inverse_slot = patch_offset_or_inverse_slot(patch_num)" in cmake)
    expect("DOOM presenter handles wipe video type", "case VIDEO_TYPE_WIPE:" in doom_platform and "doomDc32AdvanceWipe" in doom_platform)
    expect("DOOM presenter keeps overlay active list", "vpatchlists->vpatch_next[prev]" in doom_platform and "doomDc32DrawOverlayLine" in doom_platform)
    expect("DOOM receives screen orientation setting", "if (appId == DcAppIdDoom)" in (ROOT / "src" / "ui.c").read_text(encoding="utf-8") and "args.rotate = settings.rotation" in (ROOT / "src" / "ui.c").read_text(encoding="utf-8"))
    expect("DOOM applies screen orientation setting", "doomDc32Canvas.flipped = 1u" in doom_app and "args && args->rotate" in doom_app)


def check_artifacts() -> None:
    apps_dir = ROOT / "build" / "apps"
    contract_words = build_contract_words()

    expect("app artifacts directory exists", apps_dir.is_dir())
    for name, runtime in APPS.items():
        path = apps_dir / name
        expect(f"{name} exists", path.is_file())
        data = path.read_bytes()
        expect(f"{name} has a full header", len(data) >= HEADER_SIZE)
        fields = struct.unpack(HEADER_FMT, data[:HEADER_SIZE])
        magic, header_size, abi = fields[:3]
        runtime_id = fields[3]
        flags = fields[4]
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
        reserved = fields[19:]
        scratch_start, scratch_size = active_scratch(data_addr, data_size, bss_addr, bss_size)

        expect(f"{name} magic", magic == MAGIC)
        expect(f"{name} header size", header_size == HEADER_SIZE)
        expect(f"{name} ABI", abi == ABI)
        expect(f"{name} runtime", runtime_id == runtime)
        if name in LARGE_XIP_APPS:
            expect(f"{name} flags", flags == DCAPP_IMAGE_FLAG_LARGE_XIP)
            expect(f"{name} load addr", load_addr == QSPI_ROM_START)
            expect(f"{name} image fits large XIP region", image_size <= QSPI_ROM_SIZE_MAX)
        else:
            expect(f"{name} flags", flags == 0)
            expect(f"{name} load addr", load_addr == LOAD_ADDR)
            expect(f"{name} image fits app cache", image_size <= QSPI_APP_CACHE_SIZE)
        expect(f"{name} image size", image_size == len(data))
        expect(f"{name} build contract magic", reserved[0] == DCAPP_CONTRACT_MAGIC)
        expect(f"{name} build contract words", tuple(reserved[1 : 1 + DCAPP_CONTRACT_HASH_WORDS]) == contract_words)
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
        if name == "trex.DC32":
            expect("T-Rex remains below 256 KiB", image_size < QSPI_APP_CACHE_SIZE)
            expect("T-Rex abort offset", HEADER_SIZE <= (abort_offset & ~1) < image_size)
        if name == "arkanoid.DC32":
            expect("Arkanoid remains below 256 KiB", image_size < QSPI_APP_CACHE_SIZE)
            expect("Arkanoid abort offset", HEADER_SIZE <= (abort_offset & ~1) < image_size)
        if name == "sokoban.DC32":
            expect("Sokoban abort offset", HEADER_SIZE <= (abort_offset & ~1) < image_size)
        if name == "openjazz.DC32":
            expect("OpenJazz remains below 256 KiB", image_size < QSPI_APP_CACHE_SIZE)
            expect("OpenJazz reserves at least 56 KiB active scratch", scratch_size >= OPENJAZZ_ACTIVE_SCRATCH_MIN)
        if name == "chips.DC32":
            expect("Chip's Challenge abort offset", HEADER_SIZE <= (abort_offset & ~1) < image_size)
        if name == "scorch.DC32":
            expect("Scorched Earth abort offset", HEADER_SIZE <= (abort_offset & ~1) < image_size)
        if name == "pipe.DC32":
            expect("Pipe Dream abort offset", HEADER_SIZE <= (abort_offset & ~1) < image_size)
        if name == "cave.DC32":
            expect("Cave Story abort offset", HEADER_SIZE <= (abort_offset & ~1) < image_size)
        if name == "openjazz.DC32":
            expect("Jazz Jackrabbit abort offset", HEADER_SIZE <= (abort_offset & ~1) < image_size)
        expect(f"{name} CRC", (binascii.crc32(data[HEADER_SIZE:]) & 0xFFFFFFFF) == crc32)


def main() -> int:
    check_flash_layout()
    check_doom_whx()
    check_loader_source()
    check_artifacts()
    check_doom_shortptr_symbols()
    print("DCAPP static tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
