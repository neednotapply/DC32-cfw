#include "apps/port/port_runtime.h"

#include <string.h>
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "ui.h"

#ifdef DC32_OPENJAZZ
#include "apps/openjazz/openjazz_heap.h"

void dc32PortHeapInit(void *base, uint32_t size)
{
	(void)dc32OjHeapInit(base, size, NULL, 0);
}

bool dc32PortHeapAddRegion(void *base, uint32_t size)
{
	(void)base;
	(void)size;
	return false;
}

void dc32PortHeapInitDefault(void)
{
	dc32PortHeapInit(DC32_PORT_CART_HEAP_START, DC32_PORT_CART_HEAP_SIZE);
}

void *dc32PortMalloc(size_t size)
{
	return dc32OjHeapMalloc(size);
}

void *dc32PortCalloc(size_t nmemb, size_t size)
{
	return dc32OjHeapCalloc(nmemb, size);
}

void *dc32PortRealloc(void *ptr, size_t size)
{
	return dc32OjHeapRealloc(ptr, size);
}

void dc32PortFree(void *ptr)
{
	dc32OjHeapFree(ptr);
}

uint32_t dc32PortHeapBytesUsed(void)
{
	return dc32OjHeapBytesUsed(dc32OjHeapSelected());
}

uint32_t dc32PortHeapPeakBytesUsed(void)
{
	return dc32OjHeapPeakBytesUsed(dc32OjHeapSelected());
}

uint32_t dc32PortHeapBytesFree(void)
{
	return dc32OjHeapBytesFree(dc32OjHeapSelected());
}

uint32_t dc32PortHeapLargestFreeBlock(void)
{
	return dc32OjHeapLargestFreeBlock(dc32OjHeapSelected());
}

#else
struct Dc32PortBlock {
	uint32_t size;
	struct Dc32PortBlock *next;
	bool free;
};

static struct Dc32PortBlock *mPortHeap;
static struct Dc32PortBlock *mPortHeapTail;

static uint32_t dc32PortAlignUp(uint32_t value, uint32_t align)
{
	return (value + align - 1u) & ~(align - 1u);
}

void dc32PortHeapInit(void *base, uint32_t size)
{
	uintptr_t start = dc32PortAlignUp((uint32_t)(uintptr_t)base, 8u);
	uintptr_t end = ((uint32_t)(uintptr_t)base + size) & ~(uintptr_t)7u;

	mPortHeap = NULL;
	mPortHeapTail = NULL;
	if (end <= start)
		return;
	(void)dc32PortHeapAddRegion((void*)start, (uint32_t)(end - start));
}

bool dc32PortHeapAddRegion(void *base, uint32_t size)
{
	uintptr_t start = dc32PortAlignUp((uint32_t)(uintptr_t)base, 8u);
	uintptr_t end = ((uint32_t)(uintptr_t)base + size) & ~(uintptr_t)7u;
	struct Dc32PortBlock *block;

	if (end <= start + sizeof(*block))
		return false;
	block = (struct Dc32PortBlock*)start;
	block->size = (uint32_t)(end - start - sizeof(*block));
	block->next = NULL;
	block->free = true;
	if (mPortHeapTail)
		mPortHeapTail->next = block;
	else
		mPortHeap = block;
	mPortHeapTail = block;
	return true;
}

void dc32PortHeapInitDefault(void)
{
	dc32PortHeapInit(DC32_PORT_CART_HEAP_START, DC32_PORT_CART_HEAP_SIZE);
}

static void dc32PortSplitBlock(struct Dc32PortBlock *block, uint32_t size)
{
	struct Dc32PortBlock *next;
	uint32_t remain;

	if (block->size < size + sizeof(*next) + 16u)
		return;
	remain = block->size - size - sizeof(*next);
	next = (struct Dc32PortBlock*)((uint8_t*)(block + 1) + size);
	next->size = remain;
	next->next = block->next;
	next->free = true;
	block->size = size;
	block->next = next;
}

