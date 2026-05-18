#pragma GCC optimize ("Os")
#include <stdint.h>
#include <string.h>
#include "badgeLeds.h"
#include "dispDefcon.h"
#include "fatfs.h"
#include "fonts.h"
#include "gb.h"
#include "imageViewer.h"
#include "memMap.h"
#include "qspi.h"
#include "raster.h"
#include "sd.h"
#include "timebase.h"
#include "toolWorkspace.h"
#include "ui.h"

#define IMAGE_SD_SAFE_HZ        500000u
#define IMAGE_SD_FAST_HZ        2000000u
#define IMAGE_LOADING_DELAY_TICKS   (TICKS_PER_SECOND * 3u)

#define DCI1_MAGIC              0x31494344u
#define DCI_HEADER_SIZE         64u
#define DCI1_VERSION            1u
#define DCI_KIND_STATIC         1u
#define DCI_BPP_RGB565          16u
#define DCI_CANVAS_W            320u
#define DCI_CANVAS_H            240u
#define DCI_FRAME_BYTES         (DCI_CANVAS_W * DCI_CANVAS_H * 2u)

#define DCA1_MAGIC              0x31414344u
#define DCA_HEADER_SIZE         64u
#define DCA1_VERSION            1u
#define DCA_BPP_INDEXED         8u
#define DCA_CODEC_RAW           0u
#define DCA_CODEC_RLE           1u

struct ImageDciHeader {
	uint32_t magic;
	uint16_t headerSize;
	uint8_t version;
	uint8_t kind;
	uint16_t canvasW;
	uint16_t canvasH;
	uint8_t bpp;
	uint8_t flags;
	uint16_t reserved0;
	uint32_t frameCount;
	uint32_t loopCount;
	uint32_t payloadBytes;
	uint32_t reserved1[9];
} __attribute__((packed));

struct ImageDcaHeader {
	uint32_t magic;
	uint16_t headerSize;
	uint8_t version;
	uint8_t bpp;
	uint16_t canvasW;
	uint16_t canvasH;
	uint16_t frameCount;
	uint16_t paletteCount;
	uint32_t loopCount;
	uint32_t payloadBytes;
	uint32_t frameTableOffset;
	uint32_t paletteOffset;
	uint32_t dataOffset;
	uint32_t flags;
	uint32_t reserved[6];
} __attribute__((packed));

struct ImageDcaFrame {
	uint32_t delayMs;
	uint16_t rectCount;
	uint16_t flags;
	uint32_t dataOffset;
	uint32_t dataBytes;
} __attribute__((packed));

struct ImageDcaRect {
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
	uint8_t codec;
	uint8_t reserved0;
	uint32_t payloadBytes;
	uint16_t reserved1;
} __attribute__((packed));

typedef char ImageDciHeaderSizeCheck[(sizeof(struct ImageDciHeader) == DCI_HEADER_SIZE) ? 1 : -1];
typedef char ImageDcaHeaderSizeCheck[(sizeof(struct ImageDcaHeader) == DCA_HEADER_SIZE) ? 1 : -1];
typedef char ImageDcaFrameSizeCheck[(sizeof(struct ImageDcaFrame) == 16u) ? 1 : -1];
typedef char ImageDcaRectSizeCheck[(sizeof(struct ImageDcaRect) == 16u) ? 1 : -1];

static bool mImageFastSdDisabled;

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
	return imagePrvEndsWithNoCase(name, ".dci") || imagePrvEndsWithNoCase(name, ".dca");
}

static bool imagePrvReadExact(struct FatfsFil *fil, void *buf, uint32_t size)
{
	uint32_t numRead = 0;

	return fatfsFileRead(fil, buf, size, &numRead) && numRead == size;
}

static uint64_t imagePrvMsToTicks(uint32_t ms)
{
	if (!ms)
		ms = 1;
	return ((uint64_t)ms * TICKS_PER_SECOND + 999u) / 1000u;
}

