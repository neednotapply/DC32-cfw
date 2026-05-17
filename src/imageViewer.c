#pragma GCC optimize ("Os")
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "badgeLeds.h"
#include "dispDefcon.h"
#include "fatfs.h"
#include "gb.h"
#include "imageViewer.h"
#include "raster.h"
#include "timebase.h"
#include "toolWorkspace.h"
#include "gifdec.h"
#include "picojpeg.h"
#include "pngle.h"

#define IMAGE_MAX_DIMENSION       2048u
#define IMAGE_MAX_PIXELS          (2048u * 1024u)
#define GIF_MAX_WIDTH             RASTER_LOGICAL_WIDTH
#define GIF_MAX_HEIGHT            RASTER_LOGICAL_HEIGHT
#define IMAGE_READ_CHUNK          1024u
#define GIF_DELAY_MIN_TICKS       (TICKS_PER_SECOND / 50u)
#define GIF_DELAY_DEFAULT_TICKS   (TICKS_PER_SECOND / 10u)

struct ImageAllocHeader {
	size_t size;
};

struct ImageAllocSpan {
	uint8_t *ptr;
	size_t size;
	size_t used;
};

struct ImageDrawCtx {
	struct Canvas *cnv;
	struct RasterRect fit;
	uint32_t width;
	uint32_t height;
	bool tooLarge;
	bool ready;
};

struct JpegReadCtx {
	struct FatfsFil *fil;
	bool failed;
};

static struct ImageAllocSpan mAllocSpans[3];
static struct FatfsFil *mGifFile;

static bool imagePrvEndsWithNoCase(const char *str, const char *suffix)
{
	uint32_t strLen = strlen(str), suffixLen = strlen(suffix), i;

	if (strLen < suffixLen)
		return false;
	str += strLen - suffixLen;
	for (i = 0; i < suffixLen; i++) {
		char a = str[i], b = suffix[i];

		if (a >= 'A' && a <= 'Z')
			a += 'a' - 'A';
		if (b >= 'A' && b <= 'Z')
			b += 'a' - 'A';
		if (a != b)
			return false;
	}
	return true;
}

bool imageViewerFileName(const char *name)
{
	return imagePrvEndsWithNoCase(name, ".png") || imagePrvEndsWithNoCase(name, ".jpg") ||
	       imagePrvEndsWithNoCase(name, ".jpeg") || imagePrvEndsWithNoCase(name, ".gif");
}

static bool imagePrvValidStaticSize(uint32_t width, uint32_t height)
{
	return width && height && width <= IMAGE_MAX_DIMENSION && height <= IMAGE_MAX_DIMENSION &&
	       width <= IMAGE_MAX_PIXELS / height;
}

void imageViewerAllocReset(void)
{
	struct ToolWorkspaceSpan cart = toolWorkspaceGet(ToolWorkspaceCartRam);
	struct ToolWorkspaceSpan wram = toolWorkspaceGet(ToolWorkspaceWram);
	struct ToolWorkspaceSpan vram = toolWorkspaceGet(ToolWorkspaceVram);

	mAllocSpans[0].ptr = cart.ptr;
	mAllocSpans[0].size = cart.size;
	mAllocSpans[0].used = 0;
	mAllocSpans[1].ptr = wram.ptr;
	mAllocSpans[1].size = wram.size;
	mAllocSpans[1].used = 0;
	mAllocSpans[2].ptr = vram.ptr;
	mAllocSpans[2].size = vram.size;
	mAllocSpans[2].used = 0;
}

void *imageViewerAlloc(size_t size)
{
	size_t total = (size + sizeof(struct ImageAllocHeader) + 3u) &~ (size_t)3u;
	uint_fast8_t i;

	if (!size)
		return NULL;
	for (i = 0; i < sizeof(mAllocSpans) / sizeof(*mAllocSpans); i++) {
		struct ImageAllocSpan *span = &mAllocSpans[i];

		if (!span->ptr || total > span->size - span->used)
			continue;
		{
			struct ImageAllocHeader *hdr = (struct ImageAllocHeader*)(span->ptr + span->used);
			span->used += total;
			hdr->size = size;
			return hdr + 1;
		}
	}
	return NULL;
}

