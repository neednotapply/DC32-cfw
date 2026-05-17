#ifndef _RASTER_H_
#define _RASTER_H_

#include <stdbool.h>
#include <stdint.h>
#include "ui.h"

#define RASTER_LOGICAL_WIDTH  320u
#define RASTER_LOGICAL_HEIGHT 240u

struct RasterRect {
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
};

uint16_t rasterRgb565(uint32_t r, uint32_t g, uint32_t b);
uint16_t rasterRgba8888OverBlack(const uint8_t rgba[4]);
struct RasterRect rasterFit(uint32_t srcW, uint32_t srcH, uint32_t dstW, uint32_t dstH);
void rasterClear(const struct Canvas *cnv, uint16_t color);
void rasterFillRect(const struct Canvas *cnv, const struct RasterRect *rect, uint16_t color);
void rasterPutPixel(const struct Canvas *cnv, uint32_t x, uint32_t y, uint16_t color);
void rasterDrawScaledPixel(const struct Canvas *cnv, const struct RasterRect *fit, uint32_t srcW, uint32_t srcH, uint32_t srcX, uint32_t srcY, uint16_t color);

#endif