static bool imagePrvValidDci1Header(const struct ImageDciHeader *hdr, uint32_t fileSize)
{
	if (hdr->magic != DCI1_MAGIC || hdr->headerSize != DCI_HEADER_SIZE ||
	    hdr->version != DCI1_VERSION || hdr->kind != DCI_KIND_STATIC ||
	    hdr->canvasW != DCI_CANVAS_W || hdr->canvasH != DCI_CANVAS_H ||
	    hdr->bpp != DCI_BPP_RGB565 || hdr->flags || hdr->frameCount != 1 ||
	    hdr->payloadBytes != DCI_FRAME_BYTES)
		return false;
	return fileSize == DCI_HEADER_SIZE + DCI_FRAME_BYTES;
}

static bool imagePrvRangeOk(uint32_t off, uint32_t size, uint32_t fileSize)
{
	return off <= fileSize && size <= fileSize - off;
}

static bool imagePrvValidDca1Header(const struct ImageDcaHeader *hdr, uint32_t fileSize)
{
	uint32_t paletteBytes, frameTableBytes;

	if (hdr->magic != DCA1_MAGIC || hdr->headerSize != DCA_HEADER_SIZE ||
	    hdr->version != DCA1_VERSION || hdr->bpp != DCA_BPP_INDEXED ||
	    hdr->canvasW != DCI_CANVAS_W || hdr->canvasH != DCI_CANVAS_H ||
	    !hdr->frameCount || !hdr->paletteCount || hdr->paletteCount > 256u ||
	    hdr->flags || fileSize > QSPI_ROM_SIZE_MAX || fileSize != DCA_HEADER_SIZE + hdr->payloadBytes)
		return false;
	paletteBytes = (uint32_t)hdr->paletteCount * sizeof(uint16_t);
	frameTableBytes = (uint32_t)hdr->frameCount * sizeof(struct ImageDcaFrame);
	if (!imagePrvRangeOk(hdr->paletteOffset, paletteBytes, fileSize) ||
	    !imagePrvRangeOk(hdr->frameTableOffset, frameTableBytes, fileSize) ||
	    hdr->paletteOffset < DCA_HEADER_SIZE || hdr->frameTableOffset < DCA_HEADER_SIZE ||
	    hdr->dataOffset < DCA_HEADER_SIZE || hdr->dataOffset > fileSize)
		return false;
	return true;
}

static enum ImageViewerResult imagePrvPollKeys(uint_fast16_t *prevKeysP, bool *handledP)
{
	uint_fast16_t keys = uiGetUiKeysRaw();
	uint_fast16_t prevKeys = *prevKeysP;

	*handledP = true;
	if ((keys & UI_KEY_BIT_CENTER) && !(prevKeys & UI_KEY_BIT_CENTER))
		return ImageViewerResultMenu;
	if ((keys & KEY_BIT_B) && !(prevKeys & KEY_BIT_B))
		return ImageViewerResultBack;
	if ((keys & KEY_BIT_LEFT) && !(prevKeys & KEY_BIT_LEFT))
		return ImageViewerResultPrev;
	if ((keys & KEY_BIT_RIGHT) && !(prevKeys & KEY_BIT_RIGHT))
		return ImageViewerResultNext;
	*prevKeysP = keys;
	*handledP = false;
	return ImageViewerResultBack;
}

static enum ImageViewerResult imagePrvWaitForStaticInput(void)
{
	uint_fast16_t prevKeys = uiGetUiKeysRaw();

	while (1) {
		bool handled;
		enum ImageViewerResult ret = imagePrvPollKeys(&prevKeys, &handled);

		if (handled)
			return ret;
		badgeLedsTick();
	}
}

static uint32_t imagePrvTextWidth(const char *text, enum Font font)
{
	uint32_t width = 0;

	while (*text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text))
			width += glyph.width;
		text++;
	}
	return width;
}

static void imagePrvDrawText(struct Canvas *cnv, int32_t x, int32_t y, const char *text, enum Font font, uint16_t color)
{
	while (*text) {
		struct FontGlyphInfo glyph;
		uint32_t row, col;

		if (!fontGetGlyphInfo(&glyph, font, (unsigned char)*text)) {
			text++;
			continue;
		}
		for (row = 0; row < glyph.height; row++) {
			for (col = 0; col < glyph.width; col++) {
				if (fontGetGlyphPixel(&glyph, row, col))
					rasterPutPixel(cnv, (uint32_t)(x + (int32_t)col), (uint32_t)(y + (int32_t)row), color);
			}
		}
		x += glyph.width;
		text++;
	}
}

