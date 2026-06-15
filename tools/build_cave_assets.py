#!/usr/bin/env python3
"""Generate compact Cave Story NXEngine sprite metadata for the badge port."""

from __future__ import annotations

import argparse
import re
import struct
import sys
from pathlib import Path

sys.dont_write_bytecode = True


SIF_SECTION_SHEETS = 1
SIF_SECTION_SPRITES = 2
S_DIR_END = 0
S_DIR_DRAW_POINT = 1
S_DIR_ACTION_POINT = 2
S_DIR_ACTION_POINT_2 = 3
S_DIR_PF_BBOX = 4


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate DC32 Cave Story NXEngine sprite metadata.")
    parser.add_argument("--nxengine-root", type=Path, default=Path("third_party") / "nxengine-evo")
    parser.add_argument("--output-c", type=Path, help="Path to write cave_assets.c")
    parser.add_argument("--output-h", type=Path, help="Path to write cave_assets.h")
    return parser.parse_args()


def interactive() -> bool:
    return sys.stdin.isatty()


def prompt_path(label: str, default: Path | None = None) -> Path:
    suffix = f" [{default}]" if default else ""
    while True:
        try:
            raw = input(f"{label}{suffix}: ").strip().strip('"')
        except EOFError:
            if default is not None:
                return default
            raise ValueError(f"missing required path: {label}") from None
        if not raw and default is not None:
            return default
        if raw:
            return Path(raw)
        print("Please enter a path.")


def resolve_inputs(args: argparse.Namespace) -> argparse.Namespace:
    if args.nxengine_root is None:
        args.nxengine_root = prompt_path("Path to nxengine-evo source tree", Path("third_party") / "nxengine-evo")
    if args.output_c is None:
        args.output_c = prompt_path("Output C path", Path("build") / "cave" / "cave_assets.c") if interactive() else Path("build") / "cave" / "cave_assets.c"
    if args.output_h is None:
        args.output_h = prompt_path("Output header path", Path("build") / "cave" / "cave_assets.h") if interactive() else Path("build") / "cave" / "cave_assets.h"
    args.nxengine_root = args.nxengine_root.resolve()
    args.output_c = args.output_c.resolve()
    args.output_h = args.output_h.resolve()
    return args


def read_u8(data: bytes, pos: int) -> tuple[int, int]:
    if pos >= len(data):
        raise ValueError("SIF read past end")
    return data[pos], pos + 1


def read_u16(data: bytes, pos: int) -> tuple[int, int]:
    if pos + 2 > len(data):
        raise ValueError("SIF read past end")
    return struct.unpack_from("<H", data, pos)[0], pos + 2


def read_i16(data: bytes, pos: int) -> tuple[int, int]:
    value, pos = read_u16(data, pos)
    return struct.unpack("<h", struct.pack("<H", value))[0], pos


def read_point(data: bytes, pos: int) -> tuple[tuple[int, int], int]:
    x, pos = read_i16(data, pos)
    y, pos = read_i16(data, pos)
    return (x, y), pos


def read_rect(data: bytes, pos: int) -> tuple[tuple[int, int, int, int], int]:
    x1, pos = read_i16(data, pos)
    y1, pos = read_i16(data, pos)
    x2, pos = read_i16(data, pos)
    y2, pos = read_i16(data, pos)
    return (x1, y1, x2, y2), pos


def load_sif_sections(path: Path) -> dict[int, bytes]:
    raw = path.read_bytes()
    if len(raw) < 5 or raw[:4] != b"2FIS":
        raise ValueError(f"{path} is not an NXEngine SIF2 file")
    section_count = raw[4]
    pos = 5
    sections: dict[int, bytes] = {}
    for _ in range(section_count):
        if pos + 9 > len(raw):
            raise ValueError(f"{path} has a truncated SIF index")
        section_type = raw[pos]
        offset = struct.unpack_from("<I", raw, pos + 1)[0]
        length = struct.unpack_from("<I", raw, pos + 5)[0]
        pos += 9
        if offset > len(raw) or length > len(raw) - offset:
            raise ValueError(f"{path} has an invalid SIF section range")
        sections[section_type] = raw[offset : offset + length]
    return sections


def parse_pascal_strings(data: bytes) -> list[str]:
    count, pos = read_u16(data, 0)
    strings: list[str] = []
    for _ in range(count):
        length, pos = read_u8(data, pos)
        if length == 255:
            length, pos = read_u16(data, pos)
        if pos + length > len(data):
            raise ValueError("truncated SIF string array")
        strings.append(data[pos : pos + length].decode("utf-8", "replace"))
        pos += length
    return strings


def parse_sprites(data: bytes) -> list[dict[str, object]]:
    count, pos = read_u16(data, 0)
    sprites: list[dict[str, object]] = []
    for _ in range(count):
        width, pos = read_u8(data, pos)
        height, pos = read_u8(data, pos)
        sheet, pos = read_u8(data, pos)
        frame_count, pos = read_u8(data, pos)
        dir_count, pos = read_u8(data, pos)
        if dir_count > 4:
            raise ValueError("SIF sprite has more than four directions")
        for _dir in range(dir_count):
            _bbox, pos = read_rect(data, pos)
        _solid, pos = read_rect(data, pos)
        _spawn, pos = read_point(data, pos)
        for _list in range(4):
            point_count, pos = read_u8(data, pos)
            for _point in range(point_count):
                _pt, pos = read_point(data, pos)
        dirs: list[tuple[int, int, int, int]] = []
        for _frame in range(frame_count):
            for _dir in range(dir_count):
                sheet_offset, pos = read_point(data, pos)
                drawpoint = (0, 0)
                while True:
                    field, pos = read_u8(data, pos)
                    if field == S_DIR_END:
                        break
                    if field == S_DIR_DRAW_POINT:
                        drawpoint, pos = read_point(data, pos)
                    elif field in (S_DIR_ACTION_POINT, S_DIR_ACTION_POINT_2):
                        _point, pos = read_point(data, pos)
                    elif field == S_DIR_PF_BBOX:
                        _rect, pos = read_rect(data, pos)
                    else:
                        raise ValueError(f"unknown SIF direction field {field}")
                dirs.append((sheet_offset[0], sheet_offset[1], drawpoint[0], drawpoint[1]))
        sprites.append(
            {
                "width": width,
                "height": height,
                "sheet": sheet,
                "frames": frame_count,
                "dirs": dir_count,
                "entries": dirs,
            }
        )
    return sprites


