/*
 * Native DC32 adaptation of wkeeling/arkanoid commit
 * 7e0e876cd034ebd62890e65352c7ef0b12b45df5.
 */
#include "apps/arkanoid/arkanoid_port.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "apps/port/port_runtime.h"
#include "arkanoid_assets.h"
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "ui.h"

#define ARK_SCREEN_W 320
#define ARK_SCREEN_H 240
#define ARK_ARENA_X 49
#define ARK_ARENA_W 222
#define ARK_ARENA_H 240
#define ARK_SOURCE_W 600
#define ARK_SOURCE_H 650
#define ARK_SIDE_W 22.0f
#define ARK_TOP_H 22.0f
#define ARK_RIGHT_X 578.0f
#define ARK_BOTTOM_Y 650.0f
#define ARK_BRICK_W 43.0f
#define ARK_BRICK_H 21.0f
#define ARK_BALL_SIZE 10.0f
#define ARK_PADDLE_H 20.0f
#define ARK_PADDLE_NORMAL_W 79.0f
#define ARK_PADDLE_WIDE_W 119.0f
#define ARK_PADDLE_Y 570.0f
#define ARK_FPS 60u
#define ARK_MAX_BRICKS 160u
#define ARK_MAX_BALLS 9u
#define ARK_MAX_POWERUPS 16u
#define ARK_MAX_BULLETS 8u
#define ARK_MAX_ENEMIES 3u
#define ARK_PI 3.14159265358979323846f
#define ARK_TWO_PI (ARK_PI * 2.0f)
#define ARK_SAVE_MAGIC 0x314b5241u
#define ARK_SAVE_VERSION 1u
#define ARK_SAVE_CHECK_XOR 0x9d537ac1u

enum ArkState {
	ArkStateTitle,
	ArkStateReady,
	ArkStatePlay,
	ArkStateLifeLost,
	ArkStateRoundEnd,
	ArkStateGameOver,
};

enum ArkBrickColor {
	ArkBrickBlue,
	ArkBrickCyan,
	ArkBrickGold,
	ArkBrickGreen,
	ArkBrickOrange,
	ArkBrickPink,
	ArkBrickRed,
	ArkBrickSilver,
	ArkBrickWhite,
	ArkBrickYellow,
};

enum ArkPowerupType {
	ArkPowerNone,
	ArkPowerCatch,
	ArkPowerDuplicate,
	ArkPowerExpand,
	ArkPowerLife,
	ArkPowerLaser,
	ArkPowerSlow,
};

enum ArkEnemyType {
	ArkEnemyCone,
	ArkEnemyPyramid,
	ArkEnemyMolecule,
	ArkEnemyCube,
};

enum ArkEnemyState {
	ArkEnemyHidden,
	ArkEnemyWaiting,
	ArkEnemyActive,
	ArkEnemyExploding,
};

struct ArkBrick {
	float x;
	float y;
	uint8_t color;
	uint8_t hits;
	int8_t destroyAfter;
	uint8_t powerup;
	bool alive;
};

struct ArkBall {
	float x;
	float y;
	float vx;
	float vy;
	float speed;
	float baseSpeed;
	bool active;
	bool anchored;
};

struct ArkPowerup {
	float x;
	float y;
	uint8_t type;
	uint8_t frame;
	bool active;
};

struct ArkBullet {
	float x;
	float y;
	bool active;
};

struct ArkEnemy {
	float x;
	float y;
	float angle;
	uint16_t timer;
	uint16_t steerTimer;
	uint8_t frame;
	uint8_t state;
};

struct ArkSave {
	uint32_t magic;
	uint16_t version;
	uint16_t size;
	uint32_t highScore;
	uint32_t check;
};

struct ArkGame {
	const struct DcAppHostApi *host;
	const struct DcAppRunArgs *args;
	struct DcAppDrawCtx draw;
	uint8_t *frame;
	struct ArkBrick bricks[ARK_MAX_BRICKS];
	struct ArkBall balls[ARK_MAX_BALLS];
	struct ArkPowerup powerups[ARK_MAX_POWERUPS];
	struct ArkBullet bullets[ARK_MAX_BULLETS];
	struct ArkEnemy enemies[ARK_MAX_ENEMIES];
	uint32_t rng;
	uint32_t score;
	uint32_t highScore;
	uint16_t stateTimer;
	uint16_t destroyed;
	uint16_t destructible;
	uint16_t transientPowerTimer;
	uint8_t brickCount;
	uint8_t ballCount;
	uint8_t round;
	uint8_t selectedRound;
	uint8_t lives;
	uint8_t activePower;
	uint8_t enemyType;
	uint8_t doorFrame;
	uint8_t state;
	float paddleX;
	float paddleW;
	float paddleSpeed;
	bool enemiesReleased;
	bool paused;
	bool saveDirty;
	bool completed;
	bool catchEnabled;
	bool laserEnabled;
	bool restartReady;
	bool doorRight;
};

static bool mArkanoidAbort;

static const enum ArkanoidAssetId mConeFrames[25] = {
	ArkAssetEnemyCone1, ArkAssetEnemyCone2, ArkAssetEnemyCone3, ArkAssetEnemyCone4,
	ArkAssetEnemyCone5, ArkAssetEnemyCone6, ArkAssetEnemyCone7, ArkAssetEnemyCone8,
	ArkAssetEnemyCone9, ArkAssetEnemyCone10, ArkAssetEnemyCone11, ArkAssetEnemyCone12,
	ArkAssetEnemyCone13, ArkAssetEnemyCone14, ArkAssetEnemyCone15, ArkAssetEnemyCone16,
	ArkAssetEnemyCone17, ArkAssetEnemyCone18, ArkAssetEnemyCone19, ArkAssetEnemyCone20,
	ArkAssetEnemyCone21, ArkAssetEnemyCone22, ArkAssetEnemyCone23, ArkAssetEnemyCone24,
	ArkAssetEnemyCone25,
};

static const enum ArkanoidAssetId mPyramidFrames[25] = {
	ArkAssetEnemyPyramid1, ArkAssetEnemyPyramid2, ArkAssetEnemyPyramid3, ArkAssetEnemyPyramid4,
	ArkAssetEnemyPyramid5, ArkAssetEnemyPyramid6, ArkAssetEnemyPyramid7, ArkAssetEnemyPyramid8,
	ArkAssetEnemyPyramid9, ArkAssetEnemyPyramid10, ArkAssetEnemyPyramid11, ArkAssetEnemyPyramid12,
	ArkAssetEnemyPyramid13, ArkAssetEnemyPyramid14, ArkAssetEnemyPyramid15, ArkAssetEnemyPyramid16,
	ArkAssetEnemyPyramid17, ArkAssetEnemyPyramid18, ArkAssetEnemyPyramid19, ArkAssetEnemyPyramid20,
	ArkAssetEnemyPyramid21, ArkAssetEnemyPyramid22, ArkAssetEnemyPyramid23, ArkAssetEnemyPyramid24,
	ArkAssetEnemyPyramid25,
};

static const enum ArkanoidAssetId mMoleculeFrames[25] = {
	ArkAssetEnemyMolecule1, ArkAssetEnemyMolecule2, ArkAssetEnemyMolecule3, ArkAssetEnemyMolecule4,
	ArkAssetEnemyMolecule5, ArkAssetEnemyMolecule6, ArkAssetEnemyMolecule7, ArkAssetEnemyMolecule8,
	ArkAssetEnemyMolecule9, ArkAssetEnemyMolecule10, ArkAssetEnemyMolecule11, ArkAssetEnemyMolecule12,
	ArkAssetEnemyMolecule13, ArkAssetEnemyMolecule14, ArkAssetEnemyMolecule15, ArkAssetEnemyMolecule16,
	ArkAssetEnemyMolecule17, ArkAssetEnemyMolecule18, ArkAssetEnemyMolecule19, ArkAssetEnemyMolecule20,
	ArkAssetEnemyMolecule21, ArkAssetEnemyMolecule22, ArkAssetEnemyMolecule23, ArkAssetEnemyMolecule24,
	ArkAssetEnemyMolecule25,
};

