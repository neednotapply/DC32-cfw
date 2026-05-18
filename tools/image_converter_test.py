#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image


REPO = Path(__file__).resolve().parents[1]
CONVERTER = REPO / "tools" / "image_converter.py"
spec = importlib.util.spec_from_file_location("image_converter", CONVERTER)
image_converter = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(image_converter)


def read_header(path: Path) -> dict[str, int]:
    data = path.read_bytes()
    fields = struct.unpack_from(image_converter.DCI_HEADER_FORMAT, data)
    return {
        "magic": fields[0],
        "header_size": fields[1],
        "version": fields[2],
        "kind": fields[3],
        "width": fields[4],
        "height": fields[5],
        "bpp": fields[6],
        "flags": fields[7],
        "frame_count": fields[9],
        "loop_count": fields[10],
        "payload_bytes": fields[11],
        "file_size": len(data),
    }


def fb_pixel(payload: bytes, x: int, y: int) -> int:
    offset = (x * image_converter.CANVAS_H + (image_converter.CANVAS_H - 1 - y)) * 2
    return struct.unpack_from("<H", payload, offset)[0]


def expect(name: str, condition: bool) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def test_static_png_and_jpeg(tmp: Path) -> None:
    tmp.mkdir()
    png = tmp / "static.png"
    jpg = tmp / "photo.jpg"
    Image.new("RGB", (2, 1), (0, 0, 0)).save(png)
    with Image.open(png) as image:
        image.putpixel((0, 0), (255, 0, 0))
        image.putpixel((1, 0), (0, 255, 0))
        image.save(png)
    Image.new("RGB", (4, 3), (32, 64, 128)).save(jpg)

    image_converter.main(["--target-fps", "0", str(tmp)])
    png_dci = png.with_suffix(".dci")
    jpg_dci = jpg.with_suffix(".dci")
    png_hdr = read_header(png_dci)
    jpg_hdr = read_header(jpg_dci)
    expect("static PNG kind", png_hdr["kind"] == image_converter.DCI_KIND_STATIC)
    expect("static JPEG kind", jpg_hdr["kind"] == image_converter.DCI_KIND_STATIC)
    expect("static payload size", png_hdr["payload_bytes"] == image_converter.DCI_FRAME_BYTES)
    expect("static file size", png_hdr["file_size"] == image_converter.DCI_HEADER_SIZE + image_converter.DCI_FRAME_BYTES)
    payload = png_dci.read_bytes()[image_converter.DCI_HEADER_SIZE:]
    expect("static red pixel", fb_pixel(payload, 80, 120) == image_converter.rgb565(255, 0, 0))
    expect("static green pixel", fb_pixel(payload, 240, 120) == image_converter.rgb565(0, 255, 0))


def gif_frame_count(path: Path) -> int:
    with Image.open(path) as image:
        return getattr(image, "n_frames", 1)


def test_animated_gif_optimized_sidecar_and_replace(tmp: Path) -> None:
    tmp.mkdir()
    gif = tmp / "anim.gif"
    first = Image.new("RGBA", (10, 10), (255, 0, 0, 255))
    second = Image.new("RGBA", (10, 10), (0, 0, 0, 0))
    for y in range(3, 7):
        for x in range(3, 7):
            second.putpixel((x, y), (0, 255, 0, 255))
    first.save(
        gif,
        save_all=True,
        append_images=[second],
        duration=[50, 120],
        loop=3,
        disposal=[1, 1],
    )

    image_converter.main(["--keep-originals", "--gif-colors", "8", str(tmp)])
    badge_gif = tmp / "anim.badge.gif"
    expect("animated GIF does not create DCI", not gif.with_suffix(".dci").exists())
    expect("optimized badge GIF exists", badge_gif.exists())
    expect("source GIF preserved", gif.exists())
    with Image.open(badge_gif) as image:
        expect("optimized GIF is 320 wide", image.width == image_converter.CANVAS_W)
        expect("optimized GIF is 240 high", image.height == image_converter.CANVAS_H)
        expect("optimized GIF frame count", getattr(image, "n_frames", 1) == 2)
        expect("optimized GIF loop preserved", int(image.info.get("loop", 0) or 0) == 3)
        image.seek(0)
        expect("first GIF delay preserved", int(image.info.get("duration", 0)) == 50)
        image.seek(1)
        expect("second GIF delay preserved", int(image.info.get("duration", 0)) == 120)

    replace = tmp / "replace.gif"
    first.save(replace, save_all=True, append_images=[second], duration=[40, 80], loop=1)
    image_converter.main(["--replace-originals", "--gif-max-size", "160x120", str(tmp)])
    expect("replace did not create sidecar", not (tmp / "replace.badge.gif").exists())
    expect("replace remains animated", gif_frame_count(replace) == 2)
    with Image.open(replace) as image:
        expect("replace size capped", image.width <= 160 and image.height <= 120)


def test_collision_dry_run_clean_and_default(tmp: Path) -> None:
    tmp.mkdir()
    collision = tmp / "collision"
    collision.mkdir()
    Image.new("RGB", (1, 1), (1, 2, 3)).save(collision / "same.png")
    Image.new("RGB", (1, 1), (1, 2, 3)).save(collision / "same.jpg")
    try:
        image_converter.main([str(collision)])
    except image_converter.ConversionError:
        pass
    else:
        raise SystemExit("FAIL: collision rejected")

    dry = tmp / "dry"
    dry.mkdir()
    Image.new("RGB", (1, 1), (1, 2, 3)).save(dry / "dry.png")
    image_converter.main(["--dry-run", str(dry)])
    expect("dry run writes nothing", not (dry / "dry.dci").exists())
    (dry / "orphan.dci").write_bytes(b"old")
    image_converter.main(["--clean", str(dry)])
    expect("clean removes orphan", not (dry / "orphan.dci").exists())

    images = tmp / "IMAGES"
    images.mkdir()
    shutil.copy(CONVERTER, images / "image_converter.py")
    Image.new("RGB", (1, 1), (255, 0, 0)).save(images / "default.png")
    subprocess.run([sys.executable, str(images / "image_converter.py")], check=True, cwd=tmp)
    expect("default script folder conversion", (images / "default.dci").exists())


def main() -> None:
    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp = Path(tmp_dir)
        test_static_png_and_jpeg(tmp / "static")
        test_animated_gif_optimized_sidecar_and_replace(tmp / "animated")
        test_collision_dry_run_clean_and_default(tmp / "misc")
    print("image converter tests passed")


if __name__ == "__main__":
    main()
