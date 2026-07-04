#!/usr/bin/env python3
"""Build the offline DC32 asset pack from pinned YSoccer 19 sources."""

from __future__ import annotations

import argparse
import binascii
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

sys.dont_write_bytecode = True

try:
    from PIL import Image
except ImportError as exc:  # pragma: no cover - build environment failure
    raise SystemExit("Pillow is required to build the YSoccer asset pack") from exc


UPSTREAM_COMMIT = "689d8b5cb162e5f270a4d6df44b5eeb82b6170d7"
MAGIC = b"DC32YSOCV5\0\0"
VERSION = 5
HEADER_FMT = "<12sII20sIIIIII"
TEAM_FMT = "<32s16sHBBHHHHI4s"
PLAYER_FMT = "<20s12B"
ASSET_FMT = "<24sIIIIHH"
AUDIO_HEADER_FMT = "<8sI"
AUDIO_ENTRY_FMT = "<8sIIIhBB"
PLAYERS_PER_TEAM = 16
RETAINED_KIT_STYLES = ("PLAIN", "VERTICAL", "HORIZONTAL", "BIG_CHECK")

# YSoccer 19's shipped database contains almost entirely Italian 1964-65 clubs.
# This compact badge roster presents recognizable national teams while retaining
# source-derived player records and the upstream recolorable kit artwork.
NATIONAL_TEAMS = (
    ("ARGENTINA", "ARG", "VERTICAL", "#75aadb", "#ffffff", "#202b4f", "#ffffff", 0),
    ("BRAZIL", "BRA", "PLAIN", "#f7df1e", "#168b3f", "#2055a5", "#ffffff", 3),
    ("ENGLAND", "ENG", "PLAIN", "#ffffff", "#d71920", "#1b2b55", "#ffffff", 4),
    ("FRANCE", "FRA", "PLAIN", "#2055a5", "#ffffff", "#ffffff", "#d71920", 7),
    ("GERMANY", "GER", "PLAIN", "#ffffff", "#202020", "#202020", "#ffffff", 9),
    ("ITALY", "ITA", "PLAIN", "#1768b0", "#ffffff", "#1768b0", "#1768b0", 51),
    ("NETHERLANDS", "NED", "PLAIN", "#f36c21", "#202020", "#ffffff", "#f36c21", 16),
    ("SPAIN", "ESP", "PLAIN", "#d71920", "#f4cf18", "#263d78", "#d71920", 17),
    ("PORTUGAL", "POR", "VERTICAL", "#d71920", "#168b3f", "#168b3f", "#d71920", 18),
    ("BELGIUM", "BEL", "PLAIN", "#d71920", "#202020", "#202020", "#d71920", 19),
    ("CROATIA", "CRO", "BIG_CHECK", "#d71920", "#ffffff", "#ffffff", "#2055a5", 26),
    ("DENMARK", "DEN", "PLAIN", "#d71920", "#ffffff", "#ffffff", "#d71920", 29),
    ("SWEDEN", "SWE", "PLAIN", "#f4cf18", "#2055a5", "#2055a5", "#f4cf18", 37),
    ("NORWAY", "NOR", "PLAIN", "#d71920", "#2055a5", "#ffffff", "#2055a5", 41),
    ("IRELAND", "IRL", "PLAIN", "#168b3f", "#ffffff", "#ffffff", "#168b3f", 44),
    ("SCOTLAND", "SCO", "PLAIN", "#1b2b55", "#ffffff", "#1b2b55", "#1b2b55", 49),
    ("WALES", "WAL", "PLAIN", "#d71920", "#ffffff", "#d71920", "#d71920", 50),
    ("USA", "USA", "HORIZONTAL", "#ffffff", "#d71920", "#2055a5", "#ffffff", 5),
    ("MEXICO", "MEX", "VERTICAL", "#168b3f", "#ffffff", "#d71920", "#d71920", 10),
    ("JAPAN", "JPN", "PLAIN", "#2055a5", "#ffffff", "#ffffff", "#2055a5", 15),
    ("SOUTH KOREA", "KOR", "PLAIN", "#d71920", "#2055a5", "#202020", "#ffffff", 20),
    ("AUSTRALIA", "AUS", "PLAIN", "#f4cf18", "#168b3f", "#168b3f", "#f4cf18", 35),
    ("CAMEROON", "CMR", "VERTICAL", "#168b3f", "#d71920", "#f4cf18", "#168b3f", 47),
    ("NIGERIA", "NGA", "VERTICAL", "#168b3f", "#ffffff", "#168b3f", "#ffffff", 14),
)
PINNED_ASSET_SHA256 = {
    "images/player/PLAIN.png": "49d79366f621d5dd1dee78cc7d87f52a25f0bd11e5bc811b346103e6ae8106ab",
    "images/stadium/generic_00.png": "4ff9203bf8034a0107f82e991117712e04bfb434a551421276764dc21fb1dbc8",
    "sounds/kick.ogg": "9962149e7bf7754509c5d87c1202dc61d1e4f02f37ba02f3f96626be06d246bb",
    "data/teams/1964-65/CLUB_TEAMS/EUROPE/ITALY/team.ac_milan.json":
        "8b5e1bfd0f9c1866de0d53996c5f83d1b7d4d5b1760065748dd3083f6e1feeb1",
}

