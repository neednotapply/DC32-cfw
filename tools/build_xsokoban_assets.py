#!/usr/bin/env python3
"""Generate compact DC32 Sokoban assets from the public-domain XSokoban tree."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True


LEVEL_COUNT = 90
MAX_ROWS = 20
MAX_COLS = 20
TILE_SIZE = 20
WALL_NAMES = (
    "lonewall",
    "southwall",
    "westwall",
    "llcornerwall",
    "northwall",
    "vertiwall",
    "ulcornerwall",
    "west_twall",
    "eastwall",
    "lrcornerwall",
    "horizwall",
    "south_twall",
    "urcornerwall",
    "east_twall",
    "north_twall",
    "centerwall",
)
BASE_TILES = ("floor", "goal", "man", "saveman", "object", "treasure")
ALL_TILES = (*BASE_TILES, *WALL_NAMES)
LEGAL_LEVEL_CHARS = {"@", "+", ".", "$", "*", " ", "#"}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, help="Path to vendored xsokoban source tree")
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
        default = Path("third_party") / "xsokoban"
        args.source_root = prompt_path("XSokoban source root", default) if interactive() else default
    if args.output_c is None:
        default = Path("build") / "xsokoban" / "xsokoban_assets.c"
        args.output_c = prompt_path("Output C file", default) if interactive() else default
    if args.output_h is None:
        default = Path("build") / "xsokoban" / "xsokoban_assets.h"
        args.output_h = prompt_path("Output header file", default) if interactive() else default
    return args.source_root.resolve(), args.output_c.resolve(), args.output_h.resolve()


def rgb332(hex_color: str) -> int:
    if not hex_color.startswith("#") or len(hex_color) != 7:
        return 0
    r = int(hex_color[1:3], 16)
    g = int(hex_color[3:5], 16)
    b = int(hex_color[5:7], 16)
    return (r & 0xE0) | ((g >> 3) & 0x1C) | (b >> 6)


def xpm_strings(path: Path) -> list[str]:
    strings: list[str] = []
    for line in path.read_text(encoding="ascii").splitlines():
        match = re.search(r'"(.*)"', line)
        if match:
            strings.append(match.group(1))
    if not strings:
        raise ValueError(f"{path} does not contain XPM strings")
    return strings


def parse_xpm(path: Path) -> list[int]:
    strings = xpm_strings(path)
    width, height, ncolors, cpp = (int(part) for part in strings[0].split()[:4])
    if width != TILE_SIZE or height != TILE_SIZE:
        raise ValueError(f"{path} is {width}x{height}, expected {TILE_SIZE}x{TILE_SIZE}")
    colors: dict[str, int] = {}
    for line in strings[1 : 1 + ncolors]:
        key = line[:cpp]
        parts = line[cpp:].strip().split()
        color = "None"
        for idx, part in enumerate(parts):
            if part == "c" and idx + 1 < len(parts):
                color = parts[idx + 1]
                break
        colors[key] = rgb332(color)
    pixels: list[int] = []
    for row in strings[1 + ncolors : 1 + ncolors + height]:
        if len(row) != width * cpp:
            raise ValueError(f"{path} has malformed pixel row")
        for x in range(width):
            pixels.append(colors[row[x * cpp : (x + 1) * cpp]])
    return pixels


def parse_screen(path: Path) -> tuple[int, int, list[str]]:
    lines = path.read_text(encoding="ascii").splitlines()
    if not lines:
        raise ValueError(f"{path} is empty")
    rows = len(lines)
    cols = max(len(line) for line in lines)
    if rows > MAX_ROWS or cols > MAX_COLS:
        raise ValueError(f"{path} is {rows}x{cols}, max is {MAX_ROWS}x{MAX_COLS}")
    packets = goals = players = 0
    cells = [" "] * (MAX_ROWS * MAX_COLS)
    for y, line in enumerate(lines):
        for x, ch in enumerate(line):
            if ch not in LEGAL_LEVEL_CHARS:
                raise ValueError(f"{path} has illegal character {ch!r}")
            if ch in "$*":
                packets += 1
            if ch in ".*+":
                goals += 1
            if ch in "@+":
                players += 1
            cells[y * MAX_COLS + x] = ch
    if packets != goals or packets == 0:
        raise ValueError(f"{path} has {packets} packets and {goals} goals")
    if players != 1:
        raise ValueError(f"{path} has {players} players")
    return rows, cols, cells


def c_u8_array(values: list[int], indent: str = "\t") -> str:
    rows = []
    for i in range(0, len(values), 20):
        rows.append(indent + ", ".join(f"0x{v:02x}" for v in values[i : i + 20]))
    return ",\n".join(rows)


def c_char_array(values: list[str], indent: str = "\t\t") -> str:
    rows = []
    for i in range(0, len(values), MAX_COLS):
        rows.append(indent + ", ".join(f"'{ch}'" if ch != "'" else "'\\''" for ch in values[i : i + MAX_COLS]))
    return ",\n".join(rows)


def write_header(path: Path) -> None:
    enum_lines = [
        "\tXsokobanTileFloor,",
        "\tXsokobanTileGoal,",
        "\tXsokobanTileMan,",
        "\tXsokobanTileSaveman,",
        "\tXsokobanTileObject,",
        "\tXsokobanTileTreasure,",
    ]
    for idx, name in enumerate(WALL_NAMES):
        enum_lines.append(f"\tXsokobanTileWall{idx},")
    text = f"""#ifndef XSOKOBAN_ASSETS_H
