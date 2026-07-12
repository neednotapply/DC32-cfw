/*
 * DC32 UI, persistence, rendering and controls for the YSoccer 19-derived
 * native match engine. GPL-2.0-only.
 */
#include "apps/soccer/soccer_port.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "apps/port/port_runtime.h"
#include "apps/soccer/soccer_core.h"
#include "audioPwm.h"
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "memMap.h"
#include "qspi.h"
#include "toolWorkspace.h"
#include "ui.h"

#define SOCCER_SCREEN_W 320
#define SOCCER_SCREEN_H 240
#define SOCCER_RENDER_HZ 30u
#define SOCCER_PACK_PATH "/APPS/soccer-ysoccer.pak"
#define SOCCER_SAVE_ROOT "/SAVE/PORTS/SOCCER"
#define SOCCER_SAVE_PATH SOCCER_SAVE_ROOT "/STATE.SAV"
#define SOCCER_IMPORT_PATH SOCCER_SAVE_ROOT "/IMPORT/TEAM.000"
#define SOCCER_EXPORT_PATH SOCCER_SAVE_ROOT "/EXPORT/TEAM.000"
#define SOCCER_REPLAY_PATH SOCCER_SAVE_ROOT "/REPLAYS/LAST.RPL"
#define SOCCER_HIGHLIGHT_PATH SOCCER_SAVE_ROOT "/HILITES/LAST.HIL"
#define SOCCER_PACK_VERSION 5u
#define SOCCER_TEAM_COUNT 24u
#define SOCCER_LEGACY_TEAM_COUNT 68u
#define SOCCER_REPLAY_FRAMES 192u
#define SOCCER_SAVE_MAGIC 0x31534353u
#define SOCCER_SAVE_VERSION 4u
#define SOCCER_TEAM_MAGIC 0x31545359u
#define SOCCER_MENU_ITEMS 13u
#define SOCCER_TEAM_PLAYERS 16u
#define SOCCER_ASSET_PALETTE_MAX 96u
#define SOCCER_SPRITE_CACHE_REGULAR_SLOTS 32u
#define SOCCER_SPRITE_CACHE_LARGE_SLOTS 2u
#define SOCCER_SPRITE_CACHE_SLOTS (SOCCER_SPRITE_CACHE_REGULAR_SLOTS + SOCCER_SPRITE_CACHE_LARGE_SLOTS)
#define SOCCER_SPRITE_CACHE_PIXELS 1024u
#define SOCCER_SPRITE_CACHE_LARGE_PIXELS 2500u
#define SOCCER_HAIR_STYLES 42u
#define SOCCER_KEEPER_FRAMES 20u
#define SOCCER_PITCH_TEXTURE_SIZE 32u
#define SOCCER_PITCH_TEXTURE_SOURCE_X 900u
#define SOCCER_PITCH_TEXTURE_SOURCE_Y 56u
#define SOCCER_FRAMEBUFFER_BYTES (SOCCER_SCREEN_W * SOCCER_SCREEN_H)
#define SOCCER_XIP_CACHE_VERSION 1u
#define SOCCER_XIP_CACHE_DATA_OFFSET 0x1000u
#define SOCCER_XIP_CACHE_SAFETY 0x20000u
#define SOCCER_XIP_CACHE_MAX_ENTRIES 48u
#define SOCCER_XIP_CACHE_CHUNK 0x8000u

enum SoccerScreen {
	SoccerScreenTitle,
	SoccerScreenMain,
	SoccerScreenTeam,
	SoccerScreenCompetition,
	SoccerScreenMatch,
	SoccerScreenPause,
	SoccerScreenResult,
	SoccerScreenReplay,
	SoccerScreenOptions,
	SoccerScreenEditor,
	SoccerScreenPlayers,
	SoccerScreenKeyboard,
	SoccerScreenTactics,
	SoccerScreenTransfer,
	SoccerScreenCredits,
};

enum SoccerPurpose {
	SoccerPurposeFriendly,
	SoccerPurposeCup,
	SoccerPurposeLeague,
	SoccerPurposeTournament,
	SoccerPurposeTraining,
	SoccerPurposeCpu,
	SoccerPurposeEditor,
};

enum SoccerCacheResult {
	SoccerCacheReady,
	SoccerCacheFailed,
	SoccerCacheCancelled,
};

struct __attribute__((packed)) SoccerPackHeader {
	char magic[12];
	uint32_t version;
	uint32_t teamCount;
	uint8_t commit[20];
	uint32_t recordSize;
	uint32_t tableCrc;
	uint32_t playerOffset;
	uint32_t assetDirOffset;
	uint32_t assetCount;
	uint32_t assetDirCrc;
};

struct __attribute__((packed)) SoccerTeamRecord {
	char name[32];
	char style[16];
	uint16_t country;
	uint8_t division;
	uint8_t playerCount;
	uint16_t shirt1;
	uint16_t shirt2;
	uint16_t shorts;
	uint16_t socks;
	uint32_t seed;
	uint8_t reserved[4];
};

struct __attribute__((packed)) SoccerPlayerRecord {
	char name[20];
	uint8_t number;
	uint8_t role;
	uint8_t skin;
	uint8_t hairColor;
	uint8_t hairStyle;
	uint8_t passing;
	uint8_t shooting;
	uint8_t heading;
	uint8_t tackling;
	uint8_t control;
	uint8_t speed;
	uint8_t finishing;
};

struct __attribute__((packed)) SoccerAssetEntry {
	char name[24];
	uint32_t offset;
	uint32_t size;
	uint32_t unpackedSize;
	uint32_t crc;
	uint16_t width;
	uint16_t height;
};

struct __attribute__((packed)) SoccerXipCacheHeader {
	char magic[12];
	uint32_t version;
	uint32_t packVersion;
	uint8_t commit[20];
	uint32_t manifestCrc;
	uint32_t entryCount;
	uint32_t totalSize;
	uint32_t dataOffset;
	uint32_t tableCrc;
	uint32_t payloadCrc;
	uint32_t reserved[48];
};

struct __attribute__((packed)) SoccerXipCacheEntry {
	char name[24];
	uint32_t offset;
	uint32_t size;
	uint32_t crc;
};

struct SoccerImage {
	uint32_t assetOffset;
	uint32_t pixelsOffset;
	uint16_t width;
	uint16_t height;
	uint16_t paletteCount;
	uint8_t frameWidth;
	uint8_t frameHeight;
	const uint8_t *cachedPixels;
	uint32_t palette[SOCCER_ASSET_PALETTE_MAX];
	bool valid;
};

struct SoccerSpriteCache {
	uint32_t assetOffset;
	uint16_t frame;
	uint16_t width;
	uint16_t height;
	uint16_t capacity;
	uint32_t age;
	uint8_t *pixels;
};

struct SoccerReplayFrame {
	int16_t x[SOCCER_REPLAY_ENTITIES];
	int16_t y[SOCCER_REPLAY_ENTITIES];
	uint8_t z;
	uint8_t score0;
	uint8_t score1;
	uint8_t period;
};

struct SoccerReplayHeader {
	uint32_t magic;
	uint16_t version;
	uint16_t count;
	uint32_t check;
};

struct SoccerCustomTeam {
	uint32_t magic;
	char name[24];
	uint8_t shirt1;
	uint8_t shirt2;
	uint8_t shorts;
	uint8_t rating;
	char playerNames[16][16];
	uint8_t playerRatings[16];
	uint32_t check;
};

struct SoccerSave {
	uint32_t magic;
	uint16_t version;
	uint16_t size;
	uint32_t generation;
	uint8_t difficulty;
	uint8_t matchMinutes;
	uint8_t weather;
	uint8_t language;
	uint8_t music;
	uint8_t effects;
	uint8_t radar;
	uint8_t formation;
	uint16_t homeTeam;
	uint16_t awayTeam;
	uint8_t competitionType;
	uint8_t competitionRound;
	uint8_t competitionActive;
	uint8_t competitionWon;
	uint16_t competitionTeams[8];
	struct SoccerTableRow table[8];
	struct SoccerCustomTeam custom;
	uint32_t check;
};

struct SoccerApp {
	const struct DcAppHostApi *host;
	const struct DcAppRunArgs *args;
	struct DcAppDrawCtx draw;
	struct Dc32PortPak pak;
	struct SoccerPackHeader packHeader;
	struct SoccerTeamRecord home;
	struct SoccerTeamRecord away;
	struct SoccerPlayerRecord rosters[2][SOCCER_TEAM_PLAYERS];
	struct SoccerImage playerImages[2];
	struct SoccerImage keeperImage;
	struct SoccerImage ballImage;
	struct SoccerImage goalTopImage;
	struct SoccerImage goalBottomImage;
	struct SoccerImage shadowImage;
	struct SoccerImage hairImages[SOCCER_HAIR_STYLES];
	struct SoccerImage stadiumImages[4];
	struct SoccerSpriteCache spriteCache[SOCCER_SPRITE_CACHE_SLOTS];
	uint8_t spriteCachePixels[SOCCER_SPRITE_CACHE_REGULAR_SLOTS][SOCCER_SPRITE_CACHE_PIXELS];
	uint8_t spriteCacheLargePixels[SOCCER_SPRITE_CACHE_LARGE_SLOTS][SOCCER_SPRITE_CACHE_LARGE_PIXELS];
	uint8_t pitchTextureRows[SOCCER_PITCH_TEXTURE_SIZE]
		[SOCCER_SCREEN_W + SOCCER_PITCH_TEXTURE_SIZE];
	uint8_t fogPalette[256];
	int8_t playerOrigins[16][8][2];
	int8_t keeperOrigins[SOCCER_KEEPER_FRAMES][8][2];
	int8_t playerHairMap[16][8][4];
	struct SoccerMatch match;
	struct SoccerSave save;
	struct SoccerReplayFrame *replay;
	uint32_t replayWrite;
	uint32_t replayCount;
	uint32_t replayRead;
	uint64_t logicAccum;
	uint64_t lastLogicTime;
	uint32_t xipCacheStart;
	uint32_t xipCacheSize;
	const struct SoccerXipCacheEntry *xipCacheEntries;
	uint32_t xipCacheEntryCount;
	uint_fast16_t matchPrevKeys;
	uint32_t seed;
	uint16_t teamCursor;
	int16_t cameraX;
	int16_t cameraY;
	uint8_t screen;
	uint8_t previousScreen;
	uint8_t menuSelection;
	uint8_t subSelection;
	uint8_t purpose;
	uint8_t transferSelection;
	uint8_t keyboardCursor;
	uint8_t keyboardLength;
	uint8_t keyboardTarget;
	uint8_t playerCursor;
	uint8_t visibleMenuOffset;
	uint8_t penaltyScore[2];
	bool selectingAway;
	bool competitionMatch;
	bool extraTimePlayed;
	bool penaltyShootout;
	bool xipCacheActive;
	bool streamingFallback;
	bool touchDown;
	bool touchPressed;
	int16_t touchX;
	int16_t touchY;
	char status[52];
};

_Static_assert(sizeof(struct SoccerApp) < 130u * 1024u,
	"YSoccer runtime state leaves no room for replay/framebuffer allocations");
_Static_assert(sizeof(struct SoccerApp) +
	SOCCER_REPLAY_FRAMES * sizeof(struct SoccerReplayFrame) +
	SOCCER_FRAMEBUFFER_BYTES + 128u <= DC32_PORT_CART_HEAP_SIZE,
	"YSoccer app, replay, and framebuffer exceed the port heap");
_Static_assert(sizeof(struct SoccerXipCacheHeader) == 256u,
	"YSoccer XIP cache header ABI");
_Static_assert(sizeof(struct SoccerXipCacheEntry) == 36u,
	"YSoccer XIP cache entry ABI");

extern uint8_t __app_image_end[];

static volatile bool mSoccerAbort;

static uint16_t soccerRgb(uint32_t r, uint32_t g, uint32_t b)
{
	return dcAppDrawRgb565(r, g, b);
}

static uint32_t soccerCrc32Update(uint32_t crc, const void *data, uint32_t size)
{
	const uint8_t *bytes = data;

	crc = ~crc;
	while (size--) {
		crc ^= *bytes++;
		for (uint8_t bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
	}
	return ~crc;
}

static uint32_t soccerTextWidth(const char *text, enum Font font)
{
	uint32_t width = 0;

	while (text && *text) {
		struct FontGlyphInfo glyph;
		if (fontGetGlyphInfo(&glyph, font, (uint8_t)*text))
			width += glyph.width + 1u;
		text++;
	}
	return width ? width - 1u : 0;
}

static void soccerText(struct SoccerApp *app, int32_t x, int32_t y, const char *text,
	enum Font font, uint16_t color)
{
	while (text && *text) {
		struct FontGlyphInfo glyph;
		if (fontGetGlyphInfo(&glyph, font, (uint8_t)*text)) {
			for (uint8_t row = 0; row < glyph.height; row++)
				for (uint8_t col = 0; col < glyph.width; col++)
					if (fontGetGlyphPixel(&glyph, row, col))
						dcAppDrawPixel(&app->draw, x + col, y + row, color);
			x += glyph.width + 1;
		}
		text++;
	}
}

static void soccerCentered(struct SoccerApp *app, int32_t y, const char *text,
	enum Font font, uint16_t color)
{
	int32_t width = (int32_t)soccerTextWidth(text, font);
	soccerText(app, (SOCCER_SCREEN_W - width) / 2, y, text, font, color);
}

static void soccerSetStatus(struct SoccerApp *app, const char *text)
{
	snprintf(app->status, sizeof(app->status), "%s", text ? text : "");
}

static uint32_t soccerAlignUp(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1u) & ~(alignment - 1u);
}

static bool soccerCacheAssetWanted(const char name[24])
{
	return name && name[0];
}

static bool soccerDrawCacheProgress(struct SoccerApp *app, const char *asset,
	uint32_t entry, uint32_t count, uint32_t done, uint32_t total)
{
	char line[64];
	struct DcAppLoadingState loading = {
		.appName = "Sensible Soccer",
		.title = "Preloading game",
		.hint = "B cancels   FN menu",
		.done = done,
		.total = total,
		.animationStep = app->draw.frame,
	};

	snprintf(line, sizeof(line), "%s  %lu/%lu", asset ? asset : "CACHE",
		(unsigned long)entry, (unsigned long)count);
	loading.detail = line;
	dcAppDrawLoading(&app->draw, &loading);
	if (!dcAppDrawFrame(&app->draw, 0) || mSoccerAbort)
		return false;
	return !(app->draw.pressed & KEY_BIT_B);
}

static bool soccerCachePayloadValid(const struct SoccerXipCacheEntry *entries,
	uint32_t count, uint32_t expectedCrc)
{
	uint32_t crc = 0;

	for (uint32_t i = 0; i < count; i++) {
		const uint8_t *payload = flashUncachedPtr(entries[i].offset);

		if (!payload)
			return false;
		crc = soccerCrc32Update(crc, payload, entries[i].size);
	}
	return crc == expectedCrc;
}

static bool soccerActivateXipCache(struct SoccerApp *app,
	const struct SoccerXipCacheEntry *expected, uint32_t count,
	uint32_t manifestCrc, uint32_t totalSize)
{
	static const char magic[12] = "DC32YSX1";
	const struct SoccerXipCacheHeader *header = flashUncachedPtr(app->xipCacheStart);
	const struct SoccerXipCacheEntry *entries = flashUncachedPtr(
		app->xipCacheStart + sizeof(*header));
	uint32_t tableSize = count * sizeof(*entries);

	if (!header || !entries || memcmp(header->magic, magic, sizeof(magic)) ||
		header->version != SOCCER_XIP_CACHE_VERSION ||
		header->packVersion != SOCCER_PACK_VERSION ||
		memcmp(header->commit, app->packHeader.commit, sizeof(header->commit)) ||
		header->manifestCrc != manifestCrc || header->entryCount != count ||
		header->totalSize != totalSize ||
		header->dataOffset != SOCCER_XIP_CACHE_DATA_OFFSET ||
		header->tableCrc != soccerCrc32Update(0, entries, tableSize) ||
		memcmp(entries, expected, tableSize) ||
		!soccerCachePayloadValid(entries, count, header->payloadCrc))
		return false;
	app->xipCacheEntries = entries;
	app->xipCacheEntryCount = count;
	app->xipCacheActive = true;
	app->streamingFallback = false;
	return true;
}

static bool soccerFindAsset(struct SoccerApp *app, const char *name,
	struct SoccerAssetEntry *wanted)
{
	if (!app || !name || !wanted || app->packHeader.assetCount > 128u)
		return false;
	for (uint32_t i = 0; i < app->packHeader.assetCount; i++) {
		struct SoccerAssetEntry entry;
		uint32_t offset = app->packHeader.assetDirOffset + i * sizeof(entry);

		if (!dc32PortReadAssetPack(&app->pak, offset, &entry, sizeof(entry)))
			return false;
		entry.name[sizeof(entry.name) - 1u] = 0;
		if (!strcmp(entry.name, name)) {
			*wanted = entry;
			return true;
		}
	}
	return false;
}

