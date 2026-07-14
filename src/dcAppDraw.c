#include "dcAppDraw.h"

#include <string.h>
#include "dispDefcon.h"
#include "fonts.h"
#include "ui.h"

static uint16_t mDcAppDrawRgb332To565[256];
static bool mDcAppDrawPaletteReady;

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

static void dcAppDrawPrvInitPalette(void)
{
	if (mDcAppDrawPaletteReady)
		return;
	for (uint32_t i = 0; i < 256u; i++)
		mDcAppDrawRgb332To565[i] = dcAppDrawPrvRgb332ToRgb565((uint8_t)i);
	mDcAppDrawPaletteReady = true;
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

static uint32_t dcAppDrawPrvTextWidth(const char *text, enum Font font)
{
	uint32_t width = 0;

	while (text && *text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text))
			width += glyph.width + 1u;
		text++;
	}
	return width ? width - 1u : 0u;
}

static void dcAppDrawPrvCanvasPixel(const struct Canvas *cnv, int32_t x, int32_t y,
	uint16_t color)
{
	uint16_t *fb;

	if (!cnv || !cnv->framebuffer || cnv->bpp != 16u || x < 0 || y < 0 ||
		x >= (int32_t)cnv->w || y >= (int32_t)cnv->h)
		return;
	fb = (uint16_t*)cnv->framebuffer;
	fb[dcAppDrawPrvDisplayIndex(cnv, (uint32_t)x, (uint32_t)y)] = color;
}

static void dcAppDrawPrvCanvasFill(const struct Canvas *cnv, int32_t x, int32_t y,
	int32_t w, int32_t h, uint16_t color)
{
	if (cnv && cnv->framebuffer && cnv->bpp == 16u && x == 0 && y == 0 &&
		w == (int32_t)cnv->w && h == (int32_t)cnv->h) {
		uint16_t *fb = (uint16_t*)cnv->framebuffer;

		for (uint32_t i = 0; i < cnv->w * cnv->h; i++)
			fb[i] = color;
		return;
	}
	for (int32_t yy = y; yy < y + h; yy++)
		for (int32_t xx = x; xx < x + w; xx++)
			dcAppDrawPrvCanvasPixel(cnv, xx, yy, color);
}

static void dcAppDrawPrvCanvasText(const struct Canvas *cnv, int32_t x, int32_t y,
	const char *text, enum Font font, uint16_t color)
{
	while (text && *text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text)) {
			for (uint_fast8_t row = 0; row < glyph.height; row++)
				for (uint_fast8_t col = 0; col < glyph.width; col++)
					if (fontGetGlyphPixel(&glyph, row, col))
						dcAppDrawPrvCanvasPixel(cnv, x + col, y + row, color);
			x += glyph.width + 1;
		}
		text++;
	}
}

static void dcAppDrawPrvCanvasCentered(const struct Canvas *cnv, int32_t y,
	const char *text, enum Font font, uint16_t color)
{
	uint32_t width = dcAppDrawPrvTextWidth(text, font);
	uint32_t screenWidth = cnv ? cnv->w : 0;

	if (width > screenWidth)
		width = screenWidth;
	dcAppDrawPrvCanvasText(cnv, (int32_t)(screenWidth - width) / 2, y, text, font, color);
}

static void dcAppDrawPrvCtxText(struct DcAppDrawCtx *ctx, int32_t x, int32_t y,
	const char *text, enum Font font, uint16_t color)
{
	while (text && *text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text)) {
			for (uint_fast8_t row = 0; row < glyph.height; row++)
				for (uint_fast8_t col = 0; col < glyph.width; col++)
					if (fontGetGlyphPixel(&glyph, row, col))
						dcAppDrawPixel(ctx, x + col, y + row, color);
			x += glyph.width + 1;
		}
		text++;
	}
}

static void dcAppDrawPrvCtxCentered(struct DcAppDrawCtx *ctx, int32_t y,
	const char *text, enum Font font, uint16_t color)
{
	uint32_t width = dcAppDrawPrvTextWidth(text, font);

	if (width > ctx->w)
		width = ctx->w;
	dcAppDrawPrvCtxText(ctx, (int32_t)(ctx->w - width) / 2, y, text, font, color);
}

static uint32_t dcAppDrawPrvLoadingFill(uint32_t width,
	const struct DcAppLoadingState *state)
{
	uint32_t fill;

	if (state && state->total) {
		fill = (uint32_t)(((uint64_t)width * state->done) / state->total);
		return fill > width ? width : fill;
	}
	return width / 4u;
}

