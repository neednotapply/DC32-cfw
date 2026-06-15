#include "apps/cave/cave_port.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cave_assets.h"
#include "apps/port/port_runtime.h"
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "ui.h"

#ifndef DCAPP_RUNTIME_ID
#error "DCAPP_RUNTIME_ID must be provided by the app target"
#endif

const struct DcAppImageHeader dcAppImageHeader __attribute__((section(".dcapp_header"), used, aligned(256))) = {
	.magic = DCAPP_MAGIC,
	.headerSize = DCAPP_HEADER_SIZE,
	.abiVersion = DCAPP_ABI_VERSION,
	.runtime = DCAPP_RUNTIME_ID,
	.flags = DCAPP_IMAGE_FLAG_LARGE_XIP,
	.loadAddr = 0x10100000u,
	.appRamStart = 0x2005F000u,
	.appRamSize = 0x00014000u,
};

#define CAVE_SCREEN_W DC32_PORT_SCREEN_W
#define CAVE_SCREEN_H DC32_PORT_SCREEN_H
#define CAVE_PACK_PATH "/APPS/cave.pak"
#define CAVE_PACK_MAGIC "DC32CAVEPAK"
#define CAVE_PACK_HEADER_SIZE 20u
#define CAVE_PACK_ENTRY_SIZE 16u
#define CAVE_MAX_ENTRIES 384u
#define CAVE_MAX_PATH 96u
#define CAVE_TILE 16
#define CAVE_FIX_SHIFT 8
#define CAVE_FIX_SCALE (1 << CAVE_FIX_SHIFT)
#define CAVE_SAVE_MAGIC 0x45564143u
#define CAVE_SAVE_VERSION 3u
#define CAVE_STAGE_START_POINT 13u
#define CAVE_STAGE_START_EVENT 91u
#define CAVE_NUM_GAME_FLAGS 8000u
#define CAVE_FLAG_BYTES ((CAVE_NUM_GAME_FLAGS + 7u) / 8u)
#define CAVE_MAP_FLAG_COUNT 128u
#define CAVE_MAP_FLAG_BYTES ((CAVE_MAP_FLAG_COUNT + 7u) / 8u)
#define CAVE_ITEM_COUNT 256u
#define CAVE_ITEM_BYTES ((CAVE_ITEM_COUNT + 7u) / 8u)
#define CAVE_WEAPON_COUNT 32u
#define CAVE_WEAPON_BYTES ((CAVE_WEAPON_COUNT + 7u) / 8u)
#define CAVE_MAX_ENTITIES 256u
#define CAVE_MAX_BULLETS 8u
#define CAVE_NPC_TBL_ENTRY_SIZE 0x18u
#define CAVE_TSC_TEXT_MAX 224u
#define CAVE_TA_SOLID_PLAYER 0x00001u
#define CAVE_TA_SOLID_NPC 0x00002u
#define CAVE_TA_SOLID_SHOT 0x00004u
#define CAVE_TA_SOLID (CAVE_TA_SOLID_PLAYER | CAVE_TA_SOLID_NPC | CAVE_TA_SOLID_SHOT)
#define CAVE_FLAG_INVULNERABLE 0x0004u
#define CAVE_FLAG_SHOOTABLE 0x0020u
#define CAVE_FLAG_SCRIPT_ON_TOUCH 0x0100u
#define CAVE_FLAG_SCRIPT_ON_DEATH 0x0200u
#define CAVE_FLAG_APPEAR_ON_FLAGID 0x0800u
#define CAVE_FLAG_FACES_RIGHT 0x1000u
#define CAVE_FLAG_SCRIPT_ON_ACTIVATE 0x2000u
#define CAVE_FLAG_DISAPPEAR_ON_FLAGID 0x4000u

struct CaveEntry {
	uint32_t offset;
	uint32_t size;
	uint32_t crc32;
	uint16_t nameOfs;
	uint16_t nameLen;
};

struct CaveBitmap {
	uint16_t w;
	uint16_t h;
	uint8_t *pixels;
};

struct CaveMap {
	uint16_t w;
	uint16_t h;
	uint8_t *tiles;
};

struct CaveEntity {
	int16_t x;
	int16_t y;
	uint16_t flagNum;
	uint16_t eventNum;
	uint16_t npcType;
	uint16_t flags;
	uint8_t layer;
	uint16_t hp;
	uint16_t actionNum;
	uint8_t tscDir;
	bool alive;
	bool removed;
};

struct CaveBullet {
	int32_t x;
	int32_t y;
	int32_t vx;
	int32_t vy;
	uint8_t ttl;
	uint8_t dir;
	bool alive;
};

struct CaveNpcInfo {
	uint16_t flags;
	uint16_t life;
	uint8_t sheetId;
	uint8_t deathSound;
	uint8_t hurtSound;
	uint8_t size;
	uint32_t experience;
	uint32_t damage;
	uint8_t hitLeft;
	uint8_t hitTop;
	uint8_t hitRight;
	uint8_t hitBottom;
	uint8_t dispLeft;
	uint8_t dispTop;
	uint8_t dispRight;
	uint8_t dispBottom;
};

struct CaveStageRecord {
	char filename[32];
	char stagename[35];
	uint8_t tileset;
	uint8_t bgNo;
	uint8_t scrollType;
	uint8_t bossNo;
	uint8_t npcSet1;
	uint8_t npcSet2;
};

struct CaveScriptVm {
	uint8_t *headScript;
	uint32_t headScriptSize;
	uint8_t *mapScript;
	uint32_t mapScriptSize;
	const uint8_t *activeScript;
	uint32_t activeScriptSize;
	uint32_t end;
	uint16_t event;
	uint32_t pos;
	uint16_t waitFrames;
	uint16_t faceId;
	uint16_t itemId;
	bool running;
	bool waitingForButton;
	bool messageOpen;
	bool messageTop;
	bool messageBorder;
	bool textFast;
	bool controlsLocked;
	char text[CAVE_TSC_TEXT_MAX];
};

struct CaveSave {
	uint32_t magic;
	uint16_t version;
	uint16_t size;
	uint16_t stage;
	int16_t playerX;
	int16_t playerY;
	uint16_t life;
	uint16_t maxLife;
	uint32_t equipMask;
	uint8_t flags[CAVE_FLAG_BYTES];
	uint8_t skipFlags[CAVE_FLAG_BYTES];
	uint8_t mapFlags[CAVE_MAP_FLAG_BYTES];
	uint8_t itemFlags[CAVE_ITEM_BYTES];
	uint8_t weaponFlags[CAVE_WEAPON_BYTES];
};

struct CaveGame {
	const struct DcAppHostApi *host;
	const struct DcAppRunArgs *args;
	struct DcAppDrawCtx draw;
	struct Dc32PortPak pak;
	struct CaveEntry *entries;
	char *entryNames;
	uint32_t entryCount;
	uint8_t *frame;
	uint8_t *stageDat;
	uint32_t stageDatSize;
	uint8_t stageCount;
	struct CaveStageRecord stage;
	struct CaveMap map;
	uint8_t tileCode[256];
	uint32_t tileAttr[256];
	uint32_t tileKey[256];
	bool tileAttrsLoaded;
	struct CaveEntity *entities;
	uint16_t entityCount;
	struct CaveNpcInfo *npcInfo;
	uint16_t npcInfoCount;
	struct CaveBitmap tileset;
	struct CaveBitmap backdrop;
	struct CaveBitmap myChar;
	struct CaveBitmap npc0;
	struct CaveBitmap npcSym;
	struct CaveBitmap npcRegu;
	struct CaveBitmap npcStage1;
	struct CaveBitmap npcStage2;
	struct CaveBitmap bulletSheet;
	struct CaveBitmap nxTempSheet;
	char nxTempSheetName[CAVE_MAX_PATH];
	struct CaveScriptVm script;
	uint16_t stageIndex;
	int16_t playerX;
	int16_t playerY;
	int32_t playerFx;
	int32_t playerFy;
	int32_t playerVx;
	int32_t playerVy;
	uint16_t playerAnimTick;
	uint16_t life;
	uint16_t maxLife;
	uint32_t equipMask;
	bool playerFacingRight;
	bool playerOnGround;
	bool playerAimUp;
	bool playerAimDown;
	bool playerHidden;
	uint8_t controlMode;
	uint16_t lastNumber;
	uint16_t lastSfx;
	uint16_t quakeFrames;
	uint8_t fadeFrames;
	uint8_t fadeMode;
	struct CaveBullet bullets[CAVE_MAX_BULLETS];
	uint8_t flags[CAVE_FLAG_BYTES];
	uint8_t skipFlags[CAVE_FLAG_BYTES];
	uint8_t mapFlags[CAVE_MAP_FLAG_BYTES];
	uint8_t itemFlags[CAVE_ITEM_BYTES];
	uint8_t weaponFlags[CAVE_WEAPON_BYTES];
	uint8_t bannerFrames;
	bool running;
	bool pakOpen;
	char banner[44];
};

static volatile bool mCaveAbort;
static uint8_t mCaveFrame[CAVE_SCREEN_W * CAVE_SCREEN_H];

static const char *const mCaveTilesets[] = {
	"0", "Pens", "Eggs", "EggX", "EggIn", "Store", "Weed", "Barr",
	"Maze", "Sand", "Mimi", "Cave", "River", "Gard", "Almond", "Oside",
	"Cent", "Jail", "White", "Fall", "Hell", "Labo",
};

static const char *const mCaveBackdrops[] = {
	"bk0", "bkBlue", "bkGreen", "bkBlack", "bkGard", "bkMaze", "bkGray", "bkRed",
	"bkWater", "bkMoon", "bkFog", "bkFall", "bkLight", "bkSunset", "bkHellish",
};

static const char *const mCaveNpcSets[] = {
	"guest", "0", "eggs1", "ravil", "weed", "maze", "sand", "omg",
	"cemet", "bllg", "plant", "frog", "curly", "stream", "ironh", "toro",
	"x", "dark", "almo1", "eggs2", "twind", "moon", "cent", "heri",
	"red", "miza", "dr", "almo2", "kings", "hell", "press", "priest",
	"ballos", "island",
};

static const char *const mCaveNpcSheetNames[] = {
	"Guest", "0", "Eggs1", "Ravil", "Weed", "Maze", "Sand", "Omg",
	"Cemet", "Bllg", "Plant", "Frog", "Curly", "Stream", "IronH", "Toro",
	"X", "Dark", "Almo1", "Eggs2", "TwinD", "Moon", "Cent", "Heri",
	"Red", "Miza", "Dr", "Almo2", "Kings", "Hell", "Press", "Priest",
	"Ballos", "Island",
};

static const uint32_t mCaveDefaultTileKey[256] = {
	0, CAVE_TA_SOLID, CAVE_TA_SOLID, CAVE_TA_SOLID, CAVE_TA_SOLID,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, CAVE_TA_SOLID, CAVE_TA_SOLID, 0, 0, CAVE_TA_SOLID, CAVE_TA_SOLID, CAVE_TA_SOLID,
	CAVE_TA_SOLID, 0, 0, 0, 0, 0, 0, 0,
	0, CAVE_TA_SOLID, 0, CAVE_TA_SOLID, CAVE_TA_SOLID, 0, 0, 0,
	0, 0, 0, CAVE_TA_SOLID, CAVE_TA_SOLID, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CAVE_TA_SOLID,
	CAVE_TA_SOLID, 0, CAVE_TA_SOLID, CAVE_TA_SOLID,
	CAVE_TA_SOLID, CAVE_TA_SOLID, 0, 0, 0, 0, 0x10u, 0x10u,
	0x10u, 0x10u, 0, CAVE_TA_SOLID, CAVE_TA_SOLID, 0, CAVE_TA_SOLID, CAVE_TA_SOLID,
	0, CAVE_TA_SOLID, 0, CAVE_TA_SOLID, CAVE_TA_SOLID, 0, 0, 0,
	0, 0, 0, CAVE_TA_SOLID, CAVE_TA_SOLID, CAVE_TA_SOLID, CAVE_TA_SOLID, CAVE_TA_SOLID,
	CAVE_TA_SOLID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	CAVE_TA_SOLID, CAVE_TA_SOLID, CAVE_TA_SOLID, CAVE_TA_SOLID,
};

static uint16_t caveRgb(uint32_t r, uint32_t g, uint32_t b)
{
	return dcAppDrawRgb565(r, g, b);
}

static uint16_t caveRd16(const uint8_t *ptr)
{
	return (uint16_t)ptr[0] | (uint16_t)((uint16_t)ptr[1] << 8);
}

static uint32_t caveRd32(const uint8_t *ptr)
{
	return (uint32_t)ptr[0] | ((uint32_t)ptr[1] << 8) | ((uint32_t)ptr[2] << 16) | ((uint32_t)ptr[3] << 24);
}

