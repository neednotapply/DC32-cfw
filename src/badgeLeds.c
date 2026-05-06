#include <string.h>
#include "badgeLeds.h"
#include "pioWS2812.h"
#include "timebase.h"


#define LED_GAME_WRITE_PAUSE_TICKS		(TICKS_PER_SECOND * 2)
#define LED_DEFAULT_TINT				56
#define LED_MIN_BRIGHTNESS				15
#define LED_MIN_SPEED					1
#define LED_MAX_SPEED					10


static struct Settings mLedSettings;
static bool mHaveLedSettings;
static uint64_t mNextLedFrameTime, mLastGameLedWriteTime;
static uint8_t mLedFrame;


static uint_fast8_t badgeLedsPrvSanitizeMode(uint_fast8_t mode)
{
	return mode < LedModeNumModes ? mode : LedModeOff;
}

static uint_fast8_t badgeLedsPrvSanitizeColor(uint_fast8_t color)
{
	return color < LedColorNumColors ? color : LedColorCustom;
}

static uint_fast8_t badgeLedsPrvSanitizeSpeed(uint_fast8_t speed)
{
	return speed >= LED_MIN_SPEED && speed <= LED_MAX_SPEED ? speed : 4;
}

static uint_fast8_t badgeLedsPrvBrightness(void)
{
	return mLedSettings.ledBrightness;
}

static uint_fast8_t badgeLedsPrvSanitizeBrightness(uint_fast8_t brightness)
{
	return brightness >= LED_MIN_BRIGHTNESS ? brightness : LED_MIN_BRIGHTNESS;
}

const char* badgeLedsModeName(uint_fast8_t mode)
{
	static const char names[][11] = {
		[LedModeOff] = "OFF",
		[LedModeSolid] = "SOLID",
		[LedModeRainbow] = "RAINBOW",
		[LedModePulse] = "PULSE",
		[LedModeTravelingDot] = "DOT",
		[LedModeRandom] = "RANDOM",
		[LedModeFlashlight] = "FLASHLIGHT",
	};

	mode = badgeLedsPrvSanitizeMode(mode);
	return names[mode];
}

const char* badgeLedsColorName(uint_fast8_t color)
{
	static const char names[][8] = {
		[LedColorCustom] = "CUSTOM",
		[LedColorRainbow] = "RAINBOW",
		[LedColorFlame] = "FLAME",
		[LedColorRandom] = "RANDOM",
	};

	color = badgeLedsPrvSanitizeColor(color);
	return names[color];
}

static uint_fast8_t badgeLedsPrvScale(uint_fast8_t val, uint_fast8_t scale)
{
	return ((uint32_t)val * scale) / 255;
}

static void badgeLedsPrvSetRgb(uint32_t ledIdx, uint_fast8_t red, uint_fast8_t green, uint_fast8_t blue)
{
	uint_fast8_t brightness = badgeLedsPrvBrightness();

	ws2812SetRgb(ledIdx, badgeLedsPrvScale(red, brightness), badgeLedsPrvScale(green, brightness), badgeLedsPrvScale(blue, brightness));
}

static void badgeLedsPrvWheel(uint_fast8_t pos, uint8_t *rP, uint8_t *gP, uint8_t *bP)
{
	if (pos < 85) {
		*rP = 255 - pos * 3;
		*gP = pos * 3;
		*bP = 0;
	}
	else if (pos < 170) {
		pos -= 85;
		*rP = 0;
		*gP = 255 - pos * 3;
		*bP = pos * 3;
	}
	else {
		pos -= 170;
		*rP = pos * 3;
		*gP = 0;
		*bP = 255 - pos * 3;
	}
}

static uint32_t badgeLedsPrvRand(uint32_t seed)
{
	seed ^= seed >> 16;
	seed *= 0x7feb352dUL;
	seed ^= seed >> 15;
	seed *= 0x846ca68bUL;
	seed ^= seed >> 16;
	return seed;
}

static void badgeLedsPrvCustomColor(uint8_t *rP, uint8_t *gP, uint8_t *bP, bool useDefault)
{
	*rP = mLedSettings.ledRed;
	*gP = mLedSettings.ledGreen;
	*bP = mLedSettings.ledBlue;

	if (useDefault && !*rP && !*gP && !*bP)
		*rP = *gP = *bP = LED_DEFAULT_TINT;
}

