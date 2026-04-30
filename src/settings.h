#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

enum LedMode {
	LedModeOff,
	LedModeSolid,
	LedModeRainbow,
	LedModeFlame,
	LedModeTravelingDot,
	LedModeNumModes,
};

struct Settings {
	uint8_t actLikeGBC	:	1;
	uint8_t speed		:	2;
	uint8_t contrast	:	5;

	uint8_t upscale		:	1;
	uint8_t brightness	:	5;
	uint8_t ledsEnabled	:	1;	//legacy v6 migration field

	uint8_t ledRed;
	uint8_t ledGreen;
	uint8_t ledBlue;
	uint8_t ledMode;
	uint8_t ledSpeed;
};


void settingsGet(struct Settings *settings);
bool settingsSet(const struct Settings *settings);




#endif

