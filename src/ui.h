#ifndef _UI_H_
#define _UI_H_

#include <stdbool.h>
#include <stdint.h>



struct Settings;

struct Canvas {
        void *framebuffer;
        uint32_t w, h;
        int8_t foreColor, backColor;            //negative means transparent
        uint8_t font;
        uint8_t bpp                     : 5;
        uint8_t indexedLe       : 1;
        uint8_t rotated         : 1;                    //90 degrees
        uint8_t flipped         : 1;                    //180 degree rotation
};

#ifdef DISP_OLED
        #include "oled.h"
#endif

#ifdef DISP_LCD_WAVESHARE
        #include "dispWaveshareLcd.h"
#endif

#ifdef DISP_LCD_DEFCON
        #include "dispDefcon.h"
#endif

enum UiGameAction {
	UiGameActionResume,
	UiGameActionRestart,
	UiGameActionSelectGame,
	UiGameActionSwitchTool,
};

enum GameRuntime {
	GameRuntimeNone,
	GameRuntimeGb,
	GameRuntimeNes,
	GameRuntimeArduboy,
};

#define UI_FN_SETTINGS_MAX 8u

struct UiFnSettings {
	const char *title;
	const char *menuLabel;
	uint8_t count;
	const char *const *labels;
	void (*value)(void *context, uint8_t index, char *dst, uint32_t dstSize);
	void (*adjust)(void *context, uint8_t index, int8_t direction);
	bool directOpen;
};

#define UI_KEY_BIT_CENTER	0x100u

struct UiTouchSample {
	uint16_t x;
	uint16_t y;
	uint16_t z;
	bool penDown;
};

struct GameSelection {
	enum GameRuntime runtime;
	uint32_t romSize;
	uint32_t saveRamSize;
};

typedef void (*UiRunGameF)(void *userData);

void uiRunToolShell(UiRunGameF runGameF, void *userData);
enum UiGameAction uiGameMenu(void);
bool uiPortMenu(struct Canvas *activeCanvas);
void uiSetFnSettings(const struct UiFnSettings *settings, void *context);


bool uiSaveSavestate(void);
bool uiFlushCurrentSaveToCard(bool force);
bool uiGetGameSelection(struct GameSelection *selectionP);

//lower level, externally provided. debounced Game Boy key state for menu flows
uint_fast8_t uiGetKeys(void);

//single-sample non-debounced read — safe to call inside tight timing loops
uint_fast8_t uiGetKeysRaw(void);

//UI-only key state, including the badge FN button above the Game Boy key bits.
uint_fast16_t uiGetUiKeys(void);
uint_fast16_t uiGetUiKeysRaw(void);
uint_fast16_t uiGetUiKeysRawNoTask(void);

bool uiReadTouchRaw(struct UiTouchSample *sampleP);
void uiPowerSetActiveBrightness(uint_fast8_t brightness);
void uiPowerApplySettings(const struct Settings *settings);
bool uiRunScreensaverMedia(uint8_t saver);
bool uiRunScreensaverImageEffect(uint8_t saver);
bool uiPowerScreenSaverWoke(void);
bool uiPowerConsumeScreenSaverWake(void);
void uiPowerSetIdleInhibited(bool inhibited);
void uiPowerSetScreenSaverContentActive(bool active);


void uiSelfTestInit(struct Canvas *cnv, bool inverted, bool flipped);
void uiSelfTestSetText(struct Canvas *cnv, unsigned row, unsigned col, const char *fmt, ...);
void uiSelfTestSetMarks(struct Canvas *cnv, uint8_t passMask, uint8_t failMask);

#endif
