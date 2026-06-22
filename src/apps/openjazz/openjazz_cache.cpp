#include "apps/openjazz/openjazz_cache.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "SDL.h"
#include "apps/openjazz/openjazz_install.h"
#include "apps/openjazz/openjazz_memory.h"
#include "apps/openjazz/openjazz_pack.h"
#include "apps/port/port_runtime.h"

extern "C" {
#include "dcApp.h"
#include "memMap.h"
#include "qspi.h"
}

static constexpr uint32_t OJ_CACHE_VERSION = 5u;
static constexpr uint32_t OJ_CACHE_SLOT_SIZE = 0x10000u;
static constexpr uint32_t OJ_CACHE_SLOT_COUNT = 2u;
static constexpr uint32_t OJ_CACHE_DATA_OFFSET =
	OJ_CACHE_SLOT_SIZE * OJ_CACHE_SLOT_COUNT;
static constexpr uint32_t OJ_CACHE_FINAL_SIZE = 0x200000u;
static constexpr uint32_t OJ_CACHE_HEADER_SIZE = 256u;
static constexpr uint32_t OJ_CACHE_WRITE_SIZE = QSPI_WRITE_GRANULARITY;
static constexpr uint32_t OJ_CACHE_FLASH_BUFFER_SIZE = 0x8000u;
static constexpr uint32_t OJ_CACHE_DECODE_SIZE =
	DC32_OJ_DECODE_PIXELS_CAPACITY + DC32_OJ_DECODE_MASK_CAPACITY;
static constexpr uint16_t OJ_CACHE_TYPE_SURFACE = 1u;
static constexpr uint16_t OJ_CACHE_TYPE_BLOB = 2u;
static constexpr uint32_t OJ_CACHE_FONT_MIN_PIXELS = 32u;
static constexpr uint32_t OJ_CACHE_BLANK_FONT_CRC = 0xab54d286u;
static constexpr uint32_t OJ_CACHE_REQUIRED_ENTRIES = 1010u;
static constexpr uint32_t OJ_CACHE_MANIFEST_CRC = 0xca091a34u;
static constexpr uint32_t OJ_CACHE_TABLE_BYTES =
	OJ_CACHE_REQUIRED_ENTRIES * 40u;
static constexpr uint32_t OJ_SHAREWARE_FINGERPRINT = 0xe9e3f7bdu;
static constexpr uint32_t OJ_SHAREWARE_PACK_SIZE = 2906760u;
static constexpr uint32_t OJ_SHAREWARE_ENTRY_COUNT = 57u;
static constexpr char OJ_CACHE_MAGIC[12] = {
	'D', 'C', '3', '2', 'J', 'A', 'Z', 'Z', 'X', 'I', 'P', '\0'
};

struct OjCacheHeader {
	char magic[12];
	uint32_t version;
	uint32_t generation;
	uint32_t sourceFingerprint;
	uint32_t sourcePackSize;
	uint32_t entryCount;
	uint32_t totalSize;
	uint32_t dataOffset;
	uint32_t tableCrc;
	uint32_t payloadCrc;
	uint32_t reserved[52];
};

struct OjCacheEntry {
	char name[16];
	uint32_t offset;
	uint32_t size;
	uint32_t crc;
	uint16_t width;
	uint16_t height;
	uint16_t pitch;
	uint8_t colorKey;
	uint8_t flags;
	uint16_t type;
	uint16_t reserved;
};

struct OjCacheWriter {
	OjCacheEntry entry;
	uint32_t written;
	uint32_t crc;
	bool active;
	bool failed;
};

static_assert(sizeof(OjCacheHeader) == OJ_CACHE_HEADER_SIZE,
	"OpenJazz cache header");
static_assert(sizeof(OjCacheEntry) == 40u, "OpenJazz cache entry");
static_assert(((OJ_CACHE_TABLE_BYTES + 255u) & ~255u) +
	OJ_CACHE_FLASH_BUFFER_SIZE + OJ_CACHE_DECODE_SIZE <=
	DC32_PORT_OPENJAZZ_LEVEL_SIZE,
	"OpenJazz cache workspace exceeds fixed level arena");
static_assert(160u * 160u <= OJ_CACHE_FLASH_BUFFER_SIZE,
	"OpenJazz font atlas exceeds flash staging buffer");
static_assert(OJ_CACHE_FINAL_SIZE <= QSPI_ROM_SIZE_MAX,
	"OpenJazz cache exceeds shared QSPI staging window");

static const struct DcAppHostApi *gOjCacheHost;
static const struct DcAppRunArgs *gOjCacheArgs;
static OjCacheHeader gOjCacheHeader;
static OjCacheEntry *gOjCacheBuildEntries;
static uint8_t *gOjCacheFlashBuffer;
static uint8_t *gOjCacheDecodeScratch;
static uint32_t gOjCacheBuildCount;
static uint32_t gOjCacheTotalSize;
static uint32_t gOjCacheBufferOffset;
static uint32_t gOjCacheBufferUsed;
static int32_t gOjCacheActiveSlot = -1;
static OjCacheWriter gOjCacheWriter;
static bool gOjCacheSealed;
static bool gOjCacheWorkspaceHeld;
static bool gOjCacheBuildFailed;
static char gOjCacheError[80];
static char gOjCacheLastAsset[16];
static uint32_t ojAlignUp(uint32_t value, uint32_t align)
{
	return (value + align - 1u) & ~(align - 1u);
}