void *imageViewerCalloc(size_t count, size_t size)
{
	size_t total;
	void *ptr;

	if (count && size > ((size_t)-1) / count)
		return NULL;
	total = count * size;
	ptr = imageViewerAlloc(total);
	if (ptr)
		memset(ptr, 0, total);
	return ptr;
}

void *imageViewerRealloc(void *ptr, size_t size)
{
	struct ImageAllocHeader *hdr;
	void *newPtr;

	if (!ptr)
		return imageViewerAlloc(size);
	if (!size)
		return NULL;
	hdr = ((struct ImageAllocHeader*)ptr) - 1;
	if (size <= hdr->size) {
		hdr->size = size;
		return ptr;
	}
	newPtr = imageViewerAlloc(size);
	if (newPtr)
		memcpy(newPtr, ptr, hdr->size);
	return newPtr;
}

void imageViewerFree(void *ptr)
{
	(void)ptr;
}

int imageViewerGifOpen(const char *name, int flags)
{
	(void)name;
	(void)flags;
	return mGifFile ? 0 : -1;
}

int imageViewerGifRead(int fd, void *buf, unsigned int count)
{
	uint32_t numRead = 0;

	(void)fd;
	if (!mGifFile || !fatfsFileRead(mGifFile, buf, count, &numRead))
		return -1;
	return (int)numRead;
}

int imageViewerGifClose(int fd)
{
	(void)fd;
	return 0;
}

long imageViewerGifSeek(int fd, long offset, int whence)
{
	uint32_t pos;

	(void)fd;
	if (!mGifFile)
		return -1;
	switch (whence) {
	case 0:
		pos = (uint32_t)offset;
		break;
	case 1:
		pos = fatfsFileTell(mGifFile) + (uint32_t)offset;
		break;
	case 2:
		pos = fatfsFileGetSize(mGifFile) + (uint32_t)offset;
		break;
	default:
		return -1;
	}
	if (!fatfsFileSeek(mGifFile, pos))
		return -1;
	return (long)pos;
}

static bool imagePrvReadExact(struct FatfsFil *fil, void *buf, uint32_t size)
{
	uint32_t n = 0;

	return fatfsFileRead(fil, buf, size, &n) && n == size;
}

static uint32_t imagePrvReadBe32(const uint8_t *buf)
{
	return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
}

static uint16_t imagePrvReadLe16(const uint8_t *buf)
{
	return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

static enum ImageViewerResult imagePrvCheckPngHeader(struct FatfsFil *fil, uint32_t *widthP, uint32_t *heightP)
{
	uint8_t hdr[24];
	static const uint8_t sig[] = {137, 80, 78, 71, 13, 10, 26, 10};

	if (!fatfsFileSeek(fil, 0) || !imagePrvReadExact(fil, hdr, sizeof(hdr)))
		return ImageViewerResultReadError;
	if (memcmp(hdr, sig, sizeof(sig)) || memcmp(hdr + 12, "IHDR", 4))
		return ImageViewerResultUnsupported;
	*widthP = imagePrvReadBe32(hdr + 16);
	*heightP = imagePrvReadBe32(hdr + 20);
	if (!imagePrvValidStaticSize(*widthP, *heightP))
		return ImageViewerResultTooLarge;
	return fatfsFileSeek(fil, 0) ? ImageViewerResultBack : ImageViewerResultReadError;
}

static void imagePrvPngInit(pngle_t *pngle, uint32_t w, uint32_t h)
{
	struct ImageDrawCtx *ctx = (struct ImageDrawCtx*)pngle_get_user_data(pngle);

	ctx->width = w;
	ctx->height = h;
	ctx->fit = rasterFit(w, h, ctx->cnv->w, ctx->cnv->h);
	ctx->ready = imagePrvValidStaticSize(w, h);
	ctx->tooLarge = !ctx->ready;
}

static void imagePrvPngDraw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t rgba[4])
{
	struct ImageDrawCtx *ctx = (struct ImageDrawCtx*)pngle_get_user_data(pngle);
	uint32_t dx, dy;
	uint16_t color;

	if (!ctx->ready || x >= ctx->width || y >= ctx->height)
		return;
	color = rasterRgba8888OverBlack(rgba);
	for (dy = 0; dy < h; dy++)
		for (dx = 0; dx < w; dx++)
			rasterDrawScaledPixel(ctx->cnv, &ctx->fit, ctx->width, ctx->height, x + dx, y + dy, color);
}

