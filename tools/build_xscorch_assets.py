#!/usr/bin/env python3
"""Generate compact badge weapon tables from xscorch data files."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True


FLAG_BITS = {
    "null": 0x00000001,
    "defer": 0x00000002,
    "smoke": 0x00000004,
    "plasma": 0x00000100,
    "liquid": 0x00000200,
    "dirt": 0x00000400,
    "beam": 0x00000800,
    "riot": 0x00001000,
    "triple": 0x00002000,
    "roller": 0x00004000,
    "digger": 0x00008000,
    "sapper": 0x00010000,
    "scatter": 0x01000000,
    "spider": 0x02000000,
    "chain": 0x04000000,
    "at_apex": 0x10000000,
    "at_land": 0x20000000,
    "at_rand": 0x40000000,
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path)
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
        default = Path("third_party") / "xscorch"
        args.source_root = prompt_path("xscorch source root", default) if interactive() else default
    if args.output_c is None:
        default = Path("build") / "xscorch" / "xscorch_assets.c"
        args.output_c = prompt_path("Output C file", default) if interactive() else default
    if args.output_h is None:
        default = Path("build") / "xscorch" / "xscorch_assets.h"
        args.output_h = prompt_path("Output header file", default) if interactive() else default
    return args.source_root.resolve(), args.output_c.resolve(), args.output_h.resolve()


def c_string(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def field_text(body: str, name: str, default: str = "") -> str:
    quoted = re.search(rf"\b{name}\s*=\s*\"([^\"]*)\"", body, flags=re.S)
    if quoted:
        return quoted.group(1)
    match = re.search(rf"\b{name}\s*=\s*([^;\r\n]+)", body, flags=re.S)
    if not match:
        return default
    value = match.group(1).strip()
    return value


def field_int(body: str, name: str, default: int = 0) -> int:
    value = field_text(body, name, str(default))
    try:
        return int(float(value))
    except ValueError:
        return default


def field_bool(body: str, name: str, default: bool = False) -> bool:
    value = field_text(body, name, "true" if default else "false").lower()
    return value in {"true", "yes", "1"}


def parse_flags(*values: str) -> int:
    out = 0
    for value in values:
        for token in re.split(r"[, \t\r\n]+", value.strip()):
            token = token.strip().lower()
            if token:
                out |= FLAG_BITS.get(token, 0)
    return out


def parse_weapons(path: Path) -> list[dict[str, object]]:
    text = path.read_text(encoding="utf-8", errors="replace")
    weapons: list[dict[str, object]] = []
    for match in re.finditer(r"(\w+)\s*=\s*weapons_class\s*\{(.*?)\};", text, flags=re.S):
        ident, body = match.groups()
        weapons.append({
            "ident": ident,
            "name": field_text(body, "weaponName", ident),
            "description": field_text(body, "description", ""),
            "arms": field_int(body, "armsLevel"),
            "price": field_int(body, "price"),
            "bundle": field_int(body, "bundle"),
            "radius": field_int(body, "radius"),
            "force": field_int(body, "force"),
            "liquid": field_int(body, "liquid"),
            "scatter": field_int(body, "scatter"),
            "children": field_int(body, "children"),
            "useless": field_bool(body, "useless"),
            "indirect": field_bool(body, "indirect"),
            "flags": parse_flags(field_text(body, "stateFlags"), field_text(body, "phoenixFlags")),
            "child": field_text(body, "phoenixChild", ""),
        })
    if len(weapons) < 8:
        raise ValueError(f"expected xscorch weapon table in {path}")
    return weapons


def main() -> int:
    args = parse_args()
    root, output_c, output_h = resolve_inputs(args)
    weapons = parse_weapons(root / "data" / "weapons.def")
    output_h.parent.mkdir(parents=True, exist_ok=True)
    output_c.parent.mkdir(parents=True, exist_ok=True)

    output_h.write_text(f"""#ifndef XSCORCH_ASSETS_H
#define XSCORCH_ASSETS_H

#include <stdbool.h>
#include <stdint.h>

#define XSCORCH_WEAPON_COUNT {len(weapons)}u
#define XSCORCH_WEAPON_FLAG_PLASMA 0x00000100u
#define XSCORCH_WEAPON_FLAG_LIQUID 0x00000200u
#define XSCORCH_WEAPON_FLAG_DIRT 0x00000400u
#define XSCORCH_WEAPON_FLAG_BEAM 0x00000800u
#define XSCORCH_WEAPON_FLAG_RIOT 0x00001000u
#define XSCORCH_WEAPON_FLAG_TRIPLE 0x00002000u
#define XSCORCH_WEAPON_FLAG_ROLLER 0x00004000u
#define XSCORCH_WEAPON_FLAG_DIGGER 0x00008000u
#define XSCORCH_WEAPON_FLAG_SCATTER 0x01000000u
#define XSCORCH_WEAPON_FLAG_SPIDER 0x02000000u

struct XscorchWeaponInfo {{
\tconst char *ident;
\tconst char *name;
\tconst char *description;
\tconst char *child;
\tint16_t armsLevel;
\tint32_t price;
\tint16_t bundle;
\tint16_t radius;
\tint16_t force;
\tint16_t liquid;
\tint16_t scatter;
\tint16_t children;
\tuint32_t flags;
\tbool useless;
\tbool indirect;
}};

extern const struct XscorchWeaponInfo xscorchWeapons[XSCORCH_WEAPON_COUNT];

#endif
""", encoding="ascii", newline="\n")

    lines = [
        '#include "xscorch_assets.h"',
        "",
        "const struct XscorchWeaponInfo xscorchWeapons[XSCORCH_WEAPON_COUNT] = {",
    ]
    for index, weapon in enumerate(weapons):
        lines.append(f"\t/* {index}: {weapon['ident']} */")
        lines.append(
            "\t{"
            f"{c_string(str(weapon['ident']))}, "
            f"{c_string(str(weapon['name']))}, "
            f"{c_string(str(weapon['description']))}, "
            f"{c_string(str(weapon['child']))}, "
            f"{weapon['arms']}, {weapon['price']}, {weapon['bundle']}, "
            f"{weapon['radius']}, {weapon['force']}, {weapon['liquid']}, "
            f"{weapon['scatter']}, {weapon['children']}, "
            f"0x{int(weapon['flags']):08x}u, "
            f"{'true' if weapon['useless'] else 'false'}, "
            f"{'true' if weapon['indirect'] else 'false'}"
            "},"
        )
    lines.append("};")
    output_c.write_text("\n".join(lines) + "\n", encoding="ascii", newline="\n")
    print(f"Wrote {output_c} and {output_h} from xscorch weapons.def")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