ROLE_COLORS = {
    0xE20020: 1, 0xBF001B: 2, 0x9C0016: 3, 0x790011: 4,
    0x01FFC6: 5, 0x00C79B: 6, 0x008B6C: 7, 0x006A52: 8,
    0xCC00FF: 9, 0x8600A7: 10, 0x610079: 11, 0x420052: 12,
    0xF6FF00: 13, 0xCDD400: 14, 0xA3A900: 15, 0x7A7E00: 16,
    0x0097EE: 17, 0x0088D6: 18, 0x0079BF: 19, 0x006AA7: 20,
    0xFF6300: 21, 0xB54200: 22, 0x631800: 23,
    0x907130: 24, 0x715930: 25, 0x514030: 26,
}

ROLE_IDS = {
    "GOALKEEPER": 0, "RIGHT_BACK": 1, "LEFT_BACK": 2, "DEFENDER": 3,
    "RIGHT_WINGER": 4, "LEFT_WINGER": 5, "MIDFIELDER": 6, "ATTACKER": 7,
}
SKIN_IDS = {name: index for index, name in enumerate(
    ("PINK", "BLACK", "PALE", "ASIATIC", "ARAB", "MULATTO", "RED", "ALIEN", "YODA"))}
HAIR_IDS = {name: index for index, name in enumerate(
    ("BLACK", "BLOND", "BROWN", "RED", "PLATINUM", "GRAY", "WHITE", "PUNK_FUCHSIA", "PUNK_BLOND"))}
HAIR_STYLES = (
    "BALD_A", "BALD_B", "CURLY_A", "CURLY_B", "CURLY_C", "CURLY_D", "CURLY_E", "CURLY_F",
    "PIGTAIL_A", "PIGTAIL_B", "PIGTAIL_C", "SHAVED", "SHORT_A", "SHORT_B", "SHORT_C",
    "SMOOTH_A", "SMOOTH_B", "SMOOTH_C", "SMOOTH_D", "SMOOTH_E", "SMOOTH_F", "SMOOTH_G",
    "SMOOTH_H", "SMOOTH_I", "SMOOTH_J", "SMOOTH_K", "SMOOTH_L", "SMOOTH_M", "SMOOTH_N",
    "SMOOTH_O", "SMOOTH_P", "SPECIAL_A", "SPECIAL_B", "SPECIAL_C", "SPECIAL_D", "SPECIAL_E",
    "SPECIAL_F", "SPECIAL_G", "SPECIAL_H", "THINNING_A", "THINNING_B", "THINNING_C",
)
HAIR_STYLE_IDS = {name: index for index, name in enumerate(HAIR_STYLES)}
IMA_STEPS = (
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544,
    598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707,
    1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871,
    5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635,
    13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
)
IMA_INDEX = (-1, -1, -1, -1, 2, 4, 6, 8)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True,
                        help="vendored java/android/assets directory")
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def pinned_asset_digest(path: Path) -> str:
    data = path.read_bytes()
    if path.suffix.casefold() == ".json":
        data = data.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    return hashlib.sha256(data).hexdigest()


