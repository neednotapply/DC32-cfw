#include <string.h>
#include "badgeLeds.h"
#include "gb.h"
#include "pioWS2812.h"
#include "timebase.h"
#include "ui.h"


#define LED_GAME_WRITE_PAUSE_TICKS		(TICKS_PER_SECOND * 2)
#define LED_NON_SETTINGS_REFRESH_TICKS	TICKS_PER_SECOND
#define LED_DYNAMIC_STALL_TICKS		(TICKS_PER_SECOND * 2)
#define LED_DEFAULT_TINT				56
#define LED_MIN_SPEED					1
#define LED_MAX_SPEED					10
#define LED_REACTIVE_DEFAULT_SPEED		4
#define LED_REACTIVE_BUTTON_FADE_MS		300
#define LED_REACTIVE_TOUCH_POLL_TICKS	(TICKS_PER_SECOND / 25)
#define LED_SILK_TO_INDEX(n)			((n) - 1)


struct BadgeLedButtonZone {
	uint_fast16_t keyMask;
	uint8_t ledIdx;
};


static const struct BadgeLedButtonZone mButtonZones[] = {
	{KEY_BIT_UP | KEY_BIT_DOWN | KEY_BIT_LEFT | KEY_BIT_RIGHT, LED_SILK_TO_INDEX(5)},
	{KEY_BIT_A | KEY_BIT_B, LED_SILK_TO_INDEX(6)},
	{KEY_BIT_START | KEY_BIT_SEL, 6},
	{UI_KEY_BIT_CENTER, LED_SILK_TO_INDEX(8)},
};


static struct Settings mLedSettings;
static bool mHaveLedSettings;
static uint64_t mNextLedFrameTime, mLastGameLedWriteTime, mNextGameLedRefreshTime,
	mNextOverrideRefreshTime, mNextTouchPollTime, mTouchLastPressed;
static volatile uint32_t mDynamicLedDeadline;
static volatile bool mDynamicLedWatchdogArmed;
static uint64_t mButtonLastPressed[sizeof(mButtonZones) / sizeof(*mButtonZones)];
static bool mTouchSeen;
static bool mButtonSeen[sizeof(mButtonZones) / sizeof(*mButtonZones)];
static uint8_t mLedFrame;
static bool mIdle;


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
	return brightness > 255u ? 255u : brightness;
}

static bool badgeLedsPrvModeIsAnimated(uint_fast8_t mode)
{
	mode = badgeLedsPrvSanitizeMode(mode);
	if (mode == LedModeOff)
		return false;
	if (mode == LedModeSolid || mode == LedModeFlashlight || mode == LedModeFrontOn)
		return badgeLedsPrvSanitizeColor(mLedSettings.ledColor) != LedColorCustom;
	return true;
}

static void badgeLedsPrvArmDynamicWatchdog(void)
{
	mDynamicLedDeadline = (uint32_t)getTime() + LED_DYNAMIC_STALL_TICKS;
	mDynamicLedWatchdogArmed = true;
}

static void badgeLedsPrvDisarmDynamicWatchdog(void)
{
	mDynamicLedWatchdogArmed = false;
}

