#pragma GCC optimize ("Os")
#include "apps/picoware_app.h"

#include <math.h>
#include <stdint.h>
#include <string.h>
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "gb.h"

#define PICOWARE_SCREEN_W 320u
#define PICOWARE_SCREEN_H 240u
#define PICOWARE_FPS 30u

struct PicowareAppCtx {
	const struct DcAppHostApi *host;
	struct DcAppDrawCtx draw;
	uint32_t rng;
	uint32_t frame;
};

static uint8_t mPicowareBackbuffer[PICOWARE_SCREEN_W * PICOWARE_SCREEN_H];

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

static bool pwFrame(struct PicowareAppCtx *ctx)
{
	bool running = dcAppDrawFrame(&ctx->draw, UI_KEY_BIT_CENTER);

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

#if DCAPP_RUNTIME_ID == 220
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
