#include "raster.h"
#include "dispDefcon.h"

uint16_t rasterRgb565(uint32_t r, uint32_t g, uint32_t b)
{
	return (uint16_t)((r & 0xf8u) << 8) | (uint16_t)((g & 0xfcu) << 3) | (uint16_t)(b >> 3);
}

uint16_t rasterRgba8888OverBlack(const uint8_t rgba[4])
{
	uint32_t a = rgba[3];

	if (a != 255u) {
		return rasterRgb565((rgba[0] * a + 127u) / 255u,
		                    (rgba[1] * a + 127u) / 255u,
		                    (rgba[2] * a + 127u) / 255u);
	}
	return rasterRgb565(rgba[0], rgba[1], rgba[2]);
}

struct RasterRect rasterFit(uint32_t srcW, uint32_t srcH, uint32_t dstW, uint32_t dstH)
{
	struct RasterRect rect = {0};
	uint32_t wByH, hByW;

	if (!srcW || !srcH || !dstW || !dstH)
		return rect;

	wByH = dstH * srcW / srcH;
	if (wByH <= dstW) {
		rect.w = (uint16_t)wByH;
		rect.h = (uint16_t)dstH;
	}
	else {
		hByW = dstW * srcH / srcW;
		rect.w = (uint16_t)dstW;
		rect.h = (uint16_t)hByW;
	}
	if (!rect.w)
		rect.w = 1;
	if (!rect.h)
		rect.h = 1;
	rect.x = (uint16_t)((dstW - rect.w) / 2u);
	rect.y = (uint16_t)((dstH - rect.h) / 2u);
	return rect;
}

static uint32_t rasterPrvIndex(const struct Canvas *cnv, uint32_t x, uint32_t y)
{
	uint32_t rowItems = cnv->rotated ? cnv->h : cnv->w;

	if (cnv->flipped) {
		x = cnv->w - 1u - x;
		y = cnv->h - 1u - y;
	}
	if (cnv->rotated)
		return x * rowItems + (cnv->h - 1u - y);
	return y * rowItems + x;
}

void rasterPutPixel(const struct Canvas *cnv, uint32_t x, uint32_t y, uint16_t color)
{
	uint16_t *fb = (uint16_t*)cnv->framebuffer;

	if (!fb || x >= cnv->w || y >= cnv->h)
		return;
	fb[rasterPrvIndex(cnv, x, y)] = color;
}

void rasterClear(const struct Canvas *cnv, uint16_t color)
{
	struct RasterRect rect = {.x = 0, .y = 0, .w = (uint16_t)cnv->w, .h = (uint16_t)cnv->h};

	rasterFillRect(cnv, &rect, color);
}

void rasterFillRect(const struct Canvas *cnv, const struct RasterRect *rect, uint16_t color)
{
	uint32_t x0, y0, x1, y1, x, y;

	if (!rect || !rect->w || !rect->h)
		return;
	x0 = rect->x;
	y0 = rect->y;
	x1 = x0 + rect->w;
	y1 = y0 + rect->h;
	if (x1 > cnv->w)
		x1 = cnv->w;
	if (y1 > cnv->h)
		y1 = cnv->h;

	for (y = y0; y < y1; y++)
		for (x = x0; x < x1; x++)
			rasterPutPixel(cnv, x, y, color);
}

void rasterDrawScaledPixel(const struct Canvas *cnv, const struct RasterRect *fit, uint32_t srcW, uint32_t srcH, uint32_t srcX, uint32_t srcY, uint16_t color)
{
	struct RasterRect rect;
	uint32_t x0, x1, y0, y1;

	if (!fit || !fit->w || !fit->h || !srcW || !srcH)
		return;

	x0 = fit->x + (srcX * fit->w) / srcW;
	x1 = fit->x + ((srcX + 1u) * fit->w + srcW - 1u) / srcW;
	y0 = fit->y + (srcY * fit->h) / srcH;
	y1 = fit->y + ((srcY + 1u) * fit->h + srcH - 1u) / srcH;
	if (x1 <= x0)
		x1 = x0 + 1u;
	if (y1 <= y0)
		y1 = y0 + 1u;

	rect.x = (uint16_t)x0;
	rect.y = (uint16_t)y0;
	rect.w = (uint16_t)(x1 - x0);
	rect.h = (uint16_t)(y1 - y0);
	rasterFillRect(cnv, &rect, color);
}
