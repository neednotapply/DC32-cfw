#ifndef _UI_H_
#define _UI_H_



struct Canvas {
	void *framebuffer;
	uint32_t w, h;
	int8_t foreColor, backColor;		//negative means transparent
	uint8_t font;
	uint8_t bpp			: 5;
	uint8_t indexedLe	: 1;
	uint8_t rotated		: 1;			//90 degrees
	uint8_t flipped		: 1;			//180 degree rotation
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

void uiPreGame(void);
void uiInGame(void);


bool uiSaveSavestate(void);

//lower level, externally provided. just like gb key getting, except no call to uiInGame()
uint_fast8_t uiGetKeys(void);


void uiSelfTestInit(struct Canvas *cnv, bool inverted, bool flipped);
void uiSelfTestSetText(struct Canvas *cnv, unsigned row, unsigned col, const char *fmt, ...);
void uiSelfTestSetMarks(struct Canvas *cnv, uint8_t passMask, uint8_t failMask);

#endif

