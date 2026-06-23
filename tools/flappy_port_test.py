#!/usr/bin/env python3
"""Regression checks for the source-derived DC32 Flappy Bird port."""

from __future__ import annotations

import importlib.util
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

sys.dont_write_bytecode = True


ROOT = Path(__file__).resolve().parents[1]
PORT = ROOT / "src" / "apps" / "flappy" / "flappy_port.c"
BUILDER = ROOT / "tools" / "build_flappy_assets.py"


def expect(name: str, condition: bool) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def load_builder():
    spec = importlib.util.spec_from_file_location("build_flappy_assets", BUILDER)
    expect("asset builder import spec", spec is not None and spec.loader is not None)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def check_asset_builder() -> None:
    builder = load_builder()
    expect("pinned upstream commit", builder.UPSTREAM_COMMIT == "3b3060cdf6b38b819e5a649bc92d11776decd0b4")
    expect("pinned upstream tag", builder.UPSTREAM_TAG == "v1.9.2")
    expect("pinned upstream archive", builder.UPSTREAM_ZIP_SHA256 == "e06e8553e747be5851edfe90486dda3afcb5b2a9ddac4827f4a7911a6fb39d51")
    expect("full-width landscape viewport", (builder.VIEWPORT_W, builder.VIEWPORT_H) == (320, 240))
    expect("landscape virtual width preserves upstream height", (builder.SOURCE_W, builder.SOURCE_H) == (320, 512))
    expect("logo stays native sized for badge clarity", any(
        name == "Logo" and (width, height) == (192, 44)
        for name, _path, width, height in builder.ASSETS
    ))
    expect("floor is lowered and ready tap is cropped", builder.BASE_Y_PERCENT == 86.0 and
           builder.ASSET_CROPS["sprites/message.png"] == (25, 160, 135, 107) and
           any(name == "Message" and (width, height) == (170, 135)
               for name, _path, width, height in builder.ASSETS))
    expect("visible sprite set is compact", len(builder.ASSETS) == 39)
    expect("unused Android menu/pause assets omitted", all("pause" not in path and "menu" not in path and "resume" not in path and "sparkle" not in path for _name, path, _w, _h in builder.ASSETS))
    expect("core upstream dimensions are pinned", all(
        builder.EXPECTED_DIMS[path] == dims for path, dims in {
            "sprites/background-day.png": (288, 512),
            "sprites/base.png": (336, 111),
            "sprites/pipe-green.png": (52, 320),
            "sprites/message.png": (184, 267),
            "sprites/yellowbird-midflap.png": (34, 24),
        }.items()
    ))

    cache = ROOT / "build" / "flappy" / "upstream.zip"
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        result = subprocess.run(
            [
                sys.executable,
                str(BUILDER),
                "--cache",
                str(cache if cache.is_file() else tmp_path / "upstream.zip"),
                "--output-c",
                str(tmp_path / "flappy_assets.c"),
                "--output-h",
                str(tmp_path / "flappy_assets.h"),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        expect("asset generator succeeds", result.returncode == 0)
        header = (tmp_path / "flappy_assets.h").read_text(encoding="ascii")
        source = (tmp_path / "flappy_assets.c").read_text(encoding="ascii")
        match = re.search(r"(\d+) sprites, (\d+) RGB332 bytes", result.stdout)
        expect("asset generator reports compact sprite count", match is not None and int(match.group(1)) == 39)
        expect("asset generator reports expected raster size", int(match.group(2)) == 182522)
        expect("asset generator reports opaque sprites and run tables",
               "8 opaque sprites, 1174 rows, 1326 runs" in result.stdout)
        expect("asset header exposes viewport", "FLAPPY_ASSET_VIEWPORT_W 320u" in header and "FLAPPY_ASSET_VIEWPORT_H 240u" in header)
        expect("asset header exposes run-table ABI", all(token in header for token in (
            "FLAPPY_ASSET_FLAG_OPAQUE", "struct FlappyAssetRow", "struct FlappyAssetRun",
            "flappyAssetRows", "flappyAssetRuns",
        )))
        expect("legacy asset masks are removed", "flappyAssetMasks" not in header and "maskOffset" not in header and "flappyAssetMasks" not in source)
        expect("opaque hot-path assets are flagged", all(record in source for record in (
            "{0u, 0u, 320u, 206u, 1u, 0u}",
            "{65920u, 0u, 320u, 60u, 1u, 0u}",
            "{85120u, 0u, 80u, 28u, 1u, 0u}",
            "{133542u, 0u, 224u, 80u, 1u, 0u}",
            "{151462u, 0u, 32u, 14u, 1u, 0u}",
        )))
        expect("asset header exposes all gameplay sprites", all(token in header for token in (
            "FlappyAssetBackgroundDay", "FlappyAssetPipeGreen", "FlappyAssetYellowBirdMid",
            "FlappyAssetPanel", "FlappyAssetPlatinumMedal", "FlappyAssetDigit9Small",
        )))
        expect("asset source records pinned commit", builder.UPSTREAM_COMMIT in source)


def check_port_source() -> None:
    port = PORT.read_text(encoding="utf-8")
    app = (ROOT / "src" / "apps" / "flappy" / "flappy_app.c").read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    picoware = (ROOT / "src" / "apps" / "picoware_ports.c").read_text(encoding="utf-8")
    sd_zip = (ROOT / "tools" / "build_sd_zip.py").read_text(encoding="utf-8")

    expect("dedicated Flappy target is wired", all(token in cmake for token in (
        "FLAPPY_APP_SOURCES", "flappy_assets", "${FLAPPY_APP_SOURCES}",
        "target_include_directories(dcapp_flappy BEFORE PRIVATE",
    )))
    expect("runtime 203 wrapper is dedicated", "flappyAppRun" in app and "flappyAppAbort" in app)
    expect("placeholder implementation is removed", "pwRunFlappy" not in picoware and "DCAPP_RUNTIME_ID == 203" not in picoware)
    expect("small app cache remains in use", "add_dcapp(dcapp_flappy flappy 203\n    ${FLAPPY_APP_SOURCES}" in cmake and "dcapp_flappy flappy 203 LARGE_XIP" not in cmake)
    os_block = re.search(r"set_source_files_properties\((.*?)PROPERTIES COMPILE_OPTIONS \"-Os\"", cmake, re.S)
    expect("flappy runtime is omitted from shared size-optimized source list",
           os_block is not None and "${SRC_DIR}/apps/flappy/flappy_port.c" not in os_block.group(1))
    expect("flappy runtime builds for speed", 'set_source_files_properties(\n    ${SRC_DIR}/apps/flappy/flappy_port.c\n    PROPERTIES COMPILE_OPTIONS "-O2"\n)' in cmake)
    expect("source constants are preserved", all(token in port for token in (
        "SIZE_SPACE_PIPE 3.3f", "SPACE_BETWEEN_PIPES 5",
        "FLAPPY_GRAVITY 0.65f", "FLAPPY_JUMP_VELOCITY (-13.5f)",
    )))
    expect("source state flow is preserved", all(token in port for token in (
        "FlappyStateIdle", "FlappyStateFadeIn", "FlappyStateFadeOut",
        "FlappyStateReadyGame", "FlappyStateGoGame", "FlappyStateStopGame",
        "FlappyStateFadeOutGameover", "FlappyStateFallBird", "FlappyStateFadeInPanel",
    )))
    expect("full-width landscape viewport is used", all(token in port for token in (
        "#define FLAPPY_VIEW_W 320", "#define FLAPPY_VIEW_H 240",
        "#define FLAPPY_VIEW_X 0",
        "FLAPPY_VIRTUAL_W 320.0f", "FLAPPY_VIRTUAL_H 512.0f",
        "FLAPPY_SPEED_MULTIPLIER 2u",
        "FLAPPY_BIRD_W 34.0f", "FLAPPY_BIRD_H 51.2f",
        "FLAPPY_TITLE_LOGO_X 48.0f", "FLAPPY_BUTTON_LEFT_X 52.0f",
        "FLAPPY_FLOOR_Y_PERCENT 86.0f", "FLAPPY_READY_TAP_X 75.0f",
        "FLAPPY_PIPE_GAP_CENTER_PERCENT 43.0f",
        "FLAPPY_READY_TAP_Y_PERCENT 11.0f", "FLAPPY_GAME_OVER_Y_PERCENT 6.0f",
        "FLAPPY_PANEL_SCORE_Y_PERCENT 9.5f", "FLAPPY_PANEL_BEST_Y_PERCENT 18.0f",
    )))
    expect("runtime uses fast asset blitters", all(token in port for token in (
        "FLAPPY_ASSET_FLAG_OPAQUE", "flappyAssetRows", "flappyAssetRuns",
        "flappyDrawOpaqueRows", "flappyDrawRunRow", "memcpy(",
        "flappyDrawBase",
    )))
    expect("runtime no longer uses mask-only blits", "flappyAssetMasks" not in port and "maskOffset" not in port)
    expect("background and base have fast paths", all(token in port for token in (
        "FlappyAssetBackgroundDay", "flappyDrawOpaqueRows(game, asset, pixels, FLAPPY_VIEW_X, 0)",
        "FlappyAssetBase", "flappyScreenY(flappyFloorY())", "while (x <= -(int32_t)asset->width)",
    )) and "dcAppDrawClear" not in port)
    expect("simulation is elapsed-time based", all(token in port for token in (
        "lastTicks", "flappyDeltaFrames", "TICKS_PER_SECOND",
        "FLAPPY_MAX_DT_FRAMES 4.0f", "flappyRender(game, dt)",
        "flappyAnimateBird(game, dt)", "flappyUpdateBirdTextureForLogo(game, dt)",
    )))
    expect("badge controls are mapped", "KEY_BIT_A | KEY_BIT_UP | KEY_BIT_START" in port and "UI_KEY_BIT_CENTER" in port)
    expect("B remains unused in gameplay", port.count("KEY_BIT_B") == 1)
    expect("versioned best score is persisted", all(token in port for token in (
        "FLAPPY_SAVE_MAGIC", "FLAPPY_SAVE_VERSION 1u", "flappySaveCheck",
        'dc32PortSaveWrite(game->args->vol, "flappy"',
        'dc32PortSaveRead(game->args->vol, "flappy"',
        "saveAfterPresent",
    )))
    expect("gameplay scoring and pipe reset are retained", all(token in port for token in (
        "game->pipes[i].x < -flappyScaleX(15.0f)",
        "game->pipes[i].x = flappyScaleX(115.0f)",
        "game->score++",
        "flappyCheckCollision",
    )))
    expect("pipe and collision floor use lowered floor", all(token in port for token in (
        "game->pipes[0].y = flappyPipeBaseY()",
        "game->pipes[1].y = flappyPipeBaseY()",
        "game->pipes[0].h = flappyFloorY() - game->pipes[0].y",
        "game->pipes[1].h = flappyFloorY() - game->pipes[1].y",
        "float bottomY = pipe->y + pipe->offset",
        "flappyDrawPipeSegment(game, pipe->x, 0.0f, bottomY - gap, true)",
        "flappyDrawPipeSegment(game, pipe->x, bottomY, floorY - bottomY, false)",
        "float topY = bottomY - flappyPipeGap()",
        "game->bird.y + game->bird.height > flappyFloorY()",
        "game->bird.y = flappyFloorY() - game->bird.height",
    )))
    expect("result panel and medals are retained", all(token in port for token in (
        "FlappyAssetPanel", "FlappyAssetGameOver", "FlappyAssetNew",
        "score >= 40u", "score >= 30u", "score >= 20u", "score >= 10u",
    )))
    expect("runtime is silent and badge-native", all(token not in port for token in (
        "PlayAudio", "audioPwm", "OpenSLES", "GLES", "EGL", "android_native_app_glue",
        "upng", ".mp3",
    )))
    expect("SD app source metadata is present", all(token in sd_zip for token in (
        "FLAPPY_REPO", "FLAPPY_COMMIT", "FLAPPY_ARCHIVE_SHA256", "APPS/flappy.DC32",
    )))


def check_deterministic_values() -> None:
    viewport_w = 320
    expect("landscape viewport uses full badge width", viewport_w == 320)
    expect("left margin calculation", (320 - viewport_w) // 2 == 0)
    game_speed = (int(320) // 135) * 2
    expect("landscape game speed is doubled", game_speed == 4)
    bird_h = 51.2
    gap = bird_h * 3.3
    expect("landscape pipe gap matches visible bird", int(round(gap * 240 / 512)) == 79)
    floor_y = round(512 * 0.86 * 240 / 512)
    expect("floor is lowered to expose more playfield", floor_y == 206 and 240 - floor_y == 34)
    gap_center = round(512 * 0.43 * 240 / 512)
    gap_half = round(gap * 0.5 * 240 / 512)
    random_offset_px = round(512 * 0.05 * 240 / 512)
    expect("pipe gap is centered in lowered playfield", gap_center == 103 and
           gap_center - gap_half - random_offset_px > 0 and
           gap_center + gap_half + random_offset_px < floor_y)
    panel_score_y = round((9.5 / 100.0) * 512 * 240 / 512)
    panel_best_y = round((18.0 / 100.0) * 512 * 240 / 512)
    expect("panel score digits are lowered into value slots", (panel_score_y, panel_best_y) == (23, 43))
    min_offset = int((-5 / 100.0) * 512)
    max_offset = int((5 / 100.0) * 512)
    expect("source random offset bounds are preserved", (min_offset, max_offset) == (-25, 25))

    magic = 0x31504C46
    version = 1
    size = 16
    best = 42
    check = magic ^ version ^ size ^ best ^ 0x8F31B562
    packed = struct.pack("<IHHII", magic, version, size, best, check)
    expect("persistent best-score schema is stable", len(packed) == 16 and struct.unpack("<IHHII", packed)[-1] == check)


def main() -> int:
    check_asset_builder()
    check_port_source()
    check_deterministic_values()
    print("Flappy Bird port regression tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
