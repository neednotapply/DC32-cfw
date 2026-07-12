/*
 * Native DC32 adaptation of wayou/t-rex-runner commit
 * 5455bfa408ec6b707c7300ff194b7390733a766d.
 *
 * The original source and sprite are retained under third_party/t-rex-runner.
 */
#include "apps/trex/trex_port.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "apps/port/port_runtime.h"
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "trex_assets.h"
#include "ui.h"

#define TREX_SCREEN_W 320
#define TREX_SCREEN_H 240
#define TREX_CANVAS_W 320
#define TREX_CANVAS_H 150
#define TREX_WAIT_CANVAS_Y 10
#define TREX_ARCADE_CANVAS_Y 16
#define TREX_FPS 60u
#define TREX_FRAME_MS (1000.0f / 60.0f)

#define TREX_RUNNER_ACCELERATION 0.001f
#define TREX_RUNNER_BOTTOM_PAD 10
#define TREX_RUNNER_CLEAR_TIME 3000.0f
#define TREX_RUNNER_GAMEOVER_CLEAR_TIME 750.0f
#define TREX_RUNNER_GAP_COEFFICIENT 0.6f
#define TREX_RUNNER_INVERT_FADE_DURATION 12000.0f
#define TREX_RUNNER_INVERT_DISTANCE 700u
#define TREX_RUNNER_MAX_BLINK_COUNT 3u
#define TREX_RUNNER_MAX_OBSTACLE_DUPLICATION 2u
#define TREX_RUNNER_MAX_SPEED 13.0f
#define TREX_RUNNER_SPEED 6.0f

#define TREX_PLAYER_DROP_VELOCITY (-5.0f)
#define TREX_PLAYER_GRAVITY 0.6f
#define TREX_PLAYER_HEIGHT 47
#define TREX_PLAYER_INITIAL_JUMP_VELOCITY (-10.0f)
#define TREX_PLAYER_INTRO_DURATION 1500.0f
#define TREX_PLAYER_MAX_JUMP_HEIGHT 30
#define TREX_PLAYER_MIN_JUMP_HEIGHT 30
#define TREX_PLAYER_SPEED_DROP_COEFFICIENT 3.0f
#define TREX_PLAYER_START_X 50
#define TREX_PLAYER_WIDTH 44
#define TREX_PLAYER_DUCK_WIDTH 59
#define TREX_PLAYER_BLINK_TIMING 7000.0f

#define TREX_INTRO_DURATION 400.0f
#define TREX_ACHIEVEMENT_DISTANCE 100u
#define TREX_DISTANCE_COEFFICIENT 0.025f
#define TREX_FLASH_DURATION 250.0f
#define TREX_FLASH_ITERATIONS 3u

#define TREX_MAX_OBSTACLES 8u
#define TREX_MAX_CLOUDS 6u
#define TREX_SAVE_MAGIC 0x31585254u
#define TREX_SAVE_VERSION 1u
#define TREX_SAVE_CHECK_XOR 0xa5c39e71u

enum TrexSprite {
	TrexSpriteRestartX = 2,
	TrexSpriteRestartY = 2,
	TrexSpriteCloudX = 86,
	TrexSpriteCloudY = 2,
	TrexSpritePterodactylX = 134,
	TrexSpritePterodactylY = 2,
	TrexSpriteCactusSmallX = 228,
	TrexSpriteCactusSmallY = 2,
	TrexSpriteCactusLargeX = 332,
	TrexSpriteCactusLargeY = 2,
	TrexSpriteMoonX = 484,
	TrexSpriteMoonY = 2,
	TrexSpriteStarX = 645,
	TrexSpriteStarY = 2,
	TrexSpriteTextX = 655,
	TrexSpriteTextY = 2,
	TrexSpritePlayerX = 848,
	TrexSpritePlayerY = 2,
	TrexSpriteHorizonX = 2,
	TrexSpriteHorizonY = 54,
};

enum TrexStatus {
	TrexStatusCrashed,
	TrexStatusDucking,
	TrexStatusJumping,
	TrexStatusRunning,
	TrexStatusWaiting,
};

enum TrexObstacleType {
	TrexObstacleSmall,
	TrexObstacleLarge,
	TrexObstaclePterodactyl,
	TrexObstacleTypeCount,
};

struct TrexCollisionBox {
	int16_t x;
	int16_t y;
	int16_t w;
	int16_t h;
};

struct TrexObstacleConfig {
	int16_t spriteX;
	int16_t spriteY;
	int16_t width;
	int16_t height;
	int16_t yPos;
	float multipleSpeed;
	float minSpeed;
	int16_t minGap;
	uint8_t collisionCount;
	uint8_t frameCount;
	float frameRate;
	float speedOffset;
	struct TrexCollisionBox collision[5];
};

struct TrexObstacle {
	enum TrexObstacleType type;
	int16_t x;
	int16_t y;
	int16_t width;
	int16_t gap;
	int8_t size;
	int8_t currentFrame;
	float timer;
	float speedOffset;
	bool followingCreated;
	struct TrexCollisionBox collision[5];
};

struct TrexCloud {
	float x;
	int16_t y;
	int16_t gap;
};

struct TrexStar {
	float x;
	int16_t y;
	int16_t sourceY;
};

struct TrexNight {
	float x;
	float opacity;
	uint8_t phase;
	bool drawStars;
	struct TrexStar stars[2];
};

struct TrexPlayer {
	int16_t x;
	int16_t y;
	int16_t groundY;
	int16_t minJumpY;
	float jumpVelocity;
	float timer;
	float msPerFrame;
	float blinkDelay;
	float blinkElapsed;
	uint8_t currentFrame;
	uint8_t waitingDrawFrame;
	uint8_t blinkCount;
	uint8_t jumpCount;
	enum TrexStatus status;
	bool jumping;
	bool ducking;
	bool reachedMinHeight;
	bool speedDrop;
	bool playingIntro;
};

struct TrexSave {
	uint32_t magic;
	uint16_t version;
	uint16_t size;
	uint32_t highestDistance;
	uint32_t check;
};

