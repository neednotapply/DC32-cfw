#include "apps/sokoban/sokoban_port.h"

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
#include "xsokoban_assets.h"

#define SOK_SAVE_MAGIC 0x444b4f53u
#define SOK_SAVE_VERSION 1u
#define SOK_UNDO_DEPTH 64u
#define SOK_SCREEN_W DC32_PORT_SCREEN_W
#define SOK_SCREEN_H DC32_PORT_SCREEN_H
#define SOK_HUD_H 28u

enum SokAction {
	SokActionNone,
	SokActionMove,
	SokActionPush,
	SokActionSave,
	SokActionUnsave,
	SokActionStoreMove,
	SokActionStorePush,
};

struct SokPos {
	uint8_t row;
	uint8_t col;
};

struct SokSnapshot {
	uint32_t moves;
	uint32_t pushes;
	uint8_t level;
	uint8_t rows;
	uint8_t cols;
	uint8_t savepack;
	uint8_t packets;
	struct SokPos player;
	char map[XSOKOBAN_MAX_ROWS * XSOKOBAN_MAX_COLS];
};

struct SokSave {
	uint32_t magic;
	uint16_t version;
	uint16_t size;
	struct SokSnapshot state;
	bool solved;
	uint8_t reserved[3];
};

struct SokGame {
	const struct DcAppHostApi *host;
	const struct DcAppRunArgs *args;
	struct DcAppDrawCtx draw;
	uint8_t *frame;
	struct SokSnapshot state;
	struct SokSnapshot undo[SOK_UNDO_DEPTH];
	uint8_t undoHead;
	uint8_t undoCount;
	uint8_t repeatDelay;
	uint_fast16_t repeatKey;
	bool running;
	bool solved;
};

static volatile bool mSokobanAbort;

static uint8_t sokRgb332(uint32_t r, uint32_t g, uint32_t b)
{
	return (uint8_t)(r & 0xe0u) | (uint8_t)((g >> 3) & 0x1cu) | (uint8_t)(b >> 6);
}

static uint16_t sokRgb565(uint32_t r, uint32_t g, uint32_t b)
{
	return dcAppDrawRgb565(r, g, b);
}

static char *sokCell(struct SokSnapshot *state, uint8_t row, uint8_t col)
{
	return &state->map[(uint32_t)row * XSOKOBAN_MAX_COLS + col];
}

static char sokCellAt(const struct SokSnapshot *state, int32_t row, int32_t col)
{
	if (row < 0 || col < 0 || row >= state->rows || col >= state->cols)
		return '#';
	return state->map[(uint32_t)row * XSOKOBAN_MAX_COLS + (uint32_t)col];
}

static bool sokIsClear(char ch)
{
	return ch == ' ' || ch == '.';
}

static bool sokIsPacket(char ch)
{
	return ch == '$' || ch == '*';
}

static void sokLoadLevel(struct SokGame *game, uint8_t level)
{
	const struct XsokobanLevel *src = &xsokobanLevels[level];
	struct SokSnapshot *state = &game->state;

	memset(state, 0, sizeof(*state));
	state->level = level;
	state->rows = src->rows;
	state->cols = src->cols;
	memset(state->map, ' ', sizeof(state->map));
	for (uint8_t row = 0; row < src->rows; row++) {
		for (uint8_t col = 0; col < src->cols; col++) {
			char ch = src->cells[(uint32_t)row * XSOKOBAN_MAX_COLS + col];

			*sokCell(state, row, col) = ch;
			if (ch == '@' || ch == '+') {
				state->player.row = row;
				state->player.col = col;
			}
			if (ch == '$' || ch == '*')
				state->packets++;
			if (ch == '*')
				state->savepack++;
		}
	}
	game->undoHead = 0;
	game->undoCount = 0;
	game->repeatDelay = 0;
	game->repeatKey = 0;
	game->solved = state->packets && state->savepack == state->packets;
}

