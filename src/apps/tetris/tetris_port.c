/*
 * DC32 presentation, controls, persistence, and buzzer adapter for the
 * NullpoMino-derived Tetris engine.
 */
#include "apps/tetris/tetris_port.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "apps/port/port_runtime.h"
#include "apps/tetris/tetris_core.h"
#include "audioPwm.h"
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "ui.h"

#define TETRIS_SCREEN_W 320
#define TETRIS_SCREEN_H 240
#define TETRIS_FPS 60u
#define TETRIS_FIELD_X 100
#define TETRIS_FIELD_Y 0
#define TETRIS_CELL 12
#define TETRIS_FIELD_PIXEL_W (TETRIS_FIELD_WIDTH * TETRIS_CELL)
#define TETRIS_FIELD_PIXEL_H (TETRIS_FIELD_HEIGHT * TETRIS_CELL)
#define TETRIS_RANKS 5
#define TETRIS_SAVE_MAGIC 0x31544554u
#define TETRIS_SAVE_VERSION 1u
#define TETRIS_SAVE_XOR 0x91e10da5u

enum TetrisScreen {
	TetrisScreenTitle,
	TetrisScreenRecords,
	TetrisScreenPlaying,
	TetrisScreenPaused,
	TetrisScreenFinished,
};

struct TetrisMarathonRecord {
	uint32_t score;
	uint32_t lines;
	uint32_t time;
};

struct TetrisLineRecord {
	uint32_t time;
	uint32_t pieces;
};

struct TetrisUltraRecord {
	uint32_t score;
	uint32_t lines;
};

struct TetrisSave {
	uint32_t magic;
	uint16_t version;
	uint16_t size;
	uint8_t reservedAudioEnabled;
	uint8_t reservedVolume;
	uint8_t selectedMode;
	uint8_t selectedRule;
	struct TetrisMarathonRecord marathon[TetrisRuleCount][TETRIS_RANKS];
	struct TetrisLineRecord line[TetrisRuleCount][TETRIS_RANKS];
	struct TetrisUltraRecord ultraScore[TetrisRuleCount][TETRIS_RANKS];
	struct TetrisUltraRecord ultraLines[TetrisRuleCount][TETRIS_RANKS];
	uint32_t check;
};

struct TetrisApp {
	const struct DcAppHostApi *host;
	const struct DcAppRunArgs *args;
	struct DcAppDrawCtx draw;
	struct TetrisGame game;
	struct TetrisSave save;
	uint32_t seed;
	uint32_t toneFrames;
	uint8_t screen;
	uint8_t titleSelection;
	uint8_t recordsRule;
	bool saveDirty;
	bool recordedRun;
};

static uint8_t mTetrisFrame[TETRIS_SCREEN_W * TETRIS_SCREEN_H];
static volatile bool mTetrisAbort;

static uint16_t tetrisRgb(uint32_t r, uint32_t g, uint32_t b)
{
	return dcAppDrawRgb565(r, g, b);
}

static uint16_t tetrisBlockColor(uint8_t piece)
{
	static const uint16_t colors[TetrisPieceCount] = {
		0x07ff, 0xfd20, 0xffe0, 0xf800, 0x801f, 0x001f, 0x07e0,
	};

	return piece < TetrisPieceCount ? colors[piece] : tetrisRgb(110, 120, 135);
}

static uint32_t tetrisTextWidth(const char *text, enum Font font)
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

static void tetrisText(struct TetrisApp *app, int32_t x, int32_t y,
	const char *text, enum Font font, uint16_t color)
{
	while (text && *text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text)) {
			for (uint8_t row = 0; row < glyph.height; row++)
				for (uint8_t col = 0; col < glyph.width; col++)
					if (fontGetGlyphPixel(&glyph, row, col))
						dcAppDrawPixel(&app->draw, x + col, y + row, color);
			x += glyph.width + 1;
		}
		text++;
	}
}

static void tetrisCentered(struct TetrisApp *app, int32_t left, int32_t width,
	int32_t y, const char *text, enum Font font, uint16_t color)
{
	uint32_t textWidth = tetrisTextWidth(text, font);

	tetrisText(app, left + (width - (int32_t)textWidth) / 2, y, text, font, color);
}

