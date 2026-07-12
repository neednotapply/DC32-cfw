#pragma GCC optimize ("Os")
#include <stdint.h>
#include <string.h>
#include "badgeLeds.h"
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "fatfs.h"
#include "fonts.h"
#include "gb.h"
#include "imageViewer.h"
#include "memMap.h"
#include "picojpeg.h"
#include "qspi.h"
#include "raster.h"
#include "sd.h"
#include "timebase.h"
#include "toolWorkspace.h"
#include "ui.h"

#define IMAGE_SD_SAFE_HZ        500000u
#define IMAGE_SD_FAST_HZ        2000000u
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
#define DC32_GIF_TRAILER_MAGIC  "DC32GIF1"
#define DC32_GIF_TRAILER_SIZE   8u
#define IMAGE_DIRECT_MAX_W       2048u
#define IMAGE_DIRECT_MAX_H       2048u

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
static bool mImageAutoAdvance;

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
	return imagePrvEndsWithNoCase(name, ".gif") || imagePrvEndsWithNoCase(name, ".dci") || imagePrvEndsWithNoCase(name, ".dca") ||
		imagePrvEndsWithNoCase(name, ".jpg") || imagePrvEndsWithNoCase(name, ".jpeg") ||
		imagePrvEndsWithNoCase(name, ".bmp");
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

	if (mImageAutoAdvance)
		return ImageViewerResultNext;

	while (1) {
		bool handled;
		enum ImageViewerResult ret = imagePrvPollKeys(&prevKeys, &handled);

		if (handled)
			return ret;
		badgeLedsTick();
		timebaseIdleWaitMsec(10u);
	}
}

