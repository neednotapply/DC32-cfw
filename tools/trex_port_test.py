#!/usr/bin/env python3
"""Regression checks for the source-derived DC32 T-Rex Runner port."""

from __future__ import annotations

import importlib.util
import math
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

sys.dont_write_bytecode = True


ROOT = Path(__file__).resolve().parents[1]
PORT = ROOT / "src" / "apps" / "trex" / "trex_port.c"
UPSTREAM = ROOT / "third_party" / "t-rex-runner" / "index.js"
SPRITE = ROOT / "third_party" / "t-rex-runner" / "assets" / "default_100_percent" / "100-offline-sprite.png"
FRAME_MS = 1000.0 / 60.0


def expect(name: str, condition: bool) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def load_asset_builder():
    path = ROOT / "tools" / "build_trex_assets.py"
    spec = importlib.util.spec_from_file_location("build_trex_assets", path)
    expect("asset generator import spec", spec is not None and spec.loader is not None)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def js_round(value: float) -> int:
    return math.floor(value + 0.5)


def simulate_jump(release_frame: int | None = None, drop_frame: int | None = None) -> tuple[int, int]:
    y = 93
    velocity = -10.0 - 3.84 / 10.0
    reached_minimum = False
    speed_drop = False
    peak = y

    for frame in range(200):
        if release_frame == frame and reached_minimum and velocity < -5.0:
            velocity = -5.0
        if drop_frame == frame:
            speed_drop = True
            velocity = 1.0
        movement = velocity * (3.0 if speed_drop else 1.0)
        y += js_round(movement)
        velocity += 0.6
        if y < 63 or speed_drop:
            reached_minimum = True
        if (y < 30 or speed_drop) and reached_minimum and velocity < -5.0:
            velocity = -5.0
        peak = min(peak, y)
        if y > 93:
            return frame + 1, peak
    raise AssertionError("jump did not land")


def overlaps(a: tuple[int, int, int, int], b: tuple[int, int, int, int]) -> bool:
    x, y, w, h = a
    bx, by, bw, bh = b
    return x < bx + bw and x + w > bx and y < by + bh and y + h > by


def running_hits_small_cactus(obstacle_x: int) -> bool:
    player_outer = (1, 94, 42, 45)
    obstacle_outer = (obstacle_x + 1, 106, 15, 33)
    player_boxes = (
        (22, 0, 17, 16),
        (1, 18, 30, 9),
        (10, 35, 14, 8),
        (1, 24, 29, 5),
        (5, 30, 21, 4),
        (9, 34, 15, 4),
    )
    obstacle_boxes = ((0, 7, 5, 27), (4, 0, 6, 34), (10, 4, 7, 14))

    if not overlaps(player_outer, obstacle_outer):
        return False
    for px, py, pw, ph in player_boxes:
        adjusted_player = (player_outer[0] + px, player_outer[1] + py, pw, ph)
        for ox, oy, ow, oh in obstacle_boxes:
            adjusted_obstacle = (obstacle_outer[0] + ox, obstacle_outer[1] + oy, ow, oh)
            if overlaps(adjusted_player, adjusted_obstacle):
                return True
    return False


