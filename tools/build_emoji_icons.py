#!/usr/bin/env python3
"""Capture approved Unicode Emoji 17.0 Browser glyphs and build DC32 DCEI assets.

Capture is deliberately a release-art step.  CI validates and packages the
committed PNG/DCEI files, but does not require Chrome or regenerate artwork.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE_URL = "https://unicode.org/emoji/charts/full-emoji-list.html"
CHROME_CANDIDATES = (
    Path(r"C:/Program Files/Google/Chrome/Application/chrome.exe"),
    Path(r"C:/Program Files (x86)/Google/Chrome/Application/chrome.exe"),
)
# Catalog order is the approved mapping.  Repeated codepoints intentionally
# share one SD-resident asset directory.
CATALOG = (
    ("Game Boy", "1f3ae", "🎮"), ("NES", "1f579-fe0f", "🕹️"),
    ("Arduboy", "1f4df", "📟"), ("Universal Remote", "1f4fa", "📺"),
    ("Image Viewer", "1f5bc-fe0f", "🖼️"), ("Music", "1f3b5", "🎵"),
    ("BadUSB", "1f50c", "🔌"), ("Autoclicker", "1f5b1-fe0f", "🖱️"),
    ("USB Gamepad", "1f3ae", "🎮"), ("Laser Tag", "1f3af", "🎯"),
    ("RaspyJack Remote", "1f967", "🥧"), ("Pwnagotchi Remote", "1f431", "🐱"),
    ("Pong", "1f3d3", "🏓"), ("Tetris", "1f9f1", "🧱"),
    ("Arkanoid", "1f9f1", "🧱"), ("Flappy Bird", "1f426", "🐦"),
    ("T-Rex Runner", "1f996", "🦖"), ("DOOM", "1f479", "👹"),
    ("Chip's Challenge", "1f48e", "💎"), ("Scorched Earth", "1f4a3", "💣"),
    ("Pipe Dream", "1f527", "🔧"), ("Sokoban", "1f4e6", "📦"),
    ("OpenJazz", "1f430", "🐰"), ("Sensible Soccer", "26bd", "⚽"),
    ("Starfield", "1f31f", "🌟"), ("Spiro", "1f300", "🌀"),
    ("Cube", "1f9ca", "🧊"), ("DVD Bounce", "1f4c0", "📀"),
    ("Scrolling Pattern", "1f504", "🔄"),
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def chrome() -> Path:
    found = next((p for p in CHROME_CANDIDATES if p.is_file()), None)
    if not found:
        raise SystemExit("Chrome was not found; use the committed browser captures.")
    return found


def capture(source_dir: Path) -> None:
    """Use Chrome's native color-emoji renderer, with a transparent canvas."""
    # The Unicode chart is the authority for the approved Browser glyph
    # sequences.  Rendering remains local so the capture is reproducible from
    # the recorded browser/font environment instead of downloading artwork.
    chart = subprocess.run(
        [str(chrome()), "--headless=new", "--disable-gpu", "--dump-dom", SOURCE_URL],
        stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, encoding="utf-8", errors="replace",
    ).stdout
    if "Full Emoji List, v17.0" not in chart or "Browser" not in chart:
        raise SystemExit("Unicode Emoji 17.0 Browser chart did not have the expected format")
    missing = sorted({emoji for _, _, emoji in CATALOG
                      if emoji not in chart and emoji.replace("\ufe0f", "") not in chart})
    if missing:
        raise SystemExit(f"Approved emoji missing from Unicode Browser chart: {', '.join(missing)}")
    source_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="dc32-emoji-") as temp:
        temp_dir = Path(temp)
        captured: set[str] = set()
        for _, icon_id, emoji in CATALOG:
            if icon_id in captured:
                continue
            captured.add(icon_id)
            html = temp_dir / f"{icon_id}.html"
            png = source_dir / f"{icon_id}.png"
            html.write_text(
                "<!doctype html><meta charset=utf-8><style>html,body{margin:0;width:256px;"
                "height:256px;background:transparent;overflow:hidden}body{display:grid;place-items:center;"
                "font-family:'Segoe UI Emoji','Noto Color Emoji',sans-serif;font-size:224px;line-height:1}</style>"
                f"<body>{emoji}</body>", encoding="utf-8")
            subprocess.run([
                str(chrome()), "--headless=new", "--disable-gpu", "--hide-scrollbars",
                "--default-background-color=00000000", "--window-size=256,256",
                f"--screenshot={png}", html.as_uri(),
            ], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def crop_visible(image: Image.Image) -> Image.Image:
    alpha = image.getchannel("A")
    bbox = alpha.getbbox()
    if not bbox:
        raise ValueError("browser capture contained no visible glyph")
    return image.crop(bbox)


def write_dcei(image: Image.Image, path: Path, size: int) -> None:
    # LANCZOS downsizing preserves the browser artwork while generating the
    # fixed-size variant required by the device API.
    rgba = image.resize((size, size), Image.Resampling.LANCZOS).convert("RGBA")
    payload = rgba.tobytes()
    header = struct.pack("<4sHHHBBI", b"DCEI", 1, size, size, 32, 0, len(payload))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(header + payload)


def build(source_dir: Path, output_dir: Path) -> None:
    entries: dict[str, dict[str, object]] = {}
    for _, icon_id, emoji in CATALOG:
        if icon_id in entries:
            continue
        source = source_dir / f"{icon_id}.png"
        if not source.is_file():
            raise SystemExit(f"Missing browser capture: {source}")
        art = crop_visible(Image.open(source).convert("RGBA"))
        files: dict[str, dict[str, object]] = {}
        for size in (32, 48, 64):
            dst = output_dir / icon_id / f"{size}.dcei"
            write_dcei(art, dst, size)
            files[str(size)] = {"path": f"/ICONS/{icon_id}/{size}.dcei", "sha256": sha256(dst),
                                "width": size, "height": size, "format": "RGBA8888"}
        entries[icon_id] = {"emoji": emoji, "sequence": " ".join(f"U+{ord(c):04X}" for c in emoji),
                            "browser_capture": f"assets/emoji-browser/{icon_id}.png", "files": files}
    manifest = {
        "schema": 1, "format": {"magic": "DCEI", "version": 1, "header_bytes": 16,
                                    "pixels": "RGBA8888, row-major"},
        "source": {"unicode_emoji_version": "17.0", "url": SOURCE_URL,
                   "representation": "Browser", "renderer": "Google Chrome headless native emoji rendering",
                   "font_stack": "Segoe UI Emoji, Noto Color Emoji, sans-serif"},
        "catalog": [{"app": app, "icon_id": icon_id, "emoji": emoji} for app, icon_id, emoji in CATALOG],
        "icons": entries,
    }
    (output_dir / "manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture", action="store_true", help="capture transparent artwork with installed Chrome")
    parser.add_argument("--source-dir", type=Path, default=ROOT / "assets" / "emoji-browser")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "assets" / "icons")
    args = parser.parse_args()
    if args.capture:
        capture(args.source_dir)
    build(args.source_dir, args.output_dir)


if __name__ == "__main__":
    main()
