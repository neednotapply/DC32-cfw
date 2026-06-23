#!/usr/bin/env python3
"""Generate a compact RGB332 Arkanoid sprite atlas from the pinned upstream source."""

from __future__ import annotations

import argparse
import binascii
import hashlib
import io
import re
import ssl
import struct
import sys
import urllib.request
import zipfile
import zlib
from pathlib import Path

sys.dont_write_bytecode = True


UPSTREAM_COMMIT = "7e0e876cd034ebd62890e65352c7ef0b12b45df5"
UPSTREAM_URL = f"https://codeload.github.com/wkeeling/arkanoid/zip/{UPSTREAM_COMMIT}"
UPSTREAM_ZIP_SHA256 = "2b633c078859548257089e3c3742a86b4ac699855d8c1aa806d25f03d0acb4a7"
GRAPHICS_PREFIX = f"arkanoid-{UPSTREAM_COMMIT}/arkanoid/data/graphics/"
ARENA_W = 222
ARENA_H = 240
SOURCE_W = 600
SOURCE_H = 650


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


def read_chunks(data: bytes) -> tuple[dict[bytes, list[bytes]], int, int, int, int]:
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("asset is not a PNG")
    chunks: dict[bytes, list[bytes]] = {}
    pos = 8
    width = height = bit_depth = color_type = -1
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
        if kind == b"IEND":
            break
    if width <= 0 or height <= 0 or b"IDAT" not in chunks:
        raise ValueError("PNG is missing required chunks")
    return chunks, width, height, bit_depth, color_type


def decode_png(data: bytes) -> tuple[int, int, list[tuple[int, int, int, int]]]:
    chunks, width, height, _bit_depth, color_type = read_chunks(data)
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
        palette = [tuple(payload[i : i + 3]) for i in range(0, len(payload), 3)]
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


def scaled_asset(data: bytes) -> tuple[int, int, list[int], list[int]]:
    width, height, source = decode_png(data)
    out_w = max(1, (width * ARENA_W + SOURCE_W // 2) // SOURCE_W)
    out_h = max(1, (height * ARENA_H + SOURCE_H // 2) // SOURCE_H)
    pixels: list[int] = []
    alpha: list[int] = []
    for y in range(out_h):
        sy = min(height - 1, y * height // out_h)
        for x in range(out_w):
            sx = min(width - 1, x * width // out_w)
            r, g, b, a = source[sy * width + sx]
            pixels.append(rgb332(r, g, b))
            alpha.append(1 if a >= 128 else 0)
    mask = [0] * ((len(alpha) + 7) // 8)
    for pos, opaque in enumerate(alpha):
        if opaque:
            mask[pos >> 3] |= 1 << (pos & 7)
    return out_w, out_h, pixels, mask


def enum_name(filename: str) -> str:
    stem = Path(filename).stem
    return "ArkAsset" + "".join(part.capitalize() for part in re.split(r"[^A-Za-z0-9]+", stem) if part)


def load_archive(cache: Path) -> bytes:
    data = cache.read_bytes() if cache.is_file() else b""
    if not data:
        request = urllib.request.Request(UPSTREAM_URL, headers={"User-Agent": "DC32-cfw asset builder"})
        # The archive is authenticated by its pinned SHA-256. Avoid depending
        # on the host Python certificate store, which is broken on some badge
        # development Windows installations.
        response = urllib.request.urlopen(
            request, timeout=60, context=ssl._create_unverified_context()
        )
        with response:
            data = response.read()
        cache.parent.mkdir(parents=True, exist_ok=True)
        cache.write_bytes(data)
    if hashlib.sha256(data).hexdigest() != UPSTREAM_ZIP_SHA256:
        raise ValueError(f"{cache} does not match the pinned upstream Arkanoid archive")
    return data


def write_header(path: Path, names: list[str]) -> None:
    enum_lines = [f"\t{enum_name(name)} = {index}," for index, name in enumerate(names)]
    text = f"""#ifndef ARKANOID_ASSETS_H
#define ARKANOID_ASSETS_H

#include <stdint.h>

struct ArkanoidAsset {{
\tuint32_t pixelOffset;
\tuint32_t maskOffset;
\tuint16_t width;
\tuint16_t height;
}};

enum ArkanoidAssetId {{
{chr(10).join(enum_lines)}
\tArkAssetCount = {len(names)}
}};

extern const struct ArkanoidAsset arkanoidAssets[ArkAssetCount];
extern const uint8_t arkanoidAssetPixels[];
extern const uint8_t arkanoidAssetMasks[];

#endif
"""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="ascii", newline="\n")


def format_bytes(values: list[int]) -> list[str]:
    return ["\t" + ", ".join(f"0x{value:02x}" for value in values[pos : pos + 24]) + ","
            for pos in range(0, len(values), 24)]


def write_source(
    path: Path,
    header_name: str,
    records: list[tuple[int, int, int, int]],
    pixels: list[int],
    masks: list[int],
) -> None:
    record_lines = [
        f"\t{{{pixel_offset}u, {mask_offset}u, {width}u, {height}u}},"
        for pixel_offset, mask_offset, width, height in records
    ]
    lines = [
        f"/* Generated from wkeeling/arkanoid commit {UPSTREAM_COMMIT}. */",
        f'#include "{header_name}"',
        "",
        "const struct ArkanoidAsset arkanoidAssets[ArkAssetCount] = {",
        *record_lines,
        "};",
        "",
        "const uint8_t arkanoidAssetPixels[] = {",
        *format_bytes(pixels),
        "};",
        "",
        "const uint8_t arkanoidAssetMasks[] = {",
        *format_bytes(masks),
        "};",
        "",
    ]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="ascii", newline="\n")


def main() -> int:
    args = parse_args()
    cache = (args.cache or Path("build/arkanoid/upstream.zip")).resolve()
    output_c = (args.output_c or Path("build/arkanoid/arkanoid_assets.c")).resolve()
    output_h = (args.output_h or Path("build/arkanoid/arkanoid_assets.h")).resolve()
    archive = load_archive(cache)
    with zipfile.ZipFile(io.BytesIO(archive)) as zf:
        names = sorted(
            Path(info.filename).name
            for info in zf.infolist()
            if info.filename.startswith(GRAPHICS_PREFIX) and info.filename.lower().endswith(".png")
        )
        records: list[tuple[int, int, int, int]] = []
        pixels: list[int] = []
        masks: list[int] = []
        for name in names:
            width, height, asset_pixels, asset_mask = scaled_asset(zf.read(GRAPHICS_PREFIX + name))
            records.append((len(pixels), len(masks), width, height))
            pixels.extend(asset_pixels)
            masks.extend(asset_mask)
    write_header(output_h, names)
    write_source(output_c, output_h.name, records, pixels, masks)
    print(f"Wrote {output_c} and {output_h}: {len(names)} sprites, {len(pixels)} RGB332 bytes")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
