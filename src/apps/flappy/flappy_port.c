/*
 * Native DC32 adaptation of VadimBoev/FlappyBird v1.9.2 commit
 * 3b3060cdf6b38b819e5a649bc92d11776decd0b4.
 */
#include "apps/flappy/flappy_port.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "apps/port/port_runtime.h"
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "flappy_assets.h"
#include "gb.h"

#define FLAPPY_SCREEN_W 320
#define FLAPPY_SCREEN_H 240
#define FLAPPY_VIEW_W 320
#define FLAPPY_VIEW_H 240
#define FLAPPY_VIEW_X 0
#define FLAPPY_VIRTUAL_W 320.0f
#define FLAPPY_VIRTUAL_H 512.0f
#define FLAPPY_FPS 60u
#define FLAPPY_SPEED_MULTIPLIER 2u
#define FLAPPY_MAX_DT_FRAMES 4.0f
#define FLAPPY_ANIM_INTERVAL_FRAMES 7.0f
#define FLAPPY_BIRD_W 34.0f
#define FLAPPY_BIRD_H 51.2f
#define FLAPPY_TITLE_LOGO_X 48.0f
#define FLAPPY_TITLE_BIRD_X 240.0f
#define FLAPPY_FLOOR_Y_PERCENT 86.0f
#define FLAPPY_PIPE_GAP_CENTER_PERCENT 43.0f
#define FLAPPY_READY_TAP_X 75.0f
#define FLAPPY_READY_TAP_Y_PERCENT 11.0f
#define FLAPPY_GAME_OVER_Y_PERCENT 6.0f
#define FLAPPY_PANEL_SCORE_Y_PERCENT 9.5f
#define FLAPPY_PANEL_BEST_Y_PERCENT 18.0f
#define FLAPPY_BUTTON_LEFT_X 52.0f
#define FLAPPY_BUTTON_RIGHT_X 188.0f

#define SIZE_SPACE_PIPE 3.3f
#define SPACE_BETWEEN_PIPES 5
#define FLAPPY_GRAVITY 0.65f
#define FLAPPY_JUMP_VELOCITY (-13.5f)
#define FLAPPY_SAVE_MAGIC 0x31504c46u
#define FLAPPY_SAVE_VERSION 1u
#define FLAPPY_SAVE_CHECK_XOR 0x8f31b562u

enum FlappyGameState {
	FlappyStateIdle,
	FlappyStateFadeIn,
	FlappyStateFadeOut,
	FlappyStateReadyGame,
	FlappyStateGoGame,
	FlappyStateStopGame,
	FlappyStateFadeOutGameover,
	FlappyStateFallBird,
	FlappyStateFadeInPanel,
};

struct FlappyBird {
	float x;
	float y;
	float velocity;
	float angle;
	float width;
	float height;
	enum FlappyAssetId currentTexture;
	uint32_t frame;
	float frameTimer;
};

struct FlappyPipe {
	float x;
	float y;
	float w;
	float h;
	float offset;
};

struct FlappySave {
	uint32_t magic;
	uint16_t version;
	uint16_t size;
	uint32_t bestScore;
	uint32_t check;
};

struct FlappyGame {
	const struct DcAppHostApi *host;
	const struct DcAppRunArgs *args;
	struct DcAppDrawCtx draw;
	uint8_t *frame;
	uint32_t rng;
	uint32_t score;
	uint32_t bestScore;
	uint64_t lastTicks;
	float offsetBase;
	uint32_t gameSpeed;
	float alpha;
	float fadeOutAlpha;
	float panelY;
	bool newBestScore;
	bool saveDirty;
	bool saveAfterPresent;
	enum FlappyGameState currentState;
	struct FlappyBird bird;
	struct FlappyPipe pipes[2];
	float logoY;
	float birdY;
	float logoVelocity;
	float birdVelocity;
	float logoFrameTimer;
	enum FlappyAssetId curTextureAnimBirdForLogo;
	uint32_t currentFrameForLogo;
};

static bool mFlappyAbort;

static float flappyScaleX(float percent)
{
	return (percent / 100.0f) * FLAPPY_VIRTUAL_W;
}

static float flappyScaleY(float percent)
{
	return (percent / 100.0f) * FLAPPY_VIRTUAL_H;
}

static float flappyFloorY(void)
{
	return flappyScaleY(FLAPPY_FLOOR_Y_PERCENT);
}

static float flappyPipeGap(void)
{
	return FLAPPY_BIRD_H * SIZE_SPACE_PIPE;
}

static float flappyPipeBaseY(void)
{
	return flappyScaleY(FLAPPY_PIPE_GAP_CENTER_PERCENT) + flappyPipeGap() * 0.5f;
}

