#!/usr/bin/env python3
"""Generate the DC32 T-Rex grayscale sprite asset from the pinned Wayou PNG."""

from __future__ import annotations

import argparse
import binascii
import hashlib
import struct
import sys
import zlib
from pathlib import Path

sys.dont_write_bytecode = True


EXPECTED_SHA256 = "e306705c996676db01f4072ed3d6f33d89089a848ab0b2a0ba07a2d866ec309f"
EXPECTED_WIDTH = 1233
EXPECTED_HEIGHT = 68
TRANSPARENT_GRAY = 0
SPRITE_CROPS = {
    "RESTART": (2, 2, 36, 32),
    "CLOUD": (86, 2, 46, 14),
    "PTERODACTYL": (134, 2, 92, 40),
    "CACTUS_SMALL": (228, 2, 102, 35),
    "CACTUS_LARGE": (332, 2, 150, 50),
    "MOON": (484, 2, 160, 40),
    "STAR": (645, 2, 9, 18),
    "TEXT_SPRITE": (655, 2, 191, 24),
    "TREX": (848, 2, 382, 47),
    "HORIZON": (2, 54, 1200, 12),
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path)
    parser.add_argument("--output-c", type=Path)
    parser.add_argument("--output-h", type=Path)
    return parser.parse_args()


def resolve_inputs(args: argparse.Namespace) -> tuple[Path, Path, Path]:
    source = args.input or Path("third_party/t-rex-runner/assets/default_100_percent/100-offline-sprite.png")
    output_c = args.output_c or Path("build/trex/trex_assets.c")
    output_h = args.output_h or Path("build/trex/trex_assets.h")
    return source.resolve(), output_c.resolve(), output_h.resolve()


def paeth(a: int, b: int, c: int) -> int:
    value = a + b - c
    pa = abs(value - a)
    pb = abs(value - b)
    pc = abs(value - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c


def decode_png(path: Path) -> tuple[int, int, int, list[int]]:
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != EXPECTED_SHA256:
        raise ValueError(f"{path} does not match pinned T-Rex sprite SHA-256")
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path} is not a PNG")

    pos = 8
    width = height = transparent = -1
    compressed: list[bytes] = []
    saw_iend = False
    while pos + 12 <= len(data):
        size = struct.unpack_from(">I", data, pos)[0]
        kind = data[pos + 4 : pos + 8]
        payload = data[pos + 8 : pos + 8 + size]
        crc = struct.unpack_from(">I", data, pos + 8 + size)[0]
        if binascii.crc32(kind + payload) & 0xFFFFFFFF != crc:
            raise ValueError(f"{path} has a corrupt {kind.decode('ascii', 'replace')} chunk")
        pos += size + 12

        if kind == b"IHDR":
            width, height, bit_depth, color_type, compression, filtering, interlace = struct.unpack(
                ">IIBBBBB", payload
            )
            if (
                bit_depth != 8
                or color_type != 0
                or compression != 0
                or filtering != 0
                or interlace != 0
            ):
                raise ValueError(f"{path} must be a non-interlaced 8-bit grayscale PNG")
        elif kind == b"tRNS":
            if len(payload) != 2:
                raise ValueError(f"{path} has malformed grayscale transparency")
            transparent = struct.unpack(">H", payload)[0]
        elif kind == b"IDAT":
            compressed.append(payload)
        elif kind == b"IEND":
            saw_iend = True
            break

    if not saw_iend or width <= 0 or height <= 0 or not compressed:
        raise ValueError(f"{path} is missing required PNG chunks")
    if (width, height) != (EXPECTED_WIDTH, EXPECTED_HEIGHT):
        raise ValueError(f"{path} is {width}x{height}, expected {EXPECTED_WIDTH}x{EXPECTED_HEIGHT}")
    if transparent != TRANSPARENT_GRAY:
        raise ValueError(f"{path} transparent grayscale value is {transparent}, expected {TRANSPARENT_GRAY}")

    raw = zlib.decompress(b"".join(compressed))
    stride = width
    expected = height * (stride + 1)
    if len(raw) != expected:
        raise ValueError(f"{path} decoded to {len(raw)} bytes, expected {expected}")

    pixels: list[int] = []
    previous = bytearray(stride)
    offset = 0
    for _ in range(height):
        filter_type = raw[offset]
        offset += 1
        row = bytearray(raw[offset : offset + stride])
        offset += stride
        for x in range(stride):
            left = row[x - 1] if x else 0
            above = previous[x]
            upper_left = previous[x - 1] if x else 0
            if filter_type == 1:
                row[x] = (row[x] + left) & 0xFF
            elif filter_type == 2:
                row[x] = (row[x] + above) & 0xFF
            elif filter_type == 3:
                row[x] = (row[x] + ((left + above) >> 1)) & 0xFF
            elif filter_type == 4:
                row[x] = (row[x] + paeth(left, above, upper_left)) & 0xFF
            elif filter_type != 0:
                raise ValueError(f"{path} uses unsupported PNG filter {filter_type}")
        pixels.extend(row)
        previous = row
    return width, height, transparent, pixels


def validate_crops(width: int, height: int) -> None:
    for name, (x, y, crop_w, crop_h) in SPRITE_CROPS.items():
        if x < 0 or y < 0 or crop_w <= 0 or crop_h <= 0 or x + crop_w > width or y + crop_h > height:
            raise ValueError(f"{name} crop {x},{y} {crop_w}x{crop_h} exceeds {width}x{height}")


def write_header(path: Path) -> None:
    text = f"""#ifndef TREX_ASSETS_H
#define TREX_ASSETS_H

#include <stdint.h>

#define TREX_SPRITE_WIDTH {EXPECTED_WIDTH}u
#define TREX_SPRITE_HEIGHT {EXPECTED_HEIGHT}u
#define TREX_SPRITE_PIXELS (TREX_SPRITE_WIDTH * TREX_SPRITE_HEIGHT)
#define TREX_SPRITE_TRANSPARENT {TRANSPARENT_GRAY}u

extern const uint8_t trexSpritePixels[TREX_SPRITE_PIXELS];

#endif
"""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="ascii", newline="\n")


def write_source(path: Path, header_name: str, pixels: list[int]) -> None:
    lines = [
        "/* Generated from wayou/t-rex-runner commit 5455bfa408ec6b707c7300ff194b7390733a766d. */",
        f'#include "{header_name}"',
        "",
        "const uint8_t trexSpritePixels[TREX_SPRITE_PIXELS] = {",
    ]
    for pos in range(0, len(pixels), 24):
        lines.append("\t" + ", ".join(f"0x{value:02x}" for value in pixels[pos : pos + 24]) + ",")
    lines.extend(["};", ""])
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="ascii", newline="\n")


def main() -> int:
    args = parse_args()
    source, output_c, output_h = resolve_inputs(args)
    width, height, _transparent, pixels = decode_png(source)
    validate_crops(width, height)
    if len(pixels) != EXPECTED_WIDTH * EXPECTED_HEIGHT:
        raise ValueError("generated raster size is incorrect")
    write_header(output_h)
    write_source(output_c, output_h.name, pixels)
    print(f"Wrote {output_c} and {output_h} from pinned {width}x{height} T-Rex sprite")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
