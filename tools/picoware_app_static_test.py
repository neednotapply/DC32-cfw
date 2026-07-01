#!/usr/bin/env python3
"""Static checks for Picoware-derived SD apps and shared badge draw helpers."""

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PICOWARE_APPS = {
    "Starfield": (220, "starfield.DC32"),
    "Spiro": (221, "spiro.DC32"),
    "Cube": (222, "cube.DC32"),
}
BANNED_TOKENS = (
    "malloc(",
    "calloc(",
    "realloc(",
    "free(",
    "new ",
    "delete ",
    "#include <Arduino",
    "Arduino.h",
    "MicroPython",
    "mp_obj",
    "WiFi",
    "Bluetooth",
    "Keyboard",
    "Adafruit",
    "TFT_",
    "ILI9341",
    "pico_display",
    "displayio",
)


def expect(name: str, condition: bool) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def function_body(text: str, name: str) -> str:
    match = re.search(rf"static [^{{;]+ {name}\([^)]*\)\n\{{", text)
    expect(f"{name} exists", match is not None)
    depth = 0
    start = match.end() - 1
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start : i + 1]
    expect(f"{name} has balanced body", False)
    return ""


def main() -> int:
    dcapp_h = (ROOT / "src" / "dcApp.h").read_text(encoding="utf-8")
    dcapp_c = (ROOT / "src" / "dcApp.c").read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    builder = (ROOT / "tools" / "build_sd_zip.py").read_text(encoding="utf-8")
    ui = (ROOT / "src" / "ui.c").read_text(encoding="utf-8")
    port = (ROOT / "src" / "apps" / "picoware_ports.c").read_text(encoding="utf-8")
    wrapper = (ROOT / "src" / "apps" / "picoware_app.c").read_text(encoding="utf-8")
    helper_h = (ROOT / "src" / "dcAppDraw.h").read_text(encoding="utf-8")
    helper_c = (ROOT / "src" / "dcAppDraw.c").read_text(encoding="utf-8")
    combined_port = port + "\n" + wrapper

    old_source_var = "CATALOG" + "_APP_SOURCES"
    old_filename = "catalog" + "_app"
    old_apps_tool = "UiTool" + "Apps"
    old_intro_helper = "pw" + "Intro"
    old_play_text = "A/" + "Arrows" + " = Play"
    old_exit_text = "B/" + "Center" + " = Exit"

    expect("resident Apps registry helper exists", "dcAppCatalogEntries" in dcapp_h and "dcAppCatalogFind" in dcapp_c)
    expect("main menu uses categories", all(token in ui for token in ("UiToolUsb", "UiToolMedia", "UiToolGames")) and old_apps_tool not in ui)
    top_menu = re.search(r"static const enum UiToolId toolOrder\[\] = \{(?P<body>.*?)\};", ui, re.S)
    expect("top-level menu order exists", top_menu is not None)
    expect("BadUSB is not top-level", "UiToolBadUsb" not in top_menu.group("body"))
    expect("USB category contains BadUSB", '{"BadUSB", UiCategoryEntryTool, UiToolBadUsb, 0}' in ui)
    expect("category return waits for key release", "uiPrvCategoryReturnFence" in ui and "uiPrvWaitKeysReleased();" in ui)
    expect("media category contains screensavers", all(token in ui for token in ("DcAppIdStarfield", "DcAppIdSpiro", "DcAppIdCube")))
    expect("games category contains supported small ports", all(token in ui for token in ("DcAppIdPong", "DcAppIdTetris", "DcAppIdArkanoid", "DcAppIdFlappy", "DcAppIdTrex")))
    expect("retired Labyrinth app is fully removed", "DcAppIdLabyrinth" not in dcapp_h + dcapp_c + ui and '"labyrinth.DC32"' not in builder and "dcapp_labyrinth" not in cmake and "pwRunLabyrinth" not in port and 'apps/labyrinth.DC32' in cmake)
    expect("retired Video app is excluded and stale build outputs are cleaned", '"video.DC32"' not in builder and 'apps/video.DC32' in cmake and 'apps/video.raw' in cmake)
    expect("Picoware wrapper uses runtime-specific header", "DCAPP_RUNTIME_ID" in wrapper and "picowareAppRun" in wrapper)
    expect("Picoware source list is named for the port, not the registry", "PICOWARE_APP_SOURCES" in cmake and old_source_var not in cmake)
    expect("Pong uses its dedicated source tree", "PONG_APP_SOURCES" in cmake and "${PONG_APP_SOURCES}" in cmake)
    expect("Pong id declared", "DcAppIdPong = 200" in dcapp_h)
    expect("Pong app path", '"/APPS/pong.DC32"' in dcapp_c)
    expect("Pong launcher visible", '"Pong"' in dcapp_c)
    expect("Pong CMake target", "dcapp_pong pong 200" in cmake)
    expect("Pong packaged", '"pong.DC32"' in builder)
    expect("Pong is removed from the Picoware placeholder", "DCAPP_RUNTIME_ID == 200" not in port and "pwRunPong" not in port)
    expect("Tetris is removed from the Picoware placeholder", "DCAPP_RUNTIME_ID == 201" not in port and "pwRunTetris" not in port)
    expect("Tetris uses its dedicated source tree", "TETRIS_APP_SOURCES" in cmake and "${TETRIS_APP_SOURCES}" in cmake)
    expect("Tetris id declared", "DcAppIdTetris = 201" in dcapp_h)
    expect("Tetris app path", '"/APPS/tetris.DC32"' in dcapp_c)
    expect("Tetris launcher visible", '"Tetris"' in dcapp_c)
    expect("Tetris CMake target", "dcapp_tetris tetris 201" in cmake)
    expect("Tetris packaged", '"tetris.DC32"' in builder)
    expect("Flappy Bird is removed from the Picoware placeholder", "pwRunFlappy" not in port and "DCAPP_RUNTIME_ID == 203" not in port)
    expect("Flappy Bird uses its dedicated source-derived target", "FLAPPY_APP_SOURCES" in cmake and "${FLAPPY_APP_SOURCES}" in cmake and "flappy_assets" in cmake)
    for label, (runtime, filename) in PICOWARE_APPS.items():
        target = filename.removesuffix(".DC32")
        expect(f"{label} id declared", f"= {runtime}" in dcapp_h)
        expect(f"{label} app path", f'"/APPS/{filename}"' in dcapp_c)
        expect(f"{label} launcher visible", f'"{label}"' in dcapp_c and "true" in dcapp_c)
        expect(f"{label} CMake target", f" {target} {runtime}" in cmake)
        expect(f"{label} packaged", f'"{filename}"' in builder)
        expect(f"{label} dispatch present", f"DCAPP_RUNTIME_ID == {runtime}" in port)
    expect("T-Rex is no longer dispatched by the Picoware placeholder", "DCAPP_RUNTIME_ID == 205" not in port and "pwRunTrex" not in port)
    expect("T-Rex has a dedicated source-derived target", "TREX_APP_SOURCES" in cmake and "${TREX_APP_SOURCES}" in cmake)
    expect("Arkanoid is no longer dispatched by the Picoware placeholder", "DCAPP_RUNTIME_ID == 202" not in port and "pwRunArkanoid" not in port)
    expect("Arkanoid has a dedicated source-derived target", "ARKANOID_APP_SOURCES" in cmake and "${ARKANOID_APP_SOURCES}" in cmake)

    for token in BANNED_TOKENS:
        expect(f"Picoware ports avoid {token}", token not in combined_port)
    expect("Picoware ports avoid old implementation filenames", old_filename not in cmake and old_filename not in combined_port)
    expect("Picoware app has no splash helper", old_intro_helper not in port and old_play_text not in port and old_exit_text not in port)
    frame_body = function_body(port, "pwFrame")
    expect("Picoware app exits on center only", "dcAppDrawFrame(&ctx->draw, UI_KEY_BIT_CENTER)" in frame_body)
    expect("Picoware frame does not exit on B", "KEY_BIT_B" not in frame_body)
    expect("Picoware app waits for stale launcher buttons", "dcAppDrawWaitRelease(&ctx->draw, KEY_BIT_A | KEY_BIT_B | UI_KEY_BIT_CENTER)" in port)
    expect("Picoware app owns only a lightweight backbuffer", "static uint8_t mPicowareBackbuffer[PICOWARE_SCREEN_W * PICOWARE_SCREEN_H]" in port)
    expect("Picoware app initializes shared draw helper", "dcAppDrawInit(&ctx->draw" in port)
    expect("Picoware app uses shared draw primitives", all(token in port for token in ("dcAppDrawClear", "dcAppDrawFill", "dcAppDrawPixel", "dcAppDrawLine")))
    expect("Picoware app avoids private presenter", "pwPresent" not in port and "Rgb565ToRgb332" not in port and "Rgb332ToRgb565" not in port)
    expect("Picoware app sets app framerate", "dispSetFramerate(PICOWARE_FPS)" in port and "dispSetFramerate(60)" in port)

    for symbol in (
        "dcAppDrawRgb565",
        "dcAppDrawInit",
        "dcAppDrawClear",
        "dcAppDrawFill",
        "dcAppDrawPixel",
        "dcAppDrawLine",
        "dcAppDrawPresent",
        "dcAppDrawFrame",
        "dcAppDrawWaitRelease",
    ):
        expect(f"{symbol} is declared", symbol in helper_h)
        expect(f"{symbol} is linker-rooted", f"-Wl,--undefined={symbol}" in cmake)
    expect("shared draw helper is firmware-owned", "${SRC_DIR}/dcAppDraw.c" in cmake)
    expect("shared draw helper paces present with scanout", "dispPrvFrameCtrWait()" in helper_c and "dispPrvWaitForScanoutStart()" in helper_c)
    expect("shared draw helper presents completed backbuffer", "dcAppDrawPresent(ctx);" in helper_c)
    expect("shared draw helper handles canvas orientation", "dcAppDrawPrvDisplayIndex" in helper_c and "cnv->rotated" in helper_c and "cnv->flipped" in helper_c)
    print("Picoware app static tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
