#include "apps/pipe/pipe_port.h"

#include <new>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "apps/port/port_runtime.h"
extern "C" {
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "ui.h"
}
#include "Board.h"
#include "Queue.h"
#include "Randomizer.h"
#include "TilePiece.h"

#ifndef DCAPP_RUNTIME_ID
#error "DCAPP_RUNTIME_ID must be provided by the app target"
#endif

extern "C" const struct DcAppImageHeader dcAppImageHeader __attribute__((section(".dcapp_header"), used, aligned(256))) = {
	.magic = DCAPP_MAGIC,
	.headerSize = DCAPP_HEADER_SIZE,
	.abiVersion = DCAPP_ABI_VERSION,
	.runtime = DCAPP_RUNTIME_ID,
	.flags = DCAPP_IMAGE_FLAG_LARGE_XIP,
	.loadAddr = 0x10100000u,
	.appRamStart = 0x2005F000u,
	.appRamSize = 0x00014000u,
};

static constexpr uint32_t PIPE_SCREEN_W = DC32_PORT_SCREEN_W;
static constexpr uint32_t PIPE_SCREEN_H = DC32_PORT_SCREEN_H;
static constexpr const char *PIPE_ASSET_PATH = "/APPS/pipe-pipedreamer.pak";
static constexpr uint32_t PIPE_SAVE_MAGIC = 0x45504950u;
static constexpr uint16_t PIPE_SAVE_VERSION = 1u;
static constexpr int PIPE_BOARD_COLS = 10;
static constexpr int PIPE_BOARD_ROWS = 7;
static constexpr int PIPE_QUEUE_SIZE = 5;
static constexpr int PIPE_CELL = 22;
static constexpr int PIPE_BOARD_X = 78;
static constexpr int PIPE_BOARD_Y = 48;
static constexpr int PIPE_QUEUE_X = 12;
static constexpr int PIPE_QUEUE_Y = 54;
static constexpr int PIPE_MIN_SCORE_TO_ADVANCE = 200;

struct PipeSave {
	uint32_t magic;
	uint16_t version;
	uint16_t size;
	int32_t level;
	int32_t cumulativeScore;
	int32_t highScore;
};

struct PipeGame {
	const struct DcAppHostApi *host;
	const struct DcAppRunArgs *args;
	struct DcAppDrawCtx draw;
	uint8_t *frame;
	Randomizer *randomizer;
	Board *board;
	Queue *queue;
	int difficultyLevel;
	int cumulativeScore;
	int highScore;
	int countDown;
	int blockInteraction;
	int cursorCol;
	int cursorRow;
	bool fastForward;
	bool stopped;
	bool running;
	char banner[40];
	int bannerFrames;
};

static volatile bool mPipeAbort;

void *operator new(size_t size)
{
	return dc32PortMalloc(size ? size : 1u);
}

void *operator new[](size_t size)
{
	return dc32PortMalloc(size ? size : 1u);
}

void operator delete(void *ptr) noexcept
{
	dc32PortFree(ptr);
}

void operator delete[](void *ptr) noexcept
{
	dc32PortFree(ptr);
}

void operator delete(void *ptr, size_t) noexcept
{
	dc32PortFree(ptr);
}

void operator delete[](void *ptr, size_t) noexcept
{
	dc32PortFree(ptr);
}

static uint16_t pipeRgb(uint32_t r, uint32_t g, uint32_t b)
{
	return dcAppDrawRgb565(r, g, b);
}

static uint32_t pipeTextWidth(const char *text, enum Font font)
{
	uint32_t w = 0;
	struct FontGlyphInfo glyph;

	if (!text)
		return 0;
	while (*text) {
		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text))
			w += glyph.width + 1u;
		text++;
	}
	return w;
}

