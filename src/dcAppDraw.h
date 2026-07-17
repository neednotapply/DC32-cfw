#ifndef DC_APP_DRAW_H
#define DC_APP_DRAW_H

#include <stdbool.h>
#include <stdint.h>
#include "dcApp.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DcAppDrawCtx {
	const struct DcAppHostApi *host;
	struct Canvas displayCnv;
	uint8_t *fb;
	uint32_t w;
	uint32_t h;
	uint_fast16_t keys;
	uint_fast16_t prevKeys;
	uint_fast16_t pressed;
	uint32_t frame;
	struct FatfsVol *vol;
};

/*
 * The common loading surface for DCAPPs and built-in tools.  App-specific
 * stages and detail strings remain available, but the placement, fonts,
 * colours, and progress treatment are intentionally shared.
 */
struct DcAppLoadingState {
	const char *appName;
	const char *iconId;
	struct FatfsVol *iconVol;
	const char *title;
	const char *detail;
	const char *hint;
	uint32_t done;
	uint32_t total;
	uint32_t animationStep;
};

uint16_t dcAppDrawRgb565(uint32_t r, uint32_t g, uint32_t b);
void dcAppDrawSetThemeColor(uint8_t red, uint8_t green, uint8_t blue);
bool dcAppDrawInit(struct DcAppDrawCtx *ctx, const struct DcAppHostApi *host, const struct DcAppRunArgs *args, void *backbuffer, uint32_t w, uint32_t h);
void dcAppDrawClear(struct DcAppDrawCtx *ctx, uint16_t color);
void dcAppDrawFill(struct DcAppDrawCtx *ctx, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
void dcAppDrawPixel(struct DcAppDrawCtx *ctx, int32_t x, int32_t y, uint16_t color);
void dcAppDrawLine(struct DcAppDrawCtx *ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color);
/* Draw a DCEI RGBA8888 asset from /ICONS/<iconId>/<size>.dcei.  Missing or
 * invalid assets return false and leave the caller's UI usable. */
bool dcAppDrawIcon(struct DcAppDrawCtx *ctx, const char *iconId, uint16_t size,
	int32_t x, int32_t y, uint16_t background);
bool dcAppDrawIconCanvas(const struct Canvas *cnv, struct FatfsVol *vol,
	const char *iconId, uint16_t size, int32_t x, int32_t y, uint16_t background);
void dcAppDrawPresent(struct DcAppDrawCtx *ctx);
bool dcAppDrawFrame(struct DcAppDrawCtx *ctx, uint_fast16_t exitMask);
void dcAppDrawWaitRelease(struct DcAppDrawCtx *ctx, uint_fast16_t mask);
void dcAppDrawLoading(struct DcAppDrawCtx *ctx, const struct DcAppLoadingState *state);
void dcAppDrawLoadingCanvas(const struct Canvas *cnv, const struct DcAppLoadingState *state);

#ifdef __cplusplus
}
#endif

#endif
