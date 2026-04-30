#include <string.h>
#include "badgeLeds.h"
#include "pioWS2812.h"
#include "timebase.h"


#define LED_GAME_WRITE_PAUSE_TICKS		(TICKS_PER_SECOND * 2)
#define LED_DEFAULT_TINT				56
#define LED_MAX_SPEED					4


static struct Settings mLedSettings;
static bool mHaveLedSettings;
static uint64_t mNextLedFrameTime, mLastGameLedWriteTime;
static uint8_t mLedFrame;


static uint_fast8_t badgeLedsPrvSanitizeMode(uint_fast8_t mode)
{
	return mode < LedModeNumModes ? mode : LedModeOff;
}

static uint_fast8_t badgeLedsPrvSanitizeSpeed(uint_fast8_t speed)
{
	return speed <= LED_MAX_SPEED ? speed : 2;
}

const char* badgeLedsModeName(uint_fast8_t mode)
{
	static const char names[][8] = {
		[LedModeOff] = "OFF",
		[LedModeSolid] = "SOLID",
		[LedModeRainbow] = "RAINBOW",
		[LedModeFlame] = "FLAME",
		[LedModeTravelingDot] = "DOT",
	};

	mode = badgeLedsPrvSanitizeMode(mode);
	return names[mode];
}

static uint_fast8_t badgeLedsPrvScale(uint_fast8_t val, uint_fast8_t scale)
{
	return ((uint32_t)val * scale) / 255;
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

static uint_fast8_t badgeLedsPrvFrameIntervalIdx(uint_fast8_t speed)
{
	static const uint8_t intervals[] = {24, 16, 10, 6, 3};

	speed = badgeLedsPrvSanitizeSpeed(speed);
	return intervals[speed];
}

static void badgeLedsPrvRenderSolid(uint_fast8_t red, uint_fast8_t green, uint_fast8_t blue)
{
	ws2812SetAllRgb(red, green, blue);
	ws2812refresh();
}

static void badgeLedsPrvRenderRainbow(void)
{
	uint_fast8_t i;

	for (i = 0; i < NUM_WS2812s; i++) {
		uint8_t r, g, b;

		badgeLedsPrvWheel(mLedFrame * 5 + i * 28, &r, &g, &b);
		ws2812SetRgb(i, r / 2, g / 2, b / 2);
	}
	ws2812refresh();
}

static void badgeLedsPrvRenderFlame(void)
{
	uint_fast8_t i;
	uint_fast8_t pulse = (mLedFrame & 0x1f) <= 15 ? (mLedFrame & 0x0f) : (31 - (mLedFrame & 0x1f));

	for (i = 0; i < NUM_WS2812s; i++) {
		uint_fast8_t flicker = (mLedFrame * 17 + i * 47 + (i * i * 13)) & 0x3f;
		uint_fast8_t heat = 48 + pulse * 4 + flicker / 2;

		ws2812SetRgb(i, heat, heat / 3 + flicker / 4, flicker / 16);
	}
	ws2812refresh();
}

static void badgeLedsPrvRenderTravelingDot(void)
{
	uint_fast8_t i, pos = mLedFrame % NUM_WS2812s;
	uint_fast8_t red = mLedSettings.ledRed, green = mLedSettings.ledGreen, blue = mLedSettings.ledBlue;

	if (!red && !green && !blue)
		red = green = blue = LED_DEFAULT_TINT;

	for (i = 0; i < NUM_WS2812s; i++) {
		uint_fast8_t dist = i > pos ? i - pos : pos - i;
		uint_fast8_t scale;

		if (dist > NUM_WS2812s / 2)
			dist = NUM_WS2812s - dist;

		scale = dist == 0 ? 255 : (dist == 1 ? 96 : (dist == 2 ? 32 : 0));
		ws2812SetRgb(i, badgeLedsPrvScale(red, scale), badgeLedsPrvScale(green, scale), badgeLedsPrvScale(blue, scale));
	}
	ws2812refresh();
}

static void badgeLedsPrvRenderCurrent(void)
{
	switch (badgeLedsPrvSanitizeMode(mLedSettings.ledMode)) {
		case LedModeSolid:
			badgeLedsPrvRenderSolid(mLedSettings.ledRed, mLedSettings.ledGreen, mLedSettings.ledBlue);
			break;

		case LedModeRainbow:
			badgeLedsPrvRenderRainbow();
			break;

		case LedModeFlame:
			badgeLedsPrvRenderFlame();
			break;

		case LedModeTravelingDot:
			badgeLedsPrvRenderTravelingDot();
			break;

		case LedModeOff:
		default:
			badgeLedsPrvRenderSolid(0, 0, 0);
			break;
	}
}

void badgeLedsApplySettings(const struct Settings *settings, bool force)
{
	bool changed = !mHaveLedSettings || memcmp(&mLedSettings, settings, sizeof(mLedSettings));

	mLedSettings = *settings;
	mLedSettings.ledMode = badgeLedsPrvSanitizeMode(mLedSettings.ledMode);
	mLedSettings.ledSpeed = badgeLedsPrvSanitizeSpeed(mLedSettings.ledSpeed);
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