static void imagePrvDrawLoading(struct Canvas *cnv)
{
	static const char text[] = "Loading...";
	enum Font font = FontLarge;
	uint32_t textW = imagePrvTextWidth(text, font);
	uint32_t textH = fontGetHeight(font);

	rasterClear(cnv, 0);
	imagePrvDrawText(cnv, (int32_t)((cnv->w - textW) / 2u), (int32_t)((cnv->h - textH) / 2u), text, font, 0xffffu);
}

static bool imagePrvFlashLoadRangeRaw(struct FatfsFil *fil, uint32_t fileOffset, uint32_t size, struct Canvas *slowLoadCnv)
{
	struct ToolWorkspaceSpan mem;
	uint32_t bufSz, pos;
	uint8_t *buf;
	uint64_t start = getTime();
	bool ret = false;
	bool loadingDrawn = false;

	if (!toolWorkspaceAcquire(ToolWorkspaceWram, ToolWorkspaceOwnerImage, &mem))
		return false;
	buf = (uint8_t*)mem.ptr;
	bufSz = mem.size &~ (QSPI_ERASE_GRANULARITY - 1u);
	if (bufSz > 32768u)
		bufSz = 32768u;
	if (bufSz < QSPI_ERASE_GRANULARITY)
		goto out;
	if (!fatfsFileSeek(fil, fileOffset))
		goto out;
	for (pos = 0; pos < size; ) {
		uint32_t now = size - pos, numRead = 0, writeSz, eraseSz;

		if (now > bufSz)
			now = bufSz;
		if (!fatfsFileRead(fil, buf, now, &numRead) || numRead != now)
			goto out;
		writeSz = (now + QSPI_WRITE_GRANULARITY - 1u) &~ (QSPI_WRITE_GRANULARITY - 1u);
		eraseSz = (now + QSPI_ERASE_GRANULARITY - 1u) &~ (QSPI_ERASE_GRANULARITY - 1u);
		if (writeSz != now)
			memset(buf + now, 0, writeSz - now);
		if (!flashWrite(QSPI_ROM_START + pos, eraseSz, buf, writeSz))
			goto out;
		pos += now;
		if (slowLoadCnv && !loadingDrawn && getTime() - start >= IMAGE_LOADING_DELAY_TICKS) {
			imagePrvDrawLoading(slowLoadCnv);
			loadingDrawn = true;
		}
	}
	ret = true;
out:
	toolWorkspaceRelease(ToolWorkspaceWram, ToolWorkspaceOwnerImage);
	return ret;
}

static bool imagePrvFlashLoadRange(struct FatfsFil *fil, uint32_t fileOffset, uint32_t size, struct Canvas *slowLoadCnv)
{
	bool ret;

	if (size > QSPI_ROM_SIZE_MAX)
		return false;
	if (!mImageFastSdDisabled) {
		sdSetSpeedLimit(IMAGE_SD_FAST_HZ);
		ret = imagePrvFlashLoadRangeRaw(fil, fileOffset, size, slowLoadCnv);
		sdSetSpeedLimit(IMAGE_SD_SAFE_HZ);
		if (ret)
			return true;
		mImageFastSdDisabled = true;
	}
	sdSetSpeedLimit(IMAGE_SD_SAFE_HZ);
	return imagePrvFlashLoadRangeRaw(fil, fileOffset, size, slowLoadCnv);
}

static enum ImageViewerResult imagePrvShowDci1Static(struct Canvas *cnv, struct FatfsFil *fil)
{
	const uint16_t *src = (const uint16_t*)QSPI_ROM_START;
	uint16_t *dst = (uint16_t*)cnv->framebuffer;
	uint32_t i;

	if (!imagePrvFlashLoadRange(fil, DCI_HEADER_SIZE, DCI_FRAME_BYTES, NULL))
		return ImageViewerResultReadError;
	dispPrvWaitForScanoutStart();
	if (!cnv->flipped) {
		memcpy(cnv->framebuffer, src, DCI_FRAME_BYTES);
	}
	else {
		for (i = 0; i < DCI_FRAME_BYTES / sizeof(uint16_t); i++)
			dst[i] = src[DCI_FRAME_BYTES / sizeof(uint16_t) - 1u - i];
	}
	return imagePrvWaitForStaticInput();
}

