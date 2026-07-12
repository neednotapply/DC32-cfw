#include "apps/openjazz/openjazz_install.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "apps/openjazz/openjazz_pack.h"
#include "apps/port/port_runtime.h"
extern "C" {
#include "dcApp.h"
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "fatfs.h"
#include "fonts.h"
#include "gb.h"
}
#include "miniz.h"

static constexpr uint32_t OJ_HEADER_SIZE = 20u;
static constexpr uint32_t OJ_ENTRY_SIZE = 16u;
static constexpr uint32_t OJ_VERSION = 1u;
static constexpr uint32_t OJ_MAX_ZIP_ENTRIES = 512u;
static constexpr uint32_t OJ_MAX_NAME = 12u;

struct OjInstallEntry {
	char name[OJ_MAX_NAME + 1u];
	uint32_t zipIndex;
	uint32_t size;
	uint32_t crc;
	uint32_t offset;
};

struct OjWriteContext {
	struct FatfsFil *file;
	uint32_t base;
};

static const char *const gOjRequired[] = {
	"PANEL.000", "MENU.000", "MAINCHAR.000",
	"FONT2.0FN", "FONTBIG.0FN", "FONTINY.0FN", "FONTMN1.0FN", "FONTMN2.0FN",
	"STARTUP.0SC", "SOUNDS.000",
	"LEVEL0.000", "LEVEL1.000", "LEVEL2.000",
	"BLOCKS.000", "SPRITES.000", "PLANET.000",
};

static const struct DcAppHostApi *gOjLoadingHost;
static const struct DcAppRunArgs *gOjLoadingArgs;
static char gOjLoadingStage[64];
static char gOjLoadingAsset[64];
static uint32_t gOjLoadingStep;
static uint32_t gOjLoadingEntry;
static uint32_t gOjLoadingEntryTotal;
static uint32_t gOjLoadingBytes;
static uint32_t gOjLoadingByteTotal;
static uint64_t gOjLoadingLastDraw;
static bool gOjLoadingActive;

static uint32_t ojDisplayIndex(const struct Canvas *cnv, uint32_t x, uint32_t y)
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

static struct Canvas ojCanvas(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct Canvas cnv = {};

	if (args && args->canvas)
		cnv = *args->canvas;
	if (!cnv.framebuffer && host && host->displayFb)
		cnv.framebuffer = host->displayFb();
	if (!cnv.w)
		cnv.w = 320u;
	if (!cnv.h)
		cnv.h = 240u;
	if (!cnv.bpp)
		cnv.bpp = 16u;
	if (!args || !args->canvas)
		cnv.rotated = 1u;
	if (args && args->rotate)
		cnv.flipped = 1u;
	return cnv;
}

static void ojPixel(const struct Canvas *cnv, int32_t x, int32_t y, uint16_t color)
{
	uint16_t *fb;

	if (!cnv || !cnv->framebuffer || cnv->bpp != 16u ||
			x < 0 || y < 0 || x >= 320 || y >= 240)
		return;
	fb = (uint16_t*)cnv->framebuffer;
	fb[ojDisplayIndex(cnv, (uint32_t)x, (uint32_t)y)] = color;
}

static uint32_t ojTextWidth(const char *text, enum Font font)
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

static void ojText(const struct Canvas *cnv, int32_t x, int32_t y,
	const char *text, enum Font font, uint16_t color)
{
	while (text && *text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text)) {
			for (uint_fast8_t row = 0; row < glyph.height; row++)
				for (uint_fast8_t col = 0; col < glyph.width; col++)
					if (fontGetGlyphPixel(&glyph, row, col))
						ojPixel(cnv, x + col, y + row, color);
			x += glyph.width + 1;
		}
		text++;
	}
}

static void ojCentered(const struct Canvas *cnv, int32_t y,
	const char *text, enum Font font, uint16_t color)
{
	uint32_t width = ojTextWidth(text, font);

	ojText(cnv, (int32_t)((320u - (width < 320u ? width : 320u)) / 2u), y, text, font, color);
}

