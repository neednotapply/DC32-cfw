#!/usr/bin/env python3
"""Build a DC32 Chip's Challenge asset pack from user-supplied data."""

from __future__ import annotations

import argparse
import binascii
import struct
import sys
from pathlib import Path

sys.dont_write_bytecode = True

from build_tworld_assets import ALPHA_BYTES, OUT_TILE_SIZE, TILE_COUNT, TILE_POS


MAGIC = b"DC32CHIPSPK"
VERSION = 1
HEADER_FMT = "<11sBII"
ENTRY_FMT = "<HHIII"
HEADER_SIZE = struct.calcsize(HEADER_FMT)
ENTRY_SIZE = struct.calcsize(ENTRY_FMT)
CHIPS_DAT_MAGIC = 0x0002AAAC
TILE_MAGIC = b"DC32CHIPTIL"
TILE_VERSION = 1
TILE_HEADER_FMT = "<11sBHHHH"
TILE_HEADER_SIZE = struct.calcsize(TILE_HEADER_FMT)
DEFAULT_TWORLD_COLUMNS = 7
DEFAULT_TWORLD_ROWS = 16
WIN_EXE_TILE_RESOURCE_ID = 0x0134
WIN_TILE_COLUMNS = 13
WIN_TILE_ROWS = 16
WIN_TILE_SIZE = 32

try:
    from PIL import Image
except Exception:  # pragma: no cover - exercised only on hosts without Pillow
    Image = None


def align4(value: int) -> int:
    return (value + 3) & ~3


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate CHIPS.DAT and optional Win 3.1 CHIPS.EXE tile graphics, then write /APPS/chips.pak."
    )
    parser.add_argument("--chips-dat", type=Path, help="User-provided CHIPS.DAT level file or extracted Win 3.1 game folder")
    parser.add_argument("--tiles", type=Path, help="Optional CHIPS.EXE, tile image, or tile-pack file")
    parser.add_argument("--output", type=Path, help="Path to write chips.pak")
    return parser.parse_args()


def interactive() -> bool:
    return sys.stdin.isatty()


def default_output_path() -> Path:
    apps = Path("APPS")
    return apps / "chips.pak" if apps.is_dir() else Path("chips.pak")


def first_existing(paths: tuple[Path, ...]) -> Path | None:
    for path in paths:
        if path.is_file():
            return path
    return None


def default_chips_dat_path() -> Path | None:
    return first_existing((
        Path("CHIPS.DAT"),
        Path("chips.dat"),
        Path("tools") / "CHIPS.DAT",
        Path("tools") / "chips.dat",
    ))


def default_tile_path(chips_dat: Path | None = None) -> Path | None:
    candidates: list[Path] = []
    if chips_dat is not None:
        parent = chips_dat.parent
        candidates.extend((
            parent / "CHIPS.EXE",
            parent / "chips.exe",
        ))
    candidates.extend((
        Path("CHIPS.EXE"),
        Path("chips.exe"),
        Path("tools") / "CHIPS.EXE",
        Path("tools") / "chips.exe",
    ))
    exe = first_existing(tuple(candidates))
    if exe is not None:
        return exe
    return first_existing((
        Path("chips.bmp"),
        Path("chips.png"),
        Path("tiles.bmp"),
        Path("tiles.png"),
        Path("tools") / "chips.bmp",
        Path("tools") / "chips.png",
        Path("tools") / "tiles.bmp",
        Path("tools") / "tiles.png",
    ))


def prompt_path(label: str, default: Path | None = None, optional: bool = False) -> Path | None:
    suffix = f" [{default}]" if default else ""
    while True:
        try:
            raw = input(f"{label}{suffix}: ").strip().strip('"')
        except EOFError:
            if default is not None:
                return default
            if optional:
                return None
            raise ValueError(f"missing required path: {label}") from None
        if not raw and default is not None:
            return default
        if not raw and optional:
            return None
        if raw:
            return Path(raw)
        print("Please enter a path.")