static uint32_t ojCrc32Update(uint32_t crc, const void *data, uint32_t size)
{
	const uint8_t *src = (const uint8_t*)data;

	while (size--) {
		crc ^= *src++;
		for (uint32_t bit = 0; bit < 8u; bit++)
			crc = (crc >> 1) ^
				(0xedb88320u & (uint32_t)-(int32_t)(crc & 1u));
	}
	return crc;
}

static uint32_t ojCrc32(const void *data, uint32_t size)
{
	return ojCrc32Update(0xffffffffu, data, size) ^ 0xffffffffu;
}

static const OjCacheHeader *ojHeaderAt(uint32_t slot)
{
	return (const OjCacheHeader*)flashUncachedPtr(
		QSPI_ROM_START + slot * OJ_CACHE_SLOT_SIZE);
}

static const OjCacheEntry *ojTableAt(uint32_t slot)
{
	return (const OjCacheEntry*)flashUncachedPtr(
		QSPI_ROM_START + slot * OJ_CACHE_SLOT_SIZE + OJ_CACHE_HEADER_SIZE);
}

static const void *ojPayloadAt(uint32_t offset)
{
	return flashUncachedPtr(QSPI_ROM_START + offset);
}

static void ojSetError(const char *phase, uint32_t offset)
{
	const char *asset = gOjCacheWriter.active &&
		gOjCacheWriter.entry.name[0] ? gOjCacheWriter.entry.name :
		(gOjCacheLastAsset[0] ? gOjCacheLastAsset : "cache");

	gOjCacheBuildFailed = true;
	snprintf(gOjCacheError, sizeof(gOjCacheError), "%s %s @%08x",
		asset,
		phase ? phase : "Cache failure",
		(unsigned int)(QSPI_ROM_START + offset));
	dc32OjLoadingFailure("Cache flash failed", gOjCacheError);
}

void dc32OjCacheBuildFailure(const char *asset, const char *reason)
{
	gOjCacheBuildFailed = true;
	snprintf(gOjCacheError, sizeof(gOjCacheError), "%s: %s",
		asset && *asset ? asset : "cache",
		reason && *reason ? reason : "build failed");
	dc32OjLoadingFailure("Cache build failed", gOjCacheError);
}

static bool ojNameValid(const char name[16])
{
	for (uint32_t i = 0; i < 16u; i++) {
		unsigned char ch = (unsigned char)name[i];

		if (!ch)
			return i != 0u;
		if (ch < 0x20u || ch > 0x7eu)
			return false;
	}
	return false;
}

static bool ojExpectedEntry(uint32_t index, char name[16], uint16_t *type)
{
	static const char fixed[][16] = {
		"PANELRAW", "PANELBIG", "PANELSMALL",
		"FONT2.0FN", "FONTBIG.0FN", "FONTINY.0FN", "FONTMN1.0FN",
		"FONTMN2.0FN", "LEVELFONT", "BONUSFONT",
	};
	uint32_t item;

	memset(name, 0, 16u);
	*type = OJ_CACHE_TYPE_SURFACE;
	if (index < sizeof(fixed) / sizeof(fixed[0])) {
		memcpy(name, fixed[index], sizeof(fixed[0]));
		return true;
	}
	if (index < 19u) {
		if (index == 10u) {
			memcpy(name, "PANELHUD", 8u);
		} else if (index < 17u) {
			memcpy(name, "PANELAMMO0", 10u);
			name[9] = (char)('0' + index - 11u);
		} else {
			memcpy(name, "PANELBG0", 8u);
			name[7] = (char)('0' + index - 17u);
		}
		return true;
	}
	if (index < 28u) {
		uint32_t world = (index - 19u) / 3u;

		item = (index - 19u) % 3u;
		if (item < 2u) {
			memcpy(name, "PAL000A", 7u);
			name[5] = (char)('0' + world);
			name[6] = (char)('A' + item);
			*type = OJ_CACHE_TYPE_BLOB;
		} else {
			memcpy(name, "BLOCKS.000", 10u);
			name[9] = (char)('0' + world);
		}
		return true;
	}
	if (index < 956u) {
		static const char worlds[][3] = {
			{'0', '0', '0'}, {'0', '0', '1'},
			{'0', '0', '2'}, {'0', '1', '8'},
		};
		uint32_t relative = index - 28u;
		uint32_t sprite = relative % 232u;

		memcpy(name, "S000.000", 8u);
		memcpy(name + 1, worlds[relative / 232u], 3u);
		name[5] = (char)('0' + sprite / 100u);
		name[6] = (char)('0' + (sprite / 10u) % 10u);
		name[7] = (char)('0' + sprite % 10u);
		return true;
	}
	if (index < 1007u) {
		uint32_t sprite = index - 956u;

		memcpy(name, "BONUSSPR.000", 12u);
		name[9] = (char)('0' + sprite / 100u);
		name[10] = (char)('0' + (sprite / 10u) % 10u);
		name[11] = (char)('0' + sprite % 10u);
		return true;
	}
	if (index < OJ_CACHE_REQUIRED_ENTRIES) {
		static const char tail[][16] = {
			"BONUSBG", "BONUSPAL", "BONUSTILES",
		};

		item = index - 1007u;
		memcpy(name, tail[item], sizeof(tail[0]));
		if (item == 1u)
			*type = OJ_CACHE_TYPE_BLOB;
		return true;
	}
	return false;
}

