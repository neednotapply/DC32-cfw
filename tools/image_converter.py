#!/usr/bin/env python3
"""Convert images into DC32 badge-friendly image viewer files."""

from __future__ import annotations

import argparse
import struct
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

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

DCA_MAGIC = 0x31414344
DCA_VERSION = 1
DCA_BPP_INDEXED = 8
DCA_HEADER_FORMAT = "<IHBBHHHHIIIIII6I"
DCA_FRAME_FORMAT = "<IHHII"
DCA_RECT_FORMAT = "<HHHHBBIH"
DCA_HEADER_SIZE = struct.calcsize(DCA_HEADER_FORMAT)
DCA_FRAME_SIZE = struct.calcsize(DCA_FRAME_FORMAT)
DCA_RECT_SIZE = struct.calcsize(DCA_RECT_FORMAT)
DCA_CODEC_RAW = 0
DCA_CODEC_RLE = 1
DCA_DEFAULT_MAX_BYTES = 3 * 1024 * 1024
DCA_TILE = 16

SUPPORTED_SUFFIXES = {".png", ".jpg", ".jpeg", ".gif", ".webp"}
ANIMATED_SUFFIXES = {".gif", ".webp", ".png"}


class ConversionError(RuntimeError):
    pass


@dataclass(frozen=True)
class ConvertPlan:
    still_sources: list[Path]
    animated_sources: list[Path]
    skipped_outputs: int


@dataclass(frozen=True)
class ConvertFailure:
    source: Path
    reason: str


@dataclass(frozen=True)
class DcaRect:
    x: int
    y: int
    w: int
    h: int
    codec: int
    payload: bytes


@dataclass(frozen=True)
class DcaFrame:
    delay_ms: int
    rects: list[DcaRect]


@dataclass(frozen=True)
class DcaBuild:
    colors: int
    data: bytes
    warnings: list[str]


def rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def fit_size(src_w: int, src_h: int, max_w: int = CANVAS_W, max_h: int = CANVAS_H, *, upscale: bool = True) -> tuple[int, int]:
    if src_w <= 0 or src_h <= 0:
        return 1, 1
    if src_w * max_h > src_h * max_w:
        out_w = max_w
        out_h = max(1, max_w * src_h // src_w)
    else:
        out_h = max_h
        out_w = max(1, max_h * src_w // src_h)
    if not upscale and out_w > src_w and out_h > src_h:
        return src_w, src_h
    return max(1, out_w), max(1, out_h)


def render_rgb_frame(frame: Image.Image) -> Image.Image:
    src = frame.convert("RGBA")
    out_w, out_h = fit_size(src.width, src.height)
    resized = src.resize((out_w, out_h), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (CANVAS_W, CANVAS_H), (0, 0, 0, 255))
    canvas.alpha_composite(resized, ((CANVAS_W - out_w) // 2, (CANVAS_H - out_h) // 2))
    return canvas.convert("RGB")


def render_dci_payload(frame: Image.Image) -> bytes:
    rgb = render_rgb_frame(frame)
    pixels = rgb.load()
    out = bytearray(DCI_FRAME_BYTES)

    for y in range(CANVAS_H):
        for x in range(CANVAS_W):
            r, g, b = pixels[x, y]
            offset = (x * CANVAS_H + (CANVAS_H - 1 - y)) * 2
            struct.pack_into("<H", out, offset, rgb565(r, g, b))
    return bytes(out)


def pack_dci_header(kind: int, frame_count: int, loop_count: int, payload_bytes: int, *, magic: int = DCI_MAGIC, version: int = DCI_VERSION, flags: int = 0) -> bytes:
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
        payload = render_dci_payload(image)
    dst.write_bytes(pack_dci_header(DCI_KIND_STATIC, 1, 0, len(payload)) + payload)


def source_paths(root: Path) -> list[Path]:
    return sorted(path for path in root.rglob("*") if path.is_file() and path.suffix.lower() in SUPPORTED_SUFFIXES)


def is_animated_source(src: Path) -> bool:
    if src.suffix.lower() not in ANIMATED_SUFFIXES:
        return False
    try:
        with Image.open(src) as image:
            return getattr(image, "is_animated", False) or getattr(image, "n_frames", 1) > 1
    except OSError:
        return False


def validate_collisions(sources: list[Path], animated_sources: set[Path]) -> None:
    by_dest: dict[Path, list[Path]] = defaultdict(list)
    for src in sources:
        by_dest[src.with_suffix(".dca" if src in animated_sources else ".dci")].append(src)
    collisions = {dst: srcs for dst, srcs in by_dest.items() if len(srcs) > 1}
    if collisions:
        lines = ["Multiple sources would write the same converted file:"]
        for dst, srcs in collisions.items():
            lines.append(f"  {dst}:")
            for src in srcs:
                lines.append(f"    {src}")
        raise ConversionError("\n".join(lines))


def clean_orphans(root: Path, dry_run: bool) -> int:
    removed = 0
    for converted in sorted([*root.rglob("*.dci"), *root.rglob("*.dca")]):
        keep = False
        for suffix in SUPPORTED_SUFFIXES:
            src = converted.with_suffix(suffix)
            if src.exists() and (converted.suffix.lower() == ".dca") == is_animated_source(src):
                keep = True
                break
        if keep:
            continue
        print(f"{'would remove' if dry_run else 'remove'} {converted}")
        removed += 1
        if not dry_run:
            converted.unlink()
    return removed


def animation_frames(src: Path) -> tuple[list[Image.Image], list[int], int]:
    frames: list[Image.Image] = []
    durations: list[int] = []
    with Image.open(src) as image:
        loop = int(image.info.get("loop", 0) or 0)
        for frame in ImageSequence.Iterator(image):
            durations.append(max(1, int(frame.info.get("duration", image.info.get("duration", 33)) or 33)))
            frames.append(render_rgb_frame(frame.copy()))
    if not frames:
        raise ConversionError(f"{src} has no animation frames")
    return frames, durations, loop


def quantize_frames(frames: list[Image.Image], colors: int) -> tuple[list[Image.Image], list[int]]:
    if len(frames) == 1:
        palette_source = frames[0].quantize(colors=colors, method=Image.Quantize.MEDIANCUT)
    else:
        contact = Image.new("RGB", (CANVAS_W, CANVAS_H * len(frames)))
        for idx, frame in enumerate(frames):
            contact.paste(frame, (0, CANVAS_H * idx))
        palette_source = contact.quantize(colors=colors, method=Image.Quantize.MEDIANCUT)
    quantized = [frame.quantize(palette=palette_source, dither=Image.Dither.NONE) for frame in frames]
    raw_palette = palette_source.getpalette() or []
    palette: list[int] = []
    for idx in range(colors):
        base = idx * 3
        if base + 2 < len(raw_palette):
            palette.append(rgb565(raw_palette[base], raw_palette[base + 1], raw_palette[base + 2]))
        else:
            palette.append(0)
    return quantized, palette


def rle_encode(data: bytes) -> bytes:
    out = bytearray()
    pos = 0
    data_len = len(data)
    while pos < data_len:
        run_len = 1
        while pos + run_len < data_len and run_len < 128 and data[pos + run_len] == data[pos]:
            run_len += 1
        if run_len >= 3:
            out.append(0x80 | (run_len - 1))
            out.append(data[pos])
            pos += run_len
            continue

        lit_start = pos
        pos += 1
        while pos < data_len and pos - lit_start < 128:
            look_run = 1
            while pos + look_run < data_len and look_run < 128 and data[pos + look_run] == data[pos]:
                look_run += 1
            if look_run >= 3:
                break
            pos += 1
        lit_len = pos - lit_start
        out.append(lit_len - 1)
        out.extend(data[lit_start:pos])
    return bytes(out)


def rect_indices(image: Image.Image, x: int, y: int, w: int, h: int) -> bytes:
    pixels = image.load()
    out = bytearray(w * h)
    pos = 0
    for px in range(x, x + w):
        for py in range(y + h - 1, y - 1, -1):
            out[pos] = pixels[px, py]
            pos += 1
    return bytes(out)


def changed_tile_runs(cur: Image.Image, prev: Image.Image | None) -> list[tuple[int, int, int, int]]:
    if prev is None:
        return [(0, 0, CANVAS_W, CANVAS_H)]
    cur_bytes = cur.tobytes()
    prev_bytes = prev.tobytes()
    runs: list[tuple[int, int, int, int]] = []
    for ty in range(0, CANVAS_H, DCA_TILE):
        run_x: int | None = None
        run_w = 0
        h = min(DCA_TILE, CANVAS_H - ty)
        for tx in range(0, CANVAS_W, DCA_TILE):
            w = min(DCA_TILE, CANVAS_W - tx)
            changed = False
            for row in range(h):
                off = (ty + row) * CANVAS_W + tx
                if cur_bytes[off:off + w] != prev_bytes[off:off + w]:
                    changed = True
                    break
            if changed:
                if run_x is None:
                    run_x = tx
                    run_w = w
                else:
                    run_w += w
            elif run_x is not None:
                runs.append((run_x, ty, run_w, h))
                run_x = None
                run_w = 0
        if run_x is not None:
            runs.append((run_x, ty, run_w, h))
    return runs


def make_dca_frames(frames: list[Image.Image]) -> list[DcaFrame]:
    out: list[DcaFrame] = []
    prev: Image.Image | None = None
    for idx, frame in enumerate(frames):
        rects: list[DcaRect] = []
        for x, y, w, h in changed_tile_runs(frame, prev):
            raw = rect_indices(frame, x, y, w, h)
            rle = rle_encode(raw)
            if len(rle) + DCA_RECT_SIZE < len(raw):
                rects.append(DcaRect(x, y, w, h, DCA_CODEC_RLE, rle))
            else:
                rects.append(DcaRect(x, y, w, h, DCA_CODEC_RAW, raw))
        if idx == 0 and not rects:
            rects.append(DcaRect(0, 0, CANVAS_W, CANVAS_H, DCA_CODEC_RAW, rect_indices(frame, 0, 0, CANVAS_W, CANVAS_H)))
        out.append(DcaFrame(0, rects))
        prev = frame
    return out


def dca_color_attempts(colors: int) -> list[int]:
    attempts = [colors]
    for fallback in [192, 160, 128, 96, 64, 48, 32, 24, 16, 12, 8, 4, 2]:
        if fallback < colors and fallback not in attempts:
            attempts.append(fallback)
    return attempts


def build_dca(src: Path, rgb_frames: list[Image.Image], durations: list[int], loop: int, colors: int, target_fps: int) -> DcaBuild:
    quantized, palette = quantize_frames(rgb_frames, colors)
    dca_frames = make_dca_frames(quantized)
    dca_frames = [DcaFrame(durations[idx], frame.rects) for idx, frame in enumerate(dca_frames)]

    palette_offset = DCA_HEADER_SIZE
    palette_bytes = b"".join(struct.pack("<H", color) for color in palette)
    frame_table_offset = palette_offset + len(palette_bytes)
    data_offset = frame_table_offset + len(dca_frames) * DCA_FRAME_SIZE
    frame_records = bytearray()
    data = bytearray()
    warnings: list[str] = []
    target_budget = max(1, DCA_DEFAULT_MAX_BYTES // max(1, target_fps * 8))
    over_budget_frames = 0
    max_frame_data = 0

    for frame in dca_frames:
        frame_data_start = data_offset + len(data)
        frame_data = bytearray()
        for rect in frame.rects:
            frame_data.extend(struct.pack(DCA_RECT_FORMAT, rect.x, rect.y, rect.w, rect.h, rect.codec, 0, len(rect.payload), 0))
            frame_data.extend(rect.payload)
        max_frame_data = max(max_frame_data, len(frame_data))
        if len(frame_data) > target_budget:
            over_budget_frames += 1
        frame_records.extend(struct.pack(DCA_FRAME_FORMAT, frame.delay_ms, len(frame.rects), 0, frame_data_start, len(frame_data)))
        data.extend(frame_data)

    if over_budget_frames:
        warnings.append(
            f"{src.name}: {over_budget_frames}/{len(dca_frames)} frame payloads exceed the {target_fps} fps budget "
            f"(max {max_frame_data} bytes, budget {target_budget} bytes)"
        )

    payload_bytes = len(palette_bytes) + len(frame_records) + len(data)
    header = struct.pack(
        DCA_HEADER_FORMAT,
        DCA_MAGIC,
        DCA_HEADER_SIZE,
        DCA_VERSION,
        DCA_BPP_INDEXED,
        CANVAS_W,
        CANVAS_H,
        len(dca_frames),
        len(palette),
        loop,
        payload_bytes,
        frame_table_offset,
        palette_offset,
        data_offset,
        0,
        *([0] * 6),
    )
    out = header + palette_bytes + bytes(frame_records) + bytes(data)
    return DcaBuild(colors, out, warnings)


def write_dca(src: Path, dst: Path, colors: int, target_fps: int, max_bytes: int, final_dst: Path | None = None) -> list[str]:
    rgb_frames, durations, loop = animation_frames(src)
    best: DcaBuild | None = None
    for attempt_colors in dca_color_attempts(colors):
        build = build_dca(src, rgb_frames, durations, loop, attempt_colors, target_fps)
        if best is None or len(build.data) < len(best.data):
            best = build
        if len(build.data) <= max_bytes:
            dst.write_bytes(build.data)
            warnings = list(build.warnings)
            if attempt_colors != colors:
                warnings.append(f"{src.name}: reduced animated palette from {colors} to {attempt_colors} colors to fit {max_bytes} bytes")
            return warnings

    if best is not None:
        display_dst = final_dst if final_dst is not None else dst
        raise ConversionError(
            f"{src} -> {display_dst} would be {len(best.data)} bytes at {best.colors} colors after palette reduction; max is {max_bytes}"
        )
    raise ConversionError(f"{src} has no animation frames")


def plan_conversions(sources: list[Path], args: argparse.Namespace) -> ConvertPlan:
    animated_sources = [src for src in sources if is_animated_source(src)]
    animated_set = set(animated_sources)
    still_sources = [src for src in sources if src not in animated_set]
    skipped = 0

    if not args.force:
        pending_still = []
        for src in still_sources:
            dst = src.with_suffix(".dci")
            if dst.exists() and dst.stat().st_mtime >= src.stat().st_mtime:
                skipped += 1
            else:
                pending_still.append(src)
        pending_anim = []
        for src in animated_sources:
            dst = src.with_suffix(".dca")
            if dst.exists() and dst.stat().st_mtime >= src.stat().st_mtime:
                skipped += 1
            else:
                pending_anim.append(src)
        still_sources = pending_still
        animated_sources = pending_anim

    return ConvertPlan(still_sources=still_sources, animated_sources=animated_sources, skipped_outputs=skipped)


def print_plan(plan: ConvertPlan) -> None:
    print(f"{len(plan.still_sources)} still image(s) to convert to .dci")
    print(f"{len(plan.animated_sources)} animated image(s) to convert to .dca")
    print(f"{plan.skipped_outputs} existing output(s) will be skipped")


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


def convert_animated_source(src: Path, force: bool, dry_run: bool, colors: int, target_fps: int, max_bytes: int) -> str:
    dst = src.with_suffix(".dca")
    if dst.exists() and not force and dst.stat().st_mtime >= src.stat().st_mtime:
        return f"skip {dst}"
    if dry_run:
        return f"would convert {src} -> {dst}"
    tmp = dst.with_suffix(dst.suffix + ".tmp")
    try:
        warnings = write_dca(src, tmp, colors, target_fps, max_bytes, dst)
        tmp.replace(dst)
    finally:
        if tmp.exists():
            tmp.unlink()
    for warning in warnings:
        print(f"warning: {warning}", file=sys.stderr)
    return f"convert {src} -> {dst}"


def convert_sources(plan: ConvertPlan, args: argparse.Namespace) -> tuple[int, list[ConvertFailure]]:
    converted = 0
    failures: list[ConvertFailure] = []

    for src in plan.still_sources:
        try:
            result = convert_static_source(src, args.force, args.dry_run)
            print(result)
            if result.startswith("convert "):
                converted += 1
        except (ConversionError, OSError) as exc:
            failures.append(ConvertFailure(src, str(exc)))
            print(f"error: failed {src}: {exc}", file=sys.stderr)

    for src in plan.animated_sources:
        try:
            result = convert_animated_source(src, args.force, args.dry_run, args.colors, args.target_fps, args.max_bytes)
            print(result)
            if result.startswith("convert "):
                converted += 1
        except (ConversionError, OSError) as exc:
            failures.append(ConvertFailure(src, str(exc)))
            print(f"error: failed {src}: {exc}", file=sys.stderr)

    return converted, failures


def print_summary(converted: int, skipped: int, failures: list[ConvertFailure]) -> None:
    print(f"converted {converted} file(s), skipped {skipped} existing output(s), failed {len(failures)} file(s)")
    if failures:
        print("failed source(s):")
        for failure in failures:
            print(f"  {failure.source}")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", nargs="?", type=Path, help="Image folder to convert; defaults to this script's folder")
    parser.add_argument("--force", action="store_true", help="Regenerate existing outputs")
    parser.add_argument("--dry-run", action="store_true", help="Print work without writing files")
    parser.add_argument("--clean", action="store_true", help="Remove .dci/.dca files whose source image no longer exists")
    parser.add_argument("--colors", type=int, default=64, help="Animated palette size, 2-256 colors (default: 64)")
    parser.add_argument("--target-fps", type=int, default=30, help="Target animation FPS used for warnings (default: 30)")
    parser.add_argument("--max-bytes", type=int, default=DCA_DEFAULT_MAX_BYTES, help="Max .dca file size in bytes")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    root = (args.root if args.root is not None else Path(__file__).resolve().parent).resolve()
    if args.colors < 2 or args.colors > 256:
        raise ConversionError("--colors must be between 2 and 256")
    if args.target_fps < 1 or args.target_fps > 120:
        raise ConversionError("--target-fps must be between 1 and 120")
    if args.max_bytes < DCA_HEADER_SIZE:
        raise ConversionError("--max-bytes is too small")
    if not root.is_dir():
        raise ConversionError(f"{root} is not a directory")
    if args.clean:
        clean_orphans(root, args.dry_run)
    sources = source_paths(root)
    animated_set = {src for src in sources if is_animated_source(src)}
    validate_collisions(sources, animated_set)
    plan = plan_conversions(sources, args)
    print_plan(plan)
    converted, failures = convert_sources(plan, args)
    print_summary(converted, plan.skipped_outputs, failures)
    if not sources:
        print(f"No compatible source images found in {root}")
    return 1 if failures else 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ConversionError as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
