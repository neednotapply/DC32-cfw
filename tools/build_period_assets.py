#!/usr/bin/env python3
"""Build redistributable period-port asset packs from checked-out source trees."""

from __future__ import annotations

import argparse
import binascii
import json
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

sys.dont_write_bytecode = True


VERSION = 1
HEADER_FMT = "<12sII"
ENTRY_FMT = "<HHIII"
HEADER_SIZE = struct.calcsize(HEADER_FMT)
ENTRY_SIZE = struct.calcsize(ENTRY_FMT)


@dataclass(frozen=True)
class AssetSource:
    key: str
    pak_name: str
    magic: bytes
    required_any: tuple[str, ...]
    include_suffixes: tuple[str, ...]
    reference: str


SOURCES = {
    "tworld": AssetSource(
        key="tworld",
        pak_name="chips-tworld.pak",
        magic=b"DC32TWORLD\0\0",
        required_any=("sets", "res", "data", "tiles"),
        include_suffixes=(".bmp", ".png", ".gif", ".dat", ".txt", ".ccl"),
        reference="https://www.microstupidity.com/tworld/",
    ),
    "xsokoban": AssetSource(
        key="xsokoban",
        pak_name="sokoban-xsokoban.pak",
        magic=b"DC32XSOKO\0\0",
        required_any=("screens", "levels", "objects"),
        include_suffixes=(".xbm", ".xpm", ".txt", ".screen", ".data"),
        reference="https://www.cs.cornell.edu/andru/xsokoban.html",
    ),
    "pipedreamer": AssetSource(
        key="pipedreamer",
        pak_name="pipe-pipedreamer.pak",
        magic=b"DC32PIPEPK\0",
        required_any=("Images", "Resources", "Source"),
        include_suffixes=(".png", ".jpg", ".jpeg", ".bmp", ".txt", ".json"),
        reference="https://github.com/escalonely/PipeDreamer/tree/main",
    ),
    "xscorch": AssetSource(
        key="xscorch",
        pak_name="scorch-xscorch.pak",
        magic=b"DC32SCORCH\0",
        required_any=("data", "pixmaps", "src"),
        include_suffixes=(".xpm", ".png", ".txt", ".dat", ".cfg", ".def"),
        reference="https://www.xscorch.org/",
    ),
}


def align4(value: int) -> int:
    return (value + 3) & ~3


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, help="Directory containing checked-out upstream source trees")
    parser.add_argument("--output-dir", type=Path, help="Directory where .pak files and manifest are written")
    parser.add_argument("--only", choices=sorted(SOURCES), action="append", help="Build only this source key; may be repeated")
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


def prompt_sources(default_keys: list[str]) -> list[str]:
    print("Available redistributable packs: " + ", ".join(default_keys))
    raw = input("Build which packs? Press Enter for all, or comma-separate names: ").strip()
    if not raw:
        return default_keys
    selected = [part.strip().lower() for part in raw.split(",") if part.strip()]
    bad = [key for key in selected if key not in SOURCES]
    if bad:
        raise ValueError(f"unknown pack key(s): {', '.join(bad)}")
    return selected


def resolve_inputs(args: argparse.Namespace) -> tuple[Path, Path, list[str]]:
    default_source_root = Path("third_party")
    default_output_dir = Path("build") / "apps"
    if args.source_root is None:
        args.source_root = prompt_path("Source root containing upstream trees", default_source_root) if interactive() else default_source_root
    if args.output_dir is None:
        args.output_dir = prompt_path("Output directory for .pak files", default_output_dir) if interactive() else default_output_dir
    keys = args.only or (prompt_sources(sorted(SOURCES)) if interactive() else sorted(SOURCES))
    return args.source_root.resolve(), args.output_dir.resolve(), keys


def find_source_root(base: Path, spec: AssetSource) -> Path:
    candidates = [base / spec.key, base / spec.key.replace("-", "_")]
    if spec.key == "pipedreamer":
        candidates.append(base / "PipeDreamer")
    if spec.key == "xscorch":
        candidates.append(base / "xscorch-0.2.1")
    for candidate in candidates:
        if candidate.is_dir():
            return candidate
    raise FileNotFoundError(f"missing source tree for {spec.key} under {base}")


def validate_source(root: Path, spec: AssetSource) -> None:
    if not any((root / rel).exists() for rel in spec.required_any):
        required = ", ".join(spec.required_any)
        raise FileNotFoundError(f"{root} does not look like {spec.key}; expected one of: {required}")


def collect_files(root: Path, spec: AssetSource) -> list[tuple[str, Path]]:
    files: list[tuple[str, Path]] = []
    for path in sorted((p for p in root.rglob("*") if p.is_file()), key=lambda p: p.as_posix().casefold()):
        rel = path.relative_to(root)
        include = path.suffix.lower() in spec.include_suffixes
        if spec.key == "xsokoban" and path.name.startswith("screen."):
            include = True
        if ".git" in rel.parts or not include:
            continue
        files.append((rel.as_posix(), path))
    if not files:
        raise FileNotFoundError(f"no redistributable asset candidates found for {spec.key} in {root}")
    return files


def build_pack(spec: AssetSource, files: list[tuple[str, Path]]) -> bytes:
    table_size = HEADER_SIZE + sum(ENTRY_SIZE + len(rel.encode("utf-8")) for rel, _ in files)
    data_offset = align4(table_size)
    entries = bytearray()
    payload = bytearray(b"\xff" * (data_offset - table_size))
    chunks: list[bytes] = []
    offset = data_offset

    for rel, path in files:
        rel_bytes = rel.encode("utf-8")
        data = path.read_bytes()
        entries += struct.pack(ENTRY_FMT, len(rel_bytes), 0, offset, len(data), binascii.crc32(data) & 0xFFFFFFFF)
        entries += rel_bytes
        chunks.append(data)
        offset = align4(offset + len(data))

    out = bytearray(struct.pack(HEADER_FMT, spec.magic, VERSION, len(files)))
    out += entries
    out += payload
    for data in chunks:
        out += data
        out += b"\xff" * (align4(len(out)) - len(out))
    return bytes(out)


def main() -> int:
    args = parse_args()
    base, out_dir, keys = resolve_inputs(args)
    manifest: dict[str, object] = {"schema": 1, "packs": {}}

    out_dir.mkdir(parents=True, exist_ok=True)
    for key in keys:
        spec = SOURCES[key]
        root = find_source_root(base, spec)
        validate_source(root, spec)
        files = collect_files(root, spec)
        pak = out_dir / spec.pak_name
        pak.write_bytes(build_pack(spec, files))
        manifest["packs"][key] = {
            "file": spec.pak_name,
            "reference": spec.reference,
            "source_root": root.as_posix(),
            "file_count": len(files),
        }
        print(f"Wrote {pak} with {len(files)} files")

    (out_dir / "period-assets-manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
