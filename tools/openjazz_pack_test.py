#!/usr/bin/env python3
"""Regression tests for the OpenJazz host pack builder."""

from __future__ import annotations

import importlib.util
import argparse
import builtins
import os
import binascii
import struct
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILDER_PATH = ROOT / "tools" / "build_openjazz_pack.py"
CACHE_MAGIC = b"DC32JAZZXIP\0"
CACHE_VERSION = 5
CACHE_SLOT_SIZE = 0x10000
CACHE_DATA_OFFSET = CACHE_SLOT_SIZE * 2
CACHE_WRITE_SIZE = 256
CACHE_ERASE_SIZE = 0x10000
CACHE_SIZE_MAX = 0x300000
CACHE_HEADER_FMT = "<12s9I52I"
CACHE_ENTRY_FMT = "<16sIIIHHHBBHH"
CACHE_HEADER_SIZE = struct.calcsize(CACHE_HEADER_FMT)
CACHE_ENTRY_SIZE = struct.calcsize(CACHE_ENTRY_FMT)
CACHE_FINAL_SIZE = 0x200000
CACHE_FLASH_BUFFER_SIZE = 0x8000
CACHE_DECODE_SIZE = 6144 + 2048
CACHE_REQUIRED_ENTRIES = 1010
CACHE_MANIFEST_CRC = 0xCA091A34
CACHE_LEVEL_ARENA_SIZE = 99_328
FONT_CACHE_NAMES = {
    "FONT2.0FN", "FONTBIG.0FN", "FONTINY.0FN", "FONTMN1.0FN", "FONTMN2.0FN"
}
FONT_MIN_PIXELS = 32
CACHE_CHECKPOINTS = (
    (10, "BONUSFONT"),
    (19, "PANELBG1"),
    (22, "BLOCKS.000"),
    (25, "BLOCKS.001"),
    (28, "BLOCKS.002"),
    (260, "S000.231"),
    (492, "S001.231"),
    (724, "S002.231"),
    (956, "S018.231"),
    (1007, "BONUSSPR.050"),
    (1008, "BONUSBG"),
    (1009, "BONUSPAL"),
    (1010, "BONUSTILES"),
)


