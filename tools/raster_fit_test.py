#!/usr/bin/env python3

def raster_fit(src_w, src_h, dst_w=320, dst_h=240):
    if not src_w or not src_h or not dst_w or not dst_h:
        return (0, 0, 0, 0)

    w_by_h = dst_h * src_w // src_h
    if w_by_h <= dst_w:
        w = w_by_h
        h = dst_h
    else:
        h = dst_w * src_h // src_w
        w = dst_w

    if not w:
        w = 1
    if not h:
        h = 1
    return ((dst_w - w) // 2, (dst_h - h) // 2, w, h)


def expect(name, got, want):
    if got != want:
        raise SystemExit(f"{name}: got {got}, want {want}")


def main():
    expect("exact", raster_fit(320, 240), (0, 0, 320, 240))
    expect("landscape", raster_fit(640, 240), (0, 60, 320, 120))
    expect("portrait", raster_fit(240, 640), (115, 0, 90, 240))
    expect("square", raster_fit(100, 100), (40, 0, 240, 240))
    expect("tiny upscale", raster_fit(1, 1), (40, 0, 240, 240))
    expect("large downscale", raster_fit(2048, 1024), (0, 40, 320, 160))
    print("raster fit tests passed")


if __name__ == "__main__":
    main()