static void tetrisNumber(char *text, uint32_t size, uint32_t value)
{
	char reverse[12];
	uint32_t count = 0;
	uint32_t out = 0;

	if (!size)
		return;
	if (!value)
		reverse[count++] = '0';
	while (value && count < sizeof(reverse)) {
		reverse[count++] = (char)('0' + value % 10u);
		value /= 10u;
	}
	while (count && out + 1u < size)
		text[out++] = reverse[--count];
	text[out] = 0;
}

static void tetrisAppend(char *text, uint32_t size, const char *suffix)
{
	uint32_t pos = 0;

	if (!size)
		return;
	while (pos < size && text[pos])
		pos++;
	while (suffix && *suffix && pos + 1u < size)
		text[pos++] = *suffix++;
	if (pos < size)
		text[pos] = 0;
}

static void tetrisAppendTwoDigits(char *text, uint32_t size, uint32_t value)
{
	uint32_t pos = 0;

	if (!size)
		return;
	while (pos < size && text[pos])
		pos++;
	if (pos + 2u >= size)
		return;
	text[pos++] = (char)('0' + value / 10u % 10u);
	text[pos++] = (char)('0' + value % 10u);
	text[pos] = 0;
}

static void tetrisTime(char *text, uint32_t size, uint32_t frames)
{
	uint32_t minutes = frames / 3600u;
	uint32_t seconds = frames / 60u % 60u;
	uint32_t hundredths = frames % 60u * 100u / 60u;

	tetrisNumber(text, size, minutes);
	tetrisAppend(text, size, ":");
	tetrisAppendTwoDigits(text, size, seconds);
	tetrisAppend(text, size, ".");
	tetrisAppendTwoDigits(text, size, hundredths);
}

static uint32_t tetrisSaveCheck(const struct TetrisSave *save)
{
	const uint8_t *bytes = (const uint8_t *)save;
	uint32_t check = TETRIS_SAVE_XOR;

	for (uint32_t i = 0; i < offsetof(struct TetrisSave, check); i++)
		check = (check << 5) ^ (check >> 27) ^ bytes[i];
	return check;
}

static void tetrisDefaultSave(struct TetrisSave *save)
{
	memset(save, 0, sizeof(*save));
	save->magic = TETRIS_SAVE_MAGIC;
	save->version = TETRIS_SAVE_VERSION;
	save->size = sizeof(*save);
	for (uint8_t rule = 0; rule < TetrisRuleCount; rule++)
		for (uint8_t rank = 0; rank < TETRIS_RANKS; rank++)
			save->line[rule][rank].time = UINT32_MAX;
	save->check = tetrisSaveCheck(save);
}

static void tetrisLoadSave(struct TetrisApp *app)
{
	struct TetrisSave loaded;

	tetrisDefaultSave(&app->save);
	if (!app->args->vol ||
			!dc32PortSaveRead(app->args->vol, "tetris", &loaded, sizeof(loaded)))
		return;
	if (loaded.magic != TETRIS_SAVE_MAGIC || loaded.version != TETRIS_SAVE_VERSION ||
			loaded.size != sizeof(loaded) || loaded.check != tetrisSaveCheck(&loaded))
		return;
	if (loaded.selectedMode >= TetrisModeCount ||
			loaded.selectedRule >= TetrisRuleCount)
		return;
	app->save = loaded;
}

static bool tetrisWriteSave(struct TetrisApp *app)
{
	if (!app->saveDirty)
		return true;
	app->save.magic = TETRIS_SAVE_MAGIC;
	app->save.version = TETRIS_SAVE_VERSION;
	app->save.size = sizeof(app->save);
	app->save.check = tetrisSaveCheck(&app->save);
	if (!app->args->vol ||
			!dc32PortSaveWrite(app->args->vol, "tetris", &app->save, sizeof(app->save)))
		return false;
	app->saveDirty = false;
	return true;
}

static bool tetrisMarathonBetter(const struct TetrisMarathonRecord *a,
	const struct TetrisMarathonRecord *b)
{
	if (a->score != b->score)
		return a->score > b->score;
	if (a->lines != b->lines)
		return a->lines > b->lines;
	return !b->time || a->time < b->time;
}