static char caveLower(char ch)
{
	if (ch >= 'A' && ch <= 'Z')
		return (char)(ch - 'A' + 'a');
	return ch;
}

static bool cavePathEqualNoCase(const char *a, const char *b)
{
	if (!a || !b)
		return false;
	while (*a && *b) {
		char ca = caveLower(*a++);
		char cb = caveLower(*b++);

		if (ca == '\\')
			ca = '/';
		if (cb == '\\')
			cb = '/';
		if (ca != cb)
			return false;
	}
	return *a == *b;
}

static bool caveBitGet(const uint8_t *bits, uint32_t count, uint32_t index)
{
	if (!bits || index >= count)
		return false;
	return (bits[index / 8u] & (uint8_t)(1u << (index & 7u))) != 0;
}

static void caveBitSet(uint8_t *bits, uint32_t count, uint32_t index, bool value)
{
	uint8_t mask;

	if (!bits || index >= count)
		return;
	mask = (uint8_t)(1u << (index & 7u));
	if (value)
		bits[index / 8u] |= mask;
	else
		bits[index / 8u] &= (uint8_t)~mask;
}

static uint32_t caveTextWidth(const char *text, enum Font font)
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

static void caveDrawText(struct DcAppDrawCtx *draw, int32_t x, int32_t y, const char *text, enum Font font, uint16_t color)
{
	if (!draw || !text)
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

static void caveDrawCentered(struct DcAppDrawCtx *draw, int32_t y, const char *text, enum Font font, uint16_t color)
{
	int32_t x = (int32_t)((CAVE_SCREEN_W - caveTextWidth(text, font)) / 2u);

	caveDrawText(draw, x, y, text, font, color);
}

static void caveBanner(struct CaveGame *game, const char *text)
{
	if (!game)
		return;
	snprintf(game->banner, sizeof(game->banner), "%s", text ? text : "");
	game->bannerFrames = 75;
}

static bool cavePackReadEntries(struct CaveGame *game)
{
	uint8_t header[CAVE_PACK_HEADER_SIZE];
	uint32_t pos = CAVE_PACK_HEADER_SIZE;
	uint32_t count;
	uint32_t nameBytes = 0;

	if (!dc32PortReadAssetPack(&game->pak, 0, header, sizeof(header)))
		return false;
	if (memcmp(header, CAVE_PACK_MAGIC, 11) || header[11] != 0 || caveRd32(header + 12) != 1u)
		return false;
	count = caveRd32(header + 16);
	if (!count || count > CAVE_MAX_ENTRIES)
		return false;
	game->entries = dc32PortCalloc(count, sizeof(*game->entries));
	if (!game->entries)
		return false;
	game->entryCount = count;
	for (uint32_t i = 0; i < count; i++) {
		uint8_t raw[CAVE_PACK_ENTRY_SIZE];
		uint16_t nameLen;

		if (!dc32PortReadAssetPack(&game->pak, pos, raw, sizeof(raw)))
			return false;
		pos += sizeof(raw);
		nameLen = caveRd16(raw);
		game->entries[i].offset = caveRd32(raw + 4);
		game->entries[i].size = caveRd32(raw + 8);
		game->entries[i].crc32 = caveRd32(raw + 12);
		if (!nameLen || nameLen >= CAVE_MAX_PATH)
			return false;
		if (nameBytes > UINT16_MAX || nameBytes > UINT32_MAX - (uint32_t)nameLen - 1u)
			return false;
		game->entries[i].nameLen = nameLen;
		nameBytes += (uint32_t)nameLen + 1u;
		pos += nameLen;
	}
	game->entryNames = dc32PortMalloc(nameBytes);
	if (!game->entryNames)
		return false;
	pos = CAVE_PACK_HEADER_SIZE;
	nameBytes = 0;
	for (uint32_t i = 0; i < count; i++) {
		uint8_t raw[CAVE_PACK_ENTRY_SIZE];
		uint16_t nameLen;

		if (!dc32PortReadAssetPack(&game->pak, pos, raw, sizeof(raw)))
			return false;
		pos += sizeof(raw);
		nameLen = caveRd16(raw);
		game->entries[i].nameOfs = (uint16_t)nameBytes;
		if (!dc32PortReadAssetPack(&game->pak, pos, game->entryNames + nameBytes, nameLen))
			return false;
		game->entryNames[nameBytes + nameLen] = 0;
		nameBytes += (uint32_t)nameLen + 1u;
		pos += nameLen;
	}
	return true;
}

static const char *caveEntryName(const struct CaveGame *game, const struct CaveEntry *entry)
{
	if (!game || !entry || !game->entryNames)
		return "";
	return game->entryNames + entry->nameOfs;
}

static struct CaveEntry *caveFindEntry(struct CaveGame *game, const char *name)
{
	if (!game || !name)
		return NULL;
	for (uint32_t i = 0; i < game->entryCount; i++)
		if (!strcmp(caveEntryName(game, &game->entries[i]), name))
			return &game->entries[i];
	for (uint32_t i = 0; i < game->entryCount; i++)
		if (cavePathEqualNoCase(caveEntryName(game, &game->entries[i]), name))
			return &game->entries[i];
	return NULL;
}

static uint8_t *caveLoadFile(struct CaveGame *game, const char *name, uint32_t *sizeOut)
{
	struct CaveEntry *entry = caveFindEntry(game, name);
	uint8_t *data;

	if (sizeOut)
		*sizeOut = 0;
	if (!entry || !entry->size)
		return NULL;
	data = dc32PortMalloc(entry->size);
	if (!data)
		return NULL;
	if (!dc32PortReadAssetPack(&game->pak, entry->offset, data, entry->size)) {
		dc32PortFree(data);
		return NULL;
	}
	if (sizeOut)
		*sizeOut = entry->size;
	return data;
}

static uint8_t caveRgbTo332(uint8_t r, uint8_t g, uint8_t b)
{
	return (uint8_t)((r & 0xe0u) | ((g >> 3) & 0x1cu) | (b >> 6));
}

static void caveFreeBitmap(struct CaveBitmap *bitmap)
{
	if (!bitmap)
		return;
	dc32PortFree(bitmap->pixels);
	memset(bitmap, 0, sizeof(*bitmap));
}

static bool caveLoadBitmap(struct CaveGame *game, const char *name, struct CaveBitmap *bitmap)
{
	uint32_t size;
	uint8_t *raw = caveLoadFile(game, name, &size);
	uint32_t pixelOffset;
	int32_t width;
	int32_t height;
	uint16_t bpp;
	uint32_t colors = 0;
	uint8_t palette[256];

	caveFreeBitmap(bitmap);
	if (!raw || size < 54 || raw[0] != 'B' || raw[1] != 'M') {
		dc32PortFree(raw);
		return false;
	}
	pixelOffset = caveRd32(raw + 10);
	width = (int32_t)caveRd32(raw + 18);
	height = (int32_t)caveRd32(raw + 22);
	bpp = caveRd16(raw + 28);
	if (width <= 0 || height == 0 || pixelOffset >= size || (bpp != 8 && bpp != 24)) {
		dc32PortFree(raw);
		return false;
	}
	if (height < 0)
		height = -height;
	if (bpp == 8) {
		colors = caveRd32(raw + 46);
		if (!colors)
			colors = 256;
		if (54u + colors * 4u > size || colors > 256u) {
			dc32PortFree(raw);
			return false;
		}
		for (uint32_t i = 0; i < colors; i++)
			palette[i] = caveRgbTo332(raw[54 + i * 4u + 2u], raw[54 + i * 4u + 1u], raw[54 + i * 4u]);
	}
	bitmap->w = (uint16_t)width;
	bitmap->h = (uint16_t)height;
	bitmap->pixels = dc32PortMalloc((uint32_t)width * (uint32_t)height);
	if (!bitmap->pixels) {
		dc32PortFree(raw);
		return false;
	}
	if (bpp == 8) {
		uint32_t stride = ((uint32_t)width + 3u) & ~3u;

		for (int32_t y = 0; y < height; y++) {
			uint32_t srcY = (uint32_t)(height - 1 - y);
			uint32_t row = pixelOffset + srcY * stride;

			if (row + (uint32_t)width > size)
				break;
			for (int32_t x = 0; x < width; x++)
				bitmap->pixels[y * width + x] = palette[raw[row + (uint32_t)x]];
		}
	}
	else {
		uint32_t stride = ((uint32_t)width * 3u + 3u) & ~3u;

		for (int32_t y = 0; y < height; y++) {
			uint32_t srcY = (uint32_t)(height - 1 - y);
			uint32_t row = pixelOffset + srcY * stride;

			if (row + (uint32_t)width * 3u > size)
				break;
			for (int32_t x = 0; x < width; x++) {
				uint32_t p = row + (uint32_t)x * 3u;

				bitmap->pixels[y * width + x] = caveRgbTo332(raw[p + 2u], raw[p + 1u], raw[p]);
			}
		}
	}
	dc32PortFree(raw);
	return true;
}

static bool caveLoadNpcTable(struct CaveGame *game)
{
	uint32_t size;
	uint8_t *raw = caveLoadFile(game, "npc.tbl", &size);
	uint32_t count;
	uint32_t pos;

	dc32PortFree(game->npcInfo);
	game->npcInfo = NULL;
	game->npcInfoCount = 0;
	if (!raw || size < CAVE_NPC_TBL_ENTRY_SIZE) {
		dc32PortFree(raw);
		return false;
	}
	count = size / CAVE_NPC_TBL_ENTRY_SIZE;
	if (count > 512u)
		count = 512u;
	game->npcInfo = dc32PortCalloc(count, sizeof(*game->npcInfo));
	if (!game->npcInfo) {
		dc32PortFree(raw);
		return false;
	}
	game->npcInfoCount = (uint16_t)count;
	pos = 0;
	for (uint32_t i = 0; i < count; i++, pos += 2u)
		game->npcInfo[i].flags = caveRd16(raw + pos);
	for (uint32_t i = 0; i < count; i++, pos += 2u)
		game->npcInfo[i].life = caveRd16(raw + pos);
	for (uint32_t i = 0; i < count; i++, pos++)
		game->npcInfo[i].sheetId = raw[pos];
	for (uint32_t i = 0; i < count; i++, pos++)
		game->npcInfo[i].deathSound = raw[pos];
	for (uint32_t i = 0; i < count; i++, pos++)
		game->npcInfo[i].hurtSound = raw[pos];
	for (uint32_t i = 0; i < count; i++, pos++)
		game->npcInfo[i].size = raw[pos];
	for (uint32_t i = 0; i < count; i++, pos += 4u)
		game->npcInfo[i].experience = caveRd32(raw + pos);
	for (uint32_t i = 0; i < count; i++, pos += 4u)
		game->npcInfo[i].damage = caveRd32(raw + pos);
	for (uint32_t i = 0; i < count; i++, pos += 4u) {
		game->npcInfo[i].hitLeft = raw[pos];
		game->npcInfo[i].hitTop = raw[pos + 1u];
		game->npcInfo[i].hitRight = raw[pos + 2u];
		game->npcInfo[i].hitBottom = raw[pos + 3u];
	}
	for (uint32_t i = 0; i < count; i++, pos += 4u) {
		game->npcInfo[i].dispLeft = raw[pos];
		game->npcInfo[i].dispTop = raw[pos + 1u];
		game->npcInfo[i].dispRight = raw[pos + 2u];
		game->npcInfo[i].dispBottom = raw[pos + 3u];
	}
	dc32PortFree(raw);
	return true;
}

static const char *caveStageResourceName(const char *name);

static bool caveLoadMap(struct CaveGame *game, const char *name)
{
	char path[80];
	uint32_t size;
	uint8_t *data;
	uint16_t w;
	uint16_t h;

	dc32PortFree(game->map.tiles);
	memset(&game->map, 0, sizeof(game->map));
	snprintf(path, sizeof(path), "Stage/%s.pxm", caveStageResourceName(name));
	data = caveLoadFile(game, path, &size);
	if (!data || size < 8 || memcmp(data, "PXM", 3) || data[3] != 0x10) {
		dc32PortFree(data);
		return false;
	}
	w = caveRd16(data + 4);
	h = caveRd16(data + 6);
	if (!w || !h || 8u + (uint32_t)w * h > size) {
		dc32PortFree(data);
		return false;
	}
	game->map.tiles = dc32PortMalloc((uint32_t)w * h);
	if (!game->map.tiles) {
		dc32PortFree(data);
		return false;
	}
	game->map.w = w;
	game->map.h = h;
	memcpy(game->map.tiles, data + 8, (uint32_t)w * h);
	dc32PortFree(data);
	return true;
}

static const char *caveStageResourceName(const char *name)
{
	if (name && !strcmp(name, "lounge"))
		return "Lounge";
	return name ? name : "";
}

static void caveLoadTileKey(struct CaveGame *game)
{
	uint32_t size;
	uint8_t *data;

	memcpy(game->tileKey, mCaveDefaultTileKey, sizeof(game->tileKey));
	data = caveLoadFile(game, "tilekey.dat", &size);
	if (!data)
		data = caveLoadFile(game, "Tilekey.dat", &size);
	if (!data || size < 256u * 4u) {
		dc32PortFree(data);
		return;
	}
	for (uint32_t i = 0; i < 256u; i++)
		game->tileKey[i] = caveRd32(data + i * 4u);
	dc32PortFree(data);
}

static void caveLoadTileAttrs(struct CaveGame *game, const char *tileset)
{
	char path[80];
	uint32_t size;
	uint8_t *data;

	for (uint32_t i = 0; i < 256u; i++) {
		game->tileCode[i] = (uint8_t)i;
		game->tileAttr[i] = game->tileKey[i];
	}
	game->tileAttrsLoaded = false;
	snprintf(path, sizeof(path), "Stage/%s.pxa", tileset ? tileset : "0");
	data = caveLoadFile(game, path, &size);
	if (!data || size < 256u) {
		dc32PortFree(data);
		return;
	}
	for (uint32_t i = 0; i < 256u; i++) {
		game->tileCode[i] = data[i];
		game->tileAttr[i] = game->tileKey[data[i]];
	}
	game->tileAttrsLoaded = true;
	dc32PortFree(data);
}

static void caveFreeEntities(struct CaveGame *game)
{
	dc32PortFree(game->entities);
	game->entities = NULL;
	game->entityCount = 0;
}

static bool caveEntityVisible(const struct CaveGame *game, const struct CaveEntity *entity)
{
	if (!entity || !(entity->npcType || entity->flagNum || entity->eventNum || entity->flags))
		return false;
	if (entity->flags & CAVE_FLAG_APPEAR_ON_FLAGID)
		return caveBitGet(game->flags, CAVE_NUM_GAME_FLAGS, entity->flagNum);
	if (entity->flags & CAVE_FLAG_DISAPPEAR_ON_FLAGID)
		return !caveBitGet(game->flags, CAVE_NUM_GAME_FLAGS, entity->flagNum);
	return true;
}

static void caveRefreshEntityVisibility(struct CaveGame *game)
{
	for (uint32_t i = 0; i < game->entityCount; i++)
		if (!game->entities[i].removed)
			game->entities[i].alive = caveEntityVisible(game, &game->entities[i]);
}

static bool caveLoadEntities(struct CaveGame *game, const char *name)
{
	char path[80];
	uint32_t size;
	uint8_t *data;
	uint8_t version;
	uint32_t count;
	uint32_t pos = 8;
	uint32_t recordSize;

	caveFreeEntities(game);
	snprintf(path, sizeof(path), "Stage/%s.pxe", caveStageResourceName(name));
	data = caveLoadFile(game, path, &size);
	if (!data)
		return true;
	if (size < 8 || memcmp(data, "PXE", 3) || (data[3] != 0 && data[3] != 0x10)) {
		dc32PortFree(data);
		return false;
	}
	version = data[3];
	count = caveRd32(data + 4);
	recordSize = 12u + (version == 0x10 ? 1u : 0u);
	if (count > CAVE_MAX_ENTITIES)
		count = CAVE_MAX_ENTITIES;
	if (8u + count * recordSize > size) {
		dc32PortFree(data);
		return false;
	}
	game->entities = dc32PortCalloc(CAVE_MAX_ENTITIES, sizeof(*game->entities));
	if (!game->entities) {
		dc32PortFree(data);
		return false;
	}
	game->entityCount = (uint16_t)count;
	for (uint32_t i = 0; i < count; i++) {
		struct CaveEntity *entity = &game->entities[i];

		entity->x = (int16_t)caveRd16(data + pos);
		entity->y = (int16_t)caveRd16(data + pos + 2u);
		entity->flagNum = caveRd16(data + pos + 4u);
		entity->eventNum = caveRd16(data + pos + 6u);
		entity->npcType = caveRd16(data + pos + 8u);
		entity->flags = caveRd16(data + pos + 10u);
		if (entity->npcType < game->npcInfoCount) {
			entity->flags |= game->npcInfo[entity->npcType].flags;
			entity->hp = game->npcInfo[entity->npcType].life;
		}
		if (!entity->hp)
			entity->hp = 1;
		entity->layer = version == 0x10 ? data[pos + 12u] : 0;
		entity->alive = caveEntityVisible(game, entity);
		pos += recordSize;
	}
	dc32PortFree(data);
	return true;
}

static void caveFreeMapScript(struct CaveGame *game)
{
	if (game->script.activeScript == game->script.mapScript) {
		game->script.running = false;
		game->script.activeScript = NULL;
	}
	dc32PortFree(game->script.mapScript);
	game->script.mapScript = NULL;
	game->script.mapScriptSize = 0;
}

static void caveFreeAllScripts(struct CaveGame *game)
{
	caveFreeMapScript(game);
	dc32PortFree(game->script.headScript);
	memset(&game->script, 0, sizeof(game->script));
}

static void caveLoadHeadScript(struct CaveGame *game)
{
	game->script.headScript = caveLoadFile(game, "Head.tsc", &game->script.headScriptSize);
	if (!game->script.headScript)
		game->script.headScript = caveLoadFile(game, "head.tsc", &game->script.headScriptSize);
}

static void caveLoadMapScript(struct CaveGame *game, const char *name)
{
	char path[80];

	caveFreeMapScript(game);
	snprintf(path, sizeof(path), "Stage/%s.tsc", caveStageResourceName(name));
	game->script.mapScript = caveLoadFile(game, path, &game->script.mapScriptSize);
}

static void caveLoadNpcSheetByName(struct CaveGame *game, const char *setName, struct CaveBitmap *bitmap)
{
	char path[80];

	snprintf(path, sizeof(path), "Npc/Npc%s.pbm", setName && *setName ? setName : "0");
	(void)caveLoadBitmap(game, path, bitmap);
}

static void caveLoadCommonNpcSheets(struct CaveGame *game)
{
	(void)game;
}

static void caveLoadStageNpcSheets(struct CaveGame *game, const struct CaveStageRecord *record)
{
	const char *npc1 = "0";
	const char *npc2 = "0";

	if (record->npcSet1 < sizeof(mCaveNpcSets) / sizeof(*mCaveNpcSets))
		npc1 = mCaveNpcSets[record->npcSet1];
	if (record->npcSet2 < sizeof(mCaveNpcSets) / sizeof(*mCaveNpcSets))
		npc2 = mCaveNpcSets[record->npcSet2];
	(void)game;
	(void)npc1;
	(void)npc2;
}

static bool caveSheetNameForNpcSet(char *dst, uint32_t dstLen, uint8_t set)
{
	if (!dst || !dstLen || set >= sizeof(mCaveNpcSheetNames) / sizeof(*mCaveNpcSheetNames))
		return false;
	snprintf(dst, dstLen, "Npc/Npc%s.pbm", mCaveNpcSheetNames[set]);
	return true;
}

static struct CaveBitmap *caveLoadedSheetForName(struct CaveGame *game, const char *name)
{
	char path[80];

	if (!game || !name || !*name)
		return NULL;
	if (cavePathEqualNoCase(name, "MyChar.pbm"))
		return &game->myChar;
	if (cavePathEqualNoCase(name, "Bullet.pbm"))
		return &game->bulletSheet;
	if (cavePathEqualNoCase(name, "Npc/Npc0.pbm"))
		return &game->npc0;
	if (cavePathEqualNoCase(name, "Npc/NpcSym.pbm"))
		return &game->npcSym;
	if (cavePathEqualNoCase(name, "Npc/NpcRegu.pbm"))
		return &game->npcRegu;
	if (caveSheetNameForNpcSet(path, sizeof(path), game->stage.npcSet1) && cavePathEqualNoCase(name, path))
		return &game->npcStage1;
	if (caveSheetNameForNpcSet(path, sizeof(path), game->stage.npcSet2) && cavePathEqualNoCase(name, path))
		return &game->npcStage2;
	if (game->stage.tileset < sizeof(mCaveTilesets) / sizeof(*mCaveTilesets)) {
		snprintf(path, sizeof(path), "Stage/Prt%s.pbm", mCaveTilesets[game->stage.tileset]);
		if (cavePathEqualNoCase(name, path))
			return &game->tileset;
	}
	return NULL;
}

static struct CaveBitmap *caveLoadNxSheet(struct CaveGame *game, const char *name)
{
	struct CaveBitmap *loaded;

	loaded = caveLoadedSheetForName(game, name);
	if (loaded && loaded->pixels)
		return loaded;
	if (game->nxTempSheetName[0] && cavePathEqualNoCase(game->nxTempSheetName, name))
		return game->nxTempSheet.pixels ? &game->nxTempSheet : NULL;
	caveFreeBitmap(&game->nxTempSheet);
	memset(game->nxTempSheetName, 0, sizeof(game->nxTempSheetName));
	snprintf(game->nxTempSheetName, sizeof(game->nxTempSheetName), "%s", name);
	if (!caveLoadBitmap(game, name, &game->nxTempSheet))
		return NULL;
	return &game->nxTempSheet;
}

static bool caveIsDigit(uint8_t ch)
{
	return ch >= '0' && ch <= '9';
}

static uint16_t caveParseEventId(const uint8_t *script, uint32_t size, uint32_t *pos)
{
	uint32_t p = *pos;
	uint32_t value = 0;

	while (p < size && caveIsDigit(script[p])) {
		value = value * 10u + (uint32_t)(script[p] - '0');
		p++;
	}
	*pos = p;
	return (uint16_t)value;
}

static bool caveFindEventInScript(const uint8_t *script, uint32_t size, uint16_t event, uint32_t *start, uint32_t *end)
{
	for (uint32_t i = 0; script && i < size; i++) {
		uint32_t p;
		uint16_t id;

		if (script[i] != '#')
			continue;
		p = i + 1u;
		id = caveParseEventId(script, size, &p);
		while (p < size && (script[p] == '\r' || script[p] == '\n'))
			p++;
		if (id != event)
			continue;
		*start = p;
		*end = size;
		for (uint32_t j = p; j < size; j++) {
			if (script[j] == '#') {
				*end = j;
				break;
			}
		}
		return true;
	}
	return false;
}

static void caveScriptStop(struct CaveGame *game)
{
	game->script.running = false;
	game->script.waitingForButton = false;
	game->script.waitFrames = 0;
	game->script.controlsLocked = false;
	game->script.messageOpen = false;
	game->script.text[0] = 0;
}

static bool caveStartScript(struct CaveGame *game, uint16_t event)
{
	uint32_t start;
	uint32_t end;

	if (!event)
		return false;
	if (caveFindEventInScript(game->script.mapScript, game->script.mapScriptSize, event, &start, &end)) {
		game->script.activeScript = game->script.mapScript;
		game->script.activeScriptSize = game->script.mapScriptSize;
	} else if (caveFindEventInScript(game->script.headScript, game->script.headScriptSize, event, &start, &end)) {
		game->script.activeScript = game->script.headScript;
		game->script.activeScriptSize = game->script.headScriptSize;
	} else {
		return false;
	}
	game->script.event = event;
	game->script.pos = start;
	game->script.end = end;
	game->script.waitFrames = 0;
	game->script.running = true;
	game->script.waitingForButton = false;
	game->script.messageOpen = false;
	game->script.controlsLocked = false;
	game->script.text[0] = 0;
	return true;
}

static int32_t caveScriptReadNumber(struct CaveGame *game)
{
	const uint8_t *script = game->script.activeScript;
	uint32_t pos = game->script.pos;
	bool negative = false;
	int32_t value = 0;

	while (pos < game->script.end &&
			(script[pos] == ':' || script[pos] == ' ' || script[pos] == '\t' || script[pos] == '\r' || script[pos] == '\n'))
		pos++;
	if (pos < game->script.end && script[pos] == '-') {
		negative = true;
		pos++;
	}
	while (pos < game->script.end && caveIsDigit(script[pos])) {
		value = value * 10 + (int32_t)(script[pos] - '0');
		pos++;
	}
	game->script.pos = pos;
	return negative ? -value : value;
}

static void caveScriptSkipParams(struct CaveGame *game)
{
	const uint8_t *script = game->script.activeScript;

	while (game->script.pos < game->script.end) {
		uint8_t ch = script[game->script.pos];

		if (ch == '<' || ch == '#' || ch == '\r' || ch == '\n')
			break;
		game->script.pos++;
	}
}

static void caveScriptAppendChar(struct CaveGame *game, uint8_t ch)
{
	size_t len;

	if (ch == '\r')
		return;
	if (ch == '\n')
		ch = ' ';
	len = strlen(game->script.text);
	if (len + 1u >= sizeof(game->script.text))
		return;
	game->script.text[len] = (char)ch;
	game->script.text[len + 1u] = 0;
	game->script.messageOpen = true;
}

static void caveScriptAppendText(struct CaveGame *game, uint8_t first)
{
	const uint8_t *script = game->script.activeScript;

	caveScriptAppendChar(game, first);
	while (game->script.pos < game->script.end && script[game->script.pos] != '<' && script[game->script.pos] != '#') {
		caveScriptAppendChar(game, script[game->script.pos]);
		game->script.pos++;
	}
}

static void caveScriptAppendNumber(struct CaveGame *game, uint16_t value)
{
	char text[8];

	snprintf(text, sizeof(text), "%u", (unsigned)value);
	for (uint32_t i = 0; text[i]; i++)
		caveScriptAppendChar(game, (uint8_t)text[i]);
}

static void caveScriptJump(struct CaveGame *game, uint16_t event)
{
	game->script.messageOpen = false;
	game->script.waitingForButton = false;
	game->script.text[0] = 0;
	if (!caveStartScript(game, event))
		caveScriptStop(game);
}

static uint32_t caveCountEntitiesByType(const struct CaveGame *game, uint16_t npcType)
{
	uint32_t count = 0;

	for (uint32_t i = 0; i < game->entityCount; i++)
		if (game->entities[i].alive && game->entities[i].npcType == npcType)
			count++;
	return count;
}

static uint32_t caveCountEntitiesByEvent(const struct CaveGame *game, uint16_t eventNum)
{
	uint32_t count = 0;

	for (uint32_t i = 0; i < game->entityCount; i++)
		if (game->entities[i].alive && game->entities[i].eventNum == eventNum)
			count++;
	return count;
}

static void caveEntityApplyTypeDefaults(struct CaveGame *game, struct CaveEntity *entity)
{
	if (!entity)
		return;
	if (entity->npcType < game->npcInfoCount) {
		const struct CaveNpcInfo *info = &game->npcInfo[entity->npcType];

		entity->flags |= info->flags;
		entity->hp = info->life ? info->life : 1u;
	} else if (!entity->hp) {
		entity->hp = 1;
	}
}

static void caveSetEntityDirection(struct CaveGame *game, struct CaveEntity *entity, uint16_t csDir)
{
	if (!entity)
		return;
	entity->tscDir = (uint8_t)csDir;
	if (csDir == 2u)
		entity->flags |= CAVE_FLAG_FACES_RIGHT;
	else if (csDir == 0u)
		entity->flags &= (uint16_t)~CAVE_FLAG_FACES_RIGHT;
	else if (csDir == 4u) {
		if ((int)entity->x * CAVE_TILE >= game->playerX)
			entity->flags &= (uint16_t)~CAVE_FLAG_FACES_RIGHT;
		else
			entity->flags |= CAVE_FLAG_FACES_RIGHT;
	}
}

static void caveDeleteEntitiesByEvent(struct CaveGame *game, uint16_t eventNum)
{
	for (uint32_t i = 0; i < game->entityCount; i++) {
		struct CaveEntity *entity = &game->entities[i];

		if (entity->alive && entity->eventNum == eventNum) {
			entity->alive = false;
			entity->removed = true;
		}
	}
}

static void caveDeleteEntitiesByType(struct CaveGame *game, uint16_t npcType)
{
	for (uint32_t i = 0; i < game->entityCount; i++) {
		struct CaveEntity *entity = &game->entities[i];

		if (entity->alive && entity->npcType == npcType) {
			entity->alive = false;
			entity->removed = true;
		}
	}
}

static void caveChangeEntitiesByEvent(struct CaveGame *game, uint16_t eventNum, uint16_t npcType, uint16_t csDir)
{
	for (uint32_t i = 0; i < game->entityCount; i++) {
		struct CaveEntity *entity = &game->entities[i];

		if (!entity->alive || entity->eventNum != eventNum)
			continue;
		entity->npcType = npcType;
		entity->flags = 0;
		entity->actionNum = 0;
		entity->removed = false;
		caveEntityApplyTypeDefaults(game, entity);
		caveSetEntityDirection(game, entity, csDir);
		entity->alive = caveEntityVisible(game, entity);
	}
}

static void caveAnimateEntitiesByEvent(struct CaveGame *game, uint16_t eventNum, uint16_t actionNum, uint16_t csDir)
{
	for (uint32_t i = 0; i < game->entityCount; i++) {
		struct CaveEntity *entity = &game->entities[i];

		if (!entity->alive || entity->eventNum != eventNum)
			continue;
		entity->actionNum = actionNum;
		caveSetEntityDirection(game, entity, csDir);
	}
}

static void caveMoveEntityByEvent(struct CaveGame *game, uint16_t eventNum, int32_t x, int32_t y, uint16_t csDir)
{
	for (uint32_t i = 0; i < game->entityCount; i++) {
		struct CaveEntity *entity = &game->entities[i];

		if (!entity->alive || entity->eventNum != eventNum)
			continue;
		entity->x = (int16_t)x;
		entity->y = (int16_t)y;
		caveSetEntityDirection(game, entity, csDir);
	}
}

static bool caveSpawnEntity(struct CaveGame *game, uint16_t npcType, int32_t x, int32_t y, uint16_t csDir)
{
	struct CaveEntity *entity;

	if (!game || !game->entities || game->entityCount >= CAVE_MAX_ENTITIES)
		return false;
	entity = &game->entities[game->entityCount++];
	memset(entity, 0, sizeof(*entity));
	entity->x = (int16_t)x;
	entity->y = (int16_t)y;
	entity->npcType = npcType;
	entity->alive = true;
	caveEntityApplyTypeDefaults(game, entity);
	caveSetEntityDirection(game, entity, csDir);
	entity->alive = caveEntityVisible(game, entity);
	return entity->alive;
}

static bool caveChangeMapTile(struct CaveGame *game, int32_t x, int32_t y, uint8_t tile)
{
	if (!game || !game->map.tiles || x < 0 || y < 0 ||
			(uint32_t)x >= game->map.w || (uint32_t)y >= game->map.h)
		return false;
	game->map.tiles[(uint32_t)y * game->map.w + (uint32_t)x] = tile;
	return true;
}

static void caveSetPlayerDirectionFromScript(struct CaveGame *game, uint16_t dir)
{
	if (dir == 0u) {
		game->playerFacingRight = false;
	} else if (dir == 2u) {
		game->playerFacingRight = true;
	} else if (dir >= 10u) {
		for (uint32_t i = 0; i < game->entityCount; i++) {
			const struct CaveEntity *entity = &game->entities[i];

			if (entity->alive && entity->eventNum == dir) {
				game->playerFacingRight = ((int)entity->x * CAVE_TILE) >= game->playerX;
				break;
			}
		}
	}
	game->playerVx = 0;
}

static void caveBoostPlayerFromScript(struct CaveGame *game, uint16_t dir)
{
	game->playerVy = -CAVE_FIX_SCALE * 2;
	if (dir >= 10u) {
		for (uint32_t i = 0; i < game->entityCount; i++) {
			const struct CaveEntity *entity = &game->entities[i];

			if (entity->alive && entity->eventNum == dir) {
				dir = game->playerX >= (int)entity->x * CAVE_TILE ? 0u : 2u;
				break;
			}
		}
	}
	if (dir == 0u) {
		game->playerFacingRight = false;
		game->playerVx = CAVE_FIX_SCALE * 2;
	} else if (dir == 2u) {
		game->playerFacingRight = true;
		game->playerVx = -CAVE_FIX_SCALE * 2;
	} else if (dir == 1u) {
		game->playerVy = -CAVE_FIX_SCALE * 2;
	} else if (dir == 3u) {
		game->playerVy = CAVE_FIX_SCALE * 2;
	}
}

static bool caveLoadStage(struct CaveGame *game, uint16_t stageIndex);
static void caveSaveGame(struct CaveGame *game);
static void caveSetPlayerPos(struct CaveGame *game, int32_t x, int32_t y);

static void caveScriptCommand(struct CaveGame *game, const char cmd[4])
{
	if (!strcmp(cmd, "END") || !strcmp(cmd, "ESC")) {
		caveScriptStop(game);
		return;
	}
	if (!strcmp(cmd, "MSG") || !strcmp(cmd, "MS2") || !strcmp(cmd, "MS3")) {
		game->script.messageOpen = true;
		game->script.messageTop = strcmp(cmd, "MSG") != 0;
		game->script.messageBorder = strcmp(cmd, "MS2") != 0;
		game->script.text[0] = 0;
		return;
	}
	if (!strcmp(cmd, "NOD")) {
		game->script.waitingForButton = true;
		return;
	}
	if (!strcmp(cmd, "KEY") || !strcmp(cmd, "PRI")) {
		game->script.controlsLocked = true;
		return;
	}
	if (!strcmp(cmd, "FRE")) {
		game->script.controlsLocked = false;
		return;
	}
	if (!strcmp(cmd, "HMC")) {
		game->playerHidden = true;
		return;
	}
	if (!strcmp(cmd, "SMC")) {
		game->playerHidden = false;
		return;
	}
	if (!strcmp(cmd, "CLR")) {
		game->script.text[0] = 0;
		return;
	}
	if (!strcmp(cmd, "CLO")) {
		game->script.messageOpen = false;
		game->script.messageTop = false;
		game->script.messageBorder = true;
		game->script.text[0] = 0;
		return;
	}
	if (!strcmp(cmd, "SAT") || !strcmp(cmd, "CAT") || !strcmp(cmd, "TUR")) {
		game->script.textFast = true;
		return;
	}
	if (!strcmp(cmd, "FAC")) {
		game->script.faceId = (uint16_t)caveScriptReadNumber(game);
		return;
	}
	if (!strcmp(cmd, "GIT")) {
		game->script.itemId = (uint16_t)caveScriptReadNumber(game);
		return;
	}
	if (!strcmp(cmd, "NUM")) {
		(void)caveScriptReadNumber(game);
		caveScriptAppendNumber(game, game->lastNumber);
		return;
	}
	if (!strcmp(cmd, "SOU")) {
		game->lastSfx = (uint16_t)caveScriptReadNumber(game);
		return;
	}
	if (!strcmp(cmd, "CMU")) {
		(void)caveScriptReadNumber(game);
		return;
	}
	if (!strcmp(cmd, "FMU") || !strcmp(cmd, "RMU")) {
		return;
	}
	if (!strcmp(cmd, "FAI") || !strcmp(cmd, "FAO")) {
		(void)caveScriptReadNumber(game);
		game->fadeFrames = 16;
		game->fadeMode = !strcmp(cmd, "FAI") ? 1u : 2u;
		game->script.waitFrames = 16;
		return;
	}
	if (!strcmp(cmd, "QUA")) {
		int32_t frames = caveScriptReadNumber(game);

		game->quakeFrames = frames > 0 ? (uint16_t)frames : 0u;
		return;
	}
	if (!strcmp(cmd, "WAI")) {
		int32_t frames = caveScriptReadNumber(game);

		game->script.waitFrames = frames > 0 ? (uint16_t)frames : 1u;
		return;
	}
	if (!strcmp(cmd, "FL+") || !strcmp(cmd, "FL-")) {
		uint16_t flag = (uint16_t)caveScriptReadNumber(game);

		caveBitSet(game->flags, CAVE_NUM_GAME_FLAGS, flag, cmd[2] == '+');
		caveRefreshEntityVisibility(game);
		return;
	}
	if (!strcmp(cmd, "SK+") || !strcmp(cmd, "SK-")) {
		uint16_t flag = (uint16_t)caveScriptReadNumber(game);

		caveBitSet(game->skipFlags, CAVE_NUM_GAME_FLAGS, flag, cmd[2] == '+');
		return;
	}
	if (!strcmp(cmd, "FF-")) {
		uint16_t from = (uint16_t)caveScriptReadNumber(game);
		uint16_t to = (uint16_t)caveScriptReadNumber(game);

		for (uint32_t flag = from; flag <= to && flag < CAVE_NUM_GAME_FLAGS; flag++) {
			if (caveBitGet(game->flags, CAVE_NUM_GAME_FLAGS, flag)) {
				caveBitSet(game->flags, CAVE_NUM_GAME_FLAGS, flag, false);
				break;
			}
			if (flag == to)
				break;
		}
		caveRefreshEntityVisibility(game);
		return;
	}
	if (!strcmp(cmd, "FLJ") || !strcmp(cmd, "SKJ")) {
		uint16_t flag = (uint16_t)caveScriptReadNumber(game);
		uint16_t event = (uint16_t)caveScriptReadNumber(game);
		bool set = !strcmp(cmd, "FLJ") ?
			caveBitGet(game->flags, CAVE_NUM_GAME_FLAGS, flag) :
			caveBitGet(game->skipFlags, CAVE_NUM_GAME_FLAGS, flag);

		if (set)
			caveScriptJump(game, event);
		return;
	}
	if (!strcmp(cmd, "IT+") || !strcmp(cmd, "IT-")) {
		uint16_t item = (uint16_t)caveScriptReadNumber(game);

		caveBitSet(game->itemFlags, CAVE_ITEM_COUNT, item, cmd[2] == '+');
		return;
	}
	if (!strcmp(cmd, "ITJ")) {
		uint16_t item = (uint16_t)caveScriptReadNumber(game);
		uint16_t event = (uint16_t)caveScriptReadNumber(game);

		if (caveBitGet(game->itemFlags, CAVE_ITEM_COUNT, item))
			caveScriptJump(game, event);
		return;
	}
	if (!strcmp(cmd, "AM+") || !strcmp(cmd, "AM-")) {
		uint16_t weapon = (uint16_t)caveScriptReadNumber(game);

		if (cmd[2] == '+')
			game->lastNumber = (uint16_t)caveScriptReadNumber(game);
		caveBitSet(game->weaponFlags, CAVE_WEAPON_COUNT, weapon, cmd[2] == '+');
		return;
	}
	if (!strcmp(cmd, "AMJ")) {
		uint16_t weapon = (uint16_t)caveScriptReadNumber(game);
		uint16_t event = (uint16_t)caveScriptReadNumber(game);

		if (caveBitGet(game->weaponFlags, CAVE_WEAPON_COUNT, weapon))
			caveScriptJump(game, event);
		return;
	}
	if (!strcmp(cmd, "EQ+") || !strcmp(cmd, "EQ-")) {
		uint32_t mask = (uint32_t)caveScriptReadNumber(game);

		if (cmd[2] == '+')
			game->equipMask |= mask;
		else
			game->equipMask &= ~mask;
		return;
	}
	if (!strcmp(cmd, "ML+")) {
		int32_t amount = caveScriptReadNumber(game);

		if (amount > 0) {
			game->maxLife = (uint16_t)(game->maxLife + (uint16_t)amount);
			game->life = (uint16_t)(game->life + (uint16_t)amount);
			if (game->life > game->maxLife)
				game->life = game->maxLife;
		}
		return;
	}
	if (!strcmp(cmd, "MP+")) {
		uint16_t stage = (uint16_t)caveScriptReadNumber(game);

		caveBitSet(game->mapFlags, CAVE_MAP_FLAG_COUNT, stage, true);
		return;
	}
	if (!strcmp(cmd, "MPJ")) {
		uint16_t event = (uint16_t)caveScriptReadNumber(game);

		if (caveBitGet(game->mapFlags, CAVE_MAP_FLAG_COUNT, game->stageIndex))
			caveScriptJump(game, event);
		return;
	}
	if (!strcmp(cmd, "LI+")) {
		int32_t amount = caveScriptReadNumber(game);

		if (amount > 0) {
			game->life = (uint16_t)(game->life + (uint16_t)amount);
			if (game->life > game->maxLife)
				game->life = game->maxLife;
		}
		return;
	}
	if (!strcmp(cmd, "MYD")) {
		uint16_t dir = (uint16_t)caveScriptReadNumber(game);

		caveSetPlayerDirectionFromScript(game, dir);
		return;
	}
	if (!strcmp(cmd, "MYB")) {
		uint16_t dir = (uint16_t)caveScriptReadNumber(game);

		caveBoostPlayerFromScript(game, dir);
		return;
	}
	if (!strcmp(cmd, "MM0")) {
		game->playerVx = 0;
		return;
	}
	if (!strcmp(cmd, "UNI")) {
		game->controlMode = (uint8_t)caveScriptReadNumber(game);
		return;
	}
	if (!strcmp(cmd, "CMP")) {
		int32_t x = caveScriptReadNumber(game);
		int32_t y = caveScriptReadNumber(game);
		uint8_t tile = (uint8_t)caveScriptReadNumber(game);

		(void)caveChangeMapTile(game, x, y, tile);
		return;
	}
	if (!strcmp(cmd, "SMP")) {
		int32_t x = caveScriptReadNumber(game);
		int32_t y = caveScriptReadNumber(game);

		if (game->map.tiles && x >= 0 && y >= 0 &&
				(uint32_t)x < game->map.w && (uint32_t)y < game->map.h) {
			uint32_t idx = (uint32_t)y * game->map.w + (uint32_t)x;

			game->map.tiles[idx]--;
		}
		return;
	}
	if (!strcmp(cmd, "NCJ")) {
		uint16_t npcType = (uint16_t)caveScriptReadNumber(game);
		uint16_t event = (uint16_t)caveScriptReadNumber(game);

		if (caveCountEntitiesByType(game, npcType))
			caveScriptJump(game, event);
		return;
	}
	if (!strcmp(cmd, "ECJ")) {
		uint16_t eventNum = (uint16_t)caveScriptReadNumber(game);
		uint16_t event = (uint16_t)caveScriptReadNumber(game);

		if (caveCountEntitiesByEvent(game, eventNum))
			caveScriptJump(game, event);
		return;
	}
	if (!strcmp(cmd, "MOV")) {
		int32_t x = caveScriptReadNumber(game);
		int32_t y = caveScriptReadNumber(game);

		caveSetPlayerPos(game, x * CAVE_TILE, y * CAVE_TILE);
		game->playerVx = 0;
		game->playerVy = 0;
		return;
	}
	if (!strcmp(cmd, "TRA")) {
		uint16_t stage = (uint16_t)caveScriptReadNumber(game);
		uint16_t event = (uint16_t)caveScriptReadNumber(game);
		int32_t x = caveScriptReadNumber(game);
		int32_t y = caveScriptReadNumber(game);

		if (caveLoadStage(game, stage)) {
			caveSetPlayerPos(game, x * CAVE_TILE, y * CAVE_TILE);
			game->playerVx = 0;
			game->playerVy = 0;
			caveSaveGame(game);
			if (!caveStartScript(game, event))
				caveScriptStop(game);
		}
		return;
	}
	if (!strcmp(cmd, "EVE")) {
		uint16_t event = (uint16_t)caveScriptReadNumber(game);

		caveScriptJump(game, event);
		return;
	}
	if (!strcmp(cmd, "DNP")) {
		uint16_t eventNum = (uint16_t)caveScriptReadNumber(game);

		caveDeleteEntitiesByEvent(game, eventNum);
		return;
	}
	if (!strcmp(cmd, "DNA")) {
		uint16_t npcType = (uint16_t)caveScriptReadNumber(game);

		caveDeleteEntitiesByType(game, npcType);
		return;
	}
	if (!strcmp(cmd, "ANP")) {
		uint16_t eventNum = (uint16_t)caveScriptReadNumber(game);
		uint16_t actionNum = (uint16_t)caveScriptReadNumber(game);
		uint16_t dir = (uint16_t)caveScriptReadNumber(game);

		caveAnimateEntitiesByEvent(game, eventNum, actionNum, dir);
		return;
	}
	if (!strcmp(cmd, "CNP") || !strcmp(cmd, "INP")) {
		uint16_t eventNum = (uint16_t)caveScriptReadNumber(game);
		uint16_t npcType = (uint16_t)caveScriptReadNumber(game);
		uint16_t dir = (uint16_t)caveScriptReadNumber(game);

		caveChangeEntitiesByEvent(game, eventNum, npcType, dir);
		if (!strcmp(cmd, "INP"))
			for (uint32_t i = 0; i < game->entityCount; i++)
				if (game->entities[i].eventNum == eventNum)
					game->entities[i].flags |= CAVE_FLAG_SCRIPT_ON_TOUCH;
		return;
	}
	if (!strcmp(cmd, "MNP")) {
		uint16_t eventNum = (uint16_t)caveScriptReadNumber(game);
		int32_t x = caveScriptReadNumber(game);
		int32_t y = caveScriptReadNumber(game);
		uint16_t dir = (uint16_t)caveScriptReadNumber(game);

		caveMoveEntityByEvent(game, eventNum, x, y, dir);
		return;
	}
	if (!strcmp(cmd, "SNP")) {
		uint16_t npcType = (uint16_t)caveScriptReadNumber(game);
		int32_t x = caveScriptReadNumber(game);
		int32_t y = caveScriptReadNumber(game);
		uint16_t dir = (uint16_t)caveScriptReadNumber(game);

		(void)caveSpawnEntity(game, npcType, x, y, dir);
		return;
	}
	if (!strcmp(cmd, "SVP")) {
		caveSaveGame(game);
		caveBanner(game, "SAVED");
		return;
	}
	if (!strcmp(cmd, "MNA")) {
		caveBanner(game, game->stage.stagename[0] ? game->stage.stagename : game->stage.filename);
		return;
	}
	caveScriptSkipParams(game);
}

static void caveScriptStep(struct CaveGame *game)
{
	const uint8_t *script = game->script.activeScript;

	if (!game->script.running || !script)
		return;
	if (game->script.waitingForButton) {
		if (game->draw.pressed & (KEY_BIT_A | KEY_BIT_B | KEY_BIT_START)) {
			game->script.waitingForButton = false;
			game->script.messageOpen = false;
			game->script.text[0] = 0;
		}
		return;
	}
	if (game->script.waitFrames) {
		game->script.waitFrames--;
		return;
	}
	for (uint32_t steps = 0; steps < 16u && game->script.running && !game->script.waitingForButton && !game->script.waitFrames; steps++) {
		uint8_t ch;

		if (game->script.pos >= game->script.end) {
			caveScriptStop(game);
			return;
		}
		ch = script[game->script.pos++];
		if (ch == '<') {
			char cmd[4] = {0, 0, 0, 0};

			if (game->script.pos + 3u > game->script.end) {
				caveScriptStop(game);
				return;
			}
			cmd[0] = (char)script[game->script.pos++];
			cmd[1] = (char)script[game->script.pos++];
			cmd[2] = (char)script[game->script.pos++];
			caveScriptCommand(game, cmd);
			script = game->script.activeScript;
		} else if (ch == '#') {
			caveScriptStop(game);
			return;
		} else {
			caveScriptAppendText(game, ch);
			if (game->script.messageOpen)
				return;
		}
	}
}

static bool caveLoadStageRecord(struct CaveGame *game, uint16_t stageIndex, struct CaveStageRecord *record)
{
	uint32_t offset;

	if (!game->stageDat || game->stageDatSize < 1)
		return false;
	game->stageCount = game->stageDat[0];
	offset = 1u + (uint32_t)stageIndex * 73u;
	if (stageIndex >= game->stageCount || offset + 73u > game->stageDatSize)
		return false;
	memset(record, 0, sizeof(*record));
	memcpy(record->filename, game->stageDat + offset, 32);
	memcpy(record->stagename, game->stageDat + offset + 32u, 35);
	record->filename[31] = 0;
	record->stagename[34] = 0;
	record->tileset = game->stageDat[offset + 67u];
	record->bgNo = game->stageDat[offset + 68u];
	record->scrollType = game->stageDat[offset + 69u];
	record->bossNo = game->stageDat[offset + 70u];
	record->npcSet1 = game->stageDat[offset + 71u];
	record->npcSet2 = game->stageDat[offset + 72u];
	return true;
}

static bool caveLoadStage(struct CaveGame *game, uint16_t stageIndex)
{
	struct CaveStageRecord record;
	char tilesetPath[80];
	char backdropPath[80];
	const char *tileset = "0";
	const char *backdrop = "bk0";

	if (!caveLoadStageRecord(game, stageIndex, &record))
		return false;
	caveFreeBitmap(&game->nxTempSheet);
	game->nxTempSheetName[0] = 0;
	if (record.tileset < sizeof(mCaveTilesets) / sizeof(*mCaveTilesets))
		tileset = mCaveTilesets[record.tileset];
	if (record.bgNo < sizeof(mCaveBackdrops) / sizeof(*mCaveBackdrops))
		backdrop = mCaveBackdrops[record.bgNo];
	snprintf(tilesetPath, sizeof(tilesetPath), "Stage/Prt%s.pbm", tileset);
	snprintf(backdropPath, sizeof(backdropPath), "%s.pbm", backdrop);
	if (!caveLoadMap(game, record.filename))
		return false;
	caveLoadTileAttrs(game, tileset);
	if (!caveLoadEntities(game, record.filename))
		return false;
	caveLoadMapScript(game, record.filename);
	caveLoadStageNpcSheets(game, &record);
	(void)caveLoadBitmap(game, tilesetPath, &game->tileset);
	(void)caveLoadBitmap(game, backdropPath, &game->backdrop);
	game->stage = record;
	game->stageIndex = stageIndex;
	if (game->playerX < 0 || game->playerY < 0 ||
			game->playerX >= (int16_t)(game->map.w * CAVE_TILE) ||
			game->playerY >= (int16_t)(game->map.h * CAVE_TILE)) {
		caveSetPlayerPos(game, 64, 64);
		game->playerVx = 0;
		game->playerVy = 0;
	} else {
		caveSetPlayerPos(game, game->playerX, game->playerY);
	}
	caveBanner(game, record.stagename[0] ? record.stagename : record.filename);
	return true;
}

static void caveSaveGame(struct CaveGame *game)
{
	struct CaveSave save;

	if (!game || !game->args || !game->args->vol)
		return;
	memset(&save, 0, sizeof(save));
	save.magic = CAVE_SAVE_MAGIC;
	save.version = CAVE_SAVE_VERSION;
	save.size = sizeof(save);
	save.stage = game->stageIndex;
	save.playerX = game->playerX;
	save.playerY = game->playerY;
	save.life = game->life;
	save.maxLife = game->maxLife;
	save.equipMask = game->equipMask;
	memcpy(save.flags, game->flags, sizeof(save.flags));
	memcpy(save.skipFlags, game->skipFlags, sizeof(save.skipFlags));
	memcpy(save.mapFlags, game->mapFlags, sizeof(save.mapFlags));
	memcpy(save.itemFlags, game->itemFlags, sizeof(save.itemFlags));
	memcpy(save.weaponFlags, game->weaponFlags, sizeof(save.weaponFlags));
	(void)dc32PortSaveWrite(game->args->vol, "cave", &save, sizeof(save));
}

static bool caveRestoreSave(struct CaveGame *game)
{
	struct CaveSave save;

	if (!game || !game->args || !game->args->vol ||
			!dc32PortSaveRead(game->args->vol, "cave", &save, sizeof(save)) ||
			save.magic != CAVE_SAVE_MAGIC || save.version != CAVE_SAVE_VERSION ||
			save.size != sizeof(save))
		return false;
	game->stageIndex = save.stage;
	game->playerX = save.playerX;
	game->playerY = save.playerY;
	game->life = save.life ? save.life : 3u;
	game->maxLife = save.maxLife ? save.maxLife : game->life;
	if (game->life > game->maxLife)
		game->life = game->maxLife;
	game->equipMask = save.equipMask;
	memcpy(game->flags, save.flags, sizeof(game->flags));
	memcpy(game->skipFlags, save.skipFlags, sizeof(game->skipFlags));
	memcpy(game->mapFlags, save.mapFlags, sizeof(game->mapFlags));
	memcpy(game->itemFlags, save.itemFlags, sizeof(game->itemFlags));
	memcpy(game->weaponFlags, save.weaponFlags, sizeof(game->weaponFlags));
	return true;
}

static bool caveTileSolid(struct CaveGame *game, uint8_t tile)
{
	static const uint8_t fallbackSolid[] = {0x05, 0x41, 0x43, 0x46, 0x54, 0x55, 0x56, 0x57};

	if (game && game->tileAttrsLoaded)
		return (game->tileAttr[tile] & CAVE_TA_SOLID_PLAYER) != 0;
	for (uint32_t i = 0; i < sizeof(fallbackSolid); i++)
		if (fallbackSolid[i] == tile)
			return true;
	return false;
}

static bool caveMapSolidAt(struct CaveGame *game, int x, int y)
{
	uint32_t tx;
	uint32_t ty;

	if (!game->map.tiles || x < 0 || y < 0)
		return true;
	tx = (uint32_t)x / CAVE_TILE;
	ty = (uint32_t)y / CAVE_TILE;
	if (tx >= game->map.w || ty >= game->map.h)
		return true;
	return caveTileSolid(game, game->map.tiles[ty * game->map.w + tx]);
}

static void caveSetPlayerPos(struct CaveGame *game, int32_t x, int32_t y)
{
	game->playerX = (int16_t)x;
	game->playerY = (int16_t)y;
	game->playerFx = x * CAVE_FIX_SCALE;
	game->playerFy = y * CAVE_FIX_SCALE;
}

static bool cavePlayerCollidesAt(struct CaveGame *game, int x, int y)
{
	return caveMapSolidAt(game, x + 3, y + 2) ||
			caveMapSolidAt(game, x + 12, y + 2) ||
			caveMapSolidAt(game, x + 3, y + 15) ||
			caveMapSolidAt(game, x + 12, y + 15);
}

static void caveMovePlayerFixedX(struct CaveGame *game)
{
	int32_t targetFx = game->playerFx + game->playerVx;
	int32_t targetX = targetFx / CAVE_FIX_SCALE;
	int step;

	if (targetX == game->playerX) {
		game->playerFx = targetFx;
		return;
	}
	step = targetX > game->playerX ? 1 : -1;
	while (game->playerX != targetX) {
		int next = game->playerX + step;

		if (cavePlayerCollidesAt(game, next, game->playerY)) {
			game->playerVx = 0;
			game->playerFx = game->playerX * CAVE_FIX_SCALE;
			return;
		}
		game->playerX = (int16_t)next;
	}
	game->playerFx = targetFx;
}

static void caveMovePlayerFixedY(struct CaveGame *game)
{
	int32_t targetFy = game->playerFy + game->playerVy;
	int32_t targetY = targetFy / CAVE_FIX_SCALE;
	int step;

	game->playerOnGround = false;
	if (targetY == game->playerY) {
		game->playerFy = targetFy;
		if (game->playerVy >= 0 && cavePlayerCollidesAt(game, game->playerX, game->playerY + 1)) {
			game->playerOnGround = true;
			game->playerVy = 0;
			game->playerFy = game->playerY * CAVE_FIX_SCALE;
		}
		return;
	}
	step = targetY > game->playerY ? 1 : -1;
	while (game->playerY != targetY) {
		int next = game->playerY + step;

		if (cavePlayerCollidesAt(game, game->playerX, next)) {
			if (step > 0)
				game->playerOnGround = true;
			game->playerVy = 0;
			game->playerFy = game->playerY * CAVE_FIX_SCALE;
			return;
		}
		game->playerY = (int16_t)next;
	}
	game->playerFy = targetFy;
}

static void caveApplyPlayerPhysics(struct CaveGame *game)
{
	const int32_t accel = CAVE_FIX_SCALE / 5;
	const int32_t friction = CAVE_FIX_SCALE / 8;
	const int32_t maxWalk = CAVE_FIX_SCALE + CAVE_FIX_SCALE / 2;
	const int32_t gravity = CAVE_FIX_SCALE / 4;
	const int32_t maxFall = CAVE_FIX_SCALE * 5;
	bool left = (game->draw.keys & KEY_BIT_LEFT) != 0;
	bool right = (game->draw.keys & KEY_BIT_RIGHT) != 0;

	game->playerAimUp = (game->draw.keys & KEY_BIT_UP) != 0;
	game->playerAimDown = (game->draw.keys & KEY_BIT_DOWN) != 0 && !game->playerOnGround;
	if (left && !right) {
		game->playerVx -= accel;
		game->playerFacingRight = false;
	} else if (right && !left) {
		game->playerVx += accel;
		game->playerFacingRight = true;
	} else if (game->playerVx > 0) {
		game->playerVx = game->playerVx > friction ? game->playerVx - friction : 0;
	} else if (game->playerVx < 0) {
		game->playerVx = game->playerVx < -friction ? game->playerVx + friction : 0;
	}
	if (game->playerVx > maxWalk)
		game->playerVx = maxWalk;
	if (game->playerVx < -maxWalk)
		game->playerVx = -maxWalk;
	if ((game->draw.pressed & KEY_BIT_A) && game->playerOnGround) {
		game->playerVy = -CAVE_FIX_SCALE * 4;
		game->playerOnGround = false;
	}
	game->playerVy += gravity;
	if (game->playerVy > maxFall)
		game->playerVy = maxFall;
	caveMovePlayerFixedX(game);
	caveMovePlayerFixedY(game);
	if (game->playerOnGround)
		game->playerFy = game->playerY * CAVE_FIX_SCALE;
	game->playerAnimTick++;
}

static void caveTransition(struct CaveGame *game, int delta)
{
	int next = (int)game->stageIndex + delta;

	if (next < 0)
		next = game->stageCount ? game->stageCount - 1 : 0;
	if (game->stageCount && next >= game->stageCount)
		next = 0;
	caveSetPlayerPos(game, 64, 64);
	game->playerVx = 0;
	game->playerVy = 0;
	if (caveLoadStage(game, (uint16_t)next))
		caveSaveGame(game);
}

static void caveDrawBitmapTile(struct CaveGame *game, int dstX, int dstY, uint8_t tile)
{
	uint32_t srcX = ((uint32_t)tile % 16u) * CAVE_TILE;
	uint32_t srcY = ((uint32_t)tile / 16u) * CAVE_TILE;

	if (!game->tileset.pixels || srcX + CAVE_TILE > game->tileset.w || srcY + CAVE_TILE > game->tileset.h) {
		uint8_t shade = (uint8_t)(48u + (tile * 19u) % 160u);
		uint16_t color = caveRgb(shade, shade, shade / 2u);

		dcAppDrawFill(&game->draw, dstX, dstY, CAVE_TILE, CAVE_TILE, color);
		return;
	}
	for (uint32_t y = 0; y < CAVE_TILE; y++) {
		for (uint32_t x = 0; x < CAVE_TILE; x++) {
			uint8_t px = game->tileset.pixels[(srcY + y) * game->tileset.w + srcX + x];

			if (px != 0xe3u)
				dcAppDrawPixel(&game->draw, dstX + (int32_t)x, dstY + (int32_t)y, dc32PortRgb332ToRgb565(px));
		}
	}
}

static void caveDrawPlayer(struct CaveGame *game, int x, int y)
{
	if (game->myChar.pixels && game->myChar.w >= 16 && game->myChar.h >= 16) {
		static const uint8_t walkFrames[] = {1, 0, 2, 0};
		static const uint8_t walkUpFrames[] = {4, 3, 5, 3};
		uint32_t frame = 0;
		uint32_t srcX;
		uint32_t srcY = game->playerFacingRight ? 16u : 0u;

		if (!game->playerOnGround) {
			if (game->playerAimUp)
				frame = 4;
			else if (game->playerAimDown)
				frame = 6;
			else
				frame = game->playerVy < 0 ? 2u : 1u;
		} else if (game->playerAimUp) {
			if (game->playerVx > CAVE_FIX_SCALE / 4 || game->playerVx < -CAVE_FIX_SCALE / 4)
				frame = walkUpFrames[(game->playerAnimTick / 5u) & 3u];
			else
				frame = 3;
		} else if (game->playerVx > CAVE_FIX_SCALE / 4 || game->playerVx < -CAVE_FIX_SCALE / 4) {
			frame = walkFrames[(game->playerAnimTick / 5u) & 3u];
		}
		srcX = frame * 16u;
		if (srcX + 16u > game->myChar.w || srcY + 16u > game->myChar.h) {
			srcX = 0;
			srcY = 0;
		}
		for (uint32_t py = 0; py < 16; py++) {
			for (uint32_t px = 0; px < 16; px++) {
				uint8_t color = game->myChar.pixels[(srcY + py) * game->myChar.w + srcX + px];

				if (color != 0xe3u && color)
					dcAppDrawPixel(&game->draw, x + (int32_t)px, y + (int32_t)py, dc32PortRgb332ToRgb565(color));
			}
		}
		return;
	}
	dcAppDrawFill(&game->draw, x + 4, y + 2, 8, 14, caveRgb(238, 238, 220));
	dcAppDrawFill(&game->draw, x + 3, y + 5, 10, 8, caveRgb(220, 40, 40));
}

static struct CaveBitmap *caveNpcSheetForId(struct CaveGame *game, uint8_t sheetId)
{
	switch (sheetId) {
	case 16:
		return &game->myChar;
	case 20:
		return &game->npcSym;
	case 21:
		return &game->npcStage1;
	case 22:
		return &game->npcStage2;
	case 23:
		return &game->npcRegu;
	default:
		return &game->npc0;
	}
}

static void caveDrawBitmapRegion(struct CaveGame *game, struct CaveBitmap *bitmap, int dstX, int dstY,
		uint32_t srcX, uint32_t srcY, uint32_t w, uint32_t h)
{
	if (!bitmap || !bitmap->pixels || !w || !h || srcX + w > bitmap->w || srcY + h > bitmap->h)
		return;
	for (uint32_t y = 0; y < h; y++) {
		for (uint32_t x = 0; x < w; x++) {
			uint8_t color = bitmap->pixels[(srcY + y) * bitmap->w + srcX + x];

			if (color != 0xe3u && color)
				dcAppDrawPixel(&game->draw, dstX + (int32_t)x, dstY + (int32_t)y, dc32PortRgb332ToRgb565(color));
		}
	}
}

static void caveDrawBitmapRegionOpaque(struct CaveGame *game, struct CaveBitmap *bitmap, int dstX, int dstY,
		uint32_t srcX, uint32_t srcY, uint32_t w, uint32_t h)
{
	if (!bitmap || !bitmap->pixels || !w || !h || srcX + w > bitmap->w || srcY + h > bitmap->h)
		return;
	for (uint32_t y = 0; y < h; y++) {
		int py = dstY + (int32_t)y;

		if (py < 0 || py >= (int)CAVE_SCREEN_H)
			continue;
		for (uint32_t x = 0; x < w; x++) {
			int px = dstX + (int32_t)x;
			uint8_t color;

			if (px < 0 || px >= (int)CAVE_SCREEN_W)
				continue;
			color = bitmap->pixels[(srcY + y) * bitmap->w + srcX + x];
			dcAppDrawPixel(&game->draw, px, py, dc32PortRgb332ToRgb565(color));
		}
	}
}

static void caveDrawBackdrop(struct CaveGame *game, int scrollX, int scrollY)
{
	int offX = 0;
	int offY = 0;
	int stepX;
	int stepY;

	if (!game->backdrop.pixels || !game->backdrop.w || !game->backdrop.h) {
		dcAppDrawClear(&game->draw, caveRgb(24, 30, 46));
		return;
	}
	if (game->stage.scrollType == 2u) {
		offX = scrollX % (int)game->backdrop.w;
		offY = scrollY % (int)game->backdrop.h;
	} else if (game->stage.scrollType == 1u || game->stage.scrollType == 9u) {
		offX = (scrollX / 2) % (int)game->backdrop.w;
		offY = (scrollY / 2) % (int)game->backdrop.h;
	} else if (game->stage.scrollType == 5u) {
		offX = (int)(game->playerAnimTick * 3u) % (int)game->backdrop.w;
	}
	if (offX < 0)
		offX += game->backdrop.w;
	if (offY < 0)
		offY += game->backdrop.h;
	stepX = (int)game->backdrop.w;
	stepY = (int)game->backdrop.h;
	for (int y = -offY; y < (int)CAVE_SCREEN_H; y += stepY)
		for (int x = -offX; x < (int)CAVE_SCREEN_W; x += stepX)
			caveDrawBitmapRegionOpaque(game, &game->backdrop, x, y, 0, 0, game->backdrop.w, game->backdrop.h);
}

static bool caveDrawNxSprite(struct CaveGame *game, uint16_t spriteId, uint8_t frame, uint8_t dir, int dstX, int dstY)
{
	const struct CaveNxSpriteMeta *meta;
	const struct CaveNxSpriteDir *spriteDir;
	const char *sheetName;
	struct CaveBitmap *sheet;
	uint32_t dirIndex;

	if (spriteId >= caveNxSpriteCount)
		return false;
	meta = &caveNxSprites[spriteId];
	if (!meta->w || !meta->h || !meta->nframes || !meta->ndirs || meta->sheet >= caveNxSheetCount)
		return false;
	frame %= meta->nframes;
	dir %= meta->ndirs;
	dirIndex = (uint32_t)meta->firstDir + (uint32_t)frame * meta->ndirs + dir;
	spriteDir = &caveNxSpriteDirs[dirIndex];
	sheetName = caveNxSheetNames[meta->sheet];
	sheet = caveLoadNxSheet(game, sheetName);
	if (!sheet || !sheet->pixels)
		return false;
	dstX -= spriteDir->dx;
	dstY -= spriteDir->dy;
	if (dstX < -(int)meta->w || dstY < -(int)meta->h || dstX >= (int)CAVE_SCREEN_W || dstY >= (int)CAVE_SCREEN_H)
		return true;
	caveDrawBitmapRegion(game, sheet, dstX, dstY, (uint32_t)spriteDir->sx, (uint32_t)spriteDir->sy, meta->w, meta->h);
	return true;
}

static bool caveDrawNxObjectSprite(struct CaveGame *game, const struct CaveEntity *entity, int scrollX, int scrollY)
{
	uint16_t spriteId;
	uint8_t frame;
	uint8_t dir;
	int dstX;
	int dstY;

	if (!entity || entity->npcType >= caveNxObjectSpriteCount)
		return false;
	spriteId = caveNxObjectSprites[entity->npcType];
	if (!spriteId)
		return false;
	frame = entity->actionNum ? (uint8_t)entity->actionNum : (uint8_t)((game->playerAnimTick / 8u) & 3u);
	dir = (entity->flags & CAVE_FLAG_FACES_RIGHT) ? 1u : 0u;
	dstX = (int)entity->x * CAVE_TILE - scrollX;
	dstY = (int)entity->y * CAVE_TILE - scrollY;
	return caveDrawNxSprite(game, spriteId, frame, dir, dstX, dstY);
}

static uint32_t caveActiveBullets(const struct CaveGame *game)
{
	uint32_t count = 0;

	for (uint32_t i = 0; i < CAVE_MAX_BULLETS; i++)
		if (game->bullets[i].alive)
			count++;
	return count;
}

static bool caveBulletHitsEntity(struct CaveGame *game, const struct CaveBullet *bullet, struct CaveEntity *entity)
{
	const struct CaveNpcInfo *info;
	int bx = bullet->x / CAVE_FIX_SCALE;
	int by = bullet->y / CAVE_FIX_SCALE;
	int ex;
	int ey;
	int left = 8;
	int top = 8;
	int right = 8;
	int bottom = 8;

	if (!entity->alive || !(entity->flags & CAVE_FLAG_SHOOTABLE) || (entity->flags & CAVE_FLAG_INVULNERABLE))
		return false;
	if (entity->npcType < game->npcInfoCount) {
		info = &game->npcInfo[entity->npcType];
		left = info->hitLeft ? info->hitLeft : left;
		top = info->hitTop ? info->hitTop : top;
		right = info->hitRight ? info->hitRight : right;
		bottom = info->hitBottom ? info->hitBottom : bottom;
	}
	ex = (int)entity->x * CAVE_TILE;
	ey = (int)entity->y * CAVE_TILE;
	return bx + 3 >= ex - left && bx - 3 <= ex + right && by + 3 >= ey - top && by - 3 <= ey + bottom;
}

static void caveDamageEntity(struct CaveGame *game, struct CaveEntity *entity)
{
	if (entity->hp > 1) {
		entity->hp--;
		return;
	}
	entity->alive = false;
	if ((entity->flags & CAVE_FLAG_SCRIPT_ON_DEATH) && entity->eventNum)
		(void)caveStartScript(game, entity->eventNum);
}

static void caveTickBullets(struct CaveGame *game)
{
	for (uint32_t i = 0; i < CAVE_MAX_BULLETS; i++) {
		struct CaveBullet *bullet = &game->bullets[i];
		int px;
		int py;

		if (!bullet->alive)
			continue;
		bullet->x += bullet->vx;
		bullet->y += bullet->vy;
		if (bullet->ttl)
			bullet->ttl--;
		px = bullet->x / CAVE_FIX_SCALE;
		py = bullet->y / CAVE_FIX_SCALE;
		if (!bullet->ttl || caveMapSolidAt(game, px, py)) {
			bullet->alive = false;
			continue;
		}
		for (uint32_t e = 0; e < game->entityCount; e++) {
			if (caveBulletHitsEntity(game, bullet, &game->entities[e])) {
				caveDamageEntity(game, &game->entities[e]);
				bullet->alive = false;
				break;
			}
		}
	}
}

static void caveFirePolarStar(struct CaveGame *game)
{
	struct CaveBullet *slot = NULL;
	int32_t speed = CAVE_FIX_SCALE * 6;
	int px = game->playerX + 8;
	int py = game->playerY + 8;
	uint8_t dir;

	if (caveActiveBullets(game) >= 2)
		return;
	for (uint32_t i = 0; i < CAVE_MAX_BULLETS; i++) {
		if (!game->bullets[i].alive) {
			slot = &game->bullets[i];
			break;
		}
	}
	if (!slot)
		return;
	memset(slot, 0, sizeof(*slot));
	if (game->draw.keys & KEY_BIT_UP) {
		dir = 2;
		slot->x = (px + (game->playerFacingRight ? 1 : -1)) * CAVE_FIX_SCALE;
		slot->y = (game->playerY - 8) * CAVE_FIX_SCALE;
		slot->vy = -speed;
	} else if ((game->draw.keys & KEY_BIT_DOWN) && !game->playerOnGround) {
		dir = 3;
		slot->x = (px + (game->playerFacingRight ? 1 : -1)) * CAVE_FIX_SCALE;
		slot->y = (game->playerY + 18) * CAVE_FIX_SCALE;
		slot->vy = speed;
	} else if (game->playerFacingRight) {
		dir = 1;
		slot->x = (game->playerX + 18) * CAVE_FIX_SCALE;
		slot->y = (py + 2) * CAVE_FIX_SCALE;
		slot->vx = speed;
	} else {
		dir = 0;
		slot->x = (game->playerX - 3) * CAVE_FIX_SCALE;
		slot->y = (py + 2) * CAVE_FIX_SCALE;
		slot->vx = -speed;
	}
	slot->dir = dir;
	slot->ttl = 50;
	slot->alive = true;
}

static void caveDrawBullets(struct CaveGame *game, int scrollX, int scrollY)
{
	for (uint32_t i = 0; i < CAVE_MAX_BULLETS; i++) {
		const struct CaveBullet *bullet = &game->bullets[i];
		int x;
		int y;
		uint32_t srcX;
		uint32_t srcY = 0;

		if (!bullet->alive)
			continue;
		x = bullet->x / CAVE_FIX_SCALE - 4 - scrollX;
		y = bullet->y / CAVE_FIX_SCALE - 4 - scrollY;
		if (x < -8 || y < -8 || x >= (int)CAVE_SCREEN_W || y >= (int)CAVE_SCREEN_H)
			continue;
		srcX = (bullet->dir & 1u) ? 8u : 0u;
		if ((bullet->dir == 2 || bullet->dir == 3) && game->bulletSheet.w >= 24)
			srcX = 16u;
		if (game->bulletSheet.pixels && srcX + 8u <= game->bulletSheet.w && srcY + 8u <= game->bulletSheet.h) {
			caveDrawBitmapRegion(game, &game->bulletSheet, x, y, srcX, srcY, 8, 8);
		} else {
			struct CaveBitmap *bullet = caveLoadNxSheet(game, "Bullet.pbm");

			if (bullet && srcX + 8u <= bullet->w && srcY + 8u <= bullet->h)
				caveDrawBitmapRegion(game, bullet, x, y, srcX, srcY, 8, 8);
			else
				dcAppDrawFill(&game->draw, x + 2, y + 2, 4, 4, caveRgb(255, 255, 160));
		}
	}
}

static bool caveDrawNpcSprite(struct CaveGame *game, const struct CaveEntity *entity, int scrollX, int scrollY)
{
	const struct CaveNpcInfo *info;
	struct CaveBitmap *sheet;
	uint32_t left = 8;
	uint32_t top = 8;
	uint32_t width = 16;
	uint32_t height = 16;
	uint32_t srcX = 0;
	int dstX;
	int dstY;

	if (caveDrawNxObjectSprite(game, entity, scrollX, scrollY))
		return true;
	if (!entity || entity->npcType >= game->npcInfoCount)
		return false;
	info = &game->npcInfo[entity->npcType];
	if (info->dispLeft || info->dispTop || info->dispRight || info->dispBottom) {
		left = info->dispLeft;
		top = info->dispTop;
		width = (uint32_t)info->dispLeft + info->dispRight;
		height = (uint32_t)info->dispTop + info->dispBottom;
	}
	if (!width || !height)
		return false;
	sheet = caveNpcSheetForId(game, info->sheetId);
	if (!sheet || !sheet->pixels || width > sheet->w || height > sheet->h)
		return false;
	if (entity->actionNum && (uint32_t)entity->actionNum * width + width <= sheet->w)
		srcX = (uint32_t)entity->actionNum * width;
	else if ((entity->flags & CAVE_FLAG_FACES_RIGHT) && width * 2u <= sheet->w)
		srcX = width;
	dstX = (int)entity->x * CAVE_TILE - (int)left - scrollX;
	dstY = (int)entity->y * CAVE_TILE - (int)top - scrollY;
	if (dstX < -(int)width || dstY < -(int)height || dstX >= (int)CAVE_SCREEN_W || dstY >= (int)CAVE_SCREEN_H)
		return true;
	caveDrawBitmapRegion(game, sheet, dstX, dstY, srcX, 0, width, height);
	return true;
}

static void caveDrawEntities(struct CaveGame *game, int scrollX, int scrollY)
{
	for (uint32_t i = 0; i < game->entityCount; i++) {
		const struct CaveEntity *entity = &game->entities[i];

		if (!entity->alive)
			continue;
		if (caveDrawNpcSprite(game, entity, scrollX, scrollY))
			continue;
	}
}

static void caveDrawWrappedText(struct CaveGame *game, int x, int y, int maxW, const char *text)
{
	char line[44];
	uint32_t lineLen = 0;
	int cy = y;

	line[0] = 0;
	while (text && *text && cy < 232) {
		char ch = *text++;

		line[lineLen] = 0;
		if (ch == '\n' || caveTextWidth(line, FontSmall) + 8u >= (uint32_t)maxW) {
			line[lineLen] = 0;
			caveDrawText(&game->draw, x, cy, line, FontSmall, caveRgb(255, 255, 255));
			lineLen = 0;
			line[0] = 0;
			cy += 12;
			if (ch == '\n')
				continue;
		}
		if (lineLen + 1u < sizeof(line))
			line[lineLen++] = ch;
	}
	if (lineLen && cy < 232) {
		line[lineLen] = 0;
		caveDrawText(&game->draw, x, cy, line, FontSmall, caveRgb(255, 255, 255));
	}
}

static void caveDrawMessageBox(struct CaveGame *game)
{
	int y;

	if (!game->script.messageOpen)
		return;
	y = game->script.messageTop ? 14 : 164;
	dcAppDrawFill(&game->draw, 12, y, 296, 62, caveRgb(8, 10, 18));
	if (game->script.messageBorder) {
		dcAppDrawFill(&game->draw, 13, y + 1, 294, 1, caveRgb(216, 216, 216));
		dcAppDrawFill(&game->draw, 13, y + 60, 294, 1, caveRgb(96, 96, 112));
		dcAppDrawFill(&game->draw, 13, y + 2, 1, 58, caveRgb(216, 216, 216));
		dcAppDrawFill(&game->draw, 306, y + 2, 1, 58, caveRgb(96, 96, 112));
	}
	if (game->script.faceId)
		caveDrawText(&game->draw, 20, y + 8, "FACE", FontSmall, caveRgb(255, 224, 160));
	if (game->script.itemId)
		caveDrawText(&game->draw, 20, y + 44, "ITEM", FontSmall, caveRgb(160, 224, 255));
	caveDrawWrappedText(game, game->script.faceId || game->script.itemId ? 56 : 20, y + 10, game->script.faceId || game->script.itemId ? 236 : 280, game->script.text);
	if (game->script.waitingForButton)
		caveDrawText(&game->draw, 276, y + 48, "OK", FontSmall, caveRgb(255, 224, 112));
}

static void caveDrawFade(struct CaveGame *game)
{
	uint8_t dark;

	if (!game->fadeMode)
		return;
	dark = game->fadeMode == 1u ? game->fadeFrames : (uint8_t)(16u - game->fadeFrames);
	if (dark >= 16u) {
		dcAppDrawFill(&game->draw, 0, 0, CAVE_SCREEN_W, CAVE_SCREEN_H, 0);
	} else if (dark) {
		for (uint32_t y = 0; y < CAVE_SCREEN_H; y += 4u)
			for (uint32_t x = 0; x < CAVE_SCREEN_W; x += 4u)
				if ((((x >> 2) * 3u + (y >> 2) * 5u) & 15u) < dark)
					dcAppDrawFill(&game->draw, (int32_t)x, (int32_t)y, 4, 4, 0);
	}
	if (game->fadeFrames)
		game->fadeFrames--;
	if (!game->fadeFrames && game->fadeMode == 1u)
		game->fadeMode = 0;
}

static void caveDraw(struct CaveGame *game)
{
	int mapW = game->map.w * CAVE_TILE;
	int mapH = game->map.h * CAVE_TILE;
	int scrollX = game->playerX - (int)CAVE_SCREEN_W / 2;
	int scrollY = game->playerY - (int)CAVE_SCREEN_H / 2;
	int shakeX = 0;
	int shakeY = 0;
	char text[72];

	if (scrollX < 0)
		scrollX = 0;
	if (scrollY < 0)
		scrollY = 0;
	if (mapW > (int)CAVE_SCREEN_W && scrollX > mapW - (int)CAVE_SCREEN_W)
		scrollX = mapW - (int)CAVE_SCREEN_W;
	if (mapH > (int)CAVE_SCREEN_H && scrollY > mapH - (int)CAVE_SCREEN_H)
		scrollY = mapH - (int)CAVE_SCREEN_H;
	if (game->quakeFrames) {
		shakeX = (game->quakeFrames & 2u) ? 2 : -2;
		shakeY = (game->quakeFrames & 1u) ? 1 : -1;
		game->quakeFrames--;
	}
	scrollX += shakeX;
	scrollY += shakeY;
	if (scrollX < 0)
		scrollX = 0;
	if (scrollY < 0)
		scrollY = 0;
	if (mapW > (int)CAVE_SCREEN_W && scrollX > mapW - (int)CAVE_SCREEN_W)
		scrollX = mapW - (int)CAVE_SCREEN_W;
	if (mapH > (int)CAVE_SCREEN_H && scrollY > mapH - (int)CAVE_SCREEN_H)
		scrollY = mapH - (int)CAVE_SCREEN_H;
	caveDrawBackdrop(game, scrollX, scrollY);
	for (uint32_t ty = 0; ty < game->map.h; ty++) {
		int dstY = (int)ty * CAVE_TILE - scrollY;

		if (dstY < -CAVE_TILE || dstY >= (int)CAVE_SCREEN_H)
			continue;
		for (uint32_t tx = 0; tx < game->map.w; tx++) {
			int dstX = (int)tx * CAVE_TILE - scrollX;

			if (dstX < -CAVE_TILE || dstX >= (int)CAVE_SCREEN_W)
				continue;
			caveDrawBitmapTile(game, dstX, dstY, game->map.tiles[ty * game->map.w + tx]);
		}
	}
	caveDrawEntities(game, scrollX, scrollY);
	caveDrawBullets(game, scrollX, scrollY);
	if (!game->playerHidden)
		caveDrawPlayer(game, game->playerX - scrollX, game->playerY - scrollY);
	snprintf(text, sizeof(text), "%s  LIFE %u/%u  STAGE %u/%u",
		game->stage.stagename[0] ? game->stage.stagename : game->stage.filename,
		(unsigned)game->life, (unsigned)game->maxLife, (unsigned)game->stageIndex, (unsigned)game->stageCount);
	caveDrawText(&game->draw, 4, 4, text, FontSmall, caveRgb(240, 240, 224));
	snprintf(text, sizeof(text), "A TALK  START SAVE  %u ENT", (unsigned)game->entityCount);
	caveDrawText(&game->draw, 4, 222, text, FontSmall, caveRgb(196, 204, 196));
	if (game->bannerFrames > 0) {
		caveDrawCentered(&game->draw, 24, game->banner, FontMedium, caveRgb(255, 255, 255));
		game->bannerFrames--;
	}
	caveDrawMessageBox(game);
	caveDrawFade(game);
	dcAppDrawPresent(&game->draw);
}

static bool caveInitData(struct CaveGame *game)
{
	bool restored;

	if (!dc32PortOpenAssetPack(game->args ? game->args->vol : NULL, CAVE_PACK_PATH, &game->pak))
		return false;
	game->pakOpen = true;
	if (!cavePackReadEntries(game))
		return false;
	game->stageDat = caveLoadFile(game, "Stage.dat", &game->stageDatSize);
	if (!game->stageDat)
		game->stageDat = caveLoadFile(game, "stage.dat", &game->stageDatSize);
	if (!game->stageDat || game->stageDatSize < 74u)
		return false;
	caveLoadTileKey(game);
	(void)caveLoadNpcTable(game);
	caveLoadCommonNpcSheets(game);
	caveLoadHeadScript(game);
	(void)caveLoadBitmap(game, "MyChar.pbm", &game->myChar);
	game->playerX = -1;
	game->playerY = -1;
	game->maxLife = 3;
	game->life = 3;
	caveBitSet(game->weaponFlags, CAVE_WEAPON_COUNT, 2u, true);
	restored = caveRestoreSave(game);
	if (!restored) {
		game->stageIndex = CAVE_STAGE_START_POINT;
		caveSetPlayerPos(game, 64, 64);
	} else {
		caveSetPlayerPos(game, game->playerX, game->playerY);
	}
	game->playerFacingRight = true;
	if (!caveLoadStage(game, game->stageIndex))
		return caveLoadStage(game, 0);
	if (!restored)
		(void)caveStartScript(game, CAVE_STAGE_START_EVENT);
	return true;
}

static bool cavePlayerNearEntity(struct CaveGame *game, const struct CaveEntity *entity, int range)
{
	int ex = (int)entity->x * CAVE_TILE + 8;
	int ey = (int)entity->y * CAVE_TILE + 8;
	int px = game->playerX + 8;
	int py = game->playerY + 8;
	int dx = ex > px ? ex - px : px - ex;
	int dy = ey > py ? ey - py : py - ey;

	return dx <= range && dy <= range;
}

static bool caveActivateNearbyEntity(struct CaveGame *game)
{
	for (uint32_t i = 0; i < game->entityCount; i++) {
		struct CaveEntity *entity = &game->entities[i];

		if (!entity->alive || !entity->eventNum)
			continue;
		if (!(entity->flags & CAVE_FLAG_SCRIPT_ON_ACTIVATE) && !cavePlayerNearEntity(game, entity, 10))
			continue;
		if (!cavePlayerNearEntity(game, entity, 24))
			continue;
		if (caveStartScript(game, entity->eventNum))
			return true;
	}
	return false;
}

static void caveCheckTouchScripts(struct CaveGame *game)
{
	if (game->script.running)
		return;
	for (uint32_t i = 0; i < game->entityCount; i++) {
		struct CaveEntity *entity = &game->entities[i];

		if (!entity->alive || !entity->eventNum || !(entity->flags & CAVE_FLAG_SCRIPT_ON_TOUCH))
			continue;
		if (cavePlayerNearEntity(game, entity, 12)) {
			(void)caveStartScript(game, entity->eventNum);
			return;
		}
	}
}

static void caveHandleInput(struct CaveGame *game)
{
	if (game->draw.pressed & UI_KEY_BIT_CENTER) {
		game->running = false;
		return;
	}
	caveScriptStep(game);
	caveTickBullets(game);
	if (game->script.running && (game->script.controlsLocked || game->script.waitingForButton))
		return;
	game->playerAimUp = (game->draw.keys & KEY_BIT_UP) != 0;
	game->playerAimDown = (game->draw.keys & KEY_BIT_DOWN) != 0 && !game->playerOnGround;
	if (game->draw.pressed & KEY_BIT_A) {
		if (caveActivateNearbyEntity(game))
			return;
	}
	if (game->draw.pressed & KEY_BIT_B)
		caveFirePolarStar(game);
	caveApplyPlayerPhysics(game);
	if (game->draw.pressed & KEY_BIT_START) {
		caveSaveGame(game);
		caveBanner(game, "SAVED");
	}
	caveCheckTouchScripts(game);
}

int caveAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct CaveGame *game;

	mCaveAbort = false;
	dc32PortHeapInitDefault();
	game = dc32PortCalloc(1, sizeof(*game));
	if (!game)
		return -1;
	game->frame = mCaveFrame;
	game->host = host;
	game->args = args;
	if (!dcAppDrawInit(&game->draw, host, args, game->frame, CAVE_SCREEN_W, CAVE_SCREEN_H))
		return -1;
	if (!caveInitData(game)) {
		dc32PortShowMissingData(host, args, "Cave Story data", CAVE_PACK_PATH, game->frame);
		return 0;
	}
	dcAppDrawWaitRelease(&game->draw, KEY_BIT_A | KEY_BIT_B | KEY_BIT_START | KEY_BIT_SEL | UI_KEY_BIT_CENTER);
	dispSetFramerate(60);
	game->running = true;
	while (game->running && !mCaveAbort) {
		caveDraw(game);
		if (!dcAppDrawFrame(&game->draw, UI_KEY_BIT_CENTER))
			break;
		caveHandleInput(game);
	}
	caveSaveGame(game);
	caveFreeAllScripts(game);
	caveFreeEntities(game);
	caveFreeBitmap(&game->tileset);
	caveFreeBitmap(&game->backdrop);
	caveFreeBitmap(&game->myChar);
	caveFreeBitmap(&game->npc0);
	caveFreeBitmap(&game->npcSym);
	caveFreeBitmap(&game->npcRegu);
	caveFreeBitmap(&game->npcStage1);
	caveFreeBitmap(&game->npcStage2);
	caveFreeBitmap(&game->bulletSheet);
	caveFreeBitmap(&game->nxTempSheet);
	dc32PortFree(game->map.tiles);
	dc32PortFree(game->stageDat);
	dc32PortFree(game->entries);
	dc32PortFree(game->entryNames);
	dc32PortFree(game->npcInfo);
	if (game->pakOpen)
		dc32PortCloseAssetPack(&game->pak);
	dcAppDrawWaitRelease(&game->draw, UI_KEY_BIT_CENTER);
	return 0;
}

void caveAppAbort(void)
{
	mCaveAbort = true;
}

int dcAppEntry(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	return caveAppRun(host, args);
}

void dcAppAbort(void)
{
	caveAppAbort();
}

void dcAppRefreshDisplayOptions(void)
{
}