struct TrexGame {
	const struct DcAppHostApi *host;
	const struct DcAppRunArgs *args;
	struct DcAppDrawCtx draw;
	uint8_t *frame;
	uint32_t rng;
	uint_fast16_t previousKeys;
	float currentSpeed;
	float distanceRan;
	float runningTime;
	float introElapsed;
	float crashElapsed;
	float invertTimer;
	uint32_t highestDistance;
	int16_t canvasY;
	int16_t clipWidth;
	int16_t horizonX[2];
	int16_t horizonSourceX[2];
	struct TrexPlayer player;
	struct TrexObstacle obstacles[TREX_MAX_OBSTACLES];
	uint8_t obstacleCount;
	enum TrexObstacleType obstacleHistory[2];
	uint8_t obstacleHistoryCount;
	struct TrexCloud clouds[TREX_MAX_CLOUDS];
	uint8_t cloudCount;
	struct TrexNight night;
	uint8_t scoreUnits;
	float scoreFlashTimer;
	uint8_t scoreFlashIterations;
	bool scoreAchievement;
	bool scorePaint;
	bool playing;
	bool paused;
	bool crashed;
	bool activated;
	bool arcade;
	bool playingIntro;
	bool inverted;
	bool invertTrigger;
	bool saveDirty;
	bool saveAfterPresent;
};

static volatile bool mTrexAbort;

static const struct TrexObstacleConfig mObstacleConfigs[TrexObstacleTypeCount] = {
	[TrexObstacleSmall] = {
		.spriteX = TrexSpriteCactusSmallX,
		.spriteY = TrexSpriteCactusSmallY,
		.width = 17,
		.height = 35,
		.yPos = 105,
		.multipleSpeed = 4.0f,
		.minSpeed = 0.0f,
		.minGap = 120,
		.collisionCount = 3,
		.collision = {{0, 7, 5, 27}, {4, 0, 6, 34}, {10, 4, 7, 14}},
	},
	[TrexObstacleLarge] = {
		.spriteX = TrexSpriteCactusLargeX,
		.spriteY = TrexSpriteCactusLargeY,
		.width = 25,
		.height = 50,
		.yPos = 90,
		.multipleSpeed = 7.0f,
		.minSpeed = 0.0f,
		.minGap = 120,
		.collisionCount = 3,
		.collision = {{0, 12, 7, 38}, {8, 0, 7, 49}, {13, 10, 10, 38}},
	},
	[TrexObstaclePterodactyl] = {
		.spriteX = TrexSpritePterodactylX,
		.spriteY = TrexSpritePterodactylY,
		.width = 46,
		.height = 40,
		.yPos = 100,
		.multipleSpeed = 999.0f,
		.minSpeed = 8.5f,
		.minGap = 150,
		.collisionCount = 5,
		.frameCount = 2,
		.frameRate = 1000.0f / 6.0f,
		.speedOffset = 0.8f,
		.collision = {{15, 15, 16, 5}, {18, 21, 24, 6}, {2, 14, 4, 3}, {6, 10, 4, 7}, {10, 8, 6, 9}},
	},
};

static const struct TrexCollisionBox mPlayerRunningBoxes[] = {
	{22, 0, 17, 16},
	{1, 18, 30, 9},
	{10, 35, 14, 8},
	{1, 24, 29, 5},
	{5, 30, 21, 4},
	{9, 34, 15, 4},
};

static const struct TrexCollisionBox mPlayerDuckingBoxes[] = {
	{1, 18, 55, 25},
};

static const uint8_t mNightPhases[] = {140, 120, 100, 60, 40, 20, 0};

static uint32_t trexRandom(struct TrexGame *game)
{
	game->rng = game->rng * 1664525u + 1013904223u;
	return game->rng;
}

static int32_t trexRandomRange(struct TrexGame *game, int32_t min, int32_t max)
{
	uint32_t span = (uint32_t)(max - min + 1);

	return min + (int32_t)(trexRandom(game) % span);
}

static float trexRandomUnit(struct TrexGame *game)
{
	return (float)(trexRandom(game) >> 8) * (1.0f / 16777216.0f);
}

static int32_t trexJsRound(float value)
{
	return (int32_t)floorf(value + 0.5f);
}

static uint8_t trexGray332(uint32_t gray)
{
	return (uint8_t)(gray & 0xe0u) | (uint8_t)((gray >> 3) & 0x1cu) | (uint8_t)(gray >> 6);
}

static uint8_t trexRgb332Gray(uint8_t color)
{
	return (uint8_t)((((color >> 2) & 7u) * 255u + 3u) / 7u);
}

static uint8_t trexDisplayGray(const struct TrexGame *game, uint8_t gray)
{
	return game->inverted ? (uint8_t)(255u - gray) : gray;
}

static void trexPutGray(struct TrexGame *game, int32_t x, int32_t y, uint8_t gray, uint8_t alpha)
{
	uint32_t index;

	if (x < 0 || y < 0 || x >= TREX_SCREEN_W || y >= TREX_SCREEN_H || !alpha)
		return;
	gray = trexDisplayGray(game, gray);
	index = (uint32_t)y * TREX_SCREEN_W + (uint32_t)x;
	if (alpha != 255u) {
		uint32_t dst = trexRgb332Gray(game->frame[index]);

		gray = (uint8_t)((gray * alpha + dst * (255u - alpha) + 127u) / 255u);
	}
	game->frame[index] = trexGray332(gray);
}

