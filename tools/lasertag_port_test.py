#!/usr/bin/env python3
"""Regression checks for the DC32 OpenLASIR Lab."""

from __future__ import annotations

import base64
import binascii
import hashlib
import struct
import tempfile
from pathlib import Path

from lasertag_protocol import COLOR_NAMES, MODE_NAMES, classify_raw, parse_flipper_ir
from lasertag_sync import decode_sync_document, decode_sync_text, inspect_qr_text


ROOT = Path(__file__).resolve().parents[1]
DEFCON33_CAPTURE = Path(r"C:\Users\mrnic\Downloads\Defcon33LaserTag.ir")
BLOCK32_CAPTURE = Path(r"C:\Users\mrnic\Downloads\block32_lasertag.ir")
DC33_RAW = (0x25DA0BF4, 0x24DB0BF4, 0x23DC0BF4, 0x22DD0BF4)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def encode(block: int, device: int, mode: int, data: int) -> int:
    return block | ((block ^ 0xFF) << 8) | (device << 16) | ((mode & 0x1F) << 24) | ((data & 7) << 29)


def raw_record(name: str, raw: int) -> str:
    values = [8100, 4050]
    for bit in range(32):
        values.extend((507, 1518 if raw & (1 << bit) else 507))
    return f"name: {name}\ntype: raw\nfrequency: 38000\nduty_cycle: 0.330000\ndata: {' '.join(map(str, values))}\n"


def fallback_captures(directory: Path) -> tuple[Path, Path]:
    defcon = directory / "Defcon33LaserTag.ir"
    block32 = directory / "block32_lasertag.ir"
    defcon.write_text("Filetype: IR signals file\nVersion: 1\n#\n" + "#\n".join(
        raw_record(name, raw) for name, raw in zip(("L_TAG_1", "L_TAG_2", "L_TAG_3", "L_TAG_3"), DC33_RAW)
    ), encoding="utf-8")
    records = []
    for mode, prefix in ((0, "FIRE"), (9, "BLINK")):
        for color, color_name in enumerate(COLOR_NAMES):
            command_high = mode | (color << 5)
            records.append(
                f"name: {prefix}_{color_name.upper()}\ntype: parsed\nprotocol: NECext\n"
                f"address: 01 FE 00 00\ncommand: 03 {command_high:02X} 00 00\n"
            )
    block32.write_text("Filetype: IR signals file\nVersion: 1\n#\n" + "#\n".join(records), encoding="utf-8")
    return defcon, block32


def check_captures() -> None:
    if DEFCON33_CAPTURE.is_file() and BLOCK32_CAPTURE.is_file():
        defcon, block32 = DEFCON33_CAPTURE, BLOCK32_CAPTURE
        defcon_records = parse_flipper_ir(defcon)
        block_records = parse_flipper_ir(block32)
    else:
        with tempfile.TemporaryDirectory() as temporary:
            defcon, block32 = fallback_captures(Path(temporary))
            defcon_records = parse_flipper_ir(defcon)
            block_records = parse_flipper_ir(block32)
    require([int(record["raw"], 16) for record in defcon_records] == list(DC33_RAW),
            "Defcon33 legacy vectors changed")
    require(all(record["kind"] == "standard_nec" for record in defcon_records),
            "Defcon33 vectors must remain standard NEC")
    require(len(block_records) == 16 and all(record["kind"] == "openlasir" for record in block_records),
            "block32 must contain all OpenLASIR vectors")
    for index, record in enumerate(block_records):
        require(record["block_id"] == 1 and record["device_id"] == 3,
                "Mj0ln1r identity changed")
        expected_mode = 0 if index < 8 else 9
        require(record["mode"] == expected_mode and record["data"] == index & 7,
                "Mj0ln1r mode/color vector changed")


def check_protocol_model() -> None:
    for mode in range(12):
        raw = encode(1, 3, mode, mode & 7)
        decoded = classify_raw(raw)
        require(decoded["kind"] == "openlasir" and decoded["mode_name"] == MODE_NAMES[mode],
                f"OpenLASIR mode {mode} did not decode")
    require(classify_raw(DC33_RAW[0])["kind"] == "standard_nec", "legacy NEC was not recognized")
    malformed = encode(1, 3, 0, 0) ^ (1 << 8)
    require(classify_raw(malformed)["kind"] == "invalid", "bad block inverse was accepted")
    require(encode(0, 42, 0, 4) == 0x802AFF00, "OpenLASIR reference vector changed")


