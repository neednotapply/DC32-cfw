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
DCI_HEADER_SIZE = struct.calcsize(DCI_HEADER_FORMAT)

DCA_MAGIC = 0x31414344
DCA_VERSION = 1
DCA_BPP_INDEXED = 8
DCA_HEADER_FORMAT = "<IHBBHHHHIIIIII6I"
DCA_FRAME_FORMAT = "<IHHII"
DCA_RECT_FORMAT = "<HHHHBBIH"
DCA_HEADER_SIZE = struct.calcsize(DCA_HEADER_FORMAT)
DCA_FRAME_SIZE = struct.calcsize(DCA_FRAME_FORMAT)
DCA_RECT_SIZE = struct.calcsize(DCA_RECT_FORMAT)


def pack_dci_header(kind, frame_count, payload_bytes, *, magic=DCI_MAGIC, version=DCI_VERSION, flags=0, width=DCI_W, height=DCI_H, bpp=DCI_BPP_RGB565):
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


def pack_dca(frame_payload: bytes, *, magic=DCA_MAGIC, width=DCI_W, height=DCI_H, bpp=DCA_BPP_INDEXED, frame_count=1, palette_count=2):
    palette_offset = DCA_HEADER_SIZE
    palette = bytes(palette_count * 2)
    frame_table_offset = palette_offset + len(palette)
    data_offset = frame_table_offset + frame_count * DCA_FRAME_SIZE
    payload_bytes = len(palette) + frame_count * DCA_FRAME_SIZE + len(frame_payload)
    header = struct.pack(
        DCA_HEADER_FORMAT,
        magic,
        DCA_HEADER_SIZE,
        DCA_VERSION,
        bpp,
        width,
        height,
        frame_count,
        palette_count,
        0,
        payload_bytes,
        frame_table_offset,
        palette_offset,
        data_offset,
        0,
        *([0] * 6),
    )
    frame = struct.pack(DCA_FRAME_FORMAT, 33, 1, 0, data_offset, len(frame_payload))
    return header + palette + frame + frame_payload


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


def valid_dca1(data):
    if len(data) < DCA_HEADER_SIZE:
        return False
    fields = struct.unpack_from(DCA_HEADER_FORMAT, data)
    magic, header_size, version, bpp, width, height, frame_count, palette_count = fields[:8]
    payload_bytes, frame_table_offset, palette_offset, data_offset, flags = fields[9:14]
    if (
        magic != DCA_MAGIC
        or header_size != DCA_HEADER_SIZE
        or version != DCA_VERSION
        or bpp != DCA_BPP_INDEXED
        or width != DCI_W
        or height != DCI_H
        or frame_count < 1
        or palette_count < 1
        or palette_count > 256
        or flags != 0
        or len(data) != DCA_HEADER_SIZE + payload_bytes
    ):
        return False
    if palette_offset < DCA_HEADER_SIZE or frame_table_offset < DCA_HEADER_SIZE:
        return False
    if palette_offset + palette_count * 2 > len(data):
        return False
    if frame_table_offset + frame_count * DCA_FRAME_SIZE > len(data):
        return False
    if data_offset < DCA_HEADER_SIZE or data_offset > len(data):
        return False
    delay_ms, rect_count, frame_flags, frame_data_offset, frame_data_bytes = struct.unpack_from(DCA_FRAME_FORMAT, data, frame_table_offset)
    return delay_ms == 33 and rect_count == 1 and frame_flags == 0 and frame_data_offset + frame_data_bytes <= len(data)


def expect(name, condition):
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def dci_copy(src, flipped=False):
    return list(reversed(src)) if flipped else list(src)