def expect(name: str, condition: bool) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def load_builder():
    spec = importlib.util.spec_from_file_location("build_openjazz_pack", BUILDER_PATH)
    if not spec or not spec.loader:
        raise RuntimeError(f"Cannot load {BUILDER_PATH}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def expect_rejected(name: str, action) -> None:
    try:
        action()
    except (ValueError, FileNotFoundError):
        return
    raise SystemExit(f"FAIL: {name}")


def expect_error_contains(name: str, action, text: str) -> None:
    try:
        action()
    except (ValueError, FileNotFoundError) as exc:
        expect(name, text in str(exc))
        return
    raise SystemExit(f"FAIL: {name}")


def align_up(value: int, alignment: int) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def manifest_crc(assets: list[tuple[str, int, int, int, int, int]]) -> int:
    data = bytearray()
    for name, _size, _width, _height, _pitch, entry_type in assets:
        data.extend(name.encode("ascii").ljust(16, b"\0"))
        data.extend(struct.pack("<H", entry_type))
    return binascii.crc32(data) & 0xFFFFFFFF


def refresh_cache_table_crc(cache: bytearray) -> None:
    entry_count = struct.unpack_from("<I", cache, 28)[0]
    table_size = entry_count * CACHE_ENTRY_SIZE
    struct.pack_into(
        "<I", cache, 40,
        binascii.crc32(
            cache[CACHE_HEADER_SIZE:CACHE_HEADER_SIZE + table_size]
        ) & 0xFFFFFFFF,
    )


def validate_font_cache_entry(
    name: str,
    payload: bytes,
    width: int = 128,
    height: int = 128,
    pitch: int = 128,
    color_key: int = 0,
    flags: int = 1,
) -> None:
    if (
        name not in FONT_CACHE_NAMES
        or width != 128
        or height != 128
        or pitch != 128
        or len(payload) != 128 * 128
        or color_key != 0
        or not flags & 1
        or sum(pixel != color_key for pixel in payload) < FONT_MIN_PIXELS
    ):
        raise ValueError(f"{name}: invalid font atlas")


def model_batched_cache_writes(
    assets: list[tuple[str, int, int, int, int, int]],
    fail_at: str | None = None,
) -> tuple[list[tuple[str, int, int]], str | None]:
    """Model the target writer: erase once, batch payload, table, header."""
    writes: list[tuple[str, int, int]] = [("erase", 0, CACHE_FINAL_SIZE)]
    offset = CACHE_DATA_OFFSET
    buffered = 0
    buffer_offset = CACHE_DATA_OFFSET
    failure_armed = False

    for name, size, _width, _height, _pitch, _entry_type in assets:
        if fail_at == name:
            failure_armed = True
        entry_offset = align_up(offset, CACHE_WRITE_SIZE)
        expect(f"{name} is sequential in the cache model", entry_offset == buffer_offset + buffered)
        padded = align_up(size, CACHE_WRITE_SIZE)
        remaining = padded
        while remaining:
            now = min(CACHE_FLASH_BUFFER_SIZE - buffered, remaining)
            buffered += now
            remaining -= now
            if buffered == CACHE_FLASH_BUFFER_SIZE:
                if failure_armed:
                    return writes, fail_at
                writes.append(("payload", buffer_offset, buffered))
                buffer_offset += buffered
                buffered = 0
        offset = entry_offset + padded

    if buffered:
        writes.append(("payload", buffer_offset, buffered))
    table_size = align_up(CACHE_REQUIRED_ENTRIES * CACHE_ENTRY_SIZE, CACHE_WRITE_SIZE)
    writes.append(("table", CACHE_HEADER_SIZE, table_size))
    writes.append(("header", 0, CACHE_HEADER_SIZE))
    return writes, None


def model_transaction_build(
    assets: list[tuple[str, int, int, int, int, int]],
    operations: list[tuple[str, int]],
) -> list[tuple[str, int]]:
    committed: list[tuple[str, int]] = []
    for name, entry_type in operations:
        index = len(committed)
        if index >= len(assets):
            raise ValueError(f"{name}: transaction after complete manifest")
        expected_name = assets[index][0]
        expected_type = assets[index][5]
        if name != expected_name or entry_type != expected_type:
            raise ValueError(
                f"{name}: expected {expected_name}/{expected_type} at entry {index}"
            )
        committed.append((name, entry_type))
    return committed


def model_cache_checkpoint(
    assets: list[tuple[str, int, int, int, int, int]],
    committed: list[tuple[str, int] | None],
    counter: int,
    expected_count: int,
    expected_last: str,
) -> None:
    if counter != len(committed):
        raise ValueError(f"counter {counter}; table {len(committed)}")
    if any(entry is None for entry in committed):
        raise ValueError("entry table corrupt")
    if counter != expected_count:
        missing = assets[counter][0] if counter < len(assets) else "cache"
        raise ValueError(
            f"{missing}: have {counter} entries; expected {expected_count}"
        )
    if not committed or committed[-1][0] != expected_last:
        raise ValueError(f"{expected_last}: checkpoint final entry mismatch")


def model_cache_seal(
    assets: list[tuple[str, int, int, int, int, int]],
    committed: list[tuple[str, int] | None],
    counter: int,
    pending: tuple[str, int, int, int, bool] | None = None,
) -> list[tuple[str, int] | None]:
    result = list(committed)
    if pending is not None:
        name, entry_type, written, size, failed = pending
        expected = assets[counter] if counter < len(assets) else None
        if (
            counter != CACHE_REQUIRED_ENTRIES - 1
            or failed
            or written != size
            or expected is None
            or name != "BONUSTILES"
            or name != expected[0]
            or entry_type != expected[5]
        ):
            raise ValueError(f"{name}: pending {written}/{size} at entry {counter}")
        result.append((name, entry_type))
        counter += 1
    model_cache_checkpoint(
        assets, result, counter, CACHE_REQUIRED_ENTRIES, "BONUSTILES"
    )
    return result


def model_checked_promotion(name: str, committed: bool) -> None:
    if not committed:
        raise ValueError(f"{name}: cache promotion finalize failed")


def build_cache_fixture(
    assets: list[tuple[str, int, int, int, int, int]],
    fingerprint: int,
    pack_size: int,
    version: int = CACHE_VERSION,
) -> bytes:
    entries = []
    offset = CACHE_DATA_OFFSET
    cache = bytearray(b"\xff" * CACHE_SIZE_MAX)

    for index, (name, size, width, height, pitch, entry_type) in enumerate(assets):
        offset = align_up(offset, CACHE_WRITE_SIZE)
        payload = bytes(((index * 37 + position) & 0xFF) for position in range(size))
        color_key = 0 if name in FONT_CACHE_NAMES else 254
        visible = (
            min(sum(pixel != color_key for pixel in payload), FONT_MIN_PIXELS)
            if name in FONT_CACHE_NAMES else 0
        )
        cache[offset:offset + size] = payload
        entries.append(struct.pack(
            CACHE_ENTRY_FMT,
            name.encode("ascii").ljust(16, b"\0"),
            offset,
            size,
            binascii.crc32(payload) & 0xFFFFFFFF,
            width,
            height,
            pitch,
            color_key,
            1,
            entry_type,
            visible if name in FONT_CACHE_NAMES else 0,
        ))
        offset += size

    total_size = align_up(offset, CACHE_ERASE_SIZE)
    table = b"".join(entries)
    header = struct.pack(
        CACHE_HEADER_FMT,
        CACHE_MAGIC,
        version,
        1,
        fingerprint,
        pack_size,
        len(entries),
        total_size,
        CACHE_DATA_OFFSET,
        binascii.crc32(table) & 0xFFFFFFFF,
        0,
        *([0] * 52),
    )
    cache[:CACHE_HEADER_SIZE] = header
    cache[CACHE_HEADER_SIZE:CACHE_HEADER_SIZE + len(table)] = table
    return bytes(cache[:total_size])


def verify_cache_fixture(cache: bytes, fingerprint: int, pack_size: int) -> list[str]:
    if len(cache) < CACHE_HEADER_SIZE:
        raise ValueError("truncated cache header")
    unpacked = struct.unpack_from(CACHE_HEADER_FMT, cache)
    (
        magic,
        version,
        _generation,
        source_fingerprint,
        source_pack_size,
        entry_count,
        total_size,
        data_offset,
        table_crc,
        _payload_crc,
        *_reserved,
    ) = unpacked
    if (
        magic != CACHE_MAGIC
        or version != CACHE_VERSION
        or source_fingerprint != fingerprint
        or source_pack_size != pack_size
        or data_offset != CACHE_DATA_OFFSET
        or entry_count != CACHE_REQUIRED_ENTRIES
        or total_size < CACHE_DATA_OFFSET
        or total_size > CACHE_SIZE_MAX
        or total_size % CACHE_ERASE_SIZE
        or len(cache) < total_size
    ):
        raise ValueError("invalid cache header")

    table_size = entry_count * CACHE_ENTRY_SIZE
    table = cache[CACHE_HEADER_SIZE:CACHE_HEADER_SIZE + table_size]
    if len(table) != table_size or (binascii.crc32(table) & 0xFFFFFFFF) != table_crc:
        raise ValueError("invalid cache table")

    names = []
    manifest = bytearray()
    next_offset = CACHE_DATA_OFFSET
    font_entries = []
    for index in range(entry_count):
        (
            raw_name,
            offset,
            size,
            crc,
            width,
            height,
            pitch,
            color_key,
            flags,
            entry_type,
            reserved,
        ) = struct.unpack_from(CACHE_ENTRY_FMT, table, index * CACHE_ENTRY_SIZE)
        name = raw_name.split(b"\0", 1)[0].decode("ascii")
        if (
            not name
            or offset != next_offset
            or offset % CACHE_WRITE_SIZE
            or offset > total_size
            or size > total_size - offset
            or not width
            or not height
            or pitch < width
            or size != pitch * height
            or entry_type not in (1, 2)
        ):
            raise ValueError("invalid cache entry")
        next_offset = offset + align_up(size, CACHE_WRITE_SIZE)
        manifest.extend(raw_name)
        manifest.extend(struct.pack("<H", entry_type))
        if 3 <= index <= 7:
            font_entries.append(
                (name, size, width, height, pitch, color_key, flags, crc, reserved)
            )
        names.append(name)
    if (binascii.crc32(manifest) & 0xFFFFFFFF) != CACHE_MANIFEST_CRC:
        raise ValueError("invalid cache manifest")
    expected_fonts = [
        "FONT2.0FN", "FONTBIG.0FN", "FONTINY.0FN", "FONTMN1.0FN", "FONTMN2.0FN"
    ]
    for expected, entry in zip(expected_fonts, font_entries):
        name, size, width, height, pitch, color_key, flags, crc, reserved = entry
        if (
            name != expected
            or size != 128 * 128
            or width != 128
            or height != 128
            or pitch != 128
            or color_key != 0
            or not flags & 1
            or reserved < FONT_MIN_PIXELS
            or crc == 0xAB54D286
        ):
            raise ValueError(f"{expected}: invalid font atlas")
    return names


def parse_jj1_sprite(data: bytes, position: int) -> tuple[int, int]:
    start = position
    width, height = struct.unpack_from("<HH", data, position)
    width *= 4
    mask_offset, pixel_offset = struct.unpack_from("<HH", data, position + 6)
    pixel_offset *= 4
    if mask_offset:
        height += 1
        position = start + 10 + mask_offset + (width // 4) * height + pixel_offset
    elif width:
        position = start + 10 + width * height
    else:
        position = start + 10
    return position, width * height


def decode_masked_pixels_reference(
    mask: bytes, encoded: bytes, length: int, key: int
) -> bytes:
    """Model the original two-transpose OpenJazz masked decoder."""
    if length <= 0 or length & 3:
        raise ValueError("invalid masked pixel length")
    quarter = length // 4
    if len(mask) != quarter:
        raise ValueError("invalid masked pixel mask")

    expanded_mask = bytearray(length)
    for count in range(length):
        expanded_mask[count] = (mask[count >> 2] >> (count & 3)) & 1
    scrambled_mask = bytearray(length)
    for count, value in enumerate(expanded_mask):
        scrambled_mask[(count >> 2) + ((count & 3) * quarter)] = value

    scrambled_pixels = bytearray([key] * length)
    encoded_position = 0
    for source, opaque in enumerate(scrambled_mask):
        if not opaque:
            continue
        while encoded_position < len(encoded) and encoded[encoded_position] == key:
            encoded_position += 1
        if encoded_position >= len(encoded):
            raise ValueError("truncated masked pixel stream")
        scrambled_pixels[source] = encoded[encoded_position]
        encoded_position += 1
    if any(value != key for value in encoded[encoded_position:]):
        raise ValueError("extra nontransparent masked pixels")
    return bytes(
        scrambled_pixels[(count >> 2) + ((count & 3) * quarter)]
        for count in range(length)
    )


def decode_masked_pixels_bounded(
    mask: bytes, encoded: bytes, length: int, key: int
) -> bytes:
    """Model the bounded DC32 decoder and its inverse planar mapping."""
    if length <= 0 or length & 3:
        raise ValueError("invalid masked pixel length")
    quarter = length // 4
    if len(mask) != quarter:
        raise ValueError("invalid masked pixel mask")

    output = bytearray([key] * length)
    encoded_position = 0
    for source in range(length):
        plane = source // quarter
        if not (mask[source % quarter] & (1 << plane)):
            continue
        while encoded_position < len(encoded) and encoded[encoded_position] == key:
            encoded_position += 1
        if encoded_position >= len(encoded):
            raise ValueError("truncated masked pixel stream")
        output[(source % quarter) * 4 + plane] = encoded[encoded_position]
        encoded_position += 1
    if any(value != key for value in encoded[encoded_position:]):
        raise ValueError("extra nontransparent masked pixels")
    return bytes(output)


def parse_and_compare_jj1_sprite(
    data: bytes, position: int, key: int = 254
) -> tuple[int, bool]:
    if position + 10 > len(data):
        raise ValueError("truncated JJ1 sprite header")
    start = position
    width, height = struct.unpack_from("<HH", data, start)
    width *= 4
    mask_offset, pixel_offset = struct.unpack_from("<HH", data, start + 6)
    pixel_offset *= 4
    position = start + 10
    if mask_offset:
        height += 1
        length = width * height
        mask_start = position + mask_offset
        quarter = length // 4
        encoded_start = mask_start + quarter
        end = encoded_start + pixel_offset
        if length <= 0 or length & 3 or end > len(data):
            raise ValueError("invalid JJ1 masked sprite bounds")
        mask = data[mask_start:encoded_start]
        encoded = data[encoded_start:end]
        if decode_masked_pixels_bounded(mask, encoded, length, key) != (
            decode_masked_pixels_reference(mask, encoded, length, key)
        ):
            raise ValueError("bounded JJ1 sprite decode differs from original")
        return end, True
    if width:
        end = position + width * height
        if end > len(data):
            raise ValueError("truncated JJ1 unmasked sprite")
        return end, False
    return position, False


def validate_jj1_sprite_file(
    data: bytes, position: int, count: int
) -> tuple[int, int]:
    records = 0
    masked = 0
    for _index in range(count):
        if position >= len(data):
            break
        if data[position] == 0xFF:
            if position + 2 > len(data):
                raise ValueError("truncated JJ1 sprite marker")
            position += 2
            continue
        position, was_masked = parse_and_compare_jj1_sprite(data, position)
        records += 1
        masked += int(was_masked)
    if position != len(data):
        raise ValueError("JJ1 sprite file has an unparsed tail")
    return records, masked


def normal_world_sprite_sizes(spec_data: bytes, main_data: bytes) -> list[int]:
    sprite_count = struct.unpack_from("<H", spec_data)[0]
    spec_position = 2 + sprite_count * 2
    main_position = 2
    sizes = []

    for index in range(sprite_count):
        main_size = 0
        spec_size = 0
        loaded = False
        if main_data[main_position] == 0xFF:
            main_position += 2
        else:
            main_position, main_size = parse_jj1_sprite(main_data, main_position)
            loaded = True
        if spec_data[spec_position] == 0xFF:
            spec_position += 2
        else:
            spec_position, spec_size = parse_jj1_sprite(spec_data, spec_position)
            loaded = True
        sizes.append(max(1, spec_size or main_size) if loaded else 1)
        if spec_position >= len(spec_data):
            sizes.extend([1] * (sprite_count - index - 1))
            break
    sizes.append(1)  # OpenJazz appends one blank sentinel sprite.
    return sizes


def bonus_sprite_records(data: bytes) -> list[tuple[int, int, int, int, int, int]]:
    position = 2
    if len(data) < position + 2:
        raise ValueError("truncated bonus sprite count")
    sprite_count = struct.unpack_from("<H", data, position)[0]
    position += 2
    records = []

    for index in range(sprite_count):
        record_offset = position
        if position + 8 > len(data):
            raise ValueError(f"BONUSSPR.{index:03d}: truncated header")
        width, height, pixels_length, mask_length = struct.unpack_from(
            "<HHHH", data, position
        )
        position += 8
        if width > 320 or height > 200:
            raise ValueError(f"BONUSSPR.{index:03d}: invalid dimensions")
        if not width:
            if height != 1 or pixels_length != 0 or mask_length != 0:
                raise ValueError(f"BONUSSPR.{index:03d}: malformed blank sprite")
            records.append((record_offset, width, height, pixels_length, mask_length, 1))
            continue
        if not height:
            raise ValueError(f"BONUSSPR.{index:03d}: zero height")
        if pixels_length != 0xFFFF:
            width *= 4
            area = width * height
            if mask_length != area // 4:
                raise ValueError(f"BONUSSPR.{index:03d}: invalid mask size")
            mask = data[position:position + mask_length]
            encoded_start = position + mask_length
            encoded_end = encoded_start + pixels_length * 4
            encoded = data[encoded_start:encoded_end]
            if len(mask) != mask_length or len(encoded) != pixels_length * 4:
                raise ValueError(f"BONUSSPR.{index:03d}: truncated masked stream")
            encoded_pos = 0
            quarter = area // 4
            for source in range(area):
                plane = source // quarter
                if mask[source % quarter] & (1 << plane):
                    while encoded_pos < len(encoded) and encoded[encoded_pos] == 0:
                        encoded_pos += 1
                    if encoded_pos >= len(encoded):
                        raise ValueError(f"BONUSSPR.{index:03d}: truncated encoded pixels")
                    encoded_pos += 1
            if any(encoded[encoded_pos:]):
                raise ValueError(f"BONUSSPR.{index:03d}: extra encoded pixels")
            position += pixels_length * 4 + mask_length
        else:
            if position + width * height > len(data):
                raise ValueError(f"BONUSSPR.{index:03d}: truncated pixel stream")
            position += width * height
        records.append((record_offset, width, height, pixels_length, mask_length, width * height))
    return records


def bonus_sprite_sizes(data: bytes) -> list[int]:
    return [record[5] for record in bonus_sprite_records(data)]


def validate_bonus_sprite_pixels(data: bytes) -> int:
    masked = 0
    for record in bonus_sprite_records(data):
        offset, width, height, pixels_length, mask_length, length = record
        if not width or pixels_length == 0xFFFF:
            continue
        payload = offset + 8
        mask = data[payload:payload + mask_length]
        encoded = data[
            payload + mask_length:
            payload + mask_length + pixels_length * 4
        ]
        if decode_masked_pixels_bounded(mask, encoded, length, 0) != (
            decode_masked_pixels_reference(mask, encoded, length, 0)
        ):
            raise ValueError("bounded bonus sprite decode differs from original")
        masked += 1
    return masked


def panel_ammo_surfaces(panel: bytes) -> list[bytes]:
    if len(panel) != 46_272:
        raise ValueError("invalid PANEL.000 decoded length")
    return [
        bytes(
            panel[
                ammo_type * 64 * 32
                + y * 64
                + (x >> 2)
                + ((x & 3) << 4)
                + 55 * 320
            ]
            for y in range(26)
            for x in range(64)
        )
        for ammo_type in range(6)
    ]


def decode_rle(data: bytes, position: int, length: int) -> tuple[bytes, int]:
    block_size = struct.unpack_from("<H", data, position)[0]
    position += 2
    end = position + block_size
    output = bytearray()
    while len(output) < length:
        code = data[position]
        position += 1
        amount = code & 0x7F
        if code & 0x80:
            value = data[position]
            position += 1
            output.extend([value] * amount)
        elif amount:
            output += data[position:position + amount]
            position += amount
        else:
            output.append(data[position])
            position += 1
            break
    if position != end or len(output) != length:
        raise ValueError("invalid RLE block")
    return bytes(output), end


def decode_rle_block(data: bytes, position: int) -> tuple[bytes, int]:
    if position + 2 > len(data):
        raise ValueError("truncated RLE header")
    block_size = struct.unpack_from("<H", data, position)[0]
    position += 2
    end = position + block_size
    if end > len(data):
        raise ValueError("truncated RLE block")
    output = bytearray()
    terminated = False
    while position < end:
        code = data[position]
        position += 1
        amount = code & 0x7F
        if code & 0x80:
            if position >= end:
                raise ValueError("truncated RLE repeat")
            output.extend([data[position]] * amount)
            position += 1
        elif amount:
            if position + amount > end:
                raise ValueError("truncated RLE copy")
            output += data[position:position + amount]
            position += amount
        else:
            if position >= end:
                raise ValueError("truncated RLE terminator")
            output.append(data[position])
            position += 1
            terminated = True
            if position != end:
                raise ValueError("RLE terminator precedes block end")
    if not terminated:
        raise ValueError("RLE block lacks terminator")
    return bytes(output), end


def decode_rle_prefix_block(
    data: bytes, position: int, prefix_length: int
) -> tuple[bytes, int, int]:
    output, end = decode_rle_block(data, position)
    if len(output) < prefix_length:
        raise ValueError("RLE block shorter than required prefix")
    return output[:prefix_length], end, len(output)


def skip_rle(data: bytes, position: int) -> int:
    return position + 2 + struct.unpack_from("<H", data, position)[0]


def normal_level_limits(data: bytes) -> tuple[int, int, int]:
    position = 39
    _grid, position = decode_rle(data, position, 256 * 64 * 2)
    position = skip_rle(data, position)
    _masks, position = decode_rle(data, position, 2048)
    paths, position = decode_rle(data, position, 16 * 512)
    position = skip_rle(data, position)  # event definitions are validated at runtime
    position = skip_rle(data, position)  # event names
    animations, _position = decode_rle(data, position, 128 * 64)
    path_points = sum(struct.unpack_from("<H", paths, index * 512)[0] or 1 for index in range(16))
    frame_counts = [animations[index * 64 + 6] for index in range(128)]
    return sum(frame_counts), max(frame_counts), path_points


def normal_level_runtime_streams(
    data: bytes,
) -> tuple[list[int], bytes, int, bytes, int]:
    position = 39
    decoded_lengths = []
    for _index in range(8):
        block, position = decode_rle_block(data, position)
        decoded_lengths.append(len(block))
    position += 16 * (8 + 1) + 9
    position += 32 * 2
    for _index in range(32):
        position += 1 + 8
    position += 1 + 12
    position += 13
    position += 1 + 12
    position += 39
    position += 4 + 2 + 2 + 2 + 2 + 1 + 2
    player_prefix, position, player_decoded = decode_rle_prefix_block(
        data, position, 38 * 2
    )
    position += 4
    bullets, position = decode_rle_block(data, position)
    _attack_names, position = decode_rle_block(data, position)
    return decoded_lengths, player_prefix, player_decoded, bullets, position


def normal_level_trailer(data: bytes) -> tuple[bytes, bytes]:
    position = normal_level_runtime_streams(data)[4]
    if len(data) - position != 50:
        raise ValueError("normal level has an invalid final position")
    trailer = data[position:position + 25]
    reserved = data[position + 25:]
    if len(trailer) != 25 or len(reserved) != 25:
        raise ValueError("truncated normal level trailer")
    if trailer[0] not in (0, 2, 8, 9, 11):
        raise ValueError("invalid background palette effect")
    if trailer[1] > 1 or (trailer[1] and trailer[2] >= 240):
        raise ValueError("invalid sky-orb fields")
    if any(sound >= 33 for sound in trailer[3:14]):
        raise ValueError("invalid level sound mapping")
    if any(animation >= 128 for animation in trailer[14:25]):
        raise ValueError("invalid level animation mapping")
    return trailer, reserved


def validate_player_mapping(prefix: bytes) -> list[int]:
    if len(prefix) != 38 * 2:
        raise ValueError("invalid player mapping prefix")
    refs = list(prefix[::2])
    if any(ref >= 128 for ref in refs):
        raise ValueError("invalid player animation reference")
    return refs


def bonus_level_limits(data: bytes) -> tuple[int, int]:
    position = 10 * 9
    position += 1 + 8  # tileset filename
    position += 9 + 13
    position += 1 + 12  # music filename
    animations = data[position:position + 32 * 64]
    position += 32 * 64 + 32 * 16
    frame_counts = [animations[index * 64 + 6] for index in range(32)]
    _tiles, position = decode_rle(data, position, 256 * 256 - 1)
    _events, position = decode_rle(data, position, 16 * 16)
    _mapping, _position = decode_rle(data, position, 256 * 256 - 1)
    return sum(frame_counts), max(frame_counts)


def main() -> int:
    builder = load_builder()
    shareware = ROOT / "third_party" / "openjazz-shareware" / "JAZZ.ZIP"

    with tempfile.TemporaryDirectory() as tmp_name:
        tmp = Path(tmp_name)
        extracted = builder.extract_zip(shareware, tmp / "shareware")
        _data_root, entries = builder.collect_entries(extracted)
        expect("Original shareware contains 57 runtime files", len(entries) == 57)
        expect("Numbered shareware worlds are retained", {"LEVEL0.001", "BLOCKS.001", "SPRITES.001"} <= {entry.name for entry in entries})
        raw = builder.build_pack(entries)
        names = [entry.name for entry in entries]
        builder.verify_pack(raw, names)
        fingerprint = 0
        for entry in entries:
            data = entry.path.read_bytes()
            fingerprint = binascii.crc32(
                entry.name.encode("ascii") + b"\0" +
                struct.pack("<II", len(data), binascii.crc32(data) & 0xFFFFFFFF),
                fingerprint,
            )
        expect("Bundled shareware normalized fingerprint", fingerprint == 0xE9E3F7BD)
        expect("Bundled shareware pack size", len(raw) == 2_906_760)

        decoded_tiles = 0
        tile_sizes = {}
        for entry in entries:
            if not entry.name.startswith("BLOCKS."):
                continue
            data = entry.path.read_bytes()
            pos = 0
            for expected in (768, 768, None):
                size = struct.unpack_from("<H", data, pos)[0]
                pos += 2 + size
                if expected is None:
                    break
            entry_decoded_tiles = 0
            for _tileset in range(4):
                marker = data[pos:pos + 2]
                pos += 2
                if marker == b"ok":
                    decoded_tiles += 60 * 1024
                    entry_decoded_tiles += 60 * 1024
                    for _tile in range(60):
                        size = struct.unpack_from("<H", data, pos)[0]
                        pos += 2 + size
                else:
                    expect("Blocks empty marker", marker == b"  ")
            expect(f"{entry.name} parses to its end", pos == len(data))
            tile_sizes[entry.name] = entry_decoded_tiles
        expect("Shareware tile cache decoded size", decoded_tiles == 552_960)
        expect("Decoded tile cache fits shared XIP", 0x20000 + decoded_tiles < 0x300000)

        entry_data = {entry.name: entry.path.read_bytes() for entry in entries}
        opaque_mask = bytes([0x0F] * 6)
        encoded_fixture = bytes(range(1, 25))
        reference_fixture = decode_masked_pixels_reference(
            opaque_mask, encoded_fixture, 12 * 2, 254
        )
        bounded_fixture = decode_masked_pixels_bounded(
            opaque_mask, encoded_fixture, 12 * 2, 254
        )
        wrong_fixture = bytearray([254] * 24)
        for source, value in enumerate(encoded_fixture):
            wrong_fixture[(source >> 2) + ((source & 3) * 6)] = value
        expect(
            "Non-square masked sprites use the inverse planar transpose",
            bounded_fixture == reference_fixture
            and bytes(wrong_fixture) != reference_fixture,
        )
        expect_rejected(
            "Truncated sprite masks are rejected",
            lambda: decode_masked_pixels_bounded(
                opaque_mask[:-1], encoded_fixture, 24, 254
            ),
        )
        expect_rejected(
            "Truncated masked sprite streams are rejected",
            lambda: decode_masked_pixels_bounded(
                opaque_mask, encoded_fixture[:-1], 24, 254
            ),
        )
        transparent_encoded = b"".join(
            (bytes((254, value)) for value in encoded_fixture)
        )
        expect(
            "Transparent-key bytes are skipped without shifting sprite pixels",
            decode_masked_pixels_bounded(
                opaque_mask, transparent_encoded, 24, 254
            ) == reference_fixture,
        )
        expect_rejected(
            "Nontransparent masked sprite tails are rejected",
            lambda: decode_masked_pixels_bounded(
                opaque_mask, encoded_fixture + b"\x7f", 24, 254
            ),
        )

        main_sprite_count = struct.unpack_from(
            "<H", entry_data["MAINCHAR.000"]
        )[0]
        normal_decode_counts = [
            validate_jj1_sprite_file(
                entry_data["MAINCHAR.000"], 2, main_sprite_count
            )
        ]
        for world in ("000", "001", "002", "018"):
            data = entry_data[f"SPRITES.{world}"]
            count = struct.unpack_from("<H", data)[0]
            normal_decode_counts.append(
                validate_jj1_sprite_file(data, 2 + count * 2, count)
            )
        expect(
            "Every bundled normal sprite record matches original decoding",
            normal_decode_counts
            == [(212, 91), (19, 19), (19, 19), (19, 17), (19, 19)],
        )
        expect(
            "Every bundled masked bonus sprite matches original decoding",
            validate_bonus_sprite_pixels(entry_data["BONUS.000"]) == 45,
        )

        panel_pixels, panel_end = decode_rle(
            entry_data["PANEL.000"], 0, 46_272
        )
        ammo_surfaces = panel_ammo_surfaces(panel_pixels)
        expect(
            "PANEL.000 decodes exactly before ammo extraction",
            panel_end == len(entry_data["PANEL.000"]),
        )
        expect(
            "All six cached ammo strips match bundled shareware",
            [binascii.crc32(surface) & 0xFFFFFFFF for surface in ammo_surfaces]
            == [
                0x0DCC777F, 0xDAAEE4F2, 0x4AD6B491,
                0x42078DA8, 0x3A49FF0F, 0x04CF0D5D,
            ],
        )
        expect(
            "The default blaster ammo strip is visibly nonblank",
            len(set(ammo_surfaces[0])) > 1,
        )

        sprite_sizes = []
        normal_sprite_sets = {}
        for world in ("000", "001", "002", "018"):
            normal_sprite_sets[world] = normal_world_sprite_sizes(
                entry_data[f"SPRITES.{world}"], entry_data["MAINCHAR.000"]
            )
            sprite_sizes.extend(normal_sprite_sets[world])
        bonus_sizes = bonus_sprite_sizes(entry_data["BONUS.000"])
        bonus_records = bonus_sprite_records(entry_data["BONUS.000"])
        sprite_sizes.extend(bonus_sizes)
        expect("All shareware sprite sets are represented", len(sprite_sizes) == 979)
        expect("All 51 bonus sprite records parse", len(bonus_records) == 51)
        expect(
            "BONUSSPR.014 is the canonical transparent blank",
            bonus_records[14][1:] == (0, 1, 0, 0, 1),
        )
        expect(
            "Bonus parsing continues after the blank through sprite 50",
            bonus_records[15][0] > bonus_records[14][0]
            and bonus_records[50][0] > bonus_records[15][0],
        )
        expect("Decoded shareware sprite cache size", sum(sprite_sizes) == 965_515)
        expect("Normal sprites fit fixed decode workspace", max(sprite_sizes[:-len(bonus_sizes)]) == 5_280)
        expect("Bonus sprites fit fixed decode workspace", max(bonus_sizes) == 6_004)
        expect("Largest sprite mask fits fixed workspace", max(sprite_sizes) // 4 <= 2_048)

        blank_offset = bonus_records[14][0]
        malformed_blank = bytearray(entry_data["BONUS.000"])
        struct.pack_into("<H", malformed_blank, blank_offset + 2, 2)
        expect_error_contains(
            "Malformed blank bonus sprite names BONUSSPR.014",
            lambda: bonus_sprite_records(bytes(malformed_blank)),
            "BONUSSPR.014",
        )
        masked_header = struct.pack("<HHHH", 1, 1, 1, 1)
        expect_error_contains(
            "Truncated bonus sprite mask names its asset",
            lambda: bonus_sprite_records(b"\0\0\1\0" + masked_header),
            "BONUSSPR.000",
        )
        expect_error_contains(
            "Truncated bonus encoded stream names its asset",
            lambda: bonus_sprite_records(
                b"\0\0\1\0" + masked_header + b"\1\1\0"
            ),
            "BONUSSPR.000",
        )
        expect_error_contains(
            "Extra nontransparent bonus pixels name their asset",
            lambda: bonus_sprite_records(
                b"\0\0\1\0" + masked_header + b"\1\1\2\0\0"
            ),
            "BONUSSPR.000",
        )

        normal_limits = [
            normal_level_limits(data)
            for name, data in entry_data.items()
            if name.startswith("LEVEL") and name[-3:].isdigit()
        ]
        expect("All eight bundled normal levels parse", len(normal_limits) == 8)
        expect("Normal animation frame pool bound", max(limit[0] for limit in normal_limits) == 531)
        expect("Normal animation per-record bound", max(limit[1] for limit in normal_limits) == 19)
        expect("Normal path pool bound", max(limit[2] for limit in normal_limits) == 294)

        runtime_streams = [
            normal_level_runtime_streams(data)
            for name, data in entry_data.items()
            if name.startswith("LEVEL") and name[-3:].isdigit()
        ]
        expected_core_lengths = [
            256 * 64 * 2, 256, 2048, 16 * 512,
            127 * 32, 127 * 16, 128 * 64, 128 * 16,
        ]
        expect(
            "Normal level core streams retain exact decoded lengths",
            all(stream[0] == expected_core_lengths for stream in runtime_streams),
        )
        expect(
            "All normal player mapping blocks decode to 352 bytes",
            all(stream[2] == 352 for stream in runtime_streams),
        )
        player_refs = [
            validate_player_mapping(stream[1]) for stream in runtime_streams
        ]
        expect(
            "Player mapping prefixes provide 38 valid references",
            all(len(refs) == 38 and min(refs) == 64 and max(refs) == 101
                for refs in player_refs),
        )
        expect(
            "Bullets remain aligned after player mapping tails",
            all(len(stream[3]) == 32 * 20 for stream in runtime_streams),
        )
        normal_files = [
            data for name, data in entry_data.items()
            if name.startswith("LEVEL") and name[-3:].isdigit()
        ]
        trailers = [normal_level_trailer(data) for data in normal_files]
        expect(
            "All normal levels have validated 25-byte runtime trailers",
            len(trailers) == 8
            and all(len(trailer) == 25 and len(reserved) == 25
                    for trailer, reserved in trailers),
        )
        expect(
            "JJ1 full-frame cinematics exceed the guaranteed transition heap",
            320 * 200 > 56 * 1024
            and len(entry_data["ENDLEVEL.0SC"]) > 0,
        )
        truncated_trailer = normal_files[0][:-26]
        expect_rejected(
            "Truncated normal level trailers are rejected",
            lambda: normal_level_trailer(truncated_trailer),
        )
        trailer_position = normal_level_runtime_streams(normal_files[0])[4]
        invalid_palette = bytearray(normal_files[0])
        invalid_palette[trailer_position] = 7
        expect_rejected(
            "Invalid trailer palette modes are rejected",
            lambda: normal_level_trailer(bytes(invalid_palette)),
        )
        invalid_animation = bytearray(normal_files[0])
        invalid_animation[trailer_position + 24] = 128
        expect_rejected(
            "Invalid trailer animation references are rejected",
            lambda: normal_level_trailer(bytes(invalid_animation)),
        )
        expect_rejected(
            "Normal level trailer final-position mismatches are rejected",
            lambda: normal_level_trailer(normal_files[0] + b"\0"),
        )
        expect_rejected(
            "Truncated player mapping block is rejected",
            lambda: decode_rle_prefix_block(b"\4\0\xca\1\0", 0, 76),
        )
        expect_rejected(
            "Player mapping shorter than 76 bytes is rejected",
            lambda: decode_rle_prefix_block(b"\4\0\xca\1\0\1", 0, 76),
        )
        expect_rejected(
            "Malformed player RLE command is rejected",
            lambda: decode_rle_prefix_block(b"\1\0\x81", 0, 76),
        )
        expect_rejected(
            "Player RLE data after its terminator is rejected",
            lambda: decode_rle_prefix_block(b"\3\0\0\1\2", 0, 1),
        )
        invalid_mapping = bytearray(runtime_streams[0][1])
        invalid_mapping[0] = 128
        expect_rejected(
            "Invalid player animation reference is rejected",
            lambda: validate_player_mapping(bytes(invalid_mapping)),
        )

        bonus_limits = [
            bonus_level_limits(entry_data[f"BONUSMAP.{world}"])
            for world in ("000", "001", "002")
        ]
        expect("Bonus animation frame pool bound", max(limit[0] for limit in bonus_limits) == 41)
        expect("Bonus animation per-record bound", max(limit[1] for limit in bonus_limits) == 6)

        bonus_background, bonus_y_pos = decode_rle_block(
            entry_data["BONUSY.000"], 0
        )
        expect(
            "Bonus background source is the complete 832x32 block",
            len(bonus_background) == 832 * 32,
        )
        cropped_background = b"".join(
            bonus_background[row * 832:row * 832 + 512]
            for row in range(20)
        )
        expect(
            "Bonus background cache crops exactly 512x20",
            len(cropped_background) == 512 * 20
            and (binascii.crc32(cropped_background) & 0xFFFFFFFF) == 0xA5C43C30,
        )
        expect(
            "Bonus background parser consumes the complete source block",
            bonus_y_pos == 2 + struct.unpack_from(
                "<H", entry_data["BONUSY.000"], 0
            )[0],
        )
        expect_rejected(
            "Truncated bonus background block is rejected",
            lambda: decode_rle_block(
                entry_data["BONUSY.000"][:bonus_y_pos - 1], 0
            ),
        )

        fixed_surface_sizes = (
            [46_272]
            + [128 * 128] * 5
            + [56 * 48, 32 * 28, 160 * 160, 160 * 160]
            + [320 * 32]
            + [64 * 26] * 6
            + [32 * 32] * 2
            + list(tile_sizes.values())
            + [32 * 1920, 512 * 20]
            + [1024] * 7
        )
        all_surface_sizes = fixed_surface_sizes + sprite_sizes
        expect("All required cache entries are counted", len(all_surface_sizes) == 1010)
        cache_end = CACHE_DATA_OFFSET
        for surface_size in all_surface_sizes:
            cache_end = align_up(cache_end, CACHE_WRITE_SIZE) + surface_size
        cache_end = align_up(cache_end, CACHE_ERASE_SIZE)
        expect("Complete optimized cache footprint", cache_end == 0x200000)
        expect("Complete optimized cache fits shared XIP", cache_end <= CACHE_SIZE_MAX)

        asset_specs: list[tuple[str, int, int, int, int, int]] = [
            ("PANELRAW", 46_272, 1, 46_272, 1, 1),
            ("PANELBIG", 56 * 48, 56, 48, 56, 1),
            ("PANELSMALL", 32 * 28, 32, 28, 32, 1),
            ("FONT2.0FN", 128 * 128, 128, 128, 128, 1),
            ("FONTBIG.0FN", 128 * 128, 128, 128, 128, 1),
            ("FONTINY.0FN", 128 * 128, 128, 128, 128, 1),
            ("FONTMN1.0FN", 128 * 128, 128, 128, 128, 1),
            ("FONTMN2.0FN", 128 * 128, 128, 128, 128, 1),
            ("LEVELFONT", 160 * 160, 160, 160, 160, 1),
            ("BONUSFONT", 160 * 160, 160, 160, 160, 1),
            ("PANELHUD", 320 * 32, 320, 32, 320, 1),
            *[
                (f"PANELAMMO{index}", 64 * 26, 64, 26, 64, 1)
                for index in range(6)
            ],
            ("PANELBG0", 32 * 32, 32, 32, 32, 1),
            ("PANELBG1", 32 * 32, 32, 32, 32, 1),
        ]
        for world in ("000", "001", "002"):
            asset_specs.extend([
                (f"PAL{world}A", 1024, 1024, 1, 1024, 2),
                (f"PAL{world}B", 1024, 1024, 1, 1024, 2),
                (f"BLOCKS.{world}", tile_sizes[f"BLOCKS.{world}"], 32,
                 tile_sizes[f"BLOCKS.{world}"] // 32, 32, 1),
            ])
        for world in ("000", "001", "002", "018"):
            asset_specs.extend(
                (f"S{world}.{index:03d}", size, 1, size, 1, 1)
                for index, size in enumerate(normal_sprite_sets[world])
            )
        asset_specs.extend(
            (f"BONUSSPR.{index:03d}", size, 1, size, 1, 1)
            for index, size in enumerate(bonus_sizes)
        )
        asset_specs.extend([
            ("BONUSBG", 512 * 20, 512, 20, 512, 1),
            ("BONUSPAL", 1024, 1024, 1, 1024, 2),
            ("BONUSTILES", 32 * 1920, 32, 1920, 32, 1),
        ])
        expect("Complete cache fixture has every required entry", len(asset_specs) == 1010)
        expect(
            "Authoritative cache manifest CRC",
            manifest_crc(asset_specs) == CACHE_MANIFEST_CRC,
        )
        operations = [(asset[0], asset[5]) for asset in asset_specs]
        committed = model_transaction_build(asset_specs, operations)
        for checkpoint_count, checkpoint_last in CACHE_CHECKPOINTS:
            model_cache_checkpoint(
                asset_specs,
                committed[:checkpoint_count],
                checkpoint_count,
                checkpoint_count,
                checkpoint_last,
            )
            expected_name = asset_specs[checkpoint_count - 1][0]
            missing = committed[:checkpoint_count - 1]
            expect_error_contains(
                f"Missing transaction is named at checkpoint {checkpoint_count}",
                lambda missing=missing, checkpoint_count=checkpoint_count,
                       checkpoint_last=checkpoint_last:
                    model_cache_checkpoint(
                        asset_specs, missing, len(missing),
                        checkpoint_count, checkpoint_last
                    ),
                expected_name,
            )
            duplicated = operations[:checkpoint_count - 1] + [
                operations[checkpoint_count - 2]
            ]
            expect_error_contains(
                f"Duplicate transaction is rejected at phase {checkpoint_count}",
                lambda duplicated=duplicated:
                    model_transaction_build(asset_specs, duplicated),
                expected_name,
            )
            reordered = list(operations[:checkpoint_count])
            reordered[-2], reordered[-1] = reordered[-1], reordered[-2]
            expect_error_contains(
                f"Reordered transaction is rejected at phase {checkpoint_count}",
                lambda reordered=reordered:
                    model_transaction_build(asset_specs, reordered),
                asset_specs[checkpoint_count - 2][0],
            )
            wrong_type = list(operations[:checkpoint_count])
            wrong_type[-1] = (wrong_type[-1][0], 3 - wrong_type[-1][1])
            expect_error_contains(
                f"Wrong transaction type is rejected at phase {checkpoint_count}",
                lambda wrong_type=wrong_type:
                    model_transaction_build(asset_specs, wrong_type),
                expected_name,
            )
        recovered = model_cache_seal(
            asset_specs,
            committed[:-1],
            CACHE_REQUIRED_ENTRIES - 1,
            ("BONUSTILES", 1, 32 * 1920, 32 * 1920, False),
        )
        expect(
            "A complete pending final BONUSTILES transaction is recovered",
            recovered == committed,
        )
        expect_rejected(
            "A partial final transaction is rejected",
            lambda: model_cache_seal(
                asset_specs,
                committed[:-1],
                CACHE_REQUIRED_ENTRIES - 1,
                ("BONUSTILES", 1, 4096, 32 * 1920, False),
            ),
        )
        expect_rejected(
            "A failed final transaction is rejected",
            lambda: model_cache_seal(
                asset_specs,
                committed[:-1],
                CACHE_REQUIRED_ENTRIES - 1,
                ("BONUSTILES", 1, 32 * 1920, 32 * 1920, True),
            ),
        )
        expect_rejected(
            "A stale committed counter is rejected",
            lambda: model_cache_seal(
                asset_specs, committed, CACHE_REQUIRED_ENTRIES - 1
            ),
        )
        corrupt_slots = list(committed)
        corrupt_slots[492] = None
        expect_rejected(
            "A corrupted SRAM table slot is rejected",
            lambda: model_cache_seal(
                asset_specs, corrupt_slots, CACHE_REQUIRED_ENTRIES
            ),
        )
        model_checked_promotion("FONTMN2.0FN", True)
        expect_rejected(
            "A failed cache promotion cannot continue with an SRAM surface",
            lambda: model_checked_promotion("FONTMN2.0FN", False),
        )
        table_workspace = align_up(
            CACHE_REQUIRED_ENTRIES * CACHE_ENTRY_SIZE, CACHE_WRITE_SIZE
        )
        expect(
            "Cache table, 32 KiB batch, and decode scratch fit the level arena",
            table_workspace + CACHE_FLASH_BUFFER_SIZE + CACHE_DECODE_SIZE
            <= CACHE_LEVEL_ARENA_SIZE,
        )
        expect(
            "Menu and level font atlases fit the reused flash staging buffer",
            max(128 * 128, 160 * 160) <= CACHE_FLASH_BUFFER_SIZE,
        )
        writes, failed_asset = model_batched_cache_writes(asset_specs)
        expect("Batched cache model completes", failed_asset is None)
        expect("Cache model erases exactly once", writes[0] == ("erase", 0, CACHE_FINAL_SIZE))
        expect(
            "Payload writes are aligned bounded batches",
            all(
                kind != "payload"
                or (
                    offset % CACHE_WRITE_SIZE == 0
                    and size % CACHE_WRITE_SIZE == 0
                    and size <= CACHE_FLASH_BUFFER_SIZE
                )
                for kind, offset, size in writes
            ),
        )
        expect(
            "Cache table and header are the final writes",
            [kind for kind, _offset, _size in writes[-2:]] == ["table", "header"],
        )
        _failed_writes, failed_asset = model_batched_cache_writes(
            asset_specs, "BONUSSPR.013"
        )
        expect(
            "Injected BONUSSPR.013 flash failure is reported instead of hanging",
            failed_asset == "BONUSSPR.013",
        )
        cache = build_cache_fixture(asset_specs, fingerprint, len(raw))
        cache_names = verify_cache_fixture(cache, fingerprint, len(raw))
        expect(
            "Cache contains every required shareware asset",
            cache_names == [asset[0] for asset in asset_specs],
        )
        expect(
            "A valid cache can be reused on subsequent launches",
            verify_cache_fixture(cache, fingerprint, len(raw)) == cache_names,
        )
        expect("Cache remains inside shared staging region", len(cache) <= CACHE_SIZE_MAX)
        interrupted_header = bytearray(cache)
        interrupted_header[:CACHE_HEADER_SIZE] = b"\xff" * CACHE_HEADER_SIZE
        expect_rejected(
            "An interrupted header-last cache commit is rejected",
            lambda: verify_cache_fixture(
                bytes(interrupted_header), fingerprint, len(raw)
            ),
        )

        missing_cache = bytearray(cache)
        struct.pack_into("<I", missing_cache, 28, CACHE_REQUIRED_ENTRIES - 1)
        refresh_cache_table_crc(missing_cache)
        expect_rejected(
            "Missing manifest entries are rejected",
            lambda: verify_cache_fixture(
                bytes(missing_cache), fingerprint, len(raw)
            ),
        )

        duplicated_cache = bytearray(cache)
        duplicated_cache[
            CACHE_HEADER_SIZE + CACHE_ENTRY_SIZE:
            CACHE_HEADER_SIZE + 2 * CACHE_ENTRY_SIZE
        ] = duplicated_cache[
            CACHE_HEADER_SIZE:CACHE_HEADER_SIZE + CACHE_ENTRY_SIZE
        ]
        refresh_cache_table_crc(duplicated_cache)
        expect_rejected(
            "Duplicated manifest entries are rejected",
            lambda: verify_cache_fixture(
                bytes(duplicated_cache), fingerprint, len(raw)
            ),
        )

        reordered_cache = bytearray(cache)
        first_entry = bytes(reordered_cache[
            CACHE_HEADER_SIZE:CACHE_HEADER_SIZE + CACHE_ENTRY_SIZE
        ])
        second_entry = bytes(reordered_cache[
            CACHE_HEADER_SIZE + CACHE_ENTRY_SIZE:
            CACHE_HEADER_SIZE + 2 * CACHE_ENTRY_SIZE
        ])
        reordered_cache[
            CACHE_HEADER_SIZE:CACHE_HEADER_SIZE + CACHE_ENTRY_SIZE
        ] = second_entry
        reordered_cache[
            CACHE_HEADER_SIZE + CACHE_ENTRY_SIZE:
            CACHE_HEADER_SIZE + 2 * CACHE_ENTRY_SIZE
        ] = first_entry
        refresh_cache_table_crc(reordered_cache)
        expect_rejected(
            "Reordered manifest entries are rejected",
            lambda: verify_cache_fixture(
                bytes(reordered_cache), fingerprint, len(raw)
            ),
        )

        renamed_cache = bytearray(cache)
        renamed_cache[CACHE_HEADER_SIZE] ^= 1
        refresh_cache_table_crc(renamed_cache)
        expect_rejected(
            "Renamed manifest entries are rejected",
            lambda: verify_cache_fixture(
                bytes(renamed_cache), fingerprint, len(raw)
            ),
        )

        wrong_type_cache = bytearray(cache)
        struct.pack_into("<H", wrong_type_cache, CACHE_HEADER_SIZE + 36, 2)
        refresh_cache_table_crc(wrong_type_cache)
        expect_rejected(
            "Wrong manifest entry types are rejected",
            lambda: verify_cache_fixture(
                bytes(wrong_type_cache), fingerprint, len(raw)
            ),
        )

        blank_font_cache = bytearray(cache)
        font_index = cache_names.index("FONTMN2.0FN")
        font_entry_offset = CACHE_HEADER_SIZE + font_index * CACHE_ENTRY_SIZE
        font_entry = list(struct.unpack_from(
            CACHE_ENTRY_FMT, blank_font_cache, font_entry_offset
        ))
        font_payload_offset = font_entry[1]
        font_payload_size = font_entry[2]
        blank_font_cache[
            font_payload_offset:font_payload_offset + font_payload_size
        ] = bytes(font_payload_size)
        font_entry[3] = binascii.crc32(bytes(font_payload_size)) & 0xFFFFFFFF
        struct.pack_into(
            CACHE_ENTRY_FMT, blank_font_cache, font_entry_offset, *font_entry
        )
        refresh_cache_table_crc(blank_font_cache)
        expect_rejected(
            "CRC-correct blank cached fonts are rejected semantically",
            lambda: verify_cache_fixture(
                bytes(blank_font_cache), fingerprint, len(raw)
            ),
        )

        schema_v4_cache = build_cache_fixture(
            asset_specs, fingerprint, len(raw), version=4
        )
        expect_rejected(
            "Schema-v4 caches are rejected for one automatic rebuild",
            lambda: verify_cache_fixture(
                schema_v4_cache, fingerprint, len(raw)
            ),
        )
        valid_font = bytes([0] * (128 * 128 - FONT_MIN_PIXELS) +
                           [247] * FONT_MIN_PIXELS)
        validate_font_cache_entry("FONTMN2.0FN", valid_font)
        expect_rejected(
            "Blank menu-font atlases are rejected",
            lambda: validate_font_cache_entry(
                "FONTMN2.0FN", bytes(128 * 128)
            ),
        )
        expect_rejected(
            "Truncated menu-font atlases are rejected",
            lambda: validate_font_cache_entry(
                "FONTMN2.0FN", valid_font[:-1]
            ),
        )
        expect_rejected(
            "Incorrectly sized menu-font atlases are rejected",
            lambda: validate_font_cache_entry(
                "FONTMN2.0FN", valid_font, width=127
            ),
        )
        expect_rejected(
            "Wrong-pitch menu-font atlases are rejected",
            lambda: validate_font_cache_entry(
                "FONTMN2.0FN", valid_font, pitch=129
            ),
        )
        expect_rejected(
            "Wrong-color-key menu-font atlases are rejected",
            lambda: validate_font_cache_entry(
                "FONTMN2.0FN", valid_font, color_key=254
            ),
        )
        expect_rejected(
            "Disabled color keys on menu-font atlases are rejected",
            lambda: validate_font_cache_entry(
                "FONTMN2.0FN", valid_font, flags=0
            ),
        )
        expect_rejected(
            "Truncated cache is rejected",
            lambda: verify_cache_fixture(cache[:-1], fingerprint, len(raw)),
        )
        stale_cache = bytearray(cache)
        struct.pack_into("<I", stale_cache, 20, fingerprint ^ 1)
        expect_rejected(
            "Stale source fingerprint is rejected",
            lambda: verify_cache_fixture(bytes(stale_cache), fingerprint, len(raw)),
        )
        corrupt_cache_table = bytearray(cache)
        corrupt_cache_table[CACHE_HEADER_SIZE] ^= 0x20
        expect_rejected(
            "Cache table CRC corruption is rejected",
            lambda: verify_cache_fixture(bytes(corrupt_cache_table), fingerprint, len(raw)),
        )
        corrupt_cache_payload = bytearray(cache)
        first_cache_offset = struct.unpack_from(
            CACHE_ENTRY_FMT, cache, CACHE_HEADER_SIZE
        )[1]
        corrupt_cache_payload[first_cache_offset] ^= 0x80
        expect(
            "Fast transactional validation does not reread payload bytes",
            verify_cache_fixture(
                bytes(corrupt_cache_payload), fingerprint, len(raw)
            ) == cache_names,
        )
        out_of_bounds_cache = bytearray(cache)
        struct.pack_into(
            "<I", out_of_bounds_cache, CACHE_HEADER_SIZE + 16,
            CACHE_FINAL_SIZE,
        )
        refresh_cache_table_crc(out_of_bounds_cache)
        expect_rejected(
            "Out-of-bounds cache entries are rejected",
            lambda: verify_cache_fixture(
                bytes(out_of_bounds_cache), fingerprint, len(raw)
            ),
        )
        oversized_cache = bytearray(cache)
        struct.pack_into("<I", oversized_cache, 32, CACHE_SIZE_MAX + CACHE_ERASE_SIZE)
        expect_rejected(
            "Oversized cache is rejected",
            lambda: verify_cache_fixture(bytes(oversized_cache), fingerprint, len(raw)),
        )

        corrupted_payload = bytearray(raw)
        _name_len, _reserved, offset, size, _crc = struct.unpack_from(builder.ENTRY_FMT, raw, builder.HEADER_SIZE)
        expect("Fixture has a non-empty first payload", size > 0)
        corrupted_payload[offset] ^= 0x80
        expect_rejected(
            "Payload CRC corruption is rejected",
            lambda: builder.verify_pack(bytes(corrupted_payload), names),
        )

        corrupted_table = bytearray(raw)
        struct.pack_into("<I", corrupted_table, builder.HEADER_SIZE + 4, len(raw) + 4)
        expect_rejected(
            "Out-of-range pack offsets are rejected",
            lambda: builder.verify_pack(bytes(corrupted_table), names),
        )

        missing = tmp / "missing"
        missing.mkdir()
        (missing / "PANEL.000").write_bytes(b"x")
        expect_rejected("Incomplete data roots are rejected", lambda: builder.collect_entries(missing))

        interactive_out = tmp / "interactive.pak"
        answers = iter((str(shareware), str(interactive_out)))
        old_input = builtins.input
        old_cwd = Path.cwd()
        old_interactive = builder.interactive
        try:
            os.chdir(tmp)
            builtins.input = lambda _prompt: next(answers)
            builder.interactive = lambda: True
            source, output = builder.resolve_inputs(
                argparse.Namespace(source=None, input_path=None, output=None)
            )
        finally:
            builder.interactive = old_interactive
            builtins.input = old_input
            os.chdir(old_cwd)
        expect("No-argument interactive source prompt resolves", source == shareware.resolve())
        expect("No-argument interactive output prompt resolves", output == interactive_out.resolve())

    print("OpenJazz pack tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