def parse_defines(path: Path, prefix: str) -> dict[str, int]:
    out: dict[str, int] = {}
    pattern = re.compile(rf"^\s*#define\s+({re.escape(prefix)}[A-Za-z0-9_]+)\s+(\d+)\b")
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = pattern.match(line)
        if match:
            out[match.group(1)] = int(match.group(2))
    return out


def parse_assignments(path: Path, objects: dict[str, int], sprites: dict[str, int]) -> list[int]:
    max_object = max(objects.values()) if objects else 0
    mapping = [0] * (max_object + 1)
    pattern = re.compile(r"ASSIGN_SPRITE\((OBJ_[A-Za-z0-9_]+),\s*(SPR_[A-Za-z0-9_]+)\)")
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = pattern.search(line)
        if not match:
            continue
        obj = objects.get(match.group(1))
        sprite = sprites.get(match.group(2))
        if obj is not None and sprite is not None:
            mapping[obj] = sprite
    return mapping


def c_string(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def write_outputs(root: Path, output_c: Path, output_h: Path) -> None:
    sif = root / "data" / "sprites.sif"
    sprites_h = root / "src" / "autogen" / "sprites.h"
    object_h = root / "src" / "object.h"
    assign_cpp = root / "src" / "autogen" / "AssignSprites.cpp"
    for path in (sif, sprites_h, object_h, assign_cpp):
        if not path.is_file():
            raise FileNotFoundError(path)

    sections = load_sif_sections(sif)
    sheets = parse_pascal_strings(sections[SIF_SECTION_SHEETS])
    sprites = parse_sprites(sections[SIF_SECTION_SPRITES])
    sprite_defines = parse_defines(sprites_h, "SPR_")
    object_defines = parse_defines(object_h, "OBJ_")
    object_sprites = parse_assignments(assign_cpp, object_defines, sprite_defines)

    first_dirs: list[int] = []
    flat_dirs: list[tuple[int, int, int, int]] = []
    for sprite in sprites:
        first_dirs.append(len(flat_dirs))
        flat_dirs.extend(sprite["entries"])  # type: ignore[arg-type]

    output_h.parent.mkdir(parents=True, exist_ok=True)
    output_c.parent.mkdir(parents=True, exist_ok=True)
    output_h.write_text(
        """#ifndef CAVE_ASSETS_H
#define CAVE_ASSETS_H

#include <stdint.h>

struct CaveNxSpriteMeta {
\tuint16_t firstDir;
\tuint16_t sheet;
\tuint8_t w;
\tuint8_t h;
\tuint8_t nframes;
\tuint8_t ndirs;
};

struct CaveNxSpriteDir {
\tint16_t sx;
\tint16_t sy;
\tint16_t dx;
\tint16_t dy;
};

extern const uint16_t caveNxSheetCount;
extern const char *const caveNxSheetNames[];
extern const uint16_t caveNxSpriteCount;
extern const struct CaveNxSpriteMeta caveNxSprites[];
extern const struct CaveNxSpriteDir caveNxSpriteDirs[];
extern const uint16_t caveNxObjectSpriteCount;
extern const uint16_t caveNxObjectSprites[];

#endif
""",
        encoding="utf-8",
    )

    lines: list[str] = [
        "/* Generated by tools/build_cave_assets.py from NXEngine-evo sprites.sif. */",
        '#include "cave_assets.h"',
        "",
        f"const uint16_t caveNxSheetCount = {len(sheets)};",
        "const char *const caveNxSheetNames[] = {",
    ]
    lines.extend(f"\t{c_string(name)}," for name in sheets)
    lines.extend(
        [
            "};",
            "",
            f"const uint16_t caveNxSpriteCount = {len(sprites)};",
            "const struct CaveNxSpriteMeta caveNxSprites[] = {",
        ]
    )
    for index, sprite in enumerate(sprites):
        lines.append(
            "\t{ "
            f"{first_dirs[index]}, {sprite['sheet']}, {sprite['width']}, {sprite['height']}, "
            f"{sprite['frames']}, {sprite['dirs']} "
            "},"
        )
    lines.extend(
        [
            "};",
            "",
            "const struct CaveNxSpriteDir caveNxSpriteDirs[] = {",
        ]
    )
    lines.extend(f"\t{{ {sx}, {sy}, {dx}, {dy} }}," for sx, sy, dx, dy in flat_dirs)
    lines.extend(
        [
            "};",
            "",
            f"const uint16_t caveNxObjectSpriteCount = {len(object_sprites)};",
            "const uint16_t caveNxObjectSprites[] = {",
        ]
    )
    for index in range(0, len(object_sprites), 16):
        values = ", ".join(str(value) for value in object_sprites[index : index + 16])
        lines.append(f"\t{values},")
    lines.extend(["};", ""])
    output_c.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {output_c} and {output_h} from NXEngine sprites.sif")


def main() -> int:
    args = resolve_inputs(parse_args())
    write_outputs(args.nxengine_root, args.output_c, args.output_h)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