static bool tetrisLineBetter(const struct TetrisLineRecord *a,
	const struct TetrisLineRecord *b)
{
	if (b->time == UINT32_MAX)
		return true;
	if (a->time != b->time)
		return a->time < b->time;
	return a->pieces < b->pieces;
}

static bool tetrisUltraScoreBetter(const struct TetrisUltraRecord *a,
	const struct TetrisUltraRecord *b)
{
	return a->score > b->score || (a->score == b->score && a->lines > b->lines);
}

static bool tetrisUltraLinesBetter(const struct TetrisUltraRecord *a,
	const struct TetrisUltraRecord *b)
{
	return a->lines > b->lines || (a->lines == b->lines && a->score > b->score);
}

static void tetrisInsertMarathon(struct TetrisMarathonRecord *records,
	const struct TetrisMarathonRecord *record)
{
	for (uint8_t rank = 0; rank < TETRIS_RANKS; rank++)
		if (tetrisMarathonBetter(record, &records[rank])) {
			for (uint8_t move = TETRIS_RANKS - 1u; move > rank; move--)
				records[move] = records[move - 1u];
			records[rank] = *record;
			return;
		}
}

static void tetrisInsertLine(struct TetrisLineRecord *records,
	const struct TetrisLineRecord *record)
{
	for (uint8_t rank = 0; rank < TETRIS_RANKS; rank++)
		if (tetrisLineBetter(record, &records[rank])) {
			for (uint8_t move = TETRIS_RANKS - 1u; move > rank; move--)
				records[move] = records[move - 1u];
			records[rank] = *record;
			return;
		}
}

static void tetrisInsertUltra(struct TetrisUltraRecord *records,
	const struct TetrisUltraRecord *record,
	bool (*better)(const struct TetrisUltraRecord *, const struct TetrisUltraRecord *))
{
	for (uint8_t rank = 0; rank < TETRIS_RANKS; rank++)
		if (better(record, &records[rank])) {
			for (uint8_t move = TETRIS_RANKS - 1u; move > rank; move--)
				records[move] = records[move - 1u];
			records[rank] = *record;
			return;
		}
}

static void tetrisRecordRun(struct TetrisApp *app)
{
	uint8_t rule = app->save.selectedRule;

	if (app->recordedRun)
		return;
	app->recordedRun = true;
	if (app->save.selectedMode == TetrisModeMarathon) {
		struct TetrisMarathonRecord record = {
			app->game.stats.score, app->game.stats.lines, app->game.stats.time,
		};

		tetrisInsertMarathon(app->save.marathon[rule], &record);
	}
	else if (app->save.selectedMode == TetrisModeLineRace) {
		if (app->game.stats.lines < 40u)
			return;
		{
			struct TetrisLineRecord record = {
				app->game.stats.time, app->game.stats.pieces,
			};

			tetrisInsertLine(app->save.line[rule], &record);
		}
	}
	else {
		struct TetrisUltraRecord record = {
			app->game.stats.score, app->game.stats.lines,
		};

		tetrisInsertUltra(app->save.ultraScore[rule], &record, tetrisUltraScoreBetter);
		tetrisInsertUltra(app->save.ultraLines[rule], &record, tetrisUltraLinesBetter);
	}
	app->saveDirty = true;
	(void)tetrisWriteSave(app);
}

static void tetrisTone(struct TetrisApp *app, uint32_t frequency, uint32_t milliseconds)
{
	if (!frequency) {
		app->toneFrames = 0;
		audioPwmStop();
		return;
	}
	(void)audioPwmTone(frequency);
	app->toneFrames = (milliseconds * TETRIS_FPS + 999u) / 1000u;
	if (!app->toneFrames)
		app->toneFrames = 1;
}

static void tetrisPlayEvents(struct TetrisApp *app, uint32_t events)
{
	if (events & TetrisEventGameOver)
		tetrisTone(app, 110, 450);
	else if (events & TetrisEventComplete)
		tetrisTone(app, 988, 300);
	else if (events & TetrisEventLevel)
		tetrisTone(app, 784, 180);
	else if (events & TetrisEventSpin)
		tetrisTone(app, 880, 150);
	else if (events & TetrisEventClear)
		tetrisTone(app, 660, 120);
	else if (events & TetrisEventLock)
		tetrisTone(app, 180, 45);
	else if (events & TetrisEventHold)
		tetrisTone(app, 520, 60);
	else if (events & TetrisEventRotate)
		tetrisTone(app, 440, 35);
	else if (events & TetrisEventMove)
		tetrisTone(app, 260, 20);
	else if (events & TetrisEventCountdown)
		tetrisTone(app, app->game.stateFrame >= 50u ? 880 : 440, 80);
}

