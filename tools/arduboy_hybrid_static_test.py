#!/usr/bin/env python3
"""Static regression checks for the badge-native Arduboy runtimes."""

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
CMAKE_SRC = ROOT / "CMakeLists.txt"
ARDENS_PORT_SRC = ROOT / "src" / "arduboy" / "arduboy_ardens_port.cpp"
HYBRID_SRC = ROOT / "src" / "arduboy" / "arduboy_port.cpp"
SIM_SRC = ROOT / "src" / "arduboy" / "arduboy_simavr_port.cpp"
ARDENS_DIR = ROOT / "third_party" / "ardens"
ARDENS_CONFIG_SRC = ARDENS_DIR / "absim_config.hpp"
ARDENS_HPP_SRC = ARDENS_DIR / "absim.hpp"
ARDENS_CPU_SRC = ARDENS_DIR / "absim_atmega32u4.hpp"
ARDENS_SOUND_SRC = ARDENS_DIR / "absim_sound.hpp"
ARDENS_RESET_SRC = ARDENS_DIR / "absim_reset.cpp"
ARDENS_EMBEDDED_SRC = ARDENS_DIR / "absim_embedded.cpp"
LINKER_SRC = ROOT / "src" / "linker_rp2350_defcon.lkr"
ARDUBOY_APP_SRC = ROOT / "src" / "apps" / "arduboy_app.c"
MAIN_SRC = ROOT / "src" / "main_rp2350_defcon.c"


def expect(ok: bool, msg: str, failures: list[str]) -> None:
    if ok:
        print(f"ok - {msg}")
    else:
        print(f"not ok - {msg}")
        failures.append(msg)


