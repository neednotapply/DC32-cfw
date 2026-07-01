#include "dcAppDraw.h"

#include <string.h>
#include "dispDefcon.h"
#include "ui.h"

static int32_t dcAppDrawPrvAbsI32(int32_t v)
{
	return v < 0 ? -v : v;
}

static uint8_t dcAppDrawPrvRgb565ToRgb332(uint16_t color)
{
	return (uint8_t)((color >> 8) & 0xe0u) |
		(uint8_t)((color >> 6) & 0x1cu) |
		(uint8_t)((color >> 3) & 0x03u);
}

static uint16_t dcAppDrawPrvRgb332ToRgb565(uint8_t color)
{
	uint32_t r = color >> 5;
	uint32_t g = (color >> 2) & 7u;
	uint32_t b = color & 3u;

	r = (r << 2) | (r >> 1);
	g = (g << 3) | g;
	b = (b << 3) | (b << 1) | (b >> 1);
	return (uint16_t)((r << 11) | (g << 5) | b);
}

static uint32_t dcAppDrawPrvDisplayIndex(const struct Canvas *cnv, uint32_t x, uint32_t y)
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

uint16_t dcAppDrawRgb565(uint32_t r, uint32_t g, uint32_t b)
{
	return (uint16_t)((r & 0xf8u) << 8) | (uint16_t)((g & 0xfcu) << 3) | (uint16_t)(b >> 3);
}

bool dcAppDrawInit(struct DcAppDrawCtx *ctx, const struct DcAppHostApi *host, const struct DcAppRunArgs *args, void *backbuffer, uint32_t w, uint32_t h)
{
	if (!ctx || !backbuffer || !w || !h)
		return false;

	memset(ctx, 0, sizeof(*ctx));
	ctx->host = host;
	ctx->fb = (uint8_t*)backbuffer;
	ctx->w = w;
	ctx->h = h;
	if (args && args->canvas)
		ctx->displayCnv = *args->canvas;
	if (!ctx->displayCnv.framebuffer && host && host->displayFb)
		ctx->displayCnv.framebuffer = host->displayFb();
	if (!ctx->displayCnv.w)
		ctx->displayCnv.w = w;
	if (!ctx->displayCnv.h)
		ctx->displayCnv.h = h;
	if (!ctx->displayCnv.bpp)
		ctx->displayCnv.bpp = 16u;
	if (args && args->rotate)
		ctx->displayCnv.flipped = 1u;
	dcAppDrawClear(ctx, 0);
	ctx->prevKeys = host && host->uiKeysRaw ? host->uiKeysRaw() : uiGetUiKeysRaw();
	return true;
}

void dcAppDrawClear(struct DcAppDrawCtx *ctx, uint16_t color)
{
	if (ctx && ctx->fb)
		memset(ctx->fb, dcAppDrawPrvRgb565ToRgb332(color), ctx->w * ctx->h);
}

void dcAppDrawFill(struct DcAppDrawCtx *ctx, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
	uint8_t px;

	if (!ctx || !ctx->fb)
		return;
	px = dcAppDrawPrvRgb565ToRgb332(color);
	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	}
	if (x >= (int32_t)ctx->w || y >= (int32_t)ctx->h || w <= 0 || h <= 0)
		return;
	if (x + w > (int32_t)ctx->w)
		w = (int32_t)ctx->w - x;
	if (y + h > (int32_t)ctx->h)
		h = (int32_t)ctx->h - y;
	while (h--) {
		memset(ctx->fb + (uint32_t)y * ctx->w + (uint32_t)x, px, (uint32_t)w);
		y++;
	}
}

void dcAppDrawPixel(struct DcAppDrawCtx *ctx, int32_t x, int32_t y, uint16_t color)
{
	if (ctx && ctx->fb && x >= 0 && y >= 0 && x < (int32_t)ctx->w && y < (int32_t)ctx->h)
		ctx->fb[(uint32_t)y * ctx->w + (uint32_t)x] = dcAppDrawPrvRgb565ToRgb332(color);
}

void dcAppDrawLine(struct DcAppDrawCtx *ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
{
	int32_t dx = dcAppDrawPrvAbsI32(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int32_t dy = -dcAppDrawPrvAbsI32(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int32_t err = dx + dy;

	while (1) {
		int32_t e2;

		dcAppDrawPixel(ctx, x0, y0, color);
		if (x0 == x1 && y0 == y1)
			break;
		e2 = 2 * err;
		if (e2 >= dy) {
			err += dy;
			x0 += sx;
		}
		if (e2 <= dx) {
			err += dx;
			y0 += sy;
		}
	}
}

void dcAppDrawPresent(struct DcAppDrawCtx *ctx)
{
	const struct Canvas *cnv;
	uint16_t *dst;

	if (!ctx || !ctx->fb)
		return;
	cnv = &ctx->displayCnv;
	dst = (uint16_t*)cnv->framebuffer;
	if (!dst || cnv->bpp != 16u || cnv->w < ctx->w || cnv->h < ctx->h)
		return;
	if (cnv->w == ctx->w && cnv->h == ctx->h && cnv->rotated && !cnv->flipped) {
		for (uint32_t x = 0; x < ctx->w; x++) {
			uint16_t *out = dst + x * ctx->h;

			for (int32_t y = (int32_t)ctx->h - 1; y >= 0; y--)
				*out++ = dcAppDrawPrvRgb332ToRgb565(ctx->fb[(uint32_t)y * ctx->w + x]);
		}
		return;
	}
	if (!cnv->rotated && !cnv->flipped) {
		uint32_t count = ctx->w * ctx->h;

		for (uint32_t i = 0; i < count; i++)
			dst[i] = dcAppDrawPrvRgb332ToRgb565(ctx->fb[i]);
		return;
	}
	for (uint32_t y = 0; y < ctx->h; y++)
		for (uint32_t x = 0; x < ctx->w; x++)
			dst[dcAppDrawPrvDisplayIndex(cnv, x, y)] = dcAppDrawPrvRgb332ToRgb565(ctx->fb[y * ctx->w + x]);
}

bool dcAppDrawFrame(struct DcAppDrawCtx *ctx, uint_fast16_t exitMask)
{
	if (!ctx)
		return false;
	dispPrvFrameCtrWait();
	if (ctx->host && ctx->host->ledsTick)
		ctx->host->ledsTick();
	dispPrvWaitForScanoutStart();
	dcAppDrawPresent(ctx);
	ctx->keys = ctx->host && ctx->host->uiKeysRaw ? ctx->host->uiKeysRaw() : uiGetUiKeysRaw();
	ctx->pressed = ctx->keys & ~ctx->prevKeys;
	ctx->prevKeys = ctx->keys;
	ctx->frame++;
	if ((ctx->pressed & UI_KEY_BIT_CENTER) && ctx->host && ctx->host->portMenu) {
		bool resume = ctx->host->portMenu(&ctx->displayCnv);

		ctx->keys = ctx->host->uiKeysRaw ? ctx->host->uiKeysRaw() : 0;
		ctx->prevKeys = ctx->keys;
		ctx->pressed = 0;
		return resume;
	}
	return (ctx->pressed & exitMask) == 0;
}

void dcAppDrawWaitRelease(struct DcAppDrawCtx *ctx, uint_fast16_t mask)
{
	if (!ctx || !ctx->host || !ctx->host->uiKeysRaw || !ctx->host->delayMsec)
		return;
	while (ctx->host->uiKeysRaw() & mask)
		ctx->host->delayMsec(10);
}
