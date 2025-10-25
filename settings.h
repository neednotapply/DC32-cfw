#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "pioWS2812.h"

struct SettingsLedColor {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

enum LedMode {
	LedModeManual = 0,
	LedModeAllOff = 1,
	LedModeCount,
};

enum RotationMode {
        RotationModeGame = 0,          // manual upright orientation for gameplay
        RotationModeBadge = 1,         // manual inverted orientation for badge wear
        RotationModeAuto = 2,
        RotationModeCount,
};

struct Settings {
	uint8_t actLikeGBC	:	1;
	uint8_t speed		:	2;
	uint8_t contrast	:	5;

	uint8_t upscale		:	1;
	uint8_t brightness	:	5;
	uint8_t ledMode	:	2;

	uint8_t rotationMode;
	uint8_t ledGlobalBrightness;
	struct SettingsLedColor ledColors[NUM_WS2812s];
};


void settingsGet(struct Settings *settings);
bool settingsSet(const struct Settings *settings);




#endif