def validate_source(source: Path) -> None:
    for relative, expected in PINNED_ASSET_SHA256.items():
        path = source / relative
        if not path.exists():
            raise ValueError(f"missing pinned YSoccer asset: {relative}")
        digest = pinned_asset_digest(path)
        if digest != expected:
            raise ValueError(f"pinned YSoccer asset changed: {relative} ({digest})")


def rgb565(color: str | int) -> int:
    value = int(color[1:], 16) if isinstance(color, str) else color
    return ((value >> 8) & 0xF800) | ((value >> 5) & 0x07E0) | ((value >> 3) & 0x001F)


def fixed_text(value: str, size: int) -> bytes:
    raw = value.encode("utf-8", errors="replace")[: size - 1]
    return raw + bytes(size - len(raw))


def country_code(value: str) -> int:
    return binascii.crc32(value.encode("ascii", errors="ignore")) & 0xFFFF


def load_teams(source: Path) -> tuple[bytes, bytes, int, set[int]]:
    records = bytearray()
    players = bytearray()
    used_hair_styles: set[int] = {0}  # Custom-team players use BALD_A.
    paths = sorted((source / "data" / "teams").rglob("team.*.json"),
                   key=lambda path: path.as_posix().casefold())
    if len(paths) != 68:
        raise ValueError(f"expected 68 complete upstream teams, found {len(paths)}")
    for country_index, spec in enumerate(NATIONAL_TEAMS):
        name, code, style, shirt1, shirt2, shorts, socks, original_index = spec
        path = paths[original_index]
        team = json.loads(path.read_text(encoding="utf-8"))
        roster = list(team.get("players") or [])[:PLAYERS_PER_TEAM]
        seed = binascii.crc32((name + ":" + path.relative_to(source).as_posix()).encode("utf-8")) & 0xFFFFFFFF
        records += struct.pack(
            TEAM_FMT,
            fixed_text(name, 32),
            fixed_text(style, 16),
            country_code(code),
            0,
            len(roster),
            rgb565(shirt1), rgb565(shirt2), rgb565(shorts), rgb565(socks),
            seed,
            struct.pack("<HBB", original_index, country_index, 0),
        )
        for index in range(PLAYERS_PER_TEAM):
            player = roster[index] if index < len(roster) else {}
            skills = player.get("skills") or {}
            rating = max(1, min(9, int(player.get("value", 25)) // 5 or 5))
            hair_style = HAIR_STYLE_IDS.get(str(player.get("hairStyle", "SMOOTH_A")), 0)
            used_hair_styles.add(hair_style)
            values = (
                int(player.get("number", index + 1)) & 0xFF,
                ROLE_IDS.get(str(player.get("role", "MIDFIELDER")), 6),
                SKIN_IDS.get(str(player.get("skinColor", "PINK")), 0),
                HAIR_IDS.get(str(player.get("hairColor", "BLACK")), 0),
                hair_style,
                int(skills.get("passing", rating)) & 0xFF,
                int(skills.get("shooting", rating)) & 0xFF,
                int(skills.get("heading", rating)) & 0xFF,
                int(skills.get("tackling", rating)) & 0xFF,
                int(skills.get("control", rating)) & 0xFF,
                int(skills.get("speed", rating)) & 0xFF,
                int(skills.get("finishing", rating)) & 0xFF,
            )
            players += struct.pack(PLAYER_FMT, fixed_text(str(player.get("shirtName") or
                player.get("name") or f"PLAYER {index + 1}"), 20), *values)
    retained = len(records) // struct.calcsize(TEAM_FMT)
    if retained != 24:
        raise ValueError(f"expected 24 national teams, found {retained}")
    return bytes(records), bytes(players), retained, used_hair_styles


def load_rgba_image(path: Path, palette_path: Path | None = None) -> Image.Image:
    source = Image.open(path)
    if palette_path is not None and source.mode == "P":
        lines = palette_path.read_text(encoding="ascii").splitlines()[3:259]
        colors = [tuple(map(int, line.split()[:3])) + (255,) for line in lines]
        transparency = source.info.get("transparency")
        pixels = []
        for index in source.getdata():
            color = colors[index] if index < len(colors) else (0, 0, 0, 255)
            pixels.append(color[:3] + ((0,) if transparency == index else (255,)))
        image = Image.new("RGBA", source.size)
        image.putdata(pixels)
        return image
    return source.convert("RGBA")


def encode_indexed_image(path: Path, palette_path: Path | None = None,
                         frame_layout: tuple[int, int, int, int, int, int] | None = None
                         ) -> tuple[bytes, int, int]:
    return encode_indexed_rgba(load_rgba_image(path, palette_path), str(path), frame_layout)


def encode_indexed_rgba(image: Image.Image, label: str,
                        frame_layout: tuple[int, int, int, int, int, int] | None = None
                        ) -> tuple[bytes, int, int]:
    palette: list[int] = [0]
    lookup: dict[int, int] = {}
    pixels = bytearray()
    for red, green, blue, alpha in image.getdata():
        if alpha < 128:
            pixels.append(0)
            continue
        color = (red << 16) | (green << 8) | blue
        value = 0x80000000 | ROLE_COLORS[color] if color in ROLE_COLORS else rgb565(color)
        index = lookup.get(value)
        if index is None:
            index = len(palette)
            if index > 255:
                raise ValueError(f"too many colors in {label}")
            lookup[value] = index
            palette.append(value)
        pixels.append(index)
    width = image.width
    height = image.height
    layout = 0
    if frame_layout is not None:
        frame_width, frame_height, columns, rows, stride_x, stride_y = frame_layout
        if (frame_width > 255 or frame_height > 255 or
                (columns - 1) * stride_x + frame_width > image.width or
                (rows - 1) * stride_y + frame_height > image.height):
            raise ValueError(f"invalid frame layout for {label}")
        packed = bytearray()
        for frame_y in range(rows):
            for frame_x in range(columns):
                for row in range(frame_height):
                    start = ((frame_y * stride_y + row) * image.width +
                             frame_x * stride_x)
                    packed += pixels[start:start + frame_width]
        pixels = packed
        width = frame_width * columns
        height = frame_height * rows
        layout = frame_width | (frame_height << 8)
    header = struct.pack("<HHHH", width, height, len(palette), layout)
    return header + struct.pack(f"<{len(palette)}I", *palette) + pixels, width, height


def image_assets(source: Path, hair_style_ids: set[int]) -> list[tuple[str, bytes, int, int]]:
    images = source / "images"
    assets: list[tuple[str, bytes, int, int]] = []
    for style in RETAINED_KIT_STYLES:
        payload, width, height = encode_indexed_image(
            images / "player" / f"{style}.png", frame_layout=(32, 32, 8, 16, 32, 32))
        assets.append((f"PLY_{style}", payload, width, height))
    for style in (HAIR_STYLES[index] for index in sorted(hair_style_ids)):
        # Upstream uses 20x20 regions on a 21-pixel grid. The final row and
        # column are editor separators and must not be rendered.
        payload, width, height = encode_indexed_image(
            images / "player" / "hairstyles" / f"{style}.png",
            frame_layout=(20, 20, 8, 10, 21, 21))
        assets.append((f"HAIR_{style}", payload, width, height))
    for name, relative in (
        ("KEEPER", "player/keeper.png"),
        ("SHADOW0", "player/shadows/player_0.png"),
        ("BALL", "ball.png"),
        ("BALL_SNOW", "ballsnow.png"),
        ("GOAL_TOP_A", "stadium/goal_top_a.png"),
        ("GOAL_BOTTOM", "stadium/goal_bottom.png"),
    ):
        frame_layout = None
        if name == "KEEPER":
            frame_layout = (50, 50, 8, 19, 50, 50)
        elif name.startswith("SHADOW"):
            frame_layout = (32, 32, 8, 16, 32, 32)
        elif name in ("BALL", "BALL_SNOW"):
            frame_layout = (8, 8, 5, 1, 8, 8)
        payload, width, height = encode_indexed_image(
            images / relative, frame_layout=frame_layout)
        assets.append((name, payload, width, height))
    stadium_palette = images / "stadium" / "palettes" / "normal_sunny.pal"
    stadium = Image.new("RGBA", (2048, 2048))
    for column in range(4):
        for row in range(4):
            tile = load_rgba_image(
                images / "stadium" / f"generic_{column}{row}.png", stadium_palette)
            stadium.paste(tile, (column * 512, row * 512))
    for name, box in (
        ("STAD_TOP", (241, 183, 1453, 279)),
        ("STAD_BOTTOM", (241, 1559, 1453, 1655)),
        ("STAD_LEFT", (241, 279, 337, 1559)),
        ("STAD_RIGHT", (1357, 279, 1453, 1559)),
    ):
        payload, width, height = encode_indexed_rgba(stadium.crop(box), name)
        assets.append((name, payload, width, height))
    for name, relative in (("PLAYER_ORIGINS", "player_origins.json"),
                           ("KEEPER_ORIGINS", "keeper_origins.json")):
        values = json.loads((source / "configs" / relative).read_text(encoding="utf-8"))
        flat = bytearray()
        for row in values:
            for pair in row:
                flat += struct.pack("<bb", max(-128, min(127, int(pair[0]))),
                                    max(-128, min(127, int(pair[1]))))
        assets.append((name, bytes(flat), len(values[0]), len(values)))
    values = json.loads((source / "configs" / "player_hair_map.json").read_text(encoding="utf-8"))
    flat = bytearray()
    for row in values:
        for item in row:
            flat += struct.pack("<bbbb", *(max(-128, min(127, int(value))) for value in item[:4]))
    assets.append(("PLAYER_HAIR_MAP", bytes(flat), len(values[0]), len(values)))
    return assets


def ima_encode(samples: list[int]) -> tuple[int, int, bytes]:
    predictor = samples[0] if samples else 0
    initial_predictor = predictor
    index = 0
    nibbles: list[int] = []
    for sample in samples[1:]:
        step = IMA_STEPS[index]
        diff = sample - predictor
        code = 8 if diff < 0 else 0
        diff = abs(diff)
        delta = step >> 3
        if diff >= step:
            code |= 4; diff -= step; delta += step
        if diff >= step >> 1:
            code |= 2; diff -= step >> 1; delta += step >> 1
        if diff >= step >> 2:
            code |= 1; delta += step >> 2
        predictor += -delta if code & 8 else delta
        predictor = max(-32768, min(32767, predictor))
        index = max(0, min(88, index + IMA_INDEX[code & 7]))
        nibbles.append(code)
    data = bytearray()
    for pos in range(0, len(nibbles), 2):
        data.append(nibbles[pos] | ((nibbles[pos + 1] if pos + 1 < len(nibbles) else 0) << 4))
    return initial_predictor, 0, bytes(data)


def read_audio(path: Path, sample_rate: int = 8000) -> list[int]:
    try:
        import soundfile
    except ImportError as exc:  # pragma: no cover - build environment failure
        raise SystemExit("python-soundfile is required to convert YSoccer OGG audio") from exc
    data, source_rate = soundfile.read(path, dtype="float32", always_2d=True)
    mono = data.mean(axis=1)
    count = max(1, round(len(mono) * sample_rate / source_rate))
    result: list[int] = []
    for index in range(count):
        source_pos = index * source_rate / sample_rate
        left = min(len(mono) - 1, int(source_pos))
        right = min(len(mono) - 1, left + 1)
        fraction = source_pos - left
        value = float(mono[left]) * (1.0 - fraction) + float(mono[right]) * fraction
        result.append(max(-32768, min(32767, round(value * 32767.0))))
    return result


def build_audio(source: Path, base_offset: int) -> bytes:
    clips = (
        ("MENU", source / "music" / "menu.ogg", 1),
        ("KICK", source / "sounds" / "kick.ogg", 0),
        ("TACKLE", source / "sounds" / "deflect.ogg", 0),
        ("WHISTLE", source / "sounds" / "whistle.ogg", 0),
        ("GOAL", source / "sounds" / "home_goal.ogg", 0),
        ("HALF", source / "sounds" / "end.ogg", 0),
        ("CARD", source / "sounds" / "whistle.ogg", 0),
        ("BOUNCE", source / "sounds" / "bounce.ogg", 0),
        ("CHANT", source / "sounds" / "chant.ogg", 0),
        ("CROWD", source / "sounds" / "crowd.ogg", 1),
        ("NET", source / "sounds" / "net.ogg", 0),
        ("POST", source / "sounds" / "post.ogg", 0),
        ("HOLD", source / "sounds" / "hold.ogg", 0),
    )
    encoded = []
    for name, path, flags in clips:
        predictor, step_index, payload = ima_encode(read_audio(path))
        encoded.append((name, predictor, step_index, flags, payload))
    header_size = struct.calcsize(AUDIO_HEADER_FMT) + len(encoded) * struct.calcsize(AUDIO_ENTRY_FMT)
    entries = bytearray()
    payloads = bytearray()
    for name, predictor, step_index, flags, payload in encoded:
        entries += struct.pack(AUDIO_ENTRY_FMT, fixed_text(name, 8),
                               base_offset + header_size + len(payloads), len(payload), 8000,
                               predictor, step_index, flags)
        payloads += payload
    return struct.pack(AUDIO_HEADER_FMT, b"YSAUDIO2", len(encoded)) + entries + payloads


def build_pack(source: Path) -> bytes:
    teams, players, team_count, hair_style_ids = load_teams(source)
    assets = image_assets(source, hair_style_ids)
    header_size = struct.calcsize(HEADER_FMT)
    directory_offset = header_size + len(teams) + len(players)
    directory_size = len(assets) * struct.calcsize(ASSET_FMT)
    payload_offset = directory_offset + directory_size
    payloads = bytearray()
    directory = bytearray()
    for name, payload, width, height in assets:
        offset = payload_offset + len(payloads)
        directory += struct.pack(ASSET_FMT, fixed_text(name, 24), offset, len(payload),
                                 len(payload), binascii.crc32(payload) & 0xFFFFFFFF,
                                 width, height)
        payloads += payload
    table_crc = binascii.crc32(teams + players) & 0xFFFFFFFF
    directory_crc = binascii.crc32(directory) & 0xFFFFFFFF
    header = struct.pack(HEADER_FMT, MAGIC, VERSION, team_count, bytes.fromhex(UPSTREAM_COMMIT),
                         struct.calcsize(TEAM_FMT), table_crc, header_size + len(teams),
                         directory_offset, len(assets), directory_crc)
    return header + teams + players + directory + payloads


def main() -> int:
    args = parse_args()
    validate_source(args.source)
    pack = build_pack(args.source)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(pack)
    digest = hashlib.sha256(pack).hexdigest()
    print(f"Wrote {args.output} ({len(pack)} bytes, 24 national teams, sha256={digest[:16]})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
