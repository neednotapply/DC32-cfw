#include "apps/chips/chips_port.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "apps/port/port_runtime.h"
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "ui.h"
#include "defs.h"
#include "play.h"
#include "state.h"
#include "tworld_assets.h"

#define CHIPS_SCREEN_W DC32_PORT_SCREEN_W
#define CHIPS_SCREEN_H DC32_PORT_SCREEN_H
#define CHIPS_PACK_PATH "/APPS/chips.pak"
#define CHIPS_PACK_MAGIC "DC32CHIPSPK"
#define CHIPS_PACK_MAGIC_LEN 11u
#define CHIPS_PACK_VERSION 1u
#define CHIPS_PACK_HEADER_SIZE 20u
#define CHIPS_PACK_ENTRY_SIZE 16u
#define CHIPS_DAT_MAGIC 0x0002AAACu
#define CHIPS_TILE_PACK_MAGIC "DC32CHIPTIL"
#define CHIPS_TILE_PACK_MAGIC_LEN 11u
#define CHIPS_TILE_PACK_VERSION 1u
#define CHIPS_TILE_PACK_HEADER_SIZE 20u
#define CHIPS_SAVE_MAGIC 0x50494843u
#define CHIPS_SAVE_VERSION 1u
#define CHIPS_VIEW_TILES 9u
#define CHIPS_BOARD_X 4
#define CHIPS_BOARD_Y 12
#define CHIPS_PANEL_X 228
#define CHIPS_TICK_FRAME_DIVIDER 3u

struct ChipsPackEntry {
	uint32_t offset;
	uint32_t size;
	uint32_t crc32;
};

struct ChipsLevelIndex {
	uint32_t offset;
	uint16_t size;
};

struct ChipsSave {
	uint32_t magic;
	uint16_t version;
	uint16_t size;
	uint16_t level;
	uint16_t reserved;
};

struct ChipsGame {
	const struct DcAppHostApi *host;
	const struct DcAppRunArgs *args;
	struct DcAppDrawCtx draw;
	uint8_t *frame;
	struct ChipsPackEntry chipsDatEntry;
	struct ChipsLevelIndex *levelIndex;
	uint8_t *levelData;
	uint32_t levelDataCapacity;
	uint8_t *tilePack;
	const uint8_t *tilePixels;
	const uint8_t *tileAlpha;
	gamesetup currentSetup;
	uint16_t levelCount;
	uint16_t currentLevel;
	uint8_t tickFrame;
	uint8_t bannerFrames;
	bool active;
	bool running;
	char banner[32];
};

static volatile bool mChipsAbort;

extern void chipsTworldResetTimer(void);
extern void chipsTworldAdvanceTick(void);
extern int batchmode;

static uint16_t chipsRgb565(uint32_t r, uint32_t g, uint32_t b)
{
	return dcAppDrawRgb565(r, g, b);
}

static uint16_t chipsRd16(const uint8_t *ptr)
{
	return (uint16_t)ptr[0] | (uint16_t)((uint16_t)ptr[1] << 8);
}

static uint32_t chipsRd32(const uint8_t *ptr)
{
	return (uint32_t)ptr[0] | ((uint32_t)ptr[1] << 8) |
		((uint32_t)ptr[2] << 16) | ((uint32_t)ptr[3] << 24);
}

static uint32_t chipsTextWidth(const char *text, enum Font font)
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

