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

enum ScreenSaver {
	ScreenSaverOff,
	ScreenSaverStarfield,
	ScreenSaverCube,
	ScreenSaverSpyro,
	ScreenSaverGif,
	ScreenSaverImageFolder,
	ScreenSaverDvdBounce,
	ScreenSaverScrollPattern,
	ScreenSaverNumModes,
};

#define SETTINGS_SCREENSAVER_PATH_MAX 64u

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
	uint8_t bootMenuCustomGif	:	1;	//legacy v6 migration field; custom boot GIF after v26

	uint8_t ledRed;
	uint8_t ledGreen;
	uint8_t ledBlue;
	uint8_t ledMode;
	uint8_t ledSpeed;
	uint8_t ledBrightness;
	uint8_t audioVolume		:	4;
	uint8_t musicLoopTrack	:	1;
	uint8_t ledColor		:	2;
	uint8_t audioMuted		:	1;

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
	uint8_t fileBrowserStartFavorites : 1;
	uint8_t pongColorTheme : 2;
	uint8_t tetrisMode : 2;
	uint8_t tetrisRule : 2;
	uint8_t portSettingsInitialized : 1;
	uint8_t themeEnabled : 1;
	uint8_t powerSaveEnabled;
	uint8_t powerSaveBrightness;
	uint8_t powerSaveTimeout;
	uint8_t screenSaver;
	uint8_t screenSaverTimeout;
	uint8_t screenSaverRotation;
	uint8_t screenSaverBrightness;
	char screenSaverGifPath[SETTINGS_SCREENSAVER_PATH_MAX];
	char screenSaverImageFolder[SETTINGS_SCREENSAVER_PATH_MAX];
};


void settingsGet(struct Settings *settings);
bool settingsSet(const struct Settings *settings);




#endif