static void badgeLedsPrvColor(uint_fast8_t ledIdx, uint_fast8_t phase, uint8_t *rP, uint8_t *gP, uint8_t *bP, bool useDefault)
{
	switch (badgeLedsPrvSanitizeColor(mLedSettings.ledColor)) {
		case LedColorRainbow:
			badgeLedsPrvWheel(phase, rP, gP, bP);
			*rP /= 2;
			*gP /= 2;
			*bP /= 2;
			break;

		case LedColorFlame: {
			uint_fast8_t flicker = badgeLedsPrvRand(((uint32_t)mLedFrame << 8) ^ ledIdx ^ phase) & 0x3f;

			*rP = 180 + flicker;
			*gP = 24 + flicker;
			*bP = flicker > 48 ? (flicker - 48) / 3 : 0;
			break;
		}

		case LedColorRandom: {
			uint32_t rnd = badgeLedsPrvRand(((uint32_t)(mLedFrame / 4) << 12) ^ ((uint32_t)ledIdx << 4) ^ phase);

			*rP = 32 + (rnd & 0x7f);
			*gP = 32 + ((rnd >> 8) & 0x7f);
			*bP = 32 + ((rnd >> 16) & 0x7f);
			break;
		}

		case LedColorCustom:
		default:
			badgeLedsPrvCustomColor(rP, gP, bP, useDefault);
			break;
	}
}

static uint_fast8_t badgeLedsPrvFrameIntervalIdx(uint_fast8_t speed)
{
	static const uint8_t intervals[] = {0, 3, 5, 7, 10, 12, 15, 17, 19, 22, 24};

	speed = badgeLedsPrvSanitizeSpeed(speed);
	return intervals[speed];
}

static void badgeLedsPrvRenderSolid(uint_fast8_t red, uint_fast8_t green, uint_fast8_t blue)
{
	uint_fast8_t brightness = badgeLedsPrvBrightness();

	ws2812SetAllRgb(badgeLedsPrvScale(red, brightness), badgeLedsPrvScale(green, brightness), badgeLedsPrvScale(blue, brightness));
	ws2812refresh();
}

static void badgeLedsPrvRenderSolidColor(void)
{
	uint8_t r, g, b;

	badgeLedsPrvColor(0, mLedFrame * 5, &r, &g, &b, false);
	badgeLedsPrvRenderSolid(r, g, b);
}

static void badgeLedsPrvRenderRainbow(void)
{
	uint_fast8_t i;

	for (i = 0; i < NUM_WS2812s; i++) {
		uint8_t r, g, b;

		badgeLedsPrvColor(i, (uint8_t)(mLedFrame * 5 + i * 28), &r, &g, &b, true);
		badgeLedsPrvSetRgb(i, r, g, b);
	}
	ws2812refresh();
}

static void badgeLedsPrvRenderPulse(void)
{
	uint_fast8_t i;
	uint8_t red, green, blue;
	uint_fast8_t pulse = (mLedFrame & 0x1f) <= 15 ? (mLedFrame & 0x0f) : (31 - (mLedFrame & 0x1f));
	uint_fast8_t scale = 48 + pulse * 13;

	badgeLedsPrvColor(0, mLedFrame * 5, &red, &green, &blue, true);

	for (i = 0; i < NUM_WS2812s; i++) {
		badgeLedsPrvSetRgb(i, badgeLedsPrvScale(red, scale), badgeLedsPrvScale(green, scale), badgeLedsPrvScale(blue, scale));
	}
	ws2812refresh();
}

static void badgeLedsPrvRenderTravelingDot(void)
{
	uint_fast8_t i, pos = mLedFrame % NUM_WS2812s;

	for (i = 0; i < NUM_WS2812s; i++) {
		uint8_t red, green, blue;
		uint_fast8_t dist = i > pos ? i - pos : pos - i;
		uint_fast8_t scale;

		if (dist > NUM_WS2812s / 2)
			dist = NUM_WS2812s - dist;

		scale = dist == 0 ? 255 : (dist == 1 ? 96 : (dist == 2 ? 32 : 0));
		badgeLedsPrvColor(i, (uint8_t)(mLedFrame * 5 + i * 13), &red, &green, &blue, true);
		badgeLedsPrvSetRgb(i, badgeLedsPrvScale(red, scale), badgeLedsPrvScale(green, scale), badgeLedsPrvScale(blue, scale));
	}
	ws2812refresh();
}