static const enum ArkanoidAssetId mCubeFrames[25] = {
	ArkAssetEnemyCube1, ArkAssetEnemyCube2, ArkAssetEnemyCube3, ArkAssetEnemyCube4,
	ArkAssetEnemyCube5, ArkAssetEnemyCube6, ArkAssetEnemyCube7, ArkAssetEnemyCube8,
	ArkAssetEnemyCube9, ArkAssetEnemyCube10, ArkAssetEnemyCube11, ArkAssetEnemyCube12,
	ArkAssetEnemyCube13, ArkAssetEnemyCube14, ArkAssetEnemyCube15, ArkAssetEnemyCube16,
	ArkAssetEnemyCube17, ArkAssetEnemyCube18, ArkAssetEnemyCube19, ArkAssetEnemyCube20,
	ArkAssetEnemyCube21, ArkAssetEnemyCube22, ArkAssetEnemyCube23, ArkAssetEnemyCube24,
	ArkAssetEnemyCube25,
};

static const enum ArkanoidAssetId mExplosionFrames[10] = {
	ArkAssetEnemyExplosion1, ArkAssetEnemyExplosion2, ArkAssetEnemyExplosion3,
	ArkAssetEnemyExplosion4, ArkAssetEnemyExplosion5, ArkAssetEnemyExplosion6,
	ArkAssetEnemyExplosion7, ArkAssetEnemyExplosion8, ArkAssetEnemyExplosion9,
	ArkAssetEnemyExplosion10,
};

static uint8_t arkRgb(uint32_t r, uint32_t g, uint32_t b)
{
	return (uint8_t)((r & 0xe0u) | ((g >> 3) & 0x1cu) | (b >> 6));
}

static uint32_t arkRandom(struct ArkGame *game)
{
	game->rng ^= game->rng << 13;
	game->rng ^= game->rng >> 17;
	game->rng ^= game->rng << 5;
	return game->rng;
}

static uint32_t arkRange(struct ArkGame *game, uint32_t limit)
{
	return limit ? arkRandom(game) % limit : 0u;
}

static float arkRandomUnit(struct ArkGame *game)
{
	return (arkRandom(game) & 0x00ffffffu) / 16777216.0f;
}

static int32_t arkScreenX(float x)
{
	return ARK_ARENA_X + (int32_t)floorf(x * ARK_ARENA_W / ARK_SOURCE_W + 0.5f);
}

static int32_t arkScreenY(float y)
{
	return (int32_t)floorf(y * ARK_ARENA_H / ARK_SOURCE_H + 0.5f);
}

static bool arkOverlap(float ax, float ay, float aw, float ah,
	float bx, float by, float bw, float bh)
{
	return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static void arkBlit(struct ArkGame *game, enum ArkanoidAssetId id, int32_t x, int32_t y)
{
	const struct ArkanoidAsset *asset;
	const uint8_t *pixels;
	const uint8_t *mask;
	uint32_t count;

	if ((uint32_t)id >= ArkAssetCount)
		return;
	asset = &arkanoidAssets[id];
	pixels = arkanoidAssetPixels + asset->pixelOffset;
	mask = arkanoidAssetMasks + asset->maskOffset;
	count = (uint32_t)asset->width * asset->height;
	for (uint32_t pos = 0; pos < count; pos++) {
		int32_t px;
		int32_t py;

		if (!(mask[pos >> 3] & (1u << (pos & 7u))))
			continue;
		px = x + (int32_t)(pos % asset->width);
		py = y + (int32_t)(pos / asset->width);
		if (px >= 0 && py >= 0 && px < ARK_SCREEN_W && py < ARK_SCREEN_H)
			game->frame[(uint32_t)py * ARK_SCREEN_W + (uint32_t)px] = pixels[pos];
	}
}

static void arkBlitRotatedCounterClockwise(struct ArkGame *game, enum ArkanoidAssetId id,
	int32_t x, int32_t y, uint32_t width, uint32_t height)
{
	const struct ArkanoidAsset *asset;
	const uint8_t *pixels;
	const uint8_t *mask;

	if ((uint32_t)id >= ArkAssetCount || !width || !height)
		return;
	asset = &arkanoidAssets[id];
	pixels = arkanoidAssetPixels + asset->pixelOffset;
	mask = arkanoidAssetMasks + asset->maskOffset;
	for (uint32_t dy = 0; dy < height; dy++)
		for (uint32_t dx = 0; dx < width; dx++) {
			uint32_t sourceX = asset->width - 1u -
				dy * asset->width / height;
			uint32_t sourceY = dx * asset->height / width;
			uint32_t sourcePos = sourceY * asset->width + sourceX;
			int32_t px = x + (int32_t)dx;
			int32_t py = y + (int32_t)dy;

			if (!(mask[sourcePos >> 3] & (1u << (sourcePos & 7u))))
				continue;
			if (px >= 0 && py >= 0 && px < ARK_SCREEN_W && py < ARK_SCREEN_H)
				game->frame[(uint32_t)py * ARK_SCREEN_W + (uint32_t)px] =
					pixels[sourcePos];
		}
}

static uint32_t arkTextWidth(const char *text, enum Font font)
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

static void arkText(struct ArkGame *game, int32_t x, int32_t y,
	const char *text, enum Font font, uint8_t color)
{
	while (text && *text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text)) {
			for (uint_fast8_t row = 0; row < glyph.height; row++)
				for (uint_fast8_t col = 0; col < glyph.width; col++)
					if (fontGetGlyphPixel(&glyph, row, col)) {
						int32_t px = x + col;
						int32_t py = y + row;

						if (px >= 0 && py >= 0 && px < ARK_SCREEN_W && py < ARK_SCREEN_H)
							game->frame[(uint32_t)py * ARK_SCREEN_W + (uint32_t)px] = color;
					}
			x += glyph.width + 1;
		}
		text++;
	}
}

static void arkCenteredText(struct ArkGame *game, int32_t left, int32_t width,
	int32_t y, const char *text, enum Font font, uint8_t color)
{
	uint32_t textWidth = arkTextWidth(text, font);

	arkText(game, left + (int32_t)(width - (int32_t)textWidth) / 2, y, text, font, color);
}

static void arkNumber(struct ArkGame *game, int32_t left, int32_t width,
	int32_t y, uint32_t value, uint8_t color)
{
	char text[16];
	uint32_t textWidth;

	snprintf(text, sizeof(text), "%lu", (unsigned long)value);
	textWidth = arkTextWidth(text, FontSmall);
	arkText(game, left + width - (int32_t)textWidth - 2, y, text, FontSmall, color);
}

static enum ArkanoidAssetId arkBrickAsset(const struct ArkBrick *brick)
{
	switch (brick->color) {
		case ArkBrickBlue: return ArkAssetBrickBlue;
		case ArkBrickCyan: return ArkAssetBrickCyan;
		case ArkBrickGold: return ArkAssetBrickGold;
		case ArkBrickGreen: return ArkAssetBrickGreen;
		case ArkBrickOrange: return ArkAssetBrickOrange;
		case ArkBrickPink: return ArkAssetBrickPink;
		case ArkBrickRed: return ArkAssetBrickRed;
		case ArkBrickSilver:
			return brick->hits ? ArkAssetBrickSilver5 : ArkAssetBrickSilver;
		case ArkBrickWhite: return ArkAssetBrickWhite;
		case ArkBrickYellow: return ArkAssetBrickYellow;
	}
	return ArkAssetBrickWhite;
}

static uint32_t arkBrickValue(const struct ArkGame *game, const struct ArkBrick *brick)
{
	static const uint16_t values[] = {100, 70, 0, 80, 60, 110, 90, 50, 40, 120};

	return brick->color == ArkBrickSilver ? 50u * game->round : values[brick->color];
}

static bool arkAddBrick(struct ArkGame *game, uint8_t gx, uint8_t gy,
	enum ArkBrickColor color, enum ArkPowerupType powerup)
{
	struct ArkBrick *brick;

	if (game->brickCount >= ARK_MAX_BRICKS)
		return false;
	brick = &game->bricks[game->brickCount++];
	memset(brick, 0, sizeof(*brick));
	brick->x = ARK_SIDE_W + gx * ARK_BRICK_W;
	brick->y = ARK_TOP_H + gy * ARK_BRICK_H;
	brick->color = color;
	brick->powerup = powerup;
	brick->destroyAfter = color == ArkBrickSilver ? 2 : color == ArkBrickGold ? -1 : 1;
	brick->alive = true;
	if (brick->destroyAfter > 0)
		game->destructible++;
	return true;
}

static void arkShuffle(struct ArkGame *game, uint8_t *values, uint32_t count)
{
	for (uint32_t i = count; i > 1u; i--) {
		uint32_t other = arkRange(game, i);
		uint8_t tmp = values[i - 1u];

		values[i - 1u] = values[other];
		values[other] = tmp;
	}
}