static uint32_t dcAppDrawPrvLoadingOffset(uint32_t width,
	const struct DcAppLoadingState *state, uint32_t fill)
{
	uint32_t travel;

	if (state && state->total)
		return 0;
	travel = width > fill ? width - fill : 0;
	return travel ? ((state ? state->animationStep : 0u) * 7u) % (travel + 1u) : 0u;
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
	dcAppDrawPrvInitPalette();
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
				*out++ = mDcAppDrawRgb332To565[ctx->fb[(uint32_t)y * ctx->w + x]];
		}
		return;
	}
	if (!cnv->rotated && !cnv->flipped) {
		uint32_t count = ctx->w * ctx->h;

		for (uint32_t i = 0; i < count; i++)
			dst[i] = mDcAppDrawRgb332To565[ctx->fb[i]];
		return;
	}
	for (uint32_t y = 0; y < ctx->h; y++)
		for (uint32_t x = 0; x < ctx->w; x++)
			dst[dcAppDrawPrvDisplayIndex(cnv, x, y)] =
				mDcAppDrawRgb332To565[ctx->fb[y * ctx->w + x]];
}

void dcAppDrawLoading(struct DcAppDrawCtx *ctx, const struct DcAppLoadingState *state)
{
	uint16_t bg = dcAppDrawRgb565(12, 12, 12);
	uint16_t primary = dcAppDrawRgb565(244, 244, 244);
	uint16_t secondary = dcAppDrawRgb565(188, 188, 188);
	uint16_t track = dcAppDrawRgb565(48, 48, 48);
	uint16_t accent = dcAppDrawRgb565(168, 168, 168);
	uint32_t barW, fill, offset;
	int32_t centerY, barX, barY;

	if (!ctx || !ctx->fb || !ctx->w || !ctx->h)
		return;
	barW = ctx->w > 64u ? ctx->w - 64u : ctx->w;
	if (barW > 280u)
		barW = 280u;
	fill = dcAppDrawPrvLoadingFill(barW, state);
	offset = dcAppDrawPrvLoadingOffset(barW, state, fill);
	centerY = (int32_t)ctx->h / 2;
	barX = ((int32_t)ctx->w - (int32_t)barW) / 2;
	barY = centerY + 24;

	dcAppDrawClear(ctx, bg);
	dcAppDrawPrvCtxCentered(ctx, centerY - 78,
		state && state->appName ? state->appName : "Loading", FontLarge, primary);
	dcAppDrawPrvCtxCentered(ctx, centerY - 38,
		state && state->title ? state->title : "Please wait", FontMedium, primary);
	if (state && state->detail && state->detail[0])
		dcAppDrawPrvCtxCentered(ctx, centerY - 12, state->detail, FontSmall, secondary);
	dcAppDrawFill(ctx, barX, barY, (int32_t)barW, 12, track);
	dcAppDrawFill(ctx, barX + (int32_t)offset, barY + 2, (int32_t)fill, 8, accent);
	if (state && state->hint && state->hint[0])
		dcAppDrawPrvCtxCentered(ctx, centerY + 54, state->hint, FontSmall, secondary);
}

void dcAppDrawLoadingCanvas(const struct Canvas *cnv, const struct DcAppLoadingState *state)
{
	uint16_t bg = dcAppDrawRgb565(12, 12, 12);
	uint16_t primary = dcAppDrawRgb565(244, 244, 244);
	uint16_t secondary = dcAppDrawRgb565(188, 188, 188);
	uint16_t track = dcAppDrawRgb565(48, 48, 48);
	uint16_t accent = dcAppDrawRgb565(168, 168, 168);
	uint32_t barW, fill, offset;
	int32_t centerY, barX, barY;

	if (!cnv || !cnv->framebuffer || cnv->bpp != 16u || !cnv->w || !cnv->h)
		return;
	barW = cnv->w > 64u ? cnv->w - 64u : cnv->w;
	if (barW > 280u)
		barW = 280u;
	fill = dcAppDrawPrvLoadingFill(barW, state);
	offset = dcAppDrawPrvLoadingOffset(barW, state, fill);
	centerY = (int32_t)cnv->h / 2;
	barX = ((int32_t)cnv->w - (int32_t)barW) / 2;
	barY = centerY + 24;

	dcAppDrawPrvCanvasFill(cnv, 0, 0, cnv->w, cnv->h, bg);
	dcAppDrawPrvCanvasCentered(cnv, centerY - 78,
		state && state->appName ? state->appName : "Loading", FontLarge, primary);
	dcAppDrawPrvCanvasCentered(cnv, centerY - 38,
		state && state->title ? state->title : "Please wait", FontMedium, primary);
	if (state && state->detail && state->detail[0])
		dcAppDrawPrvCanvasCentered(cnv, centerY - 12, state->detail, FontSmall, secondary);
	dcAppDrawPrvCanvasFill(cnv, barX, barY, (int32_t)barW, 12, track);
	dcAppDrawPrvCanvasFill(cnv, barX + (int32_t)offset, barY + 2, (int32_t)fill, 8, accent);
	if (state && state->hint && state->hint[0])
		dcAppDrawPrvCanvasCentered(cnv, centerY + 54, state->hint, FontSmall, secondary);
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
