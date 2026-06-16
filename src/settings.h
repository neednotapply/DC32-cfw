#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

enum LedMode {
	LedModeOff,
	LedModeSolid,
	LedModeRainbow,
	LedModePulse,
	LedModeFlame = LedModePulse,
	LedModeTravelingDot,
	LedModeRandom,
	LedModeFlashlight,
	LedModeFrontOn,
	LedModeReactive,
	LedModeNumModes,
};

enum LedColor {
	LedColorCustom,
	LedColorRainbow,
	LedColorFlame,
	LedColorRandom,
	LedColorNumColors,
};

enum AutoclickerButton {
	AutoclickerButtonLeft,
	AutoclickerButtonRight,
	AutoclickerButtonMiddle,
	AutoclickerButtonNumButtons,
};

enum GameBoyPalette {
	GameBoyPaletteBw,
	GameBoyPaletteDmg,
	GameBoyPaletteGbpocket,
	GameBoyPaletteBgb,
	GameBoyPaletteGbli,
	GameBoyPaletteGrafixkidgray,
	GameBoyPaletteGrafixkidgreen,
	GameBoyPaletteBlackzero,
	GameBoyPaletteGbcjp,
	GameBoyPaletteGbcu,
	GameBoyPaletteGbcua,
	GameBoyPaletteGbcub,
	GameBoyPaletteGbcl,
	GameBoyPaletteGbcla,
	GameBoyPaletteGbclb,
	GameBoyPaletteGbcd,
	GameBoyPaletteGbcda,
	GameBoyPaletteGbcdb,
	GameBoyPaletteGbcr,
	GameBoyPaletteGbceuus,
	GameBoyPaletteGbcrb,
	GameBoyPaletteGbcPreferred,
	GameBoyPaletteNumPalettes,
};

struct Settings {
	uint8_t actLikeGBC	:	1;
	uint8_t speed		:	2;
	uint8_t contrast	:	5;

	uint8_t upscale		:	1;
	uint8_t rotation	:	1;
	uint8_t brightness	:	5;
	uint8_t ledsEnabled	:	1;	//legacy v6 migration field

	uint8_t ledRed;
	uint8_t ledGreen;
	uint8_t ledBlue;
	uint8_t ledMode;
	uint8_t ledSpeed;
	uint8_t ledBrightness;
	uint8_t musicVolume		:	4;
	uint8_t musicLoopTrack	:	1;
	uint8_t ledColor		:	2;
	uint8_t reserved		:	1;

	uint16_t badUsbVid;
	uint16_t badUsbPid;
	char badUsbManufacturer[32];
	char badUsbProduct[32];
	uint8_t autoclickerButton	:	2;
	uint8_t autoclickerCps		:	6;
	uint8_t gbPalette;
	uint8_t gbSpeed		:	2;
	uint8_t gbUpscale	:	1;
	uint8_t gbcSpeed	:	2;
	uint8_t gbcUpscale	:	1;
	uint8_t nesSpeed	:	2;
	uint8_t nesUpscale	:	1;
	uint8_t emulatorReserved : 7;
};


void settingsGet(struct Settings *settings);
bool settingsSet(const struct Settings *settings);




#endif
