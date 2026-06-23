#!/usr/bin/env python3
"""Generate compact DC32 Flappy Bird assets from the pinned upstream source."""

from __future__ import annotations

import argparse
import binascii
import hashlib
import io
import ssl
import struct
import sys
import urllib.request
import zipfile
import zlib
from pathlib import Path

sys.dont_write_bytecode = True


UPSTREAM_COMMIT = "3b3060cdf6b38b819e5a649bc92d11776decd0b4"
UPSTREAM_TAG = "v1.9.2"
UPSTREAM_URL = f"https://codeload.github.com/VadimBoev/FlappyBird/zip/{UPSTREAM_COMMIT}"
UPSTREAM_ZIP_SHA256 = "e06e8553e747be5851edfe90486dda3afcb5b2a9ddac4827f4a7911a6fb39d51"
ASSET_PREFIX_SUFFIX = "FlappyBird/app/src/main/assets/"
SOURCE_W = 320
SOURCE_H = 512
VIEWPORT_W = 320
VIEWPORT_H = 240
BASE_Y_PERCENT = 86.0
BUTTON_W = 80
BUTTON_H = 28
LOGO_W = 192
LOGO_H = 44
BIRD_W = 34
BIRD_H = 24
READY_TAP_W = 170
READY_TAP_H = 135
FLAPPY_ASSET_FLAG_OPAQUE = 0x0001
ASSET_CROPS = {
    "sprites/message.png": (25, 160, 135, 107),
}


def sx(percent: float) -> int:
    return max(1, int(round((percent / 100.0) * VIEWPORT_W)))


def sy(percent: float) -> int:
    return max(1, int(round((percent / 100.0) * VIEWPORT_H)))


ASSETS = [
    ("BackgroundDay", "sprites/background-day.png", sx(100.0), sy(BASE_Y_PERCENT)),
    ("Base", "sprites/base.png", sx(100.0), sy(25.0)),
    ("Start", "buttons/start.png", BUTTON_W, BUTTON_H),
    ("ScoreButton", "buttons/score.png", BUTTON_W, BUTTON_H),
    ("Ok", "buttons/ok.png", BUTTON_W, BUTTON_H),
    ("Share", "buttons/share.png", BUTTON_W, BUTTON_H),
    ("Logo", "sprites/logo.png", LOGO_W, LOGO_H),
    ("Message", "sprites/message.png", READY_TAP_W, READY_TAP_H),
    ("GameOver", "sprites/gameover.png", 192, 42),
    ("Panel", "sprites/panel.png", sx(70.0), 80),
    ("New", "sprites/new.png", 32, 14),
    ("PipeGreen", "sprites/pipe-green.png", sx(15.0), int(round(sx(15.0) * 320.0 / 52.0))),
    ("BronzeMedal", "sprites/bronze-medal.png", 44, 44),
    ("SilverMedal", "sprites/silver-medal.png", 44, 44),
    ("GoldMedal", "sprites/gold-medal.png", 44, 44),
    ("PlatinumMedal", "sprites/platinum-medal.png", 44, 44),
    ("YellowBirdDown", "sprites/yellowbird-downflap.png", BIRD_W, BIRD_H),
    ("YellowBirdMid", "sprites/yellowbird-midflap.png", BIRD_W, BIRD_H),
    ("YellowBirdUp", "sprites/yellowbird-upflap.png", BIRD_W, BIRD_H),
    *[(f"Digit{digit}", f"sprites/{digit}.png", 18, 27) for digit in range(10)],
    *[(f"Digit{digit}Small", f"sprites/{digit}_small.png", 10, 14) for digit in range(10)],
]

