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
sys.modules[spec.name] = image_converter
spec.loader.exec_module(image_converter)


def read_dci_header(path: Path) -> dict[str, int]:
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


def read_dca(path: Path):
    data = path.read_bytes()
    fields = struct.unpack_from(image_converter.DCA_HEADER_FORMAT, data)
    hdr = {
        "magic": fields[0],
        "header_size": fields[1],
        "version": fields[2],
        "bpp": fields[3],
        "width": fields[4],
        "height": fields[5],
        "frame_count": fields[6],
        "palette_count": fields[7],
        "loop_count": fields[8],
        "payload_bytes": fields[9],
        "frame_table_offset": fields[10],
        "palette_offset": fields[11],
        "data_offset": fields[12],
        "flags": fields[13],
        "file_size": len(data),
    }
    frames = []
    for idx in range(hdr["frame_count"]):
        off = hdr["frame_table_offset"] + idx * image_converter.DCA_FRAME_SIZE
        delay_ms, rect_count, flags, data_offset, data_bytes = struct.unpack_from(image_converter.DCA_FRAME_FORMAT, data, off)
        frames.append(
            {
                "delay_ms": delay_ms,
                "rect_count": rect_count,
                "flags": flags,
                "data_offset": data_offset,
                "data_bytes": data_bytes,
            }
        )
    return hdr, frames, data


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

    image_converter.main([str(tmp)])
    png_dci = png.with_suffix(".dci")
    jpg_dci = jpg.with_suffix(".dci")
    png_hdr = read_dci_header(png_dci)
    jpg_hdr = read_dci_header(jpg_dci)
    expect("static PNG kind", png_hdr["kind"] == image_converter.DCI_KIND_STATIC)
    expect("static JPEG kind", jpg_hdr["kind"] == image_converter.DCI_KIND_STATIC)
    expect("static payload size", png_hdr["payload_bytes"] == image_converter.DCI_FRAME_BYTES)
    expect("static file size", png_hdr["file_size"] == image_converter.DCI_HEADER_SIZE + image_converter.DCI_FRAME_BYTES)
    payload = png_dci.read_bytes()[image_converter.DCI_HEADER_SIZE:]
    expect("static red pixel", fb_pixel(payload, 80, 120) == image_converter.rgb565(255, 0, 0))
    expect("static green pixel", fb_pixel(payload, 240, 120) == image_converter.rgb565(0, 255, 0))


def test_animated_gif_to_dca(tmp: Path) -> None:
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

    image_converter.main(["--colors", "8", str(tmp)])
    dca = tmp / "anim.dca"
    expect("animated GIF does not create DCI", not gif.with_suffix(".dci").exists())
    expect("DCA exists", dca.exists())
    hdr, frames, data = read_dca(dca)
    expect("DCA magic", hdr["magic"] == image_converter.DCA_MAGIC)
    expect("DCA full width", hdr["width"] == image_converter.CANVAS_W)
    expect("DCA full height", hdr["height"] == image_converter.CANVAS_H)
    expect("DCA indexed", hdr["bpp"] == image_converter.DCA_BPP_INDEXED)
    expect("DCA frame count", hdr["frame_count"] == 2)
    expect("DCA loop preserved", hdr["loop_count"] == 3)
    expect("first delay preserved", frames[0]["delay_ms"] == 50)
    expect("second delay preserved", frames[1]["delay_ms"] == 120)
    expect("first frame has rects", frames[0]["rect_count"] >= 1)
    expect("second frame delta smaller than full frame", frames[1]["data_bytes"] < image_converter.CANVAS_W * image_converter.CANVAS_H)
    expect("payload size matches file", hdr["file_size"] == image_converter.DCA_HEADER_SIZE + hdr["payload_bytes"])
    expect("frame data in file", frames[1]["data_offset"] + frames[1]["data_bytes"] <= len(data))


def test_collision_dry_run_clean_limits_and_default(tmp: Path) -> None:
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
    (dry / "orphan.dca").write_bytes(b"old")
    image_converter.main(["--clean", str(dry)])
    expect("clean removes orphan dci", not (dry / "orphan.dci").exists())
    expect("clean removes orphan dca", not (dry / "orphan.dca").exists())

    tiny = tmp / "tiny"
    tiny.mkdir()
    noisy = Image.new("RGB", (image_converter.CANVAS_W, image_converter.CANVAS_H), (0, 0, 0))
    noisy_pixels = noisy.load()
    for y in range(image_converter.CANVAS_H):
        for x in range(image_converter.CANVAS_W):
            noisy_pixels[x, y] = ((x * 7 + y * 3) & 255, (x * 5 + y * 11) & 255, (x * 13 + y * 17) & 255)
    oversized = tiny / "too_big.gif"
    noisy.save(oversized, save_all=True, append_images=[noisy.transpose(Image.Transpose.FLIP_LEFT_RIGHT)], duration=[33, 33], loop=0)
    valid_first = Image.new("RGB", (8, 8), (255, 0, 0))
    valid_second = Image.new("RGB", (8, 8), (0, 255, 0))
    valid = tiny / "z_valid.gif"
    valid_first.save(valid, save_all=True, append_images=[valid_second], duration=[33, 33], loop=0)
    rc = image_converter.main(["--max-bytes", "4096", str(tiny)])
    expect("oversized DCA returns failure", rc == 1)
    expect("oversized DCA skipped output", not oversized.with_suffix(".dca").exists())
    expect("later valid animation still converts", valid.with_suffix(".dca").exists())
    expect("failed conversion removes tmp", not list(tiny.glob("*.tmp")))

    auto = tmp / "auto"
    auto.mkdir()
    frames = []
    for frame_idx in range(8):
        frame = Image.new("RGB", (120, 90), (0, 0, 0))
        pixels = frame.load()
        for idx in range(300):
            x = (idx * 37 + frame_idx * 5) % 120
            y = (idx * 19 + frame_idx * 3) % 90
            green = 20 + ((idx * 13 + frame_idx * 17) % 45)
            pixels[x, y] = (0, green, 0)
        frames.append(frame)
    auto_anim = auto / "auto_reduce.gif"
    frames[0].save(auto_anim, save_all=True, append_images=frames[1:], duration=[33] * len(frames), loop=0)
    rc = image_converter.main(["--max-bytes", "85000", str(auto)])
    expect("auto palette reduction succeeds", rc == 0)
    expect("auto reduced DCA under max", auto_anim.with_suffix(".dca").stat().st_size <= 85000)

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
        test_animated_gif_to_dca(tmp / "animated")
        test_collision_dry_run_clean_limits_and_default(tmp / "misc")
    print("image converter tests passed")


if __name__ == "__main__":
    main()