static void ojFill(const struct Canvas *cnv, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
	if (cnv && cnv->framebuffer && cnv->bpp == 16u &&
			x == 0 && y == 0 && w == 320 && h == 240) {
		uint16_t *fb = (uint16_t*)cnv->framebuffer;

		for (uint32_t i = 0; i < 320u * 240u; i++)
			fb[i] = color;
		return;
	}
	for (int32_t yy = y; yy < y + h; yy++)
		for (int32_t xx = x; xx < x + w; xx++)
			ojPixel(cnv, xx, yy, color);
}

static void ojBeginDraw(const struct DcAppHostApi *host)
{
	if (host && host->ledsTick)
		host->ledsTick();
	dispPrvWaitForScanoutStart();
}

static void ojCopyLoadingText(char *dst, size_t size, const char *src)
{
	size_t length;

	if (!size)
		return;
	if (!src)
		src = "";
	length = strlen(src);
	if (length >= size)
		length = size - 1u;
	memcpy(dst, src, length);
	dst[length] = 0;
}

static void ojDrawLoading(bool force)
{
	struct Canvas cnv;
	char detail[80];
	struct DcAppLoadingState loading = {};
	uint64_t now = gOjLoadingHost && gOjLoadingHost->getTime ?
		gOjLoadingHost->getTime() : 0;

	if (!gOjLoadingActive || !gOjLoadingHost)
		return;
	if (!force && gOjLoadingLastDraw && now &&
			now - gOjLoadingLastDraw < TICKS_PER_SECOND / 10u)
		return;
	gOjLoadingLastDraw = now;
	cnv = ojCanvas(gOjLoadingHost, gOjLoadingArgs);
	if (gOjLoadingEntryTotal)
		snprintf(detail, sizeof(detail), "%s %u/%u",
			gOjLoadingAsset[0] ? gOjLoadingAsset : "Caching",
			(unsigned int)gOjLoadingEntry,
			(unsigned int)gOjLoadingEntryTotal);
	else
		ojCopyLoadingText(detail, sizeof(detail),
			gOjLoadingAsset[0] ? gOjLoadingAsset : "Please wait");
	loading.appName = "OpenJazz";
	loading.title = gOjLoadingStage[0] ? gOjLoadingStage : "Starting";
	loading.detail = detail;
	loading.done = gOjLoadingByteTotal ? gOjLoadingBytes : 0u;
	loading.total = gOjLoadingByteTotal;
	loading.animationStep = gOjLoadingStep;
	ojBeginDraw(gOjLoadingHost);
	dcAppDrawLoadingCanvas(&cnv, &loading);
	gOjLoadingStep++;
}

void dc32OjLoadingStart(const struct DcAppHostApi *host, const struct DcAppRunArgs *args,
	const char *stage)
{
	gOjLoadingHost = host;
	gOjLoadingArgs = args;
	ojCopyLoadingText(gOjLoadingStage, sizeof(gOjLoadingStage), stage);
	gOjLoadingAsset[0] = 0;
	gOjLoadingStep = 0;
	gOjLoadingEntry = gOjLoadingEntryTotal = 0;
	gOjLoadingBytes = gOjLoadingByteTotal = 0;
	gOjLoadingLastDraw = 0;
	gOjLoadingActive = true;
	ojDrawLoading(true);
}

void dc32OjLoadingStage(const char *stage)
{
	if (!gOjLoadingHost)
		return;
	gOjLoadingActive = true;
	ojCopyLoadingText(gOjLoadingStage, sizeof(gOjLoadingStage), stage);
	gOjLoadingAsset[0] = 0;
	gOjLoadingEntry = gOjLoadingEntryTotal = 0;
	gOjLoadingBytes = gOjLoadingByteTotal = 0;
	ojDrawLoading(true);
}

void dc32OjLoadingAsset(const char *name)
{
	ojCopyLoadingText(gOjLoadingAsset, sizeof(gOjLoadingAsset), name);
	ojDrawLoading(false);
}

