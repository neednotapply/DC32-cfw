#!/usr/bin/env python3
"""Convert images into DC32 badge-friendly image viewer files."""

from __future__ import annotations

import argparse
import struct
import sys
from collections import defaultdict
from pathlib import Path
from typing import NamedTuple

try:
    from PIL import Image, ImageSequence
except ImportError:  # pragma: no cover - exercised by users without Pillow
    print("Pillow is required. Install it with: python -m pip install Pillow", file=sys.stderr)
    raise SystemExit(1)


CANVAS_W = 320
CANVAS_H = 240
BPP_RGB565 = 16
DCI_MAGIC = 0x31494344
DCI_VERSION = 1
DCI_KIND_STATIC = 1
DCI_HEADER_FORMAT = "<IHBBHHBBHIII9I"
DCI_HEADER_SIZE = struct.calcsize(DCI_HEADER_FORMAT)
DCI_FRAME_BYTES = CANVAS_W * CANVAS_H * 2
SUPPORTED_SUFFIXES = {".png", ".jpg", ".jpeg", ".gif"}
BADGE_GIF_SUFFIX = ".badge.gif"


class ConversionError(RuntimeError):
    pass


class ConvertPlan(NamedTuple):
    still_sources: list[Path]
    animated_gifs: list[Path]
    skipped_outputs: int


def rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def parse_size(value: str) -> tuple[int, int]:
    parts = value.lower().split("x", 1)
    if len(parts) != 2:
        raise argparse.ArgumentTypeError("expected WIDTHxHEIGHT")
    try:
        width, height = int(parts[0]), int(parts[1])
    except ValueError as exc:
        raise argparse.ArgumentTypeError("expected WIDTHxHEIGHT") from exc
    if width < 1 or height < 1 or width > CANVAS_W or height > CANVAS_H:
        raise argparse.ArgumentTypeError(f"size must be between 1x1 and {CANVAS_W}x{CANVAS_H}")
    return width, height