static void arkSample(struct ArkGame *game, uint8_t limit, uint8_t count, uint8_t *out)
{
	uint8_t pool[96];

	for (uint8_t i = 0; i < limit; i++)
		pool[i] = i;
	arkShuffle(game, pool, limit);
	memcpy(out, pool, count);
}

static enum ArkPowerupType arkPowerAt(const uint8_t *indexes,
	const uint8_t *powers, uint8_t count, uint8_t index)
{
	for (uint8_t i = 0; i < count; i++)
		if (indexes[i] == index)
			return (enum ArkPowerupType)powers[i];
	return ArkPowerNone;
}

static void arkBuildRound1(struct ArkGame *game)
{
	static const enum ArkBrickColor colors[] = {
		ArkBrickSilver, ArkBrickRed, ArkBrickYellow, ArkBrickBlue, ArkBrickGreen,
	};
	uint8_t powers[16] = {
		ArkPowerCatch, ArkPowerCatch, ArkPowerCatch,
		ArkPowerExpand, ArkPowerExpand, ArkPowerExpand, ArkPowerExpand,
		ArkPowerLife, ArkPowerLife, ArkPowerLife,
		ArkPowerSlow, ArkPowerSlow,
		ArkPowerLaser, ArkPowerLaser, ArkPowerLaser, ArkPowerLaser,
	};
	uint8_t indexes[16];

	arkShuffle(game, powers, 16);
	arkSample(game, 52, 12, indexes);
	arkSample(game, 13, 4, indexes + 12);
	for (uint8_t i = 12; i < 16; i++)
		indexes[i] += 52;
	for (uint8_t row = 0; row < 5; row++)
		for (uint8_t x = 0; x < 13; x++) {
			uint8_t index = row * 13u + x;

			(void)arkAddBrick(game, x, row + 4u, colors[row],
				arkPowerAt(indexes, powers, 16, index));
		}
}

static void arkBuildRound2(struct ArkGame *game)
{
	static const enum ArkBrickColor colors[] = {
		ArkBrickWhite, ArkBrickOrange, ArkBrickCyan, ArkBrickGreen,
		ArkBrickRed, ArkBrickBlue, ArkBrickPink, ArkBrickYellow,
	};
	uint8_t firstPowers[4] = {ArkPowerSlow, ArkPowerSlow, ArkPowerCatch, ArkPowerCatch};
	uint8_t restPowers[17] = {
		ArkPowerLife, ArkPowerLife, ArkPowerLife,
		ArkPowerLaser, ArkPowerLaser, ArkPowerLaser, ArkPowerLaser,
		ArkPowerCatch, ArkPowerCatch,
		ArkPowerExpand, ArkPowerExpand, ArkPowerExpand, ArkPowerExpand,
		ArkPowerSlow, ArkPowerSlow, ArkPowerDuplicate, ArkPowerDuplicate,
	};
	uint8_t firstIndexes[4];
	uint8_t restIndexes[17];
	uint8_t count = 0;

	arkShuffle(game, firstPowers, 4);
	arkShuffle(game, restPowers, 17);
	arkSample(game, 13, 4, firstIndexes);
	arkSample(game, 91, 17, restIndexes);
	for (uint8_t x = 0; x < 13; x++) {
		uint8_t i = 12u - x;
		enum ArkPowerupType first = arkPowerAt(firstIndexes, firstPowers, 4, i);

		(void)arkAddBrick(game, x, 16, i ? ArkBrickSilver : ArkBrickRed,
			i ? first : ArkPowerSlow);
		for (uint8_t n = 0; n < i; n++) {
			uint8_t y = 15u - n;

			(void)arkAddBrick(game, x, y, colors[x % 8u],
				arkPowerAt(restIndexes, restPowers, 17, count++));
		}
	}
}

static void arkBuildRound3(struct ArkGame *game)
{
	for (uint8_t x = 0; x < 13; x++)
		(void)arkAddBrick(game, x, 4, ArkBrickGreen, ArkPowerNone);
	for (uint8_t x = 0; x < 13; x++)
		(void)arkAddBrick(game, x, 6, x < 3 ? ArkBrickWhite : ArkBrickGold, ArkPowerNone);
	for (uint8_t x = 0; x < 13; x++)
		(void)arkAddBrick(game, x, 8, ArkBrickRed, x == 5 ? ArkPowerDuplicate : ArkPowerNone);
	for (uint8_t x = 0; x < 13; x++)
		(void)arkAddBrick(game, x, 10, x < 10 ? ArkBrickGold : ArkBrickWhite, ArkPowerNone);
	for (uint8_t x = 0; x < 13; x++)
		(void)arkAddBrick(game, x, 12, ArkBrickPink,
			x == 4 ? ArkPowerDuplicate : x == 8 ? ArkPowerLife : ArkPowerNone);
	for (uint8_t x = 0; x < 13; x++)
		(void)arkAddBrick(game, x, 14, x < 3 ? ArkBrickBlue : ArkBrickGold, ArkPowerNone);
	for (uint8_t x = 0; x < 13; x++)
		(void)arkAddBrick(game, x, 16, ArkBrickCyan,
			x == 2 ? ArkPowerCatch : x == 6 ? ArkPowerLife : ArkPowerNone);
	for (uint8_t x = 0; x < 13; x++)
		(void)arkAddBrick(game, x, 18, x < 10 ? ArkBrickGold : ArkBrickCyan,
			x == 10 ? ArkPowerCatch : ArkPowerNone);
}

static void arkBuildRound4(struct ArkGame *game)
{
	static const enum ArkBrickColor column[16] = {
		ArkBrickOrange, ArkBrickCyan, ArkBrickGreen, ArkBrickSilver,
		ArkBrickBlue, ArkBrickPink, ArkBrickYellow, ArkBrickWhite,
		ArkBrickOrange, ArkBrickCyan, ArkBrickGreen, ArkBrickSilver,
		ArkBrickBlue, ArkBrickPink, ArkBrickWhite, ArkBrickYellow,
	};

	for (uint8_t x = 1; x < 12; x++) {
		if (x == 6)
			continue;
		for (uint8_t y = 0; y < 14; y++) {
			enum ArkPowerupType power = ArkPowerNone;

			if (x == 1 && y == 1) power = ArkPowerDuplicate;
			else if (x == 2 && y == 3) power = ArkPowerCatch;
			else if (x == 3 && y == 10) power = ArkPowerLaser;
			else if (x == 4 && y == 4) power = ArkPowerLife;
			else if (x == 7 && y == 11) power = ArkPowerExpand;
			else if (x == 8 && y == 0) power = ArkPowerDuplicate;
			else if (x == 9 && y == 5) power = ArkPowerLaser;
			else if (x == 10 && y == 7) power = ArkPowerLife;
			(void)arkAddBrick(game, x, y + 5u, column[(y + x - 1u) % 16u], power);
		}
	}
}

static bool arkRound5Hole(uint8_t x, uint8_t y)
{
	static const uint8_t holes[][2] = {
		{3,11},{9,11},{3,12},{5,12},{6,12},{7,12},{9,12},
		{3,13},{5,13},{6,13},{7,13},{9,13},
		{2,14},{3,14},{4,14},{6,14},{8,14},{9,14},{10,14},
		{2,15},{3,15},{4,15},{6,15},{8,15},{9,15},{10,15},
	};

	for (uint32_t i = 0; i < sizeof(holes) / sizeof(holes[0]); i++)
		if (holes[i][0] == x && holes[i][1] == y)
			return true;
	return false;
}

static void arkBuildRound5(struct ArkGame *game)
{
	(void)arkAddBrick(game, 4, 2, ArkBrickOrange, ArkPowerExpand);
	(void)arkAddBrick(game, 8, 2, ArkBrickOrange, ArkPowerLaser);
	(void)arkAddBrick(game, 5, 3, ArkBrickOrange, ArkPowerNone);
	(void)arkAddBrick(game, 7, 3, ArkBrickOrange, ArkPowerSlow);
	(void)arkAddBrick(game, 5, 4, ArkBrickOrange, ArkPowerNone);
	(void)arkAddBrick(game, 7, 4, ArkBrickOrange, ArkPowerNone);
	for (uint8_t y = 5; y < 7; y++)
		for (uint8_t x = 4; x < 9; x++)
			(void)arkAddBrick(game, x, y, ArkBrickSilver, ArkPowerNone);
	for (uint8_t y = 7; y < 9; y++)
		for (uint8_t x = 3; x < 10; x++) {
			enum ArkBrickColor color = (x == 5 || x == 7) ? ArkBrickRed : ArkBrickSilver;
			enum ArkPowerupType power =
				x == 5 && y == 8 ? ArkPowerCatch :
				x == 7 && y == 8 ? ArkPowerDuplicate : ArkPowerNone;

			(void)arkAddBrick(game, x, y, color, power);
		}
	for (uint8_t y = 9; y < 16; y++)
		for (uint8_t x = 2; x < 11; x++)
			if (!arkRound5Hole(x, y))
				(void)arkAddBrick(game, x, y, ArkBrickSilver,
					x == 7 && y == 14 ? ArkPowerLaser : ArkPowerNone);
}

