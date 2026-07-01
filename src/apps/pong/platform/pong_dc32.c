#include "apps/pong/platform/pong_platform.h"

#include <string.h>

#include "audioPwm.h"
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "ui.h"

#define PONG_DC32_FPS 30u

struct PongDc32Context {
	const struct DcAppHostApi *host;
	struct DcAppDrawCtx draw;
	struct PongRenderer *renderer;
	struct PongAudio *audio;
	uint_fast16_t previousKeys;
	uint32_t frame;
	uint32_t toneFrames;
	uint8_t previousVolume;
};

static struct PongDc32Context mPongDc32;
static uint8_t mPongBackbuffer[320u * 240u];

static uint16_t pongDc32Wheel(uint32_t value)
{
	value %= 192u;
	if (value < 64u)
		return dcAppDrawRgb565(255u - value * 4u, value * 4u, 40u);
	if (value < 128u) {
		value -= 64u;
		return dcAppDrawRgb565(40u, 255u - value * 4u, value * 4u);
	}
	value -= 128u;
	return dcAppDrawRgb565(value * 4u, 40u, 255u - value * 4u);
}

static uint16_t pongDc32Color(const struct PongDc32Context *ctx)
{
	uint8_t role = ctx->renderer ? ctx->renderer->colorRole : PongColorNeutral;
	uint8_t theme = ctx->renderer ? ctx->renderer->theme : PongColorClassic;

	if (theme == PongColorClassic) {
		if (role == PongColorField)
			return dcAppDrawRgb565(145, 145, 145);
		return dcAppDrawRgb565(235, 240, 255);
	}
	if (theme == PongColorTeams) {
		static const uint16_t colors[] = {
			0xffff, 0x7bcf, 0xff40, 0x4fff, 0xfd20, 0x87e0, 0xf81f,
		};

		return colors[role < sizeof(colors) / sizeof(colors[0]) ?
			role : PongColorNeutral];
	}
	return pongDc32Wheel((ctx->renderer ? ctx->renderer->frame : 0u) * 3u +
		(uint32_t)role * 29u);
}

static uint16_t pongDc32Background(const struct PongDc32Context *ctx)
{
	if (ctx->renderer && ctx->renderer->theme == PongColorClassic)
		return dcAppDrawRgb565(0, 0, 0);
	if (ctx->renderer && ctx->renderer->theme == PongColorRainbow)
		return dcAppDrawRgb565(5, 2, 18);
	return dcAppDrawRgb565(2, 7, 18);
}

static void pongDc32Clear(void *context)
{
	struct PongDc32Context *ctx = context;

	dcAppDrawClear(&ctx->draw, pongDc32Background(ctx));
}

static void pongDc32Rect(void *context, int32_t x, int32_t y, int32_t w, int32_t h)
{
	struct PongDc32Context *ctx = context;

	dcAppDrawFill(&ctx->draw, x, y, w, h, pongDc32Color(ctx));
}

static void pongDc32Line(void *context, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	struct PongDc32Context *ctx = context;

	dcAppDrawLine(&ctx->draw, x1, y1, x2, y2, pongDc32Color(ctx));
}

static void pongDc32Text(void *context, int32_t x, int32_t y, const char *text)
{
	struct PongDc32Context *ctx = context;

	while (*text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, FontMedium, (unsigned char)*text)) {
			for (uint_fast8_t row = 0; row < glyph.height; row++)
				for (uint_fast8_t col = 0; col < glyph.width; col++)
					if (fontGetGlyphPixel(&glyph, row, col))
						dcAppDrawPixel(&ctx->draw, x + col, y + row,
							pongDc32Color(ctx));
			x += glyph.width + 1;
		}
		text++;
	}
}

static void pongDc32Present(void *context)
{
	struct PongDc32Context *ctx = context;

	dispPrvWaitForScanoutStart();
	dcAppDrawPresent(&ctx->draw);
}

static void pongDc32Beep(void *context, uint32_t frequency, uint32_t durationMs)
{
	struct PongDc32Context *ctx = context;

	if (!frequency) {
		ctx->toneFrames = 0;
		audioPwmStop();
		return;
	}
	if (!ctx->audio || !ctx->audio->enabled || !ctx->audio->volume)
		return;
	audioPwmSetVolume(ctx->audio->volume);
	(void)audioPwmTone(frequency);
	ctx->toneFrames = (durationMs * PONG_DC32_FPS + 999u) / 1000u;
	if (!ctx->toneFrames)
		ctx->toneFrames = 1;
}

static void pongDc32Click(void *context)
{
	pongDc32Beep(context, 720u, 45u);
}

static void pongDc32ScoreSound(void *context)
{
	pongDc32Beep(context, 330u, 150u);
}

static void pongDc32WaitFrame(void *context)
{
	struct PongDc32Context *ctx = context;

	dispPrvFrameCtrWait();
	if (ctx->host && ctx->host->ledsTick)
		ctx->host->ledsTick();
	ctx->frame++;
}