static enum ImageViewerResult imagePrvRunDci1(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *locator)
{
	struct FatfsFil *fil;
	struct ImageDciHeader hdr;
	enum ImageViewerResult ret;

	fil = fatfsFileOpenWithLocator(vol, locator, OPEN_MODE_READ);
	if (!fil)
		return ImageViewerResultOpenError;
	if (!imagePrvReadExact(fil, &hdr, sizeof(hdr)) || !imagePrvValidDci1Header(&hdr, fatfsFileGetSize(fil)))
		ret = ImageViewerResultDecodeError;
	else
		ret = imagePrvShowDci1Static(cnv, fil);
	fatfsFileClose(fil);
	return ret;
}

static uint32_t imagePrvFbIndex(const struct Canvas *cnv, uint32_t x, uint32_t y)
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

static void imagePrvDcaPutPixel(const struct Canvas *cnv, const uint16_t *palette, uint32_t x, uint32_t y, uint8_t colorIdx)
{
	((uint16_t*)cnv->framebuffer)[imagePrvFbIndex(cnv, x, y)] = palette[colorIdx];
}

static bool imagePrvDcaApplyIndex(const struct Canvas *cnv, const uint16_t *palette, const struct ImageDcaRect *rect, uint32_t idx, uint8_t colorIdx)
{
	uint32_t x, y;

	if (idx >= (uint32_t)rect->w * rect->h)
		return false;
	x = rect->x + idx / rect->h;
	y = rect->y + rect->h - 1u - idx % rect->h;
	imagePrvDcaPutPixel(cnv, palette, x, y, colorIdx);
	return true;
}

static bool imagePrvDcaApplyRaw(const struct Canvas *cnv, const uint16_t *palette, const struct ImageDcaRect *rect, const uint8_t *payload)
{
	uint32_t x;

	if (cnv->rotated) {
		uint16_t *fb = (uint16_t*)cnv->framebuffer;
		for (x = 0; x < rect->w; x++) {
			uint32_t y;
			uint16_t *dst = cnv->flipped ?
				fb + (uint32_t)(cnv->w - 1u - rect->x - x) * cnv->h + rect->y :
				fb + (uint32_t)(rect->x + x) * cnv->h + (cnv->h - rect->y - rect->h);

			for (y = 0; y < rect->h; y++) {
				*dst = palette[*payload++];
				dst += cnv->flipped ? -1 : 1;
			}
		}
		return true;
	}
	for (x = 0; x < (uint32_t)rect->w * rect->h; x++)
		imagePrvDcaApplyIndex(cnv, palette, rect, x, payload[x]);
	return true;
}

static bool imagePrvDcaFillRun(const struct Canvas *cnv, const uint16_t *palette, const struct ImageDcaRect *rect, uint32_t *outPosP, uint32_t count, uint8_t colorIdx)
{
	uint32_t outPos = *outPosP;
	uint32_t expected = (uint32_t)rect->w * rect->h;

	if (count > expected - outPos)
		return false;
	if (cnv->rotated) {
		uint16_t *fb = (uint16_t*)cnv->framebuffer;
		uint16_t color = palette[colorIdx];

		while (count) {
			uint32_t col = outPos / rect->h;
			uint32_t row = outPos % rect->h;
			uint32_t now = rect->h - row;
			uint16_t *dst = cnv->flipped ?
				fb + (uint32_t)(cnv->w - 1u - rect->x - col) * cnv->h + rect->y + rect->h - 1u - row :
				fb + (uint32_t)(rect->x + col) * cnv->h + (cnv->h - rect->y - rect->h) + row;

			if (now > count)
				now = count;
			count -= now;
			outPos += now;
			while (now--) {
				*dst = color;
				dst += cnv->flipped ? -1 : 1;
			}
		}
	}
	else {
		while (count--)
			imagePrvDcaApplyIndex(cnv, palette, rect, outPos++, colorIdx);
	}
	*outPosP = outPos;
	return true;
}