static void arkBuildRound(struct ArkGame *game, uint8_t round)
{
	memset(game->bricks, 0, sizeof(game->bricks));
	game->brickCount = 0;
	game->destroyed = 0;
	game->destructible = 0;
	game->round = round;
	switch (round) {
		case 1: arkBuildRound1(game); game->enemyType = ArkEnemyCone; break;
		case 2: arkBuildRound2(game); game->enemyType = ArkEnemyPyramid; break;
		case 3: arkBuildRound3(game); game->enemyType = ArkEnemyMolecule; break;
		case 4: arkBuildRound4(game); game->enemyType = ArkEnemyCube; break;
		default: arkBuildRound5(game); game->enemyType = ArkEnemyCone; break;
	}
}

static float arkRoundBaseSpeed(const struct ArkGame *game)
{
	return game->round == 3 ? 6.0f : 8.0f;
}

static void arkBallDirection(struct ArkBall *ball, float angle, float speed)
{
	ball->speed = speed;
	ball->vx = cosf(angle) * speed;
	ball->vy = sinf(angle) * speed;
}

static void arkNormalizeVelocity(struct ArkBall *ball)
{
	float magnitude = sqrtf(ball->vx * ball->vx + ball->vy * ball->vy);

	if (magnitude < 0.001f)
		return;
	ball->vx = ball->vx / magnitude * ball->speed;
	ball->vy = ball->vy / magnitude * ball->speed;
}

static void arkResetBalls(struct ArkGame *game)
{
	memset(game->balls, 0, sizeof(game->balls));
	game->ballCount = 1;
	game->balls[0].active = true;
	game->balls[0].anchored = true;
	game->balls[0].baseSpeed = arkRoundBaseSpeed(game);
	arkBallDirection(&game->balls[0], 5.0f, game->balls[0].baseSpeed);
}

static void arkDeactivatePower(struct ArkGame *game)
{
	float base = arkRoundBaseSpeed(game);

	game->activePower = ArkPowerNone;
	game->transientPowerTimer = 0;
	game->catchEnabled = false;
	game->laserEnabled = false;
	game->paddleW = ARK_PADDLE_NORMAL_W;
	for (uint8_t i = 0; i < ARK_MAX_BALLS; i++)
		if (game->balls[i].active) {
			game->balls[i].baseSpeed = base;
			if (game->balls[i].speed < base || game->balls[i].speed > base + 4.0f)
				game->balls[i].speed = base;
			arkNormalizeVelocity(&game->balls[i]);
		}
}

static void arkAnchorBalls(struct ArkGame *game)
{
	for (uint8_t i = 0; i < ARK_MAX_BALLS; i++)
		if (game->balls[i].active && game->balls[i].anchored) {
			game->balls[i].x = game->paddleX + game->paddleW * 0.5f - ARK_BALL_SIZE * 0.5f;
			game->balls[i].y = ARK_PADDLE_Y - ARK_BALL_SIZE;
		}
}

static void arkResetActors(struct ArkGame *game, bool preserveBricks)
{
	(void)preserveBricks;
	arkDeactivatePower(game);
	game->paddleW = ARK_PADDLE_NORMAL_W;
	game->paddleX = (ARK_SOURCE_W - game->paddleW) * 0.5f;
	game->paddleSpeed = game->round == 3 ? 8.0f : 10.0f;
	memset(game->powerups, 0, sizeof(game->powerups));
	memset(game->bullets, 0, sizeof(game->bullets));
	memset(game->enemies, 0, sizeof(game->enemies));
	game->enemiesReleased = false;
	game->doorFrame = 0;
	arkResetBalls(game);
	arkAnchorBalls(game);
}

static void arkBeginReady(struct ArkGame *game, bool restart)
{
	game->state = ArkStateReady;
	game->stateTimer = 0;
	game->restartReady = restart;
	arkResetActors(game, restart);
}

static void arkStartGame(struct ArkGame *game)
{
	game->score = 0;
	game->lives = 3;
	game->completed = false;
	arkBuildRound(game, game->selectedRound);
	arkBeginReady(game, false);
}

static void arkSpawnPowerup(struct ArkGame *game, const struct ArkBrick *brick,
	enum ArkPowerupType type)
{
	for (uint8_t i = 0; i < ARK_MAX_POWERUPS; i++)
		if (!game->powerups[i].active) {
			game->powerups[i].active = true;
			game->powerups[i].type = type;
			game->powerups[i].x = brick->x + 2.5f;
			game->powerups[i].y = brick->y;
			game->powerups[i].frame = 0;
			return;
		}
}

static void arkHitBrick(struct ArkGame *game, struct ArkBrick *brick, bool laser)
{
	bool destroyed;

	if (!brick->alive || brick->destroyAfter < 0)
		return;
	brick->hits++;
	destroyed = brick->hits >= (uint8_t)brick->destroyAfter;
	if (destroyed) {
		brick->alive = false;
		game->destroyed++;
		game->score += arkBrickValue(game, brick);
	}
	if (brick->powerup != ArkPowerNone && !laser &&
			(destroyed || (arkRandom(game) & 1u))) {
		arkSpawnPowerup(game, brick, (enum ArkPowerupType)brick->powerup);
		brick->powerup = ArkPowerNone;
	}
}

static void arkEnemyRespawn(struct ArkGame *game, struct ArkEnemy *enemy)
{
	enemy->state = ArkEnemyWaiting;
	enemy->timer = (uint16_t)(60u + arkRange(game, 541u));
	enemy->frame = 0;
}

static void arkReleaseEnemies(struct ArkGame *game)
{
	if (game->enemiesReleased)
		return;
	game->enemiesReleased = true;
	for (uint8_t i = 0; i < ARK_MAX_ENEMIES; i++)
		arkEnemyRespawn(game, &game->enemies[i]);
}

static void arkExplodeEnemy(struct ArkGame *game, struct ArkEnemy *enemy)
{
	if (enemy->state != ArkEnemyActive)
		return;
	enemy->state = ArkEnemyExploding;
	enemy->timer = 0;
	enemy->frame = 0;
	game->score += 500;
}

static void arkApplyPower(struct ArkGame *game, enum ArkPowerupType type)
{
	if (type == ArkPowerLife) {
		game->lives++;
		game->activePower = type;
		game->transientPowerTimer = 120;
		return;
	}
	if (type == ArkPowerDuplicate) {
		struct ArkBall clones[ARK_MAX_BALLS];
		uint8_t cloneCount = 0;

		memset(clones, 0, sizeof(clones));
		for (uint8_t i = 0; i < ARK_MAX_BALLS && game->ballCount + cloneCount < ARK_MAX_BALLS; i++)
			if (game->balls[i].active) {
				float angle = atan2f(game->balls[i].vy, game->balls[i].vx);

				for (int8_t side = -1; side <= 1; side += 2) {
					struct ArkBall clone = game->balls[i];

					if (game->ballCount + cloneCount >= ARK_MAX_BALLS)
						break;
					clone.anchored = false;
					arkBallDirection(&clone, angle + side * 0.4f, clone.speed);
					clones[cloneCount++] = clone;
				}
			}
		for (uint8_t c = 0; c < cloneCount; c++)
			for (uint8_t i = 0; i < ARK_MAX_BALLS; i++)
				if (!game->balls[i].active) {
					game->balls[i] = clones[c];
					game->balls[i].active = true;
					game->ballCount++;
					break;
				}
		game->activePower = type;
		game->transientPowerTimer = 120;
		return;
	}
	arkDeactivatePower(game);
	game->activePower = type;
	if (type == ArkPowerCatch)
		game->catchEnabled = true;
	else if (type == ArkPowerExpand) {
		game->paddleW = ARK_PADDLE_WIDE_W;
		for (uint8_t i = 0; i < ARK_MAX_BALLS; i++)
			if (game->balls[i].active) {
				game->balls[i].baseSpeed += 1.0f;
				game->balls[i].speed += 1.0f;
				arkNormalizeVelocity(&game->balls[i]);
			}
	}
	else if (type == ArkPowerLaser) {
		game->laserEnabled = true;
		for (uint8_t i = 0; i < ARK_MAX_BALLS; i++)
			if (game->balls[i].active) {
				game->balls[i].baseSpeed += 1.0f;
				game->balls[i].speed += 1.0f;
				arkNormalizeVelocity(&game->balls[i]);
			}
	}
	else if (type == ArkPowerSlow)
		for (uint8_t i = 0; i < ARK_MAX_BALLS; i++)
			if (game->balls[i].active) {
				game->balls[i].baseSpeed = 6.0f;
				game->balls[i].speed = 6.0f;
				arkNormalizeVelocity(&game->balls[i]);
			}
	if (game->paddleX + game->paddleW > ARK_RIGHT_X)
		game->paddleX = ARK_RIGHT_X - game->paddleW;
}

