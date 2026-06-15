#!/usr/bin/env python3
"""Generate compact DC32 Chip's Challenge tiles from Tile World assets."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

sys.dont_write_bytecode = True


SRC_TILE_SIZE = 48
OUT_TILE_SIZE = 24
TILE_COUNT = 128
ALPHA_BYTES = (OUT_TILE_SIZE * OUT_TILE_SIZE + 7) // 8
MAGENTA = (255, 0, 255)


# Derived from Tile World 1.3.0 oshw-sdl/sdltile.c tileidmap. The small
# tileset path uses xopaque/yopaque for every non-empty entry and treats
# magenta as transparent when xtransp is present.
TILE_POS = (
    (0x01, 0, 0, -1, -1),
    (0x02, 1, 2, -1, -1),
    (0x03, 1, 4, -1, -1),
    (0x04, 0, 13, -1, -1),
    (0x05, 1, 3, -1, -1),
    (0x06, 3, 2, -1, -1),
    (0x07, 0, 12, -1, -1),
    (0x08, 1, 12, -1, -1),
    (0x09, 1, 13, -1, -1),
    (0x0A, 1, 11, -1, -1),
    (0x0B, 1, 10, -1, -1),
    (0x0C, 2, 13, -1, -1),
    (0x0D, 0, 11, -1, -1),
    (0x0E, 0, 3, -1, -1),
    (0x0F, 0, 4, -1, -1),
    (0x10, 2, 10, -1, -1),
    (0x11, 2, 11, -1, -1),
    (0x12, 2, 1, -1, -1),
    (0x13, 2, 15, -1, -1),
    (0x14, 2, 8, -1, -1),
    (0x15, 2, 3, -1, -1),
    (0x16, 2, 4, -1, -1),
    (0x17, 2, 7, -1, -1),
    (0x18, 2, 9, -1, -1),
    (0x19, 0, 1, -1, -1),
    (0x1A, 0, 6, -1, -1),
    (0x1B, 0, 7, -1, -1),
    (0x1C, 0, 8, -1, -1),
    (0x1D, 0, 9, -1, -1),
    (0x1E, 3, 0, -1, -1),
    (0x1F, 0, 5, -1, -1),
    (0x20, 2, 12, -1, -1),
    (0x21, 1, 14, -1, -1),
    (0x22, 1, 15, -1, -1),
    (0x23, 2, 6, -1, -1),
    (0x24, 2, 5, -1, -1),
    (0x25, 2, 14, -1, -1),
    (0x26, 3, 1, -1, -1),
    (0x27, 1, 7, -1, -1),
    (0x28, 1, 6, -1, -1),
    (0x29, 1, 9, -1, -1),
    (0x2A, 1, 8, -1, -1),
    (0x2B, 2, 2, -1, -1),
    (0x2C, 1, 5, -1, -1),
    (0x2D, 0, 2, -1, -1),
    (0x2E, 6, 5, 9, 5),
    (0x2F, 6, 4, 9, 4),
    (0x30, 6, 7, 9, 7),
    (0x31, 6, 6, 9, 6),
    (0x32, 6, 10, 9, 10),
    (0x33, 6, 11, 9, 11),
    (0x34, 6, 9, 9, 9),
    (0x35, 6, 8, 9, 8),
    (0x36, 0, 10, -1, -1),
    (0x3D, 2, 0, -1, -1),
    (0x3B, 3, 10, -1, -1),
    (0x3C, 3, 11, -1, -1),
    (0x38, 3, 4, -1, -1),
    (0x39, 3, 5, -1, -1),
    (0x3A, 3, 9, -1, -1),
    (0x37, 3, 3, -1, -1),
    (0x6C, 3, 12, -1, -1),
    (0x6D, 3, 13, -1, -1),
    (0x6E, 3, 14, -1, -1),
    (0x6F, 3, 15, -1, -1),
    (0x40, 6, 12, 9, 12),
    (0x41, 6, 13, 9, 13),
    (0x42, 6, 14, 9, 14),
    (0x43, 6, 15, 9, 15),
    (0x70, 6, 12, 9, 12),
    (0x71, 6, 13, 9, 13),
    (0x72, 6, 14, 9, 14),
    (0x73, 6, 15, 9, 15),
    (0x44, 0, 14, -1, -1),
    (0x45, 0, 15, -1, -1),
    (0x46, 1, 0, -1, -1),
    (0x47, 1, 1, -1, -1),
    (0x48, 4, 12, 7, 12),
    (0x49, 4, 13, 7, 13),
    (0x4A, 4, 14, 7, 14),
    (0x4B, 4, 15, 7, 15),
    (0x4C, 4, 8, 7, 8),
    (0x4D, 4, 9, 7, 9),
    (0x4E, 4, 10, 7, 10),
    (0x4F, 4, 11, 7, 11),
    (0x50, 5, 0, 8, 0),
    (0x51, 5, 1, 8, 1),
    (0x52, 5, 2, 8, 2),
    (0x53, 5, 3, 8, 3),
    (0x54, 4, 4, 7, 4),
    (0x55, 4, 5, 7, 5),
    (0x56, 4, 6, 7, 6),
    (0x57, 4, 7, 7, 7),
    (0x64, 4, 0, 7, 0),
    (0x65, 4, 1, 7, 1),
    (0x66, 4, 2, 7, 2),
    (0x67, 4, 3, 7, 3),
    (0x68, 6, 0, 9, 0),
    (0x69, 6, 1, 9, 1),
    (0x6A, 6, 2, 9, 2),
    (0x6B, 6, 3, 9, 3),
    (0x60, 5, 4, 8, 4),
    (0x61, 5, 5, 8, 5),
    (0x62, 5, 6, 8, 6),
    (0x63, 5, 7, 8, 7),
    (0x5C, 5, 12, 8, 12),
    (0x5D, 5, 13, 8, 13),
    (0x5E, 5, 14, 8, 14),
    (0x5F, 5, 15, 8, 15),
    (0x58, 5, 8, 8, 8),
    (0x59, 5, 9, 8, 9),
    (0x5A, 5, 10, 8, 10),
    (0x5B, 5, 11, 8, 11),
    (0x7C, 3, 3, -1, -1),
    (0x7D, 3, 6, -1, -1),
    (0x7E, 3, 7, -1, -1),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, help="Path to vendored Tile World source tree")
    parser.add_argument("--output-c", type=Path)
    parser.add_argument("--output-h", type=Path)
    return parser.parse_args()


def interactive() -> bool:
    return sys.stdin.isatty()


def prompt_path(label: str, default: Path) -> Path:
    while True:
        raw = input(f"{label} [{default}]: ").strip().strip('"')
        if not raw:
            return default
        if raw:
            return Path(raw)
        print("Please enter a path.")


def resolve_inputs(args: argparse.Namespace) -> tuple[Path, Path, Path]:
    if args.source_root is None:
        default = Path("third_party") / "tworld"
        args.source_root = prompt_path("Tile World source root", default) if interactive() else default
    if args.output_c is None:
        default = Path("build") / "tworld" / "tworld_assets.c"
        args.output_c = prompt_path("Output C file", default) if interactive() else default
    if args.output_h is None:
        default = Path("build") / "tworld" / "tworld_assets.h"
        args.output_h = prompt_path("Output header file", default) if interactive() else default
    return args.source_root.resolve(), args.output_c.resolve(), args.output_h.resolve()


def rgb332(rgb: tuple[int, int, int]) -> int:
    r, g, b = rgb
    return (r & 0xE0) | ((g >> 3) & 0x1C) | (b >> 6)


def read_bmp24(path: Path) -> tuple[int, int, list[tuple[int, int, int]]]:
    data = path.read_bytes()
    if data[:2] != b"BM":
        raise ValueError(f"{path} is not a BMP")
    offset = struct.unpack_from("<I", data, 10)[0]
    width = struct.unpack_from("<i", data, 18)[0]
    height = struct.unpack_from("<i", data, 22)[0]
    planes = struct.unpack_from("<H", data, 26)[0]
    bpp = struct.unpack_from("<H", data, 28)[0]
    compression = struct.unpack_from("<I", data, 30)[0]
    if width <= 0 or height == 0 or planes != 1 or bpp != 24 or compression != 0:
        raise ValueError(f"{path} must be an uncompressed 24-bit BMP")
    abs_h = abs(height)
    stride = ((width * bpp + 31) // 32) * 4
    pixels: list[tuple[int, int, int]] = []
    for y in range(abs_h):
        source_y = abs_h - 1 - y if height > 0 else y
        row = offset + source_y * stride
        for x in range(width):
            b, g, r = data[row + x * 3 : row + x * 3 + 3]
            pixels.append((r, g, b))
    return width, abs_h, pixels


def pixel(pixels: list[tuple[int, int, int]], width: int, x: int, y: int) -> tuple[int, int, int]:
    return pixels[y * width + x]


def extract_tile(
    pixels: list[tuple[int, int, int]],
    width: int,
    x_tile: int,
    y_tile: int,
    transparent: bool,
) -> tuple[list[int], list[int]]:
    out = [0] * (OUT_TILE_SIZE * OUT_TILE_SIZE)
    alpha = [0] * ALPHA_BYTES
    for y in range(OUT_TILE_SIZE):
        for x in range(OUT_TILE_SIZE):
            src = pixel(pixels, width, x_tile * SRC_TILE_SIZE + x * 2, y_tile * SRC_TILE_SIZE + y * 2)
            idx = y * OUT_TILE_SIZE + x
            if transparent and src == MAGENTA:
                continue
            out[idx] = rgb332(src)
            alpha[idx // 8] |= 1 << (idx % 8)
    return out, alpha


def c_array(values: list[int], indent: str = "\t\t", columns: int = 24) -> str:
    return ",\n".join(
        indent + ", ".join(f"0x{value:02x}" for value in values[i : i + columns])
        for i in range(0, len(values), columns)
    )


def write_header(path: Path) -> None:
    text = f"""#ifndef TWORLD_ASSETS_H