static bool imagePrvDcaCopyRun(const struct Canvas *cnv, const uint16_t *palette, const struct ImageDcaRect *rect, uint32_t *outPosP, const uint8_t *src, uint32_t count)
{
	uint32_t outPos = *outPosP;
	uint32_t expected = (uint32_t)rect->w * rect->h;

	if (count > expected - outPos)
		return false;
	if (cnv->rotated) {
		uint16_t *fb = (uint16_t*)cnv->framebuffer;

		while (count) {
			uint32_t col = outPos / rect->h;
			uint32_t row = outPos % rect->h;
			uint32_t now = rect->h - row;
			uint16_t *dst = cnv->flipped ?
				fb + (uint32_t)(cnv->w - 1u - rect->x - col) * cnv->h + rect->y + rect->h - 1u - row :
				fb + (uint32_t)(rect->x + col) * cnv->h + (cnv->h - rect->y - rect->h) + row;

			if (now > count)
				now = count;
			count -= now;
			outPos += now;
			while (now--) {
				*dst = palette[*src++];
				dst += cnv->flipped ? -1 : 1;
			}
		}
	}
	else {
		while (count--)
			imagePrvDcaApplyIndex(cnv, palette, rect, outPos++, *src++);
	}
	*outPosP = outPos;
	return true;
}

static bool imagePrvDcaApplyRle(const struct Canvas *cnv, const uint16_t *palette, const struct ImageDcaRect *rect, const uint8_t *payload, uint32_t payloadBytes)
{
	uint32_t pos = 0, outPos = 0, expected = (uint32_t)rect->w * rect->h;

	while (pos < payloadBytes && outPos < expected) {
		uint8_t ctrl = payload[pos++];
		uint32_t count = (ctrl & 0x7fu) + 1u;

		if (ctrl & 0x80u) {
			uint8_t color;

			if (pos >= payloadBytes || count > expected - outPos)
				return false;
			color = payload[pos++];
			if (!imagePrvDcaFillRun(cnv, palette, rect, &outPos, count, color))
				return false;
		}
		else {
			if (count > payloadBytes - pos || count > expected - outPos)
				return false;
			if (!imagePrvDcaCopyRun(cnv, palette, rect, &outPos, payload + pos, count))
				return false;
			pos += count;
		}
	}
	return pos == payloadBytes && outPos == expected;
}

static enum ImageViewerResult imagePrvDcaApplyFrame(struct Canvas *cnv, const uint8_t *base, uint32_t fileSize, const struct ImageDcaHeader *hdr, const struct ImageDcaFrame *frame)
{
	const uint16_t *palette = (const uint16_t*)(base + hdr->paletteOffset);
	const uint8_t *p;
	uint32_t left, i;

	if (frame->flags || !imagePrvRangeOk(frame->dataOffset, frame->dataBytes, fileSize))
		return ImageViewerResultDecodeError;
	p = base + frame->dataOffset;
	left = frame->dataBytes;
	for (i = 0; i < frame->rectCount; i++) {
		const struct ImageDcaRect *rect;
		const uint8_t *payload;
		uint32_t pixels;
		bool ok;

		if (left < sizeof(*rect))
			return ImageViewerResultDecodeError;
		rect = (const struct ImageDcaRect*)p;
		p += sizeof(*rect);
		left -= sizeof(*rect);
		pixels = (uint32_t)rect->w * rect->h;
		if (!rect->w || !rect->h || rect->x >= hdr->canvasW || rect->y >= hdr->canvasH ||
		    rect->w > hdr->canvasW - rect->x || rect->h > hdr->canvasH - rect->y ||
		    rect->reserved0 || rect->reserved1 || rect->payloadBytes > left)
			return ImageViewerResultDecodeError;
		payload = p;
		p += rect->payloadBytes;
		left -= rect->payloadBytes;
		if (rect->codec == DCA_CODEC_RAW) {
			if (rect->payloadBytes != pixels)
				return ImageViewerResultDecodeError;
			ok = imagePrvDcaApplyRaw(cnv, palette, rect, payload);
		}
		else if (rect->codec == DCA_CODEC_RLE) {
			ok = imagePrvDcaApplyRle(cnv, palette, rect, payload, rect->payloadBytes);
		}
		else {
			return ImageViewerResultDecodeError;
		}
		if (!ok)
			return ImageViewerResultDecodeError;
	}
	return left ? ImageViewerResultDecodeError : ImageViewerResultBack;
}