void dc32OjLoadingCacheProgress(const char *name, uint32_t entry,
	uint32_t entryTotal, uint32_t bytes, uint32_t byteTotal)
{
	ojCopyLoadingText(gOjLoadingAsset, sizeof(gOjLoadingAsset), name);
	gOjLoadingEntry = entry;
	gOjLoadingEntryTotal = entryTotal;
	gOjLoadingBytes = bytes;
	gOjLoadingByteTotal = byteTotal;
	ojDrawLoading(false);
}

void dc32OjLoadingFlashWrite(const char *name, uint32_t address, uint32_t size)
{
	char detail[64];

	snprintf(detail, sizeof(detail), "WRITE %s %x+%u",
		name && *name ? name : "cache",
		(unsigned int)address, (unsigned int)size);
	ojCopyLoadingText(gOjLoadingAsset, sizeof(gOjLoadingAsset), detail);
	ojDrawLoading(true);
}

void dc32OjLoadingFailure(const char *stage, const char *detail)
{
	if (!gOjLoadingHost)
		return;
	gOjLoadingActive = true;
	ojCopyLoadingText(gOjLoadingStage, sizeof(gOjLoadingStage),
		stage ? stage : "OpenJazz failed");
	ojCopyLoadingText(gOjLoadingAsset, sizeof(gOjLoadingAsset),
		detail ? detail : "Unknown error");
	gOjLoadingEntry = gOjLoadingEntryTotal = 0;
	gOjLoadingBytes = gOjLoadingByteTotal = 0;
	ojDrawLoading(true);
}

void dc32OjLoadingPause(void)
{
	gOjLoadingActive = false;
}

void dc32OjLoadingFinish(void)
{
	gOjLoadingActive = false;
	gOjLoadingHost = NULL;
	gOjLoadingArgs = NULL;
	gOjLoadingStage[0] = 0;
	gOjLoadingAsset[0] = 0;
}

const char *dc32OjLoadingContext(void)
{
	if (gOjLoadingAsset[0])
		return gOjLoadingAsset;
	if (gOjLoadingStage[0])
		return gOjLoadingStage;
	return "Starting engine";
}

void dc32OjShowMessage(const struct DcAppHostApi *host, const struct DcAppRunArgs *args,
	const char *title, const char *line1, const char *line2, bool waitForCenter)
{
	struct Canvas cnv = ojCanvas(host, args);
	uint16_t bg = dcAppDrawRgb565(7, 10, 18);
	uint16_t fg = dcAppDrawRgb565(246, 247, 241);
	uint16_t accent = dcAppDrawRgb565(248, 190, 66);
	uint16_t dim = dcAppDrawRgb565(157, 178, 196);

	ojBeginDraw(host);
	ojFill(&cnv, 0, 0, 320, 240, bg);
	ojCentered(&cnv, 58, title ? title : "OpenJazz", FontLarge, fg);
	ojCentered(&cnv, 106, line1 ? line1 : "", FontMedium, accent);
	ojCentered(&cnv, 132, line2 ? line2 : "", FontSmall, dim);
	if (waitForCenter)
		ojCentered(&cnv, 184, "FN exits", FontMedium, dim);
	if (!waitForCenter)
		return;
	if (host && host->uiKeysRaw && host->delayMsec) {
		while (host->uiKeysRaw() & UI_KEY_BIT_CENTER)
			host->delayMsec(10);
		while (!(host->uiKeysRaw() & UI_KEY_BIT_CENTER))
			host->delayMsec(10);
		while (host->uiKeysRaw() & UI_KEY_BIT_CENTER)
			host->delayMsec(10);
	}
}

static bool ojProgress(const struct DcAppHostApi *host, const struct DcAppRunArgs *args,
	const char *name, uint32_t done, uint32_t total)
{
	struct Canvas cnv = ojCanvas(host, args);
	const struct DcAppLoadingState loading = {
		.appName = "OpenJazz",
		.title = "Installing game data",
		.detail = name ? name : "Reading JAZZ.ZIP",
		.hint = "FN cancels",
		.done = done,
		.total = total,
		.animationStep = done,
	};

	ojBeginDraw(host);
	dcAppDrawLoadingCanvas(&cnv, &loading);
	if (host && host->delayMsec)
		host->delayMsec(8);
	return !(host && host->uiKeysRaw && (host->uiKeysRaw() & UI_KEY_BIT_CENTER));
}

