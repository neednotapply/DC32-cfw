#include <string.h>
#include "settings.h"
#include "memMap.h"
#include "printf.h"
#include "qspi.h"



#define SETTINGS_MAGIC				0x4447687a
#define SETTINGS_CUR_VER			26
#define SETTINGS_NUM_SPEEDS			4
#define SETTINGS_LED_MIN_SPEED		1
#define SETTINGS_LED_MAX_SPEED		10
#define SETTINGS_AUDIO_VOLUME_MAX	15
#define SETTINGS_LED_MODE_REACTIVE_BUTTONS_V13	9
#define SETTINGS_BADUSB_DEFAULT_VID	0x1209
#define SETTINGS_BADUSB_DEFAULT_PID	0xdc32
#define SETTINGS_AUTOCLICKER_MIN_CPS	1
#define SETTINGS_AUTOCLICKER_MAX_CPS	50


union SettingsPage {
	struct {
		uint64_t generation;
		uint32_t magic;
		uint32_t version;
		struct Settings settings;
	};
	uint8_t space[QSPI_WRITE_GRANULARITY];
};

typedef char SettingsPageSizeCheck[
	(sizeof(union SettingsPage) == QSPI_WRITE_GRANULARITY) ? 1 : -1];


static const union SettingsPage* settingsPrvFirstPage(void)
{
	return (const union SettingsPage*)QSPI_SETTINGS_START;
}

static bool settingsPrvIsPastLastPage(const union SettingsPage* sp)
{
	return ((uintptr_t)sp) == QSPI_SETTINGS_START + QSPI_SETTINGS_LEN;
}

static bool settingsPrvIsFirstPageInBlock(const union SettingsPage* sp)
{
	return !((((uintptr_t)sp) - QSPI_SETTINGS_START) % QSPI_ERASE_GRANULARITY);
}

static const union SettingsPage* settingsLocate(void)
{
	const union SettingsPage *sp, *best = NULL;
	
	if (sizeof(union SettingsPage) != QSPI_WRITE_GRANULARITY)
		return NULL;
	
	for (sp = settingsPrvFirstPage(); !settingsPrvIsPastLastPage(sp); sp++) {
		
		if (sp->magic != SETTINGS_MAGIC || sp->version > SETTINGS_CUR_VER)
			continue;
				
		if (!best || best->generation < sp->generation)
			best = sp;
	}
	
	return best;
}

