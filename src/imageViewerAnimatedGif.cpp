#pragma GCC optimize ("Os")
#include <new>
#include <stdint.h>

extern "C" {
#include "badgeLeds.h"
#include "fatfs.h"
#include "gb.h"
#include "imageViewer.h"
#include "raster.h"
#include "timebase.h"
#include "toolWorkspace.h"
#include "ui.h"
}

#include "AnimatedGIF.h"
#include "imageViewerAnimatedGif.h"

struct GifPlaybackCtx {
	struct Canvas *cnv;
	struct FatfsFil *fil;
	struct RasterRect fit;
	uint32_t canvasW;
	uint32_t canvasH;
	uint_fast16_t prevKeys;
	bool paused;
	bool handled;
	enum ImageViewerResult result;
	struct RasterRect prevRect;
	uint16_t prevBg;
	uint8_t prevDisposal;
	struct RasterRect currentRect;
	uint16_t currentBg;
	uint8_t currentDisposal;
};

static GifPlaybackCtx *mGifOpenCtx;

static uint32_t gifPrvDelayTicks(uint32_t delayMs)
{
	uint64_t ticks;

	if (!delayMs)
		delayMs = 16;
	ticks = (uint64_t)delayMs * TICKS_PER_SECOND / 1000u;
	if (!ticks)
		ticks = 1;
	if (ticks > 0xffffffffu)
		ticks = 0xffffffffu;
	return (uint32_t)ticks;
}

static enum ImageViewerResult gifPrvPollKeys(GifPlaybackCtx *ctx, bool allowPause)
{
	uint_fast16_t keys = uiGetUiKeysRaw();
	uint_fast16_t prevKeys = ctx->prevKeys;

	if ((keys & UI_KEY_BIT_CENTER) && !(prevKeys & UI_KEY_BIT_CENTER))
		return ImageViewerResultMenu;
	if ((keys & KEY_BIT_B) && !(prevKeys & KEY_BIT_B))
		return ImageViewerResultBack;
	if ((keys & KEY_BIT_LEFT) && !(prevKeys & KEY_BIT_LEFT))
		return ImageViewerResultPrev;
	if ((keys & KEY_BIT_RIGHT) && !(prevKeys & KEY_BIT_RIGHT))
		return ImageViewerResultNext;
	if (allowPause && (keys & KEY_BIT_A) && !(prevKeys & KEY_BIT_A))
		ctx->paused = !ctx->paused;
	ctx->prevKeys = keys;
	return ImageViewerResultBack;
}

static bool gifPrvHandleKeys(GifPlaybackCtx *ctx)
{
	enum ImageViewerResult ret;

	if (ctx->handled)
		return true;
	ret = gifPrvPollKeys(ctx, true);
	if (ret != ImageViewerResultBack) {
		ctx->result = ret;
		ctx->handled = true;
		return true;
	}
	return false;
}

static struct RasterRect gifPrvScaleRect(const GifPlaybackCtx *ctx, uint32_t srcX, uint32_t srcY, uint32_t srcW, uint32_t srcH)
{
	struct RasterRect rect = {0};
	uint32_t x1, y1;

	if (!srcW || !srcH || !ctx->canvasW || !ctx->canvasH || !ctx->fit.w || !ctx->fit.h)
		return rect;
	rect.x = (uint16_t)(ctx->fit.x + (srcX * ctx->fit.w) / ctx->canvasW);
	rect.y = (uint16_t)(ctx->fit.y + (srcY * ctx->fit.h) / ctx->canvasH);
	x1 = ctx->fit.x + ((srcX + srcW) * ctx->fit.w + ctx->canvasW - 1u) / ctx->canvasW;
	y1 = ctx->fit.y + ((srcY + srcH) * ctx->fit.h + ctx->canvasH - 1u) / ctx->canvasH;
	if (x1 <= rect.x)
		x1 = rect.x + 1u;
	if (y1 <= rect.y)
		y1 = rect.y + 1u;
	if (x1 > ctx->cnv->w)
		x1 = ctx->cnv->w;
	if (y1 > ctx->cnv->h)
		y1 = ctx->cnv->h;
	rect.w = (uint16_t)(x1 - rect.x);
	rect.h = (uint16_t)(y1 - rect.y);
	return rect;
}