void *dc32PortMalloc(size_t size)
{
	struct Dc32PortBlock *block;
	uint32_t wanted;

	if (!mPortHeap)
		dc32PortHeapInitDefault();
	if (!size)
		size = 1;
	if (size > UINT32_MAX - 7u)
		return NULL;
	wanted = dc32PortAlignUp((uint32_t)size, 8u);
	for (block = mPortHeap; block; block = block->next) {
		if (!block->free || block->size < wanted)
			continue;
		dc32PortSplitBlock(block, wanted);
		block->free = false;
		return block + 1;
	}
	return NULL;
}

void *dc32PortCalloc(size_t nmemb, size_t size)
{
	size_t total = nmemb * size;
	void *ptr;

	if (size && total / size != nmemb)
		return NULL;
	ptr = dc32PortMalloc(total);
	if (ptr)
		memset(ptr, 0, total);
	return ptr;
}

static void dc32PortCoalesce(void)
{
	struct Dc32PortBlock *block;

	for (block = mPortHeap; block && block->next;) {
		uint8_t *blockEnd = (uint8_t*)(block + 1) + block->size;

		if (block->free && block->next->free && blockEnd == (uint8_t*)block->next) {
			block->size += sizeof(*block) + block->next->size;
			block->next = block->next->next;
			if (!block->next)
				mPortHeapTail = block;
		}
		else {
			block = block->next;
		}
	}
}

void dc32PortFree(void *ptr)
{
	struct Dc32PortBlock *block;

	if (!ptr)
		return;
	for (block = mPortHeap; block; block = block->next)
		if (block + 1 == ptr)
			break;
	if (!block)
		return;
	block->free = true;
	dc32PortCoalesce();
}

void *dc32PortRealloc(void *ptr, size_t size)
{
	struct Dc32PortBlock *block;
	void *next;

	if (!ptr)
		return dc32PortMalloc(size);
	if (!size) {
		dc32PortFree(ptr);
		return NULL;
	}
	for (block = mPortHeap; block; block = block->next)
		if (block + 1 == ptr)
			break;
	if (!block)
		return NULL;
	if (block->size >= size)
		return ptr;
	next = dc32PortMalloc(size);
	if (next) {
		memcpy(next, ptr, block->size);
		dc32PortFree(ptr);
	}
	return next;
}

uint32_t dc32PortHeapBytesUsed(void)
{
	return 0;
}

uint32_t dc32PortHeapPeakBytesUsed(void)
{
	return 0;
}

uint32_t dc32PortHeapBytesFree(void)
{
	uint32_t freeBytes = 0;

	for (struct Dc32PortBlock *block = mPortHeap; block; block = block->next)
		if (block->free)
			freeBytes += block->size;
	return freeBytes;
}

uint32_t dc32PortHeapLargestFreeBlock(void)
{
	uint32_t largest = 0;

	for (struct Dc32PortBlock *block = mPortHeap; block; block = block->next)
		if (block->free && block->size > largest)
			largest = block->size;
	return largest;
}

#endif

uint16_t dc32PortRgb332ToRgb565(uint8_t color)
{
	uint32_t r = color >> 5;
	uint32_t g = (color >> 2) & 7u;
	uint32_t b = color & 3u;

	r = (r << 2) | (r >> 1);
	g = (g << 3) | g;
	b = (b << 3) | (b << 1) | (b >> 1);
	return (uint16_t)((r << 11) | (g << 5) | b);
}

void dc32PortPresentRgb332(struct DcAppDrawCtx *draw, const uint8_t *src, uint32_t w, uint32_t h)
{
	uint32_t srcStride;
	uint32_t rows;

	if (!draw || !draw->fb || !src || !w || !h)
		return;
	srcStride = w;
	rows = h < draw->h ? h : draw->h;
	if (w > draw->w)
		w = draw->w;
	for (uint32_t y = 0; y < rows; y++)
		memcpy(draw->fb + y * draw->w, src + y * srcStride, w);
}