static void arkFire(struct ArkGame *game)
{
	if (game->laserEnabled) {
		for (uint8_t side = 0; side < 2; side++)
			for (uint8_t i = 0; i < ARK_MAX_BULLETS; i++)
				if (!game->bullets[i].active) {
					game->bullets[i].active = true;
					game->bullets[i].x = game->paddleX +
						(side ? game->paddleW - 8.0f : 2.0f);
					game->bullets[i].y = ARK_PADDLE_Y - 15.0f;
					break;
				}
	}
	for (uint8_t i = 0; i < ARK_MAX_BALLS; i++)
		if (game->balls[i].active && game->balls[i].anchored) {
			game->balls[i].anchored = false;
			arkBallDirection(&game->balls[i], 5.0f, game->balls[i].baseSpeed);
		}
}

static void arkPaddleBounce(struct ArkGame *game, struct ArkBall *ball)
{
	static const float degrees[6] = {220.0f, 245.0f, 260.0f, 280.0f, 295.0f, 320.0f};
	float center = ball->x + ARK_BALL_SIZE * 0.5f;
	int32_t segment = (int32_t)((center - game->paddleX) * 6.0f / game->paddleW);

	if (segment < 0) segment = 0;
	if (segment > 5) segment = 5;
	ball->y = ARK_PADDLE_Y - ARK_BALL_SIZE;
	if (game->catchEnabled) {
		ball->anchored = true;
		return;
	}
	arkBallDirection(ball, degrees[segment] * ARK_PI / 180.0f, ball->speed);
}

static void arkAdjustBallSpeed(struct ArkBall *ball, float delta)
{
	ball->speed += delta;
	if (ball->speed > 15.0f)
		ball->speed = 15.0f;
	arkNormalizeVelocity(ball);
}

static void arkJitterBall(struct ArkGame *game, struct ArkBall *ball)
{
	float angle = atan2f(ball->vy, ball->vx) +
		(arkRandomUnit(game) - 0.5f) * 0.06f;

	arkBallDirection(ball, angle, ball->speed);
}

static bool arkBallBrickCollision(struct ArkGame *game, struct ArkBall *ball,
	float previousX, float previousY)
{
	for (uint8_t i = 0; i < game->brickCount; i++) {
		struct ArkBrick *brick = &game->bricks[i];

		if (!brick->alive || !arkOverlap(ball->x, ball->y, ARK_BALL_SIZE, ARK_BALL_SIZE,
				brick->x, brick->y, ARK_BRICK_W, ARK_BRICK_H))
			continue;
		if (previousX + ARK_BALL_SIZE <= brick->x || previousX >= brick->x + ARK_BRICK_W)
			ball->vx = -ball->vx;
		else
			ball->vy = -ball->vy;
		ball->x = previousX;
		ball->y = previousY;
		arkHitBrick(game, brick, false);
		arkAdjustBallSpeed(ball, 0.5f);
		arkJitterBall(game, ball);
		return true;
	}
	return false;
}

static bool arkBallEnemyCollision(struct ArkGame *game, struct ArkBall *ball)
{
	for (uint8_t i = 0; i < ARK_MAX_ENEMIES; i++) {
		struct ArkEnemy *enemy = &game->enemies[i];

		if (enemy->state == ArkEnemyActive &&
				arkOverlap(ball->x, ball->y, ARK_BALL_SIZE, ARK_BALL_SIZE,
					enemy->x, enemy->y, 40.0f, 44.0f)) {
			ball->vx = -ball->vx;
			ball->vy = -ball->vy;
			arkExplodeEnemy(game, enemy);
			return true;
		}
	}
	return false;
}

static void arkRemoveBall(struct ArkGame *game, uint8_t index)
{
	game->balls[index].active = false;
	game->balls[index].anchored = false;
	if (game->ballCount)
		game->ballCount--;
}

static void arkUpdateBall(struct ArkGame *game, uint8_t index)
{
	struct ArkBall *ball = &game->balls[index];
	uint32_t steps;
	float normalRate = game->round == 3 ? 0.07f : 0.02f;

	if (!ball->active)
		return;
	if (ball->anchored) {
		arkAnchorBalls(game);
		return;
	}
	if (ball->speed > ball->baseSpeed)
		ball->speed -= normalRate;
	else if (ball->speed < ball->baseSpeed)
		ball->speed += normalRate;
	arkNormalizeVelocity(ball);
	steps = (uint32_t)ceilf(fmaxf(fabsf(ball->vx), fabsf(ball->vy)));
	if (!steps) steps = 1;
	for (uint32_t step = 0; step < steps && ball->active; step++) {
		float previousX = ball->x;
		float previousY = ball->y;

		ball->x += ball->vx / steps;
		ball->y += ball->vy / steps;
		if (ball->x <= ARK_SIDE_W) {
			ball->x = ARK_SIDE_W;
			ball->vx = fabsf(ball->vx);
			arkAdjustBallSpeed(ball, 0.2f);
			arkJitterBall(game, ball);
		}
		else if (ball->x + ARK_BALL_SIZE >= ARK_RIGHT_X) {
			ball->x = ARK_RIGHT_X - ARK_BALL_SIZE;
			ball->vx = -fabsf(ball->vx);
			arkAdjustBallSpeed(ball, 0.2f);
			arkJitterBall(game, ball);
		}
		if (ball->y <= ARK_TOP_H) {
			ball->y = ARK_TOP_H;
			ball->vy = fabsf(ball->vy);
			arkAdjustBallSpeed(ball, 0.2f);
			arkJitterBall(game, ball);
		}
		if (ball->vy > 0.0f &&
				arkOverlap(ball->x, ball->y, ARK_BALL_SIZE, ARK_BALL_SIZE,
					game->paddleX, ARK_PADDLE_Y, game->paddleW, ARK_PADDLE_H)) {
			arkPaddleBounce(game, ball);
			break;
		}
		if (arkBallBrickCollision(game, ball, previousX, previousY))
			break;
		if (arkBallEnemyCollision(game, ball))
			break;
		if (ball->y > ARK_BOTTOM_Y) {
			arkRemoveBall(game, index);
			break;
		}
	}
}

static void arkUpdatePowerups(struct ArkGame *game)
{
	for (uint8_t i = 0; i < ARK_MAX_POWERUPS; i++) {
		struct ArkPowerup *power = &game->powerups[i];

		if (!power->active)
			continue;
		power->y += 3.0f;
		if ((game->draw.frame & 3u) == 0)
			power->frame = (power->frame + 1u) & 7u;
		if (arkOverlap(power->x, power->y, 38.0f, 19.0f,
				game->paddleX, ARK_PADDLE_Y, game->paddleW, ARK_PADDLE_H)) {
			arkApplyPower(game, (enum ArkPowerupType)power->type);
			power->active = false;
		}
		else if (power->y > ARK_BOTTOM_Y)
			power->active = false;
	}
}

