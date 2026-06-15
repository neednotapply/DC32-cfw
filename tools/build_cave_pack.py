#!/usr/bin/env python3
"""Build a DC32 Cave Story asset pack from user-supplied freeware data."""

from __future__ import annotations

import argparse
import binascii
import struct
import sys
from pathlib import Path

sys.dont_write_bytecode = True


MAGIC = b"DC32CAVEPAK\0"
VERSION = 1
HEADER_FMT = "<12sII"
ENTRY_FMT = "<HHIII"
HEADER_SIZE = struct.calcsize(HEADER_FMT)
ENTRY_SIZE = struct.calcsize(ENTRY_FMT)

REQUIRED_FILES = (
    "MyChar.pbm",
    "Arms.pbm",
    "ArmsImage.pbm",
    "Bullet.pbm",
    "Caret.pbm",
    "Face.pbm",
    "ItemImage.pbm",
    "npc.tbl",
)

STAGE_DAT_NAMES = ("Stage.dat", "stage.dat")
STAGE_SECT_NAMES = ("stage.sect", "Stage.sect")
DOUKUTSU_EXE_NAMES = ("Doukutsu.exe", "doukutsu.exe", "Doukutsu-fixed.exe", "Doukutsu_en.exe")
VANILLA_STAGE_COUNT = 95
VANILLA_STAGE_ENTRY_SIZE = 0xC8
VANILLA_STAGE_TABLE_OFFSET = 0x937B0
NX_STAGE_ENTRY_SIZE = 73

NX_TILESETS = (
    "0",
    "Pens",
    "Eggs",
    "EggX",
    "EggIn",
    "Store",
    "Weed",
    "Barr",
    "Maze",
    "Sand",
    "Mimi",
    "Cave",
    "River",
    "Gard",
    "Almond",
    "Oside",
    "Cent",
    "Jail",
    "White",
    "Fall",
    "Hell",
    "Labo",
)
NX_BACKDROPS = (
    "bk0",
    "bkBlue",
    "bkGreen",
    "bkBlack",
    "bkGard",
    "bkMaze",
    "bkGray",
    "bkRed",
    "bkWater",
    "bkMoon",
    "bkFog",
    "bkFall",
    "bkLight",
    "bkSunset",
    "bkHellish",
)
NX_NPCSETS = (
    "guest",
    "0",
    "eggs1",
    "ravil",
    "weed",
    "maze",
    "sand",
    "omg",
    "cemet",
    "bllg",
    "plant",
    "frog",
    "curly",
    "stream",
    "ironh",
    "toro",
    "x",
    "dark",
    "almo1",
    "eggs2",
    "twind",
    "moon",
    "cent",
    "heri",
    "red",
    "miza",
    "dr",
    "almo2",
    "kings",
    "hell",
    "press",
    "priest",
    "ballos",
    "island",
)


def align4(value: int) -> int:
    return (value + 3) & ~3


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert a local Cave Story freeware data directory into /APPS/cave.pak."
    )
    parser.add_argument(
        "--input",
        type=Path,
        help="Path to the Cave Story install root or its data directory. No data is downloaded or bundled.",
    )
    parser.add_argument("--output", type=Path, help="Path to write cave.pak")
    return parser.parse_args()


def interactive() -> bool:
    return sys.stdin.isatty()


def default_output_path() -> Path:
    apps = Path("APPS")
    return apps / "cave.pak" if apps.is_dir() else Path("cave.pak")


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
    if args.input is None:
        print("Cave Story pack builder")
        print("This writes the badge-ready file expected at /APPS/cave.pak.")
        args.input = prompt_path("Path to Cave Story install root or data directory")
    if args.output is None:
        args.output = prompt_path("Output pak path", default_output_path()) if interactive() else default_output_path()
    return args.input.resolve(), args.output.resolve()


def cave_data_root(path: Path) -> Path:
    if (path / "data").is_dir():
        return path / "data"
    return path


def require_file(root: Path, rel: str) -> None:
    if not (root / rel).is_file():
        raise FileNotFoundError(f"missing Cave Story data file: {root / rel}")