def prompt_yes_no(label: str, default_yes: bool) -> bool:
    suffix = "Y/n" if default_yes else "y/N"
    while True:
        try:
            raw = input(f"{label} [{suffix}]: ").strip().lower()
        except EOFError:
            return default_yes
        if not raw:
            return default_yes
        if raw in ("y", "yes"):
            return True
        if raw in ("n", "no"):
            return False
        print("Please answer yes or no.")


def resolve_chips_dat_path(path: Path) -> Path:
    if path.is_dir():
        for name in ("CHIPS.DAT", "chips.dat"):
            candidate = path / name
            if candidate.is_file():
                return candidate
        raise ValueError(f"{path} does not contain CHIPS.DAT")
    return path


def resolve_inputs(args: argparse.Namespace) -> tuple[Path, Path | None, Path]:
    if args.chips_dat is None:
        default_dat = default_chips_dat_path()
        if not interactive() and default_dat is not None:
            args.chips_dat = default_dat
        else:
            print("Chip's Challenge pack builder")
            print("This writes the badge-ready file expected at /APPS/chips.pak.")
            args.chips_dat = prompt_path("Path to CHIPS.DAT or Win 3.1 folder", default_dat)  # type: ignore[assignment]
    args.chips_dat = resolve_chips_dat_path(args.chips_dat)  # type: ignore[assignment]
    if args.tiles is None:
        detected_tiles = default_tile_path(args.chips_dat)
        detected_original_exe = (
            detected_tiles is not None and
            args.chips_dat is not None and
            detected_tiles.parent.resolve() == args.chips_dat.parent.resolve() and
            detected_tiles.name.lower() == "chips.exe"
        )
        if detected_original_exe:
            args.tiles = detected_tiles
        elif interactive():
            if detected_tiles and prompt_yes_no(f"Use detected non-Win3.1 tile graphics {detected_tiles}?", False):
                args.tiles = detected_tiles
            else:
                args.tiles = prompt_path("Optional CHIPS.EXE, tile image, or DC32CHIPTIL pack; press Enter to skip", optional=True)  # type: ignore[assignment]
    if args.output is None:
        if interactive():
            args.output = prompt_path("Output pak path", default_output_path())  # type: ignore[assignment]
        else:
            args.output = default_output_path()
    assert args.chips_dat is not None
    assert args.output is not None
    return args.chips_dat.resolve(), args.tiles.resolve() if args.tiles else None, args.output.resolve()


def validate_chips_dat(path: Path) -> bytes:
    data = path.read_bytes()
    if len(data) < 6:
        raise ValueError(f"{path} is too small to be CHIPS.DAT")
    magic, level_count = struct.unpack_from("<IH", data, 0)
    if magic != CHIPS_DAT_MAGIC:
        raise ValueError(f"{path} does not start with CHIPS.DAT magic 0x{CHIPS_DAT_MAGIC:08x}")
    if level_count == 0 or level_count > 512:
        raise ValueError(f"{path} has an implausible level count: {level_count}")
    validate_level_records(data, level_count, path)
    return data


def read_u16(data: bytes, offset: int) -> int:
    return data[offset] | (data[offset + 1] << 8)


def validate_encoded_layer(layer: bytes, path: Path, level_index: int, layer_name: str) -> None:
    pos = 0
    cells = 0
    while pos < len(layer) and cells < 32 * 32:
        tile = layer[pos]
        pos += 1
        if tile == 0xFF:
            if pos + 2 > len(layer):
                raise ValueError(f"{path} level {level_index}: truncated RLE in {layer_name} layer")
            run = layer[pos]
            pos += 2
            cells += run
        else:
            cells += 1


