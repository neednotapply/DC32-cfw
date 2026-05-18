#pragma GCC optimize ("Os")
#include <stdint.h>
#include <string.h>
#include "badgeLeds.h"
#include "dispDefcon.h"
#include "fatfs.h"
#include "gb.h"
#include "imageViewer.h"
#include "imageViewerAnimatedGif.h"
#include "ui.h"

#define DCI1_MAGIC              0x31494344u
#define DCI_HEADER_SIZE         64u
#define DCI1_VERSION            1u
#define DCI_KIND_STATIC         1u
#define DCI_BPP_RGB565          16u
#define DCI_CANVAS_W            320u
#define DCI_CANVAS_H            240u
#define DCI_FRAME_BYTES         (DCI_CANVAS_W * DCI_CANVAS_H * 2u)

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

typedef char ImageDciHeaderSizeCheck[(sizeof(struct ImageDciHeader) == DCI_HEADER_SIZE) ? 1 : -1];

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
	return imagePrvEndsWithNoCase(name, ".dci") || imagePrvEndsWithNoCase(name, ".gif");
}

static bool imagePrvReadExact(struct FatfsFil *fil, void *buf, uint32_t size)
{
	uint32_t numRead = 0;

	return fatfsFileRead(fil, buf, size, &numRead) && numRead == size;
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

static enum ImageViewerResult imagePrvShowDci1Static(struct Canvas *cnv, struct FatfsFil *fil)
{
	bool held;
	enum ImageViewerResult ret = ImageViewerResultBack;

	if (!fatfsFileSeek(fil, DCI_HEADER_SIZE))
		return ImageViewerResultReadError;
	held = dispHoldScanoutBegin();
	if (!imagePrvReadExact(fil, cnv->framebuffer, DCI_FRAME_BYTES))
		ret = ImageViewerResultReadError;
	if (held)
		dispHoldScanoutEnd();
	if (ret != ImageViewerResultBack)
		return ret;
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

enum ImageViewerResult imageViewerRun(struct Canvas *cnv, struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *initialLocator, const char *initialName)
{
	(void)rootPath;
	if (!initialLocator || !initialName)
		return ImageViewerResultOpenError;
	if (imagePrvEndsWithNoCase(initialName, ".gif"))
		return imageViewerAnimatedGifRun(cnv, vol, initialLocator);
	if (imagePrvEndsWithNoCase(initialName, ".dci"))
		return imagePrvRunDci1(cnv, vol, initialLocator);
	return ImageViewerResultUnsupported;
}