static bool ojHeaderBasic(const OjCacheHeader *header,
	uint32_t sourceFingerprint, uint32_t sourcePackSize)
{
	return header &&
		!memcmp(header->magic, OJ_CACHE_MAGIC, sizeof(OJ_CACHE_MAGIC)) &&
		header->version == OJ_CACHE_VERSION &&
		header->sourceFingerprint == sourceFingerprint &&
		header->sourcePackSize == sourcePackSize &&
		header->dataOffset == OJ_CACHE_DATA_OFFSET &&
		header->entryCount == OJ_CACHE_REQUIRED_ENTRIES &&
		header->totalSize == OJ_CACHE_FINAL_SIZE;
}

static const OjCacheEntry *ojFindInTable(const OjCacheEntry *entries,
	uint32_t count, const char *name)
{
	for (uint32_t i = 0; i < count; i++)
		if (!strncmp(entries[i].name, name, sizeof(entries[i].name)))
			return entries + i;
	return NULL;
}

static bool ojMenuFontName(const char *name)
{
	static const char *const names[] = {
		"FONT2.0FN", "FONTBIG.0FN", "FONTINY.0FN", "FONTMN1.0FN",
		"FONTMN2.0FN",
	};

	for (uint32_t i = 0; i < sizeof(names) / sizeof(names[0]); i++)
		if (!strcmp(name, names[i]))
			return true;
	return false;
}

static bool ojFontEntriesValid(const OjCacheEntry *entries, uint32_t count,
	uint32_t totalSize, bool report)
{
	static const char *const names[] = {
		"FONT2.0FN", "FONTBIG.0FN", "FONTINY.0FN", "FONTMN1.0FN",
		"FONTMN2.0FN",
	};

	if (!entries || count < 8u)
		return false;
	for (uint32_t font = 0; font < sizeof(names) / sizeof(names[0]); font++) {
		const OjCacheEntry *entry = entries + 3u + font;

		if (strncmp(entry->name, names[font], sizeof(entry->name)) ||
				entry->type != OJ_CACHE_TYPE_SURFACE ||
				entry->width != 128u || entry->height != 128u ||
				entry->pitch != 128u || entry->size != 128u * 128u ||
				entry->colorKey != 0u || !(entry->flags & 1u) ||
				entry->reserved < OJ_CACHE_FONT_MIN_PIXELS ||
				entry->crc == OJ_CACHE_BLANK_FONT_CRC ||
				entry->offset < OJ_CACHE_DATA_OFFSET ||
				entry->offset > totalSize ||
				entry->size > totalSize - entry->offset)
			goto invalid;
		continue;
invalid:
		if (report)
			dc32OjCacheBuildFailure(names[font], "invalid font atlas");
		return false;
	}
	return true;
}

static bool ojEntriesValid(const OjCacheEntry *entries, uint32_t count,
	uint32_t totalSize, bool report)
{
	uint32_t manifest = 0xffffffffu;
	uint32_t nextOffset = OJ_CACHE_DATA_OFFSET;

	if (!entries || count != OJ_CACHE_REQUIRED_ENTRIES)
		goto invalid_manifest;
	for (uint32_t i = 0; i < count; i++) {
		const OjCacheEntry *entry = entries + i;
		uint8_t type[2] = {
			(uint8_t)entry->type, (uint8_t)(entry->type >> 8)
		};

		if (!ojNameValid(entry->name))
			goto invalid_manifest;
		if (entry->offset != nextOffset ||
				(entry->offset & (OJ_CACHE_WRITE_SIZE - 1u)) ||
				entry->offset > totalSize ||
				entry->size > totalSize - entry->offset ||
				!entry->width || !entry->height ||
				entry->pitch < entry->width ||
				entry->size != (uint32_t)entry->pitch * entry->height ||
				(entry->type != OJ_CACHE_TYPE_SURFACE &&
					entry->type != OJ_CACHE_TYPE_BLOB)) {
			if (report)
				dc32OjCacheBuildFailure(entry->name,
					"invalid cache entry");
			return false;
		}
		nextOffset = entry->offset +
			ojAlignUp(entry->size, OJ_CACHE_WRITE_SIZE);
		manifest = ojCrc32Update(manifest, entry->name,
			sizeof(entry->name));
		manifest = ojCrc32Update(manifest, type, sizeof(type));
	}
	if ((manifest ^ 0xffffffffu) == OJ_CACHE_MANIFEST_CRC)
		return true;
invalid_manifest:
	if (report)
		dc32OjCacheBuildFailure("cache", "manifest mismatch");
	return false;
}

static bool ojHeaderValid(uint32_t slot, uint32_t sourceFingerprint,
	uint32_t sourcePackSize)
{
	OjCacheHeader header = *ojHeaderAt(slot);
	const OjCacheEntry *entries;

	if (!ojHeaderBasic(&header, sourceFingerprint, sourcePackSize))
		return false;
	entries = ojTableAt(slot);
	if (ojCrc32(entries, header.entryCount * sizeof(*entries)) !=
			header.tableCrc ||
			!ojEntriesValid(entries, header.entryCount,
				header.totalSize, false) ||
			!ojFontEntriesValid(entries, header.entryCount,
				header.totalSize, false))
		return false;
	return true;
}