static void gifPrvApplyPrevDisposal(GifPlaybackCtx *ctx)
{
	if (ctx->prevDisposal == 2 && ctx->prevRect.w && ctx->prevRect.h)
		rasterFillRect(ctx->cnv, &ctx->prevRect, ctx->prevBg);
	ctx->prevDisposal = 0;
}

static void gifPrvPutPixelFast(const struct Canvas *cnv, uint32_t x, uint32_t y, uint16_t color)
{
	uint16_t *fb = (uint16_t*)cnv->framebuffer;
	uint32_t index;

	if (!fb || x >= cnv->w || y >= cnv->h)
		return;
	if (cnv->flipped) {
		x = cnv->w - 1u - x;
		y = cnv->h - 1u - y;
	}
	if (cnv->rotated)
		index = x * cnv->h + (cnv->h - 1u - y);
	else
		index = y * cnv->w + x;
	fb[index] = color;
}

static void *gifPrvOpen(const char *name, int32_t *sizeP)
{
	(void)name;
	if (!mGifOpenCtx || !mGifOpenCtx->fil)
		return nullptr;
	*sizeP = (int32_t)fatfsFileGetSize(mGifOpenCtx->fil);
	fatfsFileSeek(mGifOpenCtx->fil, 0);
	return mGifOpenCtx;
}

static void gifPrvClose(void *handle)
{
	(void)handle;
}

static int32_t gifPrvRead(GIFFILE *file, uint8_t *buf, int32_t len)
{
	GifPlaybackCtx *ctx = (GifPlaybackCtx*)file->fHandle;
	uint32_t numRead = 0;

	if (!ctx || !ctx->fil || len <= 0)
		return 0;
	if (!fatfsFileRead(ctx->fil, buf, (uint32_t)len, &numRead))
		return 0;
	file->iPos = (int32_t)fatfsFileTell(ctx->fil);
	return (int32_t)numRead;
}

static int32_t gifPrvSeek(GIFFILE *file, int32_t pos)
{
	GifPlaybackCtx *ctx = (GifPlaybackCtx*)file->fHandle;

	if (!ctx || !ctx->fil || pos < 0)
		return 0;
	if (!fatfsFileSeek(ctx->fil, (uint32_t)pos))
		return 0;
	file->iPos = pos;
	return pos;
}

static void gifPrvDraw(GIFDRAW *draw)
{
	GifPlaybackCtx *ctx = (GifPlaybackCtx*)draw->pUser;
	uint32_t srcY;
	int x;
	bool unscaled;

	if (!ctx || gifPrvHandleKeys(ctx))
		return;
	srcY = (uint32_t)(draw->iY + draw->y);
	if (srcY >= ctx->canvasH)
		return;
	ctx->currentDisposal = draw->ucDisposalMethod;
	ctx->currentBg = draw->pPalette[draw->ucBackground];
	ctx->currentRect = gifPrvScaleRect(ctx, (uint32_t)draw->iX, (uint32_t)draw->iY, (uint32_t)draw->iWidth, (uint32_t)draw->iHeight);
	unscaled = ctx->fit.w == ctx->canvasW && ctx->fit.h == ctx->canvasH;
	for (x = 0; x < draw->iWidth; x++) {
		uint8_t palIdx = draw->pPixels[x];
		uint32_t srcX = (uint32_t)(draw->iX + x);

		if (srcX >= ctx->canvasW)
			continue;
		if (draw->ucHasTransparency && palIdx == draw->ucTransparent)
			continue;
		if (unscaled)
			gifPrvPutPixelFast(ctx->cnv, ctx->fit.x + srcX, ctx->fit.y + srcY, draw->pPalette[palIdx]);
		else
			rasterDrawScaledPixel(ctx->cnv, &ctx->fit, ctx->canvasW, ctx->canvasH, srcX, srcY, draw->pPalette[palIdx]);
	}
}

static enum ImageViewerResult gifPrvMapError(int err)
{
	if (err == GIF_TOO_WIDE)
		return ImageViewerResultTooLarge;
	if (err == GIF_ERROR_MEMORY)
		return ImageViewerResultNoMemory;
	if (err == GIF_EARLY_EOF)
		return ImageViewerResultReadError;
	return ImageViewerResultDecodeError;
}

