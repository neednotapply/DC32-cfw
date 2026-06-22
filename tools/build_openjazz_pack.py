#!/usr/bin/env python3
"""Build /APPS/openjazz.pak from Jazz Jackrabbit 1 data."""

from __future__ import annotations

import argparse
import binascii
import json
import re
import struct
import sys
import tempfile
import zipfile
from dataclasses import dataclass
from pathlib import Path

sys.dont_write_bytecode = True

MAGIC = b"DC32JAZZPK\0\0"
VERSION = 1
HEADER_FMT = "<12sII"
ENTRY_FMT = "<HHIII"
HEADER_SIZE = struct.calcsize(HEADER_FMT)
ENTRY_SIZE = struct.calcsize(ENTRY_FMT)
REQUIRED_FILES = {
    "PANEL.000", "MENU.000", "MAINCHAR.000",
    "FONT2.0FN", "FONTBIG.0FN", "FONTINY.0FN", "FONTMN1.0FN", "FONTMN2.0FN",
    "STARTUP.0SC", "SOUNDS.000",
    "LEVEL0.000", "LEVEL1.000", "LEVEL2.000",
    "BLOCKS.000", "SPRITES.000", "PLANET.000",
}
NUMBERED_EXTENSION = re.compile(r"\.[0-9]{3}\Z")
FIXED_SUFFIXES = {".0FN", ".0SC", ".0FM", ".PSM"}
MACRO_FILES = {f"MACRO.{number}" for number in range(1, 5)}


@dataclass(frozen=True)
class PackEntry:
    name: str
    path: Path


def align4(value: int) -> int:
    return (value + 3) & ~3


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert local Jazz Jackrabbit 1 data into /APPS/openjazz.pak."
    )
    parser.add_argument("source", nargs="?", type=Path, help=argparse.SUPPRESS)
    parser.add_argument(
        "--input",
        dest="input_path",
        type=Path,
        help="Path to a Jazz data directory or shareware/full-version ZIP",
    )
    parser.add_argument("--output", type=Path, help="Path to write openjazz.pak")
    return parser.parse_args()


def interactive() -> bool:
    return sys.stdin.isatty()


def default_output_path() -> Path:
    apps = Path("APPS")
    return apps / "openjazz.pak" if apps.is_dir() else Path("openjazz.pak")


def default_source_path() -> Path | None:
    candidates = (
        Path("JAZZ.ZIP"),
        Path("jazz.zip"),
        Path("JAZZ"),
        Path("Jazz"),
        Path("jazz"),
        Path("third_party") / "openjazz-shareware" / "JAZZ.ZIP",
    )
    return next((path for path in candidates if path.exists()), None)


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


def resolve_inputs(args: argparse.Namespace) -> tuple[Path, Path]:
    source = args.input_path or args.source
    if source is None:
        detected = default_source_path()
        if not interactive() and detected is not None:
            source = detected
        else:
            print("Jazz Jackrabbit pack builder")
            print("This writes the badge-ready file expected at /APPS/openjazz.pak.")
            source = prompt_path("Path to Jazz data directory or ZIP", detected)
    output = args.output
    if output is None:
        output = prompt_path("Output pak path", default_output_path()) if interactive() else default_output_path()
    return source.resolve(), output.resolve()


def extract_zip(source: Path, tmp: Path) -> Path:
    root = tmp.resolve()
    with zipfile.ZipFile(source) as zf:
        for info in zf.infolist():
            target = (tmp / info.filename).resolve()
            if target != root and root not in target.parents:
                raise ValueError(f"refusing unsafe ZIP entry: {info.filename}")
        zf.extractall(tmp)
    return tmp


def direct_file_map(path: Path) -> dict[str, Path]:
    files: dict[str, Path] = {}
    for item in path.iterdir():
        if not item.is_file():
            continue
        name = item.name.upper()
        if name in files:
            raise ValueError(f"duplicate case-insensitive file name in {path}: {name}")
        files[name] = item
    return files


def find_data_root(search_root: Path) -> tuple[Path, dict[str, Path]]:
    candidates: dict[Path, tuple[Path, dict[str, Path]]] = {}
    panel_files = sorted(
        (path for path in search_root.rglob("*") if path.is_file() and path.name.upper() == "PANEL.000"),
        key=lambda path: path.as_posix().casefold(),
    )
    for panel in panel_files:
        files = direct_file_map(panel.parent)
        if REQUIRED_FILES <= files.keys():
            candidates[panel.parent.resolve()] = (panel.parent, files)
    if not candidates:
        raise FileNotFoundError(
            "no Jazz data root contains the required files: " + ", ".join(sorted(REQUIRED_FILES))
        )
    if len(candidates) != 1:
        roots = ", ".join(path.as_posix() for path, _files in candidates.values())
        raise ValueError(f"multiple Jazz data roots found: {roots}")
    return next(iter(candidates.values()))