def fit_size(src_w: int, src_h: int, max_w: int = CANVAS_W, max_h: int = CANVAS_H, *, upscale: bool = True) -> tuple[int, int]:
    scale_num = min(max_w, max_h)
    if src_w * max_h > src_h * max_w:
        out_w = max_w
        out_h = max(1, max_w * src_h // src_w)
    else:
        out_h = max_h
        out_w = max(1, max_h * src_w // src_h)
    if not upscale and out_w > src_w and out_h > src_h:
        return src_w, src_h
    _ = scale_num
    return max(1, out_w), max(1, out_h)


def render_frame(frame: Image.Image) -> bytes:
    src = frame.convert("RGBA")
    out_w, out_h = fit_size(src.width, src.height)
    resized = src.resize((out_w, out_h), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (CANVAS_W, CANVAS_H), (0, 0, 0, 255))
    canvas.alpha_composite(resized, ((CANVAS_W - out_w) // 2, (CANVAS_H - out_h) // 2))
    rgb = canvas.convert("RGB")
    pixels = rgb.load()
    out = bytearray(DCI_FRAME_BYTES)

    for y in range(CANVAS_H):
        for x in range(CANVAS_W):
            r, g, b = pixels[x, y]
            offset = (x * CANVAS_H + (CANVAS_H - 1 - y)) * 2
            struct.pack_into("<H", out, offset, rgb565(r, g, b))
    return bytes(out)


def pack_header(kind: int, frame_count: int, loop_count: int, payload_bytes: int, *, magic: int = DCI_MAGIC, version: int = DCI_VERSION, flags: int = 0) -> bytes:
    return struct.pack(
        DCI_HEADER_FORMAT,
        magic,
        DCI_HEADER_SIZE,
        version,
        kind,
        CANVAS_W,
        CANVAS_H,
        BPP_RGB565,
        flags,
        0,
        frame_count,
        loop_count,
        payload_bytes,
        *([0] * 9),
    )


def write_static(src: Path, dst: Path) -> None:
    with Image.open(src) as image:
        payload = render_frame(image)
    dst.write_bytes(pack_header(DCI_KIND_STATIC, 1, 0, len(payload)) + payload)


def is_badge_gif(src: Path) -> bool:
    return src.name.lower().endswith(BADGE_GIF_SUFFIX)


def badge_gif_path(src: Path) -> Path:
    return src.with_name(src.name[:-4] + BADGE_GIF_SUFFIX)


def source_for_badge_gif(path: Path) -> Path:
    return path.with_name(path.name[: -len(BADGE_GIF_SUFFIX)] + ".gif")


def is_animated_gif(src: Path) -> bool:
    if src.suffix.lower() != ".gif" or is_badge_gif(src):
        return False
    with Image.open(src) as image:
        return getattr(image, "n_frames", 1) > 1


def source_paths(root: Path) -> list[Path]:
    return sorted(
        path
        for path in root.rglob("*")
        if path.is_file() and path.suffix.lower() in SUPPORTED_SUFFIXES and not is_badge_gif(path)
    )


def validate_collisions(sources: list[Path], animated_gifs: set[Path]) -> None:
    by_dest: dict[Path, list[Path]] = defaultdict(list)
    for src in sources:
        if src in animated_gifs:
            continue
        by_dest[src.with_suffix(".dci")].append(src)
    collisions = {dst: srcs for dst, srcs in by_dest.items() if len(srcs) > 1}
    if collisions:
        lines = ["Multiple sources would write the same .dci file:"]
        for dst, srcs in collisions.items():
            lines.append(f"  {dst}:")
            for src in srcs:
                lines.append(f"    {src}")
        raise ConversionError("\n".join(lines))


def clean_orphans(root: Path, dry_run: bool) -> int:
    removed = 0
    for dci in sorted(root.rglob("*.dci")):
        keep = False
        for suffix in SUPPORTED_SUFFIXES:
            src = dci.with_suffix(suffix)
            if src.exists() and not is_animated_gif(src):
                keep = True
                break
        if keep:
            continue
        print(f"{'would remove' if dry_run else 'remove'} {dci}")
        removed += 1
        if not dry_run:
            dci.unlink()

    for gif in sorted(root.rglob(f"*{BADGE_GIF_SUFFIX}")):
        source = source_for_badge_gif(gif)
        if source.exists():
            continue
        print(f"{'would remove' if dry_run else 'remove'} {gif}")
        removed += 1
        if not dry_run:
            gif.unlink()
    return removed


def gif_frame_canvas(frame: Image.Image, canvas_size: tuple[int, int]) -> Image.Image:
    max_w, max_h = canvas_size
    src = frame.convert("RGBA")
    out_w, out_h = fit_size(src.width, src.height, max_w, max_h, upscale=False)
    if (out_w, out_h) != src.size:
        src = src.resize((out_w, out_h), Image.Resampling.LANCZOS)
    canvas = Image.new("RGB", canvas_size, (0, 0, 0))
    canvas.paste(src.convert("RGB"), ((max_w - out_w) // 2, (max_h - out_h) // 2))
    return canvas


def quantize_gif_frames(frames: list[Image.Image], colors: int) -> list[Image.Image]:
    palette_source = frames[0].quantize(colors=colors, method=Image.Quantize.MEDIANCUT)
    return [frame.quantize(palette=palette_source, dither=Image.Dither.NONE) for frame in frames]


def validate_gif_output(path: Path, expected_frames: int, max_size: tuple[int, int]) -> None:
    with Image.open(path) as image:
        if image.format != "GIF":
            raise ConversionError(f"{path} is not a GIF after optimization")
        if getattr(image, "n_frames", 1) != expected_frames:
            raise ConversionError(f"{path} frame count changed during optimization")
        if image.width > max_size[0] or image.height > max_size[1]:
            raise ConversionError(f"{path} exceeds requested GIF size")


def write_optimized_gif(src: Path, dst: Path, colors: int, max_size: tuple[int, int]) -> None:
    with Image.open(src) as image:
        loop = int(image.info.get("loop", 0) or 0)
        frames: list[Image.Image] = []
        durations: list[int] = []
        for frame in ImageSequence.Iterator(image):
            durations.append(max(1, int(frame.info.get("duration", image.info.get("duration", 16)) or 16)))
            frames.append(gif_frame_canvas(frame.copy(), max_size))

    if not frames:
        raise ConversionError(f"{src} has no frames")
    quantized = quantize_gif_frames(frames, colors)
    tmp = dst.with_name(dst.name + ".tmp")
    try:
        quantized[0].save(
            tmp,
            format="GIF",
            save_all=True,
            append_images=quantized[1:],
            duration=durations,
            loop=loop,
            optimize=True,
            disposal=2,
        )
        validate_gif_output(tmp, len(frames), max_size)
        tmp.replace(dst)
    finally:
        if tmp.exists():
            tmp.unlink()


def plan_conversions(sources: list[Path], args: argparse.Namespace, replace_originals: bool) -> ConvertPlan:
    animated_gifs = [src for src in sources if is_animated_gif(src)]
    animated_set = set(animated_gifs)
    still_sources = [src for src in sources if src not in animated_set]
    skipped = 0

    if not args.force:
        still_to_convert = []
        for src in still_sources:
            dst = src.with_suffix(".dci")
            if dst.exists() and dst.stat().st_mtime >= src.stat().st_mtime:
                skipped += 1
            else:
                still_to_convert.append(src)
        still_sources = still_to_convert

        if not replace_originals:
            gifs_to_convert = []
            for src in animated_gifs:
                dst = badge_gif_path(src)
                if dst.exists() and dst.stat().st_mtime >= src.stat().st_mtime:
                    skipped += 1
                else:
                    gifs_to_convert.append(src)
            animated_gifs = gifs_to_convert

    if args.no_gif_optimize:
        skipped += len(animated_gifs)
        animated_gifs = []

    return ConvertPlan(still_sources=still_sources, animated_gifs=animated_gifs, skipped_outputs=skipped)


def print_plan(plan: ConvertPlan) -> None:
    print(f"{len(plan.still_sources)} still image(s) to convert to .dci")
    print(f"{len(plan.animated_gifs)} animated GIF(s) to optimize")
    print(f"{plan.skipped_outputs} existing output(s) will be skipped")


def prompt_replace_originals(animated_count: int) -> bool:
    if animated_count <= 0:
        return False
    try:
        answer = input("Replace original animated GIFs? [y/N] ")
    except EOFError:
        print("No answer received; keeping originals and writing .badge.gif sidecars")
        return False
    return answer.strip().lower() in {"y", "yes"}


def convert_static_source(src: Path, force: bool, dry_run: bool) -> str:
    dst = src.with_suffix(".dci")
    if dst.exists() and not force and dst.stat().st_mtime >= src.stat().st_mtime:
        return f"skip {dst}"
    if dry_run:
        return f"would convert {src} -> {dst}"
    tmp = dst.with_suffix(dst.suffix + ".tmp")
    try:
        write_static(src, tmp)
        tmp.replace(dst)
    finally:
        if tmp.exists():
            tmp.unlink()
    return f"convert {src} -> {dst}"


def convert_animated_gif(src: Path, replace_originals: bool, force: bool, dry_run: bool, colors: int, max_size: tuple[int, int]) -> str:
    dst = src if replace_originals else badge_gif_path(src)
    if dst.exists() and not replace_originals and not force and dst.stat().st_mtime >= src.stat().st_mtime:
        return f"skip {dst}"
    if dry_run:
        verb = "replace" if replace_originals else "optimize"
        return f"would {verb} {src} -> {dst}"
    tmp = dst.with_name(dst.name + ".tmp")
    try:
        write_optimized_gif(src, tmp, colors, max_size)
        tmp.replace(dst)
    finally:
        if tmp.exists():
            tmp.unlink()
    return f"{'replace' if replace_originals else 'optimize'} {src} -> {dst}"


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", nargs="?", type=Path, help="Image folder to convert; defaults to this script's folder")
    parser.add_argument("--force", action="store_true", help="Regenerate existing outputs")
    parser.add_argument("--dry-run", action="store_true", help="Print work without writing files")
    parser.add_argument("--clean", action="store_true", help="Remove .dci and .badge.gif files whose source image no longer exists")
    parser.add_argument("--keep-originals", action="store_true", help="Write optimized GIFs as name.badge.gif")
    parser.add_argument("--replace-originals", action="store_true", help="Replace original animated GIFs after successful optimization")
    parser.add_argument("--no-gif-optimize", action="store_true", help="Skip animated GIF optimization")
    parser.add_argument("--gif-colors", type=int, default=64, help="Animated GIF palette size, 2-256 colors (default: 64)")
    parser.add_argument("--gif-max-size", type=parse_size, default=(CANVAS_W, CANVAS_H), help="Max optimized GIF canvas size, default 320x240")
    parser.add_argument("--target-fps", type=int, default=30, help="Accepted for compatibility; GIF timing is preserved")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    root = (args.root if args.root is not None else Path(__file__).resolve().parent).resolve()
    if args.keep_originals and args.replace_originals:
        raise ConversionError("--keep-originals and --replace-originals cannot both be used")
    if args.gif_colors < 2 or args.gif_colors > 256:
        raise ConversionError("--gif-colors must be between 2 and 256")
    if not root.is_dir():
        raise ConversionError(f"{root} is not a directory")
    if args.clean:
        clean_orphans(root, args.dry_run)
    sources = source_paths(root)
    animated_set = {src for src in sources if is_animated_gif(src)}
    validate_collisions(sources, animated_set)

    replace_originals = args.replace_originals
    initial_plan = plan_conversions(sources, args, replace_originals)
    print_plan(initial_plan)
    if not args.keep_originals and not args.replace_originals and not args.dry_run and initial_plan.animated_gifs:
        replace_originals = prompt_replace_originals(len(initial_plan.animated_gifs))
    plan = plan_conversions(sources, args, replace_originals)
    if plan != initial_plan:
        print_plan(plan)

    for src in plan.still_sources:
        print(convert_static_source(src, args.force, args.dry_run))
    for src in plan.animated_gifs:
        print(convert_animated_gif(src, replace_originals, args.force, args.dry_run, args.gif_colors, args.gif_max_size))
    if not sources:
        print(f"No compatible source images found in {root}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ConversionError as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