extern "C" enum ImageViewerResult imageViewerAnimatedGifRun(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *locator)
{
	struct FatfsFil *fil;
	struct ToolWorkspaceSpan span;
	AnimatedGIF *gif;
	GifPlaybackCtx ctx = {};
	uintptr_t base;
	uintptr_t aligned;
	uintptr_t end;
	int openOk;
	int loopCount;
	int loopsDone = 0;
	uint64_t nextAt = getTime();
	enum ImageViewerResult ret = ImageViewerResultBack;

	if (!cnv || !vol || !locator)
		return ImageViewerResultOpenError;
	fil = fatfsFileOpenWithLocator(vol, locator, OPEN_MODE_READ);
	if (!fil)
		return ImageViewerResultOpenError;
	if (!toolWorkspaceAcquire(ToolWorkspaceCartRam, ToolWorkspaceOwnerImage, &span)) {
		fatfsFileClose(fil);
		return ImageViewerResultNoMemory;
	}
	base = (uintptr_t)span.ptr;
	end = base + span.size;
	aligned = (base + alignof(AnimatedGIF) - 1u) & ~(uintptr_t)(alignof(AnimatedGIF) - 1u);
	if (aligned > end || sizeof(AnimatedGIF) > end - aligned) {
		toolWorkspaceRelease(ToolWorkspaceCartRam, ToolWorkspaceOwnerImage);
		fatfsFileClose(fil);
		return ImageViewerResultNoMemory;
	}
	gif = new ((void*)aligned) AnimatedGIF();
	ctx.cnv = cnv;
	ctx.fil = fil;
	ctx.prevKeys = uiGetUiKeysRaw();
	ctx.result = ImageViewerResultBack;
	mGifOpenCtx = &ctx;
	gif->begin(GIF_PALETTE_RGB565_LE);
	openOk = gif->open("", gifPrvOpen, gifPrvClose, gifPrvRead, gifPrvSeek, gifPrvDraw);
	mGifOpenCtx = nullptr;
	if (!openOk) {
		ret = gifPrvMapError(gif->getLastError());
		goto out;
	}
	ctx.canvasW = (uint32_t)gif->getCanvasWidth();
	ctx.canvasH = (uint32_t)gif->getCanvasHeight();
	if (!ctx.canvasW || !ctx.canvasH) {
		ret = ImageViewerResultDecodeError;
		goto out;
	}
	ctx.fit = rasterFit(ctx.canvasW, ctx.canvasH, cnv->w, cnv->h);
	rasterClear(cnv, 0);
	loopCount = gif->getLoopCount();
	while (1) {
		int delayMs = 0;
		int played;
		bool drewFrame;

		while (ctx.paused || getTime() < nextAt) {
			if (gifPrvHandleKeys(&ctx)) {
				ret = ctx.result;
				goto out;
			}
			if (ctx.paused)
				nextAt = getTime();
			badgeLedsTick();
		}
		gifPrvApplyPrevDisposal(&ctx);
		ctx.currentRect = {};
		ctx.currentDisposal = 0;
		played = gif->playFrame(false, &delayMs, &ctx);
		if (ctx.handled) {
			ret = ctx.result;
			goto out;
		}
		if (played < 0) {
			ret = gifPrvMapError(gif->getLastError());
			goto out;
		}
		drewFrame = ctx.currentRect.w && ctx.currentRect.h;
		if (!drewFrame)
			ctx.currentRect = gifPrvScaleRect(&ctx, 0, 0, ctx.canvasW, ctx.canvasH);
		ctx.prevRect = ctx.currentRect;
		ctx.prevBg = ctx.currentBg;
		ctx.prevDisposal = ctx.currentDisposal;
		if (drewFrame)
			nextAt += gifPrvDelayTicks((uint32_t)delayMs);
		if (played == 0) {
			while (drewFrame && getTime() < nextAt) {
				if (gifPrvHandleKeys(&ctx)) {
					ret = ctx.result;
					goto out;
				}
				badgeLedsTick();
			}
			if (loopCount > 0 && ++loopsDone >= loopCount)
				break;
			gif->reset();
			rasterClear(cnv, 0);
			ctx.prevDisposal = 0;
			nextAt = getTime();
			continue;
		}
	}
	while (1) {
		if (gifPrvHandleKeys(&ctx)) {
			ret = ctx.result;
			goto out;
		}
		badgeLedsTick();
	}

out:
	gif->close();
	gif->~AnimatedGIF();
	toolWorkspaceRelease(ToolWorkspaceCartRam, ToolWorkspaceOwnerImage);
	fatfsFileClose(fil);
	return ret;
}