def diagnostic_sync_text(page: int, first: int, count: int) -> str:
    raw = bytearray(28)
    raw[:4] = b"DCLT"
    raw[4:12] = bytes((3, 0x80, 0, 42, 1, count, page, 3))
    struct.pack_into("<III", raw, 12, 0x12345678, 12, 34)
    struct.pack_into("<I", raw, 24, 7200)
    for index in range(first, first + count):
        raw.extend(struct.pack("<IHBB", encode(1, 3, index % 12, index & 7), index, index & 3, 0))
    raw.extend(struct.pack("<I", binascii.crc32(raw) & 0xFFFFFFFF))
    return "DC32LT2." + base64.urlsafe_b64encode(raw).decode("ascii").rstrip("=")


def check_sync_decoder() -> None:
    pages = [diagnostic_sync_text(0, 0, 13), diagnostic_sync_text(1, 13, 13), diagnostic_sync_text(2, 26, 6)]
    decoded = decode_sync_text(pages[0])
    require(decoded["schema"] == 3 and decoded["export_kind"] == "dc32_local_unsigned_diagnostic",
            "schema-3 local diagnostic export was not decoded")
    require(decoded["events"][0]["direction"] == "rx" and decoded["events"][2]["protocol"] == "openlasir",
            "diagnostic event metadata changed")
    merged = decode_sync_document("\n".join(pages))
    require(merged["complete"] and len(merged["events"]) == 32 and merged["pages_decoded"] == 3,
            "paged diagnostic export was not merged")
    legacy = bytearray(28)
    legacy[:4] = b"DCLT"
    legacy[4:12] = bytes((2, 0x8B, 0, 42, 1, 0, 0, 1))
    struct.pack_into("<III", legacy, 12, 0x12345678, 9, 2)
    struct.pack_into("<I", legacy, 24, 3)
    legacy.extend(struct.pack("<I", binascii.crc32(legacy) & 0xFFFFFFFF))
    legacy_text = "DC32LT1." + base64.urlsafe_b64encode(legacy).decode("ascii").rstrip("=")
    require(decode_sync_text(legacy_text)["schema"] == 2, "legacy DC32LT1 export no longer decodes")
    unknown = inspect_qr_text("https://official.example/sync?opaque=abc")
    require(not unknown["recognized"] and unknown["raw_text"].startswith("https://"),
            "forensic QR inspection must retain unknown raw text")


def check_wiring_and_contracts() -> None:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    header = (ROOT / "src/irRemote.h").read_text(encoding="utf-8")
    ir = (ROOT / "src/irRemote.c").read_text(encoding="utf-8")
    app = (ROOT / "src/apps/lasertag/lasertag.c").read_text(encoding="utf-8")
    package = (ROOT / "tools/build_sd_zip.py").read_text(encoding="utf-8")
    ui = (ROOT / "src/ui.c").read_text(encoding="utf-8")
    require("add_dcapp(dcapp_lasertag lasertag 106" in cmake, "Laser Tag target missing")
    require('"Universal Remote"' in ui and '"Laser Tag"' in ui, "Infrared menu label/wiring missing")
    for token in ("struct IrRemoteNecFrame", "IrRemoteNecOpenLasir", "IrRemoteNecStandard",
                  "irRemoteNecPoll", "irRemoteNecSend", "irRemoteOpenLasirSendFrame"):
        require(token in header and token in ir, f"generic NEC API missing {token}")
    for token in ("OPENLASIR LAB", "MANUAL TRANSMIT", "PACKET HISTORY", "LOCAL DIAGNOSTIC EXPORT",
                  "Mj0ln1r fire", "DC33 legacy", "mLtDc33Raw", "LT_EVENTS 32u", "LT_EVENT_TX",
                  "LT_EVENT_OPENLASIR", "LT_SYNC_PREFIX \"DC32LT2.\"", "UNSIGNED DC32 EXPORT - NOT OFFICIAL SYNC",
                  "irRemoteNecPoll", "irRemoteNecSend", "ltModeName", "CONFIRM MANUAL SEND", "legacyVector"):
        require(token in app, f"diagnostic app contract missing {token}")
    require("OpenLASIR Lab" in package and "Mj0ln1r" in package, "package metadata was not updated")
    artifact = ROOT / "build/apps/lasertag.DC32"
    if artifact.exists():
        data = artifact.read_bytes()
        require(len(data) < 256 * 1024 and struct.unpack_from("<I", data)[0] == 0x50414344,
                "Laser Tag app artifact is invalid")


def main() -> None:
    check_captures()
    check_protocol_model()
    check_sync_decoder()
    check_wiring_and_contracts()
    print("Laser Tag Lab checks passed")


if __name__ == "__main__":
    main()
