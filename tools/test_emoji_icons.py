#!/usr/bin/env python3
"""Validate committed DCEI assets and the approved catalog mapping."""
from __future__ import annotations

import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ICON_ROOT = ROOT / "assets" / "icons"


def main() -> None:
    manifest = json.loads((ICON_ROOT / "manifest.json").read_text(encoding="utf-8"))
    assert manifest["format"] == {"magic": "DCEI", "version": 1, "header_bytes": 16,
                                  "pixels": "RGBA8888, row-major"}
    assert len(manifest["catalog"]) == 29
    for catalog_entry in manifest["catalog"]:
        icon = manifest["icons"][catalog_entry["icon_id"]]
        assert catalog_entry["emoji"] == icon["emoji"]
        for size in (32, 48, 64):
            item = icon["files"][str(size)]
            path = ICON_ROOT / Path(item["path"]).relative_to("/ICONS")
            data = path.read_bytes()
            magic, version, width, height, bpp, reserved, payload_size = struct.unpack("<4sHHHBBI", data[:16])
            assert (magic, version, width, height, bpp, reserved) == (b"DCEI", 1, size, size, 32, 0)
            assert payload_size == size * size * 4 and len(data) == 16 + payload_size
            assert hashlib.sha256(data).hexdigest() == item["sha256"]
            alpha = data[16 + 3::4]
            assert any(alpha) and any(value < 255 for value in alpha)
    print(f"validated {len(manifest['icons'])} unique icons / 29 catalog mappings")


if __name__ == "__main__":
    try:
        main()
    except (AssertionError, KeyError, OSError, ValueError, struct.error) as exc:
        print(f"emoji icon validation failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