def validate_level_record(record: bytes, path: Path, level_index: int) -> None:
    if len(record) < 10:
        raise ValueError(f"{path} level {level_index}: record is too small")
    level_number = read_u16(record, 0)
    if level_number == 0:
        raise ValueError(f"{path} level {level_index}: level number is zero")
    if read_u16(record, 6) > 1:
        raise ValueError(f"{path} level {level_index}: unsupported CHIPS.DAT layer encoding")
    upper_size = read_u16(record, 8)
    pos = 10
    if pos + upper_size + 2 > len(record):
        raise ValueError(f"{path} level {level_index}: upper layer extends past record")
    validate_encoded_layer(record[pos : pos + upper_size], path, level_index, "upper")
    pos += upper_size
    lower_size = read_u16(record, pos)
    pos += 2
    if pos + lower_size + 2 > len(record):
        raise ValueError(f"{path} level {level_index}: lower layer extends past record")
    validate_encoded_layer(record[pos : pos + lower_size], path, level_index, "lower")
    pos += lower_size
    metadata_size = read_u16(record, pos)
    pos += 2
    meta_end = min(pos + metadata_size, len(record))
    while pos + 2 <= meta_end:
        field = record[pos]
        size = record[pos + 1]
        pos += 2
        if pos + size > meta_end:
            raise ValueError(f"{path} level {level_index}: metadata field {field} extends past metadata")
        pos += size
    if pos != meta_end:
        raise ValueError(f"{path} level {level_index}: trailing byte in metadata")


def validate_level_records(data: bytes, level_count: int, path: Path) -> None:
    offset = 6
    for level_index in range(1, level_count + 1):
        if offset + 2 > len(data):
            raise ValueError(f"{path}: missing size word for level {level_index}")
        size = read_u16(data, offset)
        offset += 2
        if size == 0:
            raise ValueError(f"{path}: zero-size level {level_index}")
        if offset + size > len(data):
            raise ValueError(f"{path}: level {level_index} extends past end of CHIPS.DAT")
        validate_level_record(data[offset : offset + size], path, level_index)
        offset += size
    if offset != len(data):
        raise ValueError(f"{path}: {len(data) - offset} trailing bytes after {level_count} levels")


def validate_tiles(path: Path | None) -> bytes | None:
    if not path:
        return None
    data = path.read_bytes()
    if data.startswith(TILE_MAGIC):
        validate_tile_pack(data, path)
        return data
    if data.startswith(b"MZ"):
        return build_tile_pack_from_win_exe(data, path)
    return build_tile_pack_from_image(path)


def validate_tile_pack(data: bytes, path: Path) -> None:
    if len(data) < TILE_HEADER_SIZE:
        raise ValueError(f"{path} is too small to be a DC32 Chip's tile pack")
    magic, _pad, version, tile_count, tile_size, alpha_bytes = struct.unpack_from(TILE_HEADER_FMT, data, 0)
    if magic != TILE_MAGIC:
        raise ValueError(f"{path} does not start with DC32 Chip's tile-pack magic")
    if version != TILE_VERSION:
        raise ValueError(f"{path} has unsupported tile-pack version {version}")
    if tile_count != TILE_COUNT or tile_size != OUT_TILE_SIZE or alpha_bytes != ALPHA_BYTES:
        raise ValueError(
            f"{path} tile-pack geometry is {tile_count}x{tile_size}px/{alpha_bytes} alpha bytes; "
            f"expected {TILE_COUNT}x{OUT_TILE_SIZE}px/{ALPHA_BYTES}"
        )
    expected = TILE_HEADER_SIZE + TILE_COUNT * OUT_TILE_SIZE * OUT_TILE_SIZE + TILE_COUNT * ALPHA_BYTES
    if len(data) != expected:
        raise ValueError(f"{path} is {len(data)} bytes, expected {expected} for a DC32 Chip's tile pack")


def rgb332_rgba(pixel: tuple[int, int, int, int]) -> int:
    r, g, b, _a = pixel
    return (r & 0xE0) | ((g >> 3) & 0x1C) | (b >> 6)


def read_image_rgba(path: Path) -> tuple[int, int, list[tuple[int, int, int, int]]]:
    if Image is None:
        raise ValueError(f"{path} is an image, but Pillow is not installed; install Pillow or provide a DC32CHIPTIL pack")
    try:
        with Image.open(path) as image:
            rgba = image.convert("RGBA")
            width, height = rgba.size
            return width, height, list(rgba.getdata())
    except Exception as exc:
        raise ValueError(
            f"{path} is not a supported image or DC32CHIPTIL tile pack. "
            "Use a PNG/JPEG/BMP tile sheet or a prebuilt DC32CHIPTIL file."
        ) from exc