static void pipeDrawText(struct DcAppDrawCtx *draw, int32_t x, int32_t y,
	const char *text, enum Font font, uint16_t color)
{
	struct FontGlyphInfo glyph;

	if (!draw || !text)
		return;
	while (*text) {
		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text)) {
			for (uint32_t row = 0; row < glyph.height; row++) {
				for (uint32_t col = 0; col < glyph.width; col++) {
					if (fontGetGlyphPixel(&glyph, row, col))
						dcAppDrawPixel(draw, x + (int32_t)col, y + (int32_t)row, color);
				}
			}
			x += (int32_t)(glyph.width + 1u);
		}
		text++;
	}
}

static void pipeDrawCentered(struct DcAppDrawCtx *draw, int32_t y, const char *text, enum Font font, uint16_t color)
{
	int32_t x = (int32_t)((PIPE_SCREEN_W - pipeTextWidth(text, font)) / 2u);

	pipeDrawText(draw, x, y, text, font, color);
}

static void pipeBanner(struct PipeGame *game, const char *text)
{
	if (!game)
		return;
	snprintf(game->banner, sizeof(game->banner), "%s", text ? text : "");
	game->bannerFrames = 60;
}

static int pipeLevelIndex(const struct PipeGame *game)
{
	int idx = game->difficultyLevel - 1;

	if (idx < 0)
		idx = 0;
	if (idx > 11)
		idx = 11;
	return idx;
}

static float pipeCurrentOozePerPump(const struct PipeGame *game)
{
	static const float oozePerLevel[] = {
		1.0F, 1.2F, 1.4F, 1.5F, 1.6F, 1.8F,
		2.0F, 2.2F, 2.5F, 3.0F, 3.5F, 5.0F,
	};
	float amount = oozePerLevel[pipeLevelIndex(game)];

	if (game->fastForward)
		amount *= 10.0F;
	return amount;
}

static int pipeCurrentCountdown(const struct PipeGame *game)
{
	static const int countdownPerLevel[] = {
		320, 290, 260, 230, 200, 180,
		160, 140, 120, 100, 80, 60,
	};

	return countdownPerLevel[pipeLevelIndex(game)];
}

static int pipeBonus(const struct PipeGame *game)
{
	if (game->difficultyLevel <= 1)
		return 0;
	return game->difficultyLevel * game->difficultyLevel * 15;
}

static int pipeRoundTotal(const struct PipeGame *game)
{
	return game->cumulativeScore + pipeBonus(game) + game->board->GetScoreValue();
}

static bool pipeCanAdvance(const struct PipeGame *game)
{
	return game->board->GetScoreValue() >= PIPE_MIN_SCORE_TO_ADVANCE;
}

static void pipeSaveGame(struct PipeGame *game)
{
	struct PipeSave save;

	if (!game || !game->args || !game->args->vol)
		return;
	memset(&save, 0, sizeof(save));
	save.magic = PIPE_SAVE_MAGIC;
	save.version = PIPE_SAVE_VERSION;
	save.size = sizeof(save);
	save.level = game->difficultyLevel;
	save.cumulativeScore = game->cumulativeScore;
	save.highScore = game->highScore;
	(void)dc32PortSaveWrite(game->args->vol, "pipe", &save, sizeof(save));
}

static void pipeRestoreSave(struct PipeGame *game)
{
	struct PipeSave save;

	game->difficultyLevel = 1;
	game->cumulativeScore = 0;
	game->highScore = 0;
	if (!game || !game->args || !game->args->vol ||
			!dc32PortSaveRead(game->args->vol, "pipe", &save, sizeof(save)) ||
			save.magic != PIPE_SAVE_MAGIC || save.version != PIPE_SAVE_VERSION ||
			save.size != sizeof(save))
		return;
	if (save.level >= 1 && save.level <= 99)
		game->difficultyLevel = save.level;
	if (save.cumulativeScore >= 0)
		game->cumulativeScore = save.cumulativeScore;
	if (save.highScore >= 0)
		game->highScore = save.highScore;
}