static uint32_t pongDc32TicksMs(void *context)
{
	struct PongDc32Context *ctx = context;

	return ctx->frame * (1000u / PONG_DC32_FPS);
}

static int8_t pongDc32Axis(uint_fast16_t keys, uint_fast16_t negative, uint_fast16_t positive)
{
	if ((keys & negative) && !(keys & positive))
		return -1;
	if ((keys & positive) && !(keys & negative))
		return 1;
	return 0;
}

static bool pongDc32ReadInput(void *context, struct PongInput *input)
{
	struct PongDc32Context *ctx = context;
	uint_fast16_t keys = ctx->host && ctx->host->uiKeysRaw ?
		ctx->host->uiKeysRaw() : uiGetUiKeysRaw();
	uint_fast16_t pressed = keys & ~ctx->previousKeys;

	memset(input, 0, sizeof(*input));
	if ((pressed & UI_KEY_BIT_CENTER) && ctx->host && ctx->host->portMenu) {
		bool resume = ctx->host->portMenu(&ctx->draw.displayCnv);

		keys = ctx->host->uiKeysRaw ? ctx->host->uiKeysRaw() : 0;
		ctx->previousKeys = keys;
		return resume;
	}
	input->axisX[0] = pongDc32Axis(keys, KEY_BIT_LEFT, KEY_BIT_RIGHT);
	input->axisY[0] = pongDc32Axis(keys, KEY_BIT_UP, KEY_BIT_DOWN);
	input->pressedX[0] = pongDc32Axis(pressed, KEY_BIT_LEFT, KEY_BIT_RIGHT);
	input->pressedY[0] = pongDc32Axis(pressed, KEY_BIT_UP, KEY_BIT_DOWN);
	input->confirmPressed = (pressed & KEY_BIT_A) != 0;
	input->backPressed = (pressed & KEY_BIT_B) != 0;
	ctx->previousKeys = keys;
	return (pressed & UI_KEY_BIT_CENTER) == 0;
}

static void pongDc32Tick(void *context)
{
	struct PongDc32Context *ctx = context;

	if (ctx->audio && !ctx->audio->enabled && ctx->toneFrames) {
		ctx->toneFrames = 0;
		audioPwmStop();
	}
	if (ctx->toneFrames && !--ctx->toneFrames)
		audioPwmStop();
}

static void pongDc32Shutdown(void *context)
{
	struct PongDc32Context *ctx = context;

	audioPwmStop();
	audioPwmSetVolume(ctx->previousVolume);
	dispSetFramerate(60);
	if (ctx->host && ctx->host->uiKeysRaw && ctx->host->delayMsec)
		while (ctx->host->uiKeysRaw() & (KEY_BIT_A | KEY_BIT_B | UI_KEY_BIT_CENTER))
			ctx->host->delayMsec(10);
}

bool pongDc32PlatformInit(struct PongPlatform *platform, const struct DcAppHostApi *host,
	const struct DcAppRunArgs *args)
{
	if (!platform || !host || !args || !args->canvas)
		return false;
	memset(&mPongDc32, 0, sizeof(mPongDc32));
	memset(platform, 0, sizeof(*platform));
	mPongDc32.host = host;
	mPongDc32.previousVolume = (uint8_t)audioPwmGetVolume();
	if (!dcAppDrawInit(&mPongDc32.draw, host, args, mPongBackbuffer, 320u, 240u))
		return false;
	dispSetFramerate(PONG_DC32_FPS);
	mPongDc32.previousKeys = host->uiKeysRaw ? host->uiKeysRaw() : uiGetUiKeysRaw();

	platform->context = &mPongDc32;
	platform->renderer.context = &mPongDc32;
	platform->renderer.theme = PongColorClassic;
	platform->renderer.colorRole = PongColorNeutral;
	mPongDc32.renderer = &platform->renderer;
	platform->renderer.clear = pongDc32Clear;
	platform->renderer.draw_rect = pongDc32Rect;
	platform->renderer.draw_line = pongDc32Line;
	platform->renderer.draw_text = pongDc32Text;
	platform->renderer.present = pongDc32Present;
	platform->audio.context = &mPongDc32;
	platform->audio.volume = mPongDc32.previousVolume;
	platform->audio.enabled = false;
	mPongDc32.audio = &platform->audio;
	platform->audio.beep = pongDc32Beep;
	platform->audio.click = pongDc32Click;
	platform->audio.score_sound = pongDc32ScoreSound;
	platform->timing.context = &mPongDc32;
	platform->timing.waitFrame = pongDc32WaitFrame;
	platform->timing.ticksMs = pongDc32TicksMs;
	platform->readInput = pongDc32ReadInput;
	platform->tick = pongDc32Tick;
	platform->shutdown = pongDc32Shutdown;
	return true;
}