def read_ne_bitmap_resource(exe: bytes, resource_id: int) -> bytes:
    if len(exe) < 0x40 or exe[:2] != b"MZ":
        raise ValueError("not a Windows executable")
    ne_offset = struct.unpack_from("<I", exe, 0x3C)[0]
    if ne_offset + 0x28 > len(exe) or exe[ne_offset : ne_offset + 2] != b"NE":
        raise ValueError("not a Windows 3.x NE executable")
    resource_table = ne_offset + struct.unpack_from("<H", exe, ne_offset + 0x24)[0]
    if resource_table + 2 > len(exe):
        raise ValueError("invalid NE resource table")
    shift = struct.unpack_from("<H", exe, resource_table)[0]
    pos = resource_table + 2
    while pos + 8 <= len(exe):
        resource_type, count, _reserved = struct.unpack_from("<HHI", exe, pos)
        pos += 8
        if resource_type == 0:
            break
        if pos + count * 12 > len(exe):
            raise ValueError("truncated NE resource table")
        for _ in range(count):
            off_blocks, len_blocks, _flags, raw_id, _handle, _usage = struct.unpack_from("<HHHHHH", exe, pos)
            pos += 12
            if resource_type != 0x8002 or (raw_id & 0x7FFF) != resource_id:
                continue
            offset = off_blocks << shift
            size = len_blocks << shift
            if offset > len(exe) or size > len(exe) - offset:
                raise ValueError("bitmap resource points outside executable")
            return exe[offset : offset + size]
    raise ValueError(f"missing RT_BITMAP resource 0x{resource_id:04x}")