static char ojUpper(char ch)
{
	if (ch >= 'a' && ch <= 'z')
		return (char)(ch - ('a' - 'A'));
	return ch;
}

static int ojCompare(const char *a, const char *b)
{
	while (*a && *b) {
		char ca = ojUpper(*a++);
		char cb = ojUpper(*b++);

		if (ca != cb)
			return (unsigned char)ca < (unsigned char)cb ? -1 : 1;
	}
	return *a ? 1 : (*b ? -1 : 0);
}

static const char *ojBaseName(const char *path)
{
	const char *base = path;

	while (path && *path) {
		if (*path == '/' || *path == '\\')
			base = path + 1;
		path++;
	}
	return base;
}

static bool ojIncludeName(const char *name)
{
	const char *dot = strrchr(name, '.');

	if (!dot)
		return false;
	if (dot[1] >= '0' && dot[1] <= '9' &&
			dot[2] >= '0' && dot[2] <= '9' &&
			dot[3] >= '0' && dot[3] <= '9' && !dot[4])
		return true;
	if (!ojCompare(dot, ".0FN") || !ojCompare(dot, ".0SC") ||
			!ojCompare(dot, ".0FM") || !ojCompare(dot, ".PSM"))
		return true;
	return !ojCompare(name, "MACRO.1") || !ojCompare(name, "MACRO.2") ||
		!ojCompare(name, "MACRO.3") || !ojCompare(name, "MACRO.4");
}

static bool ojHasRequired(const struct OjInstallEntry *entries, uint32_t count)
{
	for (uint32_t r = 0; r < sizeof(gOjRequired) / sizeof(gOjRequired[0]); r++) {
		bool found = false;

		for (uint32_t i = 0; i < count; i++)
			if (!ojCompare(entries[i].name, gOjRequired[r])) {
				found = true;
				break;
			}
		if (!found)
			return false;
	}
	return true;
}

static void ojSortEntries(struct OjInstallEntry *entries, uint32_t count)
{
	for (uint32_t i = 1; i < count; i++) {
		struct OjInstallEntry entry = entries[i];
		uint32_t pos = i;

		while (pos && ojCompare(entries[pos - 1u].name, entry.name) > 0) {
			entries[pos] = entries[pos - 1u];
			pos--;
		}
		entries[pos] = entry;
	}
}

static void *ojZipAlloc(void *opaque, size_t items, size_t size)
{
	(void)opaque;
	if (size && items > SIZE_MAX / size)
		return NULL;
	return dc32PortMalloc(items * size);
}

static void ojZipFree(void *opaque, void *address)
{
	(void)opaque;
	dc32PortFree(address);
}

static void *ojZipRealloc(void *opaque, void *address, size_t items, size_t size)
{
	(void)opaque;
	if (size && items > SIZE_MAX / size)
		return NULL;
	return dc32PortRealloc(address, items * size);
}

static size_t ojZipRead(void *opaque, mz_uint64 offset, void *dst, size_t size)
{
	struct FatfsFil *file = (struct FatfsFil*)opaque;
	uint32_t got = 0;

	if (offset > UINT32_MAX || size > UINT32_MAX ||
			!fatfsFileSeek(file, (uint32_t)offset) ||
			!fatfsFileRead(file, dst, (uint32_t)size, &got))
		return 0;
	return got;
}

static size_t ojPackWrite(void *opaque, mz_uint64 offset, const void *src, size_t size)
{
	struct OjWriteContext *ctx = (struct OjWriteContext*)opaque;
	uint32_t wrote = 0;
	uint64_t target = (uint64_t)ctx->base + offset;

	if (target > UINT32_MAX || size > UINT32_MAX ||
			!fatfsFileSeek(ctx->file, (uint32_t)target) ||
			!fatfsFileWrite(ctx->file, src, (uint32_t)size, &wrote))
		return 0;
	return wrote;
}

static void ojWr16(uint8_t *dst, uint16_t value)
{
	dst[0] = (uint8_t)value;
	dst[1] = (uint8_t)(value >> 8);
}