static void chipsDrawText(struct DcAppDrawCtx *draw, int32_t x, int32_t y,
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

static void chipsDrawCenteredText(struct DcAppDrawCtx *draw, int32_t y,
	const char *text, enum Font font, uint16_t color)
{
	uint32_t width = chipsTextWidth(text, font);
	int32_t x = (int32_t)((CHIPS_SCREEN_W > width ? CHIPS_SCREEN_W - width : 0u) / 2u);

	chipsDrawText(draw, x, y, text, font, color);
}

static bool chipsPackFind(struct Dc32PortPak *pak, const char *wanted, struct ChipsPackEntry *entry)
{
	uint8_t header[CHIPS_PACK_HEADER_SIZE];
	uint32_t count;
	uint32_t table;

	if (!pak || !wanted || !entry ||
			!dc32PortReadAssetPack(pak, 0, header, sizeof(header)) ||
			memcmp(header, CHIPS_PACK_MAGIC, CHIPS_PACK_MAGIC_LEN) != 0 ||
			chipsRd32(header + 12) != CHIPS_PACK_VERSION)
		return false;
	count = chipsRd32(header + 16);
	table = CHIPS_PACK_HEADER_SIZE;
	for (uint32_t i = 0; i < count; i++) {
		uint8_t raw[CHIPS_PACK_ENTRY_SIZE];
		char name[48];
		uint16_t nameLen;

		if (table > pak->size || pak->size - table < CHIPS_PACK_ENTRY_SIZE ||
				!dc32PortReadAssetPack(pak, table, raw, sizeof(raw)))
			return false;
		nameLen = chipsRd16(raw);
		table += CHIPS_PACK_ENTRY_SIZE;
		if (nameLen >= sizeof(name) || table > pak->size || pak->size - table < nameLen)
			return false;
		if (!dc32PortReadAssetPack(pak, table, name, nameLen))
			return false;
		name[nameLen] = 0;
		if (strcmp(name, wanted) == 0) {
			entry->offset = chipsRd32(raw + 4);
			entry->size = chipsRd32(raw + 8);
			entry->crc32 = chipsRd32(raw + 12);
			return entry->offset <= pak->size && entry->size <= pak->size - entry->offset;
		}
		table += nameLen;
	}
	return false;
}

static bool chipsReadPackEntry(struct Dc32PortPak *pak, const struct ChipsPackEntry *entry,
	uint32_t offset, void *dst, uint32_t size)
{
	uint32_t absolute;

	if (!pak || !entry || !dst)
		return false;
	if (offset > entry->size || size > entry->size - offset)
		return false;
	absolute = entry->offset + offset;
	if (absolute < entry->offset)
		return false;
	return dc32PortReadAssetPack(pak, absolute, dst, size);
}

static bool chipsReadPackEntryFromVol(struct FatfsVol *vol, const struct ChipsPackEntry *entry,
	uint32_t offset, void *dst, uint32_t size)
{
	struct Dc32PortPak pak;
	bool ok;

	if (!vol || !entry || !dst)
		return false;
	if (!dc32PortOpenAssetPack(vol, CHIPS_PACK_PATH, &pak))
		return false;
	ok = chipsReadPackEntry(&pak, entry, offset, dst, size);
	dc32PortCloseAssetPack(&pak);
	return ok;
}

static bool chipsReadPackFile(struct FatfsVol *vol, const char *name, uint8_t **data, uint32_t *size)
{
	struct Dc32PortPak pak;
	struct ChipsPackEntry entry;
	uint8_t *buf;
	bool ok = false;

	if (!vol || !name || !data || !size)
		return false;
	*data = NULL;
	*size = 0;
	if (!dc32PortOpenAssetPack(vol, CHIPS_PACK_PATH, &pak))
		return false;
	if (!chipsPackFind(&pak, name, &entry))
		goto out;
	buf = dc32PortMalloc(entry.size);
	if (!buf)
		goto out;
	if (!chipsReadPackEntry(&pak, &entry, 0, buf, entry.size)) {
		dc32PortFree(buf);
		goto out;
	}
	*data = buf;
	*size = entry.size;
	ok = true;
out:
	dc32PortCloseAssetPack(&pak);
	return ok;
}

static bool chipsParseLevelMetadata(gamesetup *level, uint8_t *data, uint16_t size)
{
	const uint8_t *ptr;
	const uint8_t *end;
	uint16_t upperSize;
	uint16_t lowerSize;
	uint16_t metaSize;

	if (!level || !data || size < 10)
		return false;
	memset(level, 0, sizeof(*level));
	level->number = chipsRd16(data);
	level->time = chipsRd16(data + 2);
	level->besttime = TIME_NIL;
	level->sgflags = 0;
	level->levelsize = size;
	level->leveldata = data;
	level->solutionsize = 0;
	level->solutiondata = NULL;
	level->levelhash = 0;
	level->unsolvable = NULL;
	snprintf(level->name, sizeof(level->name), "Level %d", level->number);
	level->passwd[0] = 0;

	upperSize = chipsRd16(data + 8);
	end = data + size;
	ptr = data + 10u + upperSize;
	if (ptr + 2u > end)
		return false;
	lowerSize = chipsRd16(ptr);
	ptr += 2u + lowerSize;
	if (ptr + 2u > end)
		return false;
	metaSize = chipsRd16(ptr);
	ptr += 2u;
	if (ptr + metaSize > end)
		metaSize = (uint16_t)(end - ptr);

	while (ptr + 2u <= end && ptr < data + size) {
		uint8_t field = ptr[0];
		uint8_t len = ptr[1];

		ptr += 2;
		if (ptr + len > end)
			len = (uint8_t)(end - ptr);
		switch (field) {
			case 1:
				if (len >= 2)
					level->time = chipsRd16(ptr);
				break;
			case 3:
				if (len >= sizeof(level->name))
					len = sizeof(level->name) - 1u;
				memcpy(level->name, ptr, len);
				level->name[len] = 0;
				break;
			case 6:
				for (uint8_t i = 0; i < len && i < 15u && ptr[i]; i++) {
					level->passwd[i] = (char)(ptr[i] ^ 0x99u);
					level->passwd[i + 1u] = 0;
				}
				break;
			default:
				break;
		}
		ptr += len;
	}
	(void)metaSize;
	return true;
}

static bool chipsLoadDatIndex(struct ChipsGame *game)
{
	struct Dc32PortPak pak;
	struct ChipsPackEntry entry;
	struct ChipsLevelIndex *index;
	uint8_t header[6];
	uint32_t offset = 6;
	uint32_t magic;
	uint16_t count;
	bool ok = false;

	if (!game || !game->args || !game->args->vol)
		return false;
	if (!dc32PortOpenAssetPack(game->args->vol, CHIPS_PACK_PATH, &pak))
		return false;
	if (!chipsPackFind(&pak, "chips.dat", &entry) ||
			!chipsReadPackEntry(&pak, &entry, 0, header, sizeof(header)))
		goto out;
	if (entry.size < sizeof(header))
		goto out;
	magic = chipsRd32(header);
	count = chipsRd16(header + 4);
	if (magic != CHIPS_DAT_MAGIC || !count || count > 512u)
		goto out;
	index = dc32PortCalloc(count, sizeof(*index));
	if (!index)
		goto out;
	for (uint16_t i = 0; i < count; i++) {
		uint8_t rawSize[2];
		uint16_t levelSize;

		if (!chipsReadPackEntry(&pak, &entry, offset, rawSize, sizeof(rawSize))) {
			dc32PortFree(index);
			goto out;
		}
		offset += 2u;
		levelSize = chipsRd16(rawSize);
		if (!levelSize || levelSize > entry.size - offset) {
			dc32PortFree(index);
			goto out;
		}
		index[i].offset = offset;
		index[i].size = levelSize;
		offset += levelSize;
	}
	if (offset != entry.size) {
		dc32PortFree(index);
		goto out;
	}
	game->chipsDatEntry = entry;
	game->levelIndex = index;
	game->levelCount = count;
	ok = true;
out:
	dc32PortCloseAssetPack(&pak);
	return ok;
}

static bool chipsLoadCurrentLevel(struct ChipsGame *game)
{
	const struct ChipsLevelIndex *level;
	uint8_t *data;

	if (!game || !game->args || !game->args->vol ||
			!game->levelIndex || game->currentLevel >= game->levelCount)
		return false;
	level = &game->levelIndex[game->currentLevel];
	if (game->levelDataCapacity < level->size) {
		data = dc32PortMalloc(level->size);
		if (!data)
			return false;
		dc32PortFree(game->levelData);
		game->levelData = data;
		game->levelDataCapacity = level->size;
	}
	if (!chipsReadPackEntryFromVol(game->args->vol, &game->chipsDatEntry,
			level->offset, game->levelData, level->size))
		return false;
	return chipsParseLevelMetadata(&game->currentSetup, game->levelData, level->size);
}

static bool chipsLoadTilePack(struct ChipsGame *game)
{
	uint8_t *data = NULL;
	uint32_t size = 0;
	uint16_t version;
	uint16_t tileCount;
	uint16_t tileSize;
	uint16_t alphaBytes;
	uint32_t pixelBytes;
	uint32_t alphaOffset;

	if (!game || !game->args || !chipsReadPackFile(game->args->vol, "tiles.bin", &data, &size))
		return true;
	if (size < CHIPS_TILE_PACK_HEADER_SIZE ||
			memcmp(data, CHIPS_TILE_PACK_MAGIC, CHIPS_TILE_PACK_MAGIC_LEN) != 0) {
		dc32PortFree(data);
		return false;
	}
	version = chipsRd16(data + 12u);
	tileCount = chipsRd16(data + 14u);
	tileSize = chipsRd16(data + 16u);
	alphaBytes = chipsRd16(data + 18u);
	if (version != CHIPS_TILE_PACK_VERSION || tileCount != TWORLD_TILE_COUNT ||
			tileSize != TWORLD_TILE_SIZE || alphaBytes != TWORLD_TILE_ALPHA_BYTES) {
		dc32PortFree(data);
		return false;
	}
	pixelBytes = (uint32_t)tileCount * (uint32_t)tileSize * (uint32_t)tileSize;
	alphaOffset = CHIPS_TILE_PACK_HEADER_SIZE + pixelBytes;
	if (alphaOffset > size || (uint32_t)tileCount * alphaBytes > size - alphaOffset) {
		dc32PortFree(data);
		return false;
	}
	game->tilePack = data;
	game->tilePixels = data + CHIPS_TILE_PACK_HEADER_SIZE;
	game->tileAlpha = data + alphaOffset;
	return true;
}

static void chipsSaveGame(struct ChipsGame *game)
{
	struct ChipsSave save;

	if (!game || !game->args || !game->args->vol)
		return;
	memset(&save, 0, sizeof(save));
	save.magic = CHIPS_SAVE_MAGIC;
	save.version = CHIPS_SAVE_VERSION;
	save.size = sizeof(save);
	save.level = game->currentLevel;
	save.reserved = 0;
	(void)dc32PortSaveWrite(game->args->vol, "chips", &save, sizeof(save));
}

static void chipsRestoreSave(struct ChipsGame *game)
{
	struct ChipsSave save;

	game->currentLevel = 0;
	if (!game || !game->args || !game->args->vol ||
			!dc32PortSaveRead(game->args->vol, "chips", &save, sizeof(save)) ||
			save.magic != CHIPS_SAVE_MAGIC || save.version != CHIPS_SAVE_VERSION ||
			save.size != sizeof(save))
		return;
	if (save.level < game->levelCount)
		game->currentLevel = save.level;
}

static bool chipsStartLevel(struct ChipsGame *game)
{
	if (!game || !game->levelIndex || game->currentLevel >= game->levelCount)
		return false;
	if (game->active) {
		(void)endgamestate();
		game->active = false;
	}
	if (!chipsLoadCurrentLevel(game))
		return false;
	chipsTworldResetTimer();
	game->tickFrame = 0;
	game->active = initgamestate(&game->currentSetup, Ruleset_MS) != 0;
	return game->active;
}

static void chipsBanner(struct ChipsGame *game, const char *text)
{
	if (!game)
		return;
	snprintf(game->banner, sizeof(game->banner), "%s", text ? text : "");
	game->bannerFrames = 45;
}

static int chipsCommandFromKeys(uint_fast16_t keys)
{
	int cmd = CmdNone;

	if (keys & KEY_BIT_UP)
		cmd |= CmdNorth;
	if (keys & KEY_BIT_DOWN)
		cmd |= CmdSouth;
	if (keys & KEY_BIT_LEFT)
		cmd |= CmdWest;
	if (keys & KEY_BIT_RIGHT)
		cmd |= CmdEast;
	return cmd;
}

static void chipsDrawTile(struct ChipsGame *game, uint8_t tileId, int32_t dstX, int32_t dstY)
{
	struct DcAppDrawCtx *draw = &game->draw;
	const uint8_t *pixels;
	const uint8_t *alpha;

	if (tileId >= TWORLD_TILE_COUNT)
		tileId = Empty;
	if (game->tilePixels && game->tileAlpha) {
		pixels = game->tilePixels + (uint32_t)tileId * TWORLD_TILE_SIZE * TWORLD_TILE_SIZE;
		alpha = game->tileAlpha + (uint32_t)tileId * TWORLD_TILE_ALPHA_BYTES;
	} else {
		pixels = tworldTilePixels[tileId];
		alpha = tworldTileAlpha[tileId];
	}
	for (uint32_t y = 0; y < TWORLD_TILE_SIZE; y++) {
		for (uint32_t x = 0; x < TWORLD_TILE_SIZE; x++) {
			uint32_t idx = y * TWORLD_TILE_SIZE + x;

			if (!(alpha[idx / 8u] & (1u << (idx % 8u))))
				continue;
			if (dstX + (int32_t)x >= 0 && dstY + (int32_t)y >= 0 &&
					dstX + (int32_t)x < (int32_t)draw->w && dstY + (int32_t)y < (int32_t)draw->h)
				draw->fb[((uint32_t)dstY + y) * draw->w + (uint32_t)dstX + x] = pixels[idx];
		}
	}
}

static void chipsDrawCell(struct ChipsGame *game, const gamestate *state, int32_t cellX, int32_t cellY,
	int32_t dstX, int32_t dstY)
{
	mapcell const *cell;
	uint8_t bot;
	uint8_t top;

	if (!state || cellX < 0 || cellY < 0 || cellX >= CXGRID || cellY >= CYGRID) {
		chipsDrawTile(game, Empty, dstX, dstY);
		return;
	}
	cell = &state->map[(uint32_t)cellY * CXGRID + (uint32_t)cellX];
	bot = cell->bot.id ? cell->bot.id : Empty;
	top = cell->top.id;
	chipsDrawTile(game, bot, dstX, dstY);
	if (top && top != Empty)
		chipsDrawTile(game, top, dstX, dstY);
}

static void chipsDrawInventorySlot(struct ChipsGame *game, int32_t x, int32_t y)
{
	struct DcAppDrawCtx *draw = &game->draw;
	uint16_t fill = chipsRgb565(4, 7, 12);
	uint16_t edge = chipsRgb565(56, 68, 86);

	dcAppDrawFill(draw, x, y, TWORLD_TILE_SIZE, TWORLD_TILE_SIZE, fill);
	dcAppDrawLine(draw, x, y, x + TWORLD_TILE_SIZE - 1, y, edge);
	dcAppDrawLine(draw, x, y, x, y + TWORLD_TILE_SIZE - 1, edge);
	dcAppDrawLine(draw, x + TWORLD_TILE_SIZE - 1, y, x + TWORLD_TILE_SIZE - 1,
		y + TWORLD_TILE_SIZE - 1, edge);
	dcAppDrawLine(draw, x, y + TWORLD_TILE_SIZE - 1, x + TWORLD_TILE_SIZE - 1,
		y + TWORLD_TILE_SIZE - 1, edge);
}

static void chipsDrawInventoryTile(struct ChipsGame *game, uint8_t tileId, int count,
	int32_t x, int32_t y, bool countKeys)
{
	char text[8];

	chipsDrawInventorySlot(game, x, y);
	if (count <= 0)
		return;
	chipsDrawTile(game, tileId, x, y);
	if (countKeys && count > 1) {
		snprintf(text, sizeof(text), "%d", count);
		dcAppDrawFill(&game->draw, x + 12, y + 13, 12, 11, chipsRgb565(4, 7, 12));
		chipsDrawText(&game->draw, x + 15, y + 16, text, FontSmall, chipsRgb565(244, 248, 255));
	}
}

static void chipsDrawInventory(struct ChipsGame *game, const gamestate *state)
{
	chipsDrawInventoryTile(game, Key_Red, redkeys(state), CHIPS_PANEL_X, 140, true);
	chipsDrawInventoryTile(game, Key_Blue, bluekeys(state), CHIPS_PANEL_X + 48, 140, true);
	chipsDrawInventoryTile(game, Key_Yellow, yellowkeys(state), CHIPS_PANEL_X, 166, true);
	chipsDrawInventoryTile(game, Key_Green, greenkeys(state), CHIPS_PANEL_X + 48, 166, true);
	chipsDrawInventoryTile(game, Boots_Ice, iceboots(state), CHIPS_PANEL_X - 4, 196, false);
	chipsDrawInventoryTile(game, Boots_Slide, slideboots(state), CHIPS_PANEL_X + 20, 196, false);
	chipsDrawInventoryTile(game, Boots_Fire, fireboots(state), CHIPS_PANEL_X + 44, 196, false);
	chipsDrawInventoryTile(game, Boots_Water, waterboots(state), CHIPS_PANEL_X + 68, 196, false);
}

static void chipsDrawPanel(struct ChipsGame *game, const gamestate *state)
{
	struct DcAppDrawCtx *draw = &game->draw;
	char text[48];
	const gamesetup *level = state ? state->game : &game->currentSetup;
	int timeLeft = TIME_NIL;
	uint16_t white = chipsRgb565(244, 248, 255);
	uint16_t dim = chipsRgb565(148, 164, 184);
	uint16_t accent = chipsRgb565(255, 214, 96);

	dcAppDrawFill(draw, CHIPS_PANEL_X - 6, 0, CHIPS_SCREEN_W - CHIPS_PANEL_X + 6, CHIPS_SCREEN_H,
		chipsRgb565(10, 14, 22));
	snprintf(text, sizeof(text), "LEVEL %u", game->currentLevel + 1u);
	chipsDrawText(draw, CHIPS_PANEL_X, 18, text, FontMedium, accent);
	if (level && level->name[0])
		chipsDrawText(draw, CHIPS_PANEL_X, 44, level->name, FontSmall, dim);
	if (state && level && level->time) {
		int elapsed = (state->currenttime + state->timeoffset) / TICKS_PER_SECOND;

		timeLeft = level->time > elapsed ? level->time - elapsed : 0;
	}
	if (timeLeft != TIME_NIL) {
		snprintf(text, sizeof(text), "TIME %d", timeLeft);
		chipsDrawText(draw, CHIPS_PANEL_X, 82, text, FontMedium, white);
	}
	if (state) {
		snprintf(text, sizeof(text), "CHIPS %d", state->chipsneeded);
		chipsDrawText(draw, CHIPS_PANEL_X, 110, text, FontMedium, white);
		chipsDrawInventory(game, state);
	}
	chipsDrawText(draw, CHIPS_PANEL_X, 226, "START RESET", FontSmall, dim);
}

static void chipsDraw(struct ChipsGame *game)
{
	const gamestate *state = game->active ? tworldcurrentstate() : NULL;
	int32_t centerX = state ? state->xviewpos / 8 : 16;
	int32_t centerY = state ? state->yviewpos / 8 : 16;
	int32_t left = centerX - 4;
	int32_t top = centerY - 4;

	dcAppDrawClear(&game->draw, chipsRgb565(6, 9, 14));
	for (uint32_t y = 0; y < CHIPS_VIEW_TILES; y++) {
		for (uint32_t x = 0; x < CHIPS_VIEW_TILES; x++) {
			chipsDrawCell(game, state, left + (int32_t)x, top + (int32_t)y,
				CHIPS_BOARD_X + (int32_t)(x * TWORLD_TILE_SIZE),
				CHIPS_BOARD_Y + (int32_t)(y * TWORLD_TILE_SIZE));
		}
	}
	dcAppDrawLine(&game->draw, CHIPS_BOARD_X - 1, CHIPS_BOARD_Y - 1,
		CHIPS_BOARD_X + (int32_t)(CHIPS_VIEW_TILES * TWORLD_TILE_SIZE), CHIPS_BOARD_Y - 1,
		chipsRgb565(96, 111, 135));
	dcAppDrawLine(&game->draw, CHIPS_BOARD_X - 1, CHIPS_BOARD_Y - 1,
		CHIPS_BOARD_X - 1, CHIPS_BOARD_Y + (int32_t)(CHIPS_VIEW_TILES * TWORLD_TILE_SIZE),
		chipsRgb565(96, 111, 135));
	chipsDrawPanel(game, state);
	if (game->bannerFrames) {
		dcAppDrawFill(&game->draw, 56, 96, 160, 40, chipsRgb565(8, 16, 32));
		chipsDrawCenteredText(&game->draw, 111, game->banner, FontMedium, chipsRgb565(255, 214, 96));
		game->bannerFrames--;
	}
}

static void chipsHandleLevelResult(struct ChipsGame *game, int result)
{
	if (result > 0) {
		(void)endgamestate();
		game->active = false;
		if (game->currentLevel + 1u < game->levelCount)
			game->currentLevel++;
		chipsSaveGame(game);
		(void)chipsStartLevel(game);
		chipsBanner(game, "LEVEL COMPLETE");
	}
	else if (result < 0) {
		(void)endgamestate();
		game->active = false;
		(void)chipsStartLevel(game);
		chipsBanner(game, "TRY AGAIN");
	}
}

static void chipsHandleInput(struct ChipsGame *game)
{
	if (game->draw.pressed & KEY_BIT_START) {
		(void)chipsStartLevel(game);
		chipsBanner(game, "RESET");
		return;
	}
	if (game->draw.pressed & KEY_BIT_A) {
		if (game->currentLevel + 1u < game->levelCount)
			game->currentLevel++;
		(void)chipsStartLevel(game);
		chipsSaveGame(game);
		return;
	}
	if (game->draw.pressed & KEY_BIT_B) {
		if (game->currentLevel)
			game->currentLevel--;
		(void)chipsStartLevel(game);
		chipsSaveGame(game);
		return;
	}
	if (!game->active)
		return;
	game->tickFrame++;
	if (game->tickFrame >= CHIPS_TICK_FRAME_DIVIDER) {
		int result;

		game->tickFrame = 0;
		chipsTworldAdvanceTick();
		result = doturn(chipsCommandFromKeys(game->draw.keys));
		if (result)
			chipsHandleLevelResult(game, result);
	}
}

int chipsAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct ChipsGame *game;

	mChipsAbort = false;
	dc32PortHeapInitDefault();
	game = dc32PortCalloc(1, sizeof(*game));
	if (!game)
		return -1;
	game->frame = dc32PortMalloc(CHIPS_SCREEN_W * CHIPS_SCREEN_H);
	if (!game->frame)
		return -1;
	game->host = host;
	game->args = args;
	if (!dcAppDrawInit(&game->draw, host, args, game->frame, CHIPS_SCREEN_W, CHIPS_SCREEN_H))
		return -1;
	if (!args || !args->vol || !chipsLoadDatIndex(game)) {
		dc32PortShowMissingData(host, args, "Chip's Challenge data", CHIPS_PACK_PATH, game->frame);
		return 0;
	}
	if (!chipsLoadTilePack(game)) {
		dc32PortShowMissingData(host, args, "Invalid Chip's tile pack", CHIPS_PACK_PATH, game->frame);
		return 0;
	}
	batchmode = TRUE;
	chipsRestoreSave(game);
	if (!chipsStartLevel(game)) {
		dc32PortShowMissingData(host, args, "Invalid chips.pak", CHIPS_PACK_PATH, game->frame);
		return 0;
	}
	dcAppDrawWaitRelease(&game->draw, KEY_BIT_A | KEY_BIT_B | KEY_BIT_START | UI_KEY_BIT_CENTER);
	dispSetFramerate(60);
	game->running = true;
	while (game->running && !mChipsAbort) {
		chipsDraw(game);
		if (!dcAppDrawFrame(&game->draw, UI_KEY_BIT_CENTER))
			break;
		chipsHandleInput(game);
	}
	if (game->active)
		(void)endgamestate();
	chipsSaveGame(game);
	dc32PortFree(game->levelData);
	dc32PortFree(game->levelIndex);
	dc32PortFree(game->tilePack);
	dcAppDrawWaitRelease(&game->draw, UI_KEY_BIT_CENTER);
	return 0;
}

void chipsAppAbort(void)
{
	mChipsAbort = true;
}