static void badgeLedsPrvRenderRandom(void)
{
	uint_fast8_t i;

	for (i = 0; i < NUM_WS2812s; i++) {
		uint8_t red, green, blue;
		uint32_t rnd = badgeLedsPrvRand(((uint32_t)mLedFrame << 12) ^ ((uint32_t)i << 5));
		uint_fast8_t scale = rnd & 1 ? 255 : ((rnd & 2) ? 96 : 0);

		badgeLedsPrvColor(i, rnd & 0xff, &red, &green, &blue, true);
		badgeLedsPrvSetRgb(i, badgeLedsPrvScale(red, scale), badgeLedsPrvScale(green, scale), badgeLedsPrvScale(blue, scale));
	}
	ws2812refresh();
}

static void badgeLedsPrvRenderFlashlight(void)
{
	static const uint8_t activeLeds[] = {0, 2, 8, 7};	//PCB labels LED2, LED4, LED10, LED9
	uint_fast8_t i;

	ws2812SetAllRgb(0, 0, 0);
	for (i = 0; i < sizeof(activeLeds) / sizeof(*activeLeds); i++) {
		uint8_t red, green, blue;

		badgeLedsPrvColor(activeLeds[i], (uint8_t)(mLedFrame * 5 + activeLeds[i] * 28), &red, &green, &blue, true);
		badgeLedsPrvSetRgb(activeLeds[i], red, green, blue);
	}
	ws2812refresh();
}

static void badgeLedsPrvRenderCurrent(void)
{
	switch (badgeLedsPrvSanitizeMode(mLedSettings.ledMode)) {
		case LedModeSolid:
			badgeLedsPrvRenderSolidColor();
			break;

		case LedModeRainbow:
			badgeLedsPrvRenderRainbow();
			break;

		case LedModePulse:
			badgeLedsPrvRenderPulse();
			break;

		case LedModeTravelingDot:
			badgeLedsPrvRenderTravelingDot();
			break;

		case LedModeRandom:
			badgeLedsPrvRenderRandom();
			break;

		case LedModeFlashlight:
			badgeLedsPrvRenderFlashlight();
			break;

		case LedModeOff:
		default:
			badgeLedsPrvRenderSolid(0, 0, 0);
			break;
	}
}

void badgeLedsApplySettings(const struct Settings *settings, bool force)
{
	struct Settings newSettings = *settings;
	bool changed;

	newSettings.ledMode = badgeLedsPrvSanitizeMode(newSettings.ledMode);
	newSettings.ledColor = badgeLedsPrvSanitizeColor(newSettings.ledColor);
	newSettings.ledSpeed = badgeLedsPrvSanitizeSpeed(newSettings.ledSpeed);
	newSettings.ledBrightness = badgeLedsPrvSanitizeBrightness(newSettings.ledBrightness);
	changed = !mHaveLedSettings || memcmp(&mLedSettings, &newSettings, sizeof(mLedSettings));

	mLedSettings = newSettings;
	mHaveLedSettings = true;

	if (changed || force) {
		mLedFrame = 0;
		mNextLedFrameTime = getTime();
		badgeLedsPrvRenderCurrent();
	}
}

void badgeLedsTick(void)
{
	uint64_t now;
	uint_fast8_t mode;

	if (!mHaveLedSettings)
		return;

	mode = badgeLedsPrvSanitizeMode(mLedSettings.ledMode);
	if (mode == LedModeOff || mode == LedModeSolid)
		return;

	now = getTime();
	if (mLastGameLedWriteTime && now - mLastGameLedWriteTime < LED_GAME_WRITE_PAUSE_TICKS)
		return;

	if (now < mNextLedFrameTime)
		return;

	mLedFrame++;
	mNextLedFrameTime = now + TICKS_PER_SECOND / badgeLedsPrvFrameIntervalIdx(mLedSettings.ledSpeed);
	badgeLedsPrvRenderCurrent();
}

void badgeLedsGameWrite(void)
{
	mLastGameLedWriteTime = getTime();
}