static const OjCacheEntry *ojFindCommitted(const char *name)
{
	if (gOjCacheActiveSlot < 0)
		return NULL;
	return ojFindInTable(ojTableAt((uint32_t)gOjCacheActiveSlot),
		gOjCacheHeader.entryCount, name);
}

static const OjCacheEntry *ojFindEntry(const char *name)
{
	if (gOjCacheBuildEntries) {
		const OjCacheEntry *entry = ojFindInTable(gOjCacheBuildEntries,
			gOjCacheBuildCount, name);

		if (entry)
			return entry;
	}
	return ojFindCommitted(name);
}

static SDL_Surface *ojWrapEntry(const OjCacheEntry *entry)
{
	return entry ? dc32OjSdlWrapReadOnlySurface(ojPayloadAt(entry->offset),
		entry->width, entry->height, entry->pitch, entry->colorKey,
		(entry->flags & 1u) != 0u) : NULL;
}

static bool ojViewEntry(const OjCacheEntry *entry, Dc32OjSurfaceView *view)
{
	if (!entry || !view)
		return false;
	view->pixels = (const uint8_t*)ojPayloadAt(entry->offset);
	view->width = entry->width;
	view->height = entry->height;
	view->pitch = entry->pitch;
	view->colorKey = entry->colorKey;
	view->colorKeyEnabled = (entry->flags & 1u) != 0u;
	return true;
}

static bool ojFlushBuildBuffer(void)
{
	if (!gOjCacheBufferUsed)
		return true;
	if (!gOjCacheHost || !gOjCacheHost->flashWrite ||
			(gOjCacheBufferUsed & (OJ_CACHE_WRITE_SIZE - 1u))) {
		ojSetError("Invalid flash batch", gOjCacheBufferOffset);
		return false;
	}
	dc32OjLoadingCacheProgress(
		gOjCacheWriter.active ? gOjCacheWriter.entry.name : "Writing cache",
		gOjCacheBuildCount + (gOjCacheWriter.active ? 1u : 0u),
		OJ_CACHE_REQUIRED_ENTRIES,
		gOjCacheBufferOffset + gOjCacheBufferUsed - OJ_CACHE_DATA_OFFSET,
		OJ_CACHE_FINAL_SIZE - OJ_CACHE_DATA_OFFSET);
	dc32OjLoadingFlashWrite(
		gOjCacheWriter.active ? gOjCacheWriter.entry.name : "cache",
		QSPI_ROM_START + gOjCacheBufferOffset, gOjCacheBufferUsed);
	if (!gOjCacheHost->flashWrite(
			QSPI_ROM_START + gOjCacheBufferOffset, 0,
			gOjCacheFlashBuffer, gOjCacheBufferUsed)) {
		ojSetError("Write timeout", gOjCacheBufferOffset);
		return false;
	}
	gOjCacheBufferOffset += gOjCacheBufferUsed;
	gOjCacheBufferUsed = 0;
	return true;
}

static bool ojAppendBuildData(const void *data, uint32_t size)
{
	const uint8_t *src = (const uint8_t*)data;

	if (!gOjCacheFlashBuffer || (!src && size))
		return false;
	while (size) {
		uint32_t now = OJ_CACHE_FLASH_BUFFER_SIZE - gOjCacheBufferUsed;

		if (now > size)
			now = size;
		memmove(gOjCacheFlashBuffer + gOjCacheBufferUsed, src, now);
		gOjCacheBufferUsed += now;
		src += now;
		size -= now;
		if (gOjCacheBufferUsed == OJ_CACHE_FLASH_BUFFER_SIZE &&
				!ojFlushBuildBuffer())
			return false;
	}
	return true;
}

static bool ojAppendPadding(uint32_t size)
{
	uint8_t padding[OJ_CACHE_WRITE_SIZE];

	memset(padding, 0xff, sizeof(padding));
	while (size) {
		uint32_t now = size > sizeof(padding) ? sizeof(padding) : size;

		if (!ojAppendBuildData(padding, now))
			return false;
		size -= now;
	}
	return true;
}