static enum SoccerCacheResult soccerPrepareUniversalCache(struct SoccerApp *app)
{
	static const char magic[12] = "DC32YSX1";
	struct ToolWorkspaceSpan scratch;
	struct SoccerAssetEntry *sources;
	struct SoccerXipCacheEntry *entries;
	struct SoccerXipCacheHeader header;
	uint8_t *buffer;
	uint32_t bufferSize;
	uint32_t count = 0;
	uint32_t manifestCrc = 0;
	uint32_t payloadCrc = 0;
	uint32_t payloadBytes = 0;
	uint32_t dataOffset = SOCCER_XIP_CACHE_DATA_OFFSET;
	uint32_t workspaceOffset;
	uint32_t cacheEnd = QSPI_ROM_START + QSPI_ROM_SIZE_MAX;

	if (app->xipCacheActive)
		return SoccerCacheReady;
	app->xipCacheActive = false;
	app->xipCacheEntries = NULL;
	app->xipCacheEntryCount = 0;
	app->xipCacheStart = soccerAlignUp((uint32_t)(uintptr_t)__app_image_end,
		QSPI_ERASE_GRANULARITY);
	if (app->xipCacheStart >= cacheEnd)
		return SoccerCacheFailed;
	app->xipCacheSize = cacheEnd - app->xipCacheStart;
	if (!dcAppGetActiveScratch(&scratch) || !scratch.ptr)
		return SoccerCacheFailed;
	sources = scratch.ptr;
	entries = (struct SoccerXipCacheEntry*)(sources + SOCCER_XIP_CACHE_MAX_ENTRIES);
	workspaceOffset = soccerAlignUp(
		(uint32_t)((uint8_t*)(entries + SOCCER_XIP_CACHE_MAX_ENTRIES) -
		(uint8_t*)scratch.ptr), QSPI_WRITE_GRANULARITY);
	if (workspaceOffset >= scratch.size)
		return SoccerCacheFailed;
	buffer = (uint8_t*)scratch.ptr + workspaceOffset;
	bufferSize = (scratch.size - workspaceOffset) & ~(QSPI_WRITE_GRANULARITY - 1u);
	if (bufferSize > SOCCER_XIP_CACHE_CHUNK)
		bufferSize = SOCCER_XIP_CACHE_CHUNK;
	if (bufferSize < QSPI_WRITE_GRANULARITY)
		return SoccerCacheFailed;
	memset(sources, 0, sizeof(*sources) * SOCCER_XIP_CACHE_MAX_ENTRIES);
	memset(entries, 0, sizeof(*entries) * SOCCER_XIP_CACHE_MAX_ENTRIES);
	for (uint32_t i = 0; i < app->packHeader.assetCount; i++) {
		struct SoccerAssetEntry source;
		uint32_t directoryOffset = app->packHeader.assetDirOffset + i * sizeof(source);

		if (!dc32PortReadAssetPack(&app->pak, directoryOffset, &source, sizeof(source)))
			return SoccerCacheFailed;
		source.name[sizeof(source.name) - 1u] = 0;
		if (!soccerCacheAssetWanted(source.name))
			continue;
		if (count >= SOCCER_XIP_CACHE_MAX_ENTRIES || !source.size ||
			source.offset > app->pak.size || source.size > app->pak.size - source.offset)
			return SoccerCacheFailed;
		sources[count] = source;
		memcpy(entries[count].name, source.name, sizeof(entries[count].name));
		entries[count].offset = app->xipCacheStart + dataOffset;
		entries[count].size = source.size;
		entries[count].crc = source.crc;
		manifestCrc = soccerCrc32Update(manifestCrc, source.name, sizeof(source.name));
		manifestCrc = soccerCrc32Update(manifestCrc, &source.size, sizeof(source.size));
		manifestCrc = soccerCrc32Update(manifestCrc, &source.crc, sizeof(source.crc));
		payloadBytes += source.size;
		dataOffset += soccerAlignUp(source.size, QSPI_WRITE_GRANULARITY);
		count++;
	}
	if (!count || dataOffset + SOCCER_XIP_CACHE_SAFETY > app->xipCacheSize)
		return SoccerCacheFailed;
	if (!soccerDrawCacheProgress(app, "CHECKING CACHE", 0, count, 0, payloadBytes))
		return SoccerCacheCancelled;
	if (soccerActivateXipCache(app, entries, count, manifestCrc, dataOffset))
		return SoccerCacheReady;
	if (!app->host || !app->host->flashWrite ||
		!soccerDrawCacheProgress(app, "ERASING CACHE", 0, count, 0, 0))
		return app->host && app->host->flashWrite ? SoccerCacheCancelled : SoccerCacheFailed;
	if (!app->host->flashWrite(app->xipCacheStart,
		soccerAlignUp(dataOffset, QSPI_ERASE_GRANULARITY), NULL, 0))
		return SoccerCacheFailed;
	for (uint32_t i = 0, completed = 0; i < count; i++) {
		uint32_t position = 0;
		uint32_t assetCrc = 0;

		while (position < sources[i].size) {
			uint32_t amount = sources[i].size - position;
			uint32_t writeSize;

			if (amount > bufferSize)
				amount = bufferSize;
			if (!dc32PortReadAssetPack(&app->pak, sources[i].offset + position,
				buffer, amount))
				return SoccerCacheFailed;
			assetCrc = soccerCrc32Update(assetCrc, buffer, amount);
			payloadCrc = soccerCrc32Update(payloadCrc, buffer, amount);
			writeSize = soccerAlignUp(amount, QSPI_WRITE_GRANULARITY);
			if (writeSize > amount)
				memset(buffer + amount, 0xff, writeSize - amount);
			if (!app->host->flashWrite(entries[i].offset + position, 0,
				buffer, writeSize))
				return SoccerCacheFailed;
			position += amount;
			completed += amount;
		}
		if (assetCrc != sources[i].crc)
			return SoccerCacheFailed;
		if (!soccerDrawCacheProgress(app, entries[i].name, i + 1u, count,
			completed, payloadBytes))
			return SoccerCacheCancelled;
	}
	{
		uint32_t tableSize = count * sizeof(*entries);
		uint32_t writeSize = soccerAlignUp(tableSize, QSPI_WRITE_GRANULARITY);

		memcpy(buffer, entries, tableSize);
		memset(buffer + tableSize, 0xff, writeSize - tableSize);
		if (!app->host->flashWrite(app->xipCacheStart + sizeof(header), 0,
			buffer, writeSize))
			return SoccerCacheFailed;
		memset(&header, 0, sizeof(header));
		memcpy(header.magic, magic, sizeof(magic));
		header.version = SOCCER_XIP_CACHE_VERSION;
		header.packVersion = SOCCER_PACK_VERSION;
		memcpy(header.commit, app->packHeader.commit, sizeof(header.commit));
		header.manifestCrc = manifestCrc;
		header.entryCount = count;
		header.totalSize = dataOffset;
		header.dataOffset = SOCCER_XIP_CACHE_DATA_OFFSET;
		header.tableCrc = soccerCrc32Update(0, entries, tableSize);
		header.payloadCrc = payloadCrc;
		if (!app->host->flashWrite(app->xipCacheStart, 0, &header, sizeof(header)))
			return SoccerCacheFailed;
	}
	return soccerActivateXipCache(app, entries, count, manifestCrc, dataOffset) ?
		SoccerCacheReady : SoccerCacheFailed;
}

static const struct SoccerXipCacheEntry *soccerFindCachedEntry(
	struct SoccerApp *app, const char *name)
{
	if (!app->xipCacheActive || !name)
		return NULL;
	for (uint32_t i = 0; i < app->xipCacheEntryCount; i++) {
		const struct SoccerXipCacheEntry *entry = &app->xipCacheEntries[i];

		if (!strncmp(entry->name, name, sizeof(entry->name)))
			return entry;
	}
	return NULL;
}

static bool soccerOpenImage(struct SoccerApp *app, const char *name, struct SoccerImage *image)
{
	struct SoccerAssetEntry entry;
	const struct SoccerXipCacheEntry *cacheEntry;
	const uint8_t *cached;
	uint16_t header[4];

	memset(image, 0, sizeof(*image));
	memset(&entry, 0, sizeof(entry));
	cacheEntry = soccerFindCachedEntry(app, name);
	if (cacheEntry) {
		entry.offset = cacheEntry->offset;
		entry.size = cacheEntry->size;
		entry.crc = cacheEntry->crc;
		cached = flashUncachedPtr(cacheEntry->offset);
		if (!cached)
			return false;
	} else {
		if (app->xipCacheActive)
			return false;
		if (!soccerFindAsset(app, name, &entry))
			return false;
		cached = NULL;
	}
	if (entry.size < sizeof(header))
		return false;
	if (cached)
		memcpy(header, cached, sizeof(header));
	else if (!dc32PortReadAssetPack(&app->pak, entry.offset, header, sizeof(header)))
		return false;
	if (header[2] == 0u || header[2] > SOCCER_ASSET_PALETTE_MAX)
		return false;
	if ((uint32_t)sizeof(header) + (uint32_t)header[2] * sizeof(uint32_t) +
		(uint32_t)header[0] * header[1] > entry.size)
		return false;
	if (cached)
		memcpy(image->palette, cached + sizeof(header),
			(uint32_t)header[2] * sizeof(uint32_t));
	else if (!dc32PortReadAssetPack(&app->pak, entry.offset + sizeof(header),
		image->palette, (uint32_t)header[2] * sizeof(uint32_t)))
		return false;
	image->assetOffset = entry.offset;
	image->pixelsOffset = entry.offset + sizeof(header) + (uint32_t)header[2] * sizeof(uint32_t);
	image->cachedPixels = cached ? cached + sizeof(header) +
		(uint32_t)header[2] * sizeof(uint32_t) : NULL;
	image->width = header[0];
	image->height = header[1];
	image->paletteCount = header[2];
	image->frameWidth = (uint8_t)(header[3] & 0xffu);
	image->frameHeight = (uint8_t)(header[3] >> 8);
	if ((image->frameWidth || image->frameHeight) &&
		(!image->frameWidth || !image->frameHeight ||
		 image->width % image->frameWidth || image->height % image->frameHeight))
		return false;
	image->valid = true;
	return true;
}

static uint16_t soccerRgb332(uint8_t color)
{
	return dc32PortRgb332ToRgb565(color);
}

static uint8_t soccerRgb565To332(uint16_t color)
{
	return (uint8_t)((color >> 8) & 0xe0u) |
		(uint8_t)((color >> 6) & 0x1cu) |
		(uint8_t)((color >> 3) & 0x03u);
}

static bool soccerOpenPack(struct SoccerApp *app)
{
	static const char magic[12] = {'D','C','3','2','Y','S','O','C','V','5',0,0};
	uint8_t buffer[256];
	uint32_t offset;
	uint32_t remaining;
	uint32_t crc = 0;

	if (!app->args || !app->args->vol ||
		!dc32PortOpenAssetPack(app->args->vol, SOCCER_PACK_PATH, &app->pak) ||
		!dc32PortReadAssetPack(&app->pak, 0, &app->packHeader, sizeof(app->packHeader)))
		return false;
	if (memcmp(app->packHeader.magic, magic, sizeof(magic)) ||
		app->packHeader.version != SOCCER_PACK_VERSION ||
		app->packHeader.teamCount != SOCCER_TEAM_COUNT ||
		app->packHeader.recordSize != sizeof(struct SoccerTeamRecord) ||
		app->packHeader.playerOffset != sizeof(struct SoccerPackHeader) +
			app->packHeader.teamCount * app->packHeader.recordSize ||
		app->packHeader.assetDirOffset <= app->packHeader.playerOffset ||
		app->packHeader.assetCount == 0u || app->packHeader.assetCount > 128u) {
		dc32PortCloseAssetPack(&app->pak);
		return false;
	}
	offset = sizeof(app->packHeader);
	remaining = app->packHeader.assetDirOffset - offset;
	while (remaining) {
		uint32_t amount = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
		if (!dc32PortReadAssetPack(&app->pak, offset, buffer, amount)) {
			dc32PortCloseAssetPack(&app->pak);
			return false;
		}
		crc = soccerCrc32Update(crc, buffer, amount);
		offset += amount;
		remaining -= amount;
	}
	if (crc != app->packHeader.tableCrc) {
		dc32PortCloseAssetPack(&app->pak);
		return false;
	}
	crc = 0;
	offset = app->packHeader.assetDirOffset;
	remaining = app->packHeader.assetCount * sizeof(struct SoccerAssetEntry);
	while (remaining) {
		uint32_t amount = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
		if (!dc32PortReadAssetPack(&app->pak, offset, buffer, amount)) {
			dc32PortCloseAssetPack(&app->pak);
			return false;
		}
		crc = soccerCrc32Update(crc, buffer, amount);
		offset += amount;
		remaining -= amount;
	}
	if (crc != app->packHeader.assetDirCrc) {
		dc32PortCloseAssetPack(&app->pak);
		return false;
	}
	return true;
}

static bool soccerReadTeam(struct SoccerApp *app, uint16_t index, struct SoccerTeamRecord *team)
{
	uint32_t offset;

	if (index == app->packHeader.teamCount) {
		memset(team, 0, sizeof(*team));
		snprintf(team->name, sizeof(team->name), "%s", app->save.custom.name);
		snprintf(team->style, sizeof(team->style), "PLAIN");
		team->playerCount = SOCCER_TEAM_PLAYERS;
		team->shirt1 = soccerRgb332(app->save.custom.shirt1);
		team->shirt2 = soccerRgb332(app->save.custom.shirt2);
		team->shorts = soccerRgb332(app->save.custom.shorts);
		team->socks = team->shorts;
		team->seed = soccerCrc32Update(0, &app->save.custom, sizeof(app->save.custom));
		return true;
	}
	if (index >= app->packHeader.teamCount)
		index %= app->packHeader.teamCount;
	offset = sizeof(struct SoccerPackHeader) + (uint32_t)index * app->packHeader.recordSize;
	if (!dc32PortReadAssetPack(&app->pak, offset, team, sizeof(*team)))
		return false;
	team->name[sizeof(team->name) - 1u] = 0;
	team->style[sizeof(team->style) - 1u] = 0;
	return true;
}

static bool soccerReadRoster(struct SoccerApp *app, uint16_t teamIndex,
	struct SoccerPlayerRecord roster[SOCCER_TEAM_PLAYERS])
{
	if (teamIndex == app->packHeader.teamCount) {
		memset(roster, 0, sizeof(*roster) * SOCCER_TEAM_PLAYERS);
		for (uint8_t i = 0; i < SOCCER_TEAM_PLAYERS; i++) {
			snprintf(roster[i].name, sizeof(roster[i].name), "%.15s",
				app->save.custom.playerNames[i]);
			roster[i].number = i + 1u;
			roster[i].role = i == 0u ? 0u : (i < 5u ? 3u : (i < 9u ? 6u : 7u));
			roster[i].skin = 0u;
			roster[i].passing = roster[i].shooting = roster[i].heading =
				roster[i].tackling = roster[i].control = roster[i].speed =
				roster[i].finishing = app->save.custom.playerRatings[i];
		}
		return true;
	}
	if (teamIndex >= app->packHeader.teamCount)
		teamIndex %= app->packHeader.teamCount;
	return dc32PortReadAssetPack(&app->pak,
		app->packHeader.playerOffset + (uint32_t)teamIndex * SOCCER_TEAM_PLAYERS * sizeof(*roster),
		roster, SOCCER_TEAM_PLAYERS * sizeof(*roster));
}

static bool soccerReadAssetData(struct SoccerApp *app, const char *name, void *data,
	uint32_t size)
{
	const struct SoccerXipCacheEntry *cached = soccerFindCachedEntry(app, name);
	struct SoccerAssetEntry entry;
	const uint8_t *source;

	if (cached) {
		source = flashUncachedPtr(cached->offset);
		if (cached->size != size || !source)
			return false;
		memcpy(data, source, size);
		return true;
	}
	if (app->xipCacheActive)
		return false;
	return soccerFindAsset(app, name, &entry) && entry.size == size &&
		dc32PortReadAssetPack(&app->pak, entry.offset, data, size);
}

static uint16_t soccerDarken(uint16_t color, uint8_t shade)
{
	static const uint8_t factors[4] = {10u, 9u, 8u, 7u};
	uint32_t r = (color >> 11) & 31u;
	uint32_t g = (color >> 5) & 63u;
	uint32_t b = color & 31u;
	uint32_t factor = factors[shade < 4u ? shade : 3u];

	return (uint16_t)(((r * factor / 10u) << 11) |
		((g * factor / 10u) << 5) | (b * factor / 10u));
}

static uint16_t soccerPaletteColor(const struct SoccerTeamRecord *team, uint8_t skin,
	uint8_t hair, uint32_t value)
{
	static const uint16_t skinColors[9][3] = {
		{0xf48a, 0xb2e4, 0x7983}, {0x61e4, 0x3922, 0x1040}, {0xf570, 0xbbaa, 0x92a8},
		{0xf5ea, 0xbc64, 0x8323}, {0xdc6b, 0xb2e5, 0x79e3}, {0xcbea, 0x8b27, 0x6225},
		{0xf3eb, 0xaae7, 0x5984}, {0xd6ba, 0xbdf7, 0xa514}, {0x36e0, 0x0c80, 0x0ac0},
	};
	static const uint16_t hairColors[9][3] = {
		{0x2945, 0x18c3, 0x0841}, {0xa504, 0x8403, 0x62e2}, {0xa324, 0x7a63, 0x51a2},
		{0xe460, 0xb360, 0x7a60}, {0xffef, 0xe707, 0xce40}, {0x7bcf, 0x4a69, 0x2104},
		{0xd6ba, 0xad75, 0x8430}, {0xf815, 0x7166, 0x40c3}, {0xffc6, 0x8400, 0x5aa0},
	};
	uint8_t role;
	uint8_t group;
	uint8_t shade;
	uint16_t base;

	if (!(value & 0x80000000u))
		return (uint16_t)value;
	role = (uint8_t)value;
	if (role >= 21u && role <= 23u)
		return skinColors[skin < 9u ? skin : 0u][role - 21u];
	if (role >= 24u && role <= 26u)
		return hairColors[hair < 9u ? hair : 0u][role - 24u];
	if (role < 1u || role > 20u)
		return 0xffffu;
	group = (role - 1u) / 4u;
	shade = (role - 1u) % 4u;
	base = group == 0u ? team->shirt1 : (group == 1u ? team->shirt2 :
		(group == 2u ? team->shirt2 : (group == 3u ? team->shorts : team->socks)));
	return soccerDarken(base, shade);
}