static bool sokValidateSnapshot(const struct SokSnapshot *state)
{
	uint8_t players = 0, packets = 0, savepack = 0;

	if (!state || state->level >= XSOKOBAN_LEVEL_COUNT ||
			state->rows == 0 || state->rows > XSOKOBAN_MAX_ROWS ||
			state->cols == 0 || state->cols > XSOKOBAN_MAX_COLS)
		return false;
	for (uint8_t row = 0; row < state->rows; row++) {
		for (uint8_t col = 0; col < state->cols; col++) {
			char ch = sokCellAt(state, row, col);

			switch (ch) {
				case '@':
				case '+':
					players++;
					break;
				case '$':
					packets++;
					break;
				case '*':
					packets++;
					savepack++;
					break;
				case '.':
				case ' ':
				case '#':
					break;
				default:
					return false;
			}
		}
	}
	return players == 1 && packets == state->packets && savepack == state->savepack;
}

static void sokSaveGame(struct SokGame *game)
{
	struct SokSave save;

	if (!game || !game->args || !game->args->vol)
		return;
	memset(&save, 0, sizeof(save));
	save.magic = SOK_SAVE_MAGIC;
	save.version = SOK_SAVE_VERSION;
	save.size = sizeof(save);
	save.state = game->state;
	save.solved = game->solved;
	(void)dc32PortSaveWrite(game->args->vol, "sokoban", &save, sizeof(save));
}

static void sokRestoreGame(struct SokGame *game)
{
	struct SokSave save;

	if (!game || !game->args || !game->args->vol ||
			!dc32PortSaveRead(game->args->vol, "sokoban", &save, sizeof(save)) ||
			save.magic != SOK_SAVE_MAGIC ||
			save.version != SOK_SAVE_VERSION ||
			save.size != sizeof(save) ||
			!sokValidateSnapshot(&save.state)) {
		sokLoadLevel(game, 0);
		return;
	}
	game->state = save.state;
	game->solved = save.solved;
	game->undoHead = 0;
	game->undoCount = 0;
}

static void sokPushUndo(struct SokGame *game)
{
	game->undo[game->undoHead] = game->state;
	game->undoHead = (uint8_t)((game->undoHead + 1u) % SOK_UNDO_DEPTH);
	if (game->undoCount < SOK_UNDO_DEPTH)
		game->undoCount++;
}

static bool sokUndo(struct SokGame *game)
{
	if (!game->undoCount)
		return false;
	game->undoHead = (uint8_t)((game->undoHead + SOK_UNDO_DEPTH - 1u) % SOK_UNDO_DEPTH);
	game->state = game->undo[game->undoHead];
	game->undoCount--;
	game->solved = game->state.packets && game->state.savepack == game->state.packets;
	return true;
}

static enum SokAction sokTestMove(const struct SokGame *game, int8_t dr, int8_t dc)
{
	const struct SokSnapshot *state = &game->state;
	int32_t row1 = (int32_t)state->player.row + dr;
	int32_t col1 = (int32_t)state->player.col + dc;
	int32_t row2 = row1 + dr;
	int32_t col2 = col1 + dc;
	char target = sokCellAt(state, row1, col1);

	if (sokIsPacket(target)) {
		char behind = sokCellAt(state, row2, col2);

		if (behind == ' ')
			return target == '*' ? SokActionUnsave : SokActionPush;
		if (behind == '.')
			return target == '*' ? SokActionStorePush : SokActionSave;
		return SokActionNone;
	}
	if (target == ' ')
		return SokActionMove;
	if (target == '.')
		return SokActionStoreMove;
	return SokActionNone;
}