static void trexBlit(struct TrexGame *game, int32_t sourceX, int32_t sourceY,
	int32_t width, int32_t height, int32_t targetX, int32_t targetY, uint8_t alpha)
{
	int32_t startX = 0, startY = 0, endX = width, endY = height;

	if (targetX < 0)
		startX = -targetX;
	if (targetY < 0)
		startY = -targetY;
	if (targetX + endX > game->clipWidth)
		endX = game->clipWidth - targetX;
	if (targetY + endY > TREX_CANVAS_H)
		endY = TREX_CANVAS_H - targetY;
	if (sourceX < 0)
		startX = -sourceX;
	if (sourceY < 0)
		startY = -sourceY;
	if (sourceX + endX > (int32_t)TREX_SPRITE_WIDTH)
		endX = (int32_t)TREX_SPRITE_WIDTH - sourceX;
	if (sourceY + endY > (int32_t)TREX_SPRITE_HEIGHT)
		endY = (int32_t)TREX_SPRITE_HEIGHT - sourceY;
	if (startX >= endX || startY >= endY)
		return;

	for (int32_t y = startY; y < endY; y++) {
		const uint8_t *src = trexSpritePixels +
			(uint32_t)(sourceY + y) * TREX_SPRITE_WIDTH + (uint32_t)(sourceX + startX);

		for (int32_t x = startX; x < endX; x++, src++) {
			if (*src != TREX_SPRITE_TRANSPARENT)
				trexPutGray(game, targetX + x, game->canvasY + targetY + y, *src, alpha);
		}
	}
}

static uint32_t trexTextWidth(const char *text, enum Font font)
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

static void trexDrawText(struct TrexGame *game, int32_t x, int32_t y,
	const char *text, enum Font font, uint8_t gray)
{
	while (text && *text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text)) {
			for (uint_fast8_t row = 0; row < glyph.height; row++)
				for (uint_fast8_t col = 0; col < glyph.width; col++)
					if (fontGetGlyphPixel(&glyph, row, col))
						trexPutGray(game, x + col, y + row, gray, 255u);
			x += glyph.width + 1;
		}
		text++;
	}
}

static void trexDrawPrompt(struct TrexGame *game)
{
	static const char prompt[] = "PRESS A/UP TO START";
	uint32_t width = trexTextWidth(prompt, FontLarge);

	trexDrawText(game, (int32_t)(TREX_SCREEN_W - width) / 2, 2, prompt, FontLarge, 83u);
}

static void trexSetBlinkDelay(struct TrexGame *game)
{
	game->player.blinkDelay = ceilf(trexRandomUnit(game) * TREX_PLAYER_BLINK_TIMING);
}

static void trexSetPlayerStatus(struct TrexGame *game, enum TrexStatus status)
{
	struct TrexPlayer *player = &game->player;

	player->status = status;
	player->currentFrame = 0;
	switch (status) {
	case TrexStatusWaiting:
		player->msPerFrame = 1000.0f / 3.0f;
		player->blinkElapsed = 0.0f;
		trexSetBlinkDelay(game);
		break;
	case TrexStatusRunning:
		player->msPerFrame = 1000.0f / 12.0f;
		break;
	case TrexStatusDucking:
		player->msPerFrame = 1000.0f / 8.0f;
		break;
	case TrexStatusCrashed:
	case TrexStatusJumping:
	default:
		player->msPerFrame = TREX_FRAME_MS;
		break;
	}
}

static void trexResetPlayer(struct TrexGame *game)
{
	struct TrexPlayer *player = &game->player;

	player->y = player->groundY;
	player->jumpVelocity = 0.0f;
	player->jumping = false;
	player->ducking = false;
	trexSetPlayerStatus(game, TrexStatusRunning);
	player->speedDrop = false;
	player->jumpCount = 0;
}

static void trexInitPlayer(struct TrexGame *game)
{
	struct TrexPlayer *player = &game->player;

	memset(player, 0, sizeof(*player));
	player->groundY = TREX_CANVAS_H - TREX_PLAYER_HEIGHT - TREX_RUNNER_BOTTOM_PAD;
	player->y = player->groundY;
	player->minJumpY = player->groundY - TREX_PLAYER_MIN_JUMP_HEIGHT;
	player->waitingDrawFrame = 0;
	trexSetPlayerStatus(game, TrexStatusWaiting);
}

static void trexSetDuck(struct TrexGame *game, bool duck)
{
	struct TrexPlayer *player = &game->player;

	if (duck && player->status != TrexStatusDucking) {
		trexSetPlayerStatus(game, TrexStatusDucking);
		player->ducking = true;
	}
	else if (!duck && player->status == TrexStatusDucking) {
		trexSetPlayerStatus(game, TrexStatusRunning);
		player->ducking = false;
	}
}

static void trexEndJump(struct TrexGame *game)
{
	struct TrexPlayer *player = &game->player;

	if (player->reachedMinHeight && player->jumpVelocity < TREX_PLAYER_DROP_VELOCITY)
		player->jumpVelocity = TREX_PLAYER_DROP_VELOCITY;
}

static void trexStartJump(struct TrexGame *game)
{
	struct TrexPlayer *player = &game->player;

	if (player->jumping)
		return;
	trexSetPlayerStatus(game, TrexStatusJumping);
	player->jumpVelocity = TREX_PLAYER_INITIAL_JUMP_VELOCITY - game->currentSpeed / 10.0f;
	player->jumping = true;
	player->reachedMinHeight = false;
	player->speedDrop = false;
}

static void trexSetSpeedDrop(struct TrexGame *game)
{
	game->player.speedDrop = true;
	game->player.jumpVelocity = 1.0f;
}

static int16_t trexPlayerFrameOffset(const struct TrexPlayer *player)
{
	switch (player->status) {
	case TrexStatusWaiting:
		return player->waitingDrawFrame ? 44u : 0u;
	case TrexStatusRunning:
		return player->currentFrame ? 132u : 88u;
	case TrexStatusDucking:
		return player->currentFrame ? 323u : 264u;
	case TrexStatusCrashed:
		return 220u;
	case TrexStatusJumping:
	default:
		return 0u;
	}
}

