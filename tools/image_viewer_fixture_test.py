#!/usr/bin/env python3
import struct
from pathlib import Path


DCI_MAGIC = 0x31494344
DCI_VERSION = 1
DCI_KIND_STATIC = 1
DCI_KIND_ANIM = 2
DCI_BPP_RGB565 = 16
DCI_W = 320
DCI_H = 240
DCI_FRAME_BYTES = DCI_W * DCI_H * 2
DCI_HEADER_FORMAT = "<IHBBHHBBHIII9I"
DCI_FRAME_FORMAT = "<II"
DCI_HEADER_SIZE = struct.calcsize(DCI_HEADER_FORMAT)
DCI_FRAME_HEADER_SIZE = struct.calcsize(DCI_FRAME_FORMAT)


def pack_header(kind, frame_count, payload_bytes, *, magic=DCI_MAGIC, version=DCI_VERSION, flags=0, width=DCI_W, height=DCI_H, bpp=DCI_BPP_RGB565):
    return struct.pack(
        DCI_HEADER_FORMAT,
        magic,
        DCI_HEADER_SIZE,
        version,
        kind,
        width,
        height,
        bpp,
        flags,
        0,
        frame_count,
        0,
        payload_bytes,
        *([0] * 9),
    )


def valid_dci1_static(data):
    if len(data) < DCI_HEADER_SIZE:
        return False
    fields = struct.unpack_from(DCI_HEADER_FORMAT, data)
    magic, header_size, version, kind, width, height, bpp, flags = fields[:8]
    frame_count = fields[9]
    payload_bytes = fields[11]
    return (
        magic == DCI_MAGIC
        and header_size == DCI_HEADER_SIZE
        and version == DCI_VERSION
        and kind == DCI_KIND_STATIC
        and width == DCI_W
        and height == DCI_H
        and bpp == DCI_BPP_RGB565
        and flags == 0
        and frame_count == 1
        and payload_bytes == DCI_FRAME_BYTES
        and len(data) == DCI_HEADER_SIZE + DCI_FRAME_BYTES
    )


def expect(name, condition):
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def main():
    repo = Path(__file__).resolve().parents[1]
    static_payload = bytes(DCI_FRAME_BYTES)
    static = pack_header(DCI_KIND_STATIC, 1, len(static_payload)) + static_payload
    old_anim = pack_header(DCI_KIND_ANIM, 1, DCI_FRAME_HEADER_SIZE + DCI_FRAME_BYTES) + struct.pack(DCI_FRAME_FORMAT, 10, DCI_FRAME_BYTES) + bytes(DCI_FRAME_BYTES)
    bad_magic = pack_header(DCI_KIND_STATIC, 1, DCI_FRAME_BYTES, magic=0) + static_payload
    bad_dims = pack_header(DCI_KIND_STATIC, 1, DCI_FRAME_BYTES, width=319) + static_payload
    bad_bpp = pack_header(DCI_KIND_STATIC, 1, DCI_FRAME_BYTES, bpp=24) + static_payload
    truncated = static[:-1]

    expect("header size is stable", DCI_HEADER_SIZE == 64)
    expect("frame header size is stable", DCI_FRAME_HEADER_SIZE == 8)
    expect("valid static DCI1", valid_dci1_static(static))
    expect("reject animated DCI1", not valid_dci1_static(old_anim))
    expect("reject bad magic", not valid_dci1_static(bad_magic))
    expect("reject wrong dimensions", not valid_dci1_static(bad_dims))
    expect("reject unsupported bpp", not valid_dci1_static(bad_bpp))
    expect("reject truncated payload", not valid_dci1_static(truncated))

    image_viewer_src = (repo / "src" / "imageViewer.c").read_text(encoding="utf-8")
    gif_bridge_src = (repo / "src" / "imageViewerAnimatedGif.cpp").read_text(encoding="utf-8")
    disp_header = (repo / "src" / "dispDefcon.h").read_text(encoding="utf-8")
    ui_src = (repo / "src" / "ui.c").read_text(encoding="utf-8")
    expect("image viewer accepts DCI", '".dci"' in image_viewer_src)
    expect("image viewer accepts GIF", '".gif"' in image_viewer_src)
    expect("AnimatedGIF bridge is used", "AnimatedGIF" in gif_bridge_src)
    expect("DCI stills hold scanout", "dispHoldScanoutBegin" in image_viewer_src and "dispHoldScanoutEnd" in image_viewer_src)
    expect("display exposes bounded hold helper", "dispHoldScanoutBegin" in disp_header and "dispHoldScanoutEnd" in disp_header)
    expect("badge GIF sidecars are recognized", '".badge.gif"' in ui_src)
    expect("image picker hides source GIF when sidecar exists", "uiPrvImageFileVisibleInDir" in ui_src and "fatfsFindFileAt" in ui_src)
    expect("no DCI2 playback remains", "DCI2" not in image_viewer_src)
    expect("image viewer has no direct display calls", "dispDirect" not in image_viewer_src + gif_bridge_src + disp_header)
    expect("legacy gifdec hooks removed", "imageViewerGifOpen" not in image_viewer_src)

    print("image viewer fixture tests passed")


if __name__ == "__main__":
    main()