static void tetrisDrawBlock(struct TetrisApp *app, int32_t x, int32_t y,
	int32_t size, uint8_t piece, bool ghost)
{
	uint16_t color = tetrisBlockColor(piece);
	uint16_t edge = ghost ? tetrisRgb(70, 80, 95) : tetrisRgb(235, 240, 255);
	uint16_t shade = ghost ? tetrisRgb(18, 24, 34) : color;

	if (ghost) {
		dcAppDrawLine(&app->draw, x, y, x + size - 1, y, edge);
		dcAppDrawLine(&app->draw, x, y, x, y + size - 1, edge);
		dcAppDrawLine(&app->draw, x + size - 1, y, x + size - 1, y + size - 1, edge);
		dcAppDrawLine(&app->draw, x, y + size - 1, x + size - 1, y + size - 1, edge);
		return;
	}
	dcAppDrawFill(&app->draw, x, y, size, size, shade);
	dcAppDrawLine(&app->draw, x, y, x + size - 1, y, edge);
	dcAppDrawLine(&app->draw, x, y, x, y + size - 1, edge);
	dcAppDrawLine(&app->draw, x + size - 1, y + size - 1, x + 1, y + size - 1,
		tetrisRgb(25, 28, 36));
	dcAppDrawLine(&app->draw, x + size - 1, y + size - 1, x + size - 1, y + 1,
		tetrisRgb(25, 28, 36));
}

static void tetrisDrawMiniPiece(struct TetrisApp *app, uint8_t piece,
	int32_t left, int32_t top, int32_t width, int32_t cell)
{
	int8_t minX = 8, maxX = -8, minY = 8, maxY = -8;
	int8_t x, y;
	uint8_t direction;

	if (piece >= TetrisPieceCount)
		return;
	direction = app->game.rule ? app->game.rule->spawnDirection : 0;
	for (uint8_t i = 0; i < 4; i++) {
		tetrisGamePieceBlock(&app->game, piece, direction, i, &x, &y);
		if (x < minX) minX = x;
		if (x > maxX) maxX = x;
		if (y < minY) minY = y;
		if (y > maxY) maxY = y;
	}
	left += (width - (maxX - minX + 1) * cell) / 2;
	for (uint8_t i = 0; i < 4; i++) {
		tetrisGamePieceBlock(&app->game, piece, direction, i, &x, &y);
		tetrisDrawBlock(app, left + (x - minX) * cell,
			top + (y - minY) * cell, cell, piece, false);
	}
}

static void tetrisDrawField(struct TetrisApp *app)
{
	const struct TetrisGame *game = &app->game;
	uint16_t frame = tetrisRgb(105, 120, 145);
	uint16_t black = tetrisRgb(0, 0, 0);

	dcAppDrawFill(&app->draw, TETRIS_FIELD_X - 3, TETRIS_FIELD_Y,
		3, TETRIS_FIELD_PIXEL_H, frame);
	dcAppDrawFill(&app->draw, TETRIS_FIELD_X + TETRIS_FIELD_PIXEL_W,
		TETRIS_FIELD_Y, 3, TETRIS_FIELD_PIXEL_H, frame);
	dcAppDrawFill(&app->draw, TETRIS_FIELD_X, TETRIS_FIELD_Y,
		TETRIS_FIELD_PIXEL_W, TETRIS_FIELD_PIXEL_H, black);
	for (uint8_t y = 0; y < TETRIS_FIELD_HEIGHT; y++)
		for (uint8_t x = 0; x < TETRIS_FIELD_WIDTH; x++) {
			uint8_t cell = game->board[y + TETRIS_MAX_HIDDEN][x];

			if (cell && !(game->state == TetrisStateLineClear &&
					game->clearRows[y + TETRIS_MAX_HIDDEN] &&
					((game->stateFrame / 4u) & 1u)))
				tetrisDrawBlock(app, TETRIS_FIELD_X + x * TETRIS_CELL,
					TETRIS_FIELD_Y + y * TETRIS_CELL, TETRIS_CELL,
					cell - 1u, false);
		}
	if (game->active && game->rule->ghost) {
		int8_t ghostY = tetrisGameGhostY(game);

		for (uint8_t i = 0; i < 4; i++) {
			int8_t x, y;

			tetrisGamePieceBlock(game, game->activePiece, game->activeDirection,
				i, &x, &y);
			x += game->activeX;
			y += ghostY;
			if (y >= 0)
				tetrisDrawBlock(app, TETRIS_FIELD_X + x * TETRIS_CELL,
					TETRIS_FIELD_Y + y * TETRIS_CELL, TETRIS_CELL,
					game->activePiece, true);
		}
	}
	if (game->active)
		for (uint8_t i = 0; i < 4; i++) {
			int8_t x, y;

			tetrisGamePieceBlock(game, game->activePiece, game->activeDirection,
				i, &x, &y);
			x += game->activeX;
			y += game->activeY;
			if (y >= 0)
				tetrisDrawBlock(app, TETRIS_FIELD_X + x * TETRIS_CELL,
					TETRIS_FIELD_Y + y * TETRIS_CELL, TETRIS_CELL,
					game->activePiece, false);
		}
}