#define XSOKOBAN_ASSETS_H

#include <stdint.h>

#define XSOKOBAN_LEVEL_COUNT {LEVEL_COUNT}u
#define XSOKOBAN_MAX_ROWS {MAX_ROWS}u
#define XSOKOBAN_MAX_COLS {MAX_COLS}u
#define XSOKOBAN_TILE_SIZE {TILE_SIZE}u

enum XsokobanTileId {{
{chr(10).join(enum_lines)}
\tXsokobanTileCount,
}};

struct XsokobanLevel {{
\tuint8_t rows;
\tuint8_t cols;
\tchar cells[XSOKOBAN_MAX_ROWS * XSOKOBAN_MAX_COLS];
}};

extern const struct XsokobanLevel xsokobanLevels[XSOKOBAN_LEVEL_COUNT];
extern const uint8_t xsokobanTiles[XsokobanTileCount][XSOKOBAN_TILE_SIZE * XSOKOBAN_TILE_SIZE];

#endif
"""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="ascii", newline="\n")


def write_source(path: Path, header_name: str, levels: list[tuple[int, int, list[str]]], tiles: list[list[int]]) -> None:
    lines = [
        "/* Generated from third_party/xsokoban by tools/build_xsokoban_assets.py. */",
        f'#include "{header_name}"',
        "",
        "const struct XsokobanLevel xsokobanLevels[XSOKOBAN_LEVEL_COUNT] = {",
    ]
    for idx, (rows, cols, cells) in enumerate(levels, 1):
        lines.append(f"\t/* screen.{idx} */")
        lines.append(f"\t{{{rows}u, {cols}u, {{")
        lines.append(c_char_array(cells))
        lines.append("\t}},")
    lines.extend(["};", "", "const uint8_t xsokobanTiles[XsokobanTileCount][XSOKOBAN_TILE_SIZE * XSOKOBAN_TILE_SIZE] = {"])
    for name, pixels in zip(ALL_TILES, tiles):
        lines.append(f"\t/* {name}.xpm */")
        lines.append("\t{")
        lines.append(c_u8_array(pixels, "\t\t"))
        lines.append("\t},")
    lines.append("};")
    lines.append("")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="ascii", newline="\n")


def main() -> int:
    args = parse_args()
    root, output_c, output_h = resolve_inputs(args)
    screens = root / "screens"
    bitmaps = root / "bitmaps" / "20"
    if not screens.is_dir():
        raise FileNotFoundError(f"missing xsokoban screens directory: {screens}")
    if not bitmaps.is_dir():
        raise FileNotFoundError(f"missing xsokoban 20px bitmap directory: {bitmaps}")
    levels = [parse_screen(screens / f"screen.{idx}") for idx in range(1, LEVEL_COUNT + 1)]
    tiles = [parse_xpm(bitmaps / f"{name}.xpm") for name in ALL_TILES]
    write_header(output_h)
    write_source(output_c, output_h.name, levels, tiles)
    print(f"Wrote {output_c} and {output_h} from {LEVEL_COUNT} XSokoban screens")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
