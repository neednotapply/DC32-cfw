#pragma GCC optimize ("Os")
#include "apps/picoware_app.h"

#include <math.h>
#include <stdint.h>
#include <string.h>
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"

#define PICOWARE_SCREEN_W 320u
#define PICOWARE_SCREEN_H 240u
#define PICOWARE_FPS 30u

struct PicowareAppCtx {
	const struct DcAppHostApi *host;
	struct DcAppDrawCtx draw;
	uint_fast16_t keys;
	uint_fast16_t pressed;
	uint32_t rng;
	uint32_t frame;
};

static uint8_t mPicowareBackbuffer[PICOWARE_SCREEN_W * PICOWARE_SCREEN_H];

static uint32_t pwMinU32(uint32_t a, uint32_t b)
{
	return a < b ? a : b;
}

static uint32_t pwRand(struct PicowareAppCtx *ctx)
{
	ctx->rng = ctx->rng * 1664525u + 1013904223u;
	return ctx->rng;
}

static uint16_t pwRgb(uint32_t r, uint32_t g, uint32_t b)
{
	return dcAppDrawRgb565(r, g, b);
}

static void pwClear(struct PicowareAppCtx *ctx, uint16_t color)
{
	dcAppDrawClear(&ctx->draw, color);
}

static void pwFill(struct PicowareAppCtx *ctx, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
	dcAppDrawFill(&ctx->draw, x, y, w, h, color);
}

static void pwPixel(struct PicowareAppCtx *ctx, int32_t x, int32_t y, uint16_t color)
{
	dcAppDrawPixel(&ctx->draw, x, y, color);
}

static void pwLine(struct PicowareAppCtx *ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
{
	dcAppDrawLine(&ctx->draw, x0, y0, x1, y1, color);
}

static uint32_t pwTextWidth(const char *text, enum Font font)
{
	uint32_t width = 0;

	while (*text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text))
			width += glyph.width + 1u;
		text++;
	}
	return width;
}

static void pwText(struct PicowareAppCtx *ctx, int32_t x, int32_t y, const char *text, enum Font font, uint16_t color)
{
	while (*text) {
		struct FontGlyphInfo glyph;
		uint_fast8_t row, col;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text)) {
			for (row = 0; row < glyph.height; row++)
				for (col = 0; col < glyph.width; col++)
					if (fontGetGlyphPixel(&glyph, row, col))
						pwPixel(ctx, x + col, y + row, color);
			x += glyph.width + 1;
		}
		text++;
	}
}

static void pwTextCentered(struct PicowareAppCtx *ctx, int32_t y, const char *text, enum Font font, uint16_t color)
{
	uint32_t width = pwTextWidth(text, font);

	pwText(ctx, (int32_t)((PICOWARE_SCREEN_W > width ? PICOWARE_SCREEN_W - width : 0u) / 2u), y, text, font, color);
}

static void pwU32ToText(uint32_t value, char *buf, uint32_t bufSz)
{
	char tmp[12];
	uint32_t n = 0, i = 0;

	if (!bufSz)
		return;
	if (!value)
		tmp[n++] = '0';
	while (value && n < sizeof(tmp)) {
		tmp[n++] = (char)('0' + value % 10u);
		value /= 10u;
	}
	while (i + 1u < bufSz && n)
		buf[i++] = tmp[--n];
	buf[i] = 0;
}

static void pwDrawScore(struct PicowareAppCtx *ctx, uint32_t score)
{
	char buf[16];

	pwU32ToText(score, buf, sizeof(buf));
	pwText(ctx, 8, 8, buf, FontMedium, pwRgb(255, 255, 255));
}

static bool pwFrame(struct PicowareAppCtx *ctx)
{
	bool running = dcAppDrawFrame(&ctx->draw, UI_KEY_BIT_CENTER);

	ctx->keys = ctx->draw.keys;
	ctx->pressed = ctx->draw.pressed;
	ctx->frame = ctx->draw.frame;
	return running;
}