static bool imagePrvDcaHandleKeys(uint_fast16_t *prevKeysP, bool *pausedP, enum ImageViewerResult *resultP)
{
	uint_fast16_t keys = uiGetUiKeysRaw(), prev = *prevKeysP;

	if ((keys & UI_KEY_BIT_CENTER) && !(prev & UI_KEY_BIT_CENTER)) {
		*resultP = ImageViewerResultMenu;
		return true;
	}
	if ((keys & KEY_BIT_B) && !(prev & KEY_BIT_B)) {
		*resultP = ImageViewerResultBack;
		return true;
	}
	if ((keys & KEY_BIT_LEFT) && !(prev & KEY_BIT_LEFT)) {
		*resultP = ImageViewerResultPrev;
		return true;
	}
	if ((keys & KEY_BIT_RIGHT) && !(prev & KEY_BIT_RIGHT)) {
		*resultP = ImageViewerResultNext;
		return true;
	}
	if ((keys & KEY_BIT_A) && !(prev & KEY_BIT_A))
		*pausedP = !*pausedP;
	*prevKeysP = keys;
	return false;
}

static enum ImageViewerResult imagePrvRunDcaLoaded(struct Canvas *cnv, uint32_t fileSize)
{
	const uint8_t *base = (const uint8_t*)QSPI_ROM_START;
	const struct ImageDcaHeader *hdr = (const struct ImageDcaHeader*)base;
	const struct ImageDcaFrame *frames;
	uint_fast16_t prevKeys = uiGetUiKeysRaw();
	uint32_t frameIdx = 0, loopsDone = 0;
	uint64_t nextAt = getTime();
	bool paused = false;

	if (!imagePrvValidDca1Header(hdr, fileSize))
		return ImageViewerResultDecodeError;
	frames = (const struct ImageDcaFrame*)(base + hdr->frameTableOffset);
	while (1) {
		enum ImageViewerResult ret = ImageViewerResultBack;

		while (paused || getTime() < nextAt) {
			if (imagePrvDcaHandleKeys(&prevKeys, &paused, &ret))
				return ret;
			if (paused)
				nextAt = getTime();
			badgeLedsTick();
		}
		dispPrvWaitForScanoutStart();
		ret = imagePrvDcaApplyFrame(cnv, base, fileSize, hdr, &frames[frameIdx]);
		if (ret != ImageViewerResultBack)
			return ret;
		nextAt += imagePrvMsToTicks(frames[frameIdx].delayMs);
		frameIdx++;
		if (frameIdx >= hdr->frameCount) {
			frameIdx = 0;
			if (hdr->loopCount && ++loopsDone >= hdr->loopCount)
				break;
		}
		if (imagePrvDcaHandleKeys(&prevKeys, &paused, &ret))
			return ret;
	}
	return imagePrvWaitForStaticInput();
}

static enum ImageViewerResult imagePrvRunDca(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *locator)
{
	struct FatfsFil *fil;
	struct ImageDcaHeader hdr;
	enum ImageViewerResult ret;
	uint32_t fileSize;

	fil = fatfsFileOpenWithLocator(vol, locator, OPEN_MODE_READ);
	if (!fil)
		return ImageViewerResultOpenError;
	fileSize = fatfsFileGetSize(fil);
	if (!imagePrvReadExact(fil, &hdr, sizeof(hdr)) || !imagePrvValidDca1Header(&hdr, fileSize))
		ret = ImageViewerResultDecodeError;
	else if (!imagePrvFlashLoadRange(fil, 0, fileSize, cnv))
		ret = ImageViewerResultReadError;
	else
		ret = imagePrvRunDcaLoaded(cnv, fileSize);
	fatfsFileClose(fil);
	return ret;
}

enum ImageViewerResult imageViewerRun(struct Canvas *cnv, struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *initialLocator, const char *initialName)
{
	(void)rootPath;
	if (!initialLocator || !initialName)
		return ImageViewerResultOpenError;
	if (imagePrvEndsWithNoCase(initialName, ".dca"))
		return imagePrvRunDca(cnv, vol, initialLocator);
	if (imagePrvEndsWithNoCase(initialName, ".dci"))
		return imagePrvRunDci1(cnv, vol, initialLocator);
	return ImageViewerResultUnsupported;
}