bool dc32OjCachePrepare(const struct DcAppHostApi *host,
	const struct DcAppRunArgs *args)
{
	uint32_t fingerprint;
	uint32_t packSize;
	uint32_t entryCount;
	bool valid[OJ_CACHE_SLOT_COUNT] = {};
	int32_t selected = -1;

	gOjCacheHost = host;
	gOjCacheArgs = args;
	gOjCacheBuildEntries = NULL;
	gOjCacheFlashBuffer = NULL;
	gOjCacheDecodeScratch = NULL;
	gOjCacheBuildCount = 0;
	gOjCacheActiveSlot = -1;
	gOjCacheSealed = false;
	gOjCacheWorkspaceHeld = false;
	gOjCacheBuildFailed = false;
	gOjCacheError[0] = 0;
	gOjCacheLastAsset[0] = 0;
	memset(&gOjCacheHeader, 0, sizeof(gOjCacheHeader));
	memset(&gOjCacheWriter, 0, sizeof(gOjCacheWriter));
	if (!host || !host->flashWrite ||
			!dc32OjPackFingerprint(&fingerprint, &packSize, &entryCount))
		return false;
	if (fingerprint != OJ_SHAREWARE_FINGERPRINT ||
			packSize != OJ_SHAREWARE_PACK_SIZE ||
			entryCount != OJ_SHAREWARE_ENTRY_COUNT) {
		dc32OjShowMessage(host, args, "Jazz data unsupported",
			"XIP cache supports", "bundled shareware only", true);
		return false;
	}
	dc32OjLoadingStage("Checking XIP asset cache");
	for (uint32_t slot = 0; slot < OJ_CACHE_SLOT_COUNT; slot++)
		valid[slot] = ojHeaderValid(slot, fingerprint, packSize);
	for (uint32_t slot = 0; slot < OJ_CACHE_SLOT_COUNT; slot++)
		if (valid[slot] && (selected < 0 ||
				(int32_t)(ojHeaderAt(slot)->generation -
					ojHeaderAt((uint32_t)selected)->generation) > 0))
			selected = (int32_t)slot;
	if (selected >= 0) {
		gOjCacheActiveSlot = selected;
		gOjCacheHeader = *ojHeaderAt((uint32_t)selected);
		gOjCacheTotalSize = gOjCacheHeader.totalSize;
		gOjCacheSealed = true;
		return true;
	}
	if (!dc32OjCacheWorkspaceAcquire((void**)&gOjCacheBuildEntries,
			OJ_CACHE_TABLE_BYTES, (void**)&gOjCacheFlashBuffer,
			OJ_CACHE_FLASH_BUFFER_SIZE, (void**)&gOjCacheDecodeScratch,
			OJ_CACHE_DECODE_SIZE)) {
		ojSetError("Cache workspace", 0);
		return false;
	}
	gOjCacheWorkspaceHeld = true;
	memset(gOjCacheBuildEntries, 0xff,
		ojAlignUp(OJ_CACHE_TABLE_BYTES, OJ_CACHE_WRITE_SIZE));
	memcpy(gOjCacheHeader.magic, OJ_CACHE_MAGIC, sizeof(OJ_CACHE_MAGIC));
	gOjCacheHeader.version = OJ_CACHE_VERSION;
	gOjCacheHeader.sourceFingerprint = fingerprint;
	gOjCacheHeader.sourcePackSize = packSize;
	gOjCacheHeader.dataOffset = OJ_CACHE_DATA_OFFSET;
	gOjCacheTotalSize = OJ_CACHE_DATA_OFFSET;
	gOjCacheBufferOffset = OJ_CACHE_DATA_OFFSET;
	gOjCacheBufferUsed = 0;
	dc32OjLoadingStage("Erasing XIP asset cache");
	if (!host->flashWrite(QSPI_ROM_START, OJ_CACHE_FINAL_SIZE, NULL, 0)) {
		ojSetError("Erase timeout", 0);
		dc32OjShowMessage(host, args, "OpenJazz cache failed",
			"Flash erase timeout", gOjCacheError, true);
		return false;
	}
	return true;
}

void dc32OjCacheClose(void)
{
	memset(&gOjCacheWriter, 0, sizeof(gOjCacheWriter));
	if (gOjCacheWorkspaceHeld)
		dc32OjCacheWorkspaceRelease();
	gOjCacheWorkspaceHeld = false;
	gOjCacheBuildEntries = NULL;
	gOjCacheFlashBuffer = NULL;
	gOjCacheDecodeScratch = NULL;
	gOjCacheBuildCount = 0;
	gOjCacheHost = NULL;
	gOjCacheArgs = NULL;
	gOjCacheActiveSlot = -1;
	gOjCacheSealed = false;
	gOjCacheBuildFailed = false;
	gOjCacheLastAsset[0] = 0;
}

SDL_Surface *dc32OjCacheFindSurface(const char *name)
{
	return name ? ojWrapEntry(ojFindEntry(name)) : NULL;
}

bool dc32OjCacheFindView(const char *name, Dc32OjSurfaceView *view)
{
	return name && ojViewEntry(ojFindEntry(name), view);
}

SDL_Surface *dc32OjCacheCreateStagingSurface(uint16_t width, uint16_t height)
{
	uint32_t size = (uint32_t)width * height;

	if (!gOjCacheFlashBuffer || gOjCacheWriter.active || !width || !height ||
			size > OJ_CACHE_FLASH_BUFFER_SIZE ||
			!ojFlushBuildBuffer())
		return NULL;
	memset(gOjCacheFlashBuffer, 0, size);
	return dc32OjSdlWrapWritableSurface(
		gOjCacheFlashBuffer, width, height, width);
}

uint8_t *dc32OjCacheDecodePixels(void)
{
	return gOjCacheDecodeScratch;
}

uint8_t *dc32OjCacheDecodeMask(void)
{
	return gOjCacheDecodeScratch ?
		gOjCacheDecodeScratch + DC32_OJ_DECODE_PIXELS_CAPACITY : NULL;
}

static OjCacheEntry *ojEndEntry(bool flush);

static bool ojRecoverCompleteFinalEntry(void)
{
	if (!gOjCacheWriter.active)
		return true;
	if (gOjCacheBuildCount != OJ_CACHE_REQUIRED_ENTRIES - 1u ||
			gOjCacheWriter.failed ||
			gOjCacheWriter.written != gOjCacheWriter.entry.size ||
			gOjCacheWriter.entry.type != OJ_CACHE_TYPE_SURFACE ||
			strncmp(gOjCacheWriter.entry.name, "BONUSTILES",
				sizeof(gOjCacheWriter.entry.name)))
		return false;
	return ojEndEntry(false) != NULL;
}