static void arkBulletHit(struct ArkGame *game, struct ArkBullet *bullet)
{
	for (uint8_t i = 0; i < game->brickCount; i++) {
		struct ArkBrick *brick = &game->bricks[i];

		if (brick->alive && arkOverlap(bullet->x, bullet->y, 6.0f, 15.0f,
				brick->x, brick->y, ARK_BRICK_W, ARK_BRICK_H)) {
			arkHitBrick(game, brick, true);
			bullet->active = false;
			return;
		}
	}
	for (uint8_t i = 0; i < ARK_MAX_ENEMIES; i++) {
		struct ArkEnemy *enemy = &game->enemies[i];

		if (enemy->state == ArkEnemyActive &&
				arkOverlap(bullet->x, bullet->y, 6.0f, 15.0f,
					enemy->x, enemy->y, 40.0f, 44.0f)) {
			arkExplodeEnemy(game, enemy);
			bullet->active = false;
			return;
		}
	}
}

static void arkUpdateBullets(struct ArkGame *game)
{
	for (uint8_t i = 0; i < ARK_MAX_BULLETS; i++)
		if (game->bullets[i].active) {
			game->bullets[i].y -= 15.0f;
			if (game->bullets[i].y <= ARK_TOP_H)
				game->bullets[i].active = false;
			else
				arkBulletHit(game, &game->bullets[i]);
		}
}

static void arkUpdateEnemies(struct ArkGame *game)
{
	uint16_t nearest = UINT16_MAX;
	uint8_t nearestIndex = 0;

	if (!game->enemiesReleased) {
		if (game->round != 1 || game->destroyed >= game->brickCount / 4u)
			arkReleaseEnemies(game);
		else
			return;
	}
	for (uint8_t i = 0; i < ARK_MAX_ENEMIES; i++) {
		struct ArkEnemy *enemy = &game->enemies[i];

		if (enemy->state == ArkEnemyWaiting) {
			if (enemy->timer && --enemy->timer < nearest) {
				nearest = enemy->timer;
				nearestIndex = i;
			}
			if (!enemy->timer) {
				enemy->state = ArkEnemyActive;
				enemy->x = (arkRandom(game) & 1u) ? 105.0f : 455.0f;
				enemy->y = ARK_TOP_H;
				enemy->angle = ARK_PI * 0.5f;
				enemy->steerTimer = 75;
			}
			continue;
		}
		if (enemy->state == ArkEnemyExploding) {
			enemy->timer++;
			enemy->frame = (uint8_t)(enemy->timer / 2u);
			if (enemy->frame >= 10u)
				arkEnemyRespawn(game, enemy);
			continue;
		}
		if (enemy->state != ArkEnemyActive)
			continue;
		{
			float previousX = enemy->x;
			float previousY = enemy->y;

		if ((game->draw.frame & 3u) == 0)
			enemy->frame = (enemy->frame + 1u) % 25u;
		if (enemy->steerTimer)
			enemy->steerTimer--;
		else {
			float targetX = game->paddleX + game->paddleW * 0.5f;
			float targetY = ARK_PADDLE_Y;

			enemy->angle = atan2f(targetY - enemy->y, targetX - enemy->x) +
				(arkRandomUnit(game) * 3.0f - 1.5f);
			enemy->steerTimer = (uint16_t)(30u + arkRange(game, 30u));
		}
		enemy->x += cosf(enemy->angle) * 2.0f;
		enemy->y += sinf(enemy->angle) * 2.0f;
		if (enemy->x < ARK_SIDE_W) {
			enemy->x = ARK_SIDE_W;
			enemy->angle = ARK_PI - enemy->angle;
		}
		else if (enemy->x + 40.0f > ARK_RIGHT_X) {
			enemy->x = ARK_RIGHT_X - 40.0f;
			enemy->angle = ARK_PI - enemy->angle;
		}
		if (enemy->y < ARK_TOP_H) {
			enemy->y = ARK_TOP_H;
			enemy->angle = fabsf(enemy->angle);
		}
		for (uint8_t b = 0; b < game->brickCount; b++)
			if (game->bricks[b].alive &&
					arkOverlap(enemy->x, enemy->y, 40.0f, 44.0f,
						game->bricks[b].x, game->bricks[b].y,
						ARK_BRICK_W, ARK_BRICK_H)) {
				enemy->x = previousX;
				enemy->y = previousY;
				enemy->angle += ARK_PI;
				break;
			}
		for (uint8_t other = 0; other < ARK_MAX_ENEMIES; other++)
			if (other != i && game->enemies[other].state == ArkEnemyActive &&
					arkOverlap(enemy->x, enemy->y, 40.0f, 44.0f,
						game->enemies[other].x, game->enemies[other].y,
						40.0f, 44.0f)) {
				enemy->x = previousX;
				enemy->y = previousY;
				enemy->angle += ARK_PI;
				break;
			}
		if (arkOverlap(enemy->x, enemy->y, 40.0f, 44.0f,
				game->paddleX, ARK_PADDLE_Y, game->paddleW, ARK_PADDLE_H))
			arkExplodeEnemy(game, enemy);
		else if (enemy->y > ARK_BOTTOM_Y)
			arkEnemyRespawn(game, enemy);
		}
	}
	if (nearest < 20u) {
		game->doorFrame = (uint8_t)((20u - nearest) * 6u / 20u);
		game->doorRight = (nearestIndex & 1u) != 0;
	}
	else
		game->doorFrame = 0;
}

static void arkWriteHighScore(struct ArkGame *game)
{
	struct ArkSave save = {
		.magic = ARK_SAVE_MAGIC,
		.version = ARK_SAVE_VERSION,
		.size = sizeof(struct ArkSave),
		.highScore = game->highScore,
	};

	save.check = save.magic ^ save.version ^ save.size ^ save.highScore ^ ARK_SAVE_CHECK_XOR;
	if (game->args->vol && dc32PortSaveWrite(game->args->vol, "arkanoid", &save, sizeof(save)))
		game->saveDirty = false;
}

static void arkSetGameOver(struct ArkGame *game, bool completed)
{
	game->completed = completed;
	game->state = ArkStateGameOver;
	game->stateTimer = 0;
	if (game->score > game->highScore) {
		game->highScore = game->score;
		game->saveDirty = true;
		arkWriteHighScore(game);
	}
}

static void arkUpdateGame(struct ArkGame *game)
{
	if (game->state == ArkStateReady) {
		game->stateTimer++;
		if (game->stateTimer >= 150u) {
			game->state = ArkStatePlay;
			arkFire(game);
		}
		return;
	}
	if (game->state == ArkStateLifeLost) {
		if (++game->stateTimer >= 60u) {
			if (game->lives > 1u) {
				game->lives--;
				arkBeginReady(game, true);
			}
			else {
				game->lives = 0;
				arkSetGameOver(game, false);
			}
		}
		return;
	}
	if (game->state == ArkStateRoundEnd) {
		if (++game->stateTimer >= 120u) {
			if (game->round < 5u) {
				arkBuildRound(game, game->round + 1u);
				arkBeginReady(game, false);
			}
			else {
				arkSetGameOver(game, true);
			}
		}
		return;
	}
	if (game->state != ArkStatePlay || game->paused)
		return;
	if (game->draw.keys & KEY_BIT_LEFT)
		game->paddleX -= game->paddleSpeed;
	if (game->draw.keys & KEY_BIT_RIGHT)
		game->paddleX += game->paddleSpeed;
	if (game->paddleX < ARK_SIDE_W)
		game->paddleX = ARK_SIDE_W;
	if (game->paddleX + game->paddleW > ARK_RIGHT_X)
		game->paddleX = ARK_RIGHT_X - game->paddleW;
	arkAnchorBalls(game);
	for (uint8_t i = 0; i < ARK_MAX_BALLS; i++)
		arkUpdateBall(game, i);
	arkUpdatePowerups(game);
	arkUpdateBullets(game);
	arkUpdateEnemies(game);
	if (game->transientPowerTimer && !--game->transientPowerTimer)
		game->activePower = ArkPowerNone;
	if (!game->ballCount) {
		arkDeactivatePower(game);
		game->state = ArkStateLifeLost;
		game->stateTimer = 0;
		return;
	}
	if (game->destroyed >= game->destructible) {
		arkDeactivatePower(game);
		game->state = ArkStateRoundEnd;
		game->stateTimer = 0;
	}
}

static const char *arkPowerName(uint8_t type)
{
	switch (type) {
		case ArkPowerCatch: return "CATCH";
		case ArkPowerDuplicate: return "MULTI";
		case ArkPowerExpand: return "WIDE";
		case ArkPowerLife: return "LIFE";
		case ArkPowerLaser: return "LASER";
		case ArkPowerSlow: return "SLOW";
		default: return "";
	}
}