static void pipeResetRound(struct PipeGame *game, bool restart, bool advance)
{
	if (!game || !game->board || !game->queue)
		return;
	if (restart) {
		game->difficultyLevel = 1;
		game->cumulativeScore = 0;
	}
	else if (advance) {
		game->cumulativeScore = pipeRoundTotal(game);
		game->difficultyLevel++;
	}
	game->board->Reset();
	game->queue->Reset();
	game->countDown = pipeCurrentCountdown(game);
	game->fastForward = false;
	game->stopped = false;
	game->blockInteraction = 0;
	game->cursorCol = PIPE_BOARD_COLS / 2;
	game->cursorRow = PIPE_BOARD_ROWS / 2;
	pipeBanner(game, restart ? "RESTART" : (advance ? "NEXT LEVEL" : "READY"));
}

static bool pipePlaceAt(struct PipeGame *game, int col, int row, bool forceBomb)
{
	TilePiece *tile;
	Pipe *pipe;
	bool replace;

	if (!game || game->stopped || game->blockInteraction > 0 ||
			col < 0 || col >= PIPE_BOARD_COLS || row < 0 || row >= PIPE_BOARD_ROWS)
		return false;
	tile = game->board->GetTile(col, row);
	replace = tile->GetType() == TilePiece::TYPE_NONE;
	if (!replace) {
		pipe = dynamic_cast<Pipe*>(tile);
		replace = forceBomb && pipe && pipe->IsEmpty() && !pipe->IsStart() && game->board->PopBomb();
	}
	if (!replace)
		return false;
	game->board->ReplaceTile(col, row, game->queue->Pop());
	game->blockInteraction = 5;
	pipeBanner(game, forceBomb ? "BOMB" : "PIPE");
	return true;
}

static void pipeDrawRect(struct DcAppDrawCtx *draw, int x, int y, int w, int h, uint16_t color)
{
	if (w > 0 && h > 0)
		dcAppDrawFill(draw, x, y, w, h, color);
}

static void pipeDrawSegment(struct DcAppDrawCtx *draw, int x, int y, Pipe::Direction dir, uint16_t color, int width)
{
	int c = PIPE_CELL / 2;
	int half = width / 2;

	switch (dir) {
		case Pipe::DIR_N:
			pipeDrawRect(draw, x + c - half, y + 2, width, c + 1, color);
			break;
		case Pipe::DIR_S:
			pipeDrawRect(draw, x + c - half, y + c, width, c, color);
			break;
		case Pipe::DIR_E:
			pipeDrawRect(draw, x + c, y + c - half, c, width, color);
			break;
		case Pipe::DIR_W:
			pipeDrawRect(draw, x + 2, y + c - half, c + 1, width, color);
			break;
		default:
			break;
	}
}

static void pipeDrawCore(struct DcAppDrawCtx *draw, int x, int y, uint16_t color, int width)
{
	int c = PIPE_CELL / 2;
	int half = width / 2;

	pipeDrawRect(draw, x + c - half, y + c - half, width, width, color);
}