static void tetrisDrawHud(struct TetrisApp *app)
{
	const struct TetrisGame *game = &app->game;
	uint16_t white = tetrisRgb(235, 240, 255);
	uint16_t dim = tetrisRgb(115, 130, 150);
	uint16_t cyan = tetrisRgb(70, 220, 255);
	char text[24];

	tetrisText(app, 12, 18, "HOLD", FontSmall, cyan);
	if (game->rule->hold)
		tetrisDrawMiniPiece(app, game->holdPiece, 8, 36, 84, 6);
	else
		tetrisText(app, 30, 42, "OFF", FontSmall, dim);

	tetrisText(app, 8, 80, "SCORE", FontSmall, dim);
	tetrisNumber(text, sizeof(text), game->stats.score);
	tetrisText(app, 8, 93, text, FontMedium, white);
	tetrisText(app, 8, 118, "LINES", FontSmall, dim);
	tetrisNumber(text, sizeof(text), game->stats.lines);
	tetrisText(app, 8, 131, text, FontMedium, white);
	tetrisText(app, 8, 156, "LEVEL", FontSmall, dim);
	tetrisNumber(text, sizeof(text), game->stats.level + 1u);
	tetrisText(app, 8, 169, text, FontMedium, white);
	tetrisText(app, 8, 194, "TIME", FontSmall, dim);
	tetrisTime(text, sizeof(text), game->stats.time);
	tetrisText(app, 8, 207, text, FontSmall, white);

	tetrisText(app, 235, 18, "NEXT", FontSmall, cyan);
	for (uint8_t i = 0; i < game->rule->nextDisplay && i < TETRIS_NEXT_MAX; i++)
		tetrisDrawMiniPiece(app, game->next[i], 226, 34 + i * 29, 88, 5);
	tetrisCentered(app, 224, 96, 213, game->mode->name, FontSmall, white);
	tetrisCentered(app, 224, 96, 226, game->rule->name, FontSmall, dim);
}

static void tetrisDrawOverlay(struct TetrisApp *app, const char *title,
	const char *line)
{
	uint16_t panel = tetrisRgb(8, 12, 22);
	uint16_t edge = tetrisRgb(90, 210, 255);
	uint16_t white = tetrisRgb(245, 245, 255);

	dcAppDrawFill(&app->draw, 72, 91, 176, 58, panel);
	dcAppDrawLine(&app->draw, 72, 91, 247, 91, edge);
	dcAppDrawLine(&app->draw, 72, 148, 247, 148, edge);
	tetrisCentered(app, 72, 176, 101, title, FontLarge, white);
	if (line)
		tetrisCentered(app, 72, 176, 130, line, FontSmall, edge);
}