const char* badgeLedsModeName(uint_fast8_t mode)
{
	static const char names[][11] = {
		[LedModeOff] = "OFF",
		[LedModeSolid] = "ALL ON",
		[LedModeRainbow] = "RAINBOW",
		[LedModePulse] = "PULSE",
		[LedModeTravelingDot] = "DOT",
		[LedModeRandom] = "RANDOM",
		[LedModeFlashlight] = "REAR ON",
		[LedModeFrontOn] = "FRONT ON",
		[LedModeReactive] = "REACTIVE",
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

static uint64_t badgeLedsPrvScaledDurationTicks(uint32_t msec)
{
	uint_fast8_t speed = badgeLedsPrvFrameIntervalIdx(mLedSettings.ledSpeed);
	uint64_t ticks = (uint64_t)TICKS_PER_SECOND * msec * badgeLedsPrvFrameIntervalIdx(LED_REACTIVE_DEFAULT_SPEED);

	ticks /= (uint64_t)1000 * speed;
	return ticks ? ticks : 1;
}

static void badgeLedsPrvResetReactive(void)
{
	mNextTouchPollTime = 0;
	mTouchLastPressed = 0;
	mTouchSeen = false;
	memset(mButtonLastPressed, 0, sizeof(mButtonLastPressed));
	memset(mButtonSeen, 0, sizeof(mButtonSeen));
}

static void badgeLedsPrvPollReactiveTouch(uint64_t now)
{
	struct UiTouchSample touch;

	if (now < mNextTouchPollTime)
		return;

	mNextTouchPollTime = now + LED_REACTIVE_TOUCH_POLL_TICKS;
	if (!uiReadTouchRaw(&touch))
		return;

	if (touch.penDown) {
		mTouchLastPressed = now;
		mTouchSeen = true;
	}
}

static void badgeLedsPrvPollReactiveButtons(uint64_t now)
{
	uint_fast16_t keys = uiGetUiKeysRawNoTask();
	uint_fast8_t i;

	for (i = 0; i < sizeof(mButtonZones) / sizeof(*mButtonZones); i++) {
		if (keys & mButtonZones[i].keyMask) {
			mButtonLastPressed[i] = now;
			mButtonSeen[i] = true;
		}
	}
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

static void badgeLedsPrvRenderSet(const uint8_t *activeLeds, uint_fast8_t numActiveLeds)
{
	uint_fast8_t i;

	ws2812SetAllRgb(0, 0, 0);
	for (i = 0; i < numActiveLeds; i++) {
		uint8_t red, green, blue;

		badgeLedsPrvColor(activeLeds[i], (uint8_t)(mLedFrame * 5 + activeLeds[i] * 28), &red, &green, &blue, true);
		badgeLedsPrvSetRgb(activeLeds[i], red, green, blue);
	}
	ws2812refresh();
}

static void badgeLedsPrvRenderRearOn(void)
{
	static const uint8_t activeLeds[] = {1, 3, 7, 8};

	badgeLedsPrvRenderSet(activeLeds, sizeof(activeLeds) / sizeof(*activeLeds));
}

static void badgeLedsPrvRenderFrontOn(void)
{
	static const uint8_t activeLeds[] = {0, 2, 4, 5, 6};

	badgeLedsPrvRenderSet(activeLeds, sizeof(activeLeds) / sizeof(*activeLeds));
}

static void badgeLedsPrvMixReactiveScale(uint8_t ledScales[NUM_WS2812s], uint_fast8_t ledIdx, uint_fast8_t scale)
{
	if (ledIdx < NUM_WS2812s && scale > ledScales[ledIdx])
		ledScales[ledIdx] = scale;
}

static void badgeLedsPrvAddReactiveTouch(uint64_t now, uint8_t ledScales[NUM_WS2812s])
{
	uint64_t fadeTicks = badgeLedsPrvScaledDurationTicks(LED_REACTIVE_BUTTON_FADE_MS);
	uint64_t elapsed;
	uint_fast8_t scale;
	uint_fast8_t i;

	if (!mTouchSeen)
		return;

	elapsed = now - mTouchLastPressed;
	if (elapsed >= fadeTicks) {
		mTouchSeen = false;
		return;
	}

	scale = 255 - elapsed * 255 / fadeTicks;

	for (i = 0; i < NUM_WS2812s; i++)
		badgeLedsPrvMixReactiveScale(ledScales, i, scale);
}

static void badgeLedsPrvAddReactiveButtons(uint64_t now, uint8_t ledScales[NUM_WS2812s])
{
	uint64_t fadeTicks = badgeLedsPrvScaledDurationTicks(LED_REACTIVE_BUTTON_FADE_MS);
	uint_fast8_t i;

	for (i = 0; i < sizeof(mButtonZones) / sizeof(*mButtonZones); i++) {
		uint64_t elapsed;
		uint_fast8_t scale;

		if (!mButtonSeen[i])
			continue;

		elapsed = now - mButtonLastPressed[i];
		if (elapsed >= fadeTicks) {
			mButtonSeen[i] = false;
			continue;
		}

		scale = 255 - elapsed * 255 / fadeTicks;
		badgeLedsPrvMixReactiveScale(ledScales, mButtonZones[i].ledIdx, scale);
	}
}

static void badgeLedsPrvRenderReactive(void)
{
	uint64_t now = getTime();
	uint8_t ledScales[NUM_WS2812s] = {0};
	uint_fast8_t i;

	badgeLedsPrvAddReactiveTouch(now, ledScales);
	badgeLedsPrvAddReactiveButtons(now, ledScales);

	ws2812SetAllRgb(0, 0, 0);
	for (i = 0; i < NUM_WS2812s; i++) {
		uint8_t red, green, blue;

		if (!ledScales[i])
			continue;

		badgeLedsPrvColor(i, (uint8_t)(mLedFrame * 5 + i * 28), &red, &green, &blue, true);
		badgeLedsPrvSetRgb(i, badgeLedsPrvScale(red, ledScales[i]), badgeLedsPrvScale(green, ledScales[i]), badgeLedsPrvScale(blue, ledScales[i]));
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
			badgeLedsPrvRenderRearOn();
			break;

		case LedModeFrontOn:
			badgeLedsPrvRenderFrontOn();
			break;

		case LedModeReactive:
			badgeLedsPrvRenderReactive();
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
		badgeLedsPrvResetReactive();
		if (!mIdle) {
			badgeLedsPrvRenderCurrent();
			if (badgeLedsPrvModeIsAnimated(mLedSettings.ledMode))
				badgeLedsPrvArmDynamicWatchdog();
			else
				badgeLedsPrvDisarmDynamicWatchdog();
		}
	}
}

void badgeLedsSetIdle(bool idle)
{
	if (mIdle == idle)
		return;

	mIdle = idle;
	if (idle) {
		badgeLedsPrvDisarmDynamicWatchdog();
		ws2812SetAllRgb(0, 0, 0);
		ws2812refresh();
		return;
	}

	mLedFrame = 0;
	mNextLedFrameTime = getTime();
	badgeLedsPrvResetReactive();
	if (mHaveLedSettings)
		badgeLedsPrvRenderCurrent();
	if (mHaveLedSettings && badgeLedsPrvModeIsAnimated(mLedSettings.ledMode))
		badgeLedsPrvArmDynamicWatchdog();
}

void badgeLedsOverrideRgb(uint_fast8_t red, uint_fast8_t green, uint_fast8_t blue)
{
	uint64_t now = getTime();

	badgeLedsPrvArmDynamicWatchdog();
	if (now < mNextOverrideRefreshTime)
		return;
	mNextOverrideRefreshTime = now + LED_NON_SETTINGS_REFRESH_TICKS;
	ws2812SetAllRgb(red, green, blue);
	ws2812refresh();
}

void badgeLedsTick(void)
{
	uint64_t now;
	uint_fast8_t mode;

	if (!mHaveLedSettings || mIdle)
		return;

	mode = badgeLedsPrvSanitizeMode(mLedSettings.ledMode);
	if (!badgeLedsPrvModeIsAnimated(mode)) {
		badgeLedsPrvDisarmDynamicWatchdog();
		return;
	}

	now = getTime();
	badgeLedsPrvArmDynamicWatchdog();
	if (mLastGameLedWriteTime && now - mLastGameLedWriteTime < LED_GAME_WRITE_PAUSE_TICKS)
		return;

	if (mode == LedModeReactive) {
		badgeLedsPrvPollReactiveTouch(now);
		badgeLedsPrvPollReactiveButtons(now);
	}

	if (now < mNextLedFrameTime)
		return;

	mLedFrame++;
	mNextLedFrameTime = now + TICKS_PER_SECOND / badgeLedsPrvFrameIntervalIdx(mLedSettings.ledSpeed);
	badgeLedsPrvRenderCurrent();
}

void badgeLedsWatchdogTick(void)
{
	uint32_t now = (uint32_t)getTime();

	if (!mDynamicLedWatchdogArmed || (int32_t)(now - mDynamicLedDeadline) < 0)
		return;
	mDynamicLedWatchdogArmed = false;
	ws2812SetAllRgb(0, 0, 0);
	ws2812refresh();
}

void badgeLedsGameWrite(void)
{
	mLastGameLedWriteTime = getTime();
	badgeLedsPrvArmDynamicWatchdog();
}

bool badgeLedsGameRefresh(void)
{
	uint64_t now = getTime();

	mLastGameLedWriteTime = now;
	if (now < mNextGameLedRefreshTime)
		return false;
	mNextGameLedRefreshTime = now + LED_NON_SETTINGS_REFRESH_TICKS;
	badgeLedsPrvArmDynamicWatchdog();
	return true;
}