static void ojWr32(uint8_t *dst, uint32_t value)
{
	dst[0] = (uint8_t)value;
	dst[1] = (uint8_t)(value >> 8);
	dst[2] = (uint8_t)(value >> 16);
	dst[3] = (uint8_t)(value >> 24);
}

static bool ojWriteExact(struct FatfsFil *file, const void *src, uint32_t size)
{
	uint32_t wrote = 0;

	return fatfsFileWrite(file, src, size, &wrote) && wrote == size;
}

static bool ojWritePadding(struct FatfsFil *file, uint32_t count)
{
	static const uint8_t pad[4] = {0xff, 0xff, 0xff, 0xff};

	return !count || ojWriteExact(file, pad, count);
}

static bool ojBuildPack(const struct DcAppHostApi *host, const struct DcAppRunArgs *args, bool *cancelled)
{
	struct FatfsVol *vol = args ? args->vol : NULL;
	struct FatfsFil *zipFile = NULL;
	struct FatfsFil *packFile = NULL;
	struct OjInstallEntry *entries = NULL;
	mz_zip_archive zip = {};
	uint32_t count = 0;
	uint32_t finalSize = 0;
	uint32_t tablePos = OJ_HEADER_SIZE;
	bool zipReady = false;
	bool ok = false;

	if (cancelled)
		*cancelled = false;
	if (!vol)
		goto cleanup;
	zipFile = fatfsFileOpen(vol, DC32_OPENJAZZ_ZIP_PATH, OPEN_MODE_READ);
	if (!zipFile)
		goto cleanup;
	zip.m_pAlloc = ojZipAlloc;
	zip.m_pFree = ojZipFree;
	zip.m_pRealloc = ojZipRealloc;
	zip.m_pRead = ojZipRead;
	zip.m_pIO_opaque = zipFile;
	if (!mz_zip_reader_init(&zip, fatfsFileGetSize(zipFile), 0))
		goto cleanup;
	zipReady = true;
	if (!zip.m_total_files || zip.m_total_files > OJ_MAX_ZIP_ENTRIES)
		goto cleanup;
	entries = (struct OjInstallEntry*)dc32PortCalloc(zip.m_total_files, sizeof(*entries));
	if (!entries)
		goto cleanup;
	for (uint32_t i = 0; i < zip.m_total_files; i++) {
		mz_zip_archive_file_stat stat;
		const char *base;
		uint32_t len;

		if (!mz_zip_reader_file_stat(&zip, i, &stat))
			goto cleanup;
		if (stat.m_is_directory)
			continue;
		base = ojBaseName(stat.m_filename);
		len = (uint32_t)strlen(base);
		if (!len || len > OJ_MAX_NAME || !ojIncludeName(base))
			continue;
		if (!stat.m_is_supported || stat.m_uncomp_size > UINT32_MAX)
			goto cleanup;
		for (uint32_t n = 0; n < len; n++) {
			char ch = ojUpper(base[n]);

			if (ch < 0x20 || ch > 0x7e || ch == '/' || ch == '\\')
				goto cleanup;
			entries[count].name[n] = ch;
		}
		entries[count].name[len] = 0;
		entries[count].zipIndex = i;
		entries[count].size = (uint32_t)stat.m_uncomp_size;
		entries[count].crc = stat.m_crc32;
		count++;
	}
	if (!count || !ojHasRequired(entries, count))
		goto cleanup;
	ojSortEntries(entries, count);
	for (uint32_t i = 1; i < count; i++)
		if (!ojCompare(entries[i - 1u].name, entries[i].name))
			goto cleanup;
	{
		uint64_t tableEnd = OJ_HEADER_SIZE;
		uint64_t offset;

		for (uint32_t i = 0; i < count; i++)
			tableEnd += OJ_ENTRY_SIZE + strlen(entries[i].name);
		offset = (tableEnd + 3u) & ~3ull;
		for (uint32_t i = 0; i < count; i++) {
			if (offset > UINT32_MAX)
				goto cleanup;
			entries[i].offset = (uint32_t)offset;
			offset = (offset + entries[i].size + 3u) & ~3ull;
		}
		if (offset > UINT32_MAX)
			goto cleanup;
		finalSize = (uint32_t)offset;
	}
	(void)fatfsFileDelete(vol, DC32_OPENJAZZ_PACK_PATH);
	packFile = fatfsFileOpen(vol, DC32_OPENJAZZ_PACK_PATH,
		OPEN_MODE_CREATE | OPEN_MODE_WRITE | OPEN_MODE_TRUNCATE);
	if (!packFile)
		goto cleanup;
	{
		uint8_t header[OJ_HEADER_SIZE] = {};

		memcpy(header, DC32_OPENJAZZ_PACK_MAGIC, 12);
		ojWr32(header + 12, OJ_VERSION);
		ojWr32(header + 16, count);
		if (!ojWriteExact(packFile, header, sizeof(header)))
			goto cleanup;
	}
	for (uint32_t i = 0; i < count; i++) {
		uint8_t raw[OJ_ENTRY_SIZE] = {};
		uint32_t nameLen = (uint32_t)strlen(entries[i].name);

		ojWr16(raw, (uint16_t)nameLen);
		ojWr32(raw + 4, entries[i].offset);
		ojWr32(raw + 8, entries[i].size);
		ojWr32(raw + 12, entries[i].crc);
		if (!ojWriteExact(packFile, raw, sizeof(raw)) ||
				!ojWriteExact(packFile, entries[i].name, nameLen))
			goto cleanup;
		tablePos += OJ_ENTRY_SIZE + nameLen;
	}
	if (!ojWritePadding(packFile, entries[0].offset - tablePos))
		goto cleanup;
	for (uint32_t i = 0; i < count; i++) {
		struct OjWriteContext writeCtx = {packFile, entries[i].offset};

		if (!ojProgress(host, args, entries[i].name, i, count)) {
			if (cancelled)
				*cancelled = true;
			goto cleanup;
		}
		if (!mz_zip_reader_extract_to_callback(&zip, entries[i].zipIndex, ojPackWrite, &writeCtx, 0))
			goto cleanup;
		if (!fatfsFileSeek(packFile, entries[i].offset + entries[i].size) ||
				!ojWritePadding(packFile, (4u - (entries[i].size & 3u)) & 3u))
			goto cleanup;
	}
	if (!fatfsFileTruncate(packFile, finalSize))
		goto cleanup;
	ok = ojProgress(host, args, "Ready", count, count);
	if (!ok && cancelled)
		*cancelled = true;

cleanup:
	if (packFile)
		(void)fatfsFileClose(packFile);
	if (zipReady)
		(void)mz_zip_reader_end(&zip);
	if (zipFile)
		(void)fatfsFileClose(zipFile);
	dc32PortFree(entries);
	if (!ok && vol)
		(void)fatfsFileDelete(vol, DC32_OPENJAZZ_PACK_PATH);
	return ok;
}