static void pipeDrawPipeShape(struct DcAppDrawCtx *draw, int x, int y, TilePiece::Type type, uint16_t color, int width)
{
	switch (type) {
		case TilePiece::TYPE_START_N:
			pipeDrawSegment(draw, x, y, Pipe::DIR_N, color, width);
			pipeDrawCore(draw, x, y, color, width + 2);
			break;
		case TilePiece::TYPE_START_S:
			pipeDrawSegment(draw, x, y, Pipe::DIR_S, color, width);
			pipeDrawCore(draw, x, y, color, width + 2);
			break;
		case TilePiece::TYPE_START_E:
			pipeDrawSegment(draw, x, y, Pipe::DIR_E, color, width);
			pipeDrawCore(draw, x, y, color, width + 2);
			break;
		case TilePiece::TYPE_START_W:
			pipeDrawSegment(draw, x, y, Pipe::DIR_W, color, width);
			pipeDrawCore(draw, x, y, color, width + 2);
			break;
		case TilePiece::TYPE_VERTICAL:
			pipeDrawSegment(draw, x, y, Pipe::DIR_N, color, width);
			pipeDrawSegment(draw, x, y, Pipe::DIR_S, color, width);
			pipeDrawCore(draw, x, y, color, width);
			break;
		case TilePiece::TYPE_HORIZONTAL:
			pipeDrawSegment(draw, x, y, Pipe::DIR_E, color, width);
			pipeDrawSegment(draw, x, y, Pipe::DIR_W, color, width);
			pipeDrawCore(draw, x, y, color, width);
			break;
		case TilePiece::TYPE_NW_ELBOW:
			pipeDrawSegment(draw, x, y, Pipe::DIR_N, color, width);
			pipeDrawSegment(draw, x, y, Pipe::DIR_W, color, width);
			pipeDrawCore(draw, x, y, color, width);
			break;
		case TilePiece::TYPE_NE_ELBOW:
			pipeDrawSegment(draw, x, y, Pipe::DIR_N, color, width);
			pipeDrawSegment(draw, x, y, Pipe::DIR_E, color, width);
			pipeDrawCore(draw, x, y, color, width);
			break;
		case TilePiece::TYPE_SE_ELBOW:
			pipeDrawSegment(draw, x, y, Pipe::DIR_S, color, width);
			pipeDrawSegment(draw, x, y, Pipe::DIR_E, color, width);
			pipeDrawCore(draw, x, y, color, width);
			break;
		case TilePiece::TYPE_SW_ELBOW:
			pipeDrawSegment(draw, x, y, Pipe::DIR_S, color, width);
			pipeDrawSegment(draw, x, y, Pipe::DIR_W, color, width);
			pipeDrawCore(draw, x, y, color, width);
			break;
		case TilePiece::TYPE_CROSS:
			pipeDrawSegment(draw, x, y, Pipe::DIR_N, color, width);
			pipeDrawSegment(draw, x, y, Pipe::DIR_S, color, width);
			pipeDrawSegment(draw, x, y, Pipe::DIR_E, color, width);
			pipeDrawSegment(draw, x, y, Pipe::DIR_W, color, width);
			pipeDrawCore(draw, x, y, color, width);
			break;
		default:
			break;
	}
}

static void pipeDrawTile(struct DcAppDrawCtx *draw, TilePiece *tile, int x, int y, bool selected)
{
	uint16_t cell = pipeRgb(62, 66, 62);
	uint16_t grid = pipeRgb(196, 202, 195);
	uint16_t shadow = pipeRgb(10, 11, 10);
	uint16_t metal = pipeRgb(126, 132, 124);
	uint16_t ooze = pipeRgb(0, 236, 0);
	uint16_t source = pipeRgb(224, 224, 96);
	Pipe *pipe = dynamic_cast<Pipe*>(tile);

	pipeDrawRect(draw, x, y, PIPE_CELL, PIPE_CELL, cell);
	dcAppDrawLine(draw, x, y, x + PIPE_CELL - 1, y, grid);
	dcAppDrawLine(draw, x, y, x, y + PIPE_CELL - 1, grid);
	if (!tile || tile->GetType() == TilePiece::TYPE_NONE)
		goto cursor;
	pipeDrawPipeShape(draw, x, y, tile->GetType(), shadow, 15);
	pipeDrawPipeShape(draw, x, y, tile->GetType(), tile->IsStart() ? source : metal, 9);
	if (pipe) {
		Cross *cross = dynamic_cast<Cross*>(pipe);

		if (cross) {
			if (cross->GetOozeLevel(Cross::WAY_HORIZONTAL) > MIN_OOZE_LEVEL) {
				pipeDrawSegment(draw, x, y, Pipe::DIR_E, ooze, 5);
				pipeDrawSegment(draw, x, y, Pipe::DIR_W, ooze, 5);
				pipeDrawCore(draw, x, y, ooze, 5);
			}
			if (cross->GetOozeLevel(Cross::WAY_VERTICAL) > MIN_OOZE_LEVEL) {
				pipeDrawSegment(draw, x, y, Pipe::DIR_N, ooze, 5);
				pipeDrawSegment(draw, x, y, Pipe::DIR_S, ooze, 5);
				pipeDrawCore(draw, x, y, ooze, 5);
			}
		}
		else if (pipe->GetOozeLevel() > MIN_OOZE_LEVEL || tile->IsStart()) {
			pipeDrawPipeShape(draw, x, y, tile->GetType(), ooze, 5);
		}
		if (pipe->PopExplosion() > 0) {
			pipeDrawRect(draw, x + 4, y + 4, PIPE_CELL - 8, PIPE_CELL - 8, pipeRgb(240, 68, 32));
			pipeDrawRect(draw, x + 7, y + 7, PIPE_CELL - 14, PIPE_CELL - 14, pipeRgb(255, 220, 64));
		}
	}

cursor:
	if (selected) {
		uint16_t color = pipeRgb(255, 230, 80);

		dcAppDrawLine(draw, x - 1, y - 1, x + PIPE_CELL, y - 1, color);
		dcAppDrawLine(draw, x - 1, y + PIPE_CELL, x + PIPE_CELL, y + PIPE_CELL, color);
		dcAppDrawLine(draw, x - 1, y - 1, x - 1, y + PIPE_CELL, color);
		dcAppDrawLine(draw, x + PIPE_CELL, y - 1, x + PIPE_CELL, y + PIPE_CELL, color);
	}
}