void dc32PortPresentIndexed8(struct DcAppDrawCtx *draw, const uint8_t *src, uint32_t w, uint32_t h, const uint8_t paletteRgb332[256])
{
	uint32_t srcStride;
	uint32_t rows;

	if (!draw || !draw->fb || !src || !paletteRgb332 || !w || !h)
		return;
	srcStride = w;
	rows = h < draw->h ? h : draw->h;
	if (w > draw->w)
		w = draw->w;
	for (uint32_t y = 0; y < rows; y++) {
		uint8_t *dst = draw->fb + y * draw->w;
		const uint8_t *row = src + y * srcStride;

		for (uint32_t x = 0; x < w; x++)
			dst[x] = paletteRgb332[row[x]];
	}
}

bool dc32PortOpenAssetPack(struct FatfsVol *vol, const char *path, struct Dc32PortPak *pak)
{
	struct FatfsFil *file;

	if (!vol || !path || !pak)
		return false;
	memset(pak, 0, sizeof(*pak));
	file = fatfsFileOpen(vol, path, OPEN_MODE_READ);
	if (!file)
		return false;
	pak->file = file;
	pak->size = fatfsFileGetSize(file);
	return true;
}

bool dc32PortReadAssetPack(struct Dc32PortPak *pak, uint32_t offset, void *dst, uint32_t size)
{
	uint32_t got = 0;

	if (!pak || !pak->file || !dst)
		return false;
	if (offset > pak->size || size > pak->size - offset)
		return false;
	if ((!pak->positionValid || pak->position != offset) &&
			!fatfsFileSeek(pak->file, offset)) {
		pak->positionValid = false;
		return false;
	}
	if (!fatfsFileRead(pak->file, dst, size, &got)) {
		pak->positionValid = false;
		return false;
	}
	pak->position = offset + got;
	pak->positionValid = true;
	return got == size;
}

void dc32PortCloseAssetPack(struct Dc32PortPak *pak)
{
	if (!pak || !pak->file)
		return;
	(void)fatfsFileClose(pak->file);
	memset(pak, 0, sizeof(*pak));
}

bool dc32PortEnsureSaveDir(struct FatfsVol *vol)
{
	struct FatfsDir *rootDir, *portDir = NULL;
	struct FatFileLocator loc;

	if (!vol)
		return false;

	rootDir = fatfsDirOpen(vol, "/SAVE");
	if (!rootDir) {
		if (!fatfsDirCreate(vol, "/SAVE", &loc))
			return false;
		rootDir = fatfsDirOpenWithLocator(vol, &loc);
	}
	if (!rootDir)
		return false;

	portDir = fatfsDirOpenAt(rootDir, "PORTS");
	if (!portDir) {
		if (fatfsDirCreateAt(rootDir, "PORTS", &loc))
			portDir = fatfsDirOpenWithLocator(vol, &loc);
	}
	(void)fatfsDirClose(rootDir);
	if (!portDir)
		return false;
	(void)fatfsDirClose(portDir);
	return true;
}

static bool dc32PortSavePathWithPrefix(char *dst, uint32_t dstLen, const char *prefix, const char *appName)
{
	uint32_t pos = 0;
	static const char suffix[] = ".sav";

	if (!dst || !dstLen || !prefix || !appName || !*appName)
		return false;
	for (uint32_t i = 0; prefix[i]; i++) {
		if (pos + 1 >= dstLen)
			return false;
		dst[pos++] = prefix[i];
	}
	while (*appName) {
		char ch = *appName++;

		if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-'))
			return false;
		if (pos + 1 >= dstLen)
			return false;
		dst[pos++] = ch;
	}
	for (uint32_t i = 0; suffix[i]; i++) {
		if (pos + 1 >= dstLen)
			return false;
		dst[pos++] = suffix[i];
	}
	dst[pos] = 0;
	return true;
}

static bool dc32PortSavePath(char *dst, uint32_t dstLen, const char *appName)
{
	return dc32PortSavePathWithPrefix(dst, dstLen, "/SAVE/PORTS/", appName);
}

static bool dc32PortLegacySavePath(char *dst, uint32_t dstLen, const char *appName)
{
	return dc32PortSavePathWithPrefix(dst, dstLen, "/SAVE/", appName);
}