bool dc32OjPrepareData(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct FatfsVol *vol = args ? args->vol : NULL;
	struct FatfsFil *zipFile;
	bool cancelled = false;

	if (!vol)
		return false;
	if (dc32OjPackOpen(vol, DC32_OPENJAZZ_PACK_PATH))
		return true;
	zipFile = fatfsFileOpen(vol, DC32_OPENJAZZ_ZIP_PATH, OPEN_MODE_READ);
	if (!zipFile) {
		dc32OjShowMessage(host, args, "Jazz data missing",
			"Install /APPS/JAZZ.ZIP", "or a valid openjazz.pak", true);
		return false;
	}
	(void)fatfsFileClose(zipFile);
	if (!ojBuildPack(host, args, &cancelled)) {
		if (cancelled)
			return false;
		dc32OjShowMessage(host, args, "Install failed",
			"JAZZ.ZIP is invalid", "Partial pack removed", true);
		return false;
	}
	if (!dc32OjPackOpen(vol, DC32_OPENJAZZ_PACK_PATH)) {
		(void)fatfsFileDelete(vol, DC32_OPENJAZZ_PACK_PATH);
		dc32OjShowMessage(host, args, "Install failed",
			"Pack verification failed", "Partial pack removed", true);
		return false;
	}
	return true;
}