EXPECTED_DIMS = {
    "buttons/ok.png": (80, 28),
    "buttons/score.png": (80, 28),
    "buttons/share.png": (80, 28),
    "buttons/start.png": (80, 28),
    "sprites/background-day.png": (288, 512),
    "sprites/base.png": (336, 111),
    "sprites/bronze-medal.png": (44, 44),
    "sprites/gameover.png": (192, 42),
    "sprites/gold-medal.png": (44, 44),
    "sprites/logo.png": (192, 44),
    "sprites/message.png": (184, 267),
    "sprites/new.png": (32, 14),
    "sprites/panel.png": (226, 116),
    "sprites/pipe-green.png": (52, 320),
    "sprites/platinum-medal.png": (44, 44),
    "sprites/silver-medal.png": (44, 44),
    "sprites/yellowbird-downflap.png": (34, 24),
    "sprites/yellowbird-midflap.png": (34, 24),
    "sprites/yellowbird-upflap.png": (34, 24),
    **{f"sprites/{digit}.png": (24, 36) for digit in range(10)},
    **{
        f"sprites/{digit}_small.png": ((10 if digit == 1 else 14), 20)
        for digit in range(10)
    },
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache", type=Path)
    parser.add_argument("--output-c", type=Path)
    parser.add_argument("--output-h", type=Path)
    return parser.parse_args()


def paeth(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c


def read_chunks(data: bytes) -> tuple[dict[bytes, list[bytes]], int, int, int]:
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("asset is not a PNG")
    chunks: dict[bytes, list[bytes]] = {}
    pos = 8
    width = height = color_type = -1
    while pos + 12 <= len(data):
        size = struct.unpack_from(">I", data, pos)[0]
        kind = data[pos + 4 : pos + 8]
        payload = data[pos + 8 : pos + 8 + size]
        crc = struct.unpack_from(">I", data, pos + 8 + size)[0]
        if binascii.crc32(kind + payload) & 0xFFFFFFFF != crc:
            raise ValueError(f"corrupt PNG {kind!r} chunk")
        pos += size + 12
        chunks.setdefault(kind, []).append(payload)
        if kind == b"IHDR":
            width, height, bit_depth, color_type, compression, filtering, interlace = struct.unpack(
                ">IIBBBBB", payload
            )
            if bit_depth != 8 or compression or filtering or interlace:
                raise ValueError("only non-interlaced 8-bit PNG assets are supported")
        elif kind == b"IEND":
            break
    if width <= 0 or height <= 0 or b"IDAT" not in chunks:
        raise ValueError("PNG is missing required chunks")
    return chunks, width, height, color_type


def decode_png(data: bytes) -> tuple[int, int, list[tuple[int, int, int, int]]]:
    chunks, width, height, color_type = read_chunks(data)
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}.get(color_type)
    if channels is None:
        raise ValueError(f"unsupported PNG color type {color_type}")
    stride = width * channels
    raw = zlib.decompress(b"".join(chunks[b"IDAT"]))
    if len(raw) != height * (stride + 1):
        raise ValueError("unexpected PNG raster size")
    palette: list[tuple[int, int, int]] = []
    if color_type == 3:
        payload = chunks.get(b"PLTE", [b""])[0]
        palette = [tuple(payload[pos : pos + 3]) for pos in range(0, len(payload), 3)]
    alpha_table = chunks.get(b"tRNS", [b""])[0]
    transparent_gray = transparent_rgb = None
    if color_type == 0 and len(alpha_table) >= 2:
        transparent_gray = struct.unpack(">H", alpha_table[:2])[0] & 0xFF
    elif color_type == 2 and len(alpha_table) >= 6:
        transparent_rgb = tuple(v & 0xFF for v in struct.unpack(">HHH", alpha_table[:6]))

    pixels: list[tuple[int, int, int, int]] = []
    previous = bytearray(stride)
    offset = 0
    for _ in range(height):
        filter_type = raw[offset]
        offset += 1
        row = bytearray(raw[offset : offset + stride])
        offset += stride
        for x in range(stride):
            left = row[x - channels] if x >= channels else 0
            above = previous[x]
            upper_left = previous[x - channels] if x >= channels else 0
            if filter_type == 1:
                row[x] = (row[x] + left) & 0xFF
            elif filter_type == 2:
                row[x] = (row[x] + above) & 0xFF
            elif filter_type == 3:
                row[x] = (row[x] + ((left + above) >> 1)) & 0xFF
            elif filter_type == 4:
                row[x] = (row[x] + paeth(left, above, upper_left)) & 0xFF
            elif filter_type != 0:
                raise ValueError(f"unsupported PNG filter {filter_type}")
        for x in range(width):
            pos = x * channels
            if color_type == 0:
                value = row[pos]
                alpha = 0 if value == transparent_gray else 255
                pixels.append((value, value, value, alpha))
            elif color_type == 2:
                rgb = tuple(row[pos : pos + 3])
                pixels.append((*rgb, 0 if rgb == transparent_rgb else 255))
            elif color_type == 3:
                index = row[pos]
                if index >= len(palette):
                    raise ValueError("PNG palette index is out of bounds")
                pixels.append((*palette[index], alpha_table[index] if index < len(alpha_table) else 255))
            elif color_type == 4:
                pixels.append((row[pos], row[pos], row[pos], row[pos + 1]))
            else:
                pixels.append(tuple(row[pos : pos + 4]))
        previous = row
    return width, height, pixels


def rgb332(r: int, g: int, b: int) -> int:
    return (r & 0xE0) | ((g >> 3) & 0x1C) | (b >> 6)


def load_archive(cache: Path) -> bytes:
    data = cache.read_bytes() if cache.is_file() else b""
    if not data:
        request = urllib.request.Request(UPSTREAM_URL, headers={"User-Agent": "DC32-cfw asset builder"})
        response = urllib.request.urlopen(
            request, timeout=60, context=ssl._create_unverified_context()
        )
        with response:
            data = response.read()
        cache.parent.mkdir(parents=True, exist_ok=True)
        cache.write_bytes(data)
    if hashlib.sha256(data).hexdigest() != UPSTREAM_ZIP_SHA256:
        raise ValueError(f"{cache} does not match the pinned FlappyBird archive")
    return data


def archive_entry(zf: zipfile.ZipFile, asset_path: str) -> str:
    suffix = ASSET_PREFIX_SUFFIX + asset_path
    matches = [info.filename for info in zf.infolist() if info.filename.endswith(suffix)]
    if len(matches) != 1:
        raise ValueError(f"expected one archive entry ending in {suffix}, found {len(matches)}")
    return matches[0]


def scaled_asset(
    data: bytes, asset_path: str, out_w: int, out_h: int
) -> tuple[list[int], int, list[tuple[int, int]], list[tuple[int, int]]]:
    width, height, source = decode_png(data)
    expected = EXPECTED_DIMS.get(asset_path)
    if expected and (width, height) != expected:
        raise ValueError(f"{asset_path} is {width}x{height}, expected {expected[0]}x{expected[1]}")
    crop_x, crop_y, crop_w, crop_h = ASSET_CROPS.get(asset_path, (0, 0, width, height))
    if crop_x < 0 or crop_y < 0 or crop_w <= 0 or crop_h <= 0 or crop_x + crop_w > width or crop_y + crop_h > height:
        raise ValueError(f"{asset_path} has invalid crop rectangle")
    pixels: list[int] = []
    alpha: list[int] = []
    for y in range(out_h):
        sy_i = crop_y + min(crop_h - 1, y * crop_h // out_h)
        for x in range(out_w):
            sx_i = crop_x + min(crop_w - 1, x * crop_w // out_w)
            r, g, b, a = source[sy_i * width + sx_i]
            opaque = a >= 128 and (r != 0 or g != 0 or b != 0)
            pixels.append(rgb332(r, g, b) if opaque else 0)
            alpha.append(1 if opaque else 0)
    if all(alpha):
        return pixels, FLAPPY_ASSET_FLAG_OPAQUE, [], []

    rows: list[tuple[int, int]] = []
    runs: list[tuple[int, int]] = []
    for y in range(out_h):
        run_offset = len(runs)
        x = 0
        row_start = y * out_w
        while x < out_w:
            while x < out_w and not alpha[row_start + x]:
                x += 1
            start = x
            while x < out_w and alpha[row_start + x]:
                x += 1
            if x > start:
                runs.append((start, x - start))
        rows.append((run_offset, len(runs) - run_offset))
    return pixels, 0, rows, runs


def enum_name(name: str) -> str:
    return "FlappyAsset" + name


def format_bytes(values: list[int]) -> list[str]:
    return ["\t" + ", ".join(f"0x{value:02x}" for value in values[pos : pos + 24]) + ","
            for pos in range(0, len(values), 24)]


def write_header(path: Path) -> None:
    enum_lines = [f"\t{enum_name(name)} = {idx}," for idx, (name, _path, _w, _h) in enumerate(ASSETS)]
    text = f"""#ifndef FLAPPY_ASSETS_H
#define FLAPPY_ASSETS_H

#include <stdint.h>

#define FLAPPY_ASSET_VIEWPORT_W {VIEWPORT_W}u
#define FLAPPY_ASSET_VIEWPORT_H {VIEWPORT_H}u
#define FLAPPY_ASSET_FLAG_OPAQUE 0x0001u

struct FlappyAsset {{
\tuint32_t pixelOffset;
\tuint32_t rowOffset;
\tuint16_t width;
\tuint16_t height;
\tuint16_t flags;
\tuint16_t reserved;
}};

struct FlappyAssetRow {{
\tuint16_t runOffset;
\tuint16_t runCount;
}};

struct FlappyAssetRun {{
\tuint16_t x;
\tuint16_t len;
}};

enum FlappyAssetId {{
{chr(10).join(enum_lines)}
\tFlappyAssetCount = {len(ASSETS)}
}};

extern const struct FlappyAsset flappyAssets[FlappyAssetCount];
extern const uint8_t flappyAssetPixels[];
extern const struct FlappyAssetRow flappyAssetRows[];
extern const struct FlappyAssetRun flappyAssetRuns[];

#endif
"""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="ascii", newline="\n")


def write_source(
    path: Path,
    header_name: str,
    records: list[tuple[int, int, int, int, int]],
    pixels: list[int],
    rows: list[tuple[int, int]],
    runs: list[tuple[int, int]],
) -> None:
    record_lines = [
        f"\t{{{pixel_offset}u, {row_offset}u, {width}u, {height}u, {flags}u, 0u}},"
        for pixel_offset, row_offset, width, height, flags in records
    ]
    row_lines = [
        f"\t{{{run_offset}u, {run_count}u}},"
        for run_offset, run_count in rows
    ]
    run_lines = [
        f"\t{{{x}u, {length}u}},"
        for x, length in runs
    ]
    lines = [
        f"/* Generated from VadimBoev/FlappyBird {UPSTREAM_TAG} commit {UPSTREAM_COMMIT}. */",
        f'#include "{header_name}"',
        "",
        "const struct FlappyAsset flappyAssets[FlappyAssetCount] = {",
        *record_lines,
        "};",
        "",
        "const uint8_t flappyAssetPixels[] = {",
        *format_bytes(pixels),
        "};",
        "",
        "const struct FlappyAssetRow flappyAssetRows[] = {",
        *row_lines,
        "};",
        "",
        "const struct FlappyAssetRun flappyAssetRuns[] = {",
        *run_lines,
        "};",
        "",
    ]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="ascii", newline="\n")


def main() -> int:
    args = parse_args()
    cache = (args.cache or Path("build/flappy/upstream.zip")).resolve()
    output_c = (args.output_c or Path("build/flappy/flappy_assets.c")).resolve()
    output_h = (args.output_h or Path("build/flappy/flappy_assets.h")).resolve()
    archive = load_archive(cache)
    records: list[tuple[int, int, int, int, int]] = []
    pixels: list[int] = []
    rows: list[tuple[int, int]] = []
    runs: list[tuple[int, int]] = []
    opaque_count = 0
    with zipfile.ZipFile(io.BytesIO(archive)) as zf:
        for name, asset_path, width, height in ASSETS:
            asset_pixels, flags, asset_rows, asset_runs = scaled_asset(
                zf.read(archive_entry(zf, asset_path)), asset_path, width, height
            )
            row_offset = 0 if flags & FLAPPY_ASSET_FLAG_OPAQUE else len(rows)
            records.append((len(pixels), row_offset, width, height, flags))
            pixels.extend(asset_pixels)
            rows.extend((run_offset + len(runs), run_count) for run_offset, run_count in asset_rows)
            runs.extend(asset_runs)
            if flags & FLAPPY_ASSET_FLAG_OPAQUE:
                opaque_count += 1
    write_header(output_h)
    write_source(output_c, output_h.name, records, pixels, rows, runs)
    print(
        f"Wrote {output_c} and {output_h}: {len(ASSETS)} sprites, "
        f"{len(pixels)} RGB332 bytes, {opaque_count} opaque sprites, "
        f"{len(rows)} rows, {len(runs)} runs"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