static void trexUpdatePlayerAnimation(struct TrexGame *game, float delta)
{
	struct TrexPlayer *player = &game->player;
	uint8_t frameCount = player->status == TrexStatusRunning ||
		player->status == TrexStatusDucking || player->status == TrexStatusWaiting ? 2u : 1u;

	player->timer += delta;
	if (player->playingIntro && player->x < TREX_PLAYER_START_X)
		player->x += (int16_t)trexJsRound((TREX_PLAYER_START_X / TREX_PLAYER_INTRO_DURATION) * delta);

	if (player->status == TrexStatusWaiting) {
		player->blinkElapsed += delta;
		if (player->blinkElapsed >= player->blinkDelay) {
			player->waitingDrawFrame = player->currentFrame == 0 ? 44u : 0u;
			if (player->currentFrame == 1u) {
				trexSetBlinkDelay(game);
				player->blinkElapsed = 0.0f;
				player->blinkCount++;
			}
		}
	}

	if (player->timer >= player->msPerFrame) {
		player->currentFrame = player->currentFrame == frameCount - 1u ? 0u : player->currentFrame + 1u;
		player->timer = 0.0f;
	}

	if (player->speedDrop && player->y == player->groundY) {
		player->speedDrop = false;
		trexSetDuck(game, true);
	}
}

static void trexUpdateJump(struct TrexGame *game, float delta)
{
	struct TrexPlayer *player = &game->player;
	float framesElapsed = delta / TREX_FRAME_MS;
	float movement = player->jumpVelocity * framesElapsed;

	if (player->speedDrop)
		movement *= TREX_PLAYER_SPEED_DROP_COEFFICIENT;
	player->y += (int16_t)trexJsRound(movement);
	player->jumpVelocity += TREX_PLAYER_GRAVITY * framesElapsed;

	if (player->y < player->minJumpY || player->speedDrop)
		player->reachedMinHeight = true;
	if (player->y < TREX_PLAYER_MAX_JUMP_HEIGHT || player->speedDrop)
		trexEndJump(game);
	if (player->y > player->groundY) {
		trexResetPlayer(game);
		player->jumpCount++;
	}
	trexUpdatePlayerAnimation(game, delta);
}

static void trexPlaceStars(struct TrexGame *game)
{
	int32_t segment = trexJsRound((float)TREX_CANVAS_W / 2.0f);

	for (uint32_t i = 0; i < 2u; i++) {
		game->night.stars[i].x = (float)trexRandomRange(game, segment * (int32_t)i,
			segment * (int32_t)(i + 1u));
		game->night.stars[i].y = (int16_t)trexRandomRange(game, 0, 70);
		game->night.stars[i].sourceY = (int16_t)(TrexSpriteStarY + 9 * (int32_t)i);
	}
}

static void trexResetNight(struct TrexGame *game)
{
	game->night.phase = 0;
	game->night.opacity = 0.0f;
	trexPlaceStars(game);
	game->night.drawStars = true;
}

static float trexWrapNightX(float x, float speed)
{
	return x < -20.0f ? (float)TREX_CANVAS_W : x - speed;
}

static void trexUpdateNight(struct TrexGame *game, bool active)
{
	struct TrexNight *night = &game->night;

	if (active && night->opacity == 0.0f) {
		night->phase++;
		if (night->phase >= sizeof(mNightPhases))
			night->phase = 0;
	}
	if (active && (night->opacity < 1.0f || night->opacity == 0.0f))
		night->opacity += 0.035f;
	else if (night->opacity > 0.0f)
		night->opacity -= 0.035f;

	if (night->opacity > 0.0f) {
		night->x = trexWrapNightX(night->x, 0.25f);
		if (night->drawStars)
			for (uint32_t i = 0; i < 2u; i++)
				night->stars[i].x = trexWrapNightX(night->stars[i].x, 0.3f);
	}
	else {
		night->opacity = 0.0f;
		trexPlaceStars(game);
	}
	night->drawStars = true;
}

static void trexAddCloud(struct TrexGame *game)
{
	struct TrexCloud *cloud;

	if (game->cloudCount >= TREX_MAX_CLOUDS)
		return;
	cloud = &game->clouds[game->cloudCount++];
	cloud->x = (float)TREX_CANVAS_W;
	cloud->y = (int16_t)trexRandomRange(game, 30, 71);
	cloud->gap = (int16_t)trexRandomRange(game, 100, 400);
}

static void trexUpdateClouds(struct TrexGame *game, float delta, float speed)
{
	float movement = ceilf(0.2f / 1000.0f * delta * speed);
	uint8_t out = 0;

	for (uint8_t i = 0; i < game->cloudCount; i++) {
		struct TrexCloud cloud = game->clouds[i];

		cloud.x -= movement;
		if (cloud.x + 46.0f > 0.0f)
			game->clouds[out++] = cloud;
	}
	game->cloudCount = out;
	if (game->cloudCount) {
		struct TrexCloud *last = &game->clouds[game->cloudCount - 1u];

		if (game->cloudCount < TREX_MAX_CLOUDS &&
				(float)TREX_CANVAS_W - last->x > (float)last->gap &&
				trexRandomUnit(game) < 0.5f)
			trexAddCloud(game);
	}
	else {
		trexAddCloud(game);
	}
}

static bool trexDuplicateObstacle(const struct TrexGame *game, enum TrexObstacleType type)
{
	uint8_t duplicate = 0;

	for (uint8_t i = 0; i < game->obstacleHistoryCount; i++)
		duplicate = game->obstacleHistory[i] == type ? duplicate + 1u : 0u;
	return duplicate >= TREX_RUNNER_MAX_OBSTACLE_DUPLICATION;
}