bool dc32PortSaveRead(struct FatfsVol *vol, const char *appName, void *dst, uint32_t size)
{
	char path[48];
	struct FatfsFil *file;
	uint32_t got = 0;
	bool ok;

	if (!vol || !dc32PortSavePath(path, sizeof(path), appName) || !dst)
		return false;
	file = fatfsFileOpen(vol, path, OPEN_MODE_READ);
	if (!file && dc32PortLegacySavePath(path, sizeof(path), appName))
		file = fatfsFileOpen(vol, path, OPEN_MODE_READ);
	if (!file)
		return false;
	ok = fatfsFileRead(file, dst, size, &got) && got == size;
	(void)fatfsFileClose(file);
	return ok;
}

bool dc32PortSaveWrite(struct FatfsVol *vol, const char *appName, const void *src, uint32_t size)
{
	char path[48];
	struct FatfsFil *file;
	uint32_t wrote = 0;
	bool ok;

	if (!vol || !dc32PortSavePath(path, sizeof(path), appName) || !src)
		return false;
	if (!dc32PortEnsureSaveDir(vol))
		return false;
	file = fatfsFileOpen(vol, path, OPEN_MODE_CREATE | OPEN_MODE_WRITE | OPEN_MODE_TRUNCATE);
	if (!file)
		return false;
	ok = fatfsFileWrite(file, src, size, &wrote) && wrote == size;
	(void)fatfsFileClose(file);
	return ok;
}

bool dc32PortCenterExitRequested(const struct DcAppHostApi *host)
{
	return host && host->uiKeysRaw && (host->uiKeysRaw() & UI_KEY_BIT_CENTER);
}

static uint32_t dc32PortTextWidth(const char *text, enum Font font)
{
	uint32_t width = 0;

	if (!text)
		return 0;
	while (*text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text))
			width += glyph.width + 1u;
		text++;
	}
	return width ? width - 1u : 0;
}

static void dc32PortDrawText(struct DcAppDrawCtx *draw, int32_t x, int32_t y,
	const char *text, enum Font font, uint16_t color)
{
	if (!text)
		return;
	while (*text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text)) {
			for (uint_fast8_t row = 0; row < glyph.height; row++)
				for (uint_fast8_t col = 0; col < glyph.width; col++)
					if (fontGetGlyphPixel(&glyph, row, col))
						dcAppDrawPixel(draw, x + (int32_t)col, y + (int32_t)row, color);
			x += glyph.width + 1;
		}
		text++;
	}
}

static void dc32PortDrawCenteredText(struct DcAppDrawCtx *draw, int32_t y,
	const char *text, enum Font font, uint16_t color)
{
	uint32_t width = dc32PortTextWidth(text, font);
	int32_t x = (int32_t)((DC32_PORT_SCREEN_W > width ? DC32_PORT_SCREEN_W - width : 0u) / 2u);

	dc32PortDrawText(draw, x, y, text, font, color);
}

void dc32PortShowMissingData(const struct DcAppHostApi *host, const struct DcAppRunArgs *args,
	const char *title, const char *path, void *backbuffer)
{
	struct DcAppDrawCtx draw;

	if (!dcAppDrawInit(&draw, host, args, backbuffer, DC32_PORT_SCREEN_W, DC32_PORT_SCREEN_H))
		return;
	dcAppDrawWaitRelease(&draw, UI_KEY_BIT_CENTER);
	while (dcAppDrawFrame(&draw, UI_KEY_BIT_CENTER)) {
		dcAppDrawClear(&draw, dcAppDrawRgb565(8, 10, 18));
		dc32PortDrawCenteredText(&draw, 68, title ? title : "Missing data", FontLarge, dcAppDrawRgb565(255, 255, 255));
		dc32PortDrawCenteredText(&draw, 104, "Install required file:", FontMedium, dcAppDrawRgb565(180, 210, 255));
		dc32PortDrawCenteredText(&draw, 126, path ? path : "/APPS/app.pak", FontMedium, dcAppDrawRgb565(255, 210, 120));
		dc32PortDrawCenteredText(&draw, 162, "Center exits", FontMedium, dcAppDrawRgb565(180, 180, 180));
	}
	dcAppDrawWaitRelease(&draw, UI_KEY_BIT_CENTER);
}