static enum ArkanoidAssetId arkPowerAsset(uint8_t type, uint8_t frame)
{
	static const enum ArkanoidAssetId bases[] = {
		ArkAssetPowerupCatch1, ArkAssetPowerupDuplicate1, ArkAssetPowerupExpand1,
		ArkAssetPowerupLife1, ArkAssetPowerupLaser1, ArkAssetPowerupSlow1,
	};
	static const enum ArkanoidAssetId frames[6][8] = {
		{ArkAssetPowerupCatch1,ArkAssetPowerupCatch2,ArkAssetPowerupCatch3,ArkAssetPowerupCatch4,
		 ArkAssetPowerupCatch5,ArkAssetPowerupCatch6,ArkAssetPowerupCatch7,ArkAssetPowerupCatch8},
		{ArkAssetPowerupDuplicate1,ArkAssetPowerupDuplicate2,ArkAssetPowerupDuplicate3,ArkAssetPowerupDuplicate4,
		 ArkAssetPowerupDuplicate5,ArkAssetPowerupDuplicate6,ArkAssetPowerupDuplicate7,ArkAssetPowerupDuplicate8},
		{ArkAssetPowerupExpand1,ArkAssetPowerupExpand2,ArkAssetPowerupExpand3,ArkAssetPowerupExpand4,
		 ArkAssetPowerupExpand5,ArkAssetPowerupExpand6,ArkAssetPowerupExpand7,ArkAssetPowerupExpand8},
		{ArkAssetPowerupLife1,ArkAssetPowerupLife2,ArkAssetPowerupLife3,ArkAssetPowerupLife4,
		 ArkAssetPowerupLife5,ArkAssetPowerupLife6,ArkAssetPowerupLife7,ArkAssetPowerupLife8},
		{ArkAssetPowerupLaser1,ArkAssetPowerupLaser2,ArkAssetPowerupLaser3,ArkAssetPowerupLaser4,
		 ArkAssetPowerupLaser5,ArkAssetPowerupLaser6,ArkAssetPowerupLaser7,ArkAssetPowerupLaser8},
		{ArkAssetPowerupSlow1,ArkAssetPowerupSlow2,ArkAssetPowerupSlow3,ArkAssetPowerupSlow4,
		 ArkAssetPowerupSlow5,ArkAssetPowerupSlow6,ArkAssetPowerupSlow7,ArkAssetPowerupSlow8},
	};

	if (type < ArkPowerCatch || type > ArkPowerSlow)
		return bases[0];
	return frames[type - ArkPowerCatch][frame & 7u];
}

static enum ArkanoidAssetId arkEnemyAsset(const struct ArkGame *game,
	const struct ArkEnemy *enemy)
{
	if (enemy->state == ArkEnemyExploding)
		return mExplosionFrames[enemy->frame < 10u ? enemy->frame : 9u];
	switch (game->enemyType) {
		case ArkEnemyPyramid: return mPyramidFrames[enemy->frame % 25u];
		case ArkEnemyMolecule: return mMoleculeFrames[enemy->frame % 25u];
		case ArkEnemyCube: return mCubeFrames[enemy->frame % 25u];
		default: return mConeFrames[enemy->frame % 25u];
	}
}

static enum ArkanoidAssetId arkPaddleAsset(const struct ArkGame *game)
{
	static const enum ArkanoidAssetId materialize[15] = {
		ArkAssetPaddleMaterialize1, ArkAssetPaddleMaterialize2, ArkAssetPaddleMaterialize3,
		ArkAssetPaddleMaterialize4, ArkAssetPaddleMaterialize5, ArkAssetPaddleMaterialize6,
		ArkAssetPaddleMaterialize7, ArkAssetPaddleMaterialize8, ArkAssetPaddleMaterialize9,
		ArkAssetPaddleMaterialize10, ArkAssetPaddleMaterialize11, ArkAssetPaddleMaterialize12,
		ArkAssetPaddleMaterialize13, ArkAssetPaddleMaterialize14, ArkAssetPaddleMaterialize15,
	};
	static const enum ArkanoidAssetId explode[8] = {
		ArkAssetPaddleExplode1, ArkAssetPaddleExplode2, ArkAssetPaddleExplode3,
		ArkAssetPaddleExplode4, ArkAssetPaddleExplode5, ArkAssetPaddleExplode6,
		ArkAssetPaddleExplode7, ArkAssetPaddleExplode8,
	};

	if (game->state == ArkStateReady && game->stateTimer >= 75u && game->stateTimer < 120u)
		return materialize[((game->stateTimer - 75u) / 3u) % 15u];
	if (game->state == ArkStateLifeLost)
		return explode[(game->stateTimer / 7u) < 8u ? game->stateTimer / 7u : 7u];
	if (game->activePower == ArkPowerExpand)
		return ArkAssetPaddleWidePulsate1 + (game->draw.frame / 4u) % 4u;
	if (game->activePower == ArkPowerLaser)
		return ArkAssetPaddleLaserPulsate1 + (game->draw.frame / 4u) % 4u;
	return ArkAssetPaddlePulsate1 + (game->draw.frame / 4u) % 4u;
}

static void arkDrawArena(struct ArkGame *game)
{
	uint8_t bg = game->round == 2 ? arkRgb(0, 128, 0) :
		game->round == 4 ? arkRgb(128, 0, 0) : arkRgb(0, 0, 128);

	for (int32_t y = 0; y < ARK_ARENA_H; y++)
		memset(game->frame + y * ARK_SCREEN_W + ARK_ARENA_X, bg, ARK_ARENA_W);
	arkBlit(game, ArkAssetEdgeLeft, ARK_ARENA_X, 0);
	arkBlit(game, ArkAssetEdgeRight, ARK_ARENA_X + ARK_ARENA_W -
		arkanoidAssets[ArkAssetEdgeRight].width, 0);
	arkBlit(game, ArkAssetEdgeTop, arkScreenX(ARK_SIDE_W), 0);
	if (game->doorFrame) {
		enum ArkanoidAssetId door = (enum ArkanoidAssetId)(
			(game->doorRight ? ArkAssetDoorTopRight1 : ArkAssetDoorTopLeft1) +
			game->doorFrame);

		arkBlit(game, door, arkScreenX(ARK_SIDE_W), 0);
	}
	for (uint8_t i = 0; i < game->brickCount; i++) {
		const struct ArkBrick *brick = &game->bricks[i];

		if (brick->alive)
			arkBlit(game, arkBrickAsset(brick), arkScreenX(brick->x), arkScreenY(brick->y));
	}
	for (uint8_t i = 0; i < ARK_MAX_POWERUPS; i++)
		if (game->powerups[i].active)
			arkBlit(game, arkPowerAsset(game->powerups[i].type, game->powerups[i].frame),
				arkScreenX(game->powerups[i].x), arkScreenY(game->powerups[i].y));
	for (uint8_t i = 0; i < ARK_MAX_BULLETS; i++)
		if (game->bullets[i].active)
			arkBlit(game, ArkAssetLaserBullet, arkScreenX(game->bullets[i].x),
				arkScreenY(game->bullets[i].y));
	for (uint8_t i = 0; i < ARK_MAX_ENEMIES; i++)
		if (game->enemies[i].state == ArkEnemyActive ||
				game->enemies[i].state == ArkEnemyExploding)
			arkBlit(game, arkEnemyAsset(game, &game->enemies[i]),
				arkScreenX(game->enemies[i].x), arkScreenY(game->enemies[i].y));
	if (game->state != ArkStateRoundEnd && game->state != ArkStateGameOver) {
		arkBlit(game, arkPaddleAsset(game), arkScreenX(game->paddleX), arkScreenY(ARK_PADDLE_Y));
		for (uint8_t i = 0; i < ARK_MAX_BALLS; i++)
			if (game->balls[i].active)
				arkBlit(game, ArkAssetBall, arkScreenX(game->balls[i].x),
					arkScreenY(game->balls[i].y));
	}
}