static struct SoccerSpriteCache *soccerGetFrame(struct SoccerApp *app,
	const struct SoccerImage *image, uint8_t frameX, uint8_t frameY,
	uint16_t frameWidth, uint16_t frameHeight)
{
	struct SoccerSpriteCache *slot = NULL;
	uint16_t frame = (uint16_t)frameY * 32u + frameX;

	uint32_t pixelCount = (uint32_t)frameWidth * frameHeight;

	if (!image->valid || !frameWidth || !frameHeight ||
		pixelCount > SOCCER_SPRITE_CACHE_LARGE_PIXELS ||
		(uint32_t)(frameX + 1u) * frameWidth > image->width ||
		(uint32_t)(frameY + 1u) * frameHeight > image->height)
		return NULL;
	for (uint8_t i = 0; i < SOCCER_SPRITE_CACHE_SLOTS; i++) {
		struct SoccerSpriteCache *candidate = &app->spriteCache[i];

		if (candidate->capacity < pixelCount)
			continue;
		if (candidate->assetOffset == image->assetOffset && candidate->frame == frame &&
			candidate->width == frameWidth && candidate->height == frameHeight) {
			candidate->age = app->draw.frame;
			return candidate;
		}
		if (!slot || candidate->assetOffset == 0u || candidate->age < slot->age)
			slot = candidate;
	}
	if (!slot)
		return NULL;
	if (image->frameWidth == frameWidth && image->frameHeight == frameHeight) {
		uint32_t framesAcross = image->width / frameWidth;
		uint32_t source = image->pixelsOffset +
			((uint32_t)frameY * framesAcross + frameX) * pixelCount;

		if (!dc32PortReadAssetPack(&app->pak, source, slot->pixels, pixelCount)) {
			slot->assetOffset = 0u;
			return NULL;
		}
	}
	else {
		for (uint16_t row = 0; row < frameHeight; row++) {
			uint32_t source = image->pixelsOffset +
				((uint32_t)(frameY * frameHeight + row) * image->width) +
				frameX * frameWidth;
			if (!dc32PortReadAssetPack(&app->pak, source,
				slot->pixels + (uint32_t)row * frameWidth, frameWidth)) {
				slot->assetOffset = 0u;
				return NULL;
			}
		}
	}
	slot->assetOffset = image->assetOffset;
	slot->frame = frame;
	slot->width = frameWidth;
	slot->height = frameHeight;
	slot->age = app->draw.frame;
	return slot;
}

static void soccerResetSpriteCache(struct SoccerApp *app)
{
	memset(app->spriteCache, 0, sizeof(app->spriteCache));
	for (uint8_t i = 0; i < SOCCER_SPRITE_CACHE_REGULAR_SLOTS; i++) {
		app->spriteCache[i].pixels = app->spriteCachePixels[i];
		app->spriteCache[i].capacity = SOCCER_SPRITE_CACHE_PIXELS;
	}
	for (uint8_t i = 0; i < SOCCER_SPRITE_CACHE_LARGE_SLOTS; i++) {
		uint8_t slot = SOCCER_SPRITE_CACHE_REGULAR_SLOTS + i;

		app->spriteCache[slot].pixels = app->spriteCacheLargePixels[i];
		app->spriteCache[slot].capacity = SOCCER_SPRITE_CACHE_LARGE_PIXELS;
	}
}

static void soccerDrawIndexedFrame(struct SoccerApp *app, const struct SoccerImage *image,
	const struct SoccerTeamRecord *team, uint8_t skin, uint8_t hair,
	uint8_t frameX, uint8_t frameY,
	uint16_t frameWidth, uint16_t frameHeight, int32_t x, int32_t y)
{
	struct SoccerSpriteCache *frame = NULL;
	const uint8_t *pixels = NULL;
	uint32_t pixelCount = (uint32_t)frameWidth * frameHeight;

	if (image->cachedPixels && image->frameWidth == frameWidth &&
		image->frameHeight == frameHeight && frameWidth && frameHeight &&
		(uint32_t)(frameX + 1u) * frameWidth <= image->width &&
		(uint32_t)(frameY + 1u) * frameHeight <= image->height) {
		uint32_t framesAcross = image->width / frameWidth;

		pixels = image->cachedPixels +
			((uint32_t)frameY * framesAcross + frameX) * pixelCount;
	}
	else {
		frame = soccerGetFrame(app, image, frameX, frameY, frameWidth, frameHeight);
		if (frame)
			pixels = frame->pixels;
	}
	if (!pixels)
		return;
	for (uint16_t row = 0; row < frameHeight; row++)
		for (uint16_t col = 0; col < frameWidth; col++) {
			uint8_t index = pixels[(uint32_t)row * frameWidth + col];
			if (index && index < image->paletteCount)
				dcAppDrawPixel(&app->draw, x + col, y + row,
					soccerPaletteColor(team, skin, hair, image->palette[index]));
		}
}

static void soccerDrawIndexedImage(struct SoccerApp *app, const struct SoccerImage *image,
	const struct SoccerTeamRecord *team, int32_t x, int32_t y)
{
	uint8_t row[256];

	if (!image->valid || image->width > sizeof(row))
		return;
	for (uint16_t yy = 0; yy < image->height; yy++) {
		const uint8_t *pixels = image->cachedPixels ?
			image->cachedPixels + (uint32_t)yy * image->width : row;

		if (!image->cachedPixels && !dc32PortReadAssetPack(&app->pak,
			image->pixelsOffset + (uint32_t)yy * image->width, row, image->width))
			return;
		for (uint16_t xx = 0; xx < image->width; xx++)
			if (pixels[xx] && pixels[xx] < image->paletteCount)
				dcAppDrawPixel(&app->draw, x + xx, y + yy,
					soccerPaletteColor(team, 0u, 0u, image->palette[pixels[xx]]));
	}
}