static void tetrisDrawGame(struct TetrisApp *app)
{
	char text[20];

	dcAppDrawClear(&app->draw, tetrisRgb(3, 6, 14));
	tetrisDrawField(app);
	tetrisDrawHud(app);
	if (app->game.state == TetrisStateReady) {
		if (app->game.stateFrame < 49u)
			tetrisDrawOverlay(app, "READY", 0);
		else if (app->game.stateFrame >= 50u &&
				app->game.stateFrame < 100u)
			tetrisDrawOverlay(app, "GO!", 0);
	}
	else if (app->screen == TetrisScreenPaused)
		tetrisDrawOverlay(app, "PAUSED", "START RESUME  B QUIT");
	else if (app->game.state == TetrisStateGameOver)
		tetrisDrawOverlay(app, "GAME OVER", "A RETRY  B TITLE");
	else if (app->game.state == TetrisStateResult) {
		if (app->save.selectedMode == TetrisModeLineRace)
			tetrisTime(text, sizeof(text), app->game.stats.time);
		else
			tetrisNumber(text, sizeof(text), app->game.stats.score);
		tetrisDrawOverlay(app, "COMPLETE", text);
	}
}

static void tetrisDrawTitle(struct TetrisApp *app)
{
	static const char *const labels[] = {"START", "MODE", "RULE", "RECORDS"};
	uint16_t white = tetrisRgb(240, 245, 255);
	uint16_t cyan = tetrisRgb(60, 220, 255);
	uint16_t dim = tetrisRgb(100, 120, 145);

	dcAppDrawClear(&app->draw, tetrisRgb(3, 6, 14));
	tetrisCentered(app, 0, 320, 10, "TETRIS", FontLarge, white);
	tetrisCentered(app, 0, 320, 34, "NULLPOMINO RULES", FontSmall, cyan);
	for (uint8_t i = 0; i < 4; i++) {
		int32_t y = 73 + i * 34;

		if (i == app->titleSelection) {
			dcAppDrawFill(&app->draw, 44, y - 4, 5, 16, cyan);
			dcAppDrawLine(&app->draw, 55, y + 14, 275, y + 14, cyan);
		}
		tetrisText(app, 61, y, labels[i], FontMedium, white);
		if (i == 1)
			tetrisText(app, 174, y, tetrisModeGet(app->save.selectedMode)->name,
				FontSmall, cyan);
		else if (i == 2)
			tetrisText(app, 174, y, tetrisRuleGet(app->save.selectedRule)->name,
				FontSmall, cyan);
	}
	tetrisCentered(app, 0, 320, 214,
		tetrisModeGet(app->save.selectedMode)->description, FontSmall, dim);
	tetrisCentered(app, 0, 320, 228, "A SELECT  CENTER EXIT", FontSmall, white);
}

static void tetrisDrawRecords(struct TetrisApp *app)
{
	uint8_t rule = app->recordsRule;
	uint16_t white = tetrisRgb(240, 245, 255);
	uint16_t cyan = tetrisRgb(60, 220, 255);
	uint16_t dim = tetrisRgb(100, 120, 145);
	char first[20], second[20];

	dcAppDrawClear(&app->draw, tetrisRgb(3, 6, 14));
	tetrisCentered(app, 0, 320, 8, "RECORDS", FontLarge, white);
	tetrisCentered(app, 0, 320, 33, tetrisRuleGet(rule)->name, FontSmall, cyan);
	tetrisText(app, 10, 53, "MARATHON", FontSmall, cyan);
	tetrisText(app, 114, 53, "40 LINE", FontSmall, cyan);
	tetrisText(app, 216, 53, "ULTRA", FontSmall, cyan);
	for (uint8_t rank = 0; rank < TETRIS_RANKS; rank++) {
		int32_t y = 72 + rank * 27;
		const struct TetrisMarathonRecord *marathon = &app->save.marathon[rule][rank];
		const struct TetrisLineRecord *line = &app->save.line[rule][rank];
		const struct TetrisUltraRecord *ultra = &app->save.ultraScore[rule][rank];

		first[0] = (char)('1' + rank);
		first[1] = '.';
		first[2] = 0;
		tetrisText(app, 2, y, first, FontSmall, dim);
		if (marathon->score) {
			tetrisNumber(first, sizeof(first), marathon->score);
			tetrisText(app, 19, y, first, FontSmall, white);
			tetrisNumber(second, sizeof(second), marathon->lines);
			tetrisAppend(second, sizeof(second), "L");
			tetrisText(app, 19, y + 12, second, FontSmall, dim);
		}
		if (line->time != UINT32_MAX) {
			tetrisTime(first, sizeof(first), line->time);
			tetrisText(app, 112, y, first, FontSmall, white);
			tetrisNumber(second, sizeof(second), line->pieces);
			tetrisAppend(second, sizeof(second), " PCS");
			tetrisText(app, 112, y + 12, second, FontSmall, dim);
		}
		if (ultra->score) {
			tetrisNumber(first, sizeof(first), ultra->score);
			tetrisText(app, 216, y, first, FontSmall, white);
			tetrisNumber(second, sizeof(second), ultra->lines);
			tetrisAppend(second, sizeof(second), "L");
			tetrisText(app, 216, y + 12, second, FontSmall, dim);
		}
	}
	tetrisCentered(app, 0, 320, 218, "LEFT/RIGHT RULE  B BACK", FontSmall, white);
}