static void pwWaitRelease(struct PicowareAppCtx *ctx)
{
	dcAppDrawWaitRelease(&ctx->draw, KEY_BIT_A | KEY_BIT_B | UI_KEY_BIT_CENTER);
}

static bool pwInit(struct PicowareAppCtx *ctx, const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	uint64_t now = host && host->getTime ? host->getTime() : 0x1234abcd5678ef00ull;

	memset(ctx, 0, sizeof(*ctx));
	ctx->host = host;
	ctx->rng = (uint32_t)(now ^ (now >> 32));
	return dcAppDrawInit(&ctx->draw, host, args, mPicowareBackbuffer, PICOWARE_SCREEN_W, PICOWARE_SCREEN_H);
}

#define TET_W 10
#define TET_H 18
#define TET_CELL 12
static const uint16_t mTetrominoes[7][4] = {
	{0x0f00, 0x2222, 0x00f0, 0x4444},
	{0x0660, 0x0660, 0x0660, 0x0660},
	{0x0e40, 0x4c40, 0x4e00, 0x4640},
	{0x0c60, 0x2640, 0x0c60, 0x2640},
	{0x06c0, 0x4620, 0x06c0, 0x4620},
	{0x08e0, 0x6440, 0x0e20, 0x44c0},
	{0x02e0, 0x4460, 0x0e80, 0xc440},
};
static const uint16_t mTetColors[7] = {
	0x67ff, 0xffe0, 0xf81f, 0x07e0, 0xf800, 0xfd20, 0x001f,
};

static bool tetCell(uint16_t mask, int32_t x, int32_t y)
{
	return (mask & (1u << (15 - (y * 4 + x)))) != 0;
}

static bool tetCollides(uint8_t board[TET_H][TET_W], int32_t px, int32_t py, uint8_t piece, uint8_t rot)
{
	uint16_t mask = mTetrominoes[piece][rot & 3u];

	for (int32_t y = 0; y < 4; y++) {
		for (int32_t x = 0; x < 4; x++) {
			int32_t bx, by;

			if (!tetCell(mask, x, y))
				continue;
			bx = px + x;
			by = py + y;
			if (bx < 0 || bx >= TET_W || by >= TET_H)
				return true;
			if (by >= 0 && board[by][bx])
				return true;
		}
	}
	return false;
}

static void tetPlace(uint8_t board[TET_H][TET_W], int32_t px, int32_t py, uint8_t piece, uint8_t rot)
{
	uint16_t mask = mTetrominoes[piece][rot & 3u];

	for (int32_t y = 0; y < 4; y++)
		for (int32_t x = 0; x < 4; x++)
			if (tetCell(mask, x, y) && py + y >= 0 && py + y < TET_H && px + x >= 0 && px + x < TET_W)
				board[py + y][px + x] = piece + 1u;
}

static uint32_t tetClearLines(uint8_t board[TET_H][TET_W])
{
	uint32_t lines = 0;

	for (int32_t y = TET_H - 1; y >= 0; y--) {
		bool full = true;

		for (int32_t x = 0; x < TET_W; x++)
			if (!board[y][x])
				full = false;
		if (!full)
			continue;
		for (int32_t row = y; row > 0; row--)
			memcpy(board[row], board[row - 1], TET_W);
		memset(board[0], 0, TET_W);
		y++;
		lines++;
	}
	return lines;
}

