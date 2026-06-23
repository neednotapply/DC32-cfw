#!/usr/bin/env python3
"""Static architecture checks for the shared multi-mode Pong engine."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PONG = ROOT / "src" / "apps" / "pong"
MODE_FILES = {
    "Classic Pong": "mode_classic.c",
    "Pong Doubles": "mode_doubles.c",
    "Quadrapong": "mode_quadrapong.c",
    "Super Pong": "mode_super.c",
    "Rebound": "mode_rebound.c",
    "Breakout": "mode_breakout.c",
    "Warlords-lite": "mode_warlords.c",
    "Power Pong": "mode_power.c",
}


def expect(name: str, condition: bool) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def read(path: Path) -> str:
    expect(f"{path.relative_to(ROOT)} exists", path.is_file())
    return path.read_text(encoding="utf-8")


def main() -> int:
    cmake = read(ROOT / "CMakeLists.txt")
    legacy = read(ROOT / "src" / "apps" / "picoware_ports.c")
    core_h = read(PONG / "core" / "pong_core.h")
    core_c = read(PONG / "core" / "pong_core.c")
    platform_h = read(PONG / "platform" / "pong_platform.h")
    platform_c = read(PONG / "platform" / "pong_dc32.c")
    main_c = read(PONG / "main" / "pong_main.c")
    modes_h = read(PONG / "modes" / "pong_modes.h")
    common_c = read(PONG / "modes" / "pong_mode_common.c")

    expect("Pong target has a dedicated source set", "set(PONG_APP_SOURCES" in cmake)
    expect("Pong target uses only the dedicated source set", "add_dcapp(dcapp_pong pong 200\n    ${PONG_APP_SOURCES}" in cmake)
    expect("legacy one-off Pong is removed", "pwRunPong" not in legacy and "DCAPP_RUNTIME_ID == 200" not in legacy)
    expect("logical resolution is 320x240", "PONG_SCREEN_W 320" in core_h and "PONG_SCREEN_H 240" in core_h)
    expect("core uses fixed-point vectors", all(token in core_h for token in ("PONG_FP_SHIFT", "struct PongVec", "struct PongBall")))
    expect("shared core owns paddles, balls, scoring and state", all(token in core_h for token in (
        "struct PongPaddle", "struct PongBall", "score[PONG_MAX_PLAYERS]",
        "struct PongGameState", "pongCoreCollidePaddle", "pongCoreServeBall",
    )))
    expect("core implements collision helpers", all(token in core_c for token in (
        "pongCoreRectOverlap", "pongCoreCollidePaddle", "pongCoreCollideRect",
        "pongCoreBounceVerticalWalls", "pongCoreBounceHorizontalWalls",
    )))
    expect("input is abstract axes and buttons", all(token in platform_h for token in (
        "axisX", "axisY", "confirmPressed", "backPressed",
    )))
    expect("renderer exposes only minimal draw operations", all(token in platform_h for token in (
        "clear", "draw_rect", "draw_line", "draw_text", "present",
    )))
    expect("audio exposes the minimal buzzer operations", all(token in platform_h for token in (
        "beep", "click", "score_sound",
    )))
    expect("audio state exposes enable and volume settings", "uint8_t volume" in platform_h and "bool enabled" in platform_h)
    expect("menu exposes audio, volume and color settings", all(token in main_c for token in (
        "IN-GAME AUDIO", '"VOLUME"', '"COLORS"', '"CLASSIC"', '"TEAMS"', '"RAINBOW"',
    )))
    expect("menu navigation is silent", "platform.audio.click" not in main_c and "880u" not in main_c)
    expect("returning to the menu stops active audio", "platform.audio.beep(platform.audio.context, 0, 0)" in main_c)
    expect("renderer supports the three color themes", all(token in platform_h for token in (
        "PongColorClassic", "PongColorTeams", "PongColorRainbow",
    )))
    expect("DC32 renderer applies theme-aware colors", all(token in platform_c for token in (
        "pongDc32Color", "pongDc32Wheel", "PongColorTeams", "PongColorRainbow",
    )))
    expect("hardware input stays in the DC32 adapter", "KEY_BIT_UP" in platform_c and "KEY_BIT_UP" not in core_c)
    expect("hardware audio stays in the DC32 adapter", "audioPwmTone" in platform_c and "audioPwmTone" not in common_c)
    expect("main owns menu and mode selection", all(token in main_c for token in (
        "pongMainDrawMenu", "pongModesGet", "PongMainPlaying", "mode->update", "mode->draw",
    )))
    expect("mode interface is rule-driven", all(token in modes_h for token in (
        "struct PongMode", "(*init)", "(*update)", "(*draw)",
    )))

    for label, filename in MODE_FILES.items():
        source = read(PONG / "modes" / filename)
        expect(f"{label} registers a rule module", "const struct PongMode pongMode" in source)
        expect(f"{label} uses shared game state", "struct PongGameState" in source)
        expect(f"{label} avoids direct hardware APIs", all(token not in source for token in (
            "KEY_BIT_", "dcAppDraw", "audioPwm", "dispSetFramerate",
        )))

    power = read(PONG / "modes" / "mode_power.c")
    expect("Power Pong contains all requested powerups", all(token in power for token in (
        "PowerPongEnlarge", "PowerPongShrink", "PowerPongSpeed",
        "PowerPongSlow", "PowerPongMulti",
    )))
    print("Pong port architecture tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