static void pipeDrawOozeMeter(struct PipeGame *game)
{
	int x = 10;
	int y = 20;
	int h = 72;
	int filled = 0;

	pipeDrawRect(&game->draw, x, y, 12, h, pipeRgb(20, 24, 20));
	dcAppDrawLine(&game->draw, x - 1, y - 1, x + 12, y - 1, pipeRgb(190, 190, 180));
	dcAppDrawLine(&game->draw, x - 1, y + h, x + 12, y + h, pipeRgb(190, 190, 180));
	dcAppDrawLine(&game->draw, x - 1, y - 1, x - 1, y + h, pipeRgb(190, 190, 180));
	dcAppDrawLine(&game->draw, x + 12, y - 1, x + 12, y + h, pipeRgb(190, 190, 180));
	if (game->countDown > 0) {
		int max = pipeCurrentCountdown(game);

		filled = h - (game->countDown * h) / (max ? max : 1);
	}
	else if (game->board->GetScoreValue() < PIPE_MIN_SCORE_TO_ADVANCE) {
		filled = (game->board->GetScoreValue() * h) / PIPE_MIN_SCORE_TO_ADVANCE;
	}
	else {
		filled = h;
	}
	if (filled < 0)
		filled = 0;
	if (filled > h)
		filled = h;
	pipeDrawRect(&game->draw, x + 2, y + h - filled, 8, filled, pipeRgb(0, 236, 0));
}