static void tetDraw(struct PicowareAppCtx *ctx, uint8_t board[TET_H][TET_W], int32_t px, int32_t py, uint8_t piece, uint8_t rot, uint32_t score)
{
	int32_t left = 74, top = 18;
	uint16_t mask = mTetrominoes[piece][rot & 3u];

	pwClear(ctx, pwRgb(6, 10, 18));
	pwFill(ctx, left - 3, top - 3, TET_W * TET_CELL + 6, TET_H * TET_CELL + 6, pwRgb(45, 55, 70));
	pwFill(ctx, left, top, TET_W * TET_CELL, TET_H * TET_CELL, pwRgb(0, 0, 0));
	for (int32_t y = 0; y < TET_H; y++)
		for (int32_t x = 0; x < TET_W; x++)
			if (board[y][x])
				pwFill(ctx, left + x * TET_CELL + 1, top + y * TET_CELL + 1, TET_CELL - 2, TET_CELL - 2, mTetColors[board[y][x] - 1]);
	for (int32_t y = 0; y < 4; y++)
		for (int32_t x = 0; x < 4; x++)
			if (tetCell(mask, x, y) && py + y >= 0)
				pwFill(ctx, left + (px + x) * TET_CELL + 1, top + (py + y) * TET_CELL + 1, TET_CELL - 2, TET_CELL - 2, mTetColors[piece]);
	pwText(ctx, 220, 40, "Tetris", FontLarge, pwRgb(255, 255, 255));
	pwDrawScore(ctx, score);
}

static int pwRunTetris(struct PicowareAppCtx *ctx)
{
	static uint8_t board[TET_H][TET_W];
	int32_t px = 3, py = -1;
	uint8_t piece = 0, rot = 0;
	uint32_t score = 0, drop = 0;

	memset(board, 0, sizeof(board));
	piece = (uint8_t)(pwRand(ctx) % 7u);
	while (pwFrame(ctx)) {
		if ((ctx->pressed & KEY_BIT_LEFT) && !tetCollides(board, px - 1, py, piece, rot))
			px--;
		if ((ctx->pressed & KEY_BIT_RIGHT) && !tetCollides(board, px + 1, py, piece, rot))
			px++;
		if ((ctx->pressed & KEY_BIT_A) && !tetCollides(board, px, py, piece, rot + 1u))
			rot++;
		if ((ctx->keys & KEY_BIT_DOWN) || ++drop >= pwMinU32(18u, 28u - score / 8u)) {
			drop = 0;
			if (!tetCollides(board, px, py + 1, piece, rot)) {
				py++;
			}
			else {
				tetPlace(board, px, py, piece, rot);
				score += 10u + tetClearLines(board) * 100u;
				px = 3;
				py = -1;
				rot = 0;
				piece = (uint8_t)(pwRand(ctx) % 7u);
				if (tetCollides(board, px, py, piece, rot)) {
					memset(board, 0, sizeof(board));
					score = 0;
				}
			}
		}
		tetDraw(ctx, board, px, py, piece, rot, score);
	}
	return 0;
}

static int pwRunFlappy(struct PicowareAppCtx *ctx)
{
	int32_t birdY = 1100, birdV = 0, pipeX[3] = {360, 500, 640}, gapY[3] = {80, 130, 100};
	uint32_t score = 0;

	while (pwFrame(ctx)) {
		if (ctx->pressed & (KEY_BIT_A | KEY_BIT_UP))
			birdV = -72;
		birdV += 5;
		birdY += birdV;
		for (int32_t i = 0; i < 3; i++) {
			pipeX[i] -= 4;
			if (pipeX[i] < -32) {
				pipeX[i] = 392;
				gapY[i] = 44 + (int32_t)(pwRand(ctx) % 118u);
			}
			if (pipeX[i] == 40)
				score++;
			if (pipeX[i] < 64 && pipeX[i] + 30 > 34) {
				int32_t by = birdY / 10;

				if (by < gapY[i] || by > gapY[i] + 56) {
					birdY = 1100;
					birdV = 0;
					score = 0;
					pipeX[0] = 360;
					pipeX[1] = 500;
					pipeX[2] = 640;
				}
			}
		}
		if (birdY < 0 || birdY > 2240) {
			birdY = 1100;
			birdV = 0;
			score = 0;
		}
		pwClear(ctx, pwRgb(70, 185, 230));
		pwFill(ctx, 0, 224, 320, 16, pwRgb(80, 190, 80));
		for (int32_t i = 0; i < 3; i++) {
			pwFill(ctx, pipeX[i], 0, 30, gapY[i], pwRgb(20, 130, 70));
			pwFill(ctx, pipeX[i], gapY[i] + 58, 30, 224 - gapY[i] - 58, pwRgb(20, 130, 70));
		}
		pwFill(ctx, 38, birdY / 10, 18, 14, pwRgb(255, 220, 60));
		pwFill(ctx, 50, birdY / 10 + 4, 6, 4, pwRgb(255, 130, 40));
		pwDrawScore(ctx, score);
	}
	return 0;
}