static uint8_t soccerGrassRgb332(uint16_t color, uint8_t weather)
{
	uint32_t r = ((color >> 11) & 31u) * 255u / 31u;
	uint32_t g = ((color >> 5) & 63u) * 255u / 63u;
	uint32_t b = (color & 31u) * 255u / 31u;

	if (weather == SoccerWeatherSnow) {
		r = (r + 690u) / 4u;
		g = (g + 720u) / 4u;
		b = (b + 750u) / 4u;
	}
	else if (weather == SoccerWeatherRain) {
		r = r * 3u / 4u;
		g = g * 4u / 5u + 12u;
		b = b * 3u / 4u + 48u;
	}
	else if (weather == SoccerWeatherFog) {
		r = (r * 3u + 135u) / 4u;
		g = (g * 3u + 165u) / 4u;
		b = (b * 3u + 135u) / 4u;
	}
	else {
		r += 20u;
		g += 30u;
		b += 40u;
	}
	if (r > 255u) r = 255u;
	if (g > 255u) g = 255u;
	if (b > 255u) b = 255u;
	return (uint8_t)(((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6));
}

static void soccerPreparePitchTexture(struct SoccerApp *app)
{
	struct SoccerImage *image = &app->stadiumImages[0];
	uint16_t fallback = app->match.weather == SoccerWeatherSnow ?
		soccerRgb(190, 215, 220) : (app->match.weather == SoccerWeatherRain ?
		soccerRgb(24, 103, 59) : soccerRgb(42, 154, 72));
	uint8_t fallbackPixel = soccerGrassRgb332(fallback, app->match.weather);
	uint8_t row[SOCCER_PITCH_TEXTURE_SIZE];

	for (uint32_t color = 0; color < 256u; color++) {
		uint32_t r = (color >> 5) & 7u;
		uint32_t g = (color >> 2) & 7u;
		uint32_t b = color & 3u;

		r = (r * 2u + 6u) / 3u;
		g = (g * 2u + 6u) / 3u;
		b = (b * 2u + 3u) / 3u;
		app->fogPalette[color] = (uint8_t)((r << 5) | (g << 2) | b);
	}
	memset(app->pitchTextureRows, fallbackPixel, sizeof(app->pitchTextureRows));
	if (!image->valid ||
		SOCCER_PITCH_TEXTURE_SOURCE_X + SOCCER_PITCH_TEXTURE_SIZE > image->width ||
		SOCCER_PITCH_TEXTURE_SOURCE_Y + SOCCER_PITCH_TEXTURE_SIZE > image->height)
		return;
	for (uint32_t y = 0; y < SOCCER_PITCH_TEXTURE_SIZE; y++) {
		const uint8_t *pixels = image->cachedPixels ? image->cachedPixels +
			(SOCCER_PITCH_TEXTURE_SOURCE_Y + y) * image->width +
			SOCCER_PITCH_TEXTURE_SOURCE_X : row;

		if (!image->cachedPixels && !dc32PortReadAssetPack(&app->pak,
			image->pixelsOffset + (SOCCER_PITCH_TEXTURE_SOURCE_Y + y) * image->width +
			SOCCER_PITCH_TEXTURE_SOURCE_X, row, sizeof(row)))
			return;
		for (uint32_t x = 0; x < SOCCER_SCREEN_W + SOCCER_PITCH_TEXTURE_SIZE; x++) {
			uint8_t index = pixels[x & (SOCCER_PITCH_TEXTURE_SIZE - 1u)];

			if (index && index < image->paletteCount)
				app->pitchTextureRows[y][x] =
					soccerGrassRgb332((uint16_t)image->palette[index], app->match.weather);
		}
	}
}

static bool soccerLoadMatchImages(struct SoccerApp *app)
{
	static const char *const hairStyles[SOCCER_HAIR_STYLES] = {
		"BALD_A", "BALD_B", "CURLY_A", "CURLY_B", "CURLY_C", "CURLY_D", "CURLY_E", "CURLY_F",
		"PIGTAIL_A", "PIGTAIL_B", "PIGTAIL_C", "SHAVED", "SHORT_A", "SHORT_B", "SHORT_C",
		"SMOOTH_A", "SMOOTH_B", "SMOOTH_C", "SMOOTH_D", "SMOOTH_E", "SMOOTH_F", "SMOOTH_G",
		"SMOOTH_H", "SMOOTH_I", "SMOOTH_J", "SMOOTH_K", "SMOOTH_L", "SMOOTH_M", "SMOOTH_N",
		"SMOOTH_O", "SMOOTH_P", "SPECIAL_A", "SPECIAL_B", "SPECIAL_C", "SPECIAL_D", "SPECIAL_E",
		"SPECIAL_F", "SPECIAL_G", "SPECIAL_H", "THINNING_A", "THINNING_B", "THINNING_C",
	};
	char name[24];
	bool ok = true;

	snprintf(name, sizeof(name), "PLY_%s", app->home.style[0] ? app->home.style : "PLAIN");
	if (!soccerOpenImage(app, name, &app->playerImages[0]))
		ok = soccerOpenImage(app, "PLY_PLAIN", &app->playerImages[0]) && ok;
	snprintf(name, sizeof(name), "PLY_%s", app->away.style[0] ? app->away.style : "PLAIN");
	if (!soccerOpenImage(app, name, &app->playerImages[1]))
		ok = soccerOpenImage(app, "PLY_PLAIN", &app->playerImages[1]) && ok;
	ok = soccerOpenImage(app, "KEEPER", &app->keeperImage) && ok;
	ok = soccerOpenImage(app, app->match.weather == SoccerWeatherSnow ? "BALL_SNOW" : "BALL",
		&app->ballImage) && ok;
	ok = soccerOpenImage(app, "GOAL_TOP_A", &app->goalTopImage) && ok;
	ok = soccerOpenImage(app, "GOAL_BOTTOM", &app->goalBottomImage) && ok;
	ok = soccerOpenImage(app, "SHADOW0", &app->shadowImage) && ok;
	ok = soccerOpenImage(app, "STAD_TOP", &app->stadiumImages[0]) && ok;
	ok = soccerOpenImage(app, "STAD_BOTTOM", &app->stadiumImages[1]) && ok;
	ok = soccerOpenImage(app, "STAD_LEFT", &app->stadiumImages[2]) && ok;
	ok = soccerOpenImage(app, "STAD_RIGHT", &app->stadiumImages[3]) && ok;
	ok = soccerReadAssetData(app, "PLAYER_ORIGINS", app->playerOrigins,
		sizeof(app->playerOrigins)) && ok;
	ok = soccerReadAssetData(app, "KEEPER_ORIGINS", app->keeperOrigins,
		sizeof(app->keeperOrigins)) && ok;
	ok = soccerReadAssetData(app, "PLAYER_HAIR_MAP", app->playerHairMap,
		sizeof(app->playerHairMap)) && ok;
	memset(app->hairImages, 0, sizeof(app->hairImages));
	for (uint8_t team = 0; team < 2u; team++)
		for (uint8_t i = 1u; i < SOCCER_PLAYERS_PER_TEAM; i++) {
			uint8_t style = app->rosters[team][i].hairStyle;
			if (style < SOCCER_HAIR_STYLES && !app->hairImages[style].valid) {
				snprintf(name, sizeof(name), "HAIR_%s", hairStyles[style]);
				ok = soccerOpenImage(app, name, &app->hairImages[style]) && ok;
			}
		}
	soccerPreparePitchTexture(app);
	soccerResetSpriteCache(app);
	return ok;
}

static bool soccerEnsureDir(struct FatfsVol *vol, const char *path)
{
	struct FatfsDir *dir = fatfsDirOpen(vol, path);

	if (dir) {
		fatfsDirClose(dir);
		return true;
	}
	return fatfsDirCreate(vol, path, NULL);
}

static bool soccerEnsureSaveTree(struct SoccerApp *app)
{
	struct FatfsVol *vol = app->args ? app->args->vol : NULL;

	if (!vol || !dc32PortEnsureSaveDir(vol))
		return false;
	return soccerEnsureDir(vol, SOCCER_SAVE_ROOT) &&
		soccerEnsureDir(vol, SOCCER_SAVE_ROOT "/COMP") &&
		soccerEnsureDir(vol, SOCCER_SAVE_ROOT "/TEAMS") &&
		soccerEnsureDir(vol, SOCCER_SAVE_ROOT "/TACTICS") &&
		soccerEnsureDir(vol, SOCCER_SAVE_ROOT "/REPLAYS") &&
		soccerEnsureDir(vol, SOCCER_SAVE_ROOT "/HILITES") &&
		soccerEnsureDir(vol, SOCCER_SAVE_ROOT "/IMPORT") &&
		soccerEnsureDir(vol, SOCCER_SAVE_ROOT "/EXPORT");
}

static uint32_t soccerSaveCheck(const struct SoccerSave *save)
{
	return soccerCrc32Update(0, save, offsetof(struct SoccerSave, check));
}

static bool soccerSaveValid(const struct SoccerSave *save)
{
	return save->magic == SOCCER_SAVE_MAGIC &&
		save->version >= 1u && save->version <= SOCCER_SAVE_VERSION &&
		save->size == sizeof(*save) && save->check == soccerSaveCheck(save);
}

static uint16_t soccerMapLegacyTeam(struct SoccerApp *app, uint16_t legacy,
	uint16_t fallback, bool *removed)
{
	if (legacy == 1595u || legacy == SOCCER_LEGACY_TEAM_COUNT ||
		(app->save.version == 3u && legacy == 58u))
		return SOCCER_TEAM_COUNT;
	if (removed)
		*removed = true;
	return fallback < SOCCER_TEAM_COUNT ? fallback : 0u;
}

static void soccerMigrateLegacySave(struct SoccerApp *app)
{
	bool selectionRemoved = false;

	app->save.homeTeam = soccerMapLegacyTeam(app, app->save.homeTeam, 0u,
		&selectionRemoved);
	app->save.awayTeam = soccerMapLegacyTeam(app, app->save.awayTeam, 1u,
		&selectionRemoved);
	for (uint8_t i = 0; i < 8u; i++) {
		app->save.competitionTeams[i] = soccerMapLegacyTeam(app,
			app->save.competitionTeams[i], i % SOCCER_TEAM_COUNT, NULL);
		app->save.table[i].team = soccerMapLegacyTeam(app, app->save.table[i].team,
			i % SOCCER_TEAM_COUNT, NULL);
	}
	app->save.competitionActive = 0u;
	app->save.competitionRound = 0u;
	/* Previous builds treated one displayed minute as only 30 seconds per half. */
	if (app->save.matchMinutes == 1u || app->save.matchMinutes > 5u)
		app->save.matchMinutes = 3u;
	app->save.version = SOCCER_SAVE_VERSION;
	app->save.check = soccerSaveCheck(&app->save);
	soccerSetStatus(app, selectionRemoved ? "COUNTRY ROSTER APPLIED; TEAMS RESET" :
		"COUNTRY ROSTER APPLIED");
}

static void soccerSaveDefaults(struct SoccerSave *save)
{
	memset(save, 0, sizeof(*save));
	save->magic = SOCCER_SAVE_MAGIC;
	save->version = SOCCER_SAVE_VERSION;
	save->size = sizeof(*save);
	save->difficulty = 1;
	save->matchMinutes = 3;
	save->weather = SoccerWeatherDry;
	save->music = 0;
	save->effects = 0;
	save->radar = 1;
	save->homeTeam = 0;
	save->awayTeam = 1;
	save->custom.magic = SOCCER_TEAM_MAGIC;
	snprintf(save->custom.name, sizeof(save->custom.name), "DC32 UNITED");
	save->custom.shirt1 = 0xe0;
	save->custom.shirt2 = 0x1c;
	save->custom.shorts = 0x03;
	save->custom.rating = 5;
	for (uint8_t i = 0; i < 16; i++) {
		snprintf(save->custom.playerNames[i], sizeof(save->custom.playerNames[i]), "PLAYER %02u", i + 1u);
		save->custom.playerRatings[i] = 5;
	}
	save->check = soccerSaveCheck(save);
}

static void soccerLoadSave(struct SoccerApp *app)
{
	struct FatfsFil *file;
	struct SoccerSave slots[2];
	uint32_t read = 0;
	bool valid[2] = {false, false};

	soccerSaveDefaults(&app->save);
	if (!app->args || !app->args->vol)
		return;
	file = fatfsFileOpen(app->args->vol, SOCCER_SAVE_PATH, OPEN_MODE_READ);
	if (!file)
		return;
	memset(slots, 0, sizeof(slots));
	if (fatfsFileRead(file, slots, sizeof(slots), &read) && read >= sizeof(slots[0])) {
		valid[0] = soccerSaveValid(&slots[0]);
		valid[1] = read >= sizeof(slots) && soccerSaveValid(&slots[1]);
		if (valid[0] || valid[1]) {
			uint8_t selected = valid[1] && (!valid[0] || slots[1].generation > slots[0].generation) ? 1u : 0u;
			app->save = slots[selected];
			if (app->save.version < SOCCER_SAVE_VERSION)
				soccerMigrateLegacySave(app);
		}
	}
	fatfsFileClose(file);
	/* This badge port is intentionally silent, including migrated saves. */
	app->save.music = 0;
	app->save.effects = 0;
}

static bool soccerWriteSave(struct SoccerApp *app)
{
	struct FatfsFil *file;
	uint32_t written = 0;
	uint32_t slot;

	if (!soccerEnsureSaveTree(app))
		return false;
	app->save.generation++;
	app->save.check = soccerSaveCheck(&app->save);
	slot = (app->save.generation - 1u) & 1u;
	file = fatfsFileOpen(app->args->vol, SOCCER_SAVE_PATH, OPEN_MODE_WRITE | OPEN_MODE_CREATE);
	if (!file || !fatfsFileSeek(file, slot * sizeof(app->save)) ||
		!fatfsFileWrite(file, &app->save, sizeof(app->save), &written) || written != sizeof(app->save)) {
		if (file)
			fatfsFileClose(file);
		return false;
	}
	return fatfsFileClose(file);
}

static bool soccerTransferTeam(struct SoccerApp *app, bool import)
{
	struct FatfsFil *file;
	uint8_t data[686];
	uint32_t count = 0;
	static const uint8_t palette[10] = {
		0x00, 0xff, 0xe0, 0x1c, 0x03, 0xfc, 0xe3, 0x7f, 0x92, 0x6d,
	};

	if (!soccerEnsureSaveTree(app))
		return false;
	if (import) {
		file = fatfsFileOpen(app->args->vol, SOCCER_IMPORT_PATH, OPEN_MODE_READ);
		if (!file)
			return false;
		memset(data, 0, sizeof(data));
		if (!fatfsFileRead(file, data, sizeof(data), &count) || count < sizeof(data) ||
			!fatfsFileClose(file) || data[0] != 0 || data[1] == 0)
			return false;
		memset(app->save.custom.name, 0, sizeof(app->save.custom.name));
		memcpy(app->save.custom.name, data + 7, 16);
		app->save.custom.shirt1 = palette[data[29] % 10u];
		app->save.custom.shirt2 = palette[data[30] % 10u];
		app->save.custom.shorts = palette[data[31] % 10u];
		app->save.custom.rating = data[78 + 32] % 10u;
		if (!app->save.custom.rating)
			app->save.custom.rating = 5;
		for (uint8_t i = 0; i < 16; i++) {
			uint32_t playerOffset = 78u + (uint32_t)i * 38u;
			memset(app->save.custom.playerNames[i], 0, sizeof(app->save.custom.playerNames[i]));
			memcpy(app->save.custom.playerNames[i], data + playerOffset + 3u, 15u);
			app->save.custom.playerRatings[i] = data[playerOffset + 32u] % 10u;
			if (!app->save.custom.playerRatings[i]) app->save.custom.playerRatings[i] = 5;
		}
		return soccerWriteSave(app);
	}
	memset(data, 0, sizeof(data));
	data[1] = 1; /* big-endian team count */
	data[2] = 0; /* country */
	data[3] = 0; /* index */
	data[4] = 0xdc;
	data[5] = 0x32;
	memcpy(data + 7, app->save.custom.name,
		strlen(app->save.custom.name) < 16u ? strlen(app->save.custom.name) : 16u);
	data[26] = app->save.formation;
	data[28] = 0;
	data[29] = app->save.custom.shirt1 % 10u;
	data[30] = app->save.custom.shirt2 % 10u;
	data[31] = app->save.custom.shorts % 10u;
	data[32] = app->save.custom.shorts % 10u;
	data[33] = 0;
	data[34] = 1;
	data[35] = 2;
	data[36] = 0;
	data[37] = 1;
	data[38] = 2;
	data[39] = 2;
	memcpy(data + 38, "DC32 COACH", 10);
	for (uint8_t i = 0; i < 16; i++) {
		uint32_t positionOffset = 62u + i;
		uint32_t playerOffset = 78u + (uint32_t)i * 38u;
		char name[22];

		data[positionOffset] = i;
		data[playerOffset + 2u] = i + 1u;
		snprintf(name, sizeof(name), "%.15s", app->save.custom.playerNames[i]);
		memcpy(data + playerOffset + 3u, name, strlen(name));
		data[playerOffset + 26u] = (uint8_t)((i ? (1u + i % 3u) : 0u) << 5);
		data[playerOffset + 28u] = app->save.custom.playerRatings[i];
		data[playerOffset + 29u] = (uint8_t)(app->save.custom.playerRatings[i] << 4 | app->save.custom.playerRatings[i]);
		data[playerOffset + 30u] = data[playerOffset + 29u];
		data[playerOffset + 31u] = data[playerOffset + 29u];
		data[playerOffset + 32u] = app->save.custom.playerRatings[i];
	}
	file = fatfsFileOpen(app->args->vol, SOCCER_EXPORT_PATH,
		OPEN_MODE_WRITE | OPEN_MODE_CREATE | OPEN_MODE_TRUNCATE);
	if (!file)
		return false;
	return fatfsFileWrite(file, data, sizeof(data), &count) && count == sizeof(data) &&
		fatfsFileClose(file);
}

static bool soccerWriteReplay(struct SoccerApp *app, const char *path)
{
	struct SoccerReplayHeader header = {0x31505259u, 1u, (uint16_t)app->replayCount, 0};
	struct FatfsFil *file;
	uint32_t oldest;
	uint32_t written;

	if (!app->replayCount || !soccerEnsureSaveTree(app))
		return false;
	oldest = (app->replayWrite + SOCCER_REPLAY_FRAMES - app->replayCount) % SOCCER_REPLAY_FRAMES;
	for (uint32_t i = 0; i < app->replayCount; i++)
		header.check = soccerCrc32Update(header.check,
			&app->replay[(oldest + i) % SOCCER_REPLAY_FRAMES], sizeof(*app->replay));
	file = fatfsFileOpen(app->args->vol, path,
		OPEN_MODE_WRITE | OPEN_MODE_CREATE | OPEN_MODE_TRUNCATE);
	if (!file || !fatfsFileWrite(file, &header, sizeof(header), &written) || written != sizeof(header)) {
		if (file) fatfsFileClose(file);
		return false;
	}
	for (uint32_t i = 0; i < app->replayCount; i++) {
		struct SoccerReplayFrame *frame = &app->replay[(oldest + i) % SOCCER_REPLAY_FRAMES];
		if (!fatfsFileWrite(file, frame, sizeof(*frame), &written) || written != sizeof(*frame)) {
			fatfsFileClose(file);
			return false;
		}
	}
	return fatfsFileClose(file);
}

static bool soccerLoadReplay(struct SoccerApp *app)
{
	struct SoccerReplayHeader header;
	struct FatfsFil *file;
	uint32_t read;
	uint32_t check = 0;

	if (!app->args || !app->args->vol)
		return false;
	file = fatfsFileOpen(app->args->vol, SOCCER_REPLAY_PATH, OPEN_MODE_READ);
	if (!file || !fatfsFileRead(file, &header, sizeof(header), &read) || read != sizeof(header) ||
		header.magic != 0x31505259u || header.version != 1u || header.count > SOCCER_REPLAY_FRAMES) {
		if (file) fatfsFileClose(file);
		return false;
	}
	for (uint32_t i = 0; i < header.count; i++) {
		if (!fatfsFileRead(file, &app->replay[i], sizeof(*app->replay), &read) ||
			read != sizeof(*app->replay)) {
			fatfsFileClose(file);
			return false;
		}
		check = soccerCrc32Update(check, &app->replay[i], sizeof(*app->replay));
	}
	fatfsFileClose(file);
	if (check != header.check)
		return false;
	app->replayCount = header.count;
	app->replayWrite = header.count % SOCCER_REPLAY_FRAMES;
	return true;
}

static void soccerPollTouch(struct SoccerApp *app)
{
	struct UiTouchSample sample;
	bool down = false;
	int32_t x = app->touchX;
	int32_t y = app->touchY;

	if (uiReadTouchRaw(&sample) && sample.penDown) {
		down = true;
		x = 350 - (int32_t)sample.x * 94 / 1024;
		y = 260 - (int32_t)sample.y * 71 / 1024;
		if (x < 0) x = 0;
		if (x >= SOCCER_SCREEN_W) x = SOCCER_SCREEN_W - 1;
		if (y < 0) y = 0;
		if (y >= SOCCER_SCREEN_H) y = SOCCER_SCREEN_H - 1;
	}
	app->touchPressed = down && !app->touchDown;
	app->touchDown = down;
	app->touchX = (int16_t)x;
	app->touchY = (int16_t)y;
}

static void soccerPanel(struct SoccerApp *app, const char *title)
{
	dcAppDrawClear(&app->draw, soccerRgb(5, 15, 30));
	dcAppDrawFill(&app->draw, 0, 0, SOCCER_SCREEN_W, 28, soccerRgb(10, 65, 85));
	dcAppDrawFill(&app->draw, 0, 27, SOCCER_SCREEN_W, 1, soccerRgb(80, 220, 190));
	soccerCentered(app, 7, title, FontMedium, 0xffff);
	if (app->status[0]) {
		dcAppDrawFill(&app->draw, 0, 220, SOCCER_SCREEN_W, 20, soccerRgb(8, 38, 54));
		soccerCentered(app, 225, app->status, FontSmall, soccerRgb(210, 235, 225));
	}
}

static void soccerMenuItem(struct SoccerApp *app, int32_t y, const char *text, bool selected)
{
	if (selected)
		dcAppDrawFill(&app->draw, 36, y - 3, 248, 18, soccerRgb(20, 105, 105));
	soccerCentered(app, y, text, FontMedium, selected ? 0xffff : soccerRgb(175, 205, 215));
}

static void soccerDrawTitle(struct SoccerApp *app)
{
	dcAppDrawClear(&app->draw, soccerRgb(7, 58, 38));
	for (int y = 40; y < 210; y += 24)
		dcAppDrawLine(&app->draw, 0, y, SOCCER_SCREEN_W - 1, y, soccerRgb(10, 82, 50));
	dcAppDrawFill(&app->draw, 29, 42, 262, 91, soccerRgb(3, 24, 24));
	dcAppDrawFill(&app->draw, 31, 44, 258, 87, soccerRgb(15, 112, 65));
	soccerCentered(app, 59, "SENSIBLE SOCCER", FontLarge, 0xffff);
	soccerCentered(app, 91, "YSOCCER 19", FontMedium, soccerRgb(255, 226, 80));
	soccerCentered(app, 113, "GPL-2.0 NATIVE ADAPTATION", FontSmall, soccerRgb(195, 230, 215));
	if ((app->draw.frame / 30u) & 1u)
		soccerCentered(app, 177, "A / START  PLAY", FontMedium, 0xffff);
	soccerCentered(app, 220, "24 NATIONAL TEAMS  -  SWOS IMPORT", FontSmall, soccerRgb(175, 220, 195));
}

static const char *const mMainItems[SOCCER_MENU_ITEMS] = {
	"FRIENDLY", "CUP", "LEAGUE", "TOURNAMENT", "TRAINING", "CPU MATCH",
	"TEAM EDITOR", "TACTICS", "REPLAYS / HIGHLIGHTS", "OPTIONS",
	"IMPORT / EXPORT", "CREDITS", "EXIT",
};

static const char *soccerMainLabel(const struct SoccerApp *app, uint8_t item)
{
	static const char *const translated[9][6] = {
		{"FRIENDLY", "CUP", "LEAGUE", "TOURNAMENT", "TRAINING", "EXIT"},
		{"AMICHEVOLE", "COPPA", "LEGA", "TORNEO", "ALLENAMENTO", "ESCI"},
		{"MATCH AMICAL", "COUPE", "CHAMPIONNAT", "TOURNOI", "ENTRAINEMENT", "QUITTER"},
		{"FREUNDSCHAFTSSPIEL", "POKAL", "LIGA", "TURNIER", "TRAINING", "ENDE"},
		{"PARTIDO AMISTOSO", "COPA", "LIGA", "TORNEO", "ENTRENAMIENTO", "SALIR"},
		{"MECZ TOWARZYSKI", "PUCHAR", "LIGA", "TURNIEJ", "TRENING", "WYJSCIE"},
		{"PRIATELSKY ZAPAS", "POHAR", "LIGA", "TURNAJ", "TRENING", "SPAT"},
		{"SOPRUSKOHTUMINE", "KARIKAS", "LIIGA", "TURNIIR", "TRAINING", "LAHKU"},
		{"FILIKOS AGONAS", "KYPELO", "KATIGORIA", "TOURNOUA", "TRAINING", "EXODOS"},
	};
	uint8_t language = app->save.language % 9u;

	if (item < 5u)
		return translated[language][item];
	if (item == 12u)
		return translated[language][5];
	return mMainItems[item];
}

static void soccerDrawMain(struct SoccerApp *app)
{
	uint8_t start = app->visibleMenuOffset;

	soccerPanel(app, "SENSIBLE SOCCER (YSOCCER)");
	for (uint8_t row = 0; row < 9u && start + row < SOCCER_MENU_ITEMS; row++)
		soccerMenuItem(app, 39 + row * 19, soccerMainLabel(app, start + row),
			start + row == app->menuSelection);
	soccerText(app, 5, 204, "UP/DOWN SELECT   A ENTER   B TITLE", FontSmall,
		soccerRgb(130, 180, 190));
}

static void soccerDrawTeam(struct SoccerApp *app)
{
	struct SoccerTeamRecord team;
	char line[64];

	if (!soccerReadTeam(app, app->teamCursor, &team))
		memset(&team, 0, sizeof(team));
	soccerPanel(app, app->purpose == SoccerPurposeEditor ? "SELECT TEAM TO EDIT" :
		(app->selectingAway ? "SELECT AWAY TEAM" : "SELECT HOME TEAM"));
	dcAppDrawFill(&app->draw, 25, 58, 270, 93, soccerRgb(8, 52, 62));
	dcAppDrawFill(&app->draw, 38, 72, 68, 54, team.shirt1);
	dcAppDrawFill(&app->draw, 58, 72, 28, 54, team.shirt2);
	dcAppDrawFill(&app->draw, 38, 126, 68, 15, team.shorts);
	soccerText(app, 122, 75, team.name, FontMedium, 0xffff);
	snprintf(line, sizeof(line), "TEAM %u / %lu", app->teamCursor + 1u,
		(unsigned long)app->packHeader.teamCount + 1u);
	soccerText(app, 122, 102, line, FontSmall, soccerRgb(180, 220, 210));
	snprintf(line, sizeof(line), "COUNTRY %u   DIV %u", team.country, team.division + 1u);
	soccerText(app, 122, 119, line, FontSmall, soccerRgb(180, 220, 210));
	soccerCentered(app, 173, "LEFT/RIGHT: 25   UP/DOWN: 1", FontSmall, soccerRgb(160, 205, 215));
	soccerCentered(app, 194, "A CONFIRM   B BACK", FontSmall, 0xffff);
}

static const char *soccerCompetitionName(uint8_t type)
{
	return type == 1 ? "CUP" : (type == 2 ? "LEAGUE" : "TOURNAMENT");
}

static void soccerDrawCompetition(struct SoccerApp *app)
{
	char line[64];
	struct SoccerTeamRecord team;

	soccerPanel(app, soccerCompetitionName(app->save.competitionType));
	snprintf(line, sizeof(line), "ROUND %u", app->save.competitionRound + 1u);
	soccerCentered(app, 38, line, FontMedium, soccerRgb(255, 225, 90));
	for (uint8_t i = 0; i < 8; i++) {
		struct SoccerTableRow *row = &app->save.table[i];
		if (!soccerReadTeam(app, app->save.competitionTeams[i], &team))
			continue;
		snprintf(line, sizeof(line), "%-18.18s %2u  %2u-%2u", team.name, row->points,
			row->goalsFor, row->goalsAgainst);
		soccerText(app, 26, 68 + i * 16, line, FontSmall,
			i == 0 ? soccerRgb(255, 225, 90) : soccerRgb(190, 220, 215));
	}
	if (app->save.competitionActive)
		soccerCentered(app, 202, "A PLAY NEXT MATCH   B ABANDON", FontSmall, 0xffff);
	else
		soccerCentered(app, 202, app->save.competitionWon ? "CHAMPIONS!  A CONTINUE" : "ELIMINATED  A CONTINUE",
			FontSmall, app->save.competitionWon ? soccerRgb(255, 225, 90) : soccerRgb(255, 140, 120));
}

static void soccerDrawPlayer(struct SoccerApp *app, const struct SoccerPlayer *player,
	uint8_t playerIndex, int32_t cameraX, int32_t cameraY, bool controlled)
{
	int32_t x = (int32_t)player->x - cameraX;
	int32_t y = 20 + (int32_t)player->y - cameraY;
	uint8_t local = playerIndex % SOCCER_PLAYERS_PER_TEAM;
	const struct SoccerPlayerRecord *profile = &app->rosters[player->team][local];
	uint8_t frameX = player->facing & 7u;
	uint8_t frameY = player->frameY;
	const struct SoccerTeamRecord *team = player->team ? &app->away : &app->home;
	uint8_t skin = profile->skin;
	uint8_t hair = profile->hairColor;
	int32_t originX;
	int32_t originY;

	if (player->sentOff || y < 20 || y > 239)
		return;
	if (controlled) {
		uint16_t marker = soccerRgb(255, 240, 70);

		dcAppDrawLine(&app->draw, x - 5, y - 33, x, y - 27, marker);
		dcAppDrawLine(&app->draw, x + 5, y - 33, x, y - 27, marker);
		dcAppDrawLine(&app->draw, x - 5, y - 33, x + 5, y - 33, marker);
	}
	if (local == 0u) {
		if (frameY >= 19u) frameY = 1u;
		originX = app->keeperOrigins[frameY][frameX][0];
		originY = app->keeperOrigins[frameY][frameX][1];
		soccerDrawIndexedFrame(app, &app->keeperImage, team, skin, hair, frameX, frameY,
			50u, 50u, x - originX, y - originY - (int32_t)player->actionTicks / 4);
	}
	else {
		if (frameY >= 16u) frameY = 1u;
		originX = app->playerOrigins[frameY][frameX][0];
		originY = app->playerOrigins[frameY][frameX][1];
		soccerDrawIndexedFrame(app, &app->shadowImage, team, skin, hair, frameX, frameY,
			32u, 32u, x - originX + 3, y - originY + 5);
		soccerDrawIndexedFrame(app, &app->playerImages[player->team], team, skin, hair,
			frameX, frameY, 32u, 32u, x - originX, y - originY);
		if (profile->hairStyle < SOCCER_HAIR_STYLES) {
			const int8_t *map = app->playerHairMap[frameY][frameX];
			if (map[2] || map[3])
				soccerDrawIndexedFrame(app, &app->hairImages[profile->hairStyle], team,
					skin, hair, (uint8_t)map[0], (uint8_t)map[1], 20u, 20u,
					x - originX - 9 + map[2], y - originY - 9 + map[3]);
		}
	}
}

static void soccerDrawStadiumOutside(struct SoccerApp *app, int32_t cameraX, int32_t cameraY)
{
	uint8_t pixels[320];

	for (int32_t screenY = 20; screenY < SOCCER_SCREEN_H; screenY++) {
		int32_t worldY = cameraY + screenY - 20;
		int32_t screenX = 0;

		while (screenX < SOCCER_SCREEN_W) {
			int32_t worldX = cameraX + screenX;
			struct SoccerImage *image = NULL;
			int32_t sourceX = 0;
			int32_t sourceY = 0;
			int32_t run = SOCCER_SCREEN_W - screenX;
			const uint8_t *sourcePixels;

			if (worldY < 0) {
				image = &app->stadiumImages[0];
				sourceX = worldX + 96;
				sourceY = worldY + 96;
			}
			else if (worldY >= (int32_t)SOCCER_PITCH_H) {
				image = &app->stadiumImages[1];
				sourceX = worldX + 96;
				sourceY = worldY - (int32_t)SOCCER_PITCH_H;
			}
			else if (worldX < 0) {
				image = &app->stadiumImages[2];
				sourceX = worldX + 96;
				sourceY = worldY;
				if (run > -worldX)
					run = -worldX;
			}
			else if (worldX >= (int32_t)SOCCER_PITCH_W) {
				image = &app->stadiumImages[3];
				sourceX = worldX - (int32_t)SOCCER_PITCH_W;
				sourceY = worldY;
			}
			else {
				run = (int32_t)SOCCER_PITCH_W - worldX;
				if (run > SOCCER_SCREEN_W - screenX)
					run = SOCCER_SCREEN_W - screenX;
				screenX += run;
				continue;
			}
			if (!image->valid || sourceX < 0 || sourceY < 0 ||
				sourceY >= image->height || sourceX >= image->width)
				return;
			if (run > image->width - sourceX)
				run = image->width - sourceX;
			sourcePixels = image->cachedPixels ? image->cachedPixels +
				(uint32_t)sourceY * image->width + sourceX : pixels;
			if (!image->cachedPixels && !dc32PortReadAssetPack(&app->pak,
				image->pixelsOffset + (uint32_t)sourceY * image->width + sourceX,
				pixels, (uint32_t)run))
				return;
			for (int32_t i = 0; i < run; i++)
				if (sourcePixels[i] && sourcePixels[i] < image->paletteCount)
					dcAppDrawPixel(&app->draw, screenX + i, screenY,
						soccerPaletteColor(&app->home, 0u, 0u,
							image->palette[sourcePixels[i]]));
			screenX += run;
		}
	}
}

static void soccerDrawPitchTexture(struct SoccerApp *app, int32_t cameraX, int32_t cameraY,
	int32_t left, int32_t top, int32_t right, int32_t bottom)
{
	int32_t screenLeft = left > 0 ? left : 0;
	int32_t screenRight = right < SOCCER_SCREEN_W ? right : SOCCER_SCREEN_W;
	int32_t screenTop = top > 20 ? top : 20;
	int32_t screenBottom = bottom < SOCCER_SCREEN_H ? bottom : SOCCER_SCREEN_H;

	for (int32_t screenY = screenTop; screenY < screenBottom; screenY++) {
		uint32_t worldY = (uint32_t)(cameraY + screenY - 20);
		uint32_t worldX = (uint32_t)(cameraX + screenLeft);
		uint8_t *destination = app->draw.fb + (uint32_t)screenY * app->draw.w + screenLeft;
		const uint8_t *textureRow = app->pitchTextureRows[
			worldY & (SOCCER_PITCH_TEXTURE_SIZE - 1u)];

		memcpy(destination, textureRow +
			(worldX & (SOCCER_PITCH_TEXTURE_SIZE - 1u)),
			(uint32_t)(screenRight - screenLeft));
	}
}

static void soccerDrawPitch(struct SoccerApp *app, int32_t cameraX, int32_t cameraY)
{
	int32_t top = 20 - cameraY;
	int32_t bottom = top + (int32_t)SOCCER_PITCH_H;
	int32_t left = -cameraX;
	int32_t right = left + (int32_t)SOCCER_PITCH_W;
	int32_t centerX = left + (int32_t)(SOCCER_PITCH_W / 2.0f);
	uint16_t line = app->match.weather == SoccerWeatherSnow ? soccerRgb(70, 115, 125) : 0xffff;

	dcAppDrawClear(&app->draw, soccerRgb(5, 42, 25));
	soccerDrawStadiumOutside(app, cameraX, cameraY);
	soccerDrawPitchTexture(app, cameraX, cameraY, left, top, right, bottom);
	dcAppDrawLine(&app->draw, left, top, right, top, line);
	dcAppDrawLine(&app->draw, left, bottom, right, bottom, line);
	dcAppDrawLine(&app->draw, left, top, left, bottom, line);
	dcAppDrawLine(&app->draw, right, top, right, bottom, line);
	dcAppDrawLine(&app->draw, left, top + 640, right, top + 640, line);
	/* YSoccer 19 penalty and goal-area dimensions: 572x174 and 252x58. */
	dcAppDrawLine(&app->draw, centerX - 286, top, centerX - 286, top + 174, line);
	dcAppDrawLine(&app->draw, centerX + 286, top, centerX + 286, top + 174, line);
	dcAppDrawLine(&app->draw, centerX - 286, top + 174, centerX + 286, top + 174, line);
	dcAppDrawLine(&app->draw, centerX - 126, top, centerX - 126, top + 58, line);
	dcAppDrawLine(&app->draw, centerX + 126, top, centerX + 126, top + 58, line);
	dcAppDrawLine(&app->draw, centerX - 126, top + 58, centerX + 126, top + 58, line);
	dcAppDrawLine(&app->draw, centerX - 286, bottom, centerX - 286, bottom - 174, line);
	dcAppDrawLine(&app->draw, centerX + 286, bottom, centerX + 286, bottom - 174, line);
	dcAppDrawLine(&app->draw, centerX - 286, bottom - 174, centerX + 286, bottom - 174, line);
	dcAppDrawLine(&app->draw, centerX - 126, bottom, centerX - 126, bottom - 58, line);
	dcAppDrawLine(&app->draw, centerX + 126, bottom, centerX + 126, bottom - 58, line);
	dcAppDrawLine(&app->draw, centerX - 126, bottom - 58, centerX + 126, bottom - 58, line);
	if (top > -60 && top < SOCCER_SCREEN_H)
		soccerDrawIndexedImage(app, &app->goalTopImage, &app->home, centerX - 73, top - 52);
	if (bottom > -60 && bottom < SOCCER_SCREEN_H)
		soccerDrawIndexedImage(app, &app->goalBottomImage, &app->away, centerX - 73, bottom);
}

static void soccerDrawRadar(struct SoccerApp *app)
{
	if (!app->match.radar)
		return;
	dcAppDrawFill(&app->draw, 267, 31, 47, 67, soccerRgb(2, 34, 28));
	dcAppDrawLine(&app->draw, 268, 64, 312, 64, soccerRgb(130, 180, 150));
	for (uint8_t i = 0; i < SOCCER_PLAYER_COUNT; i++) {
		const struct SoccerPlayer *player = &app->match.players[i];
		int32_t x = 269 + (int32_t)(player->x * 42.0f / SOCCER_PITCH_W);
		int32_t y = 33 + (int32_t)(player->y * 62.0f / SOCCER_PITCH_H);
		bool controlled = i == (uint8_t)app->match.controlled;

		if (player->sentOff)
			continue;
		dcAppDrawFill(&app->draw, x - (controlled ? 1 : 0), y - (controlled ? 1 : 0),
			controlled ? 3 : 2, controlled ? 3 : 2, controlled ?
			soccerRgb(255, 240, 70) : (player->team ?
			soccerRgb(255, 90, 80) : soccerRgb(70, 170, 255)));
	}
}

static void soccerDrawBallAt(struct SoccerApp *app, float worldX, float worldY,
	float worldZ, int32_t cameraX, int32_t cameraY)
{
	int32_t ballX = (int32_t)worldX - cameraX;
	int32_t ballY = 20 + (int32_t)worldY - cameraY - (int32_t)worldZ;

	/* A fixed source frame avoids the high-contrast rotating frames reading as
	 * a blink on the badge LCD. */
	soccerDrawIndexedFrame(app, &app->ballImage, &app->home, 0u, 0u,
		0u, 0u, 8u, 8u, ballX - 4, ballY - 4);
}

static void soccerDrawBallShadowAt(struct SoccerApp *app, float worldX, float worldY,
	float worldZ, int32_t cameraX, int32_t cameraY)
{
	int32_t x = (int32_t)worldX - cameraX;
	int32_t y = 22 + (int32_t)worldY - cameraY;
	int32_t radius = worldZ < 24.0f ? 3 : (worldZ < 72.0f ? 2 : 1);
	uint16_t color = app->match.weather == SoccerWeatherSnow ?
		soccerRgb(75, 100, 105) : (worldZ < 48.0f ?
		soccerRgb(9, 50, 25) : soccerRgb(25, 78, 42));

	dcAppDrawLine(&app->draw, x - radius, y, x + radius, y, color);
	if (radius > 1)
		dcAppDrawLine(&app->draw, x - radius + 1, y + 1, x + radius - 1, y + 1,
			color);
}

static void soccerDrawBall(struct SoccerApp *app, int32_t cameraX, int32_t cameraY)
{
	soccerDrawBallAt(app, app->match.ball.x, app->match.ball.y, app->match.ball.z,
		cameraX, cameraY);
}

static void soccerDrawWeather(struct SoccerApp *app)
{
	static const uint8_t fogDither[16] = {
		0u, 8u, 2u, 10u, 12u, 4u, 14u, 6u,
		3u, 11u, 1u, 9u, 15u, 7u, 13u, 5u,
	};

	if (app->match.weather == SoccerWeatherRain)
		for (uint8_t i = 0; i < 28; i++) {
			int x = (int)((i * 47u + app->draw.frame * 3u) % 320u);
			int y = (int)((i * 29u + app->draw.frame * 5u) % 240u);
			dcAppDrawLine(&app->draw, x, y, x - 2, y + 5,
				soccerRgb(135, 180, 220));
		}
	else if (app->match.weather == SoccerWeatherSnow)
		for (uint8_t i = 0; i < 22; i++)
			dcAppDrawFill(&app->draw, (i * 71u + app->draw.frame) % 320u,
				(i * 37u + app->draw.frame * 2u) % 240u, 2, 2, 0xffff);
	else if (app->match.weather == SoccerWeatherFog)
		for (uint32_t y = 20u; y < SOCCER_SCREEN_H; y++) {
			uint8_t *row = app->draw.fb + y * app->draw.w;

			for (uint32_t x = 0; x < SOCCER_SCREEN_W; x++) {
				uint8_t fogged = app->fogPalette[row[x]];

				if (fogDither[((y & 3u) << 2) | (x & 3u)] < 5u)
					fogged = app->fogPalette[fogged];
				row[x] = fogged;
			}
		}
}

static void soccerDrawMatch(struct SoccerApp *app)
{
	char score[32];
	char home[16];
	char away[16];
	char bannerText[48];
	char bannerText3[48];
	const char *banner = NULL;
	const char *banner2 = NULL;
	const char *banner3 = NULL;
	uint32_t clockSeconds;
	uint32_t periodElapsed;
	uint32_t periodLength;
	uint32_t addedMinutes = 0u;
	int32_t cameraX = (int32_t)app->match.ball.x - 160;
	int32_t cameraY = (int32_t)app->match.ball.y - 110;

	if (cameraX < -96) cameraX = -96;
	if (cameraX > (int32_t)SOCCER_PITCH_W - SOCCER_SCREEN_W + 96)
		cameraX = (int32_t)SOCCER_PITCH_W - SOCCER_SCREEN_W + 96;
	if (cameraY < -96) cameraY = -96;
	if (cameraY > (int32_t)SOCCER_PITCH_H - 220 + 96)
		cameraY = (int32_t)SOCCER_PITCH_H - 220 + 96;
	app->cameraX = (int16_t)cameraX;
	app->cameraY = (int16_t)cameraY;
	soccerDrawPitch(app, cameraX, cameraY);
	soccerDrawBallShadowAt(app, app->match.ball.x, app->match.ball.y,
		app->match.ball.z, cameraX, cameraY);
	if (app->match.ball.z < 8.0f)
		soccerDrawBall(app, cameraX, cameraY);
	for (uint8_t i = 0; i < SOCCER_PLAYER_COUNT; i++)
		soccerDrawPlayer(app, &app->match.players[i], i, cameraX, cameraY,
			i == (uint8_t)app->match.controlled);
	if (app->match.ball.z >= 8.0f)
		soccerDrawBall(app, cameraX, cameraY);
	soccerDrawWeather(app);
	dcAppDrawFill(&app->draw, 0, 0, SOCCER_SCREEN_W, 20, soccerRgb(2, 18, 24));
	clockSeconds = app->match.clockTicks / SOCCER_LOGIC_HZ;
	periodElapsed = app->match.clockTicks - app->match.periodClockStart;
	periodLength = app->match.period >= 2u ? app->match.halfTicks / 3u :
		app->match.halfTicks;
	if (periodElapsed > periodLength)
		addedMinutes = (periodElapsed - periodLength + SOCCER_LOGIC_HZ * 60u - 1u) /
			(SOCCER_LOGIC_HZ * 60u);
	snprintf(home, sizeof(home), "%.10s", app->home.name);
	snprintf(away, sizeof(away), "%.10s", app->away.name);
	soccerText(app, 4, 5, home, FontSmall, 0xffff);
	soccerText(app, 316 - (int32_t)soccerTextWidth(away, FontSmall), 5, away, FontSmall, 0xffff);
	if (addedMinutes)
		snprintf(score, sizeof(score), "%u-%u %02lu:%02lu +%lu", app->match.score[0],
			app->match.score[1], (unsigned long)(clockSeconds / 60u),
			(unsigned long)(clockSeconds % 60u), (unsigned long)addedMinutes);
	else
		snprintf(score, sizeof(score), "%u-%u  %02lu:%02lu", app->match.score[0],
			app->match.score[1], (unsigned long)(clockSeconds / 60u),
			(unsigned long)(clockSeconds % 60u));
	soccerCentered(app, 5, score, FontSmall, soccerRgb(255, 235, 130));
	soccerDrawRadar(app);
	if (app->match.restart == SoccerRestartKickoff &&
		app->match.restartPhase == SoccerRestartPhaseAnnounce) {
		if (app->match.breakReason == SoccerBreakGoal) {
			snprintf(bannerText, sizeof(bannerText), "GOAL - %.12s",
				app->match.goalTeam ? app->away.name : app->home.name);
			banner = bannerText;
		}
		else if (app->match.breakReason == SoccerBreakHalfTime)
			banner = "HALF TIME";
		else if (app->match.breakReason == SoccerBreakExtraTime)
			banner = "EXTRA TIME";
	}
	else if (app->match.finished && app->match.bannerTicks)
		banner = "FULL TIME";
	else if (app->match.restart == SoccerRestartKickoff) {
		if (app->match.restartNoticeTicks) {
			snprintf(bannerText, sizeof(bannerText), "KICK OFF: %.12s",
				app->match.restartTeam ? app->away.name : app->home.name);
			banner = bannerText;
		}
	}
	else if (app->match.restart == SoccerRestartFreeKick &&
		app->match.restartNoticeTicks &&
		app->match.foulPlayer >= 0 && app->match.foulPlayer < SOCCER_PLAYER_COUNT &&
		app->match.foulVictim >= 0 && app->match.foulVictim < SOCCER_PLAYER_COUNT) {
		static const char *const reasons[] = {
			"FOUL", "LATE TACKLE", "BACK TACKLE", "SIDE TACKLE", "FRONT TACKLE"
		};
		const struct SoccerPlayerRecord *offender = &app->rosters[
			app->match.foulPlayer / SOCCER_PLAYERS_PER_TEAM]
			[app->match.foulPlayer % SOCCER_PLAYERS_PER_TEAM];
		const struct SoccerPlayerRecord *victim = &app->rosters[
			app->match.foulVictim / SOCCER_PLAYERS_PER_TEAM]
			[app->match.foulVictim % SOCCER_PLAYERS_PER_TEAM];
		uint8_t reason = app->match.foulReason < sizeof(reasons) / sizeof(reasons[0]) ?
			app->match.foulReason : 0u;

		snprintf(bannerText, sizeof(bannerText), "%s: %.10s ON %.10s", reasons[reason],
			offender->name, victim->name);
		if (app->match.cardType == SoccerCardYellow)
			banner2 = "YELLOW CARD: RECKLESS FOUL";
		else if (app->match.cardType == SoccerCardSecondYellow)
			banner2 = "RED CARD: SECOND YELLOW";
		else if (app->match.cardType == SoccerCardRed)
			banner2 = "RED CARD: SERIOUS FOUL PLAY";
		snprintf(bannerText3, sizeof(bannerText3), "FREE KICK: %.12s",
			app->match.restartTeam ? app->away.name : app->home.name);
		banner = bannerText;
		if (banner2)
			banner3 = bannerText3;
		else
			banner2 = bannerText3;
	}
	else if (app->match.restart) {
		const char *label = app->match.restart == SoccerRestartThrowIn ? "THROW IN" :
			(app->match.restart == SoccerRestartCorner ? "CORNER" :
			(app->match.restart == SoccerRestartGoalKick ? "GOAL KICK" : "FREE KICK"));

		if (app->match.restartNoticeTicks)
			banner = label;
	}
	else if (app->match.bannerTicks && app->match.event == SoccerEventHalfTime)
		banner = "HALF TIME";
	else if (app->match.bannerTicks && app->match.period >= 2u)
		banner = "EXTRA TIME";
	if (banner) {
		if (banner2) {
			dcAppDrawFill(&app->draw, 10, banner3 ? 88 : 97, 300,
				banner3 ? 61 : 43, soccerRgb(2, 20, 26));
			soccerCentered(app, banner3 ? 96 : 105, banner, FontSmall,
				soccerRgb(255, 225, 90));
			soccerCentered(app, banner3 ? 113 : 122, banner2, FontSmall, 0xffff);
			if (banner3)
				soccerCentered(app, 130, banner3, FontSmall, soccerRgb(170, 220, 255));
		}
		else {
			dcAppDrawFill(&app->draw, 90, 104, 140, 28, soccerRgb(2, 20, 26));
			soccerCentered(app, 112, banner, FontMedium, soccerRgb(255, 225, 90));
		}
	}
}

static void soccerDrawPause(struct SoccerApp *app)
{
	static const char *const items[] = {"RESUME", "RADAR", "TACTICS / SUBSTITUTE", "ACTION REPLAY", "SAVE HIGHLIGHT", "QUIT MATCH"};

	soccerPanel(app, "MATCH OPTIONS");
	for (uint8_t i = 0; i < sizeof(items) / sizeof(items[0]); i++)
		soccerMenuItem(app, 42 + i * 26, items[i], app->subSelection == i);
}

static void soccerDrawResult(struct SoccerApp *app)
{
	char line[64];
	uint32_t possessionTotal = app->match.possession[0] + app->match.possession[1];
	uint8_t topScorer = 0;

	for (uint8_t i = 1; i < SOCCER_PLAYER_COUNT; i++)
		if (app->match.playerGoals[i] > app->match.playerGoals[topScorer])
			topScorer = i;

	soccerPanel(app, "FULL TIME");
	soccerCentered(app, 43, app->home.name, FontMedium, soccerRgb(120, 205, 255));
	snprintf(line, sizeof(line), "%u  -  %u", app->match.score[0], app->match.score[1]);
	soccerCentered(app, 72, line, FontLarge, soccerRgb(255, 225, 90));
	if (app->penaltyShootout) {
		snprintf(line, sizeof(line), "PENALTIES  %u - %u",
			app->penaltyScore[0], app->penaltyScore[1]);
		soccerCentered(app, 96, line, FontSmall, soccerRgb(255, 225, 90));
	}
	soccerCentered(app, 108, app->away.name, FontMedium, soccerRgb(255, 135, 115));
	snprintf(line, sizeof(line), "SHOTS       %u  -  %u", app->match.shots[0], app->match.shots[1]);
	soccerCentered(app, 137, line, FontSmall, soccerRgb(190, 220, 215));
	snprintf(line, sizeof(line), "POSSESSION  %lu%% - %lu%%",
		(unsigned long)(possessionTotal ? app->match.possession[0] * 100u / possessionTotal : 50u),
		(unsigned long)(possessionTotal ? app->match.possession[1] * 100u / possessionTotal : 50u));
	soccerCentered(app, 152, line, FontSmall, soccerRgb(190, 220, 215));
	snprintf(line, sizeof(line), "FOULS       %u  -  %u", app->match.fouls[0], app->match.fouls[1]);
	soccerCentered(app, 167, line, FontSmall, soccerRgb(190, 220, 215));
	if (app->match.playerGoals[topScorer]) {
		snprintf(line, sizeof(line), "TOP SCORER  %s #%u (%u)",
			app->match.players[topScorer].team ? app->away.name : app->home.name,
			app->match.players[topScorer].number, app->match.playerGoals[topScorer]);
		soccerCentered(app, 182, line, FontSmall, soccerRgb(255, 225, 90));
	}
	soccerCentered(app, 204, "A CONTINUE   SELECT REPLAY", FontSmall, 0xffff);
}

static void soccerDrawOptions(struct SoccerApp *app)
{
	static const char *const languages[] = {"ENGLISH", "ITALIANO", "FRANCAIS", "DEUTSCH", "ESPANOL", "POLSKI", "SLOVAK", "EESTI", "GREEK"};
	static const char *const weather[] = {"DRY", "RAIN", "SNOW", "FOG"};
	char lines[5][40];

	soccerPanel(app, "OPTIONS");
	snprintf(lines[0], sizeof(lines[0]), "DIFFICULTY       %u", app->save.difficulty + 1u);
	snprintf(lines[1], sizeof(lines[1]), "HALF LENGTH      %u MIN", app->save.matchMinutes);
	snprintf(lines[2], sizeof(lines[2]), "WEATHER          %s", weather[app->save.weather % 4u]);
	snprintf(lines[3], sizeof(lines[3]), "RADAR            %s", app->save.radar ? "ON" : "OFF");
	snprintf(lines[4], sizeof(lines[4]), "LANGUAGE         %s", languages[app->save.language % 9u]);
	for (uint8_t i = 0; i < 5; i++)
		soccerMenuItem(app, 52 + i * 27, lines[i], app->subSelection == i);
	soccerCentered(app, 190, "AUDIO DISABLED", FontSmall, soccerRgb(130, 180, 190));
	soccerCentered(app, 204, "LEFT/RIGHT CHANGE   B SAVE", FontSmall, soccerRgb(180, 220, 215));
}

static void soccerDrawEditor(struct SoccerApp *app)
{
	static const char *const labels[] = {"NAME", "PLAYERS", "SHIRT 1", "SHIRT 2", "SHORTS", "RATING", "EXPORT", "BACK"};
	char line[48];

	soccerPanel(app, "TEAM EDITOR");
	for (uint8_t i = 0; i < 8; i++) {
		if (i == 0)
			snprintf(line, sizeof(line), "%-10s %s", labels[i], app->save.custom.name);
		else if (i == 5)
			snprintf(line, sizeof(line), "%-10s %u", labels[i], app->save.custom.rating);
		else
			snprintf(line, sizeof(line), "%s", labels[i]);
		soccerMenuItem(app, 37 + i * 21, line, app->subSelection == i);
		if (i >= 2 && i <= 4) {
			uint8_t color = i == 2 ? app->save.custom.shirt1 : (i == 3 ? app->save.custom.shirt2 : app->save.custom.shorts);
			dcAppDrawFill(&app->draw, 260, 36 + i * 21, 16, 12, soccerRgb332(color));
		}
	}
}

static void soccerDrawPlayers(struct SoccerApp *app)
{
	char line[40];
	uint8_t start = app->playerCursor > 8u ? app->playerCursor - 8u : 0u;

	soccerPanel(app, "PLAYER EDITOR");
	for (uint8_t row = 0; row < 9u && start + row < 16u; row++) {
		uint8_t index = start + row;
		snprintf(line, sizeof(line), "%02u  %-15.15s  %u", index + 1u,
			app->save.custom.playerNames[index], app->save.custom.playerRatings[index]);
		soccerMenuItem(app, 38 + row * 19, line, app->playerCursor == index);
	}
	soccerCentered(app, 207, "A NAME   LEFT/RIGHT SKILL   B BACK", FontSmall, 0xffff);
}

static const char mKeyboard[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789-.";

static char *soccerKeyboardText(struct SoccerApp *app, uint32_t *size)
{
	if (app->keyboardTarget) {
		if (size) *size = sizeof(app->save.custom.playerNames[0]);
		return app->save.custom.playerNames[app->playerCursor];
	}
	if (size) *size = sizeof(app->save.custom.name);
	return app->save.custom.name;
}

static void soccerDrawKeyboard(struct SoccerApp *app)
{
	char *text = soccerKeyboardText(app, NULL);
	soccerPanel(app, app->keyboardTarget ? "EDIT PLAYER NAME" : "EDIT TEAM NAME");
	dcAppDrawFill(&app->draw, 24, 42, 272, 28, soccerRgb(8, 50, 62));
	soccerCentered(app, 50, text, FontMedium, 0xffff);
	for (uint8_t i = 0; i < sizeof(mKeyboard) - 1u; i++) {
		int x = 22 + (i % 10u) * 28;
		int y = 91 + (i / 10u) * 27;
		if (app->keyboardCursor == i)
			dcAppDrawFill(&app->draw, x - 7, y - 5, 22, 20, soccerRgb(22, 115, 110));
		char key[2] = {mKeyboard[i], 0};
		soccerText(app, x, y, key, FontMedium, 0xffff);
	}
	soccerCentered(app, 207, "A ADD   B DELETE   START DONE", FontSmall, 0xffff);
}

static void soccerDrawTactics(struct SoccerApp *app)
{
	static const char *const names[] = {"4-4-2", "4-3-3", "3-5-2"};

	soccerPanel(app, "TACTICS / FORMATION");
	for (uint8_t i = 0; i < 3; i++)
		soccerMenuItem(app, 58 + i * 36, names[i], app->save.formation == i);
	soccerCentered(app, 183, "UP/DOWN CHOOSE   A SAVE   B BACK", FontSmall, 0xffff);
}

static void soccerDrawTransfer(struct SoccerApp *app)
{
	soccerPanel(app, "IMPORT / EXPORT");
	soccerMenuItem(app, 62, "IMPORT SWOS TEAM.000", app->transferSelection == 0);
	soccerMenuItem(app, 98, "EXPORT SWOS TEAM.000", app->transferSelection == 1);
	soccerMenuItem(app, 134, "BACK", app->transferSelection == 2);
	soccerCentered(app, 188, "FILES: /SAVE/PORTS/SOCCER", FontSmall, soccerRgb(175, 215, 210));
}

static void soccerDrawCredits(struct SoccerApp *app)
{
	soccerPanel(app, "CREDITS / LICENSE");
	soccerCentered(app, 45, "BASED ON YSOCCER 19", FontMedium, 0xffff);
	soccerCentered(app, 69, "MASSIMO MODICA AND CONTRIBUTORS", FontSmall, soccerRgb(180, 220, 210));
	soccerCentered(app, 95, "NATIVE DC32 ADAPTATION", FontMedium, 0xffff);
	soccerCentered(app, 119, "GNU GPL VERSION 2", FontSmall, soccerRgb(255, 225, 90));
	soccerCentered(app, 151, "NO ORIGINAL SWOS DATA INCLUDED", FontSmall, soccerRgb(180, 220, 210));
	soccerCentered(app, 196, "B BACK", FontSmall, 0xffff);
}

static void soccerRecordReplay(struct SoccerApp *app)
{
	struct SoccerReplayFrame *frame;

	if (!app->replay || (app->draw.frame & 1u))
		return;
	frame = &app->replay[app->replayWrite];
	for (uint8_t i = 0; i < SOCCER_PLAYER_COUNT; i++) {
		frame->x[i] = (int16_t)app->match.players[i].x;
		frame->y[i] = (int16_t)app->match.players[i].y;
	}
	frame->x[22] = (int16_t)app->match.ball.x;
	frame->y[22] = (int16_t)app->match.ball.y;
	frame->z = (uint8_t)(app->match.ball.z < 0 ? 0 : (app->match.ball.z > 255 ? 255 : app->match.ball.z));
	frame->score0 = app->match.score[0];
	frame->score1 = app->match.score[1];
	frame->period = app->match.period;
	app->replayWrite = (app->replayWrite + 1u) % SOCCER_REPLAY_FRAMES;
	if (app->replayCount < SOCCER_REPLAY_FRAMES)
		app->replayCount++;
}

static void soccerDrawReplay(struct SoccerApp *app)
{
	struct SoccerReplayFrame *frame;
	uint32_t oldest;
	char score[24];

	if (!app->replayCount) {
		soccerPanel(app, "ACTION REPLAY");
		soccerCentered(app, 102, "NO REPLAY RECORDED", FontMedium, 0xffff);
		return;
	}
	oldest = (app->replayWrite + SOCCER_REPLAY_FRAMES - app->replayCount) % SOCCER_REPLAY_FRAMES;
	frame = &app->replay[(oldest + app->replayRead) % SOCCER_REPLAY_FRAMES];
	app->cameraX = (int16_t)(frame->x[22] - 160);
	if (app->cameraX < -96) app->cameraX = -96;
	if (app->cameraX > (int32_t)SOCCER_PITCH_W - SOCCER_SCREEN_W + 96)
		app->cameraX = (int16_t)(SOCCER_PITCH_W - SOCCER_SCREEN_W + 96);
	app->cameraY = (int16_t)(frame->y[22] - 110);
	if (app->cameraY < -96) app->cameraY = -96;
	if (app->cameraY > (int32_t)SOCCER_PITCH_H - 220 + 96)
		app->cameraY = (int16_t)(SOCCER_PITCH_H - 220 + 96);
	soccerDrawPitch(app, app->cameraX, app->cameraY);
	soccerDrawBallShadowAt(app, frame->x[22], frame->y[22], frame->z,
		app->cameraX, app->cameraY);
	if (frame->z < 8)
		soccerDrawBallAt(app, frame->x[22], frame->y[22], frame->z,
			app->cameraX, app->cameraY);
	for (uint8_t i = 0; i < SOCCER_PLAYER_COUNT; i++) {
		struct SoccerPlayer player = app->match.players[i];
		player.x = frame->x[i];
		player.y = frame->y[i];
		soccerDrawPlayer(app, &player, i, app->cameraX, app->cameraY, false);
	}
	if (frame->z >= 8)
		soccerDrawBallAt(app, frame->x[22], frame->y[22], frame->z,
			app->cameraX, app->cameraY);
	dcAppDrawFill(&app->draw, 0, 0, 320, 20, soccerRgb(2, 18, 24));
	snprintf(score, sizeof(score), "REPLAY   %u - %u", frame->score0, frame->score1);
	soccerCentered(app, 5, score, FontSmall, 0xffff);
	app->replayRead = (app->replayRead + 1u) % app->replayCount;
}

static void soccerDraw(struct SoccerApp *app)
{
	switch (app->screen) {
	case SoccerScreenTitle: soccerDrawTitle(app); break;
	case SoccerScreenMain: soccerDrawMain(app); break;
	case SoccerScreenTeam: soccerDrawTeam(app); break;
	case SoccerScreenCompetition: soccerDrawCompetition(app); break;
	case SoccerScreenMatch: soccerDrawMatch(app); break;
	case SoccerScreenPause: soccerDrawPause(app); break;
	case SoccerScreenResult: soccerDrawResult(app); break;
	case SoccerScreenReplay: soccerDrawReplay(app); break;
	case SoccerScreenOptions: soccerDrawOptions(app); break;
	case SoccerScreenEditor: soccerDrawEditor(app); break;
	case SoccerScreenPlayers: soccerDrawPlayers(app); break;
	case SoccerScreenKeyboard: soccerDrawKeyboard(app); break;
	case SoccerScreenTactics: soccerDrawTactics(app); break;
	case SoccerScreenTransfer: soccerDrawTransfer(app); break;
	case SoccerScreenCredits: soccerDrawCredits(app); break;
	default: soccerDrawMain(app); break;
	}
}

static void soccerCompetitionInit(struct SoccerApp *app, uint8_t type)
{
	memset(app->save.table, 0, sizeof(app->save.table));
	app->save.competitionType = type;
	app->save.competitionRound = 0;
	app->save.competitionActive = 1;
	app->save.competitionWon = 0;
	for (uint8_t i = 0; i < 8; i++) {
		app->save.competitionTeams[i] = i ?
			(app->save.homeTeam % app->packHeader.teamCount + i) % app->packHeader.teamCount :
			app->save.homeTeam;
		app->save.table[i].team = app->save.competitionTeams[i];
	}
	(void)soccerWriteSave(app);
}

static void soccerPrepareCompetitionMatch(struct SoccerApp *app)
{
	uint8_t opponent = 1u + app->save.competitionRound % 7u;

	app->save.awayTeam = app->save.competitionTeams[opponent];
	app->competitionMatch = true;
	soccerReadTeam(app, app->save.homeTeam, &app->home);
	soccerReadTeam(app, app->save.awayTeam, &app->away);
}

static bool soccerCacheFailureChoice(struct SoccerApp *app)
{
	dcAppDrawWaitRelease(&app->draw, KEY_BIT_A | KEY_BIT_B);
	while (!mSoccerAbort) {
		dcAppDrawClear(&app->draw, soccerRgb(2, 18, 24));
		soccerCentered(app, 55, "PRELOAD FAILED", FontLarge, soccerRgb(255, 120, 95));
		soccerCentered(app, 105, "A: STREAM FROM SD", FontMedium, 0xffff);
		soccerCentered(app, 137, "B: EXIT", FontMedium, 0xffff);
		soccerCentered(app, 188, "STREAMING MAY STUTTER", FontSmall,
			soccerRgb(255, 220, 100));
		if (!dcAppDrawFrame(&app->draw, 0))
			return false;
		if (app->draw.pressed & KEY_BIT_A)
			return true;
		if (app->draw.pressed & KEY_BIT_B)
			return false;
	}
	return false;
}

static void soccerStartMatch(struct SoccerApp *app, bool cpu, bool training)
{
	if (!soccerReadTeam(app, app->save.homeTeam, &app->home) ||
		!soccerReadTeam(app, app->save.awayTeam, &app->away)) {
		soccerSetStatus(app, "TEAM DATA READ ERROR");
		app->screen = SoccerScreenMain;
		return;
	}
	app->seed ^= app->home.seed + (app->away.seed << 1) + app->draw.frame;
	soccerCoreInit(&app->match, app->seed, app->save.matchMinutes, app->save.difficulty,
		app->save.weather, cpu, training, app->save.formation);
	if (!soccerReadRoster(app, app->save.homeTeam, app->rosters[0]) ||
		!soccerReadRoster(app, app->save.awayTeam, app->rosters[1])) {
		soccerSetStatus(app, "PLAYER DATA READ ERROR");
		app->screen = SoccerScreenMain;
		return;
	}
	for (uint8_t team = 0; team < 2u; team++)
		for (uint8_t i = 0; i < SOCCER_PLAYERS_PER_TEAM; i++) {
			struct SoccerPlayer *player = &app->match.players[team * SOCCER_PLAYERS_PER_TEAM + i];
			const struct SoccerPlayerRecord *profile = &app->rosters[team][i];

			player->number = profile->number;
			player->role = profile->role;
			player->passing = profile->passing;
			player->shooting = profile->shooting;
			player->heading = profile->heading;
			player->tackling = profile->tackling;
			player->control = profile->control;
			player->finishing = profile->finishing;
			player->speed = i == 0u ? 1.05f : 1.05f + profile->speed * 0.055f;
		}
	if (!soccerLoadMatchImages(app)) {
		soccerSetStatus(app, "YSOCCER GRAPHICS ARE CORRUPT");
		app->screen = SoccerScreenMain;
		return;
	}
	app->match.radar = app->save.radar;
	app->replayWrite = 0;
	app->replayCount = 0;
	app->replayRead = 0;
	app->logicAccum = 0;
	app->lastLogicTime = app->host && app->host->getTime ? app->host->getTime() : 0u;
	app->matchPrevKeys = app->draw.keys;
	app->extraTimePlayed = false;
	app->penaltyShootout = false;
	app->penaltyScore[0] = app->penaltyScore[1] = 0u;
	app->screen = SoccerScreenMatch;
	soccerSetStatus(app, "");
}

static uint32_t soccerPortRandom(struct SoccerApp *app)
{
	app->seed ^= app->seed << 13;
	app->seed ^= app->seed >> 17;
	app->seed ^= app->seed << 5;
	return app->seed;
}

static void soccerResolvePenaltyShootout(struct SoccerApp *app)
{
	app->penaltyShootout = true;
	app->penaltyScore[0] = app->penaltyScore[1] = 0u;
	for (uint8_t round = 0u; round < 5u; round++)
		for (uint8_t team = 0u; team < 2u; team++) {
			const struct SoccerPlayerRecord *kicker = &app->rosters[team][1u + round * 2u];
			const struct SoccerPlayerRecord *keeper = &app->rosters[1u - team][0];
			int32_t chanceValue = 62 + kicker->finishing * 3 - keeper->control * 2;
			uint8_t chance = (uint8_t)(chanceValue < 45 ? 45 :
				(chanceValue > 82 ? 82 : chanceValue));

			if (soccerPortRandom(app) % 100u < chance)
				app->penaltyScore[team]++;
		}
	for (uint8_t sudden = 0u; app->penaltyScore[0] == app->penaltyScore[1] &&
		sudden < 8u; sudden++)
		for (uint8_t team = 0u; team < 2u; team++) {
			const struct SoccerPlayerRecord *kicker = &app->rosters[team][1u + sudden % 10u];
			if (soccerPortRandom(app) % 100u < 58u + kicker->finishing * 3u)
				app->penaltyScore[team]++;
		}
	if (app->penaltyScore[0] == app->penaltyScore[1])
		app->penaltyScore[soccerPortRandom(app) & 1u]++;
}

static void soccerCompetitionResult(struct SoccerApp *app)
{
	uint8_t opponent = 1u + app->save.competitionRound % 7u;
	struct SoccerTableRow *home = &app->save.table[0];
	struct SoccerTableRow *away = &app->save.table[opponent];
	bool knockout = app->save.competitionType != 2;
	bool advanced = app->match.score[0] > app->match.score[1] ||
		(app->penaltyShootout && app->penaltyScore[0] > app->penaltyScore[1]);

	if (knockout && app->match.score[0] == app->match.score[1]) {
		if (!app->penaltyShootout)
			soccerResolvePenaltyShootout(app);
		advanced = app->penaltyScore[0] > app->penaltyScore[1];
		soccerSetStatus(app, advanced ? "WON ON PENALTIES" : "LOST ON PENALTIES");
	}

	soccerTableApply(home, away, app->match.score[0], app->match.score[1]);
	for (uint8_t i = 1; i < 7; i += 2) {
		uint8_t goalsA = (uint8_t)((app->seed >> (i * 2)) & 3u);
		uint8_t goalsB = (uint8_t)((app->seed >> (i * 2 + 3)) & 3u);
		soccerTableApply(&app->save.table[i], &app->save.table[i + 1], goalsA, goalsB);
	}
	if (knockout && !advanced)
		app->save.competitionActive = 0;
	app->save.competitionRound++;
	if ((app->save.competitionType == 1 && app->save.competitionRound >= 3u) ||
		(app->save.competitionType == 2 && app->save.competitionRound >= 7u) ||
		(app->save.competitionType == 3 && app->save.competitionRound >= 5u)) {
		app->save.competitionActive = 0;
		app->save.competitionWon = 1;
	}
	(void)soccerWriteSave(app);
}

static void soccerHandleTitle(struct SoccerApp *app)
{
	if (app->draw.pressed & (KEY_BIT_A | KEY_BIT_START)) {
		app->screen = SoccerScreenMain;
		app->menuSelection = 0;
		soccerSetStatus(app, "FULL OFFLINE EDITION");
	}
}

static void soccerMenuMove(struct SoccerApp *app, int direction, uint8_t count)
{
	int value = app->subSelection + direction;
	if (value < 0) value = count - 1;
	if (value >= count) value = 0;
	app->subSelection = (uint8_t)value;
}

static void soccerHandleMain(struct SoccerApp *app)
{
	if (app->draw.pressed & KEY_BIT_UP) {
		app->menuSelection = app->menuSelection ? app->menuSelection - 1u : SOCCER_MENU_ITEMS - 1u;
	}
	if (app->draw.pressed & KEY_BIT_DOWN)
		app->menuSelection = (app->menuSelection + 1u) % SOCCER_MENU_ITEMS;
	if (app->menuSelection < app->visibleMenuOffset)
		app->visibleMenuOffset = app->menuSelection;
	if (app->menuSelection >= app->visibleMenuOffset + 9u)
		app->visibleMenuOffset = app->menuSelection - 8u;
	if (app->draw.pressed & KEY_BIT_B) {
		app->screen = SoccerScreenTitle;
		return;
	}
	if (!(app->draw.pressed & (KEY_BIT_A | KEY_BIT_START)))
		return;
	app->selectingAway = false;
	app->teamCursor = app->save.homeTeam;
	switch (app->menuSelection) {
	case 0: app->purpose = SoccerPurposeFriendly; app->screen = SoccerScreenTeam; break;
	case 1:
		if (app->save.competitionActive && app->save.competitionType == 1) app->screen = SoccerScreenCompetition;
		else { app->purpose = SoccerPurposeCup; app->screen = SoccerScreenTeam; }
		break;
	case 2:
		if (app->save.competitionActive && app->save.competitionType == 2) app->screen = SoccerScreenCompetition;
		else { app->purpose = SoccerPurposeLeague; app->screen = SoccerScreenTeam; }
		break;
	case 3:
		if (app->save.competitionActive && app->save.competitionType == 3) app->screen = SoccerScreenCompetition;
		else { app->purpose = SoccerPurposeTournament; app->screen = SoccerScreenTeam; }
		break;
	case 4: app->purpose = SoccerPurposeTraining; app->screen = SoccerScreenTeam; break;
	case 5: app->purpose = SoccerPurposeCpu; app->screen = SoccerScreenTeam; break;
	case 6: app->purpose = SoccerPurposeEditor; app->screen = SoccerScreenTeam; break;
	case 7: app->screen = SoccerScreenTactics; break;
	case 8:
		app->previousScreen = SoccerScreenMain;
		app->replayRead = 0;
		app->screen = SoccerScreenReplay;
		break;
	case 9: app->subSelection = 0; app->screen = SoccerScreenOptions; break;
	case 10: app->transferSelection = 0; app->screen = SoccerScreenTransfer; break;
	case 11: app->screen = SoccerScreenCredits; break;
	case 12: mSoccerAbort = true; break;
	}
}

static void soccerHandleTeam(struct SoccerApp *app)
{
	int32_t cursor = app->teamCursor;
	if (app->draw.pressed & KEY_BIT_UP) cursor--;
	if (app->draw.pressed & KEY_BIT_DOWN) cursor++;
	if (app->draw.pressed & KEY_BIT_LEFT) cursor -= 25;
	if (app->draw.pressed & KEY_BIT_RIGHT) cursor += 25;
	while (cursor < 0) cursor += app->packHeader.teamCount + 1u;
	while (cursor > (int32_t)app->packHeader.teamCount) cursor -= app->packHeader.teamCount + 1u;
	app->teamCursor = (uint16_t)cursor;
	if (app->draw.pressed & KEY_BIT_B) {
		if (app->selectingAway) {
			app->selectingAway = false;
			app->teamCursor = app->save.homeTeam;
		}
		else app->screen = SoccerScreenMain;
		return;
	}
	if (!(app->draw.pressed & (KEY_BIT_A | KEY_BIT_START)))
		return;
	if (app->purpose == SoccerPurposeEditor) {
		if (app->teamCursor != app->packHeader.teamCount) {
			struct SoccerTeamRecord source;
			if (soccerReadTeam(app, app->teamCursor, &source)) {
				snprintf(app->save.custom.name, sizeof(app->save.custom.name), "%.23s", source.name);
				app->save.custom.shirt1 = soccerRgb565To332(source.shirt1);
				app->save.custom.shirt2 = soccerRgb565To332(source.shirt2);
				app->save.custom.shorts = soccerRgb565To332(source.shorts);
			}
		}
		app->save.homeTeam = app->packHeader.teamCount;
		app->subSelection = 0;
		app->screen = SoccerScreenEditor;
		return;
	}
	if (!app->selectingAway) {
		app->save.homeTeam = app->teamCursor;
		if (app->purpose == SoccerPurposeCup || app->purpose == SoccerPurposeLeague ||
			app->purpose == SoccerPurposeTournament) {
			soccerCompetitionInit(app, app->purpose == SoccerPurposeCup ? 1u :
				(app->purpose == SoccerPurposeLeague ? 2u : 3u));
			app->screen = SoccerScreenCompetition;
			return;
		}
		if (app->purpose == SoccerPurposeTraining) {
			app->save.awayTeam = (app->teamCursor + 1u) % app->packHeader.teamCount;
			app->competitionMatch = false;
			soccerStartMatch(app, false, true);
			return;
		}
		app->selectingAway = true;
		app->teamCursor = app->save.awayTeam;
	}
	else {
		app->save.awayTeam = app->teamCursor;
		app->competitionMatch = false;
		(void)soccerWriteSave(app);
		soccerStartMatch(app, app->purpose == SoccerPurposeCpu, false);
	}
}

static void soccerHandleCompetition(struct SoccerApp *app)
{
	if (!app->save.competitionActive) {
		if (app->draw.pressed & (KEY_BIT_A | KEY_BIT_B | KEY_BIT_START))
			app->screen = SoccerScreenMain;
		return;
	}
	if (app->draw.pressed & KEY_BIT_B) {
		app->save.competitionActive = 0;
		(void)soccerWriteSave(app);
		app->screen = SoccerScreenMain;
	}
	else if (app->draw.pressed & (KEY_BIT_A | KEY_BIT_START)) {
		soccerPrepareCompetitionMatch(app);
		soccerStartMatch(app, false, false);
	}
}

static struct SoccerInput soccerReadMatchInput(struct SoccerApp *app)
{
	struct SoccerInput input = {0};
	uint_fast16_t released = app->matchPrevKeys & ~app->draw.keys;

	input.x = (app->draw.keys & KEY_BIT_LEFT) ? -1 : ((app->draw.keys & KEY_BIT_RIGHT) ? 1 : 0);
	input.y = (app->draw.keys & KEY_BIT_UP) ? -1 : ((app->draw.keys & KEY_BIT_DOWN) ? 1 : 0);
	input.fire = (app->draw.keys & KEY_BIT_A) != 0;
	input.firePressed = (app->draw.pressed & KEY_BIT_A) != 0;
	input.fireReleased = (released & KEY_BIT_A) != 0;
	input.switchPressed = (app->draw.pressed & KEY_BIT_B) != 0;
	app->matchPrevKeys = app->draw.keys;
	return input;
}

static void soccerHandleMatch(struct SoccerApp *app)
{
	struct SoccerInput input = soccerReadMatchInput(app);
	uint64_t nominalElapsed = TICKS_PER_SECOND / SOCCER_RENDER_HZ;
	uint64_t now = app->host && app->host->getTime ? app->host->getTime() : 0u;
	uint64_t elapsed = nominalElapsed;

	if (app->draw.pressed & KEY_BIT_START) {
		app->subSelection = 0;
		app->screen = SoccerScreenPause;
		app->lastLogicTime = 0u;
		return;
	}
	if (app->draw.pressed & KEY_BIT_SEL) {
		app->subSelection = 0;
		app->screen = SoccerScreenPause;
		app->lastLogicTime = 0u;
		return;
	}
	if (now && app->lastLogicTime) {
		elapsed = now - app->lastLogicTime;
		if (elapsed > TICKS_PER_SECOND / 4u)
			elapsed = nominalElapsed;
	}
	app->lastLogicTime = now;
	app->logicAccum += elapsed * SOCCER_LOGIC_HZ;
	while (app->logicAccum >= TICKS_PER_SECOND) {
		soccerCoreTick(&app->match, &input);
		app->logicAccum -= TICKS_PER_SECOND;
		input.firePressed = false;
		input.fireReleased = false;
		input.switchPressed = false;
	}
	soccerRecordReplay(app);
	if (app->match.finished && !app->match.bannerTicks) {
		if (app->competitionMatch && app->save.competitionType != 2 &&
			app->match.score[0] == app->match.score[1] && !app->extraTimePlayed) {
			app->extraTimePlayed = true;
			soccerCoreStartExtraTime(&app->match);
			return;
		}
		if (app->competitionMatch && app->save.competitionType != 2 &&
			app->match.score[0] == app->match.score[1] && app->extraTimePlayed &&
			!app->penaltyShootout)
			soccerResolvePenaltyShootout(app);
		if (app->competitionMatch)
			soccerCompetitionResult(app);
		(void)soccerWriteReplay(app, SOCCER_REPLAY_PATH);
		app->screen = SoccerScreenResult;
	}
}

static void soccerHandlePause(struct SoccerApp *app)
{
	if (app->draw.pressed & KEY_BIT_UP) soccerMenuMove(app, -1, 6);
	if (app->draw.pressed & KEY_BIT_DOWN) soccerMenuMove(app, 1, 6);
	if (app->draw.pressed & (KEY_BIT_B | KEY_BIT_START)) {
		app->screen = SoccerScreenMatch;
		return;
	}
	if (!(app->draw.pressed & KEY_BIT_A))
		return;
	switch (app->subSelection) {
	case 0: app->screen = SoccerScreenMatch; break;
	case 1:
		app->match.radar = !app->match.radar;
		app->save.radar = app->match.radar;
		(void)soccerWriteSave(app);
		break;
	case 2:
		app->save.formation = (app->save.formation + 1u) % 3u;
		soccerCoreMoveToFormation(&app->match, 0, app->save.formation);
		if (app->match.substitutions[0] < 3u && app->match.controlled >= 0) {
			struct SoccerPlayer *player = &app->match.players[app->match.controlled];
			app->match.substitutions[0]++;
			app->match.clockTicks += SOCCER_LOGIC_HZ * 10u;
			player->speed += 0.04f;
			player->yellowCards = 0;
			soccerSetStatus(app, "FORMATION CHANGED - SUBSTITUTE ON");
		}
		else soccerSetStatus(app, "FORMATION CHANGED - NO SUBS LEFT");
		(void)soccerWriteSave(app);
		app->screen = SoccerScreenMatch;
		break;
	case 3:
		app->previousScreen = SoccerScreenPause;
		app->replayRead = 0;
		app->screen = SoccerScreenReplay;
		break;
	case 4:
		soccerSetStatus(app, soccerWriteReplay(app, SOCCER_HIGHLIGHT_PATH) ?
			"HIGHLIGHT SAVED" : "HIGHLIGHT SAVE FAILED");
		app->screen = SoccerScreenMatch;
		break;
	case 5:
		app->match.finished = true;
		app->match.bannerTicks = 0;
		if (app->competitionMatch) {
			app->match.score[1]++;
			soccerCompetitionResult(app);
		}
		app->screen = SoccerScreenResult;
		break;
	}
}

static void soccerHandleResult(struct SoccerApp *app)
{
	if (app->draw.pressed & KEY_BIT_SEL) {
		app->previousScreen = SoccerScreenResult;
		app->replayRead = 0;
		app->screen = SoccerScreenReplay;
	}
	else if (app->draw.pressed & (KEY_BIT_A | KEY_BIT_START | KEY_BIT_B))
		app->screen = app->competitionMatch ?
			SoccerScreenCompetition : SoccerScreenMain;
}

static void soccerHandleReplay(struct SoccerApp *app)
{
	if (app->draw.pressed & (KEY_BIT_A | KEY_BIT_B | KEY_BIT_START | KEY_BIT_SEL))
		app->screen = app->previousScreen;
}

static void soccerOptionChange(struct SoccerApp *app, int direction)
{
	switch (app->subSelection) {
	case 0: app->save.difficulty = (app->save.difficulty + 3u + direction) % 3u; break;
	case 1:
		if (direction > 0) app->save.matchMinutes = app->save.matchMinutes >= 5 ? 1 : app->save.matchMinutes + 2;
		else app->save.matchMinutes = app->save.matchMinutes <= 1 ? 5 : app->save.matchMinutes - 2;
		break;
	case 2: app->save.weather = (app->save.weather + SoccerWeatherCount + direction) % SoccerWeatherCount; break;
	case 3: app->save.radar = !app->save.radar; break;
	case 4: app->save.language = (app->save.language + 9u + direction) % 9u; break;
	}
}

static void soccerHandleOptions(struct SoccerApp *app)
{
	if (app->draw.pressed & KEY_BIT_UP) soccerMenuMove(app, -1, 5);
	if (app->draw.pressed & KEY_BIT_DOWN) soccerMenuMove(app, 1, 5);
	if (app->draw.pressed & KEY_BIT_LEFT) soccerOptionChange(app, -1);
	if (app->draw.pressed & (KEY_BIT_RIGHT | KEY_BIT_A)) soccerOptionChange(app, 1);
	if (app->draw.pressed & (KEY_BIT_B | KEY_BIT_START)) {
		(void)soccerWriteSave(app);
		app->screen = SoccerScreenMain;
	}
}

static void soccerHandleEditor(struct SoccerApp *app)
{
	if (app->draw.pressed & KEY_BIT_UP) soccerMenuMove(app, -1, 8);
	if (app->draw.pressed & KEY_BIT_DOWN) soccerMenuMove(app, 1, 8);
	if (app->draw.pressed & KEY_BIT_LEFT) {
		if (app->subSelection == 2) app->save.custom.shirt1--;
		if (app->subSelection == 3) app->save.custom.shirt2--;
		if (app->subSelection == 4) app->save.custom.shorts--;
		if (app->subSelection == 5 && app->save.custom.rating > 1) app->save.custom.rating--;
	}
	if (app->draw.pressed & KEY_BIT_RIGHT) {
		if (app->subSelection == 2) app->save.custom.shirt1++;
		if (app->subSelection == 3) app->save.custom.shirt2++;
		if (app->subSelection == 4) app->save.custom.shorts++;
		if (app->subSelection == 5 && app->save.custom.rating < 9) app->save.custom.rating++;
	}
	if (app->draw.pressed & KEY_BIT_A) {
		if (app->subSelection == 0) {
			app->keyboardTarget = 0;
			app->keyboardLength = (uint8_t)strlen(app->save.custom.name);
			app->keyboardCursor = 0;
			app->screen = SoccerScreenKeyboard;
		}
		else if (app->subSelection == 1) {
			app->playerCursor = 0;
			app->screen = SoccerScreenPlayers;
		}
		else if (app->subSelection == 6) {
			soccerSetStatus(app, soccerTransferTeam(app, false) ? "TEAM EXPORTED" : "EXPORT FAILED");
		}
		else if (app->subSelection == 7) {
			(void)soccerWriteSave(app);
			app->screen = SoccerScreenMain;
		}
	}
	if (app->draw.pressed & KEY_BIT_B) {
		(void)soccerWriteSave(app);
		app->screen = SoccerScreenMain;
	}
}

static void soccerHandlePlayers(struct SoccerApp *app)
{
	if (app->draw.pressed & KEY_BIT_UP)
		app->playerCursor = app->playerCursor ? app->playerCursor - 1u : 15u;
	if (app->draw.pressed & KEY_BIT_DOWN)
		app->playerCursor = (app->playerCursor + 1u) % 16u;
	if ((app->draw.pressed & KEY_BIT_LEFT) && app->save.custom.playerRatings[app->playerCursor] > 1u)
		app->save.custom.playerRatings[app->playerCursor]--;
	if ((app->draw.pressed & KEY_BIT_RIGHT) && app->save.custom.playerRatings[app->playerCursor] < 9u)
		app->save.custom.playerRatings[app->playerCursor]++;
	if (app->draw.pressed & KEY_BIT_A) {
		app->keyboardTarget = 1;
		app->keyboardLength = (uint8_t)strlen(app->save.custom.playerNames[app->playerCursor]);
		app->keyboardCursor = 0;
		app->screen = SoccerScreenKeyboard;
	}
	if (app->draw.pressed & (KEY_BIT_B | KEY_BIT_START)) {
		(void)soccerWriteSave(app);
		app->screen = SoccerScreenEditor;
	}
}

static void soccerHandleKeyboard(struct SoccerApp *app)
{
	uint8_t count = sizeof(mKeyboard) - 1u;
	uint32_t textSize;
	char *text = soccerKeyboardText(app, &textSize);
	if (app->draw.pressed & KEY_BIT_LEFT) app->keyboardCursor = app->keyboardCursor ? app->keyboardCursor - 1u : count - 1u;
	if (app->draw.pressed & KEY_BIT_RIGHT) app->keyboardCursor = (app->keyboardCursor + 1u) % count;
	if (app->draw.pressed & KEY_BIT_UP) app->keyboardCursor = app->keyboardCursor >= 10u ? app->keyboardCursor - 10u : app->keyboardCursor;
	if (app->draw.pressed & KEY_BIT_DOWN) app->keyboardCursor = app->keyboardCursor + 10u < count ? app->keyboardCursor + 10u : app->keyboardCursor;
	if (app->touchPressed && app->touchY >= 82 && app->touchY < 202) {
		uint8_t col = (uint8_t)((app->touchX - 15) / 28);
		uint8_t row = (uint8_t)((app->touchY - 82) / 27);
		uint8_t key = row * 10u + col;
		if (key < count) {
			app->keyboardCursor = key;
			app->draw.pressed |= KEY_BIT_A;
		}
	}
	if ((app->draw.pressed & KEY_BIT_A) && app->keyboardLength + 1u < textSize) {
		text[app->keyboardLength++] = mKeyboard[app->keyboardCursor];
		text[app->keyboardLength] = 0;
	}
	if ((app->draw.pressed & KEY_BIT_B) && app->keyboardLength) {
		text[--app->keyboardLength] = 0;
	}
	if (app->draw.pressed & (KEY_BIT_START | KEY_BIT_SEL)) {
		if (!app->keyboardLength)
			snprintf(text, textSize, "%s", app->keyboardTarget ? "PLAYER" : "DC32 UNITED");
		app->screen = app->keyboardTarget ? SoccerScreenPlayers : SoccerScreenEditor;
	}
}

static void soccerHandleTactics(struct SoccerApp *app)
{
	if (app->draw.pressed & KEY_BIT_UP) app->save.formation = (app->save.formation + 2u) % 3u;
	if (app->draw.pressed & KEY_BIT_DOWN) app->save.formation = (app->save.formation + 1u) % 3u;
	if (app->draw.pressed & (KEY_BIT_A | KEY_BIT_START)) {
		(void)soccerWriteSave(app);
		app->screen = SoccerScreenMain;
	}
	if (app->draw.pressed & KEY_BIT_B)
		app->screen = SoccerScreenMain;
}

static void soccerHandleTransfer(struct SoccerApp *app)
{
	if (app->draw.pressed & KEY_BIT_UP) app->transferSelection = app->transferSelection ? app->transferSelection - 1u : 2u;
	if (app->draw.pressed & KEY_BIT_DOWN) app->transferSelection = (app->transferSelection + 1u) % 3u;
	if (app->draw.pressed & KEY_BIT_A) {
		if (app->transferSelection == 0)
			soccerSetStatus(app, soccerTransferTeam(app, true) ? "TEAM IMPORTED" : "IMPORT FAILED OR INVALID");
		else if (app->transferSelection == 1)
			soccerSetStatus(app, soccerTransferTeam(app, false) ? "TEAM EXPORTED" : "EXPORT FAILED");
		else app->screen = SoccerScreenMain;
	}
	if (app->draw.pressed & KEY_BIT_B)
		app->screen = SoccerScreenMain;
}

static void soccerHandleInput(struct SoccerApp *app)
{
	if ((app->draw.pressed & UI_KEY_BIT_CENTER) && app->host && app->host->portMenu) {
		if (!app->host->portMenu(&app->draw.displayCnv))
			mSoccerAbort = true;
		app->draw.prevKeys = app->host->uiKeysRaw ? app->host->uiKeysRaw() : 0;
		return;
	}
	switch (app->screen) {
	case SoccerScreenTitle: soccerHandleTitle(app); break;
	case SoccerScreenMain: soccerHandleMain(app); break;
	case SoccerScreenTeam: soccerHandleTeam(app); break;
	case SoccerScreenCompetition: soccerHandleCompetition(app); break;
	case SoccerScreenMatch: soccerHandleMatch(app); break;
	case SoccerScreenPause: soccerHandlePause(app); break;
	case SoccerScreenResult: soccerHandleResult(app); break;
	case SoccerScreenReplay: soccerHandleReplay(app); break;
	case SoccerScreenOptions: soccerHandleOptions(app); break;
	case SoccerScreenEditor: soccerHandleEditor(app); break;
	case SoccerScreenPlayers: soccerHandlePlayers(app); break;
	case SoccerScreenKeyboard: soccerHandleKeyboard(app); break;
	case SoccerScreenTactics: soccerHandleTactics(app); break;
	case SoccerScreenTransfer: soccerHandleTransfer(app); break;
	case SoccerScreenCredits:
		if (app->draw.pressed & (KEY_BIT_A | KEY_BIT_B | KEY_BIT_START)) app->screen = SoccerScreenMain;
		break;
	default: break;
	}
}

int soccerAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct SoccerApp *app;
	uint8_t *framebuffer;
	enum SoccerCacheResult cacheResult;

	mSoccerAbort = false;
	audioPwmStop();
	dc32PortHeapInitDefault();
	app = dc32PortCalloc(1, sizeof(*app));
	if (!app)
		return -1;
	if (!soccerCoreSelfTest()) {
		if (host && host->log)
			host->log("YSoccer core self-test failed; continuing with runtime checks\n");
	}
	app->replay = dc32PortCalloc(SOCCER_REPLAY_FRAMES, sizeof(*app->replay));
	if (!app->replay)
		return -1;
	framebuffer = dc32PortMalloc(SOCCER_FRAMEBUFFER_BYTES);
	if (!framebuffer)
		return -1;
	app->host = host;
	app->args = args;
	app->seed = 0x19dc3225u;
	if (!dcAppDrawInit(&app->draw, host, args, framebuffer,
		SOCCER_SCREEN_W, SOCCER_SCREEN_H))
		return -1;
	{
		const struct DcAppLoadingState loading = {
			.appName = "Sensible Soccer",
			.title = "Preparing game data",
			.detail = "Opening asset pack",
			.animationStep = app->draw.frame,
		};

		dcAppDrawLoading(&app->draw, &loading);
		(void)dcAppDrawFrame(&app->draw, 0);
	}
	if (!soccerOpenPack(app)) {
		dc32PortShowMissingData(host, args, "Sensible Soccer data", SOCCER_PACK_PATH,
			framebuffer);
		return 0;
	}
	soccerLoadSave(app);
	dispSetFramerate(SOCCER_RENDER_HZ);
	cacheResult = soccerPrepareUniversalCache(app);
	if (cacheResult == SoccerCacheCancelled) {
		app->streamingFallback = true;
		soccerSetStatus(app, "PRELOAD SKIPPED; STREAMING FROM SD");
	}
	else if (cacheResult == SoccerCacheFailed) {
		if (!soccerCacheFailureChoice(app)) {
			dc32PortCloseAssetPack(&app->pak);
			dispSetFramerate(60);
			return 0;
		}
		app->streamingFallback = true;
		soccerSetStatus(app, "CACHE FAILED; STREAMING FROM SD");
	}
	(void)soccerLoadReplay(app);
	soccerReadTeam(app, app->save.homeTeam, &app->home);
	soccerReadTeam(app, app->save.awayTeam, &app->away);
	app->screen = SoccerScreenTitle;
	dcAppDrawWaitRelease(&app->draw, KEY_BIT_A | KEY_BIT_B | KEY_BIT_START | KEY_BIT_SEL | UI_KEY_BIT_CENTER);
	while (!mSoccerAbort) {
		soccerPollTouch(app);
		soccerDraw(app);
		if (!dcAppDrawFrame(&app->draw, 0))
			break;
		soccerHandleInput(app);
		if (host && host->ledsTick)
			host->ledsTick();
	}
	(void)soccerWriteSave(app);
	dc32PortCloseAssetPack(&app->pak);
	audioPwmStop();
	dispSetFramerate(60);
	dcAppDrawWaitRelease(&app->draw, UI_KEY_BIT_CENTER);
	return 0;
}

void soccerAppAbort(void)
{
	mSoccerAbort = true;
}