static void arkDrawHud(struct ArkGame *game)
{
	char text[12];
	int32_t rightX = ARK_ARENA_X + ARK_ARENA_W;
	int32_t rightW = ARK_SCREEN_W - rightX;
	uint8_t red = arkRgb(230, 0, 0);
	uint8_t white = arkRgb(255, 255, 255);
	uint8_t gray = arkRgb(150, 150, 170);

	arkBlitRotatedCounterClockwise(game, ArkAssetLogo, 3, 61, 42, 118);
	arkCenteredText(game, rightX, rightW, 5, "SCORE", FontSmall, red);
	arkNumber(game, rightX, rightW, 18, game->score, white);
	arkCenteredText(game, rightX, rightW, 42, "HIGH", FontSmall, red);
	arkNumber(game, rightX, rightW, 55, game->highScore, white);
	arkCenteredText(game, rightX, rightW, 79, "ROUND", FontSmall, red);
	snprintf(text, sizeof(text), "%u", game->round);
	arkCenteredText(game, rightX, rightW, 92, text, FontSmall, white);
	arkCenteredText(game, rightX, rightW, 116, "LIVES", FontSmall, red);
	snprintf(text, sizeof(text), "%u", game->lives);
	arkCenteredText(game, rightX, rightW, 129, text, FontSmall, white);
	if (game->activePower != ArkPowerNone) {
		arkCenteredText(game, rightX, rightW, 153, "POWER", FontSmall, red);
		arkCenteredText(game, rightX, rightW, 166,
			arkPowerName(game->activePower), FontSmall, white);
	}
	arkCenteredText(game, rightX, rightW, 220, "A FIRE", FontSmall, gray);
}

static void arkDrawTitle(struct ArkGame *game)
{
	static const uint8_t powers[] = {
		ArkPowerLaser, ArkPowerSlow, ArkPowerLife,
		ArkPowerExpand, ArkPowerCatch, ArkPowerDuplicate,
	};
	uint8_t white = arkRgb(255, 255, 255);
	uint8_t yellow = arkRgb(255, 255, 0);
	uint8_t gray = arkRgb(128, 128, 128);
	char roundText[16];

	memset(game->frame, 0, ARK_SCREEN_W * ARK_SCREEN_H);
	arkBlit(game, ArkAssetLogo,
		ARK_ARENA_X + (ARK_ARENA_W - arkanoidAssets[ArkAssetLogo].width) / 2, 4);
	arkCenteredText(game, ARK_ARENA_X, ARK_ARENA_W, 62,
		"POWERUPS", FontMedium, white);
	for (uint8_t i = 0; i < 6; i++) {
		int32_t col = i % 2u;
		int32_t row = i / 2u;
		int32_t x = ARK_ARENA_X + 14 + col * 108;
		int32_t y = 86 + row * 30;

		arkBlit(game, arkPowerAsset(powers[i], (game->draw.frame / 4u) & 7u), x, y);
		arkText(game, x + 17, y, arkPowerName(powers[i]), FontSmall, white);
	}
	snprintf(roundText, sizeof(roundText), "ROUND %u", game->selectedRound);
	arkCenteredText(game, ARK_ARENA_X, ARK_ARENA_W, 181, roundText, FontMedium, yellow);
	arkCenteredText(game, ARK_ARENA_X, ARK_ARENA_W, 201,
		"UP/DOWN SELECT", FontSmall, gray);
	arkCenteredText(game, ARK_ARENA_X, ARK_ARENA_W, 216,
		"A OR START", FontMedium, white);
}

static void arkRender(struct ArkGame *game)
{
	uint8_t white = arkRgb(255, 255, 255);
	uint8_t yellow = arkRgb(255, 255, 0);
	char text[20];

	if (game->state == ArkStateTitle) {
		arkDrawTitle(game);
		return;
	}
	memset(game->frame, 0, ARK_SCREEN_W * ARK_SCREEN_H);
	arkDrawArena(game);
	arkDrawHud(game);
	if (game->state == ArkStateReady) {
		snprintf(text, sizeof(text), "ROUND %u", game->round);
		if (game->stateTimer > 30u)
			arkCenteredText(game, ARK_ARENA_X, ARK_ARENA_W, 145, text, FontMedium, white);
		if (game->stateTimer > 75u)
			arkCenteredText(game, ARK_ARENA_X, ARK_ARENA_W, 164, "READY", FontMedium, yellow);
	}
	else if (game->state == ArkStateRoundEnd)
		arkCenteredText(game, ARK_ARENA_X, ARK_ARENA_W, 150,
			"ROUND CLEAR", FontMedium, yellow);
	else if (game->state == ArkStateGameOver) {
		arkCenteredText(game, ARK_ARENA_X, ARK_ARENA_W, 135,
			game->completed ? "ALL CLEAR" : "GAME OVER", FontLarge, yellow);
		arkCenteredText(game, ARK_ARENA_X, ARK_ARENA_W, 160,
			"A TO TITLE", FontSmall, white);
	}
	if (game->paused)
		arkCenteredText(game, ARK_ARENA_X, ARK_ARENA_W, 150,
			"PAUSED", FontLarge, yellow);
}

static void arkHandleInput(struct ArkGame *game)
{
	uint_fast16_t pressed = game->draw.pressed;

	if (game->state == ArkStateTitle) {
		if (pressed & KEY_BIT_UP)
			game->selectedRound = game->selectedRound > 1u ? game->selectedRound - 1u : 5u;
		if (pressed & KEY_BIT_DOWN)
			game->selectedRound = game->selectedRound < 5u ? game->selectedRound + 1u : 1u;
		if (pressed & (KEY_BIT_A | KEY_BIT_START))
			arkStartGame(game);
		return;
	}
	if (game->state == ArkStateGameOver) {
		if (pressed & (KEY_BIT_A | KEY_BIT_START)) {
			game->state = ArkStateTitle;
			game->round = game->selectedRound;
			game->paused = false;
		}
		return;
	}
	if (game->state == ArkStatePlay && (pressed & KEY_BIT_START)) {
		game->paused = !game->paused;
		return;
	}
	if (!game->paused && game->state == ArkStatePlay && (pressed & KEY_BIT_A))
		arkFire(game);
}

static void arkLoadHighScore(struct ArkGame *game)
{
	struct ArkSave save;
	uint32_t check;

	memset(&save, 0, sizeof(save));
	if (!game->args->vol ||
			!dc32PortSaveRead(game->args->vol, "arkanoid", &save, sizeof(save)))
		return;
	check = save.magic ^ save.version ^ save.size ^ save.highScore ^ ARK_SAVE_CHECK_XOR;
	if (save.magic == ARK_SAVE_MAGIC && save.version == ARK_SAVE_VERSION &&
			save.size == sizeof(save) && save.check == check)
		game->highScore = save.highScore;
}

static bool arkInit(struct ArkGame *game, const struct DcAppHostApi *host,
	const struct DcAppRunArgs *args)
{
	uint64_t now;

	if (!game || !host || !args || !args->canvas)
		return false;
	memset(game, 0, sizeof(*game));
	game->host = host;
	game->args = args;
	game->frame = dc32PortMalloc(ARK_SCREEN_W * ARK_SCREEN_H);
	if (!game->frame)
		return false;
	if (!dcAppDrawInit(&game->draw, host, args, game->frame, ARK_SCREEN_W, ARK_SCREEN_H))
		return false;
	now = host->getTime ? host->getTime() : 0x41524b31u;
	game->rng = (uint32_t)(now ^ (now >> 32));
	if (!game->rng)
		game->rng = 0x41524b31u;
	game->state = ArkStateTitle;
	game->selectedRound = 1;
	game->round = 1;
	game->paddleW = ARK_PADDLE_NORMAL_W;
	arkLoadHighScore(game);
	return true;
}

int arkanoidAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct ArkGame *game;

	mArkanoidAbort = false;
	dc32PortHeapInitDefault();
	game = dc32PortCalloc(1, sizeof(*game));
	if (!game)
		return -1;
	if (!arkInit(game, host, args)) {
		dc32PortFree(game->frame);
		dc32PortFree(game);
		return -1;
	}
	dispSetFramerate(ARK_FPS);
	dcAppDrawWaitRelease(&game->draw, KEY_BIT_A | KEY_BIT_B | KEY_BIT_START |
		KEY_BIT_UP | KEY_BIT_DOWN | UI_KEY_BIT_CENTER);
	arkRender(game);
	while (!mArkanoidAbort) {
		if (!dcAppDrawFrame(&game->draw, UI_KEY_BIT_CENTER))
			break;
		arkHandleInput(game);
		arkUpdateGame(game);
		arkRender(game);
	}
	if (game->score > game->highScore) {
		game->highScore = game->score;
		game->saveDirty = true;
	}
	if (game->saveDirty)
		arkWriteHighScore(game);
	dcAppDrawWaitRelease(&game->draw, UI_KEY_BIT_CENTER);
	dc32PortFree(game->frame);
	dc32PortFree(game);
	dispSetFramerate(60);
	return 0;
}

void arkanoidAppAbort(void)
{
	mArkanoidAbort = true;
}