static bool imagePrvFlashLoadRangeRaw(struct FatfsFil *fil, uint32_t fileOffset, uint32_t size, struct Canvas *slowLoadCnv)
{
	struct ToolWorkspaceSpan mem;
	uint32_t bufSz, pos;
	uint8_t *buf;
	struct DcAppLoadingState loading = {
		.appName = "Image Viewer",
		.title = "Loading image",
		.detail = "Preparing image data",
		.total = size,
	};
	bool ret = false;

	if (slowLoadCnv) {
		dispPrvWaitForScanoutStart();
		dcAppDrawLoadingCanvas(slowLoadCnv, &loading);
	}

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
		if (slowLoadCnv) {
			loading.done = pos;
			loading.animationStep++;
			dispPrvWaitForScanoutStart();
			dcAppDrawLoadingCanvas(slowLoadCnv, &loading);
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

	if (!imagePrvFlashLoadRange(fil, DCI_HEADER_SIZE, DCI_FRAME_BYTES, cnv))
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

static uint16_t imagePrvRgb555To565(uint16_t v)
{
	uint32_t r = (v >> 10) & 0x1fu;
	uint32_t g = (v >> 5) & 0x1fu;
	uint32_t b = v & 0x1fu;

	return (uint16_t)(r << 11) | (uint16_t)(((g << 1) | (g >> 4)) << 5) | (uint16_t)b;
}

static uint16_t imagePrvReadLe16(const uint8_t *p)
{
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t imagePrvReadLe32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int32_t imagePrvReadLe32s(const uint8_t *p)
{
	return (int32_t)imagePrvReadLe32(p);
}

static enum ImageViewerResult imagePrvRunBmp(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *locator)
{
	struct FatfsFil *fil;
	struct ToolWorkspaceSpan mem;
	uint8_t header[54];
	uint32_t dataOffset, dibSize, width, height, compression, rowStride;
	uint16_t planes, bpp;
	int32_t signedW, signedH;
	bool topDown;
	enum ImageViewerResult ret = ImageViewerResultDecodeError;
	uint8_t *rowBuf;
	struct RasterRect fit;

	fil = fatfsFileOpenWithLocator(vol, locator, OPEN_MODE_READ);
	if (!fil)
		return ImageViewerResultOpenError;
	if (!imagePrvReadExact(fil, header, sizeof(header)))
		goto out_file;
	if (header[0] != 'B' || header[1] != 'M')
		goto out_file;
	dataOffset = imagePrvReadLe32(header + 10);
	dibSize = imagePrvReadLe32(header + 14);
	signedW = imagePrvReadLe32s(header + 18);
	signedH = imagePrvReadLe32s(header + 22);
	planes = imagePrvReadLe16(header + 26);
	bpp = imagePrvReadLe16(header + 28);
	compression = imagePrvReadLe32(header + 30);
	if (dibSize < 40u || signedW <= 0 || signedH == 0 || planes != 1u || compression != 0u)
		goto out_file;
	width = (uint32_t)signedW;
	topDown = signedH < 0;
	height = (uint32_t)(topDown ? -signedH : signedH);
	if (!width || !height || width > IMAGE_DIRECT_MAX_W || height > IMAGE_DIRECT_MAX_H ||
			(bpp != 16u && bpp != 24u && bpp != 32u))
		goto out_file;
	rowStride = (((width * bpp) + 31u) / 32u) * 4u;
	if (!rowStride || rowStride > fatfsFileGetSize(fil) || dataOffset > fatfsFileGetSize(fil) ||
			(uint64_t)dataOffset + (uint64_t)rowStride * height > fatfsFileGetSize(fil)) {
		ret = ImageViewerResultTooLarge;
		goto out_file;
	}
	if (!toolWorkspaceAcquire(ToolWorkspaceWram, ToolWorkspaceOwnerImage, &mem)) {
		ret = ImageViewerResultNoMemory;
		goto out_file;
	}
	if (rowStride > mem.size) {
		ret = ImageViewerResultTooLarge;
		goto out_mem;
	}
	rowBuf = (uint8_t*)mem.ptr;
	fit = rasterFit(width, height, cnv->w, cnv->h);
	dispPrvWaitForScanoutStart();
	rasterClear(cnv, 0);
	for (uint32_t y = 0; y < height; y++) {
		uint32_t fileY = topDown ? y : height - 1u - y;

		if (!fatfsFileSeek(fil, dataOffset + fileY * rowStride) ||
				!imagePrvReadExact(fil, rowBuf, rowStride)) {
			ret = ImageViewerResultReadError;
			goto out_mem;
		}
		for (uint32_t x = 0; x < width; x++) {
			uint16_t color;

			if (bpp == 16u) {
				uint16_t v = imagePrvReadLe16(rowBuf + x * 2u);

				color = imagePrvRgb555To565(v);
			}
			else if (bpp == 24u) {
				const uint8_t *p = rowBuf + x * 3u;

				color = rasterRgb565(p[2], p[1], p[0]);
			}
			else {
				const uint8_t *p = rowBuf + x * 4u;

				color = rasterRgb565(p[2], p[1], p[0]);
			}
			rasterDrawScaledPixel(cnv, &fit, width, height, x, y, color);
		}
		if (!(y & 7u))
			badgeLedsTick();
	}
	ret = imagePrvWaitForStaticInput();
out_mem:
	toolWorkspaceRelease(ToolWorkspaceWram, ToolWorkspaceOwnerImage);
out_file:
	fatfsFileClose(fil);
	return ret;
}

struct ImageJpegStream {
	struct FatfsFil *fil;
	bool readError;
};

static unsigned char imagePrvJpegNeedBytes(unsigned char *buf, unsigned char bufSize, unsigned char *bytesReadP, void *userData)
{
	struct ImageJpegStream *stream = (struct ImageJpegStream*)userData;
	uint32_t got = 0;

	if (!fatfsFileRead(stream->fil, buf, bufSize, &got)) {
		stream->readError = true;
		got = 0;
	}
	*bytesReadP = (unsigned char)got;
	return 0;
}

static enum ImageViewerResult imagePrvRunJpeg(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *locator)
{
	struct FatfsFil *fil;
	struct ImageJpegStream stream;
	pjpeg_image_info_t info;
	uint8_t status;
	struct RasterRect fit;
	uint32_t mcuTotal, mcuIdx;

	fil = fatfsFileOpenWithLocator(vol, locator, OPEN_MODE_READ);
	if (!fil)
		return ImageViewerResultOpenError;
	memset(&stream, 0, sizeof(stream));
	stream.fil = fil;
	memset(&info, 0, sizeof(info));
	status = pjpeg_decode_init(&info, imagePrvJpegNeedBytes, &stream, 0);
	if (stream.readError) {
		fatfsFileClose(fil);
		return ImageViewerResultReadError;
	}
	if (status) {
		fatfsFileClose(fil);
		return status == PJPG_NOTENOUGHMEM ? ImageViewerResultNoMemory : ImageViewerResultDecodeError;
	}
	if (info.m_width <= 0 || info.m_height <= 0 ||
			(uint32_t)info.m_width > IMAGE_DIRECT_MAX_W || (uint32_t)info.m_height > IMAGE_DIRECT_MAX_H) {
		fatfsFileClose(fil);
		return ImageViewerResultTooLarge;
	}
	fit = rasterFit((uint32_t)info.m_width, (uint32_t)info.m_height, cnv->w, cnv->h);
	dispPrvWaitForScanoutStart();
	rasterClear(cnv, 0);
	mcuTotal = (uint32_t)info.m_MCUSPerRow * (uint32_t)info.m_MCUSPerCol;
	for (mcuIdx = 0; mcuIdx < mcuTotal; mcuIdx++) {
		uint32_t mcuX = (mcuIdx % (uint32_t)info.m_MCUSPerRow) * (uint32_t)info.m_MCUWidth;
		uint32_t mcuY = (mcuIdx / (uint32_t)info.m_MCUSPerRow) * (uint32_t)info.m_MCUHeight;
		uint32_t blocksPerRow = (uint32_t)info.m_MCUWidth / 8u;

		status = pjpeg_decode_mcu();
		if (status) {
			fatfsFileClose(fil);
			return stream.readError ? ImageViewerResultReadError : ImageViewerResultDecodeError;
		}
		for (uint32_t y = 0; y < (uint32_t)info.m_MCUHeight; y++) {
			uint32_t srcY = mcuY + y;

			if (srcY >= (uint32_t)info.m_height)
				continue;
			for (uint32_t x = 0; x < (uint32_t)info.m_MCUWidth; x++) {
				uint32_t srcX = mcuX + x;
				uint32_t block = (y / 8u) * blocksPerRow + x / 8u;
				uint32_t off = block * 64u + (y & 7u) * 8u + (x & 7u);
				uint8_t r, g, b;

				if (srcX >= (uint32_t)info.m_width)
					continue;
				r = info.m_pMCUBufR[off];
				g = info.m_comps == 1 ? r : info.m_pMCUBufG[off];
				b = info.m_comps == 1 ? r : info.m_pMCUBufB[off];
				rasterDrawScaledPixel(cnv, &fit, (uint32_t)info.m_width, (uint32_t)info.m_height,
					srcX, srcY, rasterRgb565(r, g, b));
			}
		}
		if (!(mcuIdx & 7u))
			badgeLedsTick();
	}
	fatfsFileClose(fil);
	return imagePrvWaitForStaticInput();
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

static enum ImageViewerResult imagePrvRunDcaLoaded(struct Canvas *cnv, const uint8_t *base, uint32_t fileSize)
{
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
			timebaseIdleWaitMsec(paused ? 10u : 1u);
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
		ret = imagePrvRunDcaLoaded(cnv, (const uint8_t*)QSPI_ROM_START, fileSize);
	fatfsFileClose(fil);
	return ret;
}

static enum ImageViewerResult imagePrvRunDc32Gif(struct Canvas *cnv, struct FatfsVol *vol,
	const struct FatFileLocator *locator)
{
	struct FatfsFil *fil;
	const uint8_t *base;
	uint32_t fileSize;
	enum ImageViewerResult result = ImageViewerResultIncompatibleGif;

	fil = fatfsFileOpenWithLocator(vol, locator, OPEN_MODE_READ);
	if (!fil)
		return ImageViewerResultOpenError;
	fileSize = fatfsFileGetSize(fil);
	if (fileSize > QSPI_ROM_SIZE_MAX || !imagePrvFlashLoadRange(fil, 0, fileSize, cnv))
		result = ImageViewerResultReadError;
	else {
		base = (const uint8_t*)QSPI_ROM_START;
		for (uint32_t off = 0; off + DC32_GIF_TRAILER_SIZE < fileSize; off++) {
			const uint8_t *payload;
			uint32_t payloadSize;

			if (memcmp(base + off, DC32_GIF_TRAILER_MAGIC, DC32_GIF_TRAILER_SIZE))
				continue;
			payload = base + off + DC32_GIF_TRAILER_SIZE;
			payloadSize = fileSize - off - DC32_GIF_TRAILER_SIZE;
			if (payloadSize >= sizeof(struct ImageDcaHeader) &&
					imagePrvValidDca1Header((const struct ImageDcaHeader*)payload, payloadSize))
				result = imagePrvRunDcaLoaded(cnv, payload, payloadSize);
			break;
		}
	}
	fatfsFileClose(fil);
	return result;
}

enum ImageViewerResult imageViewerRun(struct Canvas *cnv, struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *initialLocator, const char *initialName)
{
	(void)rootPath;
	if (!initialLocator || !initialName)
		return ImageViewerResultOpenError;
	if (imagePrvEndsWithNoCase(initialName, ".gif"))
		return imagePrvRunDc32Gif(cnv, vol, initialLocator);
	if (imagePrvEndsWithNoCase(initialName, ".dca"))
		return imagePrvRunDca(cnv, vol, initialLocator);
	if (imagePrvEndsWithNoCase(initialName, ".dci"))
		return imagePrvRunDci1(cnv, vol, initialLocator);
	if (imagePrvEndsWithNoCase(initialName, ".bmp"))
		return imagePrvRunBmp(cnv, vol, initialLocator);
	if (imagePrvEndsWithNoCase(initialName, ".jpg") || imagePrvEndsWithNoCase(initialName, ".jpeg"))
		return imagePrvRunJpeg(cnv, vol, initialLocator);
	return ImageViewerResultUnsupported;
}

enum ImageViewerResult imageViewerRunStill(struct Canvas *cnv, struct FatfsVol *vol, const char *rootPath,
	const struct FatFileLocator *initialLocator, const char *initialName)
{
	enum ImageViewerResult result;

	mImageAutoAdvance = true;
	dispPauseScanout();
	result = imageViewerRun(cnv, vol, rootPath, initialLocator, initialName);
	dispResumeScanout();
	mImageAutoAdvance = false;
	return result;
}