static void tetrisStart(struct TetrisApp *app)
{
	app->seed ^= app->draw.frame * 0x9e3779b9u + 0x7f4a7c15u;
	tetrisGameInit(&app->game, (enum TetrisModeId)app->save.selectedMode,
		(enum TetrisRuleId)app->save.selectedRule, app->seed);
	app->recordedRun = false;
	app->screen = TetrisScreenPlaying;
}

static void tetrisAdjustTitle(struct TetrisApp *app, int8_t direction)
{
	if (app->titleSelection == 1) {
		int32_t value = app->save.selectedMode + direction;

		if (value < 0) value = TetrisModeCount - 1;
		if (value >= TetrisModeCount) value = 0;
		app->save.selectedMode = value;
		app->saveDirty = true;
		(void)tetrisWriteSave(app);
	}
	else if (app->titleSelection == 2) {
		int32_t value = app->save.selectedRule + direction;

		if (value < 0) value = TetrisRuleCount - 1;
		if (value >= TetrisRuleCount) value = 0;
		app->save.selectedRule = value;
		app->saveDirty = true;
		(void)tetrisWriteSave(app);
	}
}

static void tetrisHandleTitle(struct TetrisApp *app)
{
	uint_fast16_t pressed = app->draw.pressed;

	if (pressed & KEY_BIT_UP)
		app->titleSelection = app->titleSelection ? app->titleSelection - 1u : 3u;
	if (pressed & KEY_BIT_DOWN)
		app->titleSelection = (app->titleSelection + 1u) % 4u;
	if (pressed & KEY_BIT_LEFT)
		tetrisAdjustTitle(app, -1);
	if (pressed & KEY_BIT_RIGHT)
		tetrisAdjustTitle(app, 1);
	if (pressed & KEY_BIT_A) {
		if (app->titleSelection == 0)
			tetrisStart(app);
		else if (app->titleSelection == 1 || app->titleSelection == 2)
			tetrisAdjustTitle(app, 1);
		else {
			app->recordsRule = app->save.selectedRule;
			app->screen = TetrisScreenRecords;
		}
	}
}

static void tetrisHandleRecords(struct TetrisApp *app)
{
	if (app->draw.pressed & KEY_BIT_LEFT)
		app->recordsRule = app->recordsRule ? app->recordsRule - 1u : TetrisRuleCount - 1u;
	if (app->draw.pressed & KEY_BIT_RIGHT)
		app->recordsRule = (app->recordsRule + 1u) % TetrisRuleCount;
	if (app->draw.pressed & KEY_BIT_B)
		app->screen = TetrisScreenTitle;
}

static struct TetrisInput tetrisReadGameInput(const struct TetrisApp *app)
{
	struct TetrisInput input;
	uint_fast16_t keys = app->draw.keys;
	uint_fast16_t pressed = app->draw.pressed;

	memset(&input, 0, sizeof(input));
	input.left = (keys & KEY_BIT_LEFT) != 0;
	input.right = (keys & KEY_BIT_RIGHT) != 0;
	input.down = (keys & KEY_BIT_DOWN) != 0;
	input.hardDrop = (keys & KEY_BIT_UP) != 0;
	input.rotateCw = (keys & KEY_BIT_A) != 0;
	input.rotateCcw = (keys & KEY_BIT_B) != 0;
	input.hold = (keys & KEY_BIT_SEL) != 0;
	input.leftPressed = (pressed & KEY_BIT_LEFT) != 0;
	input.rightPressed = (pressed & KEY_BIT_RIGHT) != 0;
	input.hardDropPressed = (pressed & KEY_BIT_UP) != 0;
	input.rotateCwPressed = (pressed & KEY_BIT_A) != 0;
	input.rotateCcwPressed = (pressed & KEY_BIT_B) != 0;
	input.holdPressed = (pressed & KEY_BIT_SEL) != 0;
	return input;
}