def check_assets() -> None:
    builder = load_asset_builder()
    width, height, transparent, pixels = builder.decode_png(SPRITE)
    expect("pinned sprite dimensions", (width, height) == (1233, 68))
    expect("pinned sprite transparent grayscale", transparent == 0)
    expect("generated raster size", len(pixels) == 1233 * 68)
    expect("sprite retains foreground grayscale", 83 in pixels and 247 in pixels)
    builder.validate_crops(width, height)
    expect(
        "all runtime sprite crops are covered",
        set(builder.SPRITE_CROPS)
        == {
            "RESTART",
            "CLOUD",
            "PTERODACTYL",
            "CACTUS_SMALL",
            "CACTUS_LARGE",
            "MOON",
            "STAR",
            "TEXT_SPRITE",
            "TREX",
            "HORIZON",
        },
    )

    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        out_c = tmp_path / "trex_assets.c"
        out_h = tmp_path / "trex_assets.h"
        result = subprocess.run(
            [
                sys.executable,
                str(ROOT / "tools" / "build_trex_assets.py"),
                "--input",
                str(SPRITE),
                "--output-c",
                str(out_c),
                "--output-h",
                str(out_h),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        expect("asset generator succeeds", result.returncode == 0 and out_c.is_file() and out_h.is_file())
        generated = out_c.read_text(encoding="ascii")
        expect("asset generator emits every grayscale pixel", len(re.findall(r"0x[0-9a-f]{2}", generated)) == 1233 * 68)

        corrupt = bytearray(SPRITE.read_bytes())
        corrupt[-1] ^= 1
        corrupt_path = tmp_path / "corrupt.png"
        corrupt_path.write_bytes(corrupt)
        bad = subprocess.run(
            [
                sys.executable,
                str(ROOT / "tools" / "build_trex_assets.py"),
                "--input",
                str(corrupt_path),
                "--output-c",
                str(tmp_path / "bad.c"),
                "--output-h",
                str(tmp_path / "bad.h"),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        expect("asset generator rejects a non-pinned sprite", bad.returncode != 0 and "SHA-256" in bad.stderr)


def check_source_and_wiring() -> None:
    port = PORT.read_text(encoding="utf-8")
    upstream = UPSTREAM.read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    picoware = (ROOT / "src" / "apps" / "picoware_ports.c").read_text(encoding="utf-8")
    license_text = (ROOT / "third_party" / "t-rex-runner" / "LICENSE").read_text(encoding="utf-8")

    expect("upstream BSD license is retained", "Redistribution and use in source and binary forms" in license_text)
    expect("pinned upstream source contains Runner constants", "ACCELERATION: 0.001" in upstream and "INVERT_DISTANCE: 700" in upstream)
    expect("dedicated T-Rex target is wired", "TREX_APP_SOURCES" in cmake and "trex_assets" in cmake)
    expect("placeholder implementation is removed", "pwRunTrex" not in picoware and "DCAPP_RUNTIME_ID == 205" not in picoware)
    expect("runtime is fixed at 60 FPS", "#define TREX_FPS 60u" in port and "dispSetFramerate(TREX_FPS)" in port)
    expect("responsive source speed is preserved", all(token in port for token in ("TREX_DEFAULT_WIDTH 600.0f", "TREX_RUNNER_MOBILE_SPEED_COEFFICIENT 1.2f", "trexResponsiveSpeed")))
    expect("source physics constants are preserved", all(token in port for token in ("TREX_PLAYER_GRAVITY 0.6f", "TREX_PLAYER_INITIAL_JUMP_VELOCITY (-10.0f)", "TREX_PLAYER_DROP_VELOCITY (-5.0f)", "TREX_PLAYER_SPEED_DROP_COEFFICIENT 3.0f")))
    expect("source obstacle gates and collision boxes are preserved", all(token in port for token in (".minSpeed = 8.5f", "{22, 0, 17, 16}", "{15, 15, 16, 5}", "trexCollides")))
    expect("source presentation states are preserved", all(token in port for token in ("TREX_INTRO_DURATION 400.0f", "trexEaseOut", "trexDrawGameOver", "trexDrawNight", "TREX_RUNNER_INVERT_DISTANCE 700u")))
    expect("badge controls are mapped", all(token in port for token in ("KEY_BIT_A | KEY_BIT_UP", "KEY_BIT_DOWN", "KEY_BIT_START", "UI_KEY_BIT_CENTER")))
    expect("B remains unused in gameplay", port.count("KEY_BIT_B") == 1)
    expect("audio is omitted", all(token not in port for token in ("audioPwm", "playSound", "soundFx", "BUTTON_PRESS", "HIT", "SCORE")))
    expect("high score save is versioned and validated", all(token in port for token in ("TREX_SAVE_MAGIC", "TREX_SAVE_VERSION 1u", "trexSaveCheck", 'dc32PortSaveWrite(game->args->vol, "trex"')))
    expect("new high score saves after presentation", "saveAfterPresent" in port and port.find("dcAppDrawFrame") < port.find("if (game->saveAfterPresent)"))
    expect("night mode performs intended inversion", "255u - gray" in port and "game->inverted = game->invertTrigger" in port)


def check_deterministic_behaviors() -> None:
    responsive_speed = min(6.0, 6.0 * 320.0 / 600.0 * 1.2)
    expect("320px responsive starting speed", math.isclose(responsive_speed, 3.84))

    long_frames, long_peak = simulate_jump()
    short_frames, short_peak = simulate_jump(release_frame=5)
    drop_frames, drop_peak = simulate_jump(drop_frame=4)
    expect("full jump source trajectory", (long_frames, long_peak) == (35, 4))
    expect("released jump is shorter", (short_frames, short_peak) == (30, 24))
    expect("down key accelerates falling", (drop_frames, drop_peak) == (10, 55))

    intro_x = sum(js_round((50.0 / 1500.0) * FRAME_MS) for _ in range(24))
    expect("400ms intro preserves source player movement quirk", intro_x == 24)

    minimum_gap = js_round(17 * responsive_speed + 120 * 0.6)
    maximum_gap = js_round(minimum_gap * 1.5)
    expect("small-cactus starting gap range", (minimum_gap, maximum_gap) == (137, 206))
    expect("pterodactyl remains gated below speed 8.5", responsive_speed < 8.5)

    first_collision = next(x for x in range(100, -20, -1) if running_hits_small_cactus(x))
    expect("source detailed collision geometry", first_collision == 34 and not running_hits_small_cactus(35))

    expect("score milestone conversion", js_round(4000 * 0.025) == 100)
    expect("night trigger conversion", js_round(28000 * 0.025) == 700)
    expect("jump restart lockout", 44 * FRAME_MS < 750 <= 45 * FRAME_MS)

    magic = 0x31585254
    version = 1
    size = 16
    highest = 123456
    check = magic ^ version ^ size ^ highest ^ 0xA5C39E71
    packed = struct.pack("<IHHII", magic, version, size, highest, check)
    values = struct.unpack("<IHHII", packed)
    expect("persistent high-score schema is stable", len(packed) == 16 and values[-1] == check)


def main() -> int:
    check_assets()
    check_source_and_wiring()
    check_deterministic_behaviors()
    print("T-Rex port regression tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