static void pipeDrawHud(struct PipeGame *game)
{
	char text[48];
	uint16_t white = pipeRgb(236, 238, 226);
	uint16_t dim = pipeRgb(170, 176, 164);
	uint16_t accent = pipeRgb(0, 236, 0);

	snprintf(text, sizeof(text), "PIPE DREAM");
	pipeDrawText(&game->draw, 34, 8, text, FontMedium, accent);
	snprintf(text, sizeof(text), "LV %d  SCORE %d", game->difficultyLevel, pipeRoundTotal(game));
	pipeDrawText(&game->draw, 124, 8, text, FontSmall, white);
	snprintf(text, sizeof(text), "BEST %d", game->highScore);
	pipeDrawText(&game->draw, 246, 8, text, FontSmall, dim);
	snprintf(text, sizeof(text), "BOMBS %d", game->board->GetNumBombs());
	pipeDrawText(&game->draw, 252, 48, text, FontSmall, white);
	snprintf(text, sizeof(text), "%s", game->fastForward ? "FAST" : "NORMAL");
	pipeDrawText(&game->draw, 252, 66, text, FontSmall, game->fastForward ? accent : dim);
	if (game->countDown > 0) {
		snprintf(text, sizeof(text), "OOZE %d", game->countDown);
		pipeDrawText(&game->draw, 252, 84, text, FontSmall, dim);
	}
	if (game->stopped) {
		snprintf(text, sizeof(text), pipeCanAdvance(game) ? "START NEXT" : "START RETRY");
		pipeDrawText(&game->draw, 232, 190, text, FontSmall, white);
		pipeDrawText(&game->draw, 232, 206, "B RESTART", FontSmall, dim);
	}
	else {
		pipeDrawText(&game->draw, 232, 190, "A PLACE", FontSmall, dim);
		pipeDrawText(&game->draw, 232, 206, "B BOMB", FontSmall, dim);
		pipeDrawText(&game->draw, 232, 222, "SEL FAST", FontSmall, dim);
	}
	if (game->bannerFrames > 0) {
		pipeDrawCentered(&game->draw, 225, game->banner, FontSmall, white);
		game->bannerFrames--;
	}
}

static void pipeDrawQueue(struct PipeGame *game)
{
	pipeDrawText(&game->draw, PIPE_QUEUE_X, 38, "QUEUE", FontSmall, pipeRgb(170, 176, 164));
	for (int i = 0; i < game->queue->GetSize(); i++) {
		int y = PIPE_QUEUE_Y + i * (PIPE_CELL + 4);
		Pipe *tile = game->queue->GetTile(game->queue->GetSize() - i - 1);

		pipeDrawTile(&game->draw, tile, PIPE_QUEUE_X, y, false);
	}
}

static void pipeDrawBoard(struct PipeGame *game)
{
	for (int row = 0; row < PIPE_BOARD_ROWS; row++) {
		for (int col = 0; col < PIPE_BOARD_COLS; col++) {
			int x = PIPE_BOARD_X + col * PIPE_CELL;
			int y = PIPE_BOARD_Y + row * PIPE_CELL;

			pipeDrawTile(&game->draw, game->board->GetTile(col, row), x, y,
				col == game->cursorCol && row == game->cursorRow);
		}
	}
}

static void pipeDraw(struct PipeGame *game)
{
	dcAppDrawClear(&game->draw, pipeRgb(36, 40, 36));
	pipeDrawOozeMeter(game);
	pipeDrawQueue(game);
	pipeDrawBoard(game);
	pipeDrawHud(game);
	dcAppDrawPresent(&game->draw);
}

static void pipeHandleRoundEnd(struct PipeGame *game)
{
	int total = pipeRoundTotal(game);

	if (total > game->highScore)
		game->highScore = total;
	game->stopped = true;
	pipeSaveGame(game);
	pipeBanner(game, pipeCanAdvance(game) ? "LEVEL CLEAR" : "SPILL");
}

static void pipeTick(struct PipeGame *game)
{
	if (!game || game->stopped)
		return;
	if (game->blockInteraction > 0)
		game->blockInteraction--;
	if (game->countDown > 0) {
		game->countDown -= game->fastForward ? 5 : 1;
		if (game->countDown < 0)
			game->countDown = 0;
		return;
	}
	if (!game->board->Pump(pipeCurrentOozePerPump(game)))
		pipeHandleRoundEnd(game);
}