static enum ImageViewerResult imagePrvDrawPng(struct Canvas *cnv, struct FatfsFil *fil)
{
	uint32_t width, height, numRead;
	uint8_t buf[IMAGE_READ_CHUNK];
	pngle_t *pngle;
	struct ImageDrawCtx ctx;
	enum ImageViewerResult check;

	check = imagePrvCheckPngHeader(fil, &width, &height);
	if (check != ImageViewerResultBack)
		return check;
	imageViewerAllocReset();
	pngle = pngle_new();
	if (!pngle)
		return ImageViewerResultNoMemory;
	memset(&ctx, 0, sizeof(ctx));
	ctx.cnv = cnv;
	ctx.width = width;
	ctx.height = height;
	ctx.fit = rasterFit(width, height, cnv->w, cnv->h);
	rasterClear(cnv, 0);
	pngle_set_user_data(pngle, &ctx);
	pngle_set_init_callback(pngle, imagePrvPngInit);
	pngle_set_draw_callback(pngle, imagePrvPngDraw);
	while (1) {
		uint32_t done = 0;

		if (!fatfsFileRead(fil, buf, sizeof(buf), &numRead)) {
			pngle_destroy(pngle);
			return ImageViewerResultReadError;
		}
		if (!numRead)
			break;
		while (done < numRead) {
			int eaten = pngle_feed(pngle, buf + done, numRead - done);
			if (eaten < 0) {
				pngle_destroy(pngle);
				return ctx.tooLarge ? ImageViewerResultTooLarge : ImageViewerResultDecodeError;
			}
			if (!eaten)
				break;
			done += (uint32_t)eaten;
		}
		badgeLedsTick();
	}
	pngle_destroy(pngle);
	return ctx.tooLarge ? ImageViewerResultTooLarge : ImageViewerResultBack;
}

static unsigned char imagePrvJpegNeedBytes(unsigned char *buf, unsigned char bufSize, unsigned char *bytesReadP, void *userData)
{
	struct JpegReadCtx *ctx = (struct JpegReadCtx*)userData;
	uint32_t n = 0;

	if (!fatfsFileRead(ctx->fil, buf, bufSize, &n)) {
		ctx->failed = true;
		*bytesReadP = 0;
		return PJPG_STREAM_READ_ERROR;
	}
	*bytesReadP = (unsigned char)n;
	return 0;
}

static enum ImageViewerResult imagePrvDrawJpeg(struct Canvas *cnv, struct FatfsFil *fil)
{
	pjpeg_image_info_t info;
	struct JpegReadCtx readCtx = {.fil = fil};
	struct RasterRect fit;
	unsigned char status;
	uint32_t mcuX, mcuY;

	if (!fatfsFileSeek(fil, 0))
		return ImageViewerResultReadError;
	status = pjpeg_decode_init(&info, imagePrvJpegNeedBytes, &readCtx, 0);
	if (status) {
		if (status == PJPG_UNSUPPORTED_MODE)
			return ImageViewerResultUnsupported;
		return readCtx.failed ? ImageViewerResultReadError : ImageViewerResultDecodeError;
	}
	if (!imagePrvValidStaticSize((uint32_t)info.m_width, (uint32_t)info.m_height))
		return ImageViewerResultTooLarge;
	fit = rasterFit((uint32_t)info.m_width, (uint32_t)info.m_height, cnv->w, cnv->h);
	rasterClear(cnv, 0);

	for (mcuY = 0; mcuY < (uint32_t)info.m_MCUSPerCol; mcuY++) {
		for (mcuX = 0; mcuX < (uint32_t)info.m_MCUSPerRow; mcuX++) {
			uint32_t px, py;

			status = pjpeg_decode_mcu();
			if (status == PJPG_NO_MORE_BLOCKS)
				return ImageViewerResultBack;
			if (status)
				return readCtx.failed ? ImageViewerResultReadError : ImageViewerResultDecodeError;

			for (py = 0; py < (uint32_t)info.m_MCUHeight; py++) {
				uint32_t srcY = mcuY * (uint32_t)info.m_MCUHeight + py;

				if (srcY >= (uint32_t)info.m_height)
					continue;
				for (px = 0; px < (uint32_t)info.m_MCUWidth; px++) {
					uint32_t srcX = mcuX * (uint32_t)info.m_MCUWidth + px;
					uint32_t blockX = px / 8u;
					uint32_t blockY = py / 8u;
					uint32_t blocksPerRow = (uint32_t)info.m_MCUWidth / 8u;
					uint32_t ofst = (blockY * blocksPerRow + blockX) * 64u + (py & 7u) * 8u + (px & 7u);
					uint8_t r, g, b;

					if (srcX >= (uint32_t)info.m_width)
						continue;
					r = info.m_pMCUBufR[ofst];
					g = info.m_comps == 1 ? r : info.m_pMCUBufG[ofst];
					b = info.m_comps == 1 ? r : info.m_pMCUBufB[ofst];
					rasterDrawScaledPixel(cnv, &fit, (uint32_t)info.m_width, (uint32_t)info.m_height, srcX, srcY, rasterRgb565(r, g, b));
				}
			}
		}
		badgeLedsTick();
	}
	return ImageViewerResultBack;
}