static void trexAddObstacle(struct TrexGame *game)
{
	enum TrexObstacleType type = TrexObstacleSmall;
	const struct TrexObstacleConfig *config;
	struct TrexObstacle *obstacle;
	bool found = false;

	if (game->obstacleCount >= TREX_MAX_OBSTACLES)
		return;
	for (uint32_t attempt = 0; attempt < 64u; attempt++) {
		type = (enum TrexObstacleType)trexRandomRange(game, 0, TrexObstacleTypeCount - 1);
		config = &mObstacleConfigs[type];
		if (!trexDuplicateObstacle(game, type) && game->currentSpeed >= config->minSpeed) {
			found = true;
			break;
		}
	}
	if (!found) {
		for (type = TrexObstacleSmall; type < TrexObstacleTypeCount; type++) {
			config = &mObstacleConfigs[type];
			if (!trexDuplicateObstacle(game, type) && game->currentSpeed >= config->minSpeed) {
				found = true;
				break;
			}
		}
	}
	if (!found)
		return;
	config = &mObstacleConfigs[type];
	obstacle = &game->obstacles[game->obstacleCount++];
	memset(obstacle, 0, sizeof(*obstacle));
	obstacle->type = type;
	obstacle->size = (int8_t)trexRandomRange(game, 1, 3);
	if (obstacle->size > 1 && config->multipleSpeed > game->currentSpeed)
		obstacle->size = 1;
	obstacle->width = (int16_t)(config->width * obstacle->size);
	obstacle->x = (int16_t)(TREX_CANVAS_W + config->width);
	if (type == TrexObstaclePterodactyl) {
		static const int16_t yPositions[] = {100, 75, 50};

		obstacle->y = yPositions[trexRandomRange(game, 0, 2)];
	}
	else {
		obstacle->y = config->yPos;
	}
	memcpy(obstacle->collision, config->collision, sizeof(obstacle->collision));
	if (obstacle->size > 1) {
		obstacle->collision[1].w = (int16_t)(obstacle->width -
			obstacle->collision[0].w - obstacle->collision[2].w);
		obstacle->collision[2].x = (int16_t)(obstacle->width - obstacle->collision[2].w);
	}
	if (config->speedOffset)
		obstacle->speedOffset = trexRandomUnit(game) > 0.5f ? config->speedOffset : -config->speedOffset;
	{
		int32_t minGap = trexJsRound(obstacle->width * game->currentSpeed +
			config->minGap * TREX_RUNNER_GAP_COEFFICIENT);
		int32_t maxGap = trexJsRound(minGap * 1.5f);

		obstacle->gap = (int16_t)trexRandomRange(game, minGap, maxGap);
	}

	if (game->obstacleHistoryCount == 2u)
		game->obstacleHistory[1] = game->obstacleHistory[0];
	else if (game->obstacleHistoryCount == 1u) {
		game->obstacleHistory[1] = game->obstacleHistory[0];
		game->obstacleHistoryCount = 2u;
	}
	else {
		game->obstacleHistoryCount = 1u;
	}
	game->obstacleHistory[0] = type;
}

static void trexUpdateObstacles(struct TrexGame *game, float delta)
{
	uint8_t out = 0;

	for (uint8_t i = 0; i < game->obstacleCount; i++) {
		struct TrexObstacle obstacle = game->obstacles[i];
		const struct TrexObstacleConfig *config = &mObstacleConfigs[obstacle.type];
		float speed = game->currentSpeed + obstacle.speedOffset;

		obstacle.x -= (int16_t)floorf(speed * (TREX_FPS / 1000.0f) * delta);
		if (config->frameCount) {
			obstacle.timer += delta;
			if (obstacle.timer >= config->frameRate) {
				obstacle.currentFrame = obstacle.currentFrame == (int8_t)(config->frameCount - 1u) ?
					0 : obstacle.currentFrame + 1;
				obstacle.timer = 0.0f;
			}
		}
		if (obstacle.x + obstacle.width > 0)
			game->obstacles[out++] = obstacle;
	}
	game->obstacleCount = out;
	if (game->obstacleCount) {
		struct TrexObstacle *last = &game->obstacles[game->obstacleCount - 1u];

		if (!last->followingCreated && last->x + last->width > 0 &&
				last->x + last->width + last->gap < TREX_CANVAS_W) {
			last->followingCreated = true;
			trexAddObstacle(game);
		}
	}
	else {
		trexAddObstacle(game);
	}
}

static void trexUpdateHorizonLine(struct TrexGame *game, float delta, float speed)
{
	int32_t increment = (int32_t)floorf(speed * (TREX_FPS / 1000.0f) * delta);
	uint32_t first = game->horizonX[0] <= 0 ? 0u : 1u;
	uint32_t second = first ? 0u : 1u;

	game->horizonX[first] -= (int16_t)increment;
	game->horizonX[second] = (int16_t)(game->horizonX[first] + 600);
	if (game->horizonX[first] <= -600) {
		game->horizonX[first] += 1200;
		game->horizonX[second] = (int16_t)(game->horizonX[first] - 600);
		game->horizonSourceX[first] = (int16_t)(TrexSpriteHorizonX +
			(trexRandomUnit(game) > 0.5f ? 600 : 0));
	}
}

static void trexUpdateHorizon(struct TrexGame *game, float delta, float speed,
	bool updateObstacles, bool showNight)
{
	trexUpdateHorizonLine(game, delta, speed);
	trexUpdateNight(game, showNight);
	trexUpdateClouds(game, delta, speed);
	if (updateObstacles)
		trexUpdateObstacles(game, delta);
}

static bool trexBoxesOverlap(const struct TrexCollisionBox *a, const struct TrexCollisionBox *b)
{
	return a->x < b->x + b->w && a->x + a->w > b->x &&
		a->y < b->y + b->h && a->y + a->h > b->y;
}

static bool trexCollides(const struct TrexGame *game, const struct TrexObstacle *obstacle)
{
	const struct TrexPlayer *player = &game->player;
	const struct TrexObstacleConfig *config = &mObstacleConfigs[obstacle->type];
	struct TrexCollisionBox playerOuter = {
		(int16_t)(player->x + 1), (int16_t)(player->y + 1),
		TREX_PLAYER_WIDTH - 2, TREX_PLAYER_HEIGHT - 2,
	};
	struct TrexCollisionBox obstacleOuter = {
		(int16_t)(obstacle->x + 1), (int16_t)(obstacle->y + 1),
		(int16_t)(config->width * obstacle->size - 2), (int16_t)(config->height - 2),
	};
	const struct TrexCollisionBox *playerBoxes = player->ducking ?
		mPlayerDuckingBoxes : mPlayerRunningBoxes;
	uint32_t playerBoxCount = player->ducking ? 1u :
		sizeof(mPlayerRunningBoxes) / sizeof(mPlayerRunningBoxes[0]);

	if (!trexBoxesOverlap(&playerOuter, &obstacleOuter))
		return false;
	for (uint32_t p = 0; p < playerBoxCount; p++)
		for (uint32_t o = 0; o < config->collisionCount; o++) {
			struct TrexCollisionBox a = {
				(int16_t)(playerOuter.x + playerBoxes[p].x),
				(int16_t)(playerOuter.y + playerBoxes[p].y),
				playerBoxes[p].w, playerBoxes[p].h,
			};
			struct TrexCollisionBox b = {
				(int16_t)(obstacleOuter.x + obstacle->collision[o].x),
				(int16_t)(obstacleOuter.y + obstacle->collision[o].y),
				obstacle->collision[o].w, obstacle->collision[o].h,
			};

			if (trexBoxesOverlap(&a, &b))
				return true;
		}
	return false;
}