static bool ojCommittedTableCount(uint32_t *count)
{
	uint32_t used = 0;

	if (!gOjCacheBuildEntries || !count)
		return false;
	while (used < OJ_CACHE_REQUIRED_ENTRIES &&
			(uint8_t)gOjCacheBuildEntries[used].name[0] != 0xffu) {
		if (!ojNameValid(gOjCacheBuildEntries[used].name))
			return false;
		used++;
	}
	for (uint32_t i = used; i < OJ_CACHE_REQUIRED_ENTRIES; i++)
		if ((uint8_t)gOjCacheBuildEntries[i].name[0] != 0xffu)
			return false;
	*count = used;
	return true;
}

bool dc32OjCachePendingStatus(char name[16], uint32_t *entry,
	uint32_t *written, uint32_t *size)
{
	if (name)
		memcpy(name, gOjCacheWriter.entry.name, 16u);
	if (entry)
		*entry = gOjCacheBuildCount;
	if (written)
		*written = gOjCacheWriter.written;
	if (size)
		*size = gOjCacheWriter.entry.size;
	return gOjCacheWriter.active;
}

static bool ojReportPendingWriter(void)
{
	char name[16];
	char detail[48];
	uint32_t entry;
	uint32_t written;
	uint32_t size;

	if (!dc32OjCachePendingStatus(name, &entry, &written, &size))
		return true;
	snprintf(detail, sizeof(detail), "pending %u/%u at entry %u",
		(unsigned int)written, (unsigned int)size, (unsigned int)entry);
	dc32OjCacheBuildFailure(name[0] ? name : "cache", detail);
	return false;
}

bool dc32OjCacheCheckpoint(const char *stage, uint32_t expectedCount,
	const char *expectedLastEntry)
{
	if (stage)
		dc32OjLoadingStage(stage);
	if (gOjCacheBuildFailed)
		return false;
	if (expectedCount == OJ_CACHE_REQUIRED_ENTRIES &&
			gOjCacheWriter.active &&
			!ojRecoverCompleteFinalEntry()) {
		ojReportPendingWriter();
		return false;
	}
	if (gOjCacheWriter.active) {
		ojReportPendingWriter();
		return false;
	}
	if (!gOjCacheBuildEntries ||
			(gOjCacheBuildCount &&
				!ojNameValid(
					gOjCacheBuildEntries[gOjCacheBuildCount - 1u].name)) ||
			(gOjCacheBuildCount < OJ_CACHE_REQUIRED_ENTRIES &&
				(uint8_t)gOjCacheBuildEntries[
					gOjCacheBuildCount].name[0] != 0xffu)) {
		dc32OjCacheBuildFailure("cache", "counter/table mismatch");
		return false;
	}
	if (expectedCount > OJ_CACHE_REQUIRED_ENTRIES ||
			gOjCacheBuildCount != expectedCount) {
		dc32OjCacheBuildFailure(expectedLastEntry,
			"phase entry count mismatch");
		return false;
	}
	if (expectedCount && expectedLastEntry &&
			strncmp(gOjCacheBuildEntries[expectedCount - 1u].name,
				expectedLastEntry,
				sizeof(gOjCacheBuildEntries[0].name))) {
		dc32OjCacheBuildFailure(expectedLastEntry,
			"checkpoint final entry mismatch");
		return false;
	}
	return true;
}

static bool ojBeginEntry(const char *name, uint16_t type, uint16_t width,
	uint16_t height, uint16_t pitch, uint8_t colorKey, bool colorKeyEnabled)
{
	uint32_t size;
	uint32_t offset;
	char expectedName[16];
	uint16_t expectedType;

	if (gOjCacheSealed || !gOjCacheBuildEntries || !name || !*name ||
			gOjCacheBuildFailed ||
			strlen(name) >= sizeof(gOjCacheWriter.entry.name) ||
			!width || !height || pitch < width || gOjCacheWriter.active ||
			gOjCacheBuildCount >= OJ_CACHE_REQUIRED_ENTRIES)
		return false;
	if (!ojExpectedEntry(gOjCacheBuildCount, expectedName, &expectedType) ||
			strncmp(name, expectedName, sizeof(expectedName)) ||
			type != expectedType) {
		dc32OjCacheBuildFailure(expectedName,
			"unexpected cache transaction");
		return false;
	}
	if (ojFindEntry(name)) {
		dc32OjCacheBuildFailure(name, "duplicate cache transaction");
		return false;
	}
	size = (uint32_t)pitch * height;
	offset = ojAlignUp(gOjCacheTotalSize, OJ_CACHE_WRITE_SIZE);
	if (offset != gOjCacheBufferOffset + gOjCacheBufferUsed ||
			size > OJ_CACHE_FINAL_SIZE - offset)
		return false;
	memset(&gOjCacheWriter, 0, sizeof(gOjCacheWriter));
	strncpy(gOjCacheWriter.entry.name, name,
		sizeof(gOjCacheWriter.entry.name) - 1u);
	gOjCacheWriter.entry.offset = offset;
	gOjCacheWriter.entry.size = size;
	gOjCacheWriter.entry.width = width;
	gOjCacheWriter.entry.height = height;
	gOjCacheWriter.entry.pitch = pitch;
	gOjCacheWriter.entry.colorKey = colorKey;
	gOjCacheWriter.entry.flags = colorKeyEnabled ? 1u : 0u;
	gOjCacheWriter.entry.type = type;
	gOjCacheWriter.crc = 0xffffffffu;
	gOjCacheWriter.active = true;
	strncpy(gOjCacheLastAsset, name, sizeof(gOjCacheLastAsset) - 1u);
	gOjCacheLastAsset[sizeof(gOjCacheLastAsset) - 1u] = 0;
	dc32OjLoadingCacheProgress(name, gOjCacheBuildCount + 1u,
		OJ_CACHE_REQUIRED_ENTRIES,
		offset - OJ_CACHE_DATA_OFFSET,
		OJ_CACHE_FINAL_SIZE - OJ_CACHE_DATA_OFFSET);
	return true;
}

