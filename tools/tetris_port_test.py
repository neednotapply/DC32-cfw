#!/usr/bin/env python3
"""Regression checks for the NullpoMino-derived DC32 Tetris port."""

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TETRIS = ROOT / "src" / "apps" / "tetris"


def expect(name: str, condition: bool) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def read(path: Path) -> str:
    expect(f"{path.relative_to(ROOT)} exists", path.is_file())
    return path.read_text(encoding="utf-8")


def function_body(text: str, name: str) -> str:
    match = re.search(rf"\b{name}\s*\([^)]*\)\s*\{{", text)
    expect(f"{name} exists", match is not None)
    pos = match.end()
    depth = 1
    while pos < len(text) and depth:
        if text[pos] == "{":
            depth += 1
        elif text[pos] == "}":
            depth -= 1
        pos += 1
    expect(f"{name} body is balanced", depth == 0)
    return text[match.end() : pos - 1]


def check_piece_geometry(rules: str) -> None:
    match_x = re.search(
        r"mPieceX\[TetrisPieceCount\]\[4\]\[4\]\s*=\s*\{(?P<body>.*?)\n\};",
        rules,
        re.S,
    )
    match_y = re.search(
        r"mPieceY\[TetrisPieceCount\]\[4\]\[4\]\s*=\s*\{(?P<body>.*?)\n\};",
        rules,
        re.S,
    )
    expect("piece X table exists", match_x is not None)
    expect("piece Y table exists", match_y is not None)
    x_values = [int(value) for value in re.findall(r"-?\d+", match_x.group("body"))]
    y_values = [int(value) for value in re.findall(r"-?\d+", match_y.group("body"))]
    expect("piece tables contain 112 coordinates", len(x_values) == 112 and len(y_values) == 112)
    for piece in range(7):
        for direction in range(4):
            offset = (piece * 4 + direction) * 4
            cells = set(zip(x_values[offset : offset + 4], y_values[offset : offset + 4]))
            expect(f"piece {piece} direction {direction} has four unique cells", len(cells) == 4)