static void tetrisHandleGame(struct TetrisApp *app)
{
	if (app->screen == TetrisScreenPaused) {
		if (app->draw.pressed & KEY_BIT_START)
			app->screen = TetrisScreenPlaying;
		else if (app->draw.pressed & KEY_BIT_B) {
			tetrisTone(app, 0, 0);
			app->screen = TetrisScreenTitle;
		}
		return;
	}
	if (app->draw.pressed & KEY_BIT_START) {
		app->screen = TetrisScreenPaused;
		tetrisTone(app, 0, 0);
		return;
	}
	if (app->game.state == TetrisStateGameOver ||
			app->game.state == TetrisStateResult) {
		tetrisRecordRun(app);
		if (app->draw.pressed & KEY_BIT_A)
			tetrisStart(app);
		else if (app->draw.pressed & KEY_BIT_B)
			app->screen = TetrisScreenTitle;
		return;
	}
	{
		struct TetrisInput input = tetrisReadGameInput(app);
		uint32_t events = tetrisGameUpdate(&app->game, &input);

		tetrisPlayEvents(app, events);
		if (app->game.state == TetrisStateGameOver ||
				app->game.state == TetrisStateResult)
			app->screen = TetrisScreenFinished;
	}
}

static void tetrisRender(struct TetrisApp *app)
{
	if (app->screen == TetrisScreenTitle)
		tetrisDrawTitle(app);
	else if (app->screen == TetrisScreenRecords)
		tetrisDrawRecords(app);
	else
		tetrisDrawGame(app);
}

static bool tetrisInit(struct TetrisApp *app, const struct DcAppHostApi *host,
	const struct DcAppRunArgs *args)
{
	uint64_t now;

	if (!app || !host || !args || !args->canvas)
		return false;
	memset(app, 0, sizeof(*app));
	app->host = host;
	app->args = args;
	if (!dcAppDrawInit(&app->draw, host, args, mTetrisFrame,
			TETRIS_SCREEN_W, TETRIS_SCREEN_H))
		return false;
	now = host->getTime ? host->getTime() : 0x4e554c4c504f4d49ull;
	app->seed = (uint32_t)(now ^ (now >> 32));
	tetrisLoadSave(app);
	app->recordsRule = app->save.selectedRule;
	app->screen = TetrisScreenTitle;
	return true;
}

int tetrisAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct TetrisApp app;

	mTetrisAbort = false;
	if (!tetrisInit(&app, host, args))
		return -1;
	dispSetFramerate(TETRIS_FPS);
	dcAppDrawWaitRelease(&app.draw, KEY_BIT_A | KEY_BIT_B | KEY_BIT_SEL |
		KEY_BIT_START | KEY_BIT_UP | KEY_BIT_DOWN | KEY_BIT_LEFT |
		KEY_BIT_RIGHT | UI_KEY_BIT_CENTER);
	tetrisRender(&app);
	while (!mTetrisAbort) {
		if (!dcAppDrawFrame(&app.draw, UI_KEY_BIT_CENTER))
			break;
		if (app.toneFrames && !--app.toneFrames)
			audioPwmStop();
		if (app.screen == TetrisScreenTitle)
			tetrisHandleTitle(&app);
		else if (app.screen == TetrisScreenRecords)
			tetrisHandleRecords(&app);
		else
			tetrisHandleGame(&app);
		tetrisRender(&app);
	}
	if (app.game.state == TetrisStateGameOver ||
			app.game.state == TetrisStateResult)
		tetrisRecordRun(&app);
	(void)tetrisWriteSave(&app);
	tetrisTone(&app, 0, 0);
	dcAppDrawWaitRelease(&app.draw, UI_KEY_BIT_CENTER);
	dispSetFramerate(60);
	return 0;
}

void tetrisAppAbort(void)
{
	mTetrisAbort = true;
}