static void settingsPrvNormalize(struct Settings *settings)
{
	settings->actLikeGBC = 1;
	if (settings->speed >= SETTINGS_NUM_SPEEDS)
		settings->speed = 1;
	if (settings->gbSpeed >= SETTINGS_NUM_SPEEDS)
		settings->gbSpeed = settings->speed;
	if (settings->gbcSpeed >= SETTINGS_NUM_SPEEDS)
		settings->gbcSpeed = settings->speed;
	if (settings->nesSpeed >= SETTINGS_NUM_SPEEDS)
		settings->nesSpeed = settings->speed;
	if (settings->ledMode == SETTINGS_LED_MODE_REACTIVE_BUTTONS_V13)
		settings->ledMode = LedModeReactive;
	else if (settings->ledMode >= LedModeNumModes)
		settings->ledMode = LedModeOff;
	if (settings->ledColor >= LedColorNumColors)
		settings->ledColor = LedColorCustom;
	if (settings->ledSpeed < SETTINGS_LED_MIN_SPEED || settings->ledSpeed > SETTINGS_LED_MAX_SPEED)
		settings->ledSpeed = 4;
	if (settings->audioVolume > SETTINGS_AUDIO_VOLUME_MAX)
		settings->audioVolume = 7;
	if (!settings->badUsbVid)
		settings->badUsbVid = SETTINGS_BADUSB_DEFAULT_VID;
	if (!settings->badUsbPid)
		settings->badUsbPid = SETTINGS_BADUSB_DEFAULT_PID;
	settings->badUsbManufacturer[sizeof(settings->badUsbManufacturer) - 1] = 0;
	settings->badUsbProduct[sizeof(settings->badUsbProduct) - 1] = 0;
	if (!settings->badUsbManufacturer[0] || (uint8_t)settings->badUsbManufacturer[0] == 0xff)
		strcpy(settings->badUsbManufacturer, "DC32");
	if (!settings->badUsbProduct[0] || (uint8_t)settings->badUsbProduct[0] == 0xff)
		strcpy(settings->badUsbProduct, "DC32 BadUSB");
	if (settings->autoclickerButton >= AutoclickerButtonNumButtons)
		settings->autoclickerButton = AutoclickerButtonLeft;
	if (settings->autoclickerCps < SETTINGS_AUTOCLICKER_MIN_CPS || settings->autoclickerCps > SETTINGS_AUTOCLICKER_MAX_CPS)
		settings->autoclickerCps = 5;
	if (settings->gbPalette >= GameBoyPaletteNumPalettes)
		settings->gbPalette = GameBoyPaletteBw;
	if (settings->powerSaveBrightness > 0x1fu)
		settings->powerSaveBrightness = 1u;
	if (settings->powerSaveTimeout < 5u || settings->powerSaveTimeout > 240u)
		settings->powerSaveTimeout = 30u;
	if (settings->screenSaver >= ScreenSaverNumModes)
		settings->screenSaver = ScreenSaverStarfield;
	if (settings->screenSaverTimeout < 10u || settings->screenSaverTimeout > 240u)
		settings->screenSaverTimeout = 60u;
	settings->screenSaverRotation = !!settings->screenSaverRotation;
	if (settings->screenSaverBrightness > 0x1fu)
		settings->screenSaverBrightness = 1u;
	settings->screenSaverGifPath[sizeof(settings->screenSaverGifPath) - 1] = 0;
	settings->screenSaverImageFolder[sizeof(settings->screenSaverImageFolder) - 1] = 0;
	settings->bootMenuCustomGif = false;
	settings->themeEnabled = !!settings->themeEnabled;
	settings->fileBrowserStartFavorites = !!settings->fileBrowserStartFavorites;
	if (settings->pongColorTheme >= 3u)
		settings->pongColorTheme = 0;
	if (settings->tetrisMode >= 3u)
		settings->tetrisMode = 0;
	if (settings->tetrisRule >= 3u)
		settings->tetrisRule = 0;
	settings->portSettingsInitialized = !!settings->portSettingsInitialized;
}