def main() -> int:
    cmake_text = CMAKE_SRC.read_text(encoding="utf-8")
    ardens_text = ARDENS_PORT_SRC.read_text(encoding="utf-8")
    hybrid_text = HYBRID_SRC.read_text(encoding="utf-8")
    sim_text = SIM_SRC.read_text(encoding="utf-8")
    app_text = ARDUBOY_APP_SRC.read_text(encoding="utf-8")
    main_text = MAIN_SRC.read_text(encoding="utf-8")
    ardens_config_text = ARDENS_CONFIG_SRC.read_text(encoding="utf-8")
    ardens_hpp_text = ARDENS_HPP_SRC.read_text(encoding="utf-8")
    ardens_cpu_text = ARDENS_CPU_SRC.read_text(encoding="utf-8")
    ardens_sound_text = ARDENS_SOUND_SRC.read_text(encoding="utf-8")
    ardens_reset_text = ARDENS_RESET_SRC.read_text(encoding="utf-8")
    ardens_embedded_text = ARDENS_EMBEDDED_SRC.read_text(encoding="utf-8")
    ardens_merge_text = (ARDENS_DIR / "absim_merge_instrs.cpp").read_text(encoding="utf-8")
    ardens_exec_text = (ARDENS_DIR / "absim_execute.cpp").read_text(encoding="utf-8")
    ardens_instr_text = (ARDENS_DIR / "absim_instructions.hpp").read_text(encoding="utf-8")
    ardens_spi_text = (ARDENS_DIR / "absim_spi.hpp").read_text(encoding="utf-8")
    linker_text = LINKER_SRC.read_text(encoding="utf-8")
    failures: list[str] = []

    expect("ARDUBOY_SIMAVR_EXPERIMENTAL" in cmake_text and
           "ARDUBOY_HYBRID_EXPERIMENTAL" in cmake_text,
           "runtime selector exposes simavr and hybrid only as explicit experiments", failures)
    expect("elseif(ARDUBOY_SIMAVR_EXPERIMENTAL)" in cmake_text and
           "${SRC_DIR}/arduboy/arduboy_ardens_port.cpp" in cmake_text and
           "${SRC_DIR}/arduboy/arduboy_simavr_port.cpp" in cmake_text and
           "${SRC_DIR}/arduboy/arduboy_port.cpp" in cmake_text,
           "CMake selects the Ardens port on the default branch", failures)
    expect("set(ARDUBOY_ARDENS_SOURCES)" in cmake_text and
           "absim_embedded.cpp" in cmake_text and "absim_decode.cpp" in cmake_text and
           "absim_execute.cpp" in cmake_text and "absim_merge_instrs.cpp" in cmake_text and
           "absim_reset.cpp" in cmake_text and "ARDENS_EMBEDDED=1;NDEBUG=1" in cmake_text,
           "Ardens embedded core sources are compiled with embedded trims", failures)
    expect("ARDENS_EMBEDDED" in ardens_config_text and
           "ARDENS_NO_AUDIO" in ardens_config_text and
           "ARDENS_MINIMAL_DISPLAY" in ardens_config_text and
           "ARDENS_NO_USB_BUFFER" in ardens_config_text,
           "Ardens embedded config disables debugger/audio/heavy display state", failures)
    expect("#include \"absim_atmega32u4.hpp\"" in ardens_embedded_text and
           "#include \"absim_display.hpp\"" in ardens_embedded_text,
           "Ardens peripheral/display method bodies are compiled once for firmware", failures)
    expect("decoded_prog[pc]" in ardens_cpu_text and
           "merged_prog[pc]" in ardens_cpu_text and "#ifdef ARDENS_EMBEDDED" in ardens_cpu_text,
           "embedded Ardens executes the owned decoded instruction table", failures)
    expect("uint32_t words = 0;" in ardens_merge_text and
           "for(size_t m = n; words < 254" in ardens_merge_text and
           "i0.src = (uint8_t)words;" in ardens_merge_text,
           "Ardens delay merge scans from the current instruction without shadowing the PC", failures)
    expect("INSTR_MERGED_SBIW_BRNE" in ardens_instr_text and
           "instr_merged_sbiw_brne" in ardens_exec_text and
           "cpu.peripheral_queue.next_cycle()" in ardens_exec_text and
           "i0.func = INSTR_MERGED_SBIW_BRNE" in ardens_merge_text,
           "Ardens collapses common AVR delay loops without crossing peripheral events", failures)
    expect("embedded_delay_pc" in ardens_hpp_text and
           "embedded_hle_call" in ardens_exec_text and
           "embedded_advance_timer0_delay" in ardens_exec_text and
           "embedded_timer0_overflow_addr" in ardens_hpp_text and
           "arduboyPrvDetectEmbeddedHleTargets" in ardens_text and
           "arduboyPrvScoreArduinoDelayTarget" in ardens_text and
           "arduboyPrvDetectTimer0OverflowAddr" in ardens_text and
           "arduboyPrvFindTimer0IsrVars" in ardens_text and
           "arduboyPrvRawUnconditionalJumpTarget" in ardens_text and
           "visited[48]" in ardens_text and
           "HLE:DELAY" in ardens_text and "t0=%lu" in ardens_text,
           "Ardens default has anchored Arduino delay and Timer0 HLE diagnostics", failures)
    expect("#ifndef ARDENS_NO_AUDIO" in ardens_reset_text and
           "sound_st_handler_ddrc" in ardens_reset_text and
           "#ifndef ARDENS_NO_AUDIO\n    update_sound();" in ardens_cpu_text and
           "#if ARDENS_NO_AUDIO" in ardens_sound_text and
           "sound_prev_cycle = cycle_count;" in ardens_sound_text and "return;" in ardens_sound_text,
           "Ardens audio is disabled at the lowest core layer", failures)
    expect("#ifdef ARDENS_EMBEDDED" in ardens_spi_text and
           "spi_done_cycle = cpu.cycle_count" in ardens_spi_text and
           "embedded_spi_fast_writes" in ardens_spi_text and
           "fastSpi=%lu" in ardens_text,
           "Ardens embedded SPI is instant-ready while still exposing display-transfer diagnostics", failures)
    expect("arduboyAnalyzeRom" in ardens_text and "arduboyExtractPackageToFlash" in ardens_text and
           "arduboyPrvHexParse" in ardens_text and "tinfl_decompress_mem_to_mem" in ardens_text,
           "Ardens default keeps the existing HEX and .arduboy loader surface", failures)
    expect("mArdens.cpu.fuse_lo = 0xff" in ardens_text and
           "mArdens.cpu.fuse_hi = 0xd3" in ardens_text and
           "mArdens.cpu.fuse_ext = 0xcb" in ardens_text and
           "mArdens.cpu.pc = 0" in ardens_text,
           "Ardens runtime mirrors production Arduboy fuse/reset behavior", failures)
    expect("arduboyPrvCaptureCompletedSpiByte" in ardens_text and
           "mArdens.display.send_data" in ardens_text and
           "mArdens.display.send_command" in ardens_text and
           "ARDUBOY_AVR_PORTD" in ardens_text,
           "Ardens SPI bytes feed the SSD1306 model through PORTD CS/DC state", failures)
    expect("uiGetUiKeysRaw() & UI_KEY_BIT_CENTER" in ardens_text and
           "arduboyPortInGameMenu()" in ardens_text and
           "ARDUBOY_CONTROL_POLL_CYCLES" in ardens_text,
           "center menu is polled inside the Ardens execution loop", failures)
    expect("arduboyPrvSyncEepromToSave(false)" in ardens_text and
           "mArdens.cpu.eeprom_dirty" in ardens_text and
           "ARDUBOY_SAVE_RAM_SIZE" in ardens_text,
           "Ardens EEPROM mirrors the existing 1024-byte save flow", failures)
    expect("displayDirty" in ardens_text and "arduboyPrvPresentFrame" in ardens_text and
           "mArdens.display.ram" in ardens_text and "mPresentLastState" in ardens_text and
           "srcX = ARDUBOY_DISPLAY_WIDTH - 1u - srcX" in ardens_text,
           "Ardens presenter keeps dirty rendering and the display-space source-X correction", failures)
    expect("arduboySetRotation(args->rotate)" in app_text and
           "arduboySetRotation(settings.rotation)" in app_text and
           "selection.runtime == GameRuntimeArduboy" in main_text and
           "mRotateGame = shouldRotateGame();" in main_text,
           "Arduboy launch and FN resume apply the saved screen orientation", failures)
    expect(all("void arduboySetRotation(bool flipped)" in text and
               "DISP_WIDTH * DISP_HEIGHT - 1u - dst" in text
               for text in (ardens_text, hybrid_text, sim_text)),
           "all Arduboy presenters support 180-degree framebuffer rotation", failures)
    expect("ARDUBOY ARDENS STOP" in ardens_text and
           "BOOT WATCHDOG" in ardens_text and "guest=%lu/s" in ardens_text and
           "backend=ardens audio=disabled" in ardens_text,
           "Ardens runtime exposes readable boot/perf diagnostics", failures)
    expect("ARDUBOY_SIM_SCRATCH_SIZE=0x18000" in cmake_text and
           "QSPI_RAM_SIZE_MAX=65536" in cmake_text and
           "ORIGIN = 0x2002B000, LENGTH = 0x2B000" in linker_text and
           "__ramvec_end <= 0x20056000" in linker_text,
           "global save RAM limit remains 64 KiB while the Ardens scratch window is reserved", failures)
    expect("ARDUBOY_HLE_ALLOW_HEURISTIC" in hybrid_text and "ArduboyHleMemset" in hybrid_text,
           "ProjectABE-style hybrid runtime remains present for experimental work", failures)
    expect("ARDUBOY_SIM_RUN_CYCLE_LIMIT" in sim_text and "ARDUBOY_SIMAVR_NO_NAMES" in cmake_text,
           "simavr runtime remains present as an experimental fallback", failures)

    if failures:
        print("\nfailed checks:")
        for failure in failures:
            print(f" - {failure}")
        return 1
    print("\narduboy runtime static checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