def validate_cave_data(root: Path) -> None:
    if not root.is_dir():
        raise FileNotFoundError(f"Cave Story data directory not found: {root}")
    for rel in REQUIRED_FILES:
        require_file(root, rel)
    stage_dir = root / "Stage"
    npc_dir = root / "Npc"
    if not stage_dir.is_dir():
        raise FileNotFoundError(f"missing Cave Story stage directory: {stage_dir}")
    if not npc_dir.is_dir():
        raise FileNotFoundError(f"missing Cave Story NPC graphics directory: {npc_dir}")
    if not any(stage_dir.glob("*.pxm")):
        raise FileNotFoundError(f"missing Cave Story .pxm maps in: {stage_dir}")
    if not any(stage_dir.glob("*.tsc")):
        raise FileNotFoundError(f"missing Cave Story .tsc scripts in: {stage_dir}")
    if not any(npc_dir.glob("*.pbm")):
        raise FileNotFoundError(f"missing Cave Story NPC .pbm graphics in: {npc_dir}")


def nul_field(data: bytes) -> bytes:
    return data.split(b"\0", 1)[0]


def pad_field(data: bytes, size: int) -> bytes:
    data = nul_field(data)[:size]
    return data + b"\0" * (size - len(data))


def field_name(data: bytes) -> str:
    return nul_field(data).decode("ascii", "replace").casefold()


def find_named_index(name: bytes, table: tuple[str, ...], label: str, record_index: int) -> int:
    wanted = field_name(name)
    for index, value in enumerate(table):
        if value.casefold() == wanted:
            return index
    raise ValueError(f"unrecognized Cave Story {label} {wanted!r} in executable stage record {record_index}")


def convert_vanilla_stage_section(data: bytes) -> bytes:
    if len(data) < VANILLA_STAGE_COUNT * VANILLA_STAGE_ENTRY_SIZE:
        raise ValueError("embedded Cave Story stage table is truncated")

    out = bytearray([VANILLA_STAGE_COUNT])
    for index in range(VANILLA_STAGE_COUNT):
        pos = index * VANILLA_STAGE_ENTRY_SIZE
        tileset = data[pos : pos + 32]
        filename = data[pos + 32 : pos + 64]
        scroll_type = struct.unpack_from("<i", data, pos + 64)[0]
        background = data[pos + 68 : pos + 100]
        npc_set_1 = data[pos + 100 : pos + 132]
        npc_set_2 = data[pos + 132 : pos + 164]
        boss_no = data[pos + 164]
        caption = data[pos + 165 : pos + 200]

        out += pad_field(filename, 32)
        out += pad_field(caption, 35)
        out.append(find_named_index(tileset, NX_TILESETS, "tileset", index))
        out.append(find_named_index(background, NX_BACKDROPS, "background", index))
        out.append(scroll_type & 0xFF)
        out.append(boss_no)
        out.append(find_named_index(npc_set_1, NX_NPCSETS, "NPC set 1", index))
        out.append(find_named_index(npc_set_2, NX_NPCSETS, "NPC set 2", index))

    expected = 1 + VANILLA_STAGE_COUNT * NX_STAGE_ENTRY_SIZE
    if len(out) != expected:
        raise ValueError(f"generated Stage.dat has size {len(out)}, expected {expected}")
    return bytes(out)


def find_install_root(input_path: Path, data_root: Path) -> Path:
    if (input_path / "data").resolve() == data_root.resolve():
        return input_path
    return data_root.parent


def candidate_executables(input_path: Path, data_root: Path) -> list[Path]:
    install_root = find_install_root(input_path, data_root)
    dirs: list[Path] = []
    for path in (install_root, input_path, data_root.parent, data_root):
        resolved = path.resolve()
        if resolved not in dirs:
            dirs.append(resolved)

    candidates: list[Path] = []
    seen: set[Path] = set()
    for directory in dirs:
        for name in DOUKUTSU_EXE_NAMES:
            path = directory / name
            if path.is_file() and path.resolve() not in seen:
                candidates.append(path)
                seen.add(path.resolve())
    for directory in dirs:
        for path in sorted(directory.glob("*.exe"), key=lambda p: p.name.casefold()):
            resolved = path.resolve()
            if resolved not in seen:
                candidates.append(path)
                seen.add(resolved)
    return candidates