def include_name(name: str) -> bool:
    suffix = Path(name).suffix.upper()
    return bool(NUMBERED_EXTENSION.fullmatch(suffix)) or suffix in FIXED_SUFFIXES or name in MACRO_FILES


def collect_entries(search_root: Path) -> tuple[Path, list[PackEntry]]:
    data_root, files = find_data_root(search_root)
    entries = [PackEntry(name, path) for name, path in files.items() if include_name(name)]
    entries.sort(key=lambda entry: entry.name)
    validate_entries(entries)
    return data_root, entries


def validate_entries(entries: list[PackEntry]) -> None:
    names = [entry.name for entry in entries]
    missing = sorted(REQUIRED_FILES - set(names))
    if missing:
        raise FileNotFoundError("missing required Jazz data files: " + ", ".join(missing))
    if names != sorted(names) or len(names) != len(set(names)):
        raise ValueError("pack entries must have unique sorted names")
    for name in names:
        encoded = name.encode("ascii")
        if len(encoded) > 12 or name != name.upper():
            raise ValueError(f"Jazz asset is not a normalized 8.3 name: {name}")


def build_pack(entries: list[PackEntry]) -> bytes:
    table_size = HEADER_SIZE + sum(ENTRY_SIZE + len(entry.name.encode("ascii")) for entry in entries)
    data_offset = align4(table_size)
    table = bytearray()
    chunks: list[bytes] = []
    offset = data_offset
    for entry in entries:
        name_bytes = entry.name.encode("ascii")
        data = entry.path.read_bytes()
        table += struct.pack(
            ENTRY_FMT, len(name_bytes), 0, offset, len(data), binascii.crc32(data) & 0xFFFFFFFF
        )
        table += name_bytes
        chunks.append(data)
        offset = align4(offset + len(data))
    out = bytearray(struct.pack(HEADER_FMT, MAGIC, VERSION, len(entries)))
    out += table
    out += b"\xff" * (data_offset - len(out))
    for data in chunks:
        out += data
        out += b"\xff" * (align4(len(out)) - len(out))
    return bytes(out)


def verify_pack(raw: bytes, expected_names: list[str]) -> None:
    if len(raw) < HEADER_SIZE:
        raise ValueError("generated pack is truncated")
    magic, version, count = struct.unpack_from(HEADER_FMT, raw, 0)
    if magic != MAGIC or version != VERSION or count != len(expected_names):
        raise ValueError("generated pack header does not match its input")
    pos = HEADER_SIZE
    previous_name = ""
    previous_end = 0
    actual_names: list[str] = []
    first_offset = len(raw)
    for _index in range(count):
        if pos + ENTRY_SIZE > len(raw):
            raise ValueError("generated pack table is truncated")
        name_len, reserved, offset, size, crc = struct.unpack_from(ENTRY_FMT, raw, pos)
        pos += ENTRY_SIZE
        if reserved or not name_len or name_len > 12 or pos + name_len > len(raw):
            raise ValueError("generated pack has an invalid table entry")
        name = raw[pos:pos + name_len].decode("ascii")
        pos += name_len
        if name <= previous_name or offset & 3 or offset < previous_end or size > len(raw) - offset:
            raise ValueError(f"generated pack has an invalid entry for {name}")
        if (binascii.crc32(raw[offset:offset + size]) & 0xFFFFFFFF) != crc:
            raise ValueError(f"generated pack CRC mismatch for {name}")
        if not actual_names:
            first_offset = offset
        previous_name = name
        previous_end = offset + size
        actual_names.append(name)
    if actual_names != expected_names or align4(pos) > first_offset:
        raise ValueError("generated pack table does not match its payload")


def main() -> int:
    args = parse_args()
    source, output = resolve_inputs(args)
    if not source.exists():
        raise FileNotFoundError(source)
    if source.is_file() and source.suffix.lower() != ".zip":
        raise ValueError("source file must be a ZIP archive")
    with tempfile.TemporaryDirectory() as tmp_name:
        search_root = extract_zip(source, Path(tmp_name)) if source.is_file() else source
        data_root, entries = collect_entries(search_root)
        raw = build_pack(entries)
        verify_pack(raw, [entry.name for entry in entries])
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(raw)
        manifest = {
            "schema": 1,
            "file": output.name,
            "source": source.as_posix(),
            "data_root": data_root.relative_to(search_root).as_posix() if data_root != search_root else ".",
            "file_count": len(entries),
            "required_files": sorted(REQUIRED_FILES),
            "notes": "Built locally from Jazz Jackrabbit 1 data. Generated packs are not redistributed.",
        }
        (output.parent / "openjazz-pack-manifest.json").write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
            newline="\n",
        )
        print(f"Wrote {output} with {len(entries)} files from {data_root}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