bool dc32OjCacheBeginSurface(const char *name, uint16_t width, uint16_t height,
	uint16_t pitch, uint8_t colorKey, bool colorKeyEnabled)
{
	return ojBeginEntry(name, OJ_CACHE_TYPE_SURFACE, width, height, pitch,
		colorKey, colorKeyEnabled);
}

bool dc32OjCacheWriteSurface(const void *data, uint32_t size)
{
	const uint8_t *pixels = (const uint8_t*)data;

	if (!gOjCacheWriter.active || gOjCacheWriter.failed || !data ||
			size > gOjCacheWriter.entry.size - gOjCacheWriter.written)
		return false;
	if (gOjCacheWriter.entry.type == OJ_CACHE_TYPE_SURFACE &&
			ojMenuFontName(gOjCacheWriter.entry.name))
		for (uint32_t i = 0; i < size &&
				gOjCacheWriter.entry.reserved < OJ_CACHE_FONT_MIN_PIXELS; i++)
			if (pixels[i] != gOjCacheWriter.entry.colorKey)
				gOjCacheWriter.entry.reserved++;
	gOjCacheWriter.crc = ojCrc32Update(gOjCacheWriter.crc, data, size);
	if (!ojAppendBuildData(data, size)) {
		gOjCacheWriter.failed = true;
		return false;
	}
	gOjCacheWriter.written += size;
	return true;
}

static OjCacheEntry *ojEndEntry(bool flush)
{
	OjCacheEntry *entry;
	uint32_t alignedSize;

	if (!gOjCacheWriter.active || gOjCacheWriter.failed ||
			gOjCacheWriter.written != gOjCacheWriter.entry.size) {
		dc32OjCacheCancelSurface();
		return NULL;
	}
	alignedSize = ojAlignUp(gOjCacheWriter.entry.size, OJ_CACHE_WRITE_SIZE);
	if (!ojAppendPadding(alignedSize - gOjCacheWriter.entry.size)) {
		dc32OjCacheCancelSurface();
		return NULL;
	}
	gOjCacheWriter.entry.crc = gOjCacheWriter.crc ^ 0xffffffffu;
	entry = gOjCacheBuildEntries + gOjCacheBuildCount++;
	*entry = gOjCacheWriter.entry;
	gOjCacheTotalSize = entry->offset + alignedSize;
	memset(&gOjCacheWriter, 0, sizeof(gOjCacheWriter));
	if (flush && !ojFlushBuildBuffer())
		return NULL;
	return entry;
}

SDL_Surface *dc32OjCacheEndSurface(void)
{
	return ojWrapEntry(ojEndEntry(true));
}

bool dc32OjCacheEndView(Dc32OjSurfaceView *view)
{
	return ojViewEntry(ojEndEntry(false), view);
}

void dc32OjCacheCancelSurface(void)
{
	if (!gOjCacheError[0])
		ojSetError("decode", gOjCacheTotalSize);
	gOjCacheWriter.failed = true;
}

bool dc32OjCachePromoteSurfaceChecked(const char *name, SDL_Surface **surface)
{
	SDL_Surface *cached;

	if (!surface || !*surface || !name) {
		dc32OjCacheBuildFailure(name, "missing promotion surface");
		return false;
	}
	cached = dc32OjCacheFindSurface(name);
	if (cached) {
		SDL_FreeSurface(*surface);
		*surface = cached;
		return true;
	}
	cached = dc32OjCacheStoreSurface(name, (*surface)->pixels,
		(uint16_t)(*surface)->w, (uint16_t)(*surface)->h,
		(*surface)->pitch, (uint8_t)(*surface)->format->colorkey,
		(*surface)->colorKeyEnabled != 0u);
	if (!cached) {
		if (!gOjCacheError[0])
			dc32OjCacheBuildFailure(name, "cache promotion finalize failed");
		SDL_FreeSurface(*surface);
		*surface = NULL;
		return false;
	}
	SDL_FreeSurface(*surface);
	*surface = cached;
	return true;
}

SDL_Surface *dc32OjCacheStoreSurface(const char *name, const void *pixels,
	uint16_t width, uint16_t height, uint16_t pitch, uint8_t colorKey,
	bool colorKeyEnabled)
{
	SDL_Surface *cached;

	if (!name || !pixels || !width || !height || pitch < width)
		return NULL;
	cached = dc32OjCacheFindSurface(name);
	if (cached)
		return cached;
	dc32OjLoadingAsset(name);
	if (!dc32OjCacheBeginSurface(name, width, height, pitch, colorKey,
			colorKeyEnabled) ||
			!dc32OjCacheWriteSurface(pixels, (uint32_t)pitch * height)) {
		dc32OjCacheCancelSurface();
		return NULL;
	}
	return dc32OjCacheEndSurface();
}