static bool sokMove(struct SokGame *game, int8_t dr, int8_t dc)
{
	struct SokSnapshot *state = &game->state;
	int32_t row1 = (int32_t)state->player.row + dr;
	int32_t col1 = (int32_t)state->player.col + dc;
	int32_t row2 = row1 + dr;
	int32_t col2 = col1 + dc;
	enum SokAction action = sokTestMove(game, dr, dc);

	if (action == SokActionNone || row1 < 0 || col1 < 0 ||
			row1 >= state->rows || col1 >= state->cols)
		return false;
	sokPushUndo(game);
	*sokCell(state, state->player.row, state->player.col) =
		*sokCell(state, state->player.row, state->player.col) == '@' ? ' ' : '.';
	switch (action) {
		case SokActionMove:
			*sokCell(state, (uint8_t)row1, (uint8_t)col1) = '@';
			break;
		case SokActionStoreMove:
			*sokCell(state, (uint8_t)row1, (uint8_t)col1) = '+';
			break;
		case SokActionPush:
			*sokCell(state, (uint8_t)row2, (uint8_t)col2) = '$';
			*sokCell(state, (uint8_t)row1, (uint8_t)col1) = '@';
			state->pushes++;
			break;
		case SokActionUnsave:
			*sokCell(state, (uint8_t)row2, (uint8_t)col2) = '$';
			*sokCell(state, (uint8_t)row1, (uint8_t)col1) = '+';
			state->pushes++;
			state->savepack--;
			break;
		case SokActionSave:
			*sokCell(state, (uint8_t)row2, (uint8_t)col2) = '*';
			*sokCell(state, (uint8_t)row1, (uint8_t)col1) = '@';
			state->pushes++;
			state->savepack++;
			break;
		case SokActionStorePush:
			*sokCell(state, (uint8_t)row2, (uint8_t)col2) = '*';
			*sokCell(state, (uint8_t)row1, (uint8_t)col1) = '+';
			state->pushes++;
			break;
		default:
			break;
	}
	state->player.row = (uint8_t)row1;
	state->player.col = (uint8_t)col1;
	state->moves++;
	game->solved = state->packets && state->savepack == state->packets;
	if (game->solved)
		sokSaveGame(game);
	return true;
}

static uint8_t sokWallTile(const struct SokSnapshot *state, uint8_t row, uint8_t col)
{
	uint8_t ret = 0;

	if (row > 0 && sokCellAt(state, row - 1, col) == '#')
		ret += 1;
	if (col + 1 < state->cols && sokCellAt(state, row, col + 1) == '#')
		ret += 2;
	if (row + 1 < state->rows && sokCellAt(state, row + 1, col) == '#')
		ret += 4;
	if (col > 0 && sokCellAt(state, row, col - 1) == '#')
		ret += 8;
	return XsokobanTileWall0 + ret;
}

static uint8_t sokTileForCell(const struct SokSnapshot *state, uint8_t row, uint8_t col)
{
	switch (sokCellAt(state, row, col)) {
		case '@':
			return XsokobanTileMan;
		case '+':
			return XsokobanTileSaveman;
		case '.':
			return XsokobanTileGoal;
		case '$':
			return XsokobanTileObject;
		case '*':
			return XsokobanTileTreasure;
		case '#':
			return sokWallTile(state, row, col);
		default:
			return XsokobanTileFloor;
	}
}

static void sokDrawText(struct SokGame *game, int32_t x, int32_t y, const char *text, enum Font font, uint16_t color)
{
	while (text && *text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text)) {
			for (uint_fast8_t row = 0; row < glyph.height; row++)
				for (uint_fast8_t col = 0; col < glyph.width; col++)
					if (fontGetGlyphPixel(&glyph, row, col))
						dcAppDrawPixel(&game->draw, x + (int32_t)col, y + (int32_t)row, color);
			x += glyph.width + 1;
		}
		text++;
	}
}

static uint32_t sokTextWidth(const char *text, enum Font font)
{
	uint32_t width = 0;

	while (text && *text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text))
			width += glyph.width + 1u;
		text++;
	}
	return width ? width - 1u : 0;
}