def read_dib_rgba(dib: bytes) -> tuple[int, int, list[tuple[int, int, int, int]]]:
    if len(dib) < 40:
        raise ValueError("DIB is too small")
    header_size = struct.unpack_from("<I", dib, 0)[0]
    if header_size < 40 or len(dib) < header_size:
        raise ValueError("unsupported DIB header")
    width, height, planes, bpp = struct.unpack_from("<iiHH", dib, 4)
    compression = struct.unpack_from("<I", dib, 16)[0]
    colors_used = struct.unpack_from("<I", dib, 32)[0]
    if width <= 0 or height == 0 or planes != 1 or bpp not in (1, 4, 8) or compression not in (0, 2):
        raise ValueError(f"unsupported DIB geometry/compression: {width}x{height} {bpp}bpp")
    if compression == 2 and bpp != 4:
        raise ValueError("BI_RLE4 is only supported for 4bpp DIBs")
    abs_h = abs(height)
    palette_entries = colors_used or (1 << bpp)
    palette_offset = header_size
    pixel_offset = header_size + palette_entries * 4
    if len(dib) < pixel_offset:
        raise ValueError("DIB palette is truncated")
    palette: list[tuple[int, int, int, int]] = []
    for i in range(palette_entries):
        b, g, r, _reserved = dib[palette_offset + i * 4 : palette_offset + i * 4 + 4]
        palette.append((r, g, b, 255))
    if compression == 2:
        indexed = decode_rle4(dib[pixel_offset:], width, abs_h, bottom_up=height > 0)
    else:
        stride = ((width * bpp + 31) // 32) * 4
        if len(dib) < pixel_offset + stride * abs_h:
            raise ValueError("DIB pixel data is truncated")
        indexed = []
        for y in range(abs_h):
            source_y = abs_h - 1 - y if height > 0 else y
            row = pixel_offset + source_y * stride
            for x in range(width):
                if bpp == 8:
                    idx = dib[row + x]
                elif bpp == 4:
                    byte = dib[row + x // 2]
                    idx = byte >> 4 if x % 2 == 0 else byte & 0x0F
                else:
                    byte = dib[row + x // 8]
                    idx = (byte >> (7 - (x % 8))) & 1
                indexed.append(idx)
    pixels: list[tuple[int, int, int, int]] = []
    for idx in indexed:
        if idx >= len(palette):
            raise ValueError("DIB pixel index exceeds palette")
        pixels.append(palette[idx])
    return width, abs_h, pixels


def decode_rle4(data: bytes, width: int, height: int, bottom_up: bool) -> list[int]:
    rows = [[0] * width for _ in range(height)]
    x = 0
    y = height - 1 if bottom_up else 0
    dy = -1 if bottom_up else 1
    pos = 0

    def put(value: int) -> None:
        nonlocal x
        if 0 <= y < height and 0 <= x < width:
            rows[y][x] = value & 0x0F
        x += 1

    while pos + 1 <= len(data):
        if pos + 2 > len(data):
            break
        count = data[pos]
        value = data[pos + 1]
        pos += 2
        if count:
            hi = value >> 4
            lo = value & 0x0F
            for i in range(count):
                put(hi if i % 2 == 0 else lo)
            continue
        if value == 0:
            x = 0
            y += dy
            if y < 0 or y >= height:
                break
        elif value == 1:
            break
        elif value == 2:
            if pos + 2 > len(data):
                raise ValueError("truncated BI_RLE4 delta")
            x += data[pos]
            y += dy * data[pos + 1]
            pos += 2
        else:
            absolute_count = value
            bytes_to_read = (absolute_count + 1) // 2
            if pos + bytes_to_read > len(data):
                raise ValueError("truncated BI_RLE4 absolute run")
            for i in range(absolute_count):
                byte = data[pos + i // 2]
                put(byte >> 4 if i % 2 == 0 else byte & 0x0F)
            pos += bytes_to_read
            if bytes_to_read & 1:
                pos += 1
    return [pixel for row in rows for pixel in row]


def build_tile_pack_from_win_exe(exe: bytes, path: Path) -> bytes:
    dib = read_ne_bitmap_resource(exe, WIN_EXE_TILE_RESOURCE_ID)
    width, height, pixels = read_dib_rgba(dib)
    if width != WIN_TILE_COLUMNS * WIN_TILE_SIZE or height != WIN_TILE_ROWS * WIN_TILE_SIZE:
        raise ValueError(
            f"{path} bitmap resource 0x{WIN_EXE_TILE_RESOURCE_ID:04x} is {width}x{height}, "
            f"expected {WIN_TILE_COLUMNS * WIN_TILE_SIZE}x{WIN_TILE_ROWS * WIN_TILE_SIZE}"
        )
    tiles = [[0] * (OUT_TILE_SIZE * OUT_TILE_SIZE) for _ in range(TILE_COUNT)]
    alpha = [[0] * ALPHA_BYTES for _ in range(TILE_COUNT)]
    for tile_id, xopaque, yopaque, xtransp, ytransp in TILE_POS:
        if tile_id >= TILE_COUNT:
            raise ValueError(f"tile id 0x{tile_id:02x} exceeds generated table")
        mask_x = xopaque + 6 if (xtransp >= 0 or ytransp >= 0) and xopaque >= 4 else -1
        tiles[tile_id], alpha[tile_id] = sample_win_tile(pixels, width, xopaque, yopaque, mask_x)
    out = bytearray(struct.pack(TILE_HEADER_FMT, TILE_MAGIC, 0, TILE_VERSION, TILE_COUNT, OUT_TILE_SIZE, ALPHA_BYTES))
    for tile in tiles:
        out.extend(tile)
    for mask in alpha:
        out.extend(mask)
    packed = bytes(out)
    validate_tile_pack(packed, path)
    return packed


def sample_tile(
    pixels: list[tuple[int, int, int, int]],
    width: int,
    x0: int,
    y0: int,
    cell_w: int,
    cell_h: int,
    transparent_magenta: bool,
) -> tuple[list[int], list[int]]:
    out = [0] * (OUT_TILE_SIZE * OUT_TILE_SIZE)
    alpha = [0] * ALPHA_BYTES
    for y in range(OUT_TILE_SIZE):
        sy = y0 + min(cell_h - 1, (y * cell_h + cell_h // 2) // OUT_TILE_SIZE)
        for x in range(OUT_TILE_SIZE):
            sx = x0 + min(cell_w - 1, (x * cell_w + cell_w // 2) // OUT_TILE_SIZE)
            r, g, b, a = pixels[sy * width + sx]
            idx = y * OUT_TILE_SIZE + x
            if a < 128 or (transparent_magenta and (r, g, b) == (255, 0, 255)):
                continue
            out[idx] = rgb332_rgba((r, g, b, a))
            alpha[idx // 8] |= 1 << (idx % 8)
    return out, alpha


def sample_win_tile(
    pixels: list[tuple[int, int, int, int]],
    width: int,
    x_tile: int,
    y_tile: int,
    mask_x_tile: int,
) -> tuple[list[int], list[int]]:
    out = [0] * (OUT_TILE_SIZE * OUT_TILE_SIZE)
    alpha = [0] * ALPHA_BYTES
    for y in range(OUT_TILE_SIZE):
        sy = y_tile * WIN_TILE_SIZE + min(WIN_TILE_SIZE - 1, (y * WIN_TILE_SIZE + WIN_TILE_SIZE // 2) // OUT_TILE_SIZE)
        for x in range(OUT_TILE_SIZE):
            sx = x_tile * WIN_TILE_SIZE + min(WIN_TILE_SIZE - 1, (x * WIN_TILE_SIZE + WIN_TILE_SIZE // 2) // OUT_TILE_SIZE)
            idx = y * OUT_TILE_SIZE + x
            r, g, b, _a = pixels[sy * width + sx]
            if mask_x_tile >= 0:
                mx = mask_x_tile * WIN_TILE_SIZE + min(WIN_TILE_SIZE - 1, (x * WIN_TILE_SIZE + WIN_TILE_SIZE // 2) // OUT_TILE_SIZE)
                mr, mg, mb, _ma = pixels[sy * width + mx]
                if max(mr, mg, mb) < 128:
                    continue
            out[idx] = rgb332_rgba((r, g, b, 255))
            alpha[idx // 8] |= 1 << (idx % 8)
    return out, alpha


def build_tile_pack_from_sheet(path: Path, columns: int, rows: int, tworld_layout: bool) -> bytes:
    width, height, pixels = read_image_rgba(path)
    if columns <= 0 or rows <= 0 or width % columns or height % rows:
        raise ValueError(f"{path} is {width}x{height}, which is not divisible into a {columns}x{rows} tile grid")
    cell_w = width // columns
    cell_h = height // rows
    if cell_w <= 0 or cell_h <= 0:
        raise ValueError(f"{path} has invalid tile cell geometry")
    tiles = [[0] * (OUT_TILE_SIZE * OUT_TILE_SIZE) for _ in range(TILE_COUNT)]
    alpha = [[0] * ALPHA_BYTES for _ in range(TILE_COUNT)]
    if tworld_layout:
        for tile_id, xopaque, yopaque, xtransp, ytransp in TILE_POS:
            if tile_id >= TILE_COUNT:
                raise ValueError(f"tile id 0x{tile_id:02x} exceeds generated table")
            tiles[tile_id], alpha[tile_id] = sample_tile(
                pixels, width, xopaque * cell_w, yopaque * cell_h, cell_w, cell_h, xtransp >= 0 or ytransp >= 0
            )
    else:
        if columns * rows < TILE_COUNT:
            raise ValueError(f"{path} has only {columns * rows} grid cells; expected at least {TILE_COUNT}")
        for tile_id in range(TILE_COUNT):
            x = tile_id % columns
            y = tile_id // columns
            tiles[tile_id], alpha[tile_id] = sample_tile(
                pixels, width, x * cell_w, y * cell_h, cell_w, cell_h, True
            )
    out = bytearray(struct.pack(TILE_HEADER_FMT, TILE_MAGIC, 0, TILE_VERSION, TILE_COUNT, OUT_TILE_SIZE, ALPHA_BYTES))
    for tile in tiles:
        out.extend(tile)
    for mask in alpha:
        out.extend(mask)
    packed = bytes(out)
    validate_tile_pack(packed, path)
    return packed


def guess_grid(width: int, height: int) -> tuple[int, int] | None:
    if width % DEFAULT_TWORLD_COLUMNS == 0 and height % DEFAULT_TWORLD_ROWS == 0:
        tw_cell_w = width // DEFAULT_TWORLD_COLUMNS
        tw_cell_h = height // DEFAULT_TWORLD_ROWS
        if tw_cell_w == tw_cell_h:
            return DEFAULT_TWORLD_COLUMNS, DEFAULT_TWORLD_ROWS
    for cell in (96, 64, 48, 32, 24, 16, 12, 8):
        if width % cell == 0 and height % cell == 0:
            columns = width // cell
            rows = height // cell
            if columns * rows >= TILE_COUNT:
                return columns, rows
    return None


def prompt_int(label: str, default: int) -> int:
    while True:
        raw = input(f"{label} [{default}]: ").strip()
        if not raw:
            return default
        try:
            value = int(raw, 10)
        except ValueError:
            print("Please enter a number.")
            continue
        if value > 0:
            return value
        print("Please enter a positive number.")


def prompt_choice(label: str, default_yes: bool = True) -> bool:
    suffix = "Y/n" if default_yes else "y/N"
    while True:
        raw = input(f"{label} [{suffix}]: ").strip().lower()
        if not raw:
            return default_yes
        if raw in ("y", "yes"):
            return True
        if raw in ("n", "no"):
            return False
        print("Please answer yes or no.")


def build_tile_pack_from_image(path: Path) -> bytes:
    width, height, _pixels = read_image_rgba(path)
    guessed = guess_grid(width, height)
    if guessed and guessed == (DEFAULT_TWORLD_COLUMNS, DEFAULT_TWORLD_ROWS):
        return build_tile_pack_from_sheet(path, DEFAULT_TWORLD_COLUMNS, DEFAULT_TWORLD_ROWS, True)
    if guessed and not interactive():
        return build_tile_pack_from_sheet(path, guessed[0], guessed[1], False)
    if not interactive():
        raise ValueError(
            f"{path} is {width}x{height}, not a Tile World-style {DEFAULT_TWORLD_COLUMNS}x{DEFAULT_TWORLD_ROWS} "
            "sheet and no regular grid could be inferred. Run without arguments for interactive tile import."
        )
    print(f"Tile image is {width}x{height}.")
    if guessed:
        columns, rows = guessed
    else:
        columns = prompt_int("Tile sheet columns", DEFAULT_TWORLD_COLUMNS)
        rows = prompt_int("Tile sheet rows", DEFAULT_TWORLD_ROWS)
    tworld_layout = columns == DEFAULT_TWORLD_COLUMNS and rows == DEFAULT_TWORLD_ROWS and prompt_choice(
        "Use Tile World 7x16 tile-position mapping", True
    )
    if not tworld_layout:
        print("Using simple sequential grid order: tile id 0 is the top-left cell, then left-to-right rows.")
    return build_tile_pack_from_sheet(path, columns, rows, tworld_layout)


def build_pack(files: list[tuple[str, bytes]]) -> bytes:
    table_size = HEADER_SIZE + sum(ENTRY_SIZE + len(name.encode("utf-8")) for name, _ in files)
    data_offset = align4(table_size)
    entries = bytearray()
    payload = bytearray(b"\xff" * (data_offset - table_size))
    chunks: list[bytes] = []
    offset = data_offset

    for name, data in files:
        name_bytes = name.encode("utf-8")
        crc = binascii.crc32(data) & 0xFFFFFFFF
        entries += struct.pack(ENTRY_FMT, len(name_bytes), 0, offset, len(data), crc)
        entries += name_bytes
        chunks.append(data)
        offset = align4(offset + len(data))

    out = bytearray(struct.pack(HEADER_FMT, MAGIC, 0, VERSION, len(files)))
    out += entries
    out += payload
    for data in chunks:
        out += data
        out += b"\xff" * (align4(len(out)) - len(out))
    return bytes(out)


def main() -> int:
    args = parse_args()
    chips_dat, tiles, output = resolve_inputs(args)
    files = [("chips.dat", validate_chips_dat(chips_dat))]
    tile_data = validate_tiles(tiles)
    if tile_data is not None:
        files.append(("tiles.bin", tile_data))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(build_pack(files))
    print(f"Wrote {output} with {len(files)} Chip's Challenge payloads")
    print("Install on the SD card as /APPS/chips.pak.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