def main():
    repo = Path(__file__).resolve().parents[1]
    static_payload = bytes(DCI_FRAME_BYTES)
    static = pack_dci_header(DCI_KIND_STATIC, 1, len(static_payload)) + static_payload
    old_anim = pack_dci_header(DCI_KIND_ANIM, 1, 8 + DCI_FRAME_BYTES) + struct.pack("<II", 10, DCI_FRAME_BYTES) + bytes(DCI_FRAME_BYTES)
    bad_magic = pack_dci_header(DCI_KIND_STATIC, 1, DCI_FRAME_BYTES, magic=0) + static_payload
    bad_dims = pack_dci_header(DCI_KIND_STATIC, 1, DCI_FRAME_BYTES, width=319) + static_payload
    bad_bpp = pack_dci_header(DCI_KIND_STATIC, 1, DCI_FRAME_BYTES, bpp=24) + static_payload
    truncated = static[:-1]

    rect_payload = struct.pack(DCA_RECT_FORMAT, 0, 0, 1, 1, 0, 0, 1, 0) + b"\x01"
    dca = pack_dca(rect_payload)

    expect("DCI header size is stable", DCI_HEADER_SIZE == 64)
    expect("DCA header size is stable", DCA_HEADER_SIZE == 64)
    expect("DCA frame size is stable", DCA_FRAME_SIZE == 16)
    expect("DCA rect size is stable", DCA_RECT_SIZE == 16)
    expect("valid static DCI1", valid_dci1_static(static))
    expect("reject animated DCI1", not valid_dci1_static(old_anim))
    expect("reject bad magic", not valid_dci1_static(bad_magic))
    expect("reject wrong dimensions", not valid_dci1_static(bad_dims))
    expect("reject unsupported bpp", not valid_dci1_static(bad_bpp))
    expect("reject truncated payload", not valid_dci1_static(truncated))
    expect("valid DCA1", valid_dca1(dca))
    expect("reject bad DCA magic", not valid_dca1(pack_dca(rect_payload, magic=0)))
    expect("reject bad DCA dimensions", not valid_dca1(pack_dca(rect_payload, width=319)))
    expect("reject bad DCA bpp", not valid_dca1(pack_dca(rect_payload, bpp=16)))
    expect("reject truncated DCA", not valid_dca1(dca[:-1]))
    expect("normal DCI copy preserves order", dci_copy([1, 2, 3, 4]) == [1, 2, 3, 4])
    expect("flipped DCI copy reverses order", dci_copy([1, 2, 3, 4], flipped=True) == [4, 3, 2, 1])

    image_viewer_src = (repo / "src" / "imageViewer.c").read_text(encoding="utf-8")
    cmake_src = (repo / "CMakeLists.txt").read_text(encoding="utf-8")
    disp_header = (repo / "src" / "dispDefcon.h").read_text(encoding="utf-8")
    ui_src = (repo / "src" / "ui.c").read_text(encoding="utf-8")
    expect("image viewer accepts DCI", '".dci"' in image_viewer_src)
    expect("image viewer accepts DCA", '".dca"' in image_viewer_src)
    expect("image viewer rejects GIF", '".gif"' not in image_viewer_src)
    expect("AnimatedGIF bridge is not built", "AnimatedGIF" not in cmake_src and "imageViewerAnimatedGif" not in cmake_src)
    expect("DCI path no longer holds scanout", "dispHoldScanout" not in image_viewer_src)
    expect("display hold helper removed from public header", "dispHoldScanout" not in disp_header)
    expect("image viewer preloads through QSPI scratch", "QSPI_ROM_START" in image_viewer_src and "flashWrite" in image_viewer_src)
    expect("safe SD fallback exists", "IMAGE_SD_FAST_HZ" in image_viewer_src and "IMAGE_SD_SAFE_HZ" in image_viewer_src)
    expect("slow DCA loads draw loading text", "IMAGE_LOADING_DELAY_TICKS" in image_viewer_src and '"Loading..."' in image_viewer_src)
    expect("loading text is DCA-only", "imagePrvFlashLoadRange(fil, DCI_HEADER_SIZE, DCI_FRAME_BYTES, NULL)" in image_viewer_src and "imagePrvFlashLoadRange(fil, 0, fileSize, cnv)" in image_viewer_src)
    expect("DCI flipped copy reverses staged pixels", "dst[i] = src[DCI_FRAME_BYTES / sizeof(uint16_t) - 1u - i]" in image_viewer_src)
    expect("DCA fast path handles flipped", "cnv->w - 1u - rect->x" in image_viewer_src and "dst += cnv->flipped ? -1 : 1" in image_viewer_src)
    expect("image sequence reads rotation setting", "settingsGet(&settings)" in ui_src and "viewerCanvas.flipped = settings.rotation" in ui_src)
    expect("image sequence passes viewer canvas", "imageViewerRun(&viewerCanvas" in ui_src)
    expect("image adjacent navigation uses picker sort order", "uiPrvFindAdjacentImage" in ui_src and "strsCaselesslyCompareUtf(fname, curName" in ui_src and "candidateName" in ui_src)
    expect("caseless UTF compare stops at nul", "if (!ac && !bc)" in ui_src and "return 0;" in ui_src)
    expect("image menu waits for opener release", "uiPrvWaitKeysReleased();" in ui_src and "action = uiPrvImageViewerMenu(cnv)" in ui_src)
    expect("image picker text names only DCI and DCA", "No .dci or .dca files found in /IMAGES" in ui_src)
    expect("badge GIF sidecars removed from picker", ".badge.gif" not in ui_src)
    expect("no DCI2 playback remains", "DCI2" not in image_viewer_src)
    expect("image viewer has no direct display calls", "dispDirect" not in image_viewer_src + disp_header)
    expect("legacy gifdec hooks removed", "imageViewerGifOpen" not in image_viewer_src)

    print("image viewer fixture tests passed")


if __name__ == "__main__":
    main()