#define TWORLD_ASSETS_H

#include <stdint.h>

#define TWORLD_TILE_COUNT {TILE_COUNT}u
#define TWORLD_TILE_SIZE {OUT_TILE_SIZE}u
#define TWORLD_TILE_ALPHA_BYTES {ALPHA_BYTES}u

extern const uint8_t tworldTilePixels[TWORLD_TILE_COUNT][TWORLD_TILE_SIZE * TWORLD_TILE_SIZE];
extern const uint8_t tworldTileAlpha[TWORLD_TILE_COUNT][TWORLD_TILE_ALPHA_BYTES];

#endif
"""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="ascii", newline="\n")


def write_source(path: Path, header_name: str, tiles: list[list[int]], alpha: list[list[int]]) -> None:
    lines = [
        "/* Generated from third_party/tworld/res/tiles.bmp by tools/build_tworld_assets.py. */",
        f'#include "{header_name}"',
        "",
        "const uint8_t tworldTilePixels[TWORLD_TILE_COUNT][TWORLD_TILE_SIZE * TWORLD_TILE_SIZE] = {",
    ]
    for tile_id, values in enumerate(tiles):
        lines.append(f"\t/* tile 0x{tile_id:02x} */")
        lines.append("\t{")
        lines.append(c_array(values))
        lines.append("\t},")
    lines.extend(["};", "", "const uint8_t tworldTileAlpha[TWORLD_TILE_COUNT][TWORLD_TILE_ALPHA_BYTES] = {"])
    for tile_id, values in enumerate(alpha):
        lines.append(f"\t/* tile 0x{tile_id:02x} */")
        lines.append("\t{ " + ", ".join(f"0x{value:02x}" for value in values) + " },")
    lines.append("};")
    lines.append("")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="ascii", newline="\n")


def main() -> int:
    args = parse_args()
    source_root, output_c, output_h = resolve_inputs(args)
    bmp = source_root / "res" / "tiles.bmp"
    width, height, pixels = read_bmp24(bmp)
    if width != 7 * SRC_TILE_SIZE or height != 16 * SRC_TILE_SIZE:
        raise ValueError(f"{bmp} is {width}x{height}, expected 336x768")

    tiles = [[0] * (OUT_TILE_SIZE * OUT_TILE_SIZE) for _ in range(TILE_COUNT)]
    alpha = [[0] * ALPHA_BYTES for _ in range(TILE_COUNT)]
    for tile_id, xopaque, yopaque, xtransp, ytransp in TILE_POS:
        if tile_id >= TILE_COUNT:
            raise ValueError(f"tile id 0x{tile_id:02x} exceeds generated table")
        tiles[tile_id], alpha[tile_id] = extract_tile(
            pixels, width, xopaque, yopaque, xtransp >= 0 or ytransp >= 0
        )

    write_header(output_h)
    write_source(output_c, output_h.name, tiles, alpha)
    print(f"Wrote {output_c} and {output_h} from Tile World tiles.bmp")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
