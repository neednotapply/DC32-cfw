#!/usr/bin/env python3
"""Regression checks for the source-derived DC32 Arkanoid port."""

from __future__ import annotations

import importlib.util
import re
import subprocess
import sys
import tempfile
from pathlib import Path

sys.dont_write_bytecode = True


ROOT = Path(__file__).resolve().parents[1]
PORT = ROOT / "src" / "apps" / "arkanoid" / "arkanoid_port.c"
BUILDER = ROOT / "tools" / "build_arkanoid_assets.py"


def expect(name: str, condition: bool) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def load_builder():
    spec = importlib.util.spec_from_file_location("build_arkanoid_assets", BUILDER)
    expect("asset builder import spec", spec is not None and spec.loader is not None)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def check_asset_builder() -> None:
    builder = load_builder()
    expect("pinned upstream commit", builder.UPSTREAM_COMMIT == "7e0e876cd034ebd62890e65352c7ef0b12b45df5")
    expect("centered arena scale", (builder.ARENA_W, builder.ARENA_H) == (222, 240))
    cache = ROOT / "build" / "arkanoid" / "upstream.zip"
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        result = subprocess.run(
            [
                sys.executable,
                str(BUILDER),
                "--cache",
                str(cache if cache.is_file() else tmp_path / "upstream.zip"),
                "--output-c",
                str(tmp_path / "arkanoid_assets.c"),
                "--output-h",
                str(tmp_path / "arkanoid_assets.h"),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        expect("asset generator succeeds", result.returncode == 0)
        header = (tmp_path / "arkanoid_assets.h").read_text(encoding="ascii")
        source = (tmp_path / "arkanoid_assets.c").read_text(encoding="ascii")
        expect("all upstream PNG sprites are represented", "ArkAssetCount = 268" in header)
        expect("gameplay sprite families are generated", all(token in header for token in (
            "ArkAssetBall", "ArkAssetBrickSilver10", "ArkAssetDoorTopRight7",
            "ArkAssetEnemyMolecule25", "ArkAssetPaddleWidePulsate4",
            "ArkAssetPowerupDuplicate8",
        )))
        match = re.search(r"(\d+) sprites, (\d+) RGB332 bytes", result.stdout)
        expect("scaled atlas reports expected size", match is not None and int(match.group(1)) == 268)
        expect("scaled atlas remains compact", int(match.group(2)) < 90000)
        expect("asset source records pinned commit", builder.UPSTREAM_COMMIT in source)


def check_port() -> None:
    port = PORT.read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    picoware = (ROOT / "src" / "apps" / "picoware_ports.c").read_text(encoding="utf-8")
    expect("dedicated Arkanoid target is wired", all(token in cmake for token in (
        "ARKANOID_APP_SOURCES", "arkanoid_assets", "${ARKANOID_APP_SOURCES}",
    )))
    expect("placeholder implementation is removed", "pwRunArkanoid" not in picoware and "DCAPP_RUNTIME_ID == 202" not in picoware)
    expect("source geometry is retained", all(token in port for token in (
        "#define ARK_SOURCE_W 600", "#define ARK_SOURCE_H 650",
        "#define ARK_BRICK_W 43.0f", "#define ARK_BRICK_H 21.0f",
    )))
    expect("arena and side rails are fixed", all(token in port for token in (
        "#define ARK_ARENA_X 49", "#define ARK_ARENA_W 222",
        "arkDrawHud", "arkDrawArena",
    )))
    expect("HUD uses a right-side score column", '"SCORE"' in port and '"1UP"' not in port)
    expect("left rail carries the counter-clockwise logo", "arkBlitRotatedCounterClockwise(game, ArkAssetLogo" in port)
    expect("lives use a numeric right-side entry", '"LIVES"' in port and '"VAUS"' not in port and "ArkAssetPaddleLife, rightX" not in port)
    expect("all five rounds are implemented", all(f"arkBuildRound{round_no}" in port for round_no in range(1, 6)))
    expect("brick durability rules are implemented", all(token in port for token in (
        "color == ArkBrickSilver ? 2", "color == ArkBrickGold ? -1",
        "arkBrickValue", "game->destroyed",
    )))
    expect("all required powerups are implemented", all(token in port for token in (
        "ArkPowerCatch", "ArkPowerDuplicate", "ArkPowerExpand",
        "ArkPowerLife", "ArkPowerLaser", "ArkPowerSlow",
    )))
    expect("four enemy families and door release are implemented", all(token in port for token in (
        "ArkEnemyCone", "ArkEnemyPyramid", "ArkEnemyMolecule", "ArkEnemyCube",
        "arkReleaseEnemies", "ArkAssetDoorTopLeft1", "ArkAssetDoorTopRight1",
    )))
    expect("source paddle bounce angles are retained", "{220.0f, 245.0f, 260.0f, 280.0f, 295.0f, 320.0f}" in port)
    expect("badge controls are mapped", all(token in port for token in (
        "KEY_BIT_LEFT", "KEY_BIT_RIGHT", "KEY_BIT_A", "KEY_BIT_START",
        "UI_KEY_BIT_CENTER",
    )))
    expect("B remains unused in gameplay", port.count("KEY_BIT_B") == 1)
    expect("versioned high score is persisted", all(token in port for token in (
        "ARK_SAVE_MAGIC", "ARK_SAVE_VERSION 1u", "ARK_SAVE_CHECK_XOR",
        'dc32PortSaveWrite(game->args->vol, "arkanoid"',
    )))
    expect("runtime is fixed at 60 FPS", "#define ARK_FPS 60u" in port and "dispSetFramerate(ARK_FPS)" in port)


def main() -> int:
    check_asset_builder()
    check_port()
    print("Arkanoid port regression tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
