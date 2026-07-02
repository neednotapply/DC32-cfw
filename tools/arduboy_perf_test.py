#!/usr/bin/env python3
"""Regression checks for the optimized Ardens-based Arduboy runtime."""

from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MINIROGUE_SHA256 = "6029a7873f88bfc6ee91bf08303b3c1cd19505e268c37733a0670b38c7991ff0"


def require(text: str, needle: str, context: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing {context}: {needle}")


def require_absent(text: str, needle: str, context: str) -> None:
    if needle in text:
        raise AssertionError(f"unexpected {context}: {needle}")


def parse_hex(path: Path) -> bytes:
    image = bytearray()
    eof = False
    for raw in path.read_text(encoding="ascii").splitlines():
        record = bytes.fromhex(raw.removeprefix(":"))
        if len(record) < 5 or sum(record) & 0xFF:
            raise AssertionError(f"invalid Intel HEX record in {path.name}")
        count = record[0]
        address = (record[1] << 8) | record[2]
        record_type = record[3]
        if count != len(record) - 5:
            raise AssertionError(f"bad Intel HEX length in {path.name}")
        if record_type == 0:
            end = address + count
            if len(image) < end:
                image.extend(b"\xff" * (end - len(image)))
            image[address:end] = record[4 : 4 + count]
        elif record_type == 1:
            eof = True
        else:
            raise AssertionError(f"unexpected Intel HEX record type {record_type}")
    if not eof:
        raise AssertionError(f"missing Intel HEX EOF record in {path.name}")
    return bytes(image)


def verify_hot_symbols() -> None:
    elf = ROOT / "build" / "dcapp_arduboy"
    if not elf.exists():
        return
    result = subprocess.run(
        ["arm-none-eabi-nm", "-S", "--radix=x", str(elf)],
        check=True,
        capture_output=True,
        text=True,
    )
    hot_names = (
        "advance_cycleEv",
        "advance_untilEy",
        "arduboyPrvPresentFrame",
        "arduboyPrvCaptureSpiBulk",
        "instr_merged_fill4_inc_brne",
        "instr_merged_arduboy_display",
    )
    found = {name: False for name in hot_names}
    for line in result.stdout.splitlines():
        fields = line.split()
        if len(fields) < 4:
            continue
        address = int(fields[0], 16)
        if fields[-1].endswith("_veneer"):
            continue
        for name in hot_names:
            if name in fields[-1]:
                if not 0x2005F000 <= address < 0x20073000:
                    raise AssertionError(f"hot symbol is not in app SRAM: {fields[-1]} @ {address:#x}")
                found[name] = True
    missing = [name for name, present in found.items() if not present]
    if missing:
        raise AssertionError(f"missing hot symbols in Arduboy ELF: {missing}")


def verify_page_expansion() -> None:
    pages = bytes((0x81, 0x42, 0x24, 0x18, 0xE7, 0x5A, 0xA5, 0x3C))
    for com_scan_reverse in (False, True):
        old = []
        for output_x in range(40, 200):
            logical_y = 239 - output_x
            source_y = ((logical_y - 40) * 64) // 160
            if not com_scan_reverse:
                source_y = 63 - source_y
            old.append((pages[source_y >> 3] >> (source_y & 7)) & 1)
        for flipped in (False, True):
            expected = list(reversed(old)) if flipped else old
            actual = []
            for group in range(8):
                page = None
                for x in range(20):
                    output = group * 20 + x
                    original = 159 - output if flipped else output
                    source_y = ((159 - original) * 64) // 160
                    if not com_scan_reverse:
                        source_y = 63 - source_y
                    if page is None:
                        page = source_y >> 3
                    elif page != source_y >> 3:
                        raise AssertionError("expanded group crossed a source page")
                    bit = source_y & 7
                    actual.append((pages[page] >> bit) & 1)
            if actual != expected:
                raise AssertionError(
                    f"page expansion mismatch: com={com_scan_reverse} flipped={flipped}"
                )


def verify_minirogue(path: Path) -> None:
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != MINIROGUE_SHA256:
        raise AssertionError("MiniRogue HEX does not match the pinned upstream benchmark")
    image = parse_hex(path)
    prologue_words = (
        0x922F, 0x923F, 0x924F, 0x925F, 0x926F, 0x927F, 0x928F,
        0x929F, 0x92AF, 0x92BF, 0x92CF, 0x92DF, 0x92EF, 0x92FF,
        0x930F, 0x931F, 0x93CF, 0x93DF, 0xB7CD, 0xB7DE, 0x9761,
        0xB60F, 0x94F8, 0xBFDE, 0xBE0F, 0xBFCD, 0x012C, 0x011A,
        0x872B, 0x01FA, 0x9134, 0x9631, 0x9124,
    )
    signature = b"".join(word.to_bytes(2, "little") for word in prologue_words)
    byte_address = image.find(signature)
    if byte_address < 0 or byte_address & 1:
        raise AssertionError("MiniRogue ArdBitmap 2.0.3 signature was not found")
    target = byte_address // 2
    word = lambda offset: int.from_bytes(image[(target + offset) * 2 : (target + offset) * 2 + 2], "little")
    if word(0x1F5) != 0x9508 or word(0x124) != 0x01FB:
        raise AssertionError("MiniRogue ArdBitmap structural guards changed")
    low_word, high_word = word(0x122), word(0x123)
    low = ((low_word >> 4) & 0xF0) | (low_word & 0x0F)
    high = ((high_word >> 4) & 0xF0) | (high_word & 0x0F)
    if (-((high << 8) | low)) & 0xFFFF != 0x03DE:
        raise AssertionError("MiniRogue framebuffer address inference changed")

    sprites_words = (
        0x922F, 0x923F, 0x924F, 0x925F, 0x926F, 0x927F, 0x928F,
        0x929F, 0x92AF, 0x92BF, 0x92CF, 0x92DF, 0x92EF, 0x92FF,
        0x930F, 0x931F, 0x93CF, 0x93DF, 0xD000, 0xD000, 0xB7CD,
        0xB7DE, 0x2E60,
    )
    sprites_signature = b"".join(word.to_bytes(2, "little") for word in sprites_words)
    sprites_byte_address = image.find(sprites_signature)
    if sprites_byte_address < 0 or sprites_byte_address & 1:
        raise AssertionError("MiniRogue SpritesB signature was not found")
    sprites_target = sprites_byte_address // 2
    sprites_word = lambda offset: int.from_bytes(
        image[(sprites_target + offset) * 2 : (sprites_target + offset) * 2 + 2], "little"
    )
    if sprites_word(0x153) != 0x9508 or sprites_word(0x1B) != 0x01FA:
        raise AssertionError("MiniRogue SpritesB structural guards changed")

    words = [int.from_bytes(image[i : i + 2], "little") for i in range(0, len(image) - 1, 2)]
    fill_loops = 0
    for index in range(len(words) - 5):
        seq = words[index : index + 6]
        stores = seq[:4]
        if (
            all((word & 0xFE0F) == 0x9201 for word in stores)
            and len({(word >> 4) & 0x1F for word in stores}) == 1
            and (seq[4] & 0xFE0F) == 0x9403
            and (seq[5] & 0xFC07) == 0xF401
            and ((seq[5] >> 3) & 0x7F) == ((-6) & 0x7F)
        ):
            fill_loops += 1
    display_loop = bytes.fromhex("00800ebc8111012c1197a0fdfdcf0192b9f7")
    if fill_loops != 1 or image.count(display_loop) != 1:
        raise AssertionError("MiniRogue framebuffer loop signatures changed")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--minirogue", type=Path)
    args = parser.parse_args()
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    port = (ROOT / "src/arduboy/arduboy_ardens_port.cpp").read_text(encoding="utf-8")
    package = (ROOT / "src/arduboy/arduboy_format.cpp").read_text(encoding="utf-8")
    app = (ROOT / "src/apps/arduboy_app.c").read_text(encoding="utf-8")
    core = (ROOT / "third_party/ardens/absim_atmega32u4.hpp").read_text(encoding="utf-8")
    config = (ROOT / "third_party/ardens/absim_config.hpp").read_text(encoding="utf-8")
    spi = (ROOT / "third_party/ardens/absim_spi.hpp").read_text(encoding="utf-8")
    timer = (ROOT / "third_party/ardens/absim_timer.hpp").read_text(encoding="utf-8")
    usb = (ROOT / "third_party/ardens/absim_usb.hpp").read_text(encoding="utf-8")
    execute = (ROOT / "third_party/ardens/absim_execute.cpp").read_text(encoding="utf-8")
    merge = (ROOT / "third_party/ardens/absim_merge_instrs.cpp").read_text(encoding="utf-8")
    loader = (ROOT / "src/dcApp.c").read_text(encoding="utf-8")
    linker = (ROOT / "src/linker_rp2350_defcon.lkr").read_text(encoding="utf-8")
    fast_test = (ROOT / "tools/arduboy_fast_dispatch_test.cpp").read_text(encoding="utf-8")
    window_test = (ROOT / "tools/arduboy_execution_window_test.cpp").read_text(encoding="utf-8")

    require(cmake, "DC32_ARDUBOY_HLE", "graphics HLE build option")
    require(cmake, 'DC32_ARDUBOY_HLE "Enable legacy routine-specific Arduboy graphics HLE" OFF',
            "legacy HLE disabled by default")
    require_absent(cmake, "DC32_ARDUBOY_PERF_PROFILE", "Arduboy profiling build")
    require_absent(cmake, "DC32_ARDUBOY_JIT", "Arduboy JIT build")
    require_absent(cmake, "ARDENS_FAST_DISPATCH=1", "slower experimental fast dispatcher")
    require(cmake, "arduboy_perf_roms", "benchmark ROM target")
    require(cmake, "ARDENS_GAME_ONLY=1", "game-only embedded profile")
    require(port, "#define ARDUBOY_PRESENT_RATE 60u", "60 FPS presentation")
    require(port, "* ARDUBOY_CPU_FREQ) / ARDUBOY_PRESENT_RATE", "fractional guest deadline")
    require(port, "mExpandBits[4][ARDUBOY_EXPANDED_PAGE_COUNT]", "compact page expansion map")
    require(port, "backend=single-core", "single-core runtime")
    require(port, "mArdens.cpu.advance_cycle();", "direct Core0 AVR execution")
    require(port, "if (!force && !clearFrame && !mArdens.displayDirty)", "unchanged frame skip")
    require_absent(port, "arduboyPrvPerfDrawOverlay", "profiling overlay")
    require_absent(port, "arduboyPrvCore1Worker", "Arduboy Core1 worker")
    require_absent(port, "arduboyPrvJit", "Arduboy JIT runtime")
    require_absent(port, "ArduboyDisplaySnapshot", "Arduboy display snapshots")
    require(port, "embedded_spi_sink = arduboyPrvCaptureSpiByte", "direct SPI sink")
    require(port, "arduboyPrvFxTransfer", "FX SPI flash protocol")
    require(port, "arduboyPrvMatchesArdBitmap203", "ArdBitmap structural fingerprint")
    require(port, "arduboyPrvMatchesSpritesB", "SpritesB structural fingerprint")
    require(port, "arduboyPrvHleDrawArdBitmap203", "native ArdBitmap renderer")
    require(port, "arduboyPrvHleDrawSpritesB", "native SpritesB renderer")
    require(port, "arduboyPrvHleCanAdvance", "side-effect-free peripheral fallback guard")
    require(port, "embedded_hle_sink = arduboyPrvGraphicsHle", "graphics HLE callback")
    require(port, "embedded_spi_bulk_sink = arduboyPrvCaptureSpiBulk", "bulk display callback")
    require(loader, "dcAppCore1ForceStop();", "forced Core1 app cleanup")
    require(port, "ARDUBOY_AVR_DDRE", "FX-C HWB chip select")
    require(port, "ARDUBOY_FX_CACHE_META_ADDR", "FX asset cache mapping")
    require(package, "arduboyPrvFindZipFxMember", "FX package member discovery")
    require(package, "tinfl_decompress(&inflator", "streaming FX package decompression")
    require(package, "ARDUBOY_FX_CACHE_MAGIC", "FX cache metadata")
    require(port, "if (mArdens.ledsTick)", "LED animation service")
    require(app, "host ? host->ledsTick : 0", "host LED callback wiring")
    require(port, "ARDUBOY_AVR_PINB] & 0xefu) | ((keys & KEY_BIT_A)",
            "badge-right/internal A to Arduboy B mapping")
    require(port, "ARDUBOY_AVR_PINE] & 0xbfu) | ((keys & KEY_BIT_B)",
            "badge-left/internal B to Arduboy A mapping")
    require(port, "dispPrvFrameCtrReset();", "post-start pacing reset")
    require(core, "advance_until(uint64_t deadline)", "batched execution API")
    require(core, "embedded_execute_fast(avr_instr_t i)", "differential-tested optional dispatcher")
    require(core, "return INSTR_MAP[i.func](*this, i);", "exact dispatcher fallback")
    require_absent(core, "ARDENS_PERF_PROFILE", "per-instruction profiling")
    require(config, ".fastcode.ardens", "Ardens hot-code section")
    require(spi, "cpu.embedded_spi_sink(", "SPI delivery hook")
    require(timer, "embedded_timer3_speaker_only", "Timer3 speaker suppression")
    require(timer, "embedded_timer4_speaker_only", "Timer4 speaker suppression")
    require(usb, "without scheduling a 1 kHz stream of fake SOF events", "game-only USB behavior")
    require(execute, "instr_merged_fill4_inc_brne", "bulk framebuffer clear handler")
    require(execute, "instr_merged_arduboy_display", "bulk framebuffer display handler")
    require(execute, "embedded_can_advance_timer0_delay", "deadline-safe HLE timer guard")
    require(merge, "INSTR_MERGED_FILL4_INC_BRNE", "framebuffer clear loop recognition")
    require(merge, "INSTR_MERGED_ARDUBOY_DISPLAY", "framebuffer display loop recognition")
    require(linker, "*absim_execute.cpp.obj(.text .text.*", "instruction handlers in SRAM")
    require(fast_test, "iteration < 4096", "randomized dispatcher differential coverage")
    require(fast_test, "INSTR_MAP[func](*exact, i)", "exact instruction comparison")
    require(fast_test, "fast->embedded_execute_fast(i)", "fast instruction comparison")
    require(window_test, "while(cpu->cycle_count < deadline)", "complete execution window")
    require(window_test, "hash_bytes(hash, cpu->data.data()", "execution-window RAM comparison")

    images: dict[str, bytes] = {}
    with tempfile.TemporaryDirectory() as temp:
        for kind in ("cpu", "spi", "timer", "graphics"):
            output = Path(temp) / f"arduboy_{kind}_perf.hex"
            subprocess.run(
                [sys.executable, str(ROOT / "tools/build_arduboy_perf_hex.py"), kind, str(output)],
                check=True,
            )
            images[kind] = parse_hex(output)
            if len(images[kind]) < 24 or len(images[kind]) > 256:
                raise AssertionError(f"unexpected {kind} benchmark size: {len(images[kind])}")
        fx_package = Path(temp) / "arduboy_fx_perf.arduboy"
        subprocess.run(
            [sys.executable, str(ROOT / "tools/build_arduboy_perf_hex.py"), "fx", str(fx_package)],
            check=True,
        )
        with zipfile.ZipFile(fx_package) as archive:
            if set(archive.namelist()) != {"info.json", "arduboy_fx_perf.hex", "arduboy_fx_perf-data.bin"}:
                raise AssertionError("bad FX benchmark package members")
            fxdata = archive.read("arduboy_fx_perf-data.bin")
            if len(fxdata) != 256 or fxdata[:2] != b"\xa5\x5a":
                raise AssertionError("bad FX benchmark asset data")
            fx_hex = Path(temp) / "arduboy_fx_perf.hex"
            fx_hex.write_bytes(archive.read("arduboy_fx_perf.hex"))
            images["fx"] = parse_hex(fx_hex)
    if len(set(images.values())) != len(images):
        raise AssertionError("benchmark workloads are not distinct")

    verify_page_expansion()
    verify_hot_symbols()
    if args.minirogue:
        verify_minirogue(args.minirogue)
    print("Arduboy performance checks passed")


if __name__ == "__main__":
    main()