static uint32_t trexActualDistance(uint32_t rawDistance)
{
	return rawDistance ? (uint32_t)trexJsRound(rawDistance * TREX_DISTANCE_COEFFICIENT) : 0u;
}

static void trexUpdateScore(struct TrexGame *game, float delta)
{
	uint32_t actual;

	game->scorePaint = true;
	if (!game->scoreAchievement) {
		actual = trexActualDistance((uint32_t)ceilf(game->distanceRan));
		if (actual > 99999u && game->scoreUnits == 5u)
			game->scoreUnits = 6u;
		if (actual > 0u && actual % TREX_ACHIEVEMENT_DISTANCE == 0u) {
			game->scoreAchievement = true;
			game->scoreFlashTimer = 0.0f;
		}
	}
	else if (game->scoreFlashIterations <= TREX_FLASH_ITERATIONS) {
		game->scoreFlashTimer += delta;
		if (game->scoreFlashTimer < TREX_FLASH_DURATION)
			game->scorePaint = false;
		else if (game->scoreFlashTimer > TREX_FLASH_DURATION * 2.0f) {
			game->scoreFlashTimer = 0.0f;
			game->scoreFlashIterations++;
		}
	}
	else {
		game->scoreAchievement = false;
		game->scoreFlashIterations = 0;
		game->scoreFlashTimer = 0.0f;
	}
}

static uint32_t trexSaveCheck(const struct TrexSave *save)
{
	return save->magic ^ save->version ^ save->size ^ save->highestDistance ^ TREX_SAVE_CHECK_XOR;
}

static void trexLoadHighScore(struct TrexGame *game)
{
	struct TrexSave save;

	memset(&save, 0, sizeof(save));
	if (game->args->vol && dc32PortSaveRead(game->args->vol, "trex", &save, sizeof(save)) &&
			save.magic == TREX_SAVE_MAGIC && save.version == TREX_SAVE_VERSION &&
			save.size == sizeof(save) && save.check == trexSaveCheck(&save)) {
		game->highestDistance = save.highestDistance;
		if (trexActualDistance(game->highestDistance) > 99999u)
			game->scoreUnits = 6u;
	}
}

static bool trexWriteHighScore(struct TrexGame *game)
{
	struct TrexSave save = {
		.magic = TREX_SAVE_MAGIC,
		.version = TREX_SAVE_VERSION,
		.size = sizeof(struct TrexSave),
		.highestDistance = game->highestDistance,
	};

	save.check = trexSaveCheck(&save);
	if (!game->args->vol || !dc32PortSaveWrite(game->args->vol, "trex", &save, sizeof(save)))
		return false;
	game->saveDirty = false;
	return true;
}

static void trexGameOver(struct TrexGame *game)
{
	game->playing = false;
	game->paused = true;
	game->crashed = true;
	game->scoreAchievement = false;
	game->crashElapsed = 0.0f;
	game->player.timer += 100.0f;
	trexSetPlayerStatus(game, TrexStatusCrashed);
	if (game->player.ducking)
		game->player.x++;
	if (game->distanceRan > game->highestDistance) {
		game->highestDistance = (uint32_t)ceilf(game->distanceRan);
		game->saveDirty = true;
		game->saveAfterPresent = true;
	}
}

static void trexResetHorizon(struct TrexGame *game)
{
	game->obstacleCount = 0;
	game->horizonX[0] = 0;
	game->horizonX[1] = 600;
	trexResetNight(game);
}

static void trexRestart(struct TrexGame *game)
{
	game->runningTime = 0.0f;
	game->playing = true;
	game->paused = false;
	game->crashed = false;
	game->distanceRan = 0.0f;
	/* The badge uses a fixed canvas, not Chrome's responsive mobile layout. */
	game->currentSpeed = TREX_RUNNER_SPEED;
	game->crashElapsed = 0.0f;
	game->scoreAchievement = false;
	game->scoreFlashIterations = 0;
	game->scoreFlashTimer = 0.0f;
	game->scorePaint = true;
	trexResetHorizon(game);
	trexResetPlayer(game);
	game->invertTimer = 0.0f;
	game->invertTrigger = false;
	game->inverted = false;
}

static float trexEaseOut(float progress)
{
	float parameter = progress;

	if (progress <= 0.0f)
		return 0.0f;
	if (progress >= 1.0f)
		return 1.0f;
	for (uint32_t i = 0; i < 5u; i++) {
		float oneMinus = 1.0f - parameter;
		float x = 3.0f * oneMinus * parameter * parameter * 0.58f +
			parameter * parameter * parameter;
		float derivative = 6.0f * oneMinus * parameter * 0.58f +
			3.0f * parameter * parameter;

		if (derivative > 0.0001f)
			parameter -= (x - progress) / derivative;
		if (parameter < 0.0f)
			parameter = 0.0f;
		if (parameter > 1.0f)
			parameter = 1.0f;
	}
	{
		float oneMinus = 1.0f - parameter;
		return 3.0f * oneMinus * parameter * parameter +
			parameter * parameter * parameter;
	}
}

static void trexBeginIntro(struct TrexGame *game)
{
	if (game->activated || game->crashed)
		return;
	game->playingIntro = true;
	game->player.playingIntro = true;
	game->introElapsed = 0.0f;
	game->playing = true;
	game->activated = true;
}

static void trexFinishIntro(struct TrexGame *game)
{
	game->runningTime = 0.0f;
	game->playingIntro = false;
	game->player.playingIntro = false;
	game->arcade = true;
	game->clipWidth = TREX_CANVAS_W;
	game->canvasY = TREX_ARCADE_CANVAS_Y;
}

