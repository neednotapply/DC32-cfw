#!/usr/bin/env python3
"""Decode and validate a DC32 Laser Tag QR/file sync payload."""

from __future__ import annotations

import argparse
import base64
import binascii
import hashlib
import json
import struct
from pathlib import Path


PREFIX_V1 = "DC32LT1."
PREFIX_V2 = "DC32LT2."
PREFIXES = (PREFIX_V2, PREFIX_V1)
ENTRY_SIZE = 8
SCHEMAS = (1, 2, 3)


def decode_sync_text(value: str) -> dict[str, object]:
    value = value.strip()
    matches = [(value.find(prefix), prefix) for prefix in PREFIXES if value.find(prefix) >= 0]
    if not matches:
        raise ValueError("sync payload does not contain a DC32LT1. or DC32LT2. prefix")
    marker, prefix = min(matches)
    encoded = value[marker + len(prefix):]
    encoded = encoded.split("#", 1)[0].split("&", 1)[0].strip()
    padding = "=" * ((4 - len(encoded) % 4) % 4)
    try:
        raw = base64.urlsafe_b64decode(encoded + padding)
    except (ValueError, binascii.Error) as exc:
        raise ValueError("sync payload is not valid base64url") from exc

    if len(raw) < 28 or raw[:4] != b"DCLT":
        raise ValueError("sync payload has an invalid header")
    schema = raw[4]
    if schema not in SCHEMAS:
        raise ValueError(f"unsupported sync schema {schema}")
    header_size = 28 if schema >= 2 else 24
    count = raw[9]
    page_index = raw[10]
    page_count = raw[11]
    if not page_count or page_index >= page_count:
        raise ValueError("sync payload has invalid page metadata")
    expected_size = header_size + count * ENTRY_SIZE + 4
    if len(raw) != expected_size:
        raise ValueError(f"sync payload length is {len(raw)}, expected {expected_size}")
    expected_crc = struct.unpack_from("<I", raw, len(raw) - 4)[0]
    actual_crc = binascii.crc32(raw[:-4]) & 0xFFFFFFFF
    if expected_crc != actual_crc:
        raise ValueError("sync payload CRC mismatch")

    entries = []
    for index in range(count):
        offset = header_size + index * ENTRY_SIZE
        if schema == 3:
            packet_raw, uptime_seconds, flags, _reserved = struct.unpack_from("<I H B B", raw, offset)
            entries.append({
                "raw": f"{packet_raw:08x}",
                "uptime_seconds": uptime_seconds,
                "direction": "tx" if flags & 1 else "rx",
                "protocol": "openlasir" if flags & 2 else "standard_nec",
            })
        else:
            block, device, color, _reserved, hits = struct.unpack_from("<4BI", raw, offset)
            entries.append({
                "block_id": block,
                "device_id": device,
                "color": color,
                "hits_received": hits,
            })

    flags = raw[5]
    result = {
        "schema": schema,
        "signed": not bool(flags & 0x80),
        "device_id_auto": bool(flags & 0x01),
        "block_id": raw[6],
        "device_id": raw[7],
        "color": raw[8],
        "page_index": page_index,
        "page_count": page_count,
        "badge_uid_hash": f"{struct.unpack_from('<I', raw, 12)[0]:08x}",
        "shots_fired": struct.unpack_from("<I", raw, 16)[0],
        "hits_received": struct.unpack_from("<I", raw, 20)[0],
        "payload_crc32": f"{expected_crc:08x}",
    }
    if schema == 3:
        result.update({
            "export_kind": "dc32_local_unsigned_diagnostic",
            "events": entries,
            "badge_uptime_seconds": struct.unpack_from("<I", raw, 24)[0],
        })
    elif schema == 1:
        result["opponents"] = entries
        result["auto_fire"] = bool(flags & 0x02)
    else:
        result["opponents"] = entries
        result.update({
            "sound": bool(flags & 0x02),
            "vibration": bool(flags & 0x04),
            "sync_reminder": bool(flags & 0x08),
            "at_home_mode": bool(flags & 0x10),
            "led_brightness": ("auto", "low", "medium", "high")[(flags >> 5) & 3],
            "badge_uptime_seconds": struct.unpack_from("<I", raw, 24)[0],
        })
    return result


def inspect_qr_text(value: str) -> dict[str, object]:
    """Return a lossless local report for a QR string without submitting it anywhere."""
    try:
        return {"recognized": True, "decoded": decode_sync_text(value), "raw_text": value.strip()}
    except ValueError as exc:
        text = value.strip()
        return {
            "recognized": False,
            "reason": str(exc),
            "raw_text": text,
            "length": len(text),
            "sha256": hashlib.sha256(text.encode("utf-8")).hexdigest(),
        }


def decode_sync_document(value: str) -> dict[str, object]:
    lines = [line.strip() for line in value.splitlines() if any(prefix in line for prefix in PREFIXES)]
    if not lines:
        return decode_sync_text(value)
    pages = [decode_sync_text(line) for line in lines]
    if len(pages) == 1:
        return pages[0]
    pages.sort(key=lambda page: int(page["page_index"]))
    expected = int(pages[0]["page_count"])
    identity = (pages[0]["block_id"], pages[0]["device_id"], pages[0]["badge_uid_hash"])
    if any((page["block_id"], page["device_id"], page["badge_uid_hash"]) != identity or
           int(page["page_count"]) != expected for page in pages):
        raise ValueError("sync pages do not belong to the same badge/export")
    if len({int(page["page_index"]) for page in pages}) != len(pages):
        raise ValueError("sync document contains duplicate pages")
    result = dict(pages[0])
    result["complete"] = len(pages) == expected and [int(page["page_index"]) for page in pages] == list(range(expected))
    if int(result["schema"]) == 3:
        result["events"] = [event for page in pages for event in page["events"]]
    else:
        result["opponents"] = [opponent for page in pages for opponent in page["opponents"]]
    result["pages_decoded"] = len(pages)
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("payload", help="LASERTAG.SYNC file path, QR text, or QR URL")
    parser.add_argument("--forensic", action="store_true",
                        help="retain and report an unknown QR string locally without decoding or submitting it")
    args = parser.parse_args()
    path = Path(args.payload)
    value = path.read_text(encoding="ascii") if path.is_file() else args.payload
    result = inspect_qr_text(value) if args.forensic else decode_sync_document(value)
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