static void pipeHandleInput(struct PipeGame *game)
{
	uint_fast16_t pressed = game->draw.pressed;

	if (pressed & UI_KEY_BIT_CENTER) {
		game->running = false;
		return;
	}
	if (pressed & KEY_BIT_UP)
		game->cursorRow = game->cursorRow > 0 ? game->cursorRow - 1 : PIPE_BOARD_ROWS - 1;
	if (pressed & KEY_BIT_DOWN)
		game->cursorRow = (game->cursorRow + 1) % PIPE_BOARD_ROWS;
	if (pressed & KEY_BIT_LEFT)
		game->cursorCol = game->cursorCol > 0 ? game->cursorCol - 1 : PIPE_BOARD_COLS - 1;
	if (pressed & KEY_BIT_RIGHT)
		game->cursorCol = (game->cursorCol + 1) % PIPE_BOARD_COLS;
	if (pressed & KEY_BIT_SEL) {
		game->fastForward = !game->fastForward;
		pipeBanner(game, game->fastForward ? "FAST FORWARD" : "NORMAL SPEED");
	}
	if (game->stopped) {
		if (pressed & KEY_BIT_START)
			pipeResetRound(game, false, pipeCanAdvance(game));
		if (pressed & KEY_BIT_B)
			pipeResetRound(game, true, false);
		return;
	}
	if (pressed & KEY_BIT_START)
		pipeResetRound(game, true, false);
	if (pressed & KEY_BIT_A)
		(void)pipePlaceAt(game, game->cursorCol, game->cursorRow, false);
	if (pressed & KEY_BIT_B)
		(void)pipePlaceAt(game, game->cursorCol, game->cursorRow, true);
}

static bool pipeAssetsPresent(const struct DcAppRunArgs *args)
{
	struct Dc32PortPak pak;
	bool ok;

	if (!args || !args->vol)
		return false;
	ok = dc32PortOpenAssetPack(args->vol, PIPE_ASSET_PATH, &pak);
	if (ok)
		dc32PortCloseAssetPack(&pak);
	return ok;
}

extern "C" int pipeAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct PipeGame *game;

	mPipeAbort = false;
	dc32PortHeapInitDefault();
	game = static_cast<struct PipeGame*>(dc32PortCalloc(1, sizeof(*game)));
	if (!game)
		return -1;
	game->frame = static_cast<uint8_t*>(dc32PortMalloc(PIPE_SCREEN_W * PIPE_SCREEN_H));
	if (!game->frame)
		return -1;
	game->host = host;
	game->args = args;
	if (!dcAppDrawInit(&game->draw, host, args, game->frame, PIPE_SCREEN_W, PIPE_SCREEN_H))
		return -1;
	if (!pipeAssetsPresent(args)) {
		dc32PortShowMissingData(host, args, "Pipe Dream assets", PIPE_ASSET_PATH, game->frame);
		return 0;
	}
	game->randomizer = Randomizer::GetInstance();
	game->board = new Board(PIPE_BOARD_COLS, PIPE_BOARD_ROWS);
	game->queue = new Queue(PIPE_QUEUE_SIZE);
	if (!game->randomizer || !game->board || !game->queue) {
		dc32PortShowMissingData(host, args, "Pipe Dream heap", PIPE_ASSET_PATH, game->frame);
		return 0;
	}
	pipeRestoreSave(game);
	pipeResetRound(game, false, false);
	dcAppDrawWaitRelease(&game->draw, KEY_BIT_A | KEY_BIT_B | KEY_BIT_START | KEY_BIT_SEL | UI_KEY_BIT_CENTER);
	dispSetFramerate(60);
	game->running = true;
	while (game->running && !mPipeAbort) {
		pipeDraw(game);
		if (!dcAppDrawFrame(&game->draw, UI_KEY_BIT_CENTER))
			break;
		pipeHandleInput(game);
		pipeTick(game);
	}
	pipeSaveGame(game);
	delete game->queue;
	delete game->board;
	delete game->randomizer;
	dcAppDrawWaitRelease(&game->draw, UI_KEY_BIT_CENTER);
	return 0;
}

extern "C" void pipeAppAbort(void)
{
	mPipeAbort = true;
}

extern "C" int dcAppEntry(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	return pipeAppRun(host, args);
}

extern "C" void dcAppAbort(void)
{
	pipeAppAbort();
}

extern "C" void dcAppRefreshDisplayOptions(void)
{
}