static void imagePrvDrawGifRect(struct Canvas *cnv, gd_GIF *gif, const struct RasterRect *fit)
{
	uint32_t x, y;

	for (y = 0; y < gif->fh; y++) {
		uint32_t srcY = gif->fy + y;

		for (x = 0; x < gif->fw; x++) {
			uint32_t srcX = gif->fx + x;
			uint8_t idx = gif->frame[srcY * gif->width + srcX];
			uint8_t *color;

			if (gif->gce.transparency && idx == gif->gce.tindex)
				continue;
			color = &gif->palette->colors[idx * 3u];
			rasterDrawScaledPixel(cnv, fit, gif->width, gif->height, srcX, srcY, rasterRgb565(color[0], color[1], color[2]));
		}
	}
}

static enum ImageViewerResult imagePrvDrawGif(struct Canvas *cnv, struct FatfsFil *fil)
{
	gd_GIF *gif;
	struct RasterRect fit;
	uint_fast16_t keys, prevKeys = uiGetUiKeysRaw();
	uint64_t nextFrameAt = getTime();
	bool paused = false;
	uint16_t loopsDone = 0;
	uint8_t hdr[10];
	uint32_t width, height;

	if (!fatfsFileSeek(fil, 0) || !imagePrvReadExact(fil, hdr, sizeof(hdr)))
		return ImageViewerResultReadError;
	if (memcmp(hdr, "GIF", 3))
		return ImageViewerResultUnsupported;
	width = imagePrvReadLe16(hdr + 6);
	height = imagePrvReadLe16(hdr + 8);
	if (!width || !height || width > GIF_MAX_WIDTH || height > GIF_MAX_HEIGHT)
		return ImageViewerResultTooLarge;
	if (width > 65500u / height)
		return ImageViewerResultNoMemory;
	if (!fatfsFileSeek(fil, 0))
		return ImageViewerResultReadError;
	imageViewerAllocReset();
	mGifFile = fil;
	gif = gd_open_gif("");
	mGifFile = NULL;
	if (!gif)
		return ImageViewerResultDecodeError;
	if (!gif->width || !gif->height || gif->width > GIF_MAX_WIDTH || gif->height > GIF_MAX_HEIGHT) {
		gd_close_gif(gif);
		return ImageViewerResultTooLarge;
	}
	fit = rasterFit(gif->width, gif->height, cnv->w, cnv->h);
	rasterClear(cnv, 0);
	mGifFile = fil;
	while (1) {
		uint64_t now = getTime();

		keys = uiGetUiKeysRaw();
		if ((keys & UI_KEY_BIT_CENTER) && !(prevKeys & UI_KEY_BIT_CENTER)) {
			mGifFile = NULL;
			gd_close_gif(gif);
			return ImageViewerResultExit;
		}
		if ((keys & KEY_BIT_B) && !(prevKeys & KEY_BIT_B)) {
			mGifFile = NULL;
			gd_close_gif(gif);
			return ImageViewerResultBack;
		}
		if ((keys & KEY_BIT_LEFT) && !(prevKeys & KEY_BIT_LEFT)) {
			mGifFile = NULL;
			gd_close_gif(gif);
			return ImageViewerResultPrev;
		}
		if ((keys & KEY_BIT_RIGHT) && !(prevKeys & KEY_BIT_RIGHT)) {
			mGifFile = NULL;
			gd_close_gif(gif);
			return ImageViewerResultNext;
		}
		if ((keys & KEY_BIT_A) && !(prevKeys & KEY_BIT_A))
			paused = !paused;
		prevKeys = keys;

		if (!paused && now >= nextFrameAt) {
			int frame = gd_get_frame(gif);
			uint32_t delayTicks;

			if (frame < 0) {
				mGifFile = NULL;
				gd_close_gif(gif);
				return ImageViewerResultDecodeError;
			}
			if (!frame) {
				if (gif->loop_count && ++loopsDone >= gif->loop_count)
					paused = true;
				else {
					gd_rewind(gif);
					rasterClear(cnv, 0);
					continue;
				}
			}
			else {
				imagePrvDrawGifRect(cnv, gif, &fit);
				delayTicks = gif->gce.delay ? (uint32_t)gif->gce.delay * (TICKS_PER_SECOND / 100u) : GIF_DELAY_DEFAULT_TICKS;
				if (delayTicks < GIF_DELAY_MIN_TICKS)
					delayTicks = GIF_DELAY_MIN_TICKS;
				nextFrameAt = now + delayTicks;
			}
		}
		badgeLedsTick();
	}
}

