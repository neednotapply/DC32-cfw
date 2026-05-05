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
	static const char names[][8] = {
		[LedModeOff] = "OFF",
		[LedModeSolid] = "SOLID",
		[LedModeRainbow] = "RAINBOW",
		[LedModeFlame] = "PULSE",
		[LedModeTravelingDot] = "DOT",
	};

	mode = badgeLedsPrvSanitizeMode(mode);
	return names[mode];
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

static void badgeLedsPrvRenderRainbow(void)
{
	uint_fast8_t i;

	for (i = 0; i < NUM_WS2812s; i++) {
		uint8_t r, g, b;

		badgeLedsPrvWheel((uint8_t)(mLedFrame * 5 + i * 28), &r, &g, &b);
		badgeLedsPrvSetRgb(i, r / 2, g / 2, b / 2);
	}
	ws2812refresh();
}

static void badgeLedsPrvRenderPulse(void)
{
	uint_fast8_t i;
	uint_fast8_t red = mLedSettings.ledRed, green = mLedSettings.ledGreen, blue = mLedSettings.ledBlue;
	uint_fast8_t pulse = (mLedFrame & 0x1f) <= 15 ? (mLedFrame & 0x0f) : (31 - (mLedFrame & 0x1f));
	uint_fast8_t scale = 48 + pulse * 13;

	if (!red && !green && !blue)
		red = green = blue = LED_DEFAULT_TINT;

	for (i = 0; i < NUM_WS2812s; i++) {
		badgeLedsPrvSetRgb(i, badgeLedsPrvScale(red, scale), badgeLedsPrvScale(green, scale), badgeLedsPrvScale(blue, scale));
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
		badgeLedsPrvSetRgb(i, badgeLedsPrvScale(red, scale), badgeLedsPrvScale(green, scale), badgeLedsPrvScale(blue, scale));
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
			badgeLedsPrvRenderPulse();
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
	struct Settings newSettings = *settings;
	bool changed;

	newSettings.ledMode = badgeLedsPrvSanitizeMode(newSettings.ledMode);
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