def main() -> int:
    cmake = read(ROOT / "CMakeLists.txt")
    legacy = read(ROOT / "src" / "apps" / "picoware_ports.c")
    header = read(TETRIS / "tetris_core.h")
    core = read(TETRIS / "tetris_core.c")
    rules = read(TETRIS / "tetris_rules.c")
    port = read(TETRIS / "tetris_port.c")
    app = read(TETRIS / "tetris_app.c")
    reference = read(ROOT / "third_party" / "nullpomino" / "DC32-README.md")
    license_text = read(ROOT / "third_party" / "nullpomino" / "LICENSE")
    legacy_license = read(ROOT / "third_party" / "nullpomino" / "LICENSE-NULLNONAME")

    expect("dedicated Tetris target is wired", all(token in cmake for token in (
        "set(TETRIS_APP_SOURCES",
        "add_dcapp(dcapp_tetris tetris 201\n    ${TETRIS_APP_SOURCES}",
    )))
    expect("legacy Tetris skeleton is removed", all(token not in legacy for token in (
        "pwRunTetris", "TET_W", "mTetrominoes", "DCAPP_RUNTIME_ID == 201",
    )))
    expect("runtime 201 wrapper is dedicated", "tetrisAppRun" in app and "tetrisAppAbort" in app)
    expect("field is 10 by 20 with hidden rows", all(token in header for token in (
        "TETRIS_FIELD_WIDTH 10", "TETRIS_FIELD_HEIGHT 20", "TETRIS_MAX_HIDDEN 3",
    )))
    expect("three modes are registered", all(token in rules for token in (
        '"MARATHON"', '"40 LINE RACE"', '"ULTRA 2 MIN"',
    )))
    expect("three exact rule names are registered", all(token in rules for token in (
        '"STANDARD"', '"STANDARD-FAST-B"', '"NINTENDO-R"',
    )))
    expect("standard first bag excludes S Z O", "firstBagNoSzo = true" in rules)
    expect("Nintendo rule disables modern helpers", all(token in rules for token in (
        ".bagRandomizer = false", ".hold = false", ".ghost = false",
        ".hardDrop = false", ".wallkick = false",
    )))
    expect("Nintendo READY DAS differs from delay DAS", all(token in rules + core for token in (
        ".chargeDasInReady = true", ".chargeDasInDelay = false",
        "game->rule->chargeDasInReady", "game->rule->chargeDasInDelay",
    )))
    expect("NullpoMino marathon gravity table is present", all(token in core for token in (
        "465, 731, 1280, 1707, -1, -1, -1",
        "63, 50, 39, 30, 22, 16, 12, 8, 6, 4, 3, 2, 1",
    )))
    expect("SRS normal and I kick tables are separate", all(token in core for token in (
        "mNormalLeft", "mNormalRight", "mILeft", "mIRight",
    )))
    expect("hold, ghost, DAS and lock reset paths exist", all(token in core for token in (
        "tetrisHold", "tetrisGameGhostY", "tetrisUpdateDas",
        "lockResetMove", "lockResetRotate", "lockResetFall",
    )))
    expect("first-frame movement policy is enforced", all(token in core for token in (
        "game->stateFrame > 0u || game->rule->moveFirstFrame",
        "grounded && allowControl",
    )))
    expect("Nintendo lock flash only extends nonzero ARE", all(token in core for token in (
        "if (!afterLine && delay)",
        "game->stateFrame < game->rule->lockFlashFrames",
    )))
    expect("spin, B2B, combo and all-clear scoring exists", all(token in core for token in (
        "tetrisDetectSpin", "b2bCount", "stats.combo", "allClear",
        "1800u", "3000u",
    )))
    expect("T-spin doubles use the full Nullpo score", all(token in core for token in (
        "else if (lines == 2u)",
        "points = (game->stats.b2b ? 1800u : 1200u) * level;",
    )))
    expect("fixed mode goals are enforced", all(token in core for token in (
        "stats.lines >= 150u", "stats.lines >= 40u", "stats.time >= 7200u",
    )))
    expect("Nullpo READY and GO timing is retained", all(token in core + port for token in (
        "game->stateFrame == 0u || game->stateFrame == 50u",
        "game->stateFrame >= 100u",
        "app->game.stateFrame < 49u",
        "app->game.stateFrame >= 50u",
    )))
    check_piece_geometry(rules)

    expect("badge controls are fully mapped", all(token in port for token in (
        "KEY_BIT_LEFT", "KEY_BIT_RIGHT", "KEY_BIT_DOWN", "KEY_BIT_UP",
        "KEY_BIT_A", "KEY_BIT_B", "KEY_BIT_SEL", "KEY_BIT_START",
        "UI_KEY_BIT_CENTER",
    )))
    expect("180-degree rotation is omitted", "rotate180" not in port.lower() and "180-degree" not in core.lower())
    expect("versioned checksummed save is persisted", all(token in port for token in (
        "TETRIS_SAVE_MAGIC", "TETRIS_SAVE_VERSION 1u", "TETRIS_SAVE_XOR",
        'dc32PortSaveWrite(app->args->vol, "tetris"',
    )))
    expect("last selected mode and rule are save-backed", all(token in port for token in (
        "uint8_t selectedMode;",
        "uint8_t selectedRule;",
        "loaded.selectedMode >= TetrisModeCount",
        "loaded.selectedRule >= TetrisRuleCount",
    )))
    adjust_title = function_body(port, "tetrisAdjustTitle")
    expect("mode and rule changes are written immediately", all(token in adjust_title for token in (
        "app->save.selectedMode = value;",
        "app->save.selectedRule = value;",
        "(void)tetrisWriteSave(app);",
    )))
    expect("playfield uses the full window height while staying 10 by 20", all(token in port for token in (
        "#define TETRIS_FIELD_X 100",
        "#define TETRIS_FIELD_Y 0",
        "#define TETRIS_CELL 12",
        "#define TETRIS_FIELD_PIXEL_H (TETRIS_FIELD_HEIGHT * TETRIS_CELL)",
    )))
    expect("rankings cover all mode/rule combinations", all(token in port for token in (
        "marathon[TetrisRuleCount][TETRIS_RANKS]",
        "line[TetrisRuleCount][TETRIS_RANKS]",
        "ultraScore[TetrisRuleCount][TETRIS_RANKS]",
        "ultraLines[TetrisRuleCount][TETRIS_RANKS]",
    )))
    expect("Ultra score records display their own line totals",
           "tetrisNumber(second, sizeof(second), ultra->lines);" in port)
    expect("gameplay SFX use PWM and restore volume", all(token in port for token in (
        "audioPwmTone", "audioPwmStop", "audioPwmSetVolume(app.previousVolume)",
    )))
    expect("menus remain silent", all("tetrisTone" not in function_body(port, name) for name in (
        "tetrisHandleTitle", "tetrisHandleSettings", "tetrisHandleRecords",
    )))
    expect("runtime is 60 FPS", "#define TETRIS_FPS 60u" in port and "dispSetFramerate(TETRIS_FPS)" in port)
    expect("pinned source is documented", "4de098dd0b48d991247313d8dba30b9721e6f9d9" in reference)
    expect("upstream BSD license is retained", "BSD 3-Clause License" in license_text)
    expect("adapted NullNoname BSD notice is retained", "Copyright (c) 2010, NullNoname" in legacy_license)

    image = ROOT / "build" / "apps" / "tetris.DC32"
    if image.is_file():
        expect("Tetris image stays below 256 KiB", image.stat().st_size < 0x40000)

    print("Tetris port regression tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