void settingsGet(struct Settings *settings)
{
	const union SettingsPage* sp = settingsLocate();
	uint32_t curVer;
	
	if (sp) {
		
		*settings = sp->settings;
		curVer = sp->version;
	}
	else {

		curVer = 0;
	}
	switch (curVer) {
		
		case 0:				//upgrade settings structure from nothing
			settings->actLikeGBC = 1;
			//fallthrough
		
		case 1:				//upgrade structure from v1
			settings->speed = 2;
			//fallthrough
		
		case 2:				//upgrade from v2
			settings->contrast = 14;
			//fallthrough
		
		case 3:				//upgrade from v3
			settings->upscale = true;
			//fallthrough

		case 4:				//upgrade from v4
			settings->brightness = 15;
			settings->speed = 1;	//yes, reset user preference
			//fallthrough

		case 5:				//upgrade from v5
			settings->bootMenuCustomGif = false;
			settings->ledRed = 56;
			settings->ledGreen = 56;
			settings->ledBlue = 56;
			//fallthrough

		case 6:				//upgrade from v6
			settings->ledMode = settings->bootMenuCustomGif ? LedModeSolid : LedModeOff;
			settings->ledSpeed = 2;
			//fallthrough

		case 7:				//upgrade from v7
			settings->ledBrightness = 255;
			//fallthrough

		case 8:				//upgrade from v8
			settings->rotation = 0;
			//fallthrough

		case 9:				//upgrade from v9
			settings->audioVolume = 7;
			settings->musicLoopTrack = false;
			//fallthrough

		case 10:			//upgrade music volume from 0-10 scale to 0-15 scale
			settings->audioVolume = (settings->audioVolume * 15 + 5) / 10;
			//fallthrough

		case 11:			//upgrade LED speed from 1-4 scale to 1-10 scale
			if (settings->ledSpeed <= 1)
				settings->ledSpeed = 1;
			else if (settings->ledSpeed == 2)
				settings->ledSpeed = 4;
			else if (settings->ledSpeed == 3)
				settings->ledSpeed = 7;
			else
				settings->ledSpeed = 10;
			//fallthrough

		case 12:			//split LED pattern from LED color
			if (settings->ledMode == LedModeRainbow)
				settings->ledColor = LedColorRainbow;
			else if (settings->ledMode == LedModeFlame) {
				settings->ledMode = LedModePulse;
				settings->ledColor = LedColorFlame;
			}
			else
				settings->ledColor = LedColorCustom;
			//fallthrough

		case 13:			//merge reactive touch and button LED patterns
			if (settings->ledMode == SETTINGS_LED_MODE_REACTIVE_BUTTONS_V13)
				settings->ledMode = LedModeReactive;
			//fallthrough

		case 14:			//add USB HID defaults and Autoclicker settings
			settings->badUsbVid = SETTINGS_BADUSB_DEFAULT_VID;
			settings->badUsbPid = SETTINGS_BADUSB_DEFAULT_PID;
			strcpy(settings->badUsbManufacturer, "DC32");
			strcpy(settings->badUsbProduct, "DC32 BadUSB");
			settings->autoclickerButton = AutoclickerButtonLeft;
			settings->autoclickerCps = 5;
			//fallthrough

		case 15:			//add Game Boy palette selection
			settings->gbPalette = GameBoyPaletteGbcPreferred;
			//fallthrough

		case 16:			//split shared emulator display settings by console
			settings->actLikeGBC = 1;
			settings->gbSpeed = settings->speed;
			settings->gbUpscale = settings->upscale;
			settings->gbcSpeed = settings->speed;
			settings->gbcUpscale = settings->upscale;
			settings->nesSpeed = settings->speed;
			settings->nesUpscale = settings->upscale;
			//fallthrough

		case 17:			//add universal audio mute
			settings->audioMuted = false;
			//fallthrough

		case 18:			//add display power saving and screensaver defaults
			settings->powerSaveEnabled = true;
			settings->powerSaveBrightness = 1u;
			settings->powerSaveTimeout = 30u;
			settings->screenSaver = ScreenSaverStarfield;
			settings->screenSaverTimeout = 60u;
			//fallthrough

		case 19:			//add selected screensaver media locations
			settings->screenSaverGifPath[0] = 0;
			settings->screenSaverImageFolder[0] = 0;
			//fallthrough

		case 20:			//add independent screensaver rotation
			settings->screenSaverRotation = false;
			//fallthrough

		case 21:			//add separate low-power screensaver brightness
			settings->screenSaverBrightness = 1u;
			//fallthrough

		case 22:			//add File Browser start location
			settings->fileBrowserStartFavorites = false;
			//fallthrough

		case 23:			//add persistent port settings
			settings->pongColorTheme = 0;
			settings->tetrisMode = 0;
			settings->tetrisRule = 0;
			settings->portSettingsInitialized = false;
			//fallthrough

		case 24:
		case 25:			//add shared theme choice without growing the settings page
			settings->themeEnabled = false;
			settings->bootMenuCustomGif = false;
			//fallthrough

		//other cases here, in increasing order
	}

	settingsPrvNormalize(settings);
}

bool settingsSet(const struct Settings *settings)
{
	const union SettingsPage *sp = settingsLocate();
	union SettingsPage msp;
	uint32_t numTries = 64;
	
	msp.magic = SETTINGS_MAGIC;
	msp.version = SETTINGS_CUR_VER;
	msp.settings = *settings;
	
	if (!sp) {
		msp.generation = 0;
		sp = settingsPrvFirstPage();
	}
	else {
		msp.generation = sp->generation + 1;
	}
	
	while (1) {
		
		if (settingsPrvIsPastLastPage(++sp))
			sp = settingsPrvFirstPage();
		
		if (!numTries--)
			return false;
				
		if (!flashWrite((uintptr_t)sp, settingsPrvIsFirstPageInBlock(sp) ? QSPI_ERASE_GRANULARITY : 0, &msp, sizeof(union SettingsPage)))
			continue;
		
		if (memcmp(sp, &msp, sizeof(union SettingsPage))) {
			
			//invalidate it
			msp.magic ^= 0x55555555;
			(void)flashWrite((uintptr_t)sp, 0, &msp, sizeof(union SettingsPage));
			msp.magic ^= 0x55555555;
			
			continue;
		}
		
		return true;
	}
}