static const char mMaze[12][17] = {
	"################",
	"#S   #     #   #",
	"### # # ### # ##",
	"#   # #   # #  #",
	"# ### ### # ## #",
	"# #     # #    #",
	"# # ### # #### #",
	"#   # # #      #",
	"### # # ###### #",
	"#   #         G#",
	"# ############ #",
	"################",
};

static void pwLabyrinthWin(struct PicowareAppCtx *ctx, uint32_t score)
{
	pwClear(ctx, pwRgb(3, 18, 10));
	pwTextCentered(ctx, 82, "Goal!", FontLarge, pwRgb(90, 255, 130));
	pwTextCentered(ctx, 124, "Resetting maze", FontMedium, pwRgb(210, 230, 210));
	pwDrawScore(ctx, score);
	for (uint_fast8_t i = 0; i < 24; i++)
		if (!pwFrame(ctx))
			return;
}

static int pwRunLabyrinth(struct PicowareAppCtx *ctx)
{
	int32_t px = 1, py = 1;
	uint32_t score = 0;

	while (pwFrame(ctx)) {
		int32_t nx = px, ny = py;

		if (ctx->pressed & KEY_BIT_LEFT)
			nx--;
		if (ctx->pressed & KEY_BIT_RIGHT)
			nx++;
		if (ctx->pressed & KEY_BIT_UP)
			ny--;
		if (ctx->pressed & KEY_BIT_DOWN)
			ny++;
		if (mMaze[ny][nx] != '#') {
			px = nx;
			py = ny;
			score++;
		}
		if (mMaze[py][px] == 'G') {
			pwLabyrinthWin(ctx, score);
			px = 1;
			py = 1;
			score = 0;
		}
		pwClear(ctx, pwRgb(5, 5, 10));
		for (int32_t y = 0; y < 12; y++)
			for (int32_t x = 0; x < 16; x++) {
				uint16_t color = mMaze[y][x] == '#' ? pwRgb(40, 100, 180) : pwRgb(10, 18, 28);

				if (mMaze[y][x] == 'G')
					color = pwRgb(40, 180, 80);
				pwFill(ctx, 16 + x * 18, 12 + y * 18, 17, 17, color);
			}
		pwFill(ctx, 16 + px * 18 + 4, 12 + py * 18 + 4, 9, 9, pwRgb(255, 230, 80));
		pwDrawScore(ctx, score);
	}
	return 0;
}

static int pwRunStarfield(struct PicowareAppCtx *ctx)
{
	static int16_t sx[96], sy[96], sz[96];

	for (int32_t i = 0; i < 96; i++) {
		sx[i] = (int16_t)(pwRand(ctx) % 320u) - 160;
		sy[i] = (int16_t)(pwRand(ctx) % 240u) - 120;
		sz[i] = (int16_t)(20 + pwRand(ctx) % 235u);
	}
	while (pwFrame(ctx)) {
		pwClear(ctx, 0);
		for (int32_t i = 0; i < 96; i++) {
			int32_t x, y, c;

			sz[i] -= 4;
			if (sz[i] <= 4) {
				sx[i] = (int16_t)(pwRand(ctx) % 320u) - 160;
				sy[i] = (int16_t)(pwRand(ctx) % 240u) - 120;
				sz[i] = 255;
			}
			x = 160 + sx[i] * 90 / sz[i];
			y = 120 + sy[i] * 90 / sz[i];
			c = 255 - sz[i];
			if (x >= 0 && x < 320 && y >= 0 && y < 240)
				pwFill(ctx, x, y, sz[i] < 80 ? 2 : 1, sz[i] < 80 ? 2 : 1, pwRgb(c, c, c));
		}
	}
	return 0;
}