static void trexUpdateIntro(struct TrexGame *game, float delta)
{
	float progress;

	if (!game->playingIntro)
		return;
	game->introElapsed += delta;
	progress = game->introElapsed / TREX_INTRO_DURATION;
	game->clipWidth = (int16_t)trexJsRound(TREX_PLAYER_WIDTH +
		(TREX_CANVAS_W - TREX_PLAYER_WIDTH) * trexEaseOut(progress));
	if (game->introElapsed >= TREX_INTRO_DURATION)
		trexFinishIntro(game);
}

static void trexUpdate(struct TrexGame *game)
{
	const float delta = TREX_FRAME_MS;

	if (game->crashed)
		game->crashElapsed += delta;
	if (game->playing) {
		bool hasObstacles;
		bool collision = false;

		if (game->player.jumping)
			trexUpdateJump(game, delta);
		game->runningTime += delta;
		hasObstacles = game->runningTime > TREX_RUNNER_CLEAR_TIME;

		if (game->player.jumpCount == 1u && !game->playingIntro)
			trexBeginIntro(game);
		if (game->playingIntro)
			trexUpdateHorizon(game, 0.0f, game->currentSpeed, hasObstacles, game->inverted);
		else
			trexUpdateHorizon(game, game->activated ? delta : 0.0f,
				game->currentSpeed, hasObstacles, game->inverted);

		if (hasObstacles && game->obstacleCount)
			collision = trexCollides(game, &game->obstacles[0]);
		if (!collision) {
			game->distanceRan += game->currentSpeed * delta / TREX_FRAME_MS;
			if (game->currentSpeed < TREX_RUNNER_MAX_SPEED)
				game->currentSpeed += TREX_RUNNER_ACCELERATION;
		}
		else {
			trexGameOver(game);
		}
		trexUpdateScore(game, delta);

		if (game->invertTimer > TREX_RUNNER_INVERT_FADE_DURATION) {
			game->invertTimer = 0.0f;
			game->invertTrigger = false;
			game->inverted = game->invertTrigger;
		}
		else if (game->invertTimer) {
			game->invertTimer += delta;
		}
		else {
			uint32_t actual = trexActualDistance((uint32_t)ceilf(game->distanceRan));

			if (actual > 0u) {
				game->invertTrigger = actual % TREX_RUNNER_INVERT_DISTANCE == 0u;
				if (game->invertTrigger && game->invertTimer == 0.0f) {
					game->invertTimer += delta;
					game->inverted = game->invertTrigger;
				}
			}
		}
	}

	if (game->playing || (!game->activated &&
			game->player.blinkCount < TREX_RUNNER_MAX_BLINK_COUNT))
		trexUpdatePlayerAnimation(game, delta);
	trexUpdateIntro(game, delta);
}

static void trexHandleInput(struct TrexGame *game)
{
	uint_fast16_t keys = game->draw.keys;
	uint_fast16_t pressed = keys & ~game->previousKeys;
	uint_fast16_t released = game->previousKeys & ~keys;
	uint_fast16_t jumpMask = KEY_BIT_A | KEY_BIT_UP;

	game->previousKeys = keys;
	if (game->crashed && (pressed & KEY_BIT_START)) {
		trexRestart(game);
		return;
	}
	if (!game->crashed && (pressed & jumpMask)) {
		if (!game->playing) {
			game->playing = true;
			game->paused = false;
		}
		if (!game->player.jumping && !game->player.ducking)
			trexStartJump(game);
	}
	if (game->playing && !game->crashed && (pressed & KEY_BIT_DOWN)) {
		if (game->player.jumping)
			trexSetSpeedDrop(game);
		else if (!game->player.ducking)
			trexSetDuck(game, true);
	}

	if (game->playing && !game->crashed && (released & jumpMask))
		trexEndJump(game);
	else if (released & KEY_BIT_DOWN) {
		game->player.speedDrop = false;
		trexSetDuck(game, false);
	}
	else if (game->crashed && (released & jumpMask) &&
			game->crashElapsed >= TREX_RUNNER_GAMEOVER_CLEAR_TIME)
		trexRestart(game);
}

static void trexDrawHorizon(struct TrexGame *game)
{
	for (uint32_t i = 0; i < 2u; i++)
		trexBlit(game, game->horizonSourceX[i], TrexSpriteHorizonY,
			600, 12, game->horizonX[i], 127, 255u);
}

static void trexDrawNight(struct TrexGame *game)
{
	uint8_t alpha;
	int32_t moonWidth;
	int32_t moonSourceX;

	if (game->night.opacity <= 0.0f)
		return;
	alpha = (uint8_t)(game->night.opacity >= 1.0f ? 255 :
		trexJsRound(game->night.opacity * 255.0f));
	if (game->night.drawStars)
		for (uint32_t i = 0; i < 2u; i++)
			trexBlit(game, TrexSpriteStarX, game->night.stars[i].sourceY,
				9, 9, trexJsRound(game->night.stars[i].x), game->night.stars[i].y, alpha);
	moonWidth = game->night.phase == 3u ? 40 : 20;
	moonSourceX = TrexSpriteMoonX + mNightPhases[game->night.phase];
	trexBlit(game, moonSourceX, TrexSpriteMoonY, moonWidth, 40,
		trexJsRound(game->night.x), 30, alpha);
}

static void trexDrawClouds(struct TrexGame *game)
{
	for (uint8_t i = 0; i < game->cloudCount; i++)
		trexBlit(game, TrexSpriteCloudX, TrexSpriteCloudY, 46, 14,
			(int32_t)ceilf(game->clouds[i].x), game->clouds[i].y, 255u);
}

static void trexDrawObstacles(struct TrexGame *game)
{
	for (uint8_t i = 0; i < game->obstacleCount; i++) {
		const struct TrexObstacle *obstacle = &game->obstacles[i];
		const struct TrexObstacleConfig *config = &mObstacleConfigs[obstacle->type];
		int32_t sourceX = config->spriteX +
			(config->width * obstacle->size) * (obstacle->size - 1) / 2;

		if (obstacle->currentFrame)
			sourceX += config->width * obstacle->currentFrame;
		trexBlit(game, sourceX, config->spriteY,
			config->width * obstacle->size, config->height,
			obstacle->x, obstacle->y, 255u);
	}
}