def stage_dat_from_executable(input_path: Path, data_root: Path) -> tuple[bytes, Path]:
    errors: list[str] = []
    table_size = VANILLA_STAGE_COUNT * VANILLA_STAGE_ENTRY_SIZE
    for exe in candidate_executables(input_path, data_root):
        try:
            data = exe.read_bytes()
            end = VANILLA_STAGE_TABLE_OFFSET + table_size
            if len(data) < end:
                errors.append(f"{exe.name} is too small")
                continue
            return convert_vanilla_stage_section(data[VANILLA_STAGE_TABLE_OFFSET:end]), exe
        except Exception as exc:
            errors.append(f"{exe.name}: {exc}")

    detail = "; ".join(errors) if errors else "no Cave Story executable found"
    raise FileNotFoundError(
        "missing Cave Story stage table: no data/Stage.dat was found and the embedded table "
        f"could not be extracted from Doukutsu.exe ({detail})"
    )


def load_stage_dat(input_path: Path, data_root: Path) -> tuple[bytes, str]:
    for name in STAGE_DAT_NAMES:
        path = data_root / name
        if path.is_file():
            return path.read_bytes(), str(path)
    for name in STAGE_SECT_NAMES:
        path = data_root / name
        if path.is_file():
            return convert_vanilla_stage_section(path.read_bytes()), str(path)
    data, exe = stage_dat_from_executable(input_path, data_root)
    return data, str(exe)


def collect_files(root: Path, stage_dat: bytes) -> list[tuple[str, bytes]]:
    files: list[tuple[str, bytes]] = [("Stage.dat", stage_dat)]
    allowed = {".pbm", ".pxm", ".pxe", ".tsc", ".tbl", ".dat", ".wav", ".org"}
    for path in sorted((p for p in root.rglob("*") if p.is_file()), key=lambda p: p.as_posix().casefold()):
        rel = path.relative_to(root).as_posix()
        if rel.casefold() in {name.casefold() for name in (*STAGE_DAT_NAMES, *STAGE_SECT_NAMES)}:
            continue
        if path.suffix.lower() in allowed:
            files.append((rel, path.read_bytes()))
    if not files:
        raise FileNotFoundError(f"no Cave Story data files found in: {root}")
    return files


def build_pack(files: list[tuple[str, bytes]]) -> bytes:
    files = sorted(files, key=lambda item: item[0].casefold())
    table_size = HEADER_SIZE + sum(ENTRY_SIZE + len(rel.encode("utf-8")) for rel, _ in files)
    data_offset = align4(table_size)
    entries = bytearray()
    payload = bytearray(b"\xff" * (data_offset - table_size))
    chunks: list[bytes] = []

    offset = data_offset
    for rel, data in files:
        rel_bytes = rel.encode("utf-8")
        crc = binascii.crc32(data) & 0xFFFFFFFF
        entries += struct.pack(ENTRY_FMT, len(rel_bytes), 0, offset, len(data), crc)
        entries += rel_bytes
        chunks.append(data)
        offset = align4(offset + len(data))

    out = bytearray(struct.pack(HEADER_FMT, MAGIC, VERSION, len(files)))
    out += entries
    out += payload
    for data in chunks:
        out += data
        out += b"\xff" * (align4(len(out)) - len(out))
    return bytes(out)


def main() -> int:
    args = parse_args()
    input_path, output = resolve_inputs(args)
    root = cave_data_root(input_path)
    validate_cave_data(root)
    stage_dat, stage_source = load_stage_dat(input_path, root)
    files = collect_files(root, stage_dat)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(build_pack(files))
    print(f"Wrote {output} with {len(files)} Cave Story data files")
    print(f"Stage table: {stage_source}")
    print("Install on the SD card as /APPS/cave.pak.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