static void sokDrawCentered(struct SokGame *game, int32_t y, const char *text, enum Font font, uint16_t color)
{
	uint32_t width = sokTextWidth(text, font);
	int32_t x = (int32_t)((SOK_SCREEN_W > width ? SOK_SCREEN_W - width : 0u) / 2u);

	sokDrawText(game, x, y, text, font, color);
}

static void sokDrawTile(struct SokGame *game, uint8_t tileId, int32_t x, int32_t y, uint8_t scale)
{
	const uint8_t *src = xsokobanTiles[tileId];

	for (uint8_t dy = 0; dy < scale; dy++) {
		int32_t yy = y + dy;
		uint32_t sy = ((uint32_t)dy * XSOKOBAN_TILE_SIZE) / scale;

		if (yy < 0 || yy >= (int32_t)SOK_SCREEN_H)
			continue;
		for (uint8_t dx = 0; dx < scale; dx++) {
			int32_t xx = x + dx;
			uint32_t sx = ((uint32_t)dx * XSOKOBAN_TILE_SIZE) / scale;

			if (xx >= 0 && xx < (int32_t)SOK_SCREEN_W)
				game->frame[(uint32_t)yy * SOK_SCREEN_W + (uint32_t)xx] =
					src[sy * XSOKOBAN_TILE_SIZE + sx];
		}
	}
}

static uint8_t sokBoardScale(const struct SokSnapshot *state)
{
	uint32_t maxW = SOK_SCREEN_W - 8u;
	uint32_t maxH = SOK_SCREEN_H - SOK_HUD_H - 8u;
	uint32_t scaleW = maxW / state->cols;
	uint32_t scaleH = maxH / state->rows;
	uint32_t scale = scaleW < scaleH ? scaleW : scaleH;

	if (scale > XSOKOBAN_TILE_SIZE)
		scale = XSOKOBAN_TILE_SIZE;
	if (scale < 8u)
		scale = 8u;
	return (uint8_t)scale;
}

static void sokDraw(struct SokGame *game)
{
	const struct SokSnapshot *state = &game->state;
	uint8_t bg = sokRgb332(24, 28, 34);
	uint8_t panel = sokRgb332(38, 44, 52);
	uint8_t scale = sokBoardScale(state);
	uint32_t boardW = (uint32_t)state->cols * scale;
	uint32_t boardH = (uint32_t)state->rows * scale;
	int32_t originX = (int32_t)((SOK_SCREEN_W - boardW) / 2u);
	int32_t originY = (int32_t)(SOK_HUD_H + ((SOK_SCREEN_H - SOK_HUD_H - boardH) / 2u));
	char text[64];

	memset(game->frame, bg, SOK_SCREEN_W * SOK_SCREEN_H);
	for (uint32_t y = 0; y < SOK_HUD_H; y++)
		memset(game->frame + y * SOK_SCREEN_W, panel, SOK_SCREEN_W);
	snprintf(text, sizeof(text), "XSokoban %u/90  M %lu  P %lu",
		(unsigned)state->level + 1u, (unsigned long)state->moves, (unsigned long)state->pushes);
	sokDrawText(game, 6, 6, text, FontSmall, sokRgb565(238, 238, 220));
	snprintf(text, sizeof(text), "%u/%u", (unsigned)state->savepack, (unsigned)state->packets);
	sokDrawText(game, 274, 6, text, FontSmall, sokRgb565(255, 220, 120));
	for (uint8_t row = 0; row < state->rows; row++)
		for (uint8_t col = 0; col < state->cols; col++)
			sokDrawTile(game, sokTileForCell(state, row, col),
				originX + (int32_t)col * scale,
				originY + (int32_t)row * scale,
				scale);
	if (game->solved) {
		dcAppDrawFill(&game->draw, 78, 96, 164, 48, sokRgb565(20, 24, 30));
		sokDrawCentered(game, 106, "Level complete", FontMedium, sokRgb565(255, 255, 255));
		sokDrawCentered(game, 126, "Press A", FontSmall, sokRgb565(255, 220, 120));
	}
}