static enum ImageViewerResult imagePrvWaitForStaticInput(void)
{
	uint_fast16_t keys, prevKeys = uiGetUiKeysRaw();

	while (1) {
		keys = uiGetUiKeysRaw();
		if ((keys & UI_KEY_BIT_CENTER) && !(prevKeys & UI_KEY_BIT_CENTER))
			return ImageViewerResultExit;
		if ((keys & KEY_BIT_B) && !(prevKeys & KEY_BIT_B))
			return ImageViewerResultBack;
		if ((keys & KEY_BIT_LEFT) && !(prevKeys & KEY_BIT_LEFT))
			return ImageViewerResultPrev;
		if ((keys & KEY_BIT_RIGHT) && !(prevKeys & KEY_BIT_RIGHT))
			return ImageViewerResultNext;
		prevKeys = keys;
		badgeLedsTick();
	}
}

enum ImageViewerResult imageViewerRun(struct Canvas *cnv, struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *initialLocator, const char *initialName)
{
	struct FatfsFil *fil;
	enum ImageViewerResult ret;
	bool staticImage = false;

	(void)rootPath;
	if (!initialLocator || !initialName)
		return ImageViewerResultOpenError;
	if (!toolWorkspaceAcquire(ToolWorkspaceCartRam, ToolWorkspaceOwnerImage, NULL) ||
	    !toolWorkspaceAcquire(ToolWorkspaceWram, ToolWorkspaceOwnerImage, NULL) ||
	    !toolWorkspaceAcquire(ToolWorkspaceVram, ToolWorkspaceOwnerImage, NULL)) {
		toolWorkspaceRelease(ToolWorkspaceVram, ToolWorkspaceOwnerImage);
		toolWorkspaceRelease(ToolWorkspaceWram, ToolWorkspaceOwnerImage);
		toolWorkspaceRelease(ToolWorkspaceCartRam, ToolWorkspaceOwnerImage);
		return ImageViewerResultNoMemory;
	}

	fil = fatfsFileOpenWithLocator(vol, initialLocator, OPEN_MODE_READ);
	if (!fil) {
		ret = ImageViewerResultOpenError;
		goto out_release;
	}
	if (imagePrvEndsWithNoCase(initialName, ".png")) {
		staticImage = true;
		ret = imagePrvDrawPng(cnv, fil);
	}
	else if (imagePrvEndsWithNoCase(initialName, ".jpg") || imagePrvEndsWithNoCase(initialName, ".jpeg")) {
		staticImage = true;
		ret = imagePrvDrawJpeg(cnv, fil);
	}
	else if (imagePrvEndsWithNoCase(initialName, ".gif"))
		ret = imagePrvDrawGif(cnv, fil);
	else
		ret = ImageViewerResultUnsupported;

	if (staticImage && ret == ImageViewerResultBack)
		ret = imagePrvWaitForStaticInput();
	fatfsFileClose(fil);

out_release:
	mGifFile = NULL;
	toolWorkspaceRelease(ToolWorkspaceVram, ToolWorkspaceOwnerImage);
	toolWorkspaceRelease(ToolWorkspaceWram, ToolWorkspaceOwnerImage);
	toolWorkspaceRelease(ToolWorkspaceCartRam, ToolWorkspaceOwnerImage);
	return ret;
}