static void trexDrawPlayer(struct TrexGame *game)
{
	const struct TrexPlayer *player = &game->player;
	int32_t width = player->ducking && player->status != TrexStatusCrashed ?
		TREX_PLAYER_DUCK_WIDTH : TREX_PLAYER_WIDTH;

	trexBlit(game, TrexSpritePlayerX + trexPlayerFrameOffset(player),
		TrexSpritePlayerY, width, TREX_PLAYER_HEIGHT,
		player->x, player->y, 255u);
}

static void trexDrawDigit(struct TrexGame *game, uint32_t digit, int32_t x, uint8_t alpha)
{
	if (digit <= 11u)
		trexBlit(game, TrexSpriteTextX + (int32_t)digit * 10, TrexSpriteTextY,
			10, 13, x, 10, alpha);
}

static void trexDrawNumber(struct TrexGame *game, uint32_t value, uint8_t units,
	int32_t x, uint8_t alpha)
{
	uint32_t divisor = 1u;

	for (uint8_t i = 1; i < units; i++)
		divisor *= 10u;
	for (uint8_t i = 0; i < units; i++) {
		trexDrawDigit(game, value / divisor % 10u, x + i * 11, alpha);
		if (divisor > 1u)
			divisor /= 10u;
	}
}

static void trexDrawScore(struct TrexGame *game)
{
	int32_t scoreX = TREX_CANVAS_W - 11 * (game->scoreUnits + 1);
	uint32_t actual = trexActualDistance((uint32_t)ceilf(game->distanceRan));
	uint32_t modulus = game->scoreUnits == 6u ? 1000000u : 100000u;

	if (game->scorePaint)
		trexDrawNumber(game, actual % modulus, game->scoreUnits, scoreX, 255u);
	if (game->highestDistance) {
		int32_t highX = scoreX - game->scoreUnits * 20;
		uint32_t high = trexActualDistance(game->highestDistance);

		trexDrawDigit(game, 10u, highX, 204u);
		trexDrawDigit(game, 11u, highX + 11, 204u);
		trexDrawNumber(game, high % modulus, game->scoreUnits, highX + 33,
			204u);
	}
}

static void trexDrawGameOver(struct TrexGame *game)
{
	trexBlit(game, TrexSpriteTextX, TrexSpriteTextY + 13,
		191, 11, 65, 42, 255u);
	trexBlit(game, TrexSpriteRestartX, TrexSpriteRestartY,
		36, 32, 142, 75, 255u);
}

static void trexRender(struct TrexGame *game)
{
	uint8_t background = trexGray332(trexDisplayGray(game, 247u));

	memset(game->frame, background, TREX_SCREEN_W * TREX_SCREEN_H);
	trexDrawHorizon(game);
	trexDrawNight(game);
	trexDrawClouds(game);
	trexDrawObstacles(game);
	trexDrawPlayer(game);
	if (game->crashed)
		trexDrawGameOver(game);
	trexDrawScore(game);
	if (!game->arcade)
		trexDrawPrompt(game);
}

static bool trexInit(struct TrexGame *game, const struct DcAppHostApi *host,
	const struct DcAppRunArgs *args)
{
	uint64_t now;

	if (!game || !host || !args || !args->canvas)
		return false;
	memset(game, 0, sizeof(*game));
	game->host = host;
	game->args = args;
	game->frame = dc32PortMalloc(TREX_SCREEN_W * TREX_SCREEN_H);
	if (!game->frame)
		return false;
	if (!dcAppDrawInit(&game->draw, host, args, game->frame, TREX_SCREEN_W, TREX_SCREEN_H))
		return false;
	now = host->getTime ? host->getTime() : 0x54524558u;
	game->rng = (uint32_t)(now ^ (now >> 32));
	game->currentSpeed = TREX_RUNNER_SPEED;
	game->canvasY = TREX_WAIT_CANVAS_Y;
	game->clipWidth = TREX_PLAYER_WIDTH;
	game->horizonX[0] = 0;
	game->horizonX[1] = 600;
	game->horizonSourceX[0] = TrexSpriteHorizonX;
	game->horizonSourceX[1] = TrexSpriteHorizonX + 600;
	game->night.x = TREX_CANVAS_W - 50.0f;
	game->scoreUnits = 5u;
	game->scorePaint = true;
	trexPlaceStars(game);
	trexAddCloud(game);
	trexInitPlayer(game);
	trexLoadHighScore(game);
	return true;
}

int trexAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct TrexGame *game;

	mTrexAbort = false;
	dc32PortHeapInitDefault();
	game = dc32PortCalloc(1, sizeof(*game));
	if (!game)
		return -1;
	if (!trexInit(game, host, args)) {
		dc32PortFree(game->frame);
		dc32PortFree(game);
		return -1;
	}
	dispSetFramerate(TREX_FPS);
	dcAppDrawWaitRelease(&game->draw, KEY_BIT_A | KEY_BIT_B | KEY_BIT_UP |
		KEY_BIT_DOWN | KEY_BIT_START | UI_KEY_BIT_CENTER);
	game->previousKeys = 0;
	trexRender(game);
	while (!mTrexAbort) {
		bool running = dcAppDrawFrame(&game->draw, UI_KEY_BIT_CENTER);

		if (game->saveAfterPresent) {
			game->saveAfterPresent = false;
			(void)trexWriteHighScore(game);
		}
		if (!running)
			break;
		trexHandleInput(game);
		trexUpdate(game);
		trexRender(game);
	}
	if (game->saveDirty)
		(void)trexWriteHighScore(game);
	dcAppDrawWaitRelease(&game->draw, UI_KEY_BIT_CENTER);
	dc32PortFree(game->frame);
	dc32PortFree(game);
	dispSetFramerate(60);
	return 0;
}

void trexAppAbort(void)
{
	mTrexAbort = true;
}