static uint16_t pwWheel(uint32_t value)
{
	value &= 127u;
	if (value < 32u)
		return pwRgb(0, value * 8u, 255);
	if (value < 64u)
		return pwRgb(0, 255, 255u - (value - 32u) * 8u);
	if (value < 96u)
		return pwRgb((value - 64u) * 8u, 255, 0);
	return pwRgb(255, 255u - (value - 96u) * 8u, 0);
}

static int pwRunSpiro(struct PicowareAppCtx *ctx)
{
	float phase = 0.0f;

	pwClear(ctx, 0);
	while (pwFrame(ctx)) {
		if (!(ctx->frame % 420u))
			pwClear(ctx, 0);
		for (uint32_t i = 0; i < 90u; i++) {
			float a = phase + (float)i * 0.035f;
			float b = a * 2.73f;
			int32_t x = 160 + (int32_t)(cosf(a) * 70.0f + sinf(b) * 42.0f);
			int32_t y = 120 + (int32_t)(sinf(a) * 70.0f + cosf(b) * 42.0f);

			pwPixel(ctx, x, y, pwWheel(ctx->frame + i));
		}
		phase += 0.08f;
	}
	return 0;
}

static void cubeProject(float x, float y, float z, float ax, float ay, int32_t *sx, int32_t *sy)
{
	float cx = cosf(ax), sxv = sinf(ax), cy = cosf(ay), syv = sinf(ay);
	float yy = y * cx - z * sxv;
	float zz = y * sxv + z * cx;
	float xx = x * cy + zz * syv;
	float zzz = -x * syv + zz * cy + 3.4f;
	float f = 72.0f / zzz;

	*sx = 160 + (int32_t)(xx * f);
	*sy = 120 + (int32_t)(yy * f);
}

static int pwRunCube(struct PicowareAppCtx *ctx)
{
	static const int8_t verts[8][3] = {
		{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
		{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1},
	};
	static const uint8_t edges[12][2] = {
		{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
		{6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7},
	};
	float ax = 0.0f, ay = 0.0f;

	while (pwFrame(ctx)) {
		int32_t px[8], py[8];

		pwClear(ctx, pwRgb(2, 2, 8));
		for (uint32_t i = 0; i < 8; i++)
			cubeProject((float)verts[i][0], (float)verts[i][1], (float)verts[i][2], ax, ay, &px[i], &py[i]);
		for (uint32_t i = 0; i < 12; i++)
			pwLine(ctx, px[edges[i][0]], py[edges[i][0]], px[edges[i][1]], py[edges[i][1]], pwWheel(ctx->frame + i * 8u));
		ax += 0.045f;
		ay += 0.031f;
	}
	return 0;
}

int picowareAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct PicowareAppCtx ctx;
	int ret;

	if (!host || !args || !args->canvas)
		return -1;
	dispSetFramerate(PICOWARE_FPS);
	if (!pwInit(&ctx, host, args)) {
		dispSetFramerate(60);
		return -1;
	}
	pwWaitRelease(&ctx);

#if DCAPP_RUNTIME_ID == 201
	ret = pwRunTetris(&ctx);
#elif DCAPP_RUNTIME_ID == 203
	ret = pwRunFlappy(&ctx);
#elif DCAPP_RUNTIME_ID == 204
	ret = pwRunLabyrinth(&ctx);
#elif DCAPP_RUNTIME_ID == 220
	ret = pwRunStarfield(&ctx);
#elif DCAPP_RUNTIME_ID == 221
	ret = pwRunSpiro(&ctx);
#elif DCAPP_RUNTIME_ID == 222
	ret = pwRunCube(&ctx);
#else
	(void)ctx;
	ret = -1;
#endif
	dispSetFramerate(60);
	return ret;
}