static int32_t flappyRound(float value)
{
	return (int32_t)(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

static int32_t flappyScreenX(float value)
{
	return FLAPPY_VIEW_X + flappyRound(value * (float)FLAPPY_VIEW_W / FLAPPY_VIRTUAL_W);
}

static int32_t flappyScreenY(float value)
{
	return flappyRound(value * (float)FLAPPY_VIEW_H / FLAPPY_VIRTUAL_H);
}

static int32_t flappyScreenH(float value)
{
	int32_t height = flappyRound(value * (float)FLAPPY_VIEW_H / FLAPPY_VIRTUAL_H);

	return height > 0 ? height : 1;
}

static uint8_t flappyRgb332(uint32_t r, uint32_t g, uint32_t b)
{
	return (uint8_t)((r & 0xe0u) | ((g >> 3) & 0x1cu) | (b >> 6));
}

static uint16_t flappyRgb565(uint32_t r, uint32_t g, uint32_t b)
{
	return dcAppDrawRgb565(r, g, b);
}

static uint32_t flappyRand(struct FlappyGame *game)
{
	game->rng = game->rng * 1664525u + 1013904223u;
	return game->rng;
}

static int32_t flappyRandom(struct FlappyGame *game, int32_t min, int32_t max)
{
	uint32_t span;

	if (max <= min)
		return min;
	span = (uint32_t)(max - min + 1);
	return min + (int32_t)(flappyRand(game) % span);
}

static uint64_t flappyNowTicks(struct FlappyGame *game)
{
	if (game->host && game->host->getTime)
		return game->host->getTime();
	return game->lastTicks + TICKS_PER_SECOND / FLAPPY_FPS;
}

static float flappyDeltaFrames(struct FlappyGame *game)
{
	uint64_t now = flappyNowTicks(game);
	uint64_t elapsed = now >= game->lastTicks ? now - game->lastTicks : TICKS_PER_SECOND / FLAPPY_FPS;
	float dt;

	game->lastTicks = now;
	if (!elapsed)
		return 0.0f;
	dt = (float)elapsed * (float)FLAPPY_FPS / (float)TICKS_PER_SECOND;
	return dt > FLAPPY_MAX_DT_FRAMES ? FLAPPY_MAX_DT_FRAMES : dt;
}

static float flappyMoveTowards(float current, float target, float maxDelta)
{
	if (fabsf(target - current) <= maxDelta)
		return target;
	return current + (target > current ? maxDelta : -maxDelta);
}

static uint32_t flappySaveCheck(const struct FlappySave *save)
{
	return save->magic ^ save->version ^ save->size ^ save->bestScore ^ FLAPPY_SAVE_CHECK_XOR;
}

static void flappyLoadBestScore(struct FlappyGame *game)
{
	struct FlappySave save;

	if (game->args->vol && dc32PortSaveRead(game->args->vol, "flappy", &save, sizeof(save)) &&
			save.magic == FLAPPY_SAVE_MAGIC && save.version == FLAPPY_SAVE_VERSION &&
			save.size == sizeof(save) && save.check == flappySaveCheck(&save))
		game->bestScore = save.bestScore;
}

static bool flappyWriteBestScore(struct FlappyGame *game)
{
	struct FlappySave save = {
		.magic = FLAPPY_SAVE_MAGIC,
		.version = FLAPPY_SAVE_VERSION,
		.size = sizeof(struct FlappySave),
		.bestScore = game->bestScore,
	};

	save.check = flappySaveCheck(&save);
	if (game->args->vol && dc32PortSaveWrite(game->args->vol, "flappy", &save, sizeof(save))) {
		game->saveDirty = false;
		return true;
	}
	return false;
}

static void flappyResetPipes(struct FlappyGame *game)
{
	game->pipes[0].x = flappyScaleX(100.0f);
	game->pipes[0].y = flappyPipeBaseY();
	game->pipes[0].w = flappyScaleX(15.0f);
	game->pipes[0].h = flappyFloorY() - game->pipes[0].y;
	game->pipes[0].offset = (float)flappyRandom(game,
		(int32_t)flappyScaleY(-SPACE_BETWEEN_PIPES), (int32_t)flappyScaleY(SPACE_BETWEEN_PIPES));
	game->pipes[1].x = flappyScaleX(100.0f) + flappyScaleX(60.0f);
	game->pipes[1].y = flappyPipeBaseY();
	game->pipes[1].w = flappyScaleX(15.0f);
	game->pipes[1].h = flappyFloorY() - game->pipes[1].y;
	game->pipes[1].offset = (float)flappyRandom(game,
		(int32_t)flappyScaleY(-SPACE_BETWEEN_PIPES), (int32_t)flappyScaleY(SPACE_BETWEEN_PIPES));
}

static void flappyResetBird(struct FlappyGame *game)
{
	game->bird.x = flappyScaleX(18.52f);
	game->bird.y = flappyScaleY(20.0f);
	game->bird.velocity = 0.0f;
	game->bird.angle = 0.0f;
	game->bird.width = FLAPPY_BIRD_W;
	game->bird.height = FLAPPY_BIRD_H;
	game->bird.currentTexture = FlappyAssetYellowBirdMid;
	game->bird.frame = 0;
	game->bird.frameTimer = 0.0f;
}

static void flappyResetRound(struct FlappyGame *game)
{
	game->score = 0;
	game->panelY = flappyScaleY(100.0f);
	game->fadeOutAlpha = 255.0f;
	game->newBestScore = false;
	flappyResetBird(game);
	flappyResetPipes(game);
}

static bool flappyAssetIsOpaque(const struct FlappyAsset *asset)
{
	return (asset->flags & FLAPPY_ASSET_FLAG_OPAQUE) != 0u;
}

static bool flappyAssetPixelOpaque(const struct FlappyAsset *asset, uint32_t x, uint32_t y)
{
	const struct FlappyAssetRow *row;

	if (flappyAssetIsOpaque(asset))
		return true;
	row = &flappyAssetRows[asset->rowOffset + y];
	for (uint32_t i = 0; i < row->runCount; i++) {
		const struct FlappyAssetRun *run = &flappyAssetRuns[row->runOffset + i];

		if (x >= run->x && x < (uint32_t)run->x + run->len)
			return true;
	}
	return false;
}

static void flappyCopyRun(struct FlappyGame *game, const uint8_t *src, int32_t dstX, int32_t dstY,
	uint32_t len)
{
	if (dstY < 0 || dstY >= FLAPPY_SCREEN_H || dstX >= FLAPPY_SCREEN_W || dstX + (int32_t)len <= 0)
		return;
	if (dstX < 0) {
		uint32_t skip = (uint32_t)-dstX;

		src += skip;
		len -= skip;
		dstX = 0;
	}
	if (dstX + (int32_t)len > FLAPPY_SCREEN_W)
		len = (uint32_t)(FLAPPY_SCREEN_W - dstX);
	if (len)
		memcpy(game->draw.fb + (uint32_t)dstY * FLAPPY_SCREEN_W + (uint32_t)dstX, src, len);
}

static void flappyDrawOpaqueRows(struct FlappyGame *game, const struct FlappyAsset *asset,
	const uint8_t *pixels, int32_t x, int32_t y)
{
	int32_t startRow = 0;
	int32_t endRow = (int32_t)asset->height;
	int32_t startCol = 0;
	int32_t copyWidth = (int32_t)asset->width;

	if (y < 0)
		startRow = -y;
	if (y + endRow > FLAPPY_SCREEN_H)
		endRow = FLAPPY_SCREEN_H - y;
	if (x < 0) {
		startCol = -x;
		copyWidth -= startCol;
		x = 0;
	}
	if (x + copyWidth > FLAPPY_SCREEN_W)
		copyWidth = FLAPPY_SCREEN_W - x;
	if (startRow >= endRow || copyWidth <= 0)
		return;
	for (int32_t row = startRow; row < endRow; row++) {
		const uint8_t *src = pixels + (uint32_t)row * asset->width + (uint32_t)startCol;
		uint8_t *dst = game->draw.fb + (uint32_t)(y + row) * FLAPPY_SCREEN_W + (uint32_t)x;

		memcpy(dst, src, (uint32_t)copyWidth);
	}
}

static void flappyDrawRunRow(struct FlappyGame *game, const struct FlappyAsset *asset,
	const uint8_t *pixels, uint32_t srcRow, int32_t x, int32_t y)
{
	const struct FlappyAssetRow *row;

	if (y < 0 || y >= FLAPPY_SCREEN_H)
		return;
	row = &flappyAssetRows[asset->rowOffset + srcRow];
	for (uint32_t i = 0; i < row->runCount; i++) {
		const struct FlappyAssetRun *run = &flappyAssetRuns[row->runOffset + i];
		const uint8_t *src = pixels + srcRow * asset->width + run->x;

		flappyCopyRun(game, src, x + run->x, y, run->len);
	}
}

static void flappyDrawRunRows(struct FlappyGame *game, const struct FlappyAsset *asset,
	const uint8_t *pixels, int32_t x, int32_t y)
{
	int32_t startRow = y < 0 ? -y : 0;
	int32_t endRow = y + (int32_t)asset->height > FLAPPY_SCREEN_H ?
		FLAPPY_SCREEN_H - y : (int32_t)asset->height;

	if (startRow >= endRow)
		return;
	for (int32_t row = startRow; row < endRow; row++)
		flappyDrawRunRow(game, asset, pixels, (uint32_t)row, x, y + row);
}

static void flappyDrawAssetScreen(struct FlappyGame *game, enum FlappyAssetId id, int32_t x, int32_t y)
{
	const struct FlappyAsset *asset = &flappyAssets[id];
	const uint8_t *pixels = flappyAssetPixels + asset->pixelOffset;

	if (flappyAssetIsOpaque(asset))
		flappyDrawOpaqueRows(game, asset, pixels, x, y);
	else
		flappyDrawRunRows(game, asset, pixels, x, y);
}

static void flappyDrawAsset(struct FlappyGame *game, enum FlappyAssetId id, float x, float y)
{
	flappyDrawAssetScreen(game, id, flappyScreenX(x), flappyScreenY(y));
}

static void flappyDrawRotatedAsset(struct FlappyGame *game, enum FlappyAssetId id, float x, float y, float angle)
{
	const struct FlappyAsset *asset = &flappyAssets[id];
	const uint8_t *pixels = flappyAssetPixels + asset->pixelOffset;
	int32_t dstX = flappyScreenX(x);
	int32_t dstY = flappyScreenY(y);
	float centerX = ((float)asset->width - 1.0f) * 0.5f;
	float centerY = ((float)asset->height - 1.0f) * 0.5f;
	float rad = angle * 3.1415926535f / 180.0f;
	float ca = cosf(rad);
	float sa = sinf(rad);

	for (uint32_t row = 0; row < asset->height; row++) {
		int32_t screenY = dstY + (int32_t)row;

		if (screenY < 0 || screenY >= FLAPPY_SCREEN_H)
			continue;
		for (uint32_t col = 0; col < asset->width; col++) {
			float relX = (float)col - centerX;
			float relY = (float)row - centerY;
			int32_t srcX = flappyRound(relX * ca + relY * sa + centerX);
			int32_t srcY = flappyRound(-relX * sa + relY * ca + centerY);
			int32_t screenX = dstX + (int32_t)col;
			uint32_t pos;

			if (screenX < 0 || screenX >= FLAPPY_SCREEN_W ||
					srcX < 0 || srcY < 0 || srcX >= asset->width || srcY >= asset->height)
				continue;
			pos = (uint32_t)srcY * asset->width + (uint32_t)srcX;
			if (flappyAssetPixelOpaque(asset, (uint32_t)srcX, (uint32_t)srcY))
				game->draw.fb[(uint32_t)screenY * FLAPPY_SCREEN_W + (uint32_t)screenX] = pixels[pos];
		}
	}
}

static void flappyDrawPipeRows(struct FlappyGame *game, int32_t x, int32_t y, int32_t height, bool flip)
{
	const struct FlappyAsset *asset = &flappyAssets[FlappyAssetPipeGreen];
	const uint8_t *pixels = flappyAssetPixels + asset->pixelOffset;
	int32_t drawn = 0;

	while (height > 0) {
		int32_t chunk = height < (int32_t)asset->height ? height : (int32_t)asset->height;

		for (int32_t row = 0; row < chunk; row++) {
			int32_t dstY = y + drawn + row;
			uint32_t srcRow = flip ? (uint32_t)(chunk - row - 1) : (uint32_t)row;

			flappyDrawRunRow(game, asset, pixels, srcRow, x, dstY);
		}
		drawn += chunk;
		height -= chunk;
	}
}

static void flappyDrawPipeSegment(struct FlappyGame *game, float x, float y, float height, bool flip)
{
	if (height <= 0.0f)
		return;
	flappyDrawPipeRows(game, flappyScreenX(x), flappyScreenY(y), flappyScreenH(height), flip);
}

static void flappyDrawPipes(struct FlappyGame *game)
{
	float gap = flappyPipeGap();
	float floorY = flappyFloorY();

	for (uint32_t i = 0; i < 2; i++) {
		struct FlappyPipe *pipe = &game->pipes[i];
		float bottomY = pipe->y + pipe->offset;

		flappyDrawPipeSegment(game, pipe->x, 0.0f, bottomY - gap, true);
		flappyDrawPipeSegment(game, pipe->x, bottomY, floorY - bottomY, false);
	}
}

static void flappyDrawBackground(struct FlappyGame *game)
{
	const struct FlappyAsset *asset = &flappyAssets[FlappyAssetBackgroundDay];
	const uint8_t *pixels = flappyAssetPixels + asset->pixelOffset;

	flappyDrawOpaqueRows(game, asset, pixels, FLAPPY_VIEW_X, 0);
}

static void flappyDrawBase(struct FlappyGame *game)
{
	const struct FlappyAsset *asset = &flappyAssets[FlappyAssetBase];
	const uint8_t *pixels = flappyAssetPixels + asset->pixelOffset;
	int32_t x = flappyScreenX(game->offsetBase);
	int32_t y = flappyScreenY(flappyFloorY());

	while (x <= -(int32_t)asset->width)
		x += (int32_t)asset->width;
	while (x > 0)
		x -= (int32_t)asset->width;
	if (x == 0) {
		flappyDrawOpaqueRows(game, asset, pixels, 0, y);
		return;
	}
	for (uint32_t row = 0; row < asset->height; row++) {
		int32_t dstY = y + (int32_t)row;
		const uint8_t *src = pixels + row * asset->width;
		uint32_t tail = asset->width - (uint32_t)-x;

		if (dstY < 0 || dstY >= FLAPPY_SCREEN_H)
			continue;
		memcpy(game->draw.fb + (uint32_t)dstY * FLAPPY_SCREEN_W, src + (uint32_t)-x, tail);
		memcpy(game->draw.fb + (uint32_t)dstY * FLAPPY_SCREEN_W + tail, src, (uint32_t)-x);
	}
}

static uint8_t flappyBlend(uint8_t src, uint32_t r, uint32_t g, uint32_t b, uint32_t alpha)
{
	uint32_t sr = ((src >> 5) & 7u) * 255u / 7u;
	uint32_t sg = ((src >> 2) & 7u) * 255u / 7u;
	uint32_t sb = (src & 3u) * 255u / 3u;

	if (alpha > 255u)
		alpha = 255u;
	sr = (sr * (255u - alpha) + r * alpha) / 255u;
	sg = (sg * (255u - alpha) + g * alpha) / 255u;
	sb = (sb * (255u - alpha) + b * alpha) / 255u;
	return flappyRgb332(sr, sg, sb);
}

static void flappyOverlay(struct FlappyGame *game, uint32_t r, uint32_t g, uint32_t b, float alpha)
{
	uint32_t a = alpha <= 0.0f ? 0u : (uint32_t)(alpha + 0.5f);
	uint8_t lut[256];

	if (!a)
		return;
	if (a > 255u)
		a = 255u;
	for (uint32_t i = 0; i < 256u; i++)
		lut[i] = flappyBlend((uint8_t)i, r, g, b, a);
	for (uint32_t y = 0; y < FLAPPY_VIEW_H; y++) {
		uint8_t *row = game->draw.fb + y * FLAPPY_SCREEN_W + FLAPPY_VIEW_X;

		for (uint32_t x = 0; x < FLAPPY_VIEW_W; x++)
			row[x] = lut[row[x]];
	}
}

static void flappyAnimateBird(struct FlappyGame *game, float dt)
{
	game->bird.frameTimer += dt;
	while (game->bird.frameTimer >= FLAPPY_ANIM_INTERVAL_FRAMES) {
		game->bird.frameTimer -= FLAPPY_ANIM_INTERVAL_FRAMES;
		game->bird.frame = (game->bird.frame + 1u) % 3u;
		switch (game->bird.frame) {
		case 0:
			game->bird.currentTexture = FlappyAssetYellowBirdDown;
			break;
		case 1:
			game->bird.currentTexture = FlappyAssetYellowBirdMid;
			break;
		default:
			game->bird.currentTexture = FlappyAssetYellowBirdUp;
			break;
		}
	}
}

static void flappyUpdateBirdTextureForLogo(struct FlappyGame *game, float dt)
{
	static const enum FlappyAssetId textures[3] = {
		FlappyAssetYellowBirdDown,
		FlappyAssetYellowBirdMid,
		FlappyAssetYellowBirdUp,
	};

	game->logoFrameTimer += dt;
	while (game->logoFrameTimer >= FLAPPY_ANIM_INTERVAL_FRAMES) {
		game->logoFrameTimer -= FLAPPY_ANIM_INTERVAL_FRAMES;
		game->currentFrameForLogo = (game->currentFrameForLogo + 1u) % 3u;
		game->curTextureAnimBirdForLogo = textures[game->currentFrameForLogo];
	}
}

static void flappyApplyGravity(struct FlappyGame *game, float dt)
{
	float targetAngle;

	game->bird.velocity += FLAPPY_GRAVITY * dt;
	game->bird.y += game->bird.velocity * dt;
	targetAngle = game->bird.velocity > 0.0f ? 90.0f : -30.0f;
	game->bird.angle = flappyMoveTowards(game->bird.angle, targetAngle, 2.0f * dt);
	if (game->bird.angle > 90.0f)
		game->bird.angle = 90.0f;
}

static void flappyJump(struct FlappyGame *game)
{
	game->bird.velocity = FLAPPY_JUMP_VELOCITY;
	game->bird.angle = -30.0f;
}

static bool flappyCheckCollision(struct FlappyGame *game)
{
	for (uint32_t i = 0; i < 2; i++) {
		struct FlappyPipe *pipe = &game->pipes[i];
		float bottomY = pipe->y + pipe->offset;
		float topY = bottomY - flappyPipeGap();

		if (game->bird.x < pipe->x + pipe->w && game->bird.x + game->bird.width > pipe->x &&
				(game->bird.y < topY || game->bird.y + game->bird.height > bottomY))
			return true;
	}
	if (game->bird.y + game->bird.height > flappyFloorY())
		return true;
	return game->bird.y <= 0.0f;
}

static enum FlappyAssetId flappyMedalAsset(uint32_t score)
{
	if (score >= 40u)
		return FlappyAssetPlatinumMedal;
	if (score >= 30u)
		return FlappyAssetGoldMedal;
	if (score >= 20u)
		return FlappyAssetSilverMedal;
	if (score >= 10u)
		return FlappyAssetBronzeMedal;
	return FlappyAssetCount;
}

static uint32_t flappyDigits(uint32_t score, uint8_t digits[10])
{
	uint8_t tmp[10];
	uint32_t count = 0;

	if (!score)
		tmp[count++] = 0;
	while (score && count < 10u) {
		tmp[count++] = (uint8_t)(score % 10u);
		score /= 10u;
	}
	for (uint32_t i = 0; i < count; i++)
		digits[i] = tmp[count - i - 1u];
	return count;
}

static enum FlappyAssetId flappyDigitAsset(uint8_t digit, bool small)
{
	static const enum FlappyAssetId big[10] = {
		FlappyAssetDigit0, FlappyAssetDigit1, FlappyAssetDigit2, FlappyAssetDigit3, FlappyAssetDigit4,
		FlappyAssetDigit5, FlappyAssetDigit6, FlappyAssetDigit7, FlappyAssetDigit8, FlappyAssetDigit9,
	};
	static const enum FlappyAssetId smalls[10] = {
		FlappyAssetDigit0Small, FlappyAssetDigit1Small, FlappyAssetDigit2Small, FlappyAssetDigit3Small,
		FlappyAssetDigit4Small, FlappyAssetDigit5Small, FlappyAssetDigit6Small, FlappyAssetDigit7Small,
		FlappyAssetDigit8Small, FlappyAssetDigit9Small,
	};

	return small ? smalls[digit] : big[digit];
}

static void flappyRenderScoreLeft(struct FlappyGame *game, uint32_t score, float x, float y,
	float digitWidth, bool small)
{
	uint8_t digits[10];
	uint32_t count = flappyDigits(score, digits);

	for (uint32_t i = 0; i < count; i++)
		flappyDrawAsset(game, flappyDigitAsset(digits[i], small), x + (float)i * digitWidth, y);
}

static void flappyRenderScoreCenter(struct FlappyGame *game, uint32_t score, float x, float y,
	float digitWidth)
{
	uint8_t digits[10];
	uint32_t count = flappyDigits(score, digits);

	flappyRenderScoreLeft(game, score, x - ((float)(count - 1u) * 0.5f * digitWidth), y, digitWidth, false);
}

static void flappyRenderSmallScoreRight(struct FlappyGame *game, uint32_t score, float x, float y,
	float digitWidth)
{
	uint8_t digits[10];
	uint32_t count = flappyDigits(score, digits);

	flappyRenderScoreLeft(game, score, x - (float)count * digitWidth, y, digitWidth, true);
}

static bool flappyActionPressed(struct FlappyGame *game)
{
	return (game->draw.pressed & (KEY_BIT_A | KEY_BIT_UP | KEY_BIT_START)) != 0;
}

static bool flappyButtonBump(struct FlappyGame *game, enum FlappyAssetId asset,
	float x, float y, bool released)
{
	if (released)
		y += flappyScaleY(1.0f);
	flappyDrawAsset(game, asset, x, y);
	return released;
}

static void flappyRenderBird(struct FlappyGame *game)
{
	flappyDrawRotatedAsset(game, game->bird.currentTexture, game->bird.x, game->bird.y, game->bird.angle);
}

static void flappyRender(struct FlappyGame *game, float dt)
{
	bool action = flappyActionPressed(game);

	flappyDrawBackground(game);
	if (game->currentState != FlappyStateStopGame &&
			game->currentState != FlappyStateFadeOutGameover &&
			game->currentState != FlappyStateFallBird &&
			game->currentState != FlappyStateFadeInPanel)
		game->offsetBase -= (float)game->gameSpeed * dt;
	flappyDrawBase(game);
	while (game->offsetBase <= -flappyScaleX(100.0f))
		game->offsetBase += flappyScaleX(100.0f);

	game->logoY += game->logoVelocity * dt;
	game->birdY += game->birdVelocity * dt;
	if (game->logoY > flappyScaleY(20.83f) + 25.0f || game->logoY < flappyScaleY(20.83f) - 25.0f)
		game->logoVelocity = -game->logoVelocity;
	if (game->birdY > flappyScaleY(20.83f) + 25.0f || game->birdY < flappyScaleY(20.83f) - 25.0f)
		game->birdVelocity = -game->birdVelocity;

	if (game->currentState == FlappyStateIdle || game->currentState == FlappyStateFadeIn) {
		flappyDrawAsset(game, FlappyAssetLogo, FLAPPY_TITLE_LOGO_X, game->logoY);
		flappyUpdateBirdTextureForLogo(game, dt);
		flappyDrawAsset(game, game->curTextureAnimBirdForLogo, FLAPPY_TITLE_BIRD_X, game->birdY);
		if (flappyButtonBump(game, FlappyAssetStart, FLAPPY_BUTTON_LEFT_X, flappyScaleY(65.0f), action))
			game->currentState = FlappyStateFadeIn;
		(void)flappyButtonBump(game, FlappyAssetScoreButton, FLAPPY_BUTTON_RIGHT_X, flappyScaleY(65.0f), false);
	}
	else if (game->currentState == FlappyStateFadeOut || game->currentState == FlappyStateReadyGame) {
		flappyRenderBird(game);
		flappyDrawAsset(game, FlappyAssetMessage, FLAPPY_READY_TAP_X,
			flappyScaleY(FLAPPY_READY_TAP_Y_PERCENT));
		if (action)
			game->currentState = FlappyStateGoGame;
	}
	else if (game->currentState == FlappyStateGoGame) {
		if (action)
			flappyJump(game);
		flappyApplyGravity(game, dt);
		flappyAnimateBird(game, dt);
		for (uint32_t i = 0; i < 2; i++) {
			game->pipes[i].x -= (float)game->gameSpeed * dt;
			if (game->pipes[i].x < -flappyScaleX(15.0f)) {
				game->pipes[i].x = flappyScaleX(115.0f);
				game->pipes[i].offset = (float)flappyRandom(game,
					(int32_t)flappyScaleY(-SPACE_BETWEEN_PIPES),
					(int32_t)flappyScaleY(SPACE_BETWEEN_PIPES));
			}
			if (game->bird.x + (game->bird.width * 0.5f) >= game->pipes[i].x + game->pipes[i].w &&
					game->bird.x + (game->bird.width * 0.5f) <=
					game->pipes[i].x + game->pipes[i].w + (float)game->gameSpeed * dt)
				game->score++;
		}
		if (flappyCheckCollision(game))
			game->currentState = FlappyStateStopGame;
		flappyDrawPipes(game);
		flappyRenderBird(game);
		if (game->score > 0u)
			flappyRenderScoreCenter(game, game->score, flappyScaleX(50.0f), flappyScaleY(7.0f),
				flappyScaleX(8.0f));
	}
	else if (game->currentState == FlappyStateStopGame) {
		if (game->score > game->bestScore) {
			game->bestScore = game->score;
			game->newBestScore = true;
			game->saveDirty = true;
			game->saveAfterPresent = true;
		}
		game->currentState = FlappyStateFadeOutGameover;
	}
	else if (game->currentState == FlappyStateFadeOutGameover) {
		game->fadeOutAlpha -= 5.0f * dt;
		if (game->fadeOutAlpha <= 0.0f) {
			game->fadeOutAlpha = 0.0f;
			game->currentState = FlappyStateFallBird;
		}
		flappyDrawPipes(game);
		flappyRenderBird(game);
		flappyOverlay(game, 255, 255, 255, game->fadeOutAlpha);
	}
	else if (game->currentState == FlappyStateFallBird) {
		flappyApplyGravity(game, dt);
		flappyDrawPipes(game);
		flappyRenderBird(game);
		if (game->bird.y + game->bird.height >= flappyFloorY() - game->bird.height) {
			game->bird.y = flappyFloorY() - game->bird.height;
			game->currentState = FlappyStateFadeInPanel;
		}
	}
	else if (game->currentState == FlappyStateFadeInPanel) {
		enum FlappyAssetId medal;

		flappyDrawPipes(game);
		flappyRenderBird(game);
		game->panelY = flappyMoveTowards(game->panelY, flappyScaleY(30.0f), 20.0f * dt);
		flappyDrawAsset(game, FlappyAssetPanel, flappyScaleX(15.0f), game->panelY);
		flappyRenderSmallScoreRight(game, game->score, flappyScaleX(78.0f),
			game->panelY + flappyScaleY(FLAPPY_PANEL_SCORE_Y_PERCENT), flappyScaleX(4.0f));
		flappyRenderSmallScoreRight(game, game->bestScore, flappyScaleX(78.0f),
			game->panelY + flappyScaleY(FLAPPY_PANEL_BEST_Y_PERCENT), flappyScaleX(4.0f));
		if (game->newBestScore)
			flappyDrawAsset(game, FlappyAssetNew, flappyScaleX(56.0f), game->panelY + flappyScaleY(9.0f));
		flappyDrawAsset(game, FlappyAssetGameOver, flappyScaleX(17.5f),
			flappyScaleY(FLAPPY_GAME_OVER_Y_PERCENT));
		medal = flappyMedalAsset(game->score);
		if (medal != FlappyAssetCount)
			flappyDrawAsset(game, medal, flappyScaleX(22.0f), game->panelY + flappyScaleY(6.0f));
		if (flappyButtonBump(game, FlappyAssetOk, FLAPPY_BUTTON_LEFT_X, flappyScaleY(65.0f), action)) {
			game->currentState = FlappyStateIdle;
			flappyResetRound(game);
		}
		(void)flappyButtonBump(game, FlappyAssetShare, FLAPPY_BUTTON_RIGHT_X, flappyScaleY(65.0f), false);
	}

	if (game->currentState == FlappyStateFadeIn) {
		game->alpha += 5.0f * dt;
		if (game->alpha >= 255.0f) {
			game->alpha = 255.0f;
			game->currentState = FlappyStateFadeOut;
		}
	}
	else if (game->currentState == FlappyStateFadeOut) {
		game->alpha -= 5.0f * dt;
		if (game->alpha <= 0.0f) {
			game->alpha = 0.0f;
			game->currentState = FlappyStateReadyGame;
		}
	}
	if (game->currentState == FlappyStateFadeIn || game->currentState == FlappyStateFadeOut)
		flappyOverlay(game, 0, 0, 0, game->alpha);
}

static bool flappyInit(struct FlappyGame *game, const struct DcAppHostApi *host,
	const struct DcAppRunArgs *args)
{
	uint64_t now;

	if (!game || !host || !args || !args->canvas)
		return false;
	memset(game, 0, sizeof(*game));
	game->host = host;
	game->args = args;
	game->frame = dc32PortMalloc(FLAPPY_SCREEN_W * FLAPPY_SCREEN_H);
	if (!game->frame)
		return false;
	if (!dcAppDrawInit(&game->draw, host, args, game->frame, FLAPPY_SCREEN_W, FLAPPY_SCREEN_H))
		return false;
	now = host->getTime ? host->getTime() : 0x464c5059u;
	game->rng = (uint32_t)(now ^ (now >> 32));
	if (!game->rng)
		game->rng = 0x464c5059u;
	game->lastTicks = now;
	game->gameSpeed = ((uint32_t)FLAPPY_VIRTUAL_W / 135u) * FLAPPY_SPEED_MULTIPLIER;
	game->currentState = FlappyStateIdle;
	game->logoY = flappyScaleY(20.83f);
	game->birdY = flappyScaleY(20.83f);
	game->logoVelocity = 1.1f;
	game->birdVelocity = 1.1f;
	game->logoFrameTimer = 0.0f;
	game->curTextureAnimBirdForLogo = FlappyAssetYellowBirdMid;
	game->panelY = flappyScaleY(100.0f);
	game->fadeOutAlpha = 255.0f;
	flappyResetBird(game);
	flappyResetPipes(game);
	flappyLoadBestScore(game);
	return true;
}

int flappyAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct FlappyGame *game;

	mFlappyAbort = false;
	dc32PortHeapInitDefault();
	game = dc32PortCalloc(1, sizeof(*game));
	if (!game)
		return -1;
	if (!flappyInit(game, host, args)) {
		dc32PortFree(game->frame);
		dc32PortFree(game);
		return -1;
	}
	dispSetFramerate(FLAPPY_FPS);
	dcAppDrawWaitRelease(&game->draw, KEY_BIT_A | KEY_BIT_B | KEY_BIT_UP |
		KEY_BIT_START | UI_KEY_BIT_CENTER);
	game->lastTicks = flappyNowTicks(game);
	flappyRender(game, 0.0f);
	while (!mFlappyAbort) {
		bool running = dcAppDrawFrame(&game->draw, UI_KEY_BIT_CENTER);
		float dt = flappyDeltaFrames(game);

		if (game->saveAfterPresent) {
			game->saveAfterPresent = false;
			(void)flappyWriteBestScore(game);
		}
		if (!running)
			break;
		flappyRender(game, dt);
	}
	if (game->saveDirty)
		(void)flappyWriteBestScore(game);
	dcAppDrawWaitRelease(&game->draw, UI_KEY_BIT_CENTER);
	dc32PortFree(game->frame);
	dc32PortFree(game);
	dispSetFramerate(60);
	return 0;
}

void flappyAppAbort(void)
{
	mFlappyAbort = true;
}