bool dc32OjCacheStoreView(const char *name, const void *pixels,
	uint16_t width, uint16_t height, uint16_t pitch, uint8_t colorKey,
	bool colorKeyEnabled, Dc32OjSurfaceView *view)
{
	if (!name || !pixels || !view || !width || !height || pitch < width)
		return false;
	if (dc32OjCacheFindView(name, view))
		return true;
	dc32OjLoadingAsset(name);
	if (!dc32OjCacheBeginSurface(name, width, height, pitch, colorKey,
			colorKeyEnabled) ||
			!dc32OjCacheWriteSurface(pixels, (uint32_t)pitch * height)) {
		dc32OjCacheCancelSurface();
		return false;
	}
	return dc32OjCacheEndView(view);
}

bool dc32OjCacheFindBlob(const char *name, const void **data, uint32_t *size)
{
	const OjCacheEntry *entry = name ? ojFindEntry(name) : NULL;

	if (!entry || entry->type != OJ_CACHE_TYPE_BLOB)
		return false;
	if (data)
		*data = ojPayloadAt(entry->offset);
	if (size)
		*size = entry->size;
	return true;
}

bool dc32OjCacheStoreBlob(const char *name, const void *data, uint32_t size)
{
	const void *existing;
	uint32_t existingSize;
	Dc32OjSurfaceView ignored;

	if (!name || !data || !size || size > UINT16_MAX)
		return false;
	if (dc32OjCacheFindBlob(name, &existing, &existingSize))
		return existingSize == size;
	dc32OjLoadingAsset(name);
	if (!ojBeginEntry(name, OJ_CACHE_TYPE_BLOB, (uint16_t)size, 1,
			(uint16_t)size, 0, false) ||
			!dc32OjCacheWriteSurface(data, size)) {
		dc32OjCacheCancelSurface();
		return false;
	}
	return dc32OjCacheEndView(&ignored);
}

bool dc32OjCacheCommit(void)
{
	return !gOjCacheWriter.active;
}

bool dc32OjCacheSeal(void)
{
	OjCacheHeader header;
	uint32_t tableCount;
	uint32_t tableWriteSize;

	if (gOjCacheSealed)
		return true;
	dc32OjLoadingStage("Checking cache manifest");
	if (gOjCacheBuildFailed)
		return false;
	if (!gOjCacheBuildEntries) {
		dc32OjCacheBuildFailure("cache", "entry table unavailable");
		return false;
	}
	if (gOjCacheWriter.active) {
		if (!ojRecoverCompleteFinalEntry()) {
			ojReportPendingWriter();
			return false;
		}
	}
	if (!ojCommittedTableCount(&tableCount) ||
			tableCount != gOjCacheBuildCount) {
		dc32OjCacheBuildFailure("cache", "counter/table mismatch");
		return false;
	}
	if (!dc32OjCacheCheckpoint("Checking cache manifest",
			OJ_CACHE_REQUIRED_ENTRIES, "BONUSTILES"))
		return false;
	if (!ojEntriesValid(gOjCacheBuildEntries, gOjCacheBuildCount,
			OJ_CACHE_FINAL_SIZE, true) ||
			!ojFontEntriesValid(gOjCacheBuildEntries, gOjCacheBuildCount,
				OJ_CACHE_FINAL_SIZE, true))
		return false;
	dc32OjLoadingStage("Flushing cache payload");
	if (!ojFlushBuildBuffer())
		return false;
	header = gOjCacheHeader;
	header.generation = 1u;
	header.entryCount = gOjCacheBuildCount;
	header.totalSize = OJ_CACHE_FINAL_SIZE;
	header.dataOffset = OJ_CACHE_DATA_OFFSET;
	header.tableCrc = ojCrc32(gOjCacheBuildEntries,
		gOjCacheBuildCount * sizeof(*gOjCacheBuildEntries));
	header.payloadCrc = 0u;
	tableWriteSize = ojAlignUp(
		gOjCacheBuildCount * sizeof(*gOjCacheBuildEntries),
		OJ_CACHE_WRITE_SIZE);
	dc32OjLoadingStage("Committing cache table");
	if (!gOjCacheHost->flashWrite(
			QSPI_ROM_START + OJ_CACHE_HEADER_SIZE, 0,
			gOjCacheBuildEntries, tableWriteSize)) {
		ojSetError("Table timeout", OJ_CACHE_HEADER_SIZE);
		return false;
	}
	if (!gOjCacheHost->flashWrite(QSPI_ROM_START, 0,
			&header, sizeof(header))) {
		ojSetError("Header timeout", 0);
		return false;
	}
	gOjCacheHeader = header;
	gOjCacheActiveSlot = 0;
	gOjCacheSealed = true;
	if (gOjCacheWorkspaceHeld)
		dc32OjCacheWorkspaceRelease();
	gOjCacheWorkspaceHeld = false;
	gOjCacheBuildEntries = NULL;
	gOjCacheFlashBuffer = NULL;
	gOjCacheDecodeScratch = NULL;
	gOjCacheBuildCount = 0;
	return true;
}

bool dc32OjCacheIsSealed(void)
{
	return gOjCacheSealed;
}

const char *dc32OjCacheLastError(void)
{
	return gOjCacheError;
}