static uint_fast16_t sokMoveKey(uint_fast16_t keys)
{
	if (keys & KEY_BIT_UP)
		return KEY_BIT_UP;
	if (keys & KEY_BIT_DOWN)
		return KEY_BIT_DOWN;
	if (keys & KEY_BIT_LEFT)
		return KEY_BIT_LEFT;
	if (keys & KEY_BIT_RIGHT)
		return KEY_BIT_RIGHT;
	return 0;
}

static void sokApplyMoveKey(struct SokGame *game, uint_fast16_t key)
{
	switch (key) {
		case KEY_BIT_UP:
			(void)sokMove(game, -1, 0);
			break;
		case KEY_BIT_DOWN:
			(void)sokMove(game, 1, 0);
			break;
		case KEY_BIT_LEFT:
			(void)sokMove(game, 0, -1);
			break;
		case KEY_BIT_RIGHT:
			(void)sokMove(game, 0, 1);
			break;
		default:
			break;
	}
}

static void sokAdvanceLevel(struct SokGame *game)
{
	uint8_t next = game->state.level + 1u;

	if (next >= XSOKOBAN_LEVEL_COUNT)
		next = 0;
	sokLoadLevel(game, next);
	sokSaveGame(game);
}

static void sokHandleInput(struct SokGame *game)
{
	uint_fast16_t key = sokMoveKey(game->draw.keys);

	if (game->solved) {
		if (game->draw.pressed & (KEY_BIT_A | KEY_BIT_START | KEY_BIT_RIGHT))
			sokAdvanceLevel(game);
		if (game->draw.pressed & KEY_BIT_B)
			(void)sokUndo(game);
		return;
	}
	if (game->draw.pressed & KEY_BIT_B) {
		if (sokUndo(game))
			sokSaveGame(game);
	}
	if (game->draw.pressed & KEY_BIT_START) {
		sokLoadLevel(game, game->state.level);
		sokSaveGame(game);
	}
	if (game->draw.pressed & KEY_BIT_SEL) {
		sokSaveGame(game);
	}
	if (key) {
		if ((game->draw.pressed & key) || game->repeatKey != key || game->repeatDelay == 0) {
			sokApplyMoveKey(game, key);
			game->repeatKey = key;
			game->repeatDelay = (game->draw.pressed & key) ? 12u : 4u;
		}
		else {
			game->repeatDelay--;
		}
	}
	else {
		game->repeatKey = 0;
		game->repeatDelay = 0;
	}
}

int sokobanAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct SokGame *game;

	mSokobanAbort = false;
	dc32PortHeapInitDefault();
	game = dc32PortCalloc(1, sizeof(*game));
	if (!game)
		return -1;
	game->frame = dc32PortMalloc(SOK_SCREEN_W * SOK_SCREEN_H);
	if (!game->frame)
		return -1;
	game->host = host;
	game->args = args;
	if (!dcAppDrawInit(&game->draw, host, args, game->frame, SOK_SCREEN_W, SOK_SCREEN_H))
		return -1;
	dcAppDrawWaitRelease(&game->draw, KEY_BIT_A | KEY_BIT_B | KEY_BIT_START | KEY_BIT_SEL | UI_KEY_BIT_CENTER);
	sokRestoreGame(game);
	dispSetFramerate(60);
	game->running = true;
	while (game->running && !mSokobanAbort) {
		sokDraw(game);
		if (!dcAppDrawFrame(&game->draw, UI_KEY_BIT_CENTER))
			break;
		sokHandleInput(game);
	}
	sokSaveGame(game);
	dcAppDrawWaitRelease(&game->draw, UI_KEY_BIT_CENTER);
	return 0;
}

void sokobanAppAbort(void)
{
	mSokobanAbort = true;
}
