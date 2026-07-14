#pragma GCC optimize ("Os")
#include <stdarg.h>
#include <string.h>
#include "gbCartHeader.h"
#include "settings.h"
#include "badgeLeds.h"
#include "badgePower.h"
#include "badgeRtc.h"
#include "bootGuard.h"
#include "dcApp.h"
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "memMap.h"
#include "printf.h"
#include "sleep.h"
#include "fonts.h"
#include "fatfs.h"
#include "qspi.h"
#include "cpu.h"
#include "mbc.h"
#include "irRemote.h"
#include "badUsb.h"
#include "usbHid.h"
#include "usbMsc.h"
#include "usbXinput.h"
#include "musicPlayer.h"
#include "abcPlayer.h"
#include "rtttlPlayer.h"
#include "midiPlayer.h"
#include "audioPwm.h"
#include "imageViewer.h"
#include "timebase.h"
#include "toolWorkspace.h"
#include "utf.h"
#include "ui.h"
#include "sd.h"
#include "gb.h"
#include "nes/nes.h"
#include "arduboy/arduboy.h"

#define MENU_SELECTION_CHAR				0xBB /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK */
#define UI_GAME_TITLE_BUF_SZ			64
#define UI_KEY_REPEAT_INITIAL_TICKS		((uint64_t)TICKS_PER_SECOND * 300u / 1000u)
#define UI_KEY_REPEAT_INTERVAL_TICKS	((uint64_t)TICKS_PER_SECOND * 80u / 1000u)
#define UI_HEADER_TEXT_GAP				3u
#define UI_HEADER_EDGE_PAD				5u
#define UI_HEADER_TITLE_LEFT			10u
#define UI_HEADER_BATTERY_BODY_WIDTH	34u
#define UI_HEADER_BATTERY_CAP_WIDTH	3u
#define UI_HEADER_BATTERY_WIDTH		(UI_HEADER_BATTERY_BODY_WIDTH + UI_HEADER_BATTERY_CAP_WIDTH)
#define UI_HEADER_BATTERY_GREEN		0x05a0u
#define UI_HEADER_BATTERY_AMBER		0xbde0u
#define UI_HEADER_BATTERY_RED			0xb104u
#define UI_HEADER_BATTERY_BACKGROUND	0x0000u
#define UI_HEADER_BATTERY_BOLT		0xffe0u
#define UI_SAVER_IMAGE_WIDTH			90u
#define UI_SAVER_IMAGE_HEIGHT			120u
#define UI_SAVER_IMAGE_BYTES			(UI_SAVER_IMAGE_WIDTH * UI_SAVER_IMAGE_HEIGHT * sizeof(uint16_t))

#ifdef UI_ROTATED
	#undef UI_ROTATED
	#define UI_ROTATED 1
#else
	#define UI_ROTATED	0
#endif


#define CANVAS_INITIALIZER				{.framebuffer = dispGetFb(), .w = UI_ROTATED ? DISP_HEIGHT : DISP_WIDTH, .h = UI_ROTATED ? DISP_WIDTH : DISP_HEIGHT, .bpp = DISP_BPP, .indexedLe = DISP_INDEXED_LE, .rotated = UI_ROTATED, }



struct ScreenPrintfData {
	struct Canvas *cnv;
	bool needsPreSpace;
	int32_t r, c;
	struct Utf8state u8s;
};

struct RomOption {
	struct RomOption *prev, *next;
	char name[];
} __attribute__((packed));

struct MusicOption {
	struct MusicOption *prev, *next;
	struct FatFileLocator locator;
	uint32_t size;
	uint8_t isDir;
	char name[];
};

enum MusicPlaybackControlUi {
	MusicPlaybackControlPrev,
	MusicPlaybackControlPlay,
	MusicPlaybackControlNext,
	MusicPlaybackControlLoop,
	MusicPlaybackControlTrack,
	MusicPlaybackControlVol,
	MusicPlaybackControlNum,
};

enum CurOp {
	CurentlyIdle,
	CurrentlyReading,
	CurrentlyWriting,	
};

static enum CurOp mCurCardOp = CurentlyIdle;
static uint32_t mCurCardSec;
static bool mToolExitEnabled;
static bool mToolExitRequested;
static const struct UiFnSettings *mUiFnSettings;
static void *mUiFnSettingsContext;
static const char mUiAppTitle[] = "DC32-cfw";
static const char *mUiHeaderTitle = "Main Menu";
static bool mUiHeaderInverted;
static uint16_t mUiThemeColor = 0x9cf3u;
static uint32_t mUiHeaderClockMinute = 0xffffffffu;
static uint8_t mUiHeaderBattLevel;
static bool mUiHeaderBattValid;
static enum BadgePowerMode mUiHeaderPowerMode;
static uint_fast16_t mUiRepeatKey;
static uint64_t mUiRepeatNextTicks;
static bool mUiKeyRepeated;

enum UiEmulatorConsole {
	UiEmulatorConsoleArduboy,
	UiEmulatorConsoleGameboy,
	UiEmulatorConsoleGameboyColor,
	UiEmulatorConsoleNes,
	UiEmulatorConsoleNum,
};

static enum UiEmulatorConsole uiPrvCurrentGameConsole(void);

static const char *uiPrvEmulatorConsoleName(enum UiEmulatorConsole console)
{
	switch (console) {
		case UiEmulatorConsoleArduboy:
			return "Arduboy";
		case UiEmulatorConsoleGameboy:
			return "Gameboy";
		case UiEmulatorConsoleGameboyColor:
			return "Gameboy Color";
		case UiEmulatorConsoleNes:
			return "Nintendo Entertainment System";
		default:
			return "Emulator";
	}
}

static const char *uiPrvEmulatorSettingsName(enum UiEmulatorConsole console)
{
	switch (console) {
		case UiEmulatorConsoleArduboy:
			return "Arduboy Settings";
		case UiEmulatorConsoleGameboy:
			return "Gameboy Settings";
		case UiEmulatorConsoleGameboyColor:
			return "Gameboy Color Settings";
		case UiEmulatorConsoleNes:
			return "NES Settings";
		default:
			return "Emulator Settings";
	}
}

static const char *uiPrvNesRegionName(enum NesRegion region)
{
	switch (region) {
		case NesRegionPal:
			return "PAL";
		case NesRegionDendy:
			return "DENDY";
		case NesRegionNtsc:
		default:
			return "NTSC";
	}
}


enum DialogType {
	DialogTypeOk,
	DialogTypeYesNo,
};


struct BitBufferR {
	const uint8_t *src;

	uint8_t bitBuf;
	uint8_t numBitsHere;
};

#if DISP_BPP > 8
	
	static uint_fast16_t uiPrvGreyToColor(uint_fast8_t grey)
	{
		uint_fast16_t ret = grey & 0x0f;
		uint_fast16_t ret5 = ret * 2 + ret / 8;
		uint_fast16_t ret6 = ret * 4 + ret / 4;

		if (ret == 9u)
			return mUiThemeColor;
		return (ret5 << 11) + (ret6 << 5) + ret5;
	}

#endif


static uint_fast8_t uiPrvGlyphHeight(const struct Canvas *cnv)
{
	return fontGetHeight((enum Font)cnv->font);
}

static uint_fast8_t uiPrvContentTop(struct Canvas *cnv)
{
	(void)cnv;
	return fontGetHeight(FontLarge) + fontGetHeight(FontMedium) / 4;
}

static uint_fast8_t uiPrvMenuItemHeight(struct Canvas *cnv)
{
	return uiPrvGlyphHeight(cnv) + 1;
}

static uint_fast8_t uiPrvMenuRow(struct Canvas *cnv, uint_fast8_t idx)
{
	return uiPrvContentTop(cnv) + idx * uiPrvMenuItemHeight(cnv);
}

static bool uiPrvImageFileName(const char *name);

static void uiPrvSetHeaderTitle(const char *title)
{
	mUiHeaderTitle = title ? title : mUiAppTitle;
}

static uint_fast8_t uiPrvDrawOneChar(struct Canvas *cnv, int32_t r, int32_t c, wchar_t chr)	//return char width
{
	uint_fast8_t glyphHeight = uiPrvGlyphHeight(cnv), dr, dc;
	uint16_t *fb16 = cnv->framebuffer;
	uint8_t *fb8 = cnv->framebuffer;
	struct FontGlyphInfo gi;


	if (r <= -(signed)glyphHeight || r >= (int32_t)cnv->h || c >= (int32_t)cnv->w)
		return 0;
	
	if (!fontGetGlyphInfo(&gi, (enum Font)cnv->font, chr))
		return 0;
	
	if (c + gi.width <= 0)
		return 0;

	for (dr = 0; dr < gi.height; dr++) {
		for (dc = 0; dc < gi.width; dc++) {
			
			int32_t er = r + (int_fast16_t)(uint_fast16_t)dr, ec = c + (int_fast16_t)(uint_fast16_t)dc;
			bool on = fontGetGlyphPixel(&gi, dr, dc);
			int8_t color = on ? cnv->foreColor : cnv->backColor; 
			uint32_t ep;

			if (cnv->flipped)
				ep = cnv->rotated ? (cnv->w - 1 - ec) * cnv->h + er : (cnv->h - 1 - er) * cnv->w + (cnv->w - 1 - ec);
			else
				ep = cnv->rotated ? ec * cnv->h + cnv->h - 1 - er : er * cnv->w + ec;
			
			if (er >= 0 && (uint32_t)er < cnv->h && ec >= 0 && (uint32_t)ec < cnv->w && color >= 0) {
				
				#if DISP_BPP <= 8
					uint32_t mask = (1 << cnv->bpp) - 1, shift = ep * cnv->bpp % 8;
					uint8_t *pixByte = &fb8[ep * cnv->bpp / 8];
					
					if (!cnv->indexedLe)
						shift = 8 - cnv->bpp - shift;
					
					*pixByte = ((*pixByte) &~ (mask << shift)) + (color << shift);
				#else
					
					fb16[ep] = uiPrvGreyToColor(color);
					
				#endif
			}
			
		}
	}
	
	return gi.width;
}

#if DISP_BPP > 8

	static uint_fast8_t uiPrvDrawOneCharRgb565(struct Canvas *cnv, int32_t r, int32_t c, wchar_t chr, uint16_t pxColor)
	{
		uint_fast8_t glyphHeight = uiPrvGlyphHeight(cnv), dr, dc;
		uint16_t *fb16 = cnv->framebuffer;
		struct FontGlyphInfo gi;

		if (r <= -(signed)glyphHeight || r >= (int32_t)cnv->h || c >= (int32_t)cnv->w)
			return 0;
		if (!fontGetGlyphInfo(&gi, (enum Font)cnv->font, chr))
			return 0;
		if (c + gi.width <= 0)
			return 0;

		for (dr = 0; dr < gi.height; dr++) {
			for (dc = 0; dc < gi.width; dc++) {
				int32_t er = r + (int_fast16_t)(uint_fast16_t)dr, ec = c + (int_fast16_t)(uint_fast16_t)dc;
				uint32_t ep;

				if (!fontGetGlyphPixel(&gi, dr, dc))
					continue;

				if (cnv->flipped)
					ep = cnv->rotated ? (cnv->w - 1 - ec) * cnv->h + er : (cnv->h - 1 - er) * cnv->w + (cnv->w - 1 - ec);
				else
					ep = cnv->rotated ? ec * cnv->h + cnv->h - 1 - er : er * cnv->w + ec;

				if (er >= 0 && (uint32_t)er < cnv->h && ec >= 0 && (uint32_t)ec < cnv->w)
					fb16[ep] = pxColor;
			}
		}

		return gi.width;
	}

	static uint32_t uiPrvPutsRgb565(struct Canvas *cnv, int32_t r, int32_t c, const char *str, uint16_t pxColor)
	{
		int32_t startC = c;
		struct FontGlyphInfo gi;
		bool needsPreSpace = false;
		char ch;

		while ((ch = *str++) != 0) {
			if (needsPreSpace && fontGetGlyphInfo(&gi, (enum Font)cnv->font, 0x2009))
				c += uiPrvDrawOneCharRgb565(cnv, r, c, 0x2009, pxColor);
			needsPreSpace = ch != ' ';
			c += uiPrvDrawOneCharRgb565(cnv, r, c, (unsigned char)ch, pxColor);
		}

		return c - startC;
	}

#endif

static uint32_t uiPrvCharsMeasure(struct Canvas *cnv, const char *chars, uint32_t len, uint32_t maxWidth, uint32_t *numCharsDrawnP)	//returns width used
{
	struct Utf8state u8s = UTF8_STATE_STATIC_INITIALIZER;
	bool needPreSpace = false;
	struct FontGlyphInfo gi;
	uint32_t i, w = 0;
	wchar_t ch;
	
	for (i = 0; i < len; i++) {
		
		uint32_t ret, prevI;
		uint32_t chrWidth, nowWidth = 0;
		
		if (needPreSpace) {		//inter-char spacing
			

			if (fontGetGlyphInfo(&gi, (enum Font)cnv->font, 0x2009))
				nowWidth += gi.width;
		}

		prevI = i;
		while ((ret = utf8inputByte(&u8s, (unsigned char)*chars++)) == UTF_NO_OUTPUT && i < len)
			i++;

		if (ret == UTF_NO_OUTPUT || ret == UTF_ERROR || ret == 0) {
			
			i = prevI;
			break;
		}
		
		ch = ret;
		
		needPreSpace = ch != ' ';
		chrWidth = fontGetGlyphInfo(&gi, (enum Font)cnv->font, ch) ? gi.width : 0;
		nowWidth += chrWidth;
		
		if (w + nowWidth > maxWidth)
			break;
		w += nowWidth;
	}
	
	if (numCharsDrawnP)
		*numCharsDrawnP = i;
	
	return w;
}

static uint32_t uiPrvCharsWidth(struct Canvas *cnv, const char *chars, uint32_t len)
{
	return uiPrvCharsMeasure(cnv, chars, len, 0xffffffff, NULL);
}

static uint32_t uiPrvCharsFit(struct Canvas *cnv, const char *chars, uint32_t len, uint32_t maxWidth)
{
	uint32_t numCharsDrawn;
	
	(void)uiPrvCharsMeasure(cnv, chars, len, maxWidth, &numCharsDrawn);
	
	return numCharsDrawn;
}

static void uiPrvPrintCbk(void *userData, char chr)
{
	struct ScreenPrintfData *spd = (struct ScreenPrintfData*)userData;
	uint32_t ret = utf8inputByte(&spd->u8s, chr);

	if (ret == UTF_ERROR || ret == UTF_NO_OUTPUT)
		return;
	
	if (spd->needsPreSpace)
		spd->c += uiPrvDrawOneChar(spd->cnv, spd->r, spd->c, 0x2009);	//thin space
	spd->needsPreSpace = ret != ' ';
	spd->c += uiPrvDrawOneChar(spd->cnv, spd->r, spd->c, ret);
}

static uint32_t uiPuts(struct Canvas *cnv, int32_t r, int32_t c, const char *str, uint32_t maxLen)		//returns num columns used
{
	struct ScreenPrintfData spd = {.cnv = cnv, .r = r, .c = c, .u8s = UTF8_STATE_STATIC_INITIALIZER};
	uint32_t nDrawn = 0;
	wchar_t ch;
	
	while (maxLen-- && (ch = (unsigned char)*str++) != 0) {
		
		nDrawn++;
		uiPrvPrintCbk(&spd, ch);
	}
	
	return spd.c - c;
}

static void uiVprintf(struct Canvas *cnv, int32_t r, int32_t c, const char *format, va_list vl)
{
	struct ScreenPrintfData spd = {.cnv = cnv, .r = r, .c = c, .u8s = UTF8_STATE_STATIC_INITIALIZER};

	(void)vxprintf(&spd, uiPrvPrintCbk, format, vl);
}

static void uiPrintf(struct Canvas *cnv, int32_t r, int32_t c, const char *format, ...)
{
	struct ScreenPrintfData spd = {.cnv = cnv, .r = r, .c = c, .u8s = UTF8_STATE_STATIC_INITIALIZER};
	va_list vl;
	
	va_start(vl, format);
	(void)vxprintf(&spd, uiPrvPrintCbk, format, vl);
	va_end(vl);
}

#if DISP_BPP <= 8

	static void uiPrvFillRect(struct Canvas *cnv, int32_t left, int32_t top, int32_t right, int32_t bottom)
	{
		uint32_t w, h, r, c, pixPerByte = 8 / cnv->bpp, rowItems = cnv->rotated ? cnv->h : cnv->w;
		uint8_t *fb, *fbRow = cnv->framebuffer;
		
		if (cnv->flipped) {
			int32_t t;

			t = left;
			left = cnv->w - 1 - right;
			right = cnv->w - 1 - t;

			t = top;
			top = cnv->h - 1 - bottom;
			bottom = cnv->h - 1 - t;
		}

		if (left < 0)
			left = 0;
		if (top < 0)
			top = 0;
		if (right >= (int32_t)cnv->w)
			right = cnv->w - 1;
		if (bottom >= (int32_t)cnv->h)
			bottom = cnv->h - 1;
		
		if (left > right || top > bottom)
			return;
		
		if (cnv->rotated) {
			int32_t t;

			t = left;
			left = cnv->h - 1 - bottom;
			bottom = right;
			right = cnv->h - 1 - top;
			top = t;
		}
		w = right - left + 1;
		h = bottom - top + 1;
		
		fbRow += (rowItems * top + left) * cnv->bpp / 8;	//go to right byte
		
		for (r = 0; r < h; r++) {
			
			uint32_t byteColor = cnv->foreColor, curHandledBpp = cnv->bpp;
			
			//color
			while (curHandledBpp < 8) {
				byteColor += byteColor << curHandledBpp;
				curHandledBpp <<= 1;
			}
			
			//pointers
			fb = fbRow;
			fbRow += rowItems * cnv->bpp / 8;
			
			//left partial
			c = 0;
			while (c < w && (c + left) * cnv->bpp % 8) {
				
				uint32_t mask = (1 << cnv->bpp) - 1;
				uint32_t shift = (c + left) * cnv->bpp % 8;
				
				if (!cnv->indexedLe)
					shift = 8 - cnv->bpp - shift;
				
				*fb = ((*fb) &~ (mask << shift)) + ((cnv->foreColor & mask) << shift);
				c++;
			}
			if (c)
				fb++;
			
			//full bytes
			while (c < w && w - c >= pixPerByte) {
				*fb++ = byteColor;
				c += pixPerByte;
			} 
			
			//right partial
			while (c < w) {
				
				uint32_t mask = (1 << cnv->bpp) - 1;
				uint32_t shift = (c + left) * cnv->bpp % 8;
				
				if (!cnv->indexedLe)
					shift = 8 - cnv->bpp - shift;
				
				*fb = ((*fb) &~ (mask << shift)) + ((cnv->foreColor & mask) << shift);
				c++;
			}
		}
	}

#else

	static void uiPrvFillRectEx(struct Canvas *cnv, int32_t left, int32_t top, int32_t right, int32_t bottom, uint32_t pxColor)
	{
		uint32_t w, h, r, c, rowItems = cnv->rotated ? cnv->h : cnv->w;
		uint16_t *fb, *fbRow = cnv->framebuffer;
		
		if (cnv->flipped) {
			int32_t t;

			t = left;
			left = cnv->w - 1 - right;
			right = cnv->w - 1 - t;

			t = top;
			top = cnv->h - 1 - bottom;
			bottom = cnv->h - 1 - t;
		}

		if (left < 0)
			left = 0;
		if (top < 0)
			top = 0;
		if (right >= (int32_t)cnv->w)
			right = cnv->w - 1;
		if (bottom >= (int32_t)cnv->h)
			bottom = cnv->h - 1;
		
		if (left > right || top > bottom)
			return;
		
		if (cnv->rotated) {
			int32_t t;

			t = left;
			left = cnv->h - 1 - bottom;
			bottom = right;
			right = cnv->h - 1 - top;
			top = t;
		}

		w = right - left + 1;
		h = bottom - top + 1;
			
		fbRow += rowItems * top + left;	//go to right word
		
		for (r = 0; r < h; r++) {
						
			//pointers
			fb = fbRow;
			fbRow += rowItems;
			
			//full words
			for(c = 0; c < w; c++)
				*fb++ = pxColor;
		}
	}

	static void uiPrvFillRect(struct Canvas *cnv, int32_t left, int32_t top, int32_t right, int32_t bottom)
	{
		uiPrvFillRectEx(cnv, left, top, right, bottom, uiPrvGreyToColor(cnv->foreColor));
	}

#endif

static uint32_t uiPrvDrawOneCharScaled(struct Canvas *cnv, int32_t r, int32_t c, wchar_t chr, uint_fast8_t scale)
{
	struct FontGlyphInfo gi;
	uint_fast8_t dr, dc;
	int8_t foreColor = cnv->foreColor;

	if (!fontGetGlyphInfo(&gi, (enum Font)cnv->font, chr))
		return 0;
	if (!gi.src)
		return (uint32_t)gi.width * scale;

	for (dr = 0; dr < gi.height; dr++) {
		for (dc = 0; dc < gi.width; dc++) {
			if (fontGetGlyphPixel(&gi, dr, dc)) {
				cnv->foreColor = foreColor;
				uiPrvFillRect(cnv, c + dc * scale, r + dr * scale, c + (dc + 1) * scale - 1, r + (dr + 1) * scale - 1);
			}
		}
	}

	cnv->foreColor = foreColor;
	return (uint32_t)gi.width * scale;
}

static uint32_t uiPrvScaledCharsWidth(struct Canvas *cnv, const char *chars, uint_fast8_t scale)
{
	struct FontGlyphInfo gi;
	bool needsPreSpace = false;
	uint32_t width = 0;
	char ch;

	while ((ch = *chars++) != 0) {
		if (needsPreSpace && fontGetGlyphInfo(&gi, (enum Font)cnv->font, 0x2009))
			width += (uint32_t)gi.width * scale;
		needsPreSpace = ch != ' ';
		if (fontGetGlyphInfo(&gi, (enum Font)cnv->font, (unsigned char)ch))
			width += (uint32_t)gi.width * scale;
	}

	return width;
}

static void uiPrvDrawScaledText(struct Canvas *cnv, int32_t r, int32_t c, const char *str, uint_fast8_t scale)
{
	struct FontGlyphInfo gi;
	bool needsPreSpace = false;
	char ch;

	while ((ch = *str++) != 0) {
		if (needsPreSpace && fontGetGlyphInfo(&gi, (enum Font)cnv->font, 0x2009))
			c += (uint32_t)gi.width * scale;
		needsPreSpace = ch != ' ';
		c += uiPrvDrawOneCharScaled(cnv, r, c, (unsigned char)ch, scale);
	}
}

static void uiPrvSplash(struct Canvas *cnv)
{
	uint_fast8_t scale;
	uint32_t width, height;
	uint64_t end;

	memset(cnv->framebuffer, 0, cnv->w * cnv->h * DISP_BPP / 8);
	cnv->font = FontLarge;
	cnv->foreColor = 9;
	cnv->backColor = 0;

	for (scale = 4; scale > 1; scale--) {
		width = uiPrvScaledCharsWidth(cnv, mUiAppTitle, scale);
		if (width <= cnv->w - 10)
			break;
	}
	width = uiPrvScaledCharsWidth(cnv, mUiAppTitle, scale);
	height = (uint32_t)uiPrvGlyphHeight(cnv) * scale;
	uiPrvDrawScaledText(cnv, (cnv->h - height) / 2, (cnv->w - width) / 2, mUiAppTitle, scale);

	end = getTime() + TICKS_PER_SECOND * 2;
	while (getTime() < end);
}

static uint_fast16_t uiPrvNormalizeKeypress(uint_fast16_t val)
{
	if (val & UI_KEY_BIT_CENTER)
		return UI_KEY_BIT_CENTER;
	if (val & KEY_BIT_A)
		return KEY_BIT_A;
	if (val & KEY_BIT_B)
		return KEY_BIT_B;
	if (val & KEY_BIT_UP)
		return KEY_BIT_UP;
	if (val & KEY_BIT_DOWN)
		return KEY_BIT_DOWN;
	if (val & KEY_BIT_LEFT)
		return KEY_BIT_LEFT;
	if (val & KEY_BIT_RIGHT)
		return KEY_BIT_RIGHT;

	return val;
}

static bool uiPrvRepeatableDirection(uint_fast16_t key)
{
	return key == KEY_BIT_UP || key == KEY_BIT_DOWN || key == KEY_BIT_LEFT || key == KEY_BIT_RIGHT;
}

static void uiPrvKeyRepeatReset(void)
{
	mUiRepeatKey = 0;
	mUiRepeatNextTicks = 0;
	mUiKeyRepeated = false;
}

static uint_fast16_t uiPrvRecvKeypress(void)
{
	uint_fast16_t val, prevVal = 0;
	
	while (!uiGetUiKeys());
	while ((val = uiGetUiKeys()) != 0)
		prevVal = val;

	uiPrvKeyRepeatReset();
	return uiPrvNormalizeKeypress(prevVal);
}

static void uiPrvWaitKeysReleased(void)
{
	while (uiGetUiKeys());
	uiPrvKeyRepeatReset();
}

static void uiPrvRequestToolExit(void)
{
	if (mToolExitEnabled)
		mToolExitRequested = true;
}

static void uiPrvClearToolExit(void)
{
	mToolExitRequested = false;
}

static bool uiPrvToolExitRequested(void)
{
	return mToolExitRequested;
}

static bool uiPrvCenterExitPressedRaw(void)
{
	if (mToolExitEnabled && (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER)) {
		mToolExitRequested = true;
		return true;
	}

	return false;
}

static uint32_t uiPrvHeaderCurrentClockMinute(void)
{
	if (!badgeRtcIsValid())
		return 0xffffffffu;

	return badgeRtcGet() / 60u;
}

static void uiPrvHeaderTimeText(char text[static 9])
{
	uint_fast8_t hour, minute;
	uint_fast8_t hour12;

	if (!badgeRtcGetTimeOfDay(&hour, &minute, NULL)) {
		strcpy(text, "--:-- --");
		return;
	}

	hour12 = hour % 12u;
	if (!hour12)
		hour12 = 12;
	(void)sprintf(text, "%u:%02u %s", (unsigned)hour12, (unsigned)minute, hour >= 12 ? "PM" : "AM");
}

static void uiPrvDrawHeaderBattery(struct Canvas *cnv, int32_t left, uint8_t level, bool charging, uint16_t color)
{
	int32_t top = ((int32_t)uiPrvGlyphHeight(cnv) - 13) / 2;
	int32_t right = left + UI_HEADER_BATTERY_BODY_WIDTH - 1u;

	#if DISP_BPP > 8
		#define UI_HEADER_BATTERY_BACKGROUND_RECT(l, t, r, b) uiPrvFillRectEx(cnv, l, t, r, b, UI_HEADER_BATTERY_BACKGROUND)
		#define UI_HEADER_BATTERY_OUTLINE_RECT(l, t, r, b) uiPrvFillRectEx(cnv, l, t, r, b, color)
		#define UI_HEADER_BATTERY_SEGMENT_RECT(l, t, r, b) uiPrvFillRectEx(cnv, l, t, r, b, color)
		#define UI_HEADER_BATTERY_BOLT_RECT(l, t, r, b) uiPrvFillRectEx(cnv, l, t, r, b, UI_HEADER_BATTERY_BOLT)
	#else
		(void)color;
		#define UI_HEADER_BATTERY_BACKGROUND_RECT(l, t, r, b) uiPrvFillRect(cnv, l, t, r, b)
		#define UI_HEADER_BATTERY_OUTLINE_RECT(l, t, r, b) uiPrvFillRect(cnv, l, t, r, b)
		#define UI_HEADER_BATTERY_SEGMENT_RECT(l, t, r, b) uiPrvFillRect(cnv, l, t, r, b)
		#define UI_HEADER_BATTERY_BOLT_RECT(l, t, r, b) uiPrvFillRect(cnv, l, t, r, b)
	#endif

	UI_HEADER_BATTERY_BACKGROUND_RECT(left - 1, top - 1, right + 1, top + 13);
	UI_HEADER_BATTERY_BACKGROUND_RECT(right + 1, top + 3, right + 4, top + 9);
	UI_HEADER_BATTERY_BACKGROUND_RECT(left + 1, top + 1, right - 1, top + 11);
	UI_HEADER_BATTERY_OUTLINE_RECT(left, top, right, top);
	UI_HEADER_BATTERY_OUTLINE_RECT(left, top + 12, right, top + 12);
	UI_HEADER_BATTERY_OUTLINE_RECT(left, top, left, top + 12);
	UI_HEADER_BATTERY_OUTLINE_RECT(right, top, right, top + 12);
	UI_HEADER_BATTERY_OUTLINE_RECT(right + 1, top + 4, right + UI_HEADER_BATTERY_CAP_WIDTH, top + 8);
	for (uint_fast8_t i = 0; i < level; i++)
		UI_HEADER_BATTERY_SEGMENT_RECT(left + 3 + i * 6, top + 3, left + 7 + i * 6, top + 9);

	if (charging) {
		UI_HEADER_BATTERY_BOLT_RECT(left + 20, top + 2, left + 24, top + 3);
		UI_HEADER_BATTERY_BOLT_RECT(left + 18, top + 4, left + 22, top + 5);
		UI_HEADER_BATTERY_BOLT_RECT(left + 20, top + 5, left + 23, top + 6);
		UI_HEADER_BATTERY_BOLT_RECT(left + 16, top + 7, left + 21, top + 8);
		UI_HEADER_BATTERY_BOLT_RECT(left + 18, top + 8, left + 20, top + 9);
		UI_HEADER_BATTERY_BOLT_RECT(left + 16, top + 10, left + 18, top + 11);
	}

	#undef UI_HEADER_BATTERY_BACKGROUND_RECT
	#undef UI_HEADER_BATTERY_OUTLINE_RECT
	#undef UI_HEADER_BATTERY_SEGMENT_RECT
	#undef UI_HEADER_BATTERY_BOLT_RECT
}

static uint16_t uiPrvHeaderBatteryColor(enum BadgePowerMode powerMode, uint8_t level)
{
	if (powerMode == BadgePowerModeCharging || level >= 4u)
		return UI_HEADER_BATTERY_GREEN;
	if (powerMode == BadgePowerModeLow || level <= 1u)
		return UI_HEADER_BATTERY_RED;
	return UI_HEADER_BATTERY_AMBER;
}

static void uiPrvDrawHeader(struct Canvas *cnv, bool invert, bool force)
{
	const char *windowTitle = mUiHeaderTitle;
	struct BadgePowerStatus powerStatus;
	char timeText[9];
	uint32_t titleLen, titleLeft, titleMaxRight, titleMaxWidth, timeWidth, clockMinute, timeTextLeft = 0, battIconLeft = 0;
	uint8_t battLevel = 0;
	bool battValid = false;
	bool battDrawn = false;
	bool timeDrawn = false;
	enum BadgePowerMode powerMode = BadgePowerModeNormal;
	int8_t foreColor = cnv->foreColor, backColor = cnv->backColor;
	uint8_t font = cnv->font;

	badgePowerPoll();
	if (badgePowerGetCached(&powerStatus)) {
		battValid = true;
		battLevel = powerStatus.battLevel;
		powerMode = powerStatus.mode;
	}

	clockMinute = uiPrvHeaderCurrentClockMinute();
	if (!force && clockMinute == mUiHeaderClockMinute && battValid == mUiHeaderBattValid && (!battValid || (battLevel == mUiHeaderBattLevel && powerMode == mUiHeaderPowerMode)))
		return;
	mUiHeaderClockMinute = clockMinute;
	mUiHeaderBattValid = battValid;
	mUiHeaderBattLevel = battLevel;
	mUiHeaderPowerMode = powerMode;

	uiPrvHeaderTimeText(timeText);

	cnv->font = FontLarge;
	if (battValid) {
		if (UI_HEADER_BATTERY_WIDTH + UI_HEADER_EDGE_PAD < cnv->w) {
			battIconLeft = cnv->w - UI_HEADER_BATTERY_WIDTH - UI_HEADER_EDGE_PAD;
			battDrawn = true;
		}
	}

	timeWidth = uiPrvCharsWidth(cnv, timeText, strlen(timeText));
	if (timeWidth + UI_HEADER_EDGE_PAD < cnv->w) {
		timeTextLeft = (cnv->w - timeWidth) / 2u;
		timeDrawn = true;
		if (battDrawn && timeTextLeft + timeWidth + UI_HEADER_TEXT_GAP > battIconLeft)
			timeDrawn = false;
	}

	cnv->foreColor = invert ? 0 : 9;
	uiPrvFillRect(cnv, 0, 0, cnv->w - 1, uiPrvGlyphHeight(cnv));

	cnv->foreColor = invert ? 9 : 0;
	cnv->backColor = invert ? 0 : 9;

	if (timeDrawn)
		uiPuts(cnv, 0, timeTextLeft, timeText, -1);

	titleMaxRight = cnv->w;
	if (timeDrawn)
		titleMaxRight = timeTextLeft > UI_HEADER_TEXT_GAP ? timeTextLeft - UI_HEADER_TEXT_GAP : timeTextLeft;
	if (battDrawn && battIconLeft < titleMaxRight)
		titleMaxRight = battIconLeft > UI_HEADER_TEXT_GAP ? battIconLeft - UI_HEADER_TEXT_GAP : battIconLeft;
	titleLeft = UI_HEADER_TITLE_LEFT;
	if (titleLeft >= titleMaxRight)
		titleMaxWidth = 0;
	else
		titleMaxWidth = titleMaxRight - titleLeft;
	titleLen = uiPrvCharsFit(cnv, windowTitle, strlen(windowTitle), titleMaxWidth);
	uiPuts(cnv, 0, titleLeft, windowTitle, titleLen);

	if (battDrawn) {
		#if DISP_BPP > 8
			uint16_t battColor = uiPrvHeaderBatteryColor(powerMode, battLevel);

			uiPrvDrawHeaderBattery(cnv, battIconLeft, battLevel,
				powerMode == BadgePowerModeCharging, battColor);
		#else
			uiPrvDrawHeaderBattery(cnv, battIconLeft, battLevel,
				powerMode == BadgePowerModeCharging, 0);
		#endif
	}

	cnv->font = font;
	cnv->foreColor = foreColor;
	cnv->backColor = backColor;
}

static void uiPrvRefreshHeaderClock(struct Canvas *cnv)
{
	uiPrvDrawHeader(cnv, mUiHeaderInverted, false);
}

static void uiPrvDrawMenuFooter(struct Canvas *cnv, uint_fast16_t buttons)
{
	uint8_t font;
	int8_t foreColor, backColor;
	int32_t bottomRow, actionsLeft;
	bool canGoBack = (buttons & KEY_BIT_B) != 0;
	bool canAdjust = (buttons & (KEY_BIT_LEFT | KEY_BIT_RIGHT)) != 0;
	const char *navigate = canAdjust ? "D Pad - Navigate / Adjust" : "D Pad - Navigate";
	static const char select[] = "A - Select";
	static const char back[] = "B - Back";

	if (!cnv)
		return;
	font = cnv->font;
	foreColor = cnv->foreColor;
	backColor = cnv->backColor;
	cnv->font = FontSmall;
	bottomRow = cnv->h - uiPrvGlyphHeight(cnv) - 1u;
	cnv->foreColor = 15;
	cnv->backColor = 0;
	actionsLeft = cnv->w - 10 - (int32_t)uiPrvCharsWidth(cnv, select, sizeof(select) - 1u);
	uiPuts(cnv, bottomRow, 10, navigate, -1);
	uiPuts(cnv, bottomRow - uiPrvGlyphHeight(cnv) - 1u, actionsLeft, select, sizeof(select) - 1u);
	if (canGoBack)
		uiPuts(cnv, bottomRow, actionsLeft, back, sizeof(back) - 1u);
	cnv->font = font;
	cnv->foreColor = foreColor;
	cnv->backColor = backColor;
}

void uiSetFnSettings(const struct UiFnSettings *settings, void *context)
{
	if (!settings || !settings->labels || !settings->value || !settings->adjust ||
		!settings->count || settings->count > UI_FN_SETTINGS_MAX) {
		mUiFnSettings = NULL;
		mUiFnSettingsContext = NULL;
		return;
	}
	mUiFnSettings = settings;
	mUiFnSettingsContext = context;
}

static uint_fast16_t uiPrvRecvMenuKeypress(struct Canvas *cnv)
{
	while (1) {
		uint_fast16_t val = uiGetUiKeys();
		uint_fast16_t key = uiPrvNormalizeKeypress(val);

		if (!key) {
			uiPrvKeyRepeatReset();
			uiPrvRefreshHeaderClock(cnv);
			continue;
		}

		if (uiPrvRepeatableDirection(key)) {
			uint64_t now = getTime();

			if (mUiRepeatKey != key) {
				mUiRepeatKey = key;
				mUiRepeatNextTicks = now + UI_KEY_REPEAT_INITIAL_TICKS;
				mUiKeyRepeated = false;
				return key;
			}
			if (now >= mUiRepeatNextTicks) {
				mUiRepeatNextTicks = now + UI_KEY_REPEAT_INTERVAL_TICKS;
				mUiKeyRepeated = true;
				return key;
			}
			uiPrvRefreshHeaderClock(cnv);
			continue;
		}

		while ((val = uiGetUiKeys()) != 0) {
			key = uiPrvNormalizeKeypress(val);
			uiPrvRefreshHeaderClock(cnv);
		}

		uiPrvKeyRepeatReset();
		mUiKeyRepeated = false;
		return key;
	}
}

static uint_fast8_t uiPrvMenu(struct Canvas *cnv, uint_fast8_t curChoice, uint_fast8_t numChoices, uint_fast16_t *btnsMaskP /* if passed in, return val is the button that was pressed */)
{
	uint_fast8_t i, itemHeight = uiPrvMenuItemHeight(cnv), row = uiPrvContentTop(cnv), fore = cnv->foreColor, back = cnv->backColor;
	uint_fast16_t gotKey;
	uint_fast16_t btnsMask = btnsMaskP ? *btnsMaskP : KEY_BIT_A;

	uiPrvDrawMenuFooter(cnv, btnsMask);
	
	while (1) {
		if (!mToolExitEnabled && uiPowerScreenSaverWoke()) {
			if (btnsMaskP)
				*btnsMaskP = 0;
			return curChoice;
		}
		
		for (i = 0; i < numChoices; i++) {
			
			cnv->foreColor = (i == curChoice) ? fore : back;
			uiPrvDrawOneChar(cnv, row + itemHeight * i, 1, MENU_SELECTION_CHAR);
		}
		
		switch (gotKey = uiPrvRecvMenuKeypress(cnv)) {
			case UI_KEY_BIT_CENTER:
				if (mToolExitEnabled) {
					uiPrvRequestToolExit();
					if (btnsMaskP)
						*btnsMaskP = gotKey;
					return curChoice;
				}
				break;

			case KEY_BIT_DOWN:
				if (curChoice < numChoices - 1)
					curChoice++;
				else if (!mUiKeyRepeated && numChoices)
					curChoice = 0;
				break;
			
			case KEY_BIT_UP:
				if (curChoice)
					curChoice--;
				else if (!mUiKeyRepeated && numChoices)
					curChoice = numChoices - 1;
				break;
			
			default:
				if (btnsMask & gotKey) {
					if (btnsMaskP)
						*btnsMaskP = gotKey;
					return curChoice;
				}
				break;
		}
	}
	cnv->foreColor = fore;
}

static void uiPrvReset(struct Canvas *cnv, bool invert)
{
	memset(cnv->framebuffer, invert ? 0xff : 0, cnv->w * cnv->h * DISP_BPP / 8);
	
	mUiHeaderInverted = invert;
	uiPrvDrawHeader(cnv, invert, true);

	//set colors for ui
	cnv->font = FontMedium;
	cnv->foreColor = invert ? 0 : 15;
	cnv->backColor = invert ? 15 : 0;
}

static void uiPrvBeginPacedRedraw(void)
{
	dispPrvFrameCtrWait();
	dispPrvWaitForScanoutStart();
}

static uint32_t uiPrvDrawWrappedString(struct Canvas *cnv, const char *str, uint32_t r, uint32_t c)	//return num rows printed
{
	uint32_t t, i, numRowsUsed = 1;
	
	while (*str) {
		
		uint32_t nCharsFit = 0;
		
		//handle newlines
		while (*str == '\n') {
			r += uiPrvGlyphHeight(cnv);
			c = 0;
			str++;
			numRowsUsed++;
		}
		
		//handle being at the end of the screen
		if (c >= cnv->w) {
			r += uiPrvGlyphHeight(cnv);
			c = 0;
			numRowsUsed++;
		}
		
		//find string or line end
		for (t = 0; str[t] && str[t] != '\n'; t++);

		//see how many chars fit
		nCharsFit = uiPrvCharsFit(cnv, str, t, cnv->w - c);

		//find word end before the fit limit
		if (nCharsFit < t) {
			t = nCharsFit;
			while (t && str[t] != ' ' && str[t] != '\t' && str[t] != '-')
				t--;
		}

		//if rewinding back to last break caused us to end up with nothing, take extra actions
		if (t == 0 && (nCharsFit < 3 || t <= uiPrvCharsFit(cnv, str, t, cnv->w))) {
			
			r += uiPrvGlyphHeight(cnv);
			c = 0;
			numRowsUsed++;
			continue;
		}
		nCharsFit = t;
		
		c += uiPuts(cnv, r, c, str, nCharsFit);
		str += nCharsFit;
		
		//draw spaces right where we are (including offscreen)
		while (*str == ' ') {
			
			if (c < cnv->w)
				c += uiPrvDrawOneChar(cnv, r, c, ' ');
			str++;
		}
	}
	
	return numRowsUsed;
}

static bool uiPrvGetSimpleAnswer(struct Canvas *cnv, enum DialogType dialogType)
{
	static const char *textTypes[] = {
		[DialogTypeOk] = " A = OK ",
		[DialogTypeYesNo] = " A = Yes    B = No ",
	};

	const char *opts;
	uint_fast16_t key;

	//get message
	if ((unsigned)dialogType >= sizeof(textTypes) / sizeof(*textTypes) || !textTypes[dialogType])
		opts = textTypes[DialogTypeOk];
	else
		opts = textTypes[dialogType];
	
	cnv->foreColor = 0;
	cnv->backColor = 9;
	cnv->font = FontMedium;
	uiPrintf(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, (cnv->w - uiPrvCharsWidth(cnv, opts, strlen(opts))) / 2, opts);
	while (1) {
		key = uiPrvRecvKeypress();
		
		if (key & KEY_BIT_A)
			return true;
		
		if (key & KEY_BIT_B)
			return dialogType == DialogTypeOk;

		if (key & UI_KEY_BIT_CENTER) {
			uiPrvRequestToolExit();
			return dialogType == DialogTypeOk;
		}
	}
}

static bool uiAlert(struct Canvas *cnv, const char *msg, enum DialogType dialogType)
{
	const char *msgEnd = msg + strlen(msg);
	uint32_t y;

	pr("UI_ALERT(%s)\n", msg);

	uiPrvReset(cnv, false);
	cnv->font = FontLarge;
	y = uiPrvGlyphHeight(cnv) + 1;

	cnv->font = FontMedium;
	uiPrvDrawWrappedString(cnv, msg, y, 10);
	
	return uiPrvGetSimpleAnswer(cnv, dialogType);
}

#ifndef NO_SD_CARD
	static void uiPrvCardStreamReset(void)
	{
		if (mCurCardOp == CurrentlyWriting)
			(void)sdWriteStop();
		else if (mCurCardOp == CurrentlyReading)
			(void)sdReadStop();
		mCurCardOp = CurentlyIdle;
	}

	static bool uiPrvCardReadSec(void *diskUserData, uint32_t sec, uint32_t numSec, void *dstP)
	{
		uint8_t *dst = (uint8_t*)dstP;
		
		if (!numSec)
			return false;
		
		if (mCurCardOp == CurrentlyWriting) {
			
			mCurCardOp = CurentlyIdle;
			if (!sdWriteStop())
				return false;
		}
		else if (mCurCardOp == CurrentlyReading && mCurCardSec != sec) {
			
			mCurCardOp = CurentlyIdle;
			if (!sdReadStop())
				return false;
		}
		
		if (mCurCardOp == CurentlyIdle) {
			
			if (!sdReadStart(sec, 0))
				return false;
			
			mCurCardSec = sec;
			mCurCardOp = CurrentlyReading;
		}
		
		//we are now in the reading state
		while (numSec) {
			
			if (!sdReadNext(dst)) {
				uiPrvCardStreamReset();
				return false;
			}
			
			dst += SD_BLOCK_SIZE;
			numSec--;
			mCurCardSec++;
		}
		
		return true;
	}
	
	static bool uiPrvCardWriteSec(void *diskUserData, uint32_t sec, uint32_t numSec, const void *srcP)
	{
		const uint8_t *src = (const uint8_t*)srcP;
		uint32_t i;
		
		if (!numSec)
			return false;
		
		if (mCurCardOp == CurrentlyReading) {
			
			mCurCardOp = CurentlyIdle;
			if (!sdReadStop())
				return false;
		}
		else if (mCurCardOp == CurrentlyWriting) {
			
			mCurCardOp = CurentlyIdle;
			if (!sdWriteStop())
				return false;
		}
		
		if (numSec == 1)
			return sdSecWrite(sec, src);

		if (!sdWriteStart(sec, numSec))
			return false;

		for (i = 0; i < numSec; i++) {

			if (!sdWriteNext(src + i * SD_BLOCK_SIZE)) {
				(void)sdWriteStop();
				return false;
			}
		}
		
		return sdWriteStop();
	}
	
	static bool uiPrvCardPreUnmount(void)
	{
		if (mCurCardOp == CurrentlyWriting) {
			
			mCurCardOp = CurentlyIdle;
			
			return sdWriteStop();
		}
		else if (mCurCardOp == CurrentlyReading) {
			
			mCurCardOp = CurentlyIdle;
			
			return  sdReadStop();
		}
		else {
			
			return true;
		}
	}
	
	static struct FatfsVol* uiPrvMountCardEx(struct Canvas *cnv, bool quiet, bool forceReinit)
	{
		struct FatfsVol *vol;

		if (forceReinit)
			uiPrvCardStreamReset();

		if (forceReinit || !sdGetNumSecs()) {

			if (!sdCardInit() || !sdGetNumSecs()) {
				(void)uiPrvCardPreUnmount();
				sdReportLastError();
				if (!quiet)
					uiAlert(cnv, "Insert an SD card, or check that the card can be read", DialogTypeOk);
				return NULL;	
			}
		}
		
		vol = fatfsMount(uiPrvCardReadSec, uiPrvCardWriteSec, NULL);
		if (vol)
			return vol;
		
		(void)uiPrvCardPreUnmount();
		sdReportLastError();
		if (!quiet)
			uiAlert(cnv, "Cannot find a valid FAT filesystem on the SD card", DialogTypeOk);
		return NULL;
	}

	static struct FatfsVol* uiPrvMountCard(struct Canvas *cnv, bool quiet)
	{
		return uiPrvMountCardEx(cnv, quiet, false);
	}

	static bool uiPrvImageResultCanSkip(enum ImageViewerResult result);

	bool uiRunScreensaverMedia(uint8_t saver)
	{
		struct Canvas cnv = CANVAS_INITIALIZER;
		struct Settings settings;
		struct FatfsVol *vol = uiPrvMountCard(&cnv, true);
		struct FatfsDir *dir;
		struct FatFileLocator locator;
		char name[FATFS_NAME_BUF_LEN];
		char parent[SETTINGS_SCREENSAVER_PATH_MAX];
		const char *path;
		uint32_t size;
		uint8_t attrs;
		bool playedImageInCycle = false;
		bool result = false;

		settingsGet(&settings);
		cnv.flipped = settings.screenSaverRotation;
		path = saver == ScreenSaverGif ? settings.screenSaverGifPath : settings.screenSaverImageFolder;
		if (!vol)
			return false;
		if (!path[0])
			goto out_unmount;
		if (saver == ScreenSaverGif) {
			const char *slash = strrchr(path, '/');

			if (!slash || !slash[1])
				goto out_unmount;
			if (slash == path)
				strcpy(parent, "/");
			else {
				uint32_t len = (uint32_t)(slash - path);

				if (len >= sizeof(parent))
					goto out_unmount;
				memcpy(parent, path, len);
				parent[len] = 0;
			}
			dir = fatfsDirOpen(vol, parent);
			if (!dir || !fatfsFindFileAt(dir, slash + 1, &locator)) {
				if (dir)
					fatfsDirClose(dir);
				goto out_unmount;
			}
			fatfsDirClose(dir);
			result = !uiPrvImageResultCanSkip(imageViewerRun(&cnv, vol, parent, &locator, slash + 1));
			goto out_unmount;
		}
		for (uint32_t wanted = 0; !(uiGetUiKeysRaw()); wanted++) {
			uint32_t count = 0;
			bool found = false;

			if (!(dir = fatfsDirOpen(vol, path)))
				goto out_unmount;
			while (fatfsDirRead(dir, name, &size, &attrs, &locator)) {
				if ((attrs & (FATFS_ATTR_VOL_LBL | FATFS_ATTR_DIR)) || !imageViewerFileName(name))
					continue;
				if (count++ == wanted) {
					found = true;
					break;
				}
			}
			fatfsDirClose(dir);
			if (!count)
				goto out_unmount;
			if (!found) {
				if (!playedImageInCycle)
					goto out_unmount;
				playedImageInCycle = false;
				wanted = 0xffffffffu;
				continue;
			}
			if (uiPrvImageResultCanSkip(imageViewerRunStill(&cnv, vol, path, &locator, name)))
				continue;
			playedImageInCycle = true;
			for (uint64_t until = getTime() + (uint64_t)TICKS_PER_SECOND * 5u;
					getTime() < until && !uiGetUiKeysRaw(); ) {
				badgeLedsTick();
				timebaseIdleWaitMsec(10);
			}
		}
		result = true;

	out_unmount:
		(void)uiPrvCardPreUnmount();
		(void)fatfsUnmount(vol);
		return result;
	}

	static bool uiPrvRunBootGif(struct Canvas *cnv, const char *path)
	{
		struct FatfsVol *vol;
		struct FatfsDir *dir;
		struct FatFileLocator locator;
		char parent[SETTINGS_SCREENSAVER_PATH_MAX];
		const char *slash;
		enum ImageViewerResult result;

		if (!path || !path[0])
			return false;
		vol = uiPrvMountCard(cnv, true);
		if (!vol)
			return false;
		slash = strrchr(path, '/');
		if (!slash || !slash[1])
			goto out_unmount;
		if (slash == path)
			strcpy(parent, "/");
		else {
			uint32_t len = (uint32_t)(slash - path);

			if (len >= sizeof(parent))
				goto out_unmount;
			memcpy(parent, path, len);
			parent[len] = 0;
		}
		dir = fatfsDirOpen(vol, parent);
		if (!dir || !fatfsFindFileAt(dir, slash + 1, &locator)) {
			if (dir)
				fatfsDirClose(dir);
			goto out_unmount;
		}
		fatfsDirClose(dir);
		result = imageViewerRunBoot(cnv, vol, parent, &locator, slash + 1);
		if (!uiPrvImageResultCanSkip(result)) {
			(void)uiPrvCardPreUnmount();
			(void)fatfsUnmount(vol);
			return true;
		}

	out_unmount:
		(void)uiPrvCardPreUnmount();
		(void)fatfsUnmount(vol);
		return false;
	}

	static void uiPrvBootSplash(struct Canvas *cnv)
	{
		struct Settings settings;

		settingsGet(&settings);
		if (settings.bootMenuCustomGif && uiPrvRunBootGif(cnv, settings.screenSaverGifPath))
			return;
		uiPrvSplash(cnv);
	}
	
	bool uiRunScreensaverImageEffect(uint8_t saver)
	{
		struct Canvas cnv = CANVAS_INITIALIZER;
		struct Settings settings;
		struct FatfsVol *vol;
		struct FatfsDir *dir;
		struct FatFileLocator locator;
		struct ToolWorkspaceSpan scratch;
		const char *path, *slash;
		char parent[SETTINGS_SCREENSAVER_PATH_MAX];
		uint16_t *fb = (uint16_t*)cnv.framebuffer;
		uint16_t *image;
		int32_t imageX = ((int32_t)DISP_WIDTH - UI_SAVER_IMAGE_WIDTH) / 2;
		int32_t imageY = ((int32_t)DISP_HEIGHT - UI_SAVER_IMAGE_HEIGHT) / 2;
		int32_t imageDx = 2, imageDy = 2;
		uint32_t frame = 0;
		bool result = false;

		if ((saver != ScreenSaverDvdBounce && saver != ScreenSaverScrollPattern) || !fb)
			return false;
		settingsGet(&settings);
		path = settings.screenSaverGifPath;
		if (!path[0])
			return false;
		vol = uiPrvMountCard(&cnv, true);
		if (!vol)
			return false;
		slash = strrchr(path, '/');
		if (!slash || !slash[1])
			goto out_unmount;
		if (slash == path)
			strcpy(parent, "/");
		else {
			uint32_t len = (uint32_t)(slash - path);

			if (len >= sizeof(parent))
				goto out_unmount;
			memcpy(parent, path, len);
			parent[len] = 0;
		}
		dir = fatfsDirOpen(vol, parent);
		if (!dir || !fatfsFindFileAt(dir, slash + 1, &locator)) {
			if (dir)
				(void)fatfsDirClose(dir);
			goto out_unmount;
		}
		(void)fatfsDirClose(dir);
		cnv.flipped = settings.screenSaverRotation;
		if (uiPrvImageResultCanSkip(imageViewerRunStill(&cnv, vol, parent, &locator, slash + 1)))
			goto out_unmount;
		if (!toolWorkspaceAcquire(ToolWorkspaceWram, ToolWorkspaceOwnerImage, &scratch) ||
				scratch.size < UI_SAVER_IMAGE_BYTES)
			goto out_unmount;
		image = (uint16_t*)scratch.ptr;
		for (uint32_t y = 0; y < UI_SAVER_IMAGE_HEIGHT; y++) {
			uint32_t srcY = y * (DISP_HEIGHT - 1u) / (UI_SAVER_IMAGE_HEIGHT - 1u);

			for (uint32_t x = 0; x < UI_SAVER_IMAGE_WIDTH; x++) {
				uint32_t srcX = x * (DISP_WIDTH - 1u) / (UI_SAVER_IMAGE_WIDTH - 1u);

				image[y * UI_SAVER_IMAGE_WIDTH + x] = fb[srcY * DISP_WIDTH + srcX];
			}
		}
		while (!uiGetUiKeysRaw()) {
			if (saver == ScreenSaverDvdBounce) {
				memset(fb, 0, DISP_WIDTH * DISP_HEIGHT * sizeof(*fb));
				for (uint32_t y = 0; y < UI_SAVER_IMAGE_HEIGHT; y++)
					memcpy(fb + (imageY + (int32_t)y) * DISP_WIDTH + imageX,
						image + y * UI_SAVER_IMAGE_WIDTH, UI_SAVER_IMAGE_WIDTH * sizeof(*image));
				imageX += imageDx;
				imageY += imageDy;
				if (imageX <= 0 || imageX >= (int32_t)(DISP_WIDTH - UI_SAVER_IMAGE_WIDTH)) {
					imageDx = -imageDx;
					imageX += imageDx * 2;
				}
				if (imageY <= 0 || imageY >= (int32_t)(DISP_HEIGHT - UI_SAVER_IMAGE_HEIGHT)) {
					imageDy = -imageDy;
					imageY += imageDy * 2;
				}
			}
			else {
				uint32_t scrollX = (frame * 2u) % UI_SAVER_IMAGE_WIDTH;
				uint32_t scrollY = frame % UI_SAVER_IMAGE_HEIGHT;

				for (uint32_t y = 0; y < DISP_HEIGHT; y++) {
					const uint16_t *src = image + ((y + scrollY) % UI_SAVER_IMAGE_HEIGHT) * UI_SAVER_IMAGE_WIDTH;
					uint16_t *dst = fb + y * DISP_WIDTH;

					for (uint32_t x = 0; x < DISP_WIDTH; x++)
						dst[x] = src[(x + scrollX) % UI_SAVER_IMAGE_WIDTH];
				}
			}
			dispPrvWaitForScanoutStart();
			badgeLedsTick();
			timebaseIdleWaitMsec(33u);
			frame++;
		}
		result = true;
		toolWorkspaceRelease(ToolWorkspaceWram, ToolWorkspaceOwnerImage);

	out_unmount:
		(void)uiPrvCardPreUnmount();
		(void)fatfsUnmount(vol);
		return result;
	}

	static int strsCaselesslyCompareUtf(const char *aP, const char *bP, unsigned n)
	{
		const uint8_t *a = (const uint8_t*)aP;
		const uint8_t *b = (const uint8_t*)bP;
		struct Utf8state as, bs;
		
		utf8stateStart(&as);
		utf8stateStart(&bs);
		
		while (n--) {
			
			uint32_t ac = utf8inputByte(&as, *a++);
			uint32_t bc = utf8inputByte(&bs, *b++);
			
			if (ac == UTF_ERROR || bc == UTF_ERROR)		///strings with errors are nonequal. argue all you want - i do not care
				return -1;
			
			if (ac == UTF_NO_OUTPUT && bc == UTF_NO_OUTPUT)
				continue;
			
			if (ac == UTF_NO_OUTPUT) 					//one has a char and another not -> not equal
				return -1;
			if (bc == UTF_NO_OUTPUT)
				return 1;
			if (!ac && !bc)
				return 0;
			if (!ac)
				return -1;
			if (!bc)
				return 1;
			
			ac = utfToCaseless(ac);
			bc = utfToCaseless(bc);
			
			if (ac != bc)
				return ac - bc;
		}
		return 0;
	}

	static bool uiPrvHiddenEntry(const char *name, uint8_t attrs)
	{
		return (attrs & FATFS_ATTR_HIDDEN) || name[0] == '.';
	}
	
	static void sortNames(struct RomOption **headP, struct RomOption **tailP, uint32_t count)
	{
		struct RomOption *head1 = *headP, *head2, *tail1, *tail2 = *tailP, *cur;
		uint32_t i;
			
		if (count < 2)
			return;
		
		for (head2 = head1, i = 0; i < count / 2; i++, head2 = head2->next);
		tail1 = head2->prev;
		
		tail1->next = NULL;
		head2->prev = NULL;
		
		sortNames(&head1, &tail1, i);
		sortNames(&head2, &tail2, count - i);
		
		cur = NULL;
		while (head1 || head2) {
			
			struct RomOption **theSmallerP;
			
			if (head1 && head2)
				theSmallerP = (strsCaselesslyCompareUtf(head1->name, head2->name, 0xffffffff) < 0) ? &head1 : &head2;
			else if (head1)
				theSmallerP = &head1;
			else
				theSmallerP = &head2;
			
			(*theSmallerP)->prev = cur;
			cur = (*theSmallerP);
			*theSmallerP = (*theSmallerP)->next;
		}
		*tailP = cur;
		cur->next = NULL;
		while (cur) {
			if (cur->prev)
				cur->prev->next = cur;
			*headP = cur;
			cur = cur->prev;
		}
	}
	
	static uint32_t __attribute__((noinline)) uiPrvListRoms(struct Canvas *cnv, struct FatfsVol *vol, struct RomOption **headP, struct RomOption **tailP)
	{
		struct RomOption *nextAvail = (struct RomOption*)CART_RAM_ADDR_IN_RAM, *tail = NULL, *head = NULL, *cur;
		uint32_t spaceAvail = QSPI_RAM_SIZE_MAX, count = 0;
		char fname[FATFS_NAME_BUF_LEN];
		uint_fast8_t itemHeight;
		struct FatfsDir *dir;
		uint32_t fileSz;
		uint8_t attrs;
		
		dir = fatfsDirOpen(vol, "/ROMS");
		
		if (!dir) {
			
			uiAlert(cnv, "Cannot find a /ROMS directory on the SD card", DialogTypeOk);
			return 0;
		}
		
		while (fatfsDirRead(dir, fname, &fileSz, &attrs, NULL)) {
			
			unsigned nameLen;
			
			//pr(" file '%s', %lu bytes, attr %02xh\n", fname, fileSz, attrs);
			
			if ((attrs & (FATFS_ATTR_VOL_LBL | FATFS_ATTR_DIR)) || uiPrvHiddenEntry(fname, attrs))
				continue;
			
			//roms are a power of 2 in size and must be at laest 8K
			if (fileSz < 8129 || (fileSz & (fileSz - 1)))
				continue;
			
			nameLen = strlen(fname);
			
			if (nameLen < 4)
				continue;
			
			if (strsCaselesslyCompareUtf(fname + nameLen - 4, ".gbc", 4) && strsCaselesslyCompareUtf(fname + nameLen - 3, ".gb", 3))
				continue;
			
			if (spaceAvail < sizeof(struct RomOption) + nameLen + 1)
				continue;
			
			nextAvail->prev = tail;
			memcpy(nextAvail->name, fname, nameLen + 1);
			tail = nextAvail;
			
			nextAvail = (struct RomOption*)(nextAvail->name + nameLen + 1);
			spaceAvail -= sizeof(struct RomOption) + nameLen + 1;
			count++;
		}
		fatfsDirClose(dir);
		pr("found %u roms\n", count);
		
		if (!count) {
			uiAlert(cnv, "No ROMs found in /ROMS on the card", DialogTypeOk);
			return 0;
		}
		
		//create forward links
		if (tail)
			tail->next = NULL;
		for (cur = tail; cur; cur = cur->prev) {
			if (cur->prev)
				cur->prev->next = cur;
			head = cur;
		}
		
		*headP = head;
		*tailP = tail;
		
		return count;
	}
#endif //NO_SD_CARD

static uint_fast8_t uiPrvDrawScrollbar(struct Canvas *cnv, uint_fast16_t scrollTop, uint32_t numItems, uint32_t topItem, uint32_t onscreenItems)	//return width used
{
	const uint_fast16_t scrollWidth = 5, scrollBottom = cnv->h - 1, scrollRight = cnv->w - 1, scrollLeft = scrollRight - scrollWidth + 1, arrowsHeight = (scrollWidth + 1) / 2, carWidth = 3;
	uint_fast16_t i, lineHeight, carHeight, remainLineHeight, carPos;
	
	//do not bother if not enough items to scroll
	if (onscreenItems >= numItems)
		return 0;
	
	//erase the area
	cnv->foreColor = 0;
	uiPrvFillRect(cnv, scrollLeft - 1, scrollTop, scrollRight, scrollBottom);
	
	//draw arrows
	cnv->foreColor = 13;
	for (i = 0; i < arrowsHeight; i++) {
		uiPrvFillRect(cnv, scrollLeft + (arrowsHeight - 1 - i), scrollTop + i, scrollRight - (arrowsHeight - 1 - i), scrollTop + i);
		uiPrvFillRect(cnv, scrollLeft + (arrowsHeight - 1 - i), scrollBottom - i, scrollRight - (arrowsHeight - 1 - i), scrollBottom - i);
	}
	
	//draw line
	cnv->foreColor = 8;
	uiPrvFillRect(cnv, scrollLeft + scrollWidth / 2, scrollTop + arrowsHeight, scrollLeft + scrollWidth / 2, scrollBottom - arrowsHeight);
	
	//calc car
	//car height should represent what percent of the view we see, but it also needs to be reasonble in size (not too small or big)
	//first calc a fair car
	lineHeight = (scrollBottom - scrollTop + 1) - 2 * arrowsHeight - 2; /* 2 to allow one pix space between car and arrows */
	carHeight = (onscreenItems * lineHeight + (numItems - onscreenItems) / 2) / (numItems - onscreenItems);
	//then adjust to min/max
	if (carHeight < 8)
		carHeight = 8;
	if (carHeight > lineHeight * 7 / 8)
		carHeight = lineHeight * 7 / 8;
	
	//calc position
	remainLineHeight = lineHeight - carHeight;
	carPos = (topItem * remainLineHeight + (numItems - onscreenItems) / 2) / (numItems - onscreenItems);
	
	//avoid the visual mistake of showing the car at the limit of travel if more items exist in the direction
	if (carPos == 0 && topItem != 0)
		carPos++;
	else if (carPos + carHeight == lineHeight && topItem != numItems - onscreenItems)
		carPos--;
	
	carPos++;	/* account for that space between car and arrows */

	//draw the car
	cnv->foreColor = 12;
	uiPrvFillRect(cnv, scrollLeft + (scrollWidth - carWidth) / 2, carPos + arrowsHeight + scrollTop, scrollRight - (scrollWidth - carWidth) / 2, carPos + arrowsHeight + scrollTop + carHeight - 1);
	
	return scrollWidth + 1;
}

static void uiPrvDrawTruncText(struct Canvas *cnv, int32_t r, int32_t c, uint32_t maxWidth, const char *str)
{
	uint32_t stringLen = strlen(str), numCharsFit, widthUsed = uiPrvCharsMeasure(cnv, str, stringLen, maxWidth, &numCharsFit);
	static const char truncInd[] = "...";

	if (numCharsFit == stringLen)
		uiPuts(cnv, r, c, str, stringLen);
	else {
		
		uint32_t truncIndWidth = uiPrvCharsWidth(cnv, truncInd, sizeof(truncInd) - 1);
		
		if (truncIndWidth >= maxWidth)		//do not bother with the impossible cases
			return;
		
		widthUsed = uiPrvCharsMeasure(cnv, str, stringLen, maxWidth - truncIndWidth, &numCharsFit);
		uiPuts(cnv, r, c, str, numCharsFit);
		uiPuts(cnv, r, c + widthUsed, truncInd, sizeof(truncInd) - 1);
	}
}

static bool uiPrvStrEndsWithNoCase(const char *str, const char *suffix)
{
	uint32_t strLen = strlen(str), suffixLen = strlen(suffix), i;

	if (strLen < suffixLen)
		return false;

	str += strLen - suffixLen;
	for (i = 0; i < suffixLen; i++) {
		char a = str[i], b = suffix[i];

		if (a >= 'A' && a <= 'Z')
			a += 'a' - 'A';
		if (b >= 'A' && b <= 'Z')
			b += 'a' - 'A';
		if (a != b)
			return false;
	}

	return true;
}

static bool uiPrvRomFileName(const char *fname)
{
	return uiPrvStrEndsWithNoCase(fname, ".gb") || uiPrvStrEndsWithNoCase(fname, ".gbc") ||
		uiPrvStrEndsWithNoCase(fname, ".nes") || uiPrvStrEndsWithNoCase(fname, ".hex") ||
		uiPrvStrEndsWithNoCase(fname, ".arduboy");
}

static bool uiPrvGameboyRomFileName(const char *fname)
{
	return uiPrvStrEndsWithNoCase(fname, ".gb");
}

static bool uiPrvGameboyColorRomFileName(const char *fname)
{
	return uiPrvStrEndsWithNoCase(fname, ".gbc");
}

static bool uiPrvNesRomFileName(const char *fname)
{
	return uiPrvStrEndsWithNoCase(fname, ".nes");
}

static bool uiPrvArduboyRomFileName(const char *fname)
{
	return uiPrvStrEndsWithNoCase(fname, ".hex") || uiPrvStrEndsWithNoCase(fname, ".arduboy");
}

#define GAME_META_MAGIC			0x31454d47u /* GME1 */
#define GAME_META_OFFSET		(QSPI_FILENAME_MAXLEN - QSPI_WRITE_GRANULARITY)

struct GameSelectionFlashMeta {
	uint32_t magic;
	uint32_t runtime;
	uint32_t romSize;
	uint32_t saveRamSize;
	uint32_t reserved[4];
};

enum SaveNameKind {
	SaveNameKindAuto = 0,
	SaveNameKindClean = 1,
	SaveNameKindFull = 2,
	SaveNameKindFallback = 3,
};

struct CartRamOwner {
	bool valid;
	enum GameRuntime runtime;
	uint32_t romSize;
	uint32_t saveRamSize;
	uint32_t gameHash;
};

static struct CartRamOwner mCartRamOwner;

static uint32_t uiPrvHashBytes(const void *data, uint32_t size)
{
	const uint8_t *bytes = data;
	uint32_t hash = 2166136261u;

	while (size--) {
		hash ^= *bytes++;
		hash *= 16777619u;
	}
	return hash;
}

static uint32_t uiPrvHashString(const char *str)
{
	uint32_t hash = 2166136261u;

	while (*str) {
		hash ^= (uint8_t)*str++;
		hash *= 16777619u;
	}
	return hash;
}

static const char *uiPrvRuntimeName(enum GameRuntime runtime)
{
	switch (runtime) {
		case GameRuntimeGb:
			return "GB";
		case GameRuntimeNes:
			return "NES";
		case GameRuntimeArduboy:
			return "Arduboy";
		case GameRuntimeNone:
		default:
			return "none";
	}
}

static const char *uiPrvSaveNameKindName(enum SaveNameKind kind)
{
	switch (kind) {
		case SaveNameKindFull:
			return "full";
		case SaveNameKindClean:
			return "clean";
		case SaveNameKindFallback:
			return "fallback";
		case SaveNameKindAuto:
		default:
			return "auto";
	}
}

static uint32_t uiPrvSaveFingerprint(const void *data, uint32_t size)
{
	return uiPrvHashBytes(data, size);
}

static bool uiPrvCartRamOwnerMatches(enum GameRuntime runtime, uint32_t romSize, uint32_t saveRamSize, const char *romName)
{
	return mCartRamOwner.valid &&
		mCartRamOwner.runtime == runtime &&
		mCartRamOwner.romSize == romSize &&
		mCartRamOwner.saveRamSize == saveRamSize &&
		mCartRamOwner.gameHash == uiPrvHashString(romName);
}

static bool uiPrvCartRamOwnerMatchesSelection(const struct GameSelection *selection)
{
	return uiPrvCartRamOwnerMatches(selection->runtime, selection->romSize, selection->saveRamSize, (const char*)QSPI_FILENAME_START);
}

static void uiPrvCartRamOwnerClear(const char *reason)
{
	if (mCartRamOwner.valid)
		pr("Save RAM owner: cleared (%s, runtime=%s rom=%lu ram=%lu hash=%08lx)\n",
			reason ? reason : "unknown", uiPrvRuntimeName(mCartRamOwner.runtime),
			(unsigned long)mCartRamOwner.romSize, (unsigned long)mCartRamOwner.saveRamSize,
			(unsigned long)mCartRamOwner.gameHash);
	memset(&mCartRamOwner, 0, sizeof(mCartRamOwner));
}

static void uiPrvCartRamOwnerSet(enum GameRuntime runtime, uint32_t romSize, uint32_t saveRamSize, const char *romName, const char *reason)
{
	mCartRamOwner.valid = true;
	mCartRamOwner.runtime = runtime;
	mCartRamOwner.romSize = romSize;
	mCartRamOwner.saveRamSize = saveRamSize;
	mCartRamOwner.gameHash = uiPrvHashString(romName);
	pr("Save RAM owner: set (%s, runtime=%s rom=%lu ram=%lu hash=%08lx)\n",
		reason ? reason : "unknown", uiPrvRuntimeName(runtime), (unsigned long)romSize,
		(unsigned long)saveRamSize, (unsigned long)mCartRamOwner.gameHash);
}

static bool uiPrvHydrateSaveRamFromFlash(enum GameRuntime runtime, uint32_t romSize, uint32_t saveRamSize,
	const char *romName, const char *reason, bool markOwner)
{
	uint32_t qspiHash, cartHash;

	if (saveRamSize > QSPI_RAM_SIZE_MAX) {
		pr("Savegame hydrate: refusing oversized save RAM (%lu bytes)\n", (unsigned long)saveRamSize);
		uiPrvCartRamOwnerClear("hydrate oversized");
		return false;
	}

	if (saveRamSize)
		memcpy(CART_RAM_ADDR_IN_RAM, (const void*)QSPI_RAM_COPY_START, saveRamSize);
	if (saveRamSize < QSPI_RAM_SIZE_MAX)
		memset(CART_RAM_ADDR_IN_RAM + saveRamSize, 0xff, QSPI_RAM_SIZE_MAX - saveRamSize);

	qspiHash = uiPrvSaveFingerprint((const void*)QSPI_RAM_COPY_START, saveRamSize);
	cartHash = uiPrvSaveFingerprint(CART_RAM_ADDR_IN_RAM, saveRamSize);
	pr("Savegame hydrate: %s /%s (%lu bytes, qspi=%08lx cart=%08lx, rom=%s)\n",
		reason ? reason : "load", uiPrvRuntimeName(runtime), (unsigned long)saveRamSize,
		(unsigned long)qspiHash, (unsigned long)cartHash, romName ? romName : "");

	if (saveRamSize && memcmp(CART_RAM_ADDR_IN_RAM, (const void*)QSPI_RAM_COPY_START, saveRamSize)) {
		pr("Savegame hydrate: cart RAM verification failed\n");
		uiPrvCartRamOwnerClear("hydrate verify failed");
		return false;
	}

	if (markOwner)
		uiPrvCartRamOwnerSet(runtime, romSize, saveRamSize, romName, reason);
	return true;
}

static enum GameRuntime uiPrvRuntimeForName(const char *fname)
{
	if (uiPrvStrEndsWithNoCase(fname, ".nes"))
		return GameRuntimeNes;
	if (uiPrvStrEndsWithNoCase(fname, ".hex") || uiPrvStrEndsWithNoCase(fname, ".arduboy"))
		return GameRuntimeArduboy;
	if (uiPrvStrEndsWithNoCase(fname, ".gb") || uiPrvStrEndsWithNoCase(fname, ".gbc"))
		return GameRuntimeGb;
	return GameRuntimeNone;
}

#ifndef NO_SD_CARD
	static void uiPrvCopyStr(char *dst, uint32_t dstLen, const char *src);

	typedef bool (*UiFileNameFilterF)(const char *name);
	typedef void (*UiFileDisplayNameF)(char *dst, uint32_t dstLen, const char *name);

	static UiFileNameFilterF uiPrvRomFilterForConsole(enum UiEmulatorConsole console)
	{
		switch (console) {
			case UiEmulatorConsoleArduboy:
				return uiPrvArduboyRomFileName;
			case UiEmulatorConsoleGameboy:
				return uiPrvGameboyRomFileName;
			case UiEmulatorConsoleGameboyColor:
				return uiPrvGameboyColorRomFileName;
			case UiEmulatorConsoleNes:
				return uiPrvNesRomFileName;
			default:
				return uiPrvRomFileName;
		}
	}

	static const char *uiPrvRomRootForConsole(enum UiEmulatorConsole console)
	{
		switch (console) {
			case UiEmulatorConsoleArduboy:
				return "/ROMS/AB";
			case UiEmulatorConsoleGameboy:
				return "/ROMS/GB";
			case UiEmulatorConsoleGameboyColor:
				return "/ROMS/GBC";
			case UiEmulatorConsoleNes:
				return "/ROMS/NES";
			default:
				return "/ROMS";
		}
	}

	static const char *uiPrvSaveSubdirForConsole(enum UiEmulatorConsole console)
	{
		switch (console) {
			case UiEmulatorConsoleArduboy:
				return "AB";
			case UiEmulatorConsoleGameboyColor:
				return "GBC";
			case UiEmulatorConsoleNes:
				return "NES";
			case UiEmulatorConsoleGameboy:
			default:
				return "GB";
		}
	}

	static void uiPrvSaveDirPathForConsole(enum UiEmulatorConsole console, char *dst, uint32_t dstLen)
	{
		if (!dstLen)
			return;
		(void)sprintf(dst, "/SAVE/%s", uiPrvSaveSubdirForConsole(console));
	}

	static enum UiEmulatorConsole uiPrvSaveConsoleForGame(enum GameRuntime runtime, enum RomColorSupport colorSupport, const char *romName)
	{
		if (runtime == GameRuntimeNes)
			return UiEmulatorConsoleNes;
		if (runtime == GameRuntimeArduboy)
			return UiEmulatorConsoleArduboy;
		if (runtime == GameRuntimeGb) {
			if (romName && uiPrvStrEndsWithNoCase(romName, ".gbc"))
				return UiEmulatorConsoleGameboyColor;
			if (romName && uiPrvStrEndsWithNoCase(romName, ".gb"))
				return UiEmulatorConsoleGameboy;
			return colorSupport == RomNoColor ? UiEmulatorConsoleGameboy : UiEmulatorConsoleGameboyColor;
		}
		return UiEmulatorConsoleGameboy;
	}

	static struct FatfsDir *uiPrvOpenSaveDirForConsole(struct FatfsVol *vol, enum UiEmulatorConsole console, bool create)
	{
		struct FatfsDir *rootDir, *saveDir = NULL;
		struct FatFileLocator loc;
		const char *subdir = uiPrvSaveSubdirForConsole(console);

		rootDir = fatfsDirOpen(vol, "/SAVE");
		if (!rootDir && create) {
			if (fatfsDirCreate(vol, "/SAVE", &loc))
				rootDir = fatfsDirOpenWithLocator(vol, &loc);
		}
		if (!rootDir)
			return NULL;

		saveDir = fatfsDirOpenAt(rootDir, subdir);
		if (!saveDir && create) {
			if (fatfsDirCreateAt(rootDir, subdir, &loc))
				saveDir = fatfsDirOpenWithLocator(vol, &loc);
		}
		fatfsDirClose(rootDir);
		return saveDir;
	}

	struct UiFileListCtx {
		struct MusicOption *nextAvail;
		struct MusicOption *head;
		struct MusicOption *tail;
		uint32_t spaceAvail;
		uint32_t count;
		bool overflow;
		UiFileNameFilterF filterF;
		char *fname;
	};

	#define UI_PICK_FILE_PATH_BUF_SZ		(sizeof("/BADUSB/") + FATFS_NAME_BUF_LEN * 8)
	#define UI_BROWSER_MAX_DEPTH		16
	#define UI_PICK_FILE_NAME_BUF_SZ		64
	#define UI_BROWSER_ACTION_PATH_MAX	192
	#define UI_BROWSER_FAVORITES_MAX	8
	#define UI_BROWSER_FAVORITES_PATH	"/.DC32-FAVORITES.TXT"
	#define UI_BROWSER_FAVORITES_FILE_MAX	(UI_BROWSER_FAVORITES_MAX * UI_BROWSER_ACTION_PATH_MAX)
	#define UI_BROWSER_HOLD_TICKS		((uint64_t)TICKS_PER_SECOND * 1200u / 1000u)
	#define UI_BROWSER_CONTEXT_KEY		0x8000u
	#define UI_BROWSER_FN_ACTION_KEY	0x8001u
	#define UI_BROWSER_FN_MENU_KEY		0x8002u

	struct UiBrowserClipboard {
		bool valid;
		bool isDir;
		char path[UI_BROWSER_ACTION_PATH_MAX];
	};

	struct UiBrowserFavorites {
		uint8_t count;
		char path[UI_BROWSER_FAVORITES_MAX][UI_BROWSER_ACTION_PATH_MAX];
	};

	struct UiBrowserOps {
		struct UiBrowserClipboard clipboard;
		struct UiBrowserFavorites favorites;
		bool showFavorites;
	};

	static void uiPrvLedSettings(struct Canvas *cnv, struct Settings *settings);
	static void uiPrvThemeSettings(struct Canvas *cnv, struct Settings *settings);
	static void uiPrvScreenSettings(struct Canvas *cnv, struct Settings *settings);
	static void uiPrvAudioSettings(struct Canvas *cnv, struct Settings *settings);

	static bool uiPrvFileListComesBefore(const struct MusicOption *option, const char *name, bool isDir)
	{
		if (option->isDir != isDir)
			return option->isDir;
		return strsCaselesslyCompareUtf(option->name, name, 0xffffffff) <= 0;
	}

	static bool uiPrvFileListAppend(struct UiFileListCtx *ctx, const char *fname, uint32_t fileSz, bool isDir, const struct FatFileLocator *locator)
	{
		uint32_t nameLen = strlen(fname), spaceNeeded = (sizeof(struct MusicOption) + nameLen + 1 + 3) &~ 3;
		struct MusicOption *option, *after = NULL, *before = ctx->head;

		if (spaceNeeded > ctx->spaceAvail) {
			ctx->overflow = true;
			return false;
		}

		option = ctx->nextAvail;
		option->locator = *locator;
		option->size = fileSz;
		option->isDir = isDir;
		memcpy(option->name, fname, nameLen + 1);

		while (before && uiPrvFileListComesBefore(before, fname, isDir)) {
			after = before;
			before = before->next;
		}

		option->prev = after;
		option->next = before;
		if (after)
			after->next = option;
		else
			ctx->head = option;
		if (before)
			before->prev = option;
		else
			ctx->tail = option;

		ctx->nextAvail = (struct MusicOption*)(((uint8_t*)ctx->nextAvail) + spaceNeeded);
		ctx->spaceAvail -= spaceNeeded;
		ctx->count++;
		return true;
	}

	static bool uiPrvFileListAppendTail(struct UiFileListCtx *ctx, const char *fname, uint32_t fileSz, bool isDir, const struct FatFileLocator *locator)
	{
		uint32_t nameLen = strlen(fname), spaceNeeded = (sizeof(struct MusicOption) + nameLen + 1 + 3) &~ 3;
		struct MusicOption *option;

		if (spaceNeeded > ctx->spaceAvail) {
			ctx->overflow = true;
			return false;
		}

		option = ctx->nextAvail;
		option->locator = *locator;
		option->size = fileSz;
		option->isDir = isDir;
		memcpy(option->name, fname, nameLen + 1);
		option->prev = ctx->tail;
		option->next = NULL;

		if (ctx->tail)
			ctx->tail->next = option;
		else
			ctx->head = option;
		ctx->tail = option;

		ctx->nextAvail = (struct MusicOption*)(((uint8_t*)ctx->nextAvail) + spaceNeeded);
		ctx->spaceAvail -= spaceNeeded;
		ctx->count++;
		return true;
	}

	static struct MusicOption *uiPrvFileOptionAt(struct MusicOption *head, uint32_t depth, uint32_t selectedItem)
	{
		uint32_t skip;

		if (depth && !selectedItem)
			return NULL;
		skip = selectedItem - (depth ? 1 : 0);
		while (head && skip--)
			head = head->next;
		return head;
	}

	static void uiPrvDrawDirLabel(struct Canvas *cnv, int32_t r, int32_t c, uint32_t maxWidth, const char *name)
	{
		uint32_t openWidth, stringLen, nameMaxWidth, nameWidth, numCharsFit;
		static const char open[] = "/", truncInd[] = "...";

		openWidth = uiPrvCharsWidth(cnv, open, sizeof(open) - 1);
		if (maxWidth <= openWidth)
			return;

		uiPuts(cnv, r, c, open, sizeof(open) - 1);
		c += openWidth;
		nameMaxWidth = maxWidth - openWidth;
		stringLen = strlen(name);
		nameWidth = uiPrvCharsMeasure(cnv, name, stringLen, nameMaxWidth, &numCharsFit);
		if (numCharsFit == stringLen) {
			uiPuts(cnv, r, c, name, stringLen);
			c += nameWidth;
		}
		else {
			uint32_t truncWidth = uiPrvCharsWidth(cnv, truncInd, sizeof(truncInd) - 1);

			if (truncWidth < nameMaxWidth) {
				nameWidth = uiPrvCharsMeasure(cnv, name, stringLen, nameMaxWidth - truncWidth, &numCharsFit);
				uiPuts(cnv, r, c, name, numCharsFit);
				c += nameWidth;
				uiPuts(cnv, r, c, truncInd, sizeof(truncInd) - 1);
				c += truncWidth;
			}
		}
	}

	struct UiPickEntry {
		struct FatFileLocator locator;
		uint32_t size;
		bool isDir;
		char name[FATFS_NAME_BUF_LEN];
	};

	static struct FatfsDir *uiPrvPickDirOpen(struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *dirLoc)
	{
		return dirLoc ? fatfsDirOpenWithLocator(vol, dirLoc) : fatfsDirOpen(vol, rootPath);
	}

	static bool uiPrvImageFileVisibleInDir(struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *dirLoc, const char *name)
	{
		(void)vol;
		(void)rootPath;
		(void)dirLoc;
		return uiPrvImageFileName(name);
	}

	static bool uiPrvPickEntryRead(struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *dirLoc, struct FatfsDir *dir, UiFileNameFilterF filterF, struct UiPickEntry *entry)
	{
		uint8_t attrs;

		while (fatfsDirRead(dir, entry->name, &entry->size, &attrs, &entry->locator)) {
			if ((attrs & FATFS_ATTR_VOL_LBL) || uiPrvHiddenEntry(entry->name, attrs))
				continue;
			if (attrs & FATFS_ATTR_DIR) {
				entry->isDir = true;
				return true;
			}
			if (!filterF || (filterF == uiPrvImageFileName ? uiPrvImageFileVisibleInDir(vol, rootPath, dirLoc, entry->name) : filterF(entry->name))) {
				entry->isDir = false;
				return true;
			}
		}

		return false;
	}

	static uint32_t uiPrvPickEntryList(struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *dirLoc, UiFileNameFilterF filterF, struct UiFileListCtx *ctx)
	{
		struct FatfsDir *dir = uiPrvPickDirOpen(vol, rootPath, dirLoc);
		struct UiPickEntry entry;
		uint32_t seen = 0;

		if (!dir)
			return 0;

		while (uiPrvPickEntryRead(vol, rootPath, dirLoc, dir, filterF, &entry)) {
			if (!(++seen & 0x0f))
				badgeLedsTick();
			if (!uiPrvFileListAppend(ctx, entry.name, entry.size, entry.isDir, &entry.locator))
				break;
		}
		fatfsDirClose(dir);
		return ctx->count;
	}

	static uint32_t uiPrvPickEntryCount(struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *dirLoc, UiFileNameFilterF filterF, struct UiPickEntry *entry)
	{
		struct FatfsDir *dir = uiPrvPickDirOpen(vol, rootPath, dirLoc);
		uint32_t count = 0;

		if (!dir)
			return 0;

		while (uiPrvPickEntryRead(vol, rootPath, dirLoc, dir, filterF, entry)) {
			if (!(++count & 0x0f))
				badgeLedsTick();
		}
		fatfsDirClose(dir);
		return count;
	}

	static bool uiPrvPickEntryAt(struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *dirLoc, UiFileNameFilterF filterF, uint32_t wantedIdx, struct UiPickEntry *entry)
	{
		struct FatfsDir *dir = uiPrvPickDirOpen(vol, rootPath, dirLoc);
		bool found = false;

		if (!dir)
			return false;

		while (uiPrvPickEntryRead(vol, rootPath, dirLoc, dir, filterF, entry)) {
			if (!wantedIdx--) {
				found = true;
				break;
			}
			if (!(wantedIdx & 0x0f))
				badgeLedsTick();
		}
		fatfsDirClose(dir);
		return found;
	}

	static void uiPrvAppendPathComponent(char *path, uint32_t pathBufSz, const char *name)
	{
		uint32_t pathLen = strlen(path), nameLen = strlen(name);

		if (pathLen + 1 >= pathBufSz)
			return;
		if (pathLen > 1)
			path[pathLen++] = '/';
		if (pathLen + nameLen < pathBufSz)
			memcpy(path + pathLen, name, nameLen + 1);
		else if (pathLen + 4 <= pathBufSz)
			strcpy(path + pathLen, "...");
		else if (pathLen)
			path[pathLen - 1] = 0;
	}

	static bool uiPrvBrowserMakePath(char *dst, uint32_t dstSz, const char *parent, const char *name)
	{
		uint32_t parentLen = strlen(parent), nameLen = name ? strlen(name) : 0;

		if (!parent || !dst || !dstSz || parentLen + (parentLen > 1 && nameLen ? 1u : 0u) + nameLen + 1u > dstSz)
			return false;
		memcpy(dst, parent, parentLen + 1u);
		if (nameLen)
			uiPrvAppendPathComponent(dst, dstSz, name);
		return true;
	}

	static const char *uiPrvBrowserBaseName(const char *path)
	{
		const char *base = path;

		for (; *path; path++)
			if (*path == '/')
				base = path + 1;
		return base;
	}

	static void uiPrvBrowserFavoritesLoad(struct FatfsVol *vol, struct UiBrowserFavorites *favorites)
	{
		struct FatfsFil *fil;
		uint32_t got, bytesRead = 0;
		char ch, *line;

		memset(favorites, 0, sizeof(*favorites));
		fil = fatfsFileOpen(vol, UI_BROWSER_FAVORITES_PATH, OPEN_MODE_READ);
		if (!fil)
			return;
		line = favorites->path[0];
		while (bytesRead < UI_BROWSER_FAVORITES_FILE_MAX &&
				favorites->count < UI_BROWSER_FAVORITES_MAX &&
				fatfsFileRead(fil, &ch, 1u, &got) && got) {
			uint32_t len = strlen(line);

			bytesRead += got;

			if (ch == '\r')
				continue;
			if (ch == '\n') {
				if (len && line[0] == '/')
					favorites->count++;
				else
					line[0] = 0;
				if (favorites->count < UI_BROWSER_FAVORITES_MAX)
					line = favorites->path[favorites->count];
				continue;
			}
			if (len + 1u < UI_BROWSER_ACTION_PATH_MAX) {
				line[len] = ch;
				line[len + 1u] = 0;
			}
		}
		if (favorites->count < UI_BROWSER_FAVORITES_MAX && line[0] == '/')
			favorites->count++;
		(void)fatfsFileClose(fil);
	}

	static bool uiPrvBrowserFavoritesSave(struct FatfsVol *vol, const struct UiBrowserFavorites *favorites)
	{
		struct FatfsFil *fil;
		uint32_t wrote;

		fil = fatfsFileOpen(vol, UI_BROWSER_FAVORITES_PATH,
			OPEN_MODE_WRITE | OPEN_MODE_CREATE | OPEN_MODE_TRUNCATE);
		if (!fil)
			return false;
		for (uint_fast8_t i = 0; i < favorites->count; i++) {
			uint32_t len = strlen(favorites->path[i]);

			if (!fatfsFileWrite(fil, favorites->path[i], len, &wrote) || wrote != len ||
					!fatfsFileWrite(fil, "\n", 1u, &wrote) || wrote != 1u) {
				(void)fatfsFileClose(fil);
				return false;
			}
		}
		return fatfsFileClose(fil);
	}

	static bool uiPrvBrowserFavoritesToggle(struct FatfsVol *vol, struct UiBrowserFavorites *favorites,
		const char *path, bool *addedP)
	{
		uint_fast8_t i;

		for (i = 0; i < favorites->count; i++) {
			if (strsCaselesslyCompareUtf(favorites->path[i], path, 0xffffffff))
				continue;
			memmove(favorites->path[i], favorites->path[i + 1u],
				(favorites->count - i - 1u) * sizeof(favorites->path[0]));
			favorites->count--;
			if (addedP)
				*addedP = false;
			return uiPrvBrowserFavoritesSave(vol, favorites);
		}
		if (favorites->count >= UI_BROWSER_FAVORITES_MAX || strlen(path) >= UI_BROWSER_ACTION_PATH_MAX)
			return false;
		uiPrvCopyStr(favorites->path[favorites->count++], UI_BROWSER_ACTION_PATH_MAX, path);
		if (addedP)
			*addedP = true;
		return uiPrvBrowserFavoritesSave(vol, favorites);
	}

	static uint_fast16_t uiPrvBrowserRecvKeypress(struct Canvas *cnv)
	{
		while (1) {
			uint_fast16_t keys = uiGetUiKeys();

			if (!keys) {
				uiPrvKeyRepeatReset();
				uiPrvRefreshHeaderClock(cnv);
				continue;
			}

			if (keys & KEY_BIT_A) {
				uint64_t start = getTime();
				while (uiGetUiKeys() & KEY_BIT_A) {
					if (getTime() - start >= UI_BROWSER_HOLD_TICKS) {
						uiPrvWaitKeysReleased();
						return UI_BROWSER_CONTEXT_KEY;
					}
					uiPrvRefreshHeaderClock(cnv);
				}
				return KEY_BIT_A;
			}
			if (keys & UI_KEY_BIT_CENTER) {
				uint64_t start = getTime();

				// Use the physical release edge here. Repeated debounced reads can
				// keep a brief FN tap alive long enough to look like a hold.
				while (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER) {
					if (getTime() - start >= UI_BROWSER_HOLD_TICKS) {
						uiPrvWaitKeysReleased();
						return UI_BROWSER_FN_MENU_KEY;
					}
					uiPrvRefreshHeaderClock(cnv);
				}
				return UI_BROWSER_FN_ACTION_KEY;
			}
			return uiPrvRecvMenuKeypress(cnv);
		}
	}

	static bool uiPrvBrowserCopyFile(struct Canvas *cnv, struct FatfsVol *vol,
		const char *sourcePath, const char *destPath)
	{
		struct FatfsFil *src = NULL, *dst = NULL, *exists;
		struct ToolWorkspaceSpan scratch;
		uint8_t *buf;
		uint32_t total, done = 0, now, got, wrote;
		bool ok = false;

		if (!strsCaselesslyCompareUtf(sourcePath, destPath, 0xffffffff)) {
			uiAlert(cnv, "Choose a different destination folder", DialogTypeOk);
			return false;
		}
		exists = fatfsFileOpen(vol, destPath, OPEN_MODE_READ);
		if (exists) {
			(void)fatfsFileClose(exists);
			uiAlert(cnv, "A file with that name already exists", DialogTypeOk);
			return false;
		}
		if (!toolWorkspaceAcquire(ToolWorkspaceWram, ToolWorkspaceOwnerFileBrowser, &scratch) || scratch.size < 512u) {
			uiAlert(cnv, "Not enough workspace to copy this file", DialogTypeOk);
			return false;
		}
		src = fatfsFileOpen(vol, sourcePath, OPEN_MODE_READ);
		dst = fatfsFileOpen(vol, destPath, OPEN_MODE_WRITE | OPEN_MODE_CREATE | OPEN_MODE_TRUNCATE);
		if (!src || !dst)
			goto out;
		buf = scratch.ptr;
		total = fatfsFileGetSize(src);
		while (done < total) {
			struct DcAppLoadingState loading = {
				.appName = "File Explorer",
				.title = "Copying file",
				.detail = uiPrvBrowserBaseName(sourcePath),
				.done = done,
				.total = total,
				.animationStep = done / scratch.size,
			};

			now = total - done;
			if (now > scratch.size)
				now = scratch.size;
			dcAppDrawLoadingCanvas(cnv, &loading);
			if (!fatfsFileRead(src, buf, now, &got) || got != now ||
					!fatfsFileWrite(dst, buf, now, &wrote) || wrote != now)
				goto out;
			done += now;
		}
		ok = true;
out:
		if (src)
			(void)fatfsFileClose(src);
		if (dst)
			(void)fatfsFileClose(dst);
		toolWorkspaceRelease(ToolWorkspaceWram, ToolWorkspaceOwnerFileBrowser);
		if (!ok) {
			(void)fatfsFileDelete(vol, destPath);
			uiAlert(cnv, "Copy failed", DialogTypeOk);
		}
		return ok;
	}

	static bool uiPrvBrowserEditName(struct Canvas *cnv, char *name, uint32_t nameSz)
	{
		uint32_t pos = 0, len = strlen(name);

		if (!len || len >= nameSz)
			return false;
		while (1) {
			uiPrvSetHeaderTitle("Rename");
			uiPrvReset(cnv, false);
			uiPrvDrawTruncText(cnv, uiPrvContentTop(cnv), 10, cnv->w - 20, name);
			cnv->foreColor = 9;
			uiPrvDrawOneChar(cnv, uiPrvContentTop(cnv) + uiPrvGlyphHeight(cnv) + 2, 10, name[pos]);
			cnv->foreColor = 15;
			uiPuts(cnv, cnv->h - 2u * uiPrvGlyphHeight(cnv) - 2u, 10, "Up/Dn char  L/R move  A add", -1);
			uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1u, 10, "SEL delete  START save  B cancel", -1);
			switch (uiPrvRecvMenuKeypress(cnv)) {
			case KEY_BIT_UP: name[pos] = name[pos] >= '~' ? ' ' : name[pos] + 1; break;
			case KEY_BIT_DOWN: name[pos] = name[pos] <= ' ' ? '~' : name[pos] - 1; break;
			case KEY_BIT_LEFT: if (pos) pos--; break;
			case KEY_BIT_RIGHT: if (pos + 1u < len) pos++; break;
			case KEY_BIT_A:
				if (len + 1u < nameSz) {
					memmove(name + pos + 1u, name + pos, len - pos + 1u);
					name[pos] = ' ';
					len++;
				}
				break;
			case KEY_BIT_SEL:
				if (len > 1u) { memmove(name + pos, name + pos + 1u, len - pos); len--; if (pos == len) pos--; }
				break;
			case KEY_BIT_START: return true;
			case KEY_BIT_B: return false;
			case UI_KEY_BIT_CENTER: uiPrvRequestToolExit(); return false;
			}
		}
	}

	static bool uiPrvBrowserContextMenu(struct Canvas *cnv, struct FatfsVol *vol,
		const char *parentPath, const struct MusicOption *entry, struct UiBrowserOps *ops)
	{
		enum { BrowserActionCopy, BrowserActionPaste, BrowserActionRename, BrowserActionDelete,
			BrowserActionFavorite, BrowserActionCancel };
		uint8_t actions[8];
		const char *labels[8];
		char fullPath[UI_BROWSER_ACTION_PATH_MAX], destPath[UI_BROWSER_ACTION_PATH_MAX];
		uint_fast8_t count = 0, selected, buttons = KEY_BIT_A | KEY_BIT_B;
		bool isFavorite = false, added;

		if (!uiPrvBrowserMakePath(fullPath, sizeof(fullPath), parentPath, entry ? entry->name : NULL)) {
			uiAlert(cnv, "Path is too long for File Explorer actions", DialogTypeOk);
			return false;
		}
		for (uint_fast8_t i = 0; i < ops->favorites.count; i++)
			if (!strsCaselesslyCompareUtf(ops->favorites.path[i], fullPath, 0xffffffff))
				isFavorite = true;
		if (entry) {
			actions[count] = BrowserActionCopy; labels[count++] = "Copy";
			actions[count] = BrowserActionRename; labels[count++] = "Rename";
			actions[count] = BrowserActionDelete; labels[count++] = "Delete";
		}
		if (ops->clipboard.valid) {
			actions[count] = BrowserActionPaste; labels[count++] = "Paste here";
		}
		actions[count] = BrowserActionFavorite;
		labels[count++] = isFavorite ? "Remove favorite" : "Add favorite";
		actions[count] = BrowserActionCancel; labels[count++] = "Cancel";

		uiPrvSetHeaderTitle("File actions");
		uiPrvReset(cnv, false);
		for (uint_fast8_t i = 0; i < count; i++)
			uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, labels[i], -1);
		selected = uiPrvMenu(cnv, 0, count, &buttons);
		if (uiPrvToolExitRequested() || buttons == KEY_BIT_B)
			return false;
		switch (actions[selected]) {
		case BrowserActionCopy:
			if (entry->isDir) {
				uiAlert(cnv, "Folder copy is not supported yet", DialogTypeOk);
				return false;
			}
			ops->clipboard.valid = true;
			ops->clipboard.isDir = false;
			uiPrvCopyStr(ops->clipboard.path, sizeof(ops->clipboard.path), fullPath);
			return false;
		case BrowserActionPaste:
			if (ops->clipboard.isDir || !uiPrvBrowserMakePath(destPath, sizeof(destPath), parentPath,
					uiPrvBrowserBaseName(ops->clipboard.path))) {
				uiAlert(cnv, "Cannot paste that item here", DialogTypeOk);
				return false;
			}
			return uiPrvBrowserCopyFile(cnv, vol, ops->clipboard.path, destPath);
		case BrowserActionRename:
			if (entry->isDir) {
				uiAlert(cnv, "Folder rename is not supported yet", DialogTypeOk);
				return false;
			}
			{
				char newName[UI_PICK_FILE_NAME_BUF_SZ];
				struct FatfsDir *dir;

				uiPrvCopyStr(newName, sizeof(newName), entry->name);
				if (!uiPrvBrowserEditName(cnv, newName, sizeof(newName)) || !strcmp(newName, entry->name))
					return false;
				if (!uiPrvBrowserMakePath(destPath, sizeof(destPath), parentPath, newName) ||
						!uiPrvBrowserCopyFile(cnv, vol, fullPath, destPath))
					return false;
				dir = fatfsDirOpen(vol, parentPath);
				if (!dir || !fatfsFileDeleteAt(dir, entry->name)) {
					if (dir) (void)fatfsDirClose(dir);
					uiAlert(cnv, "Renamed copy created, but source could not be deleted", DialogTypeOk);
					return true;
				}
				(void)fatfsDirClose(dir);
				return true;
			}
		case BrowserActionDelete:
			if (!uiAlert(cnv, entry->isDir ? "Delete this empty folder?" : "Delete this file?", DialogTypeYesNo))
				return false;
			{
				struct FatfsDir *dir = fatfsDirOpen(vol, parentPath);
				bool deleted = dir && (entry->isDir ? fatfsDirDeleteAt(dir, entry->name) : fatfsFileDeleteAt(dir, entry->name));

				if (dir) (void)fatfsDirClose(dir);
				if (!deleted)
					uiAlert(cnv, entry->isDir ? "Folder must be empty to delete" : "Delete failed", DialogTypeOk);
				return deleted;
			}
		case BrowserActionFavorite:
			if (!uiPrvBrowserFavoritesToggle(vol, &ops->favorites, fullPath, &added))
				uiAlert(cnv, "Could not update favorites", DialogTypeOk);
			else
				uiAlert(cnv, added ? "Added to favorites" : "Removed from favorites", DialogTypeOk);
			return false;
		default:
			return false;
		}
	}

	static bool uiPrvBrowserSettingsMenu(struct Canvas *cnv, struct FatfsVol *vol,
		const char *parentPath, struct UiBrowserOps *ops)
	{
		enum { BrowserSettingsFavorites, BrowserSettingsFolderFavorite, BrowserSettingsStart, BrowserSettingsBack };
		struct Settings settings;
		uint_fast8_t selected = 0;

		while (1) {
			const char *labels[4];
			uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;
			bool isFavorite = false, added;

			for (uint_fast8_t i = 0; i < ops->favorites.count; i++)
				if (!strsCaselesslyCompareUtf(ops->favorites.path[i], parentPath, 0xffffffff))
					isFavorite = true;
			settingsGet(&settings);
			labels[BrowserSettingsFavorites] = "Favorites";
			labels[BrowserSettingsFolderFavorite] = isFavorite ? "Remove current folder" : "Favorite current folder";
			labels[BrowserSettingsStart] = settings.fileBrowserStartFavorites ?
				"Start in Favorites: ON" : "Start in Favorites: OFF";
			labels[BrowserSettingsBack] = "Back";
			uiPrvSetHeaderTitle("App Settings");
			uiPrvReset(cnv, false);
			for (uint_fast8_t i = 0; i < BrowserSettingsBack + 1u; i++)
				uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, labels[i], -1);
			selected = uiPrvMenu(cnv, selected, BrowserSettingsBack + 1u, &button);
			if (uiPrvToolExitRequested() || button == KEY_BIT_B || selected == BrowserSettingsBack)
				return false;
			if (selected == BrowserSettingsFavorites) {
				ops->showFavorites = true;
				return true;
			}
			if (selected == BrowserSettingsFolderFavorite) {
				if (!uiPrvBrowserFavoritesToggle(vol, &ops->favorites, parentPath, &added))
					uiAlert(cnv, "Could not update favorites", DialogTypeOk);
				else
					uiAlert(cnv, added ? "Added to favorites" : "Removed from favorites", DialogTypeOk);
				continue;
			}
			settings.fileBrowserStartFavorites = !settings.fileBrowserStartFavorites;
			(void)settingsSet(&settings);
		}
	}

	enum UiBrowserFnResult {
		UiBrowserFnResume,
		UiBrowserFnReload,
		UiBrowserFnFavorites,
		UiBrowserFnExit,
	};

	static enum UiBrowserFnResult uiPrvBrowserFnMenu(struct Canvas *cnv, struct FatfsVol *vol,
		const char *parentPath, const struct MusicOption *entry, struct UiBrowserOps *ops)
	{
		enum { BrowserFnResume, BrowserFnActions, BrowserFnSettings, BrowserFnAudio,
			BrowserFnScreen, BrowserFnLeds, BrowserFnTheme, BrowserFnExit };
		static const char *labels[] = { "Resume", "File actions", "App Settings", "Audio", "Display", "LEDs", "Theme", "Exit to Main Menu" };
		uint_fast8_t selected = 0;

		while (1) {
			uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;

			uiPrvSetHeaderTitle("FN Menu");
			uiPrvReset(cnv, false);
			for (uint_fast8_t i = 0; i < sizeof(labels) / sizeof(*labels); i++)
				uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, labels[i], -1);
			selected = uiPrvMenu(cnv, selected, sizeof(labels) / sizeof(*labels), &button);
			if (uiPrvToolExitRequested())
				return UiBrowserFnExit;
			if (button == KEY_BIT_B || selected == BrowserFnResume)
				return UiBrowserFnResume;
			if (selected == BrowserFnExit)
				return UiBrowserFnExit;
			if (selected == BrowserFnActions) {
				if (uiPrvBrowserContextMenu(cnv, vol, parentPath, entry, ops))
					return ops->showFavorites ? UiBrowserFnFavorites : UiBrowserFnReload;
				continue;
			}
			if (selected == BrowserFnSettings) {
				if (uiPrvBrowserSettingsMenu(cnv, vol, parentPath, ops))
					return UiBrowserFnFavorites;
				continue;
			}
			{
				struct Settings settings;

				settingsGet(&settings);
				if (selected == BrowserFnAudio)
					uiPrvAudioSettings(cnv, &settings);
				else if (selected == BrowserFnScreen)
					uiPrvScreenSettings(cnv, &settings);
				else if (selected == BrowserFnLeds)
					uiPrvLedSettings(cnv, &settings);
				else
					uiPrvThemeSettings(cnv, &settings);
				(void)settingsSet(&settings);
				if (uiPrvToolExitRequested())
					return UiBrowserFnExit;
			}
		}
	}

	static bool uiPrvBrowserChooseLocation(struct Canvas *cnv, struct FatfsVol *vol,
		struct UiBrowserOps *ops, char *pathOut, uint32_t pathOutSz)
	{
		uint_fast8_t selected = 0;
		uint_fast8_t drawnSelected = 0xffu;
		bool redraw = true;

		while (1) {
			uint_fast8_t cancelOption = ops->favorites.count + 1u;
			uint_fast8_t totalOptions = cancelOption + 1u;
			uint_fast16_t key;
			const char *selectedPath;

			if (selected >= totalOptions)
				selected = totalOptions - 1u;

			if (redraw) {
				uiPrvSetHeaderTitle("Favorites");
				uiPrvReset(cnv, false);
				uiPuts(cnv, uiPrvMenuRow(cnv, 0), 10, "Browse SD card", -1);
				for (uint_fast8_t i = 0; i < ops->favorites.count; i++)
					uiPrvDrawTruncText(cnv, uiPrvMenuRow(cnv, i + 1u), 10, cnv->w - 20, ops->favorites.path[i]);
				uiPuts(cnv, uiPrvMenuRow(cnv, cancelOption), 10, "Back", -1);
				uiPrvDrawMenuFooter(cnv, KEY_BIT_A | KEY_BIT_B);
				drawnSelected = 0xffu;
				redraw = false;
			}
			if (drawnSelected != selected) {
				if (drawnSelected < totalOptions) {
					cnv->foreColor = 0;
					uiPrvDrawOneChar(cnv, uiPrvMenuRow(cnv, drawnSelected), 1, MENU_SELECTION_CHAR);
				}
				cnv->foreColor = 15;
				uiPrvDrawOneChar(cnv, uiPrvMenuRow(cnv, selected), 1, MENU_SELECTION_CHAR);
				drawnSelected = selected;
			}
			key = uiPrvBrowserRecvKeypress(cnv);
			selectedPath = selected && selected < cancelOption ? ops->favorites.path[selected - 1u] : "/";

			switch (key) {
			case KEY_BIT_A:
				if (selected == cancelOption)
					return false;
				uiPrvCopyStr(pathOut, pathOutSz, selectedPath);
				return true;

			case KEY_BIT_B:
				return false;

			case KEY_BIT_DOWN:
				selected = (selected + 1u) % totalOptions;
				break;

			case KEY_BIT_UP:
				selected = selected ? selected - 1u : totalOptions - 1u;
				break;

			case UI_BROWSER_CONTEXT_KEY:
			case UI_BROWSER_FN_ACTION_KEY:
			case UI_KEY_BIT_CENTER:
				(void)uiPrvBrowserContextMenu(cnv, vol, selectedPath, NULL, ops);
				redraw = true;
				break;

			case UI_BROWSER_FN_MENU_KEY:
				if (uiPrvBrowserFnMenu(cnv, vol, selectedPath, NULL, ops) == UiBrowserFnExit)
					return false;
				ops->showFavorites = false;
				redraw = true;
				break;

			default:
				break;
			}
			if (uiPrvToolExitRequested())
				return false;
		}
	}

	static bool uiPrvBrowserSplitPath(const char *path, char *parent, uint32_t parentSz,
		char *name, uint32_t nameSz)
	{
		const char *base = uiPrvBrowserBaseName(path);
		uint32_t parentLen = (uint32_t)(base - path);

		if (!base[0] || parentLen >= parentSz || strlen(base) >= nameSz)
			return false;
		if (parentLen > 1u)
			parentLen--;
		memcpy(parent, path, parentLen);
		parent[parentLen] = 0;
		if (!parent[0])
			strcpy(parent, "/");
		uiPrvCopyStr(name, nameSz, base);
		return true;
	}

	static void uiPrvPageSelection(uint32_t *selectedItemP, uint32_t *topItemP, uint32_t totalItems, uint32_t itemsOnscreen, bool forward)
	{
		uint32_t selectedItem = *selectedItemP, topItem = *topItemP, selectedOffset;

		if (!totalItems || !itemsOnscreen)
			return;

		selectedOffset = selectedItem > topItem ? selectedItem - topItem : 0;
		if (forward) {
			if (topItem + itemsOnscreen < totalItems)
				topItem += itemsOnscreen;
		}
		else {
			if (topItem > itemsOnscreen)
				topItem -= itemsOnscreen;
			else
				topItem = 0;
		}
		if (topItem + itemsOnscreen > totalItems)
			topItem = totalItems > itemsOnscreen ? totalItems - itemsOnscreen : 0;
		selectedItem = topItem + selectedOffset;
		if (selectedItem >= totalItems)
			selectedItem = totalItems - 1;

		*selectedItemP = selectedItem;
		*topItemP = topItem;
	}

	static bool uiPrvPickFile(struct Canvas *cnv, struct FatfsVol *vol, const char *rootPath, UiFileNameFilterF filterF, const char *emptyMsg, bool ignoreRootBack, struct FatFileLocator *locatorOut, char *nameOut, uint32_t nameOutSz, char *parentPathOut, uint32_t parentPathOutSz, UiFileDisplayNameF displayNameF, bool selectCurrentFolder, struct UiBrowserOps *browserOps)
	{
		struct FatFileLocator dirStack[UI_BROWSER_MAX_DEPTH];
		uint16_t pathLenStack[UI_BROWSER_MAX_DEPTH];
		struct ToolWorkspaceSpan pathMem = toolWorkspaceGet(ToolWorkspaceCartRamUpper);
		struct ToolWorkspaceSpan listMem = toolWorkspaceGet(ToolWorkspaceCartRamLower);
		char *path = (char*)pathMem.ptr;
		struct MusicOption *head = NULL;
		uint32_t numItems, topItem = 0, selectedItem = 0, depth = 0, prevTopItem, prevSelOnscreenItem;
		uint_fast8_t itemHeight, itemsOnscreen, pathTop, listTop, itemLeft;
		bool haveDirLoc = false, folderAction;
		struct FontGlyphInfo gi;

		if (!path || pathMem.size < UI_PICK_FILE_PATH_BUF_SZ || !listMem.ptr || listMem.size < sizeof(struct MusicOption)) {
			uiAlert(cnv, "Tool workspace is too small for File Manager", DialogTypeOk);
			return false;
		}
		uiPrvCartRamOwnerClear("file picker workspace");

		uiPrvCopyStr(path, UI_PICK_FILE_PATH_BUF_SZ, rootPath);
reload_dir:
		{
			struct UiFileListCtx ctx;

			memset(&ctx, 0, sizeof(ctx));
			ctx.nextAvail = (struct MusicOption*)listMem.ptr;
			ctx.spaceAvail = listMem.size;
			numItems = uiPrvPickEntryList(vol, rootPath, haveDirLoc ? &dirStack[depth - 1] : NULL, filterF, &ctx);
			head = ctx.head;
			if (ctx.overflow)
				uiAlert(cnv, "Folder has too many entries; showing what fits", DialogTypeOk);
		}
		folderAction = selectCurrentFolder || (browserOps && !numItems);
		if (!numItems && !depth && !folderAction) {
			uiAlert(cnv, emptyMsg, DialogTypeOk);
			return false;
		}

		topItem = 0;
		selectedItem = 0;
		itemHeight = uiPrvGlyphHeight(cnv) + 1;
		itemLeft = fontGetGlyphInfo(&gi, cnv->font, MENU_SELECTION_CHAR) ? gi.width + 2 : 10;
		pathTop = uiPrvContentTop(cnv);
		listTop = pathTop + itemHeight;
		itemsOnscreen = (cnv->h - listTop - 2u * itemHeight) / itemHeight;
		if (!itemsOnscreen) {
			uiAlert(cnv, "Display area too small for File Manager", DialogTypeOk);
			return false;
		}
		if (itemsOnscreen > numItems + (depth ? 1 : 0) + (folderAction ? 1 : 0))
			itemsOnscreen = numItems + (depth ? 1 : 0) + (folderAction ? 1 : 0);
		prevTopItem = topItem + 1;
		prevSelOnscreenItem = selectedItem - topItem + 1;

		while (1) {
			uint32_t i, totalItems = numItems + (depth ? 1 : 0) + (folderAction ? 1 : 0), selectedOnscreenItem = selectedItem - topItem;

			if (prevTopItem != topItem) {
				uint_fast8_t scrollWidth;

				uiPrvReset(cnv, false);
				uiPrvDrawTruncText(cnv, pathTop, 10, cnv->w - 10, path);
				scrollWidth = totalItems > itemsOnscreen ? uiPrvDrawScrollbar(cnv, listTop, totalItems, topItem, itemsOnscreen) : 0;

				for (i = 0; i < itemsOnscreen && topItem + i < totalItems; i++) {
					uint32_t item = topItem + i;
					struct MusicOption *draw;

					cnv->foreColor = 12;
					if (folderAction && item == 0) {
						uiPrvDrawDirLabel(cnv, listTop + i * itemHeight, itemLeft, cnv->w - scrollWidth - itemLeft,
							browserOps ? "FOLDER ACTIONS" : "USE THIS FOLDER");
						continue;
					}
					if (depth && item == (folderAction ? 1u : 0u)) {
						uiPrvDrawDirLabel(cnv, listTop + i * itemHeight, itemLeft, cnv->w - scrollWidth - itemLeft, "...");
						continue;
					}
					draw = head;
					for (uint32_t skip = item - (folderAction ? 1u : 0u) - (depth ? 1u : 0u); skip && draw; skip--)
						draw = draw->next;
					if (!draw)
						continue;
					if (draw->isDir)
						uiPrvDrawDirLabel(cnv, listTop + i * itemHeight, itemLeft, cnv->w - scrollWidth - itemLeft, draw->name);
					else {
						char displayName[UI_PICK_FILE_NAME_BUF_SZ];
						const char *name = draw->name;

						if (displayNameF) {
							displayNameF(displayName, sizeof(displayName), draw->name);
							name = displayName;
						}
						uiPrvDrawTruncText(cnv, listTop + i * itemHeight, itemLeft, cnv->w - scrollWidth - itemLeft, name);
					}
				}
				uiPrvDrawMenuFooter(cnv, KEY_BIT_A | KEY_BIT_B);

				prevSelOnscreenItem = selectedOnscreenItem + 1;
			}
			prevTopItem = topItem;
			if (prevSelOnscreenItem != selectedOnscreenItem) {
				if (prevSelOnscreenItem < itemsOnscreen) {
					cnv->foreColor = 0;
					uiPrvDrawOneChar(cnv, listTop + itemHeight * prevSelOnscreenItem, 1, MENU_SELECTION_CHAR);
				}
				cnv->foreColor = 15;
				uiPrvDrawOneChar(cnv, listTop + itemHeight * selectedOnscreenItem, 1, MENU_SELECTION_CHAR);
			}
			prevSelOnscreenItem = selectedOnscreenItem;

			switch (browserOps ? uiPrvBrowserRecvKeypress(cnv) : uiPrvRecvMenuKeypress(cnv)) {
				case UI_BROWSER_CONTEXT_KEY:
				case UI_BROWSER_FN_ACTION_KEY:
				case UI_KEY_BIT_CENTER:
					if (browserOps) {
						struct MusicOption *entry = NULL;

						if (!(folderAction && selectedItem == 0) &&
								!(depth && selectedItem == (folderAction ? 1u : 0u)))
							entry = uiPrvFileOptionAt(head, 0, selectedItem - (folderAction ? 1u : 0u) - (depth ? 1u : 0u));
						if (uiPrvBrowserContextMenu(cnv, vol, path, entry, browserOps)) {
							if (browserOps->showFavorites)
								return false;
							goto reload_dir;
						}
						uiPrvSetHeaderTitle("File Manager");
						prevTopItem = topItem + 1u;
					}
					break;

				case UI_BROWSER_FN_MENU_KEY:
					if (browserOps) {
						struct MusicOption *entry = NULL;
						enum UiBrowserFnResult result;

						if (!(folderAction && selectedItem == 0) &&
								!(depth && selectedItem == (folderAction ? 1u : 0u)))
							entry = uiPrvFileOptionAt(head, 0, selectedItem - (folderAction ? 1u : 0u) - (depth ? 1u : 0u));
						result = uiPrvBrowserFnMenu(cnv, vol, path, entry, browserOps);
						if (result == UiBrowserFnExit) {
							uiPrvRequestToolExit();
							return false;
						}
						if (result == UiBrowserFnFavorites)
							return false;
						if (result == UiBrowserFnReload)
							goto reload_dir;
						uiPrvSetHeaderTitle("File Manager");
						prevTopItem = topItem + 1u;
						break;
					}
					uiPrvRequestToolExit();
					return false;

				case KEY_BIT_A:
					if (selectCurrentFolder && selectedItem == 0) {
						if (parentPathOut && parentPathOutSz)
							uiPrvCopyStr(parentPathOut, parentPathOutSz, path);
						return true;
					}
					if (folderAction && selectedItem == 0)
						break;
					if (depth && selectedItem == (folderAction ? 1u : 0u)) {
						depth--;
						haveDirLoc = depth != 0;
						path[pathLenStack[depth]] = 0;
						goto reload_dir;
					}
					{
						struct MusicOption *entry = uiPrvFileOptionAt(head, 0,
							selectedItem - (folderAction ? 1u : 0u) - (depth ? 1u : 0u));

					if (!entry)
						break;
					if (entry->isDir) {
						if (depth < UI_BROWSER_MAX_DEPTH) {
							pathLenStack[depth] = strlen(path);
							dirStack[depth++] = entry->locator;
							haveDirLoc = true;
							uiPrvAppendPathComponent(path, UI_PICK_FILE_PATH_BUF_SZ, entry->name);
							goto reload_dir;
						}
						uiAlert(cnv, "Folder nesting too deep", DialogTypeOk);
						prevTopItem = topItem + 1;
					}
					else {
						*locatorOut = entry->locator;
						if (nameOut && nameOutSz)
							uiPrvCopyStr(nameOut, nameOutSz, entry->name);
						if (parentPathOut && parentPathOutSz)
							uiPrvCopyStr(parentPathOut, parentPathOutSz, path);
						return true;
					}
					}
					break;

				case KEY_BIT_B:
					if (depth) {
						depth--;
						haveDirLoc = depth != 0;
						path[pathLenStack[depth]] = 0;
						goto reload_dir;
					}
					if (ignoreRootBack)
						break;
					return false;

				case KEY_BIT_DOWN:
					if (selectedItem + 1 < totalItems) {
						selectedItem++;
						if (selectedItem >= topItem + itemsOnscreen) {
							topItem += itemsOnscreen;
							if (topItem + itemsOnscreen > totalItems)
								topItem = totalItems > itemsOnscreen ? totalItems - itemsOnscreen : 0;
						}
					}
					else if (!mUiKeyRepeated && totalItems) {
						selectedItem = 0;
						topItem = 0;
					}
					break;

				case KEY_BIT_UP:
					if (selectedItem) {
						selectedItem--;
						if (selectedItem < topItem) {
							if (topItem > itemsOnscreen)
								topItem -= itemsOnscreen;
							else
								topItem = 0;
							if (selectedItem < topItem)
								topItem = selectedItem;
						}
					}
					else if (!mUiKeyRepeated && totalItems) {
						selectedItem = totalItems - 1;
						topItem = totalItems > itemsOnscreen ? totalItems - itemsOnscreen : 0;
					}
					break;

				case KEY_BIT_RIGHT:
					uiPrvPageSelection(&selectedItem, &topItem, totalItems, itemsOnscreen, true);
					break;

				case KEY_BIT_LEFT:
					uiPrvPageSelection(&selectedItem, &topItem, totalItems, itemsOnscreen, false);
					break;
			}
		}
	}
#endif

#define LED_BRIGHTNESS_RAW_MIN			15
#define LED_BRIGHTNESS_RAW_MAX			255
#define LED_BRIGHTNESS_MENU_MIN			1
#define LED_BRIGHTNESS_MENU_MAX			10
#define LED_COLOR_RAW_MAX				255
#define LED_COLOR_MENU_MAX				25
#define LED_SPEED_MENU_MIN				1
#define LED_SPEED_MENU_MAX				10

static uint_fast8_t uiPrvLedBrightnessToMenu(uint_fast8_t brightness)
{
	if (brightness <= LED_BRIGHTNESS_RAW_MIN)
		return LED_BRIGHTNESS_MENU_MIN;
	if (brightness >= LED_BRIGHTNESS_RAW_MAX)
		return LED_BRIGHTNESS_MENU_MAX;

	return LED_BRIGHTNESS_MENU_MIN + (((uint32_t)brightness - LED_BRIGHTNESS_RAW_MIN) * (LED_BRIGHTNESS_MENU_MAX - LED_BRIGHTNESS_MENU_MIN) + (LED_BRIGHTNESS_RAW_MAX - LED_BRIGHTNESS_RAW_MIN) / 2) / (LED_BRIGHTNESS_RAW_MAX - LED_BRIGHTNESS_RAW_MIN);
}

static uint8_t uiPrvLedBrightnessFromMenu(uint_fast8_t brightness)
{
	if (brightness <= LED_BRIGHTNESS_MENU_MIN)
		return LED_BRIGHTNESS_RAW_MIN;
	if (brightness >= LED_BRIGHTNESS_MENU_MAX)
		return LED_BRIGHTNESS_RAW_MAX;

	return LED_BRIGHTNESS_RAW_MIN + (((uint32_t)brightness - LED_BRIGHTNESS_MENU_MIN) * (LED_BRIGHTNESS_RAW_MAX - LED_BRIGHTNESS_RAW_MIN) + (LED_BRIGHTNESS_MENU_MAX - LED_BRIGHTNESS_MENU_MIN) / 2) / (LED_BRIGHTNESS_MENU_MAX - LED_BRIGHTNESS_MENU_MIN);
}

static uint_fast8_t uiPrvLedColorToMenu(uint_fast8_t color)
{
	if (color >= LED_COLOR_RAW_MAX)
		return LED_COLOR_MENU_MAX;

	return ((uint32_t)color * LED_COLOR_MENU_MAX + LED_COLOR_RAW_MAX / 2) / LED_COLOR_RAW_MAX;
}

static uint8_t uiPrvLedColorFromMenu(uint_fast8_t color)
{
	if (color >= LED_COLOR_MENU_MAX)
		return LED_COLOR_RAW_MAX;

	return ((uint32_t)color * LED_COLOR_RAW_MAX + LED_COLOR_MENU_MAX / 2) / LED_COLOR_MENU_MAX;
}

static void __attribute__((noinline)) uiPrvLedSettings(struct Canvas *cnv, struct Settings *settings)
{
	int_fast8_t selOption = 0;

	uiPrvSetHeaderTitle("LEDs");
	if (settings->ledMode >= LedModeNumModes)
		settings->ledMode = LedModeOff;
	if (settings->ledSpeed < LED_SPEED_MENU_MIN || settings->ledSpeed > LED_SPEED_MENU_MAX)
		settings->ledSpeed = 4;
	settings->ledBrightness = uiPrvLedBrightnessFromMenu(uiPrvLedBrightnessToMenu(settings->ledBrightness));

	uiPrvReset(cnv, false);

	while (1) {

		int_fast8_t numOptions = 0, doneOption, ledPatternOption, ledColorOption, ledSpeedOption, ledBrightnessOption;
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;

		uiPrvReset(cnv, false);

		doneOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, doneOption), 10, "DONE", -1);

		ledPatternOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, ledPatternOption), 10, "PATTERN:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, uiPrvMenuRow(cnv, ledPatternOption), 111, "%s        ", badgeLedsModeName(settings->ledMode));

		ledColorOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, ledColorOption), 10, "THEME:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, uiPrvMenuRow(cnv, ledColorOption), 111, "%s        ", badgeLedsColorName(settings->ledColor));

		ledSpeedOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, ledSpeedOption), 10, "LED SPEED:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, uiPrvMenuRow(cnv, ledSpeedOption), 111, "%u         ", settings->ledSpeed);

		ledBrightnessOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, ledBrightnessOption), 10, "LED BRIGHT:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, uiPrvMenuRow(cnv, ledBrightnessOption), 111, "%u         ", uiPrvLedBrightnessToMenu(settings->ledBrightness));

		if (selOption >= numOptions)
			selOption = numOptions - 1;
		selOption = uiPrvMenu(cnv, selOption, numOptions, &button);
		if (uiPrvToolExitRequested())
			return;
		if (button == KEY_BIT_B || selOption == doneOption)
			return;

		if (selOption == ledPatternOption) {

			if (button == KEY_BIT_LEFT) {
				if (settings->ledMode)
					settings->ledMode--;
				else
					settings->ledMode = LedModeNumModes - 1u;
			}
			else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
				if (settings->ledMode < LedModeNumModes - 1)
					settings->ledMode++;
				else
					settings->ledMode = LedModeOff;
			}
			badgeLedsApplySettings(settings, true);
		}

		if (selOption == ledColorOption) {

			if (button == KEY_BIT_LEFT) {
				if (settings->ledColor)
					settings->ledColor--;
				else
					settings->ledColor = LedColorNumColors - 1u;
			}
			else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
				if (settings->ledColor < LedColorNumColors - 1)
					settings->ledColor++;
				else
					settings->ledColor = LedColorCustom;
			}
			badgeLedsApplySettings(settings, true);
		}

		if (selOption == ledBrightnessOption) {
			uint_fast8_t brightness = uiPrvLedBrightnessToMenu(settings->ledBrightness);

			if (button == KEY_BIT_LEFT) {
				if (brightness > LED_BRIGHTNESS_MENU_MIN)
					brightness--;
				else
					continue;
			}
			else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
				if (brightness < LED_BRIGHTNESS_MENU_MAX)
					brightness++;
				else
					continue;
			}

			settings->ledBrightness = uiPrvLedBrightnessFromMenu(brightness);
			badgeLedsApplySettings(settings, true);
		}

		if (selOption == ledSpeedOption) {

			if (button == KEY_BIT_LEFT) {
				if (settings->ledSpeed > LED_SPEED_MENU_MIN)
					settings->ledSpeed--;
				else
					continue;
			}
			else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
				if (settings->ledSpeed < LED_SPEED_MENU_MAX)
					settings->ledSpeed++;
				else
					continue;
			}

			badgeLedsApplySettings(settings, true);
		}
	}
}

static uint8_t uiPrvDisplayTimeoutAdjust(uint8_t value, int8_t direction)
{
	static const uint8_t options[] = {15u, 30u, 60u, 120u, 240u};
	uint32_t index = 0;

	for (uint32_t i = 0; i < sizeof(options) / sizeof(options[0]); i++)
		if (value == options[i]) {
			index = i;
			break;
		}
	if (direction < 0)
		index = index ? index - 1u : sizeof(options) / sizeof(options[0]) - 1u;
	else
		index = (index + 1u) % (sizeof(options) / sizeof(options[0]));
	return options[index];
}

static const char *uiPrvScreenSaverName(uint8_t saver)
{
	static const char *const names[] = {"OFF", "STARFIELD", "CUBE", "SPYRO", "GIF", "IMAGE FOLDER",
		"DVD BOUNCE", "SCROLL PATTERN"};

	return saver < ScreenSaverNumModes ? names[saver] : names[ScreenSaverStarfield];
}

static bool uiPrvScreenSaverFolderFileName(const char *name)
{
	(void)name;
	return false;
}

static void uiPrvPickScreenSaverMedia(struct Canvas *cnv, struct Settings *settings, bool folder)
{
	struct FatfsVol *vol = uiPrvMountCard(cnv, false);
	struct FatFileLocator locator;
	char name[FATFS_NAME_BUF_LEN];
	char parentPath[UI_PICK_FILE_PATH_BUF_SZ];
	char *destination = folder ? settings->screenSaverImageFolder : settings->screenSaverGifPath;
	bool picked;

	if (!vol)
		return;
	picked = uiPrvPickFile(cnv, vol, "/", folder ? uiPrvScreenSaverFolderFileName : imageViewerFileName,
		folder ? "No folders found on the SD card" : "No compatible GIF/DCA media found", false,
		&locator, name, sizeof(name), parentPath, sizeof(parentPath), NULL, folder, NULL);
	if (picked && folder) {
		if (strlen(parentPath) >= SETTINGS_SCREENSAVER_PATH_MAX)
			uiAlert(cnv, "Selected folder path is too long", DialogTypeOk);
		else
			uiPrvCopyStr(destination, SETTINGS_SCREENSAVER_PATH_MAX, parentPath);
	}
	else if (picked) {
		uint32_t parentLen = !strcmp(parentPath, "/") ? 0u : strlen(parentPath);
		uint32_t nameLen = strlen(name);

		if (parentLen + 1u + nameLen >= SETTINGS_SCREENSAVER_PATH_MAX) {
			uiAlert(cnv, "Selected GIF/DCA path is too long", DialogTypeOk);
			goto out_unmount;
		}
		if (parentLen)
			memcpy(destination, parentPath, parentLen);
		destination[parentLen] = '/';
		memcpy(destination + parentLen + 1u, name, nameLen + 1u);
	}

out_unmount:
	(void)uiPrvCardPreUnmount();
	(void)fatfsUnmount(vol);
}

static bool uiPrvBootGifFileName(const char *name)
{
	return uiPrvStrEndsWithNoCase(name, ".gif");
}

static void uiPrvPickBootGif(struct Canvas *cnv, struct Settings *settings)
{
	struct FatfsVol *vol = uiPrvMountCard(cnv, false);
	struct FatFileLocator locator;
	char name[FATFS_NAME_BUF_LEN];
	char parentPath[UI_PICK_FILE_PATH_BUF_SZ];
	uint32_t parentLen, nameLen;

	if (!vol)
		return;
	if (uiPrvPickFile(cnv, vol, "/", uiPrvBootGifFileName, "No compatible GIFs found on the SD card", false,
			&locator, name, sizeof(name), parentPath, sizeof(parentPath), NULL, false, NULL)) {
		parentLen = !strcmp(parentPath, "/") ? 0u : strlen(parentPath);
		nameLen = strlen(name);
		if (parentLen + 1u + nameLen >= sizeof(settings->screenSaverGifPath))
			uiAlert(cnv, "Selected boot GIF path is too long", DialogTypeOk);
		else {
			if (parentLen)
				memcpy(settings->screenSaverGifPath, parentPath, parentLen);
			settings->screenSaverGifPath[parentLen] = '/';
			memcpy(settings->screenSaverGifPath + parentLen + 1u, name, nameLen + 1u);
		}
	}
	(void)uiPrvCardPreUnmount();
	(void)fatfsUnmount(vol);
}

static void uiPrvApplyTheme(const struct Settings *settings)
{
	if (!settings)
		return;
	if (settings->themeEnabled) {
		mUiThemeColor = dcAppDrawRgb565(settings->ledRed, settings->ledGreen, settings->ledBlue);
		dcAppDrawSetThemeColor(settings->ledRed, settings->ledGreen, settings->ledBlue);
	}
	else {
		mUiThemeColor = 0x9cf3u;
		dcAppDrawSetThemeColor(168u, 168u, 168u);
	}
	mUiHeaderClockMinute = 0xffffffffu;
}

static void uiPrvThemeAdjustColor(struct Settings *settings, uint8_t *value, uint_fast16_t button)
{
	uint_fast8_t color = uiPrvLedColorToMenu(*value);

	if (button == KEY_BIT_LEFT) {
		if (!color)
			return;
		color--;
	}
	else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
		if (color >= LED_COLOR_MENU_MAX)
			return;
		color++;
	}
	else
		return;
	*value = uiPrvLedColorFromMenu(color);
	uiPrvApplyTheme(settings);
	badgeLedsApplySettings(settings, true);
}

static void __attribute__((noinline)) uiPrvThemeSettings(struct Canvas *cnv, struct Settings *settings)
{
	int_fast8_t selOption = 0;

	uiPrvSetHeaderTitle("Theme");
	uiPrvApplyTheme(settings);
	uiPrvReset(cnv, false);
	while (1) {
		int_fast8_t numOptions = 0, doneOption, enabledOption, redOption = -1, greenOption = -1, blueOption = -1, bootOption, bootGifOption = -1;
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;

		uiPrvSetHeaderTitle("Theme");
		uiPrvReset(cnv, false);
		doneOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, doneOption), 10, "DONE", -1);

		enabledOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, enabledOption), 10, "THEME:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, uiPrvMenuRow(cnv, enabledOption), 111, settings->themeEnabled ? "YES       " : "NO        ", -1);

		if (settings->themeEnabled) {
			redOption = numOptions++;
			cnv->foreColor = 11;
			uiPuts(cnv, uiPrvMenuRow(cnv, redOption), 10, "THEME RED:", -1);
			cnv->foreColor = 15;
			uiPrintf(cnv, uiPrvMenuRow(cnv, redOption), 111, "%u         ", uiPrvLedColorToMenu(settings->ledRed));

			greenOption = numOptions++;
			cnv->foreColor = 11;
			uiPuts(cnv, uiPrvMenuRow(cnv, greenOption), 10, "THEME GREEN:", -1);
			cnv->foreColor = 15;
			uiPrintf(cnv, uiPrvMenuRow(cnv, greenOption), 111, "%u         ", uiPrvLedColorToMenu(settings->ledGreen));

			blueOption = numOptions++;
			cnv->foreColor = 11;
			uiPuts(cnv, uiPrvMenuRow(cnv, blueOption), 10, "THEME BLUE:", -1);
			cnv->foreColor = 15;
			uiPrintf(cnv, uiPrvMenuRow(cnv, blueOption), 111, "%u         ", uiPrvLedColorToMenu(settings->ledBlue));
		}

		bootOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, bootOption), 10, "BOOT MENU:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, uiPrvMenuRow(cnv, bootOption), 111,
			settings->bootMenuCustomGif ? "CUSTOM GIF" : "TEXT      ", -1);

		if (settings->bootMenuCustomGif) {
			bootGifOption = numOptions++;
			cnv->foreColor = 11;
			uiPuts(cnv, uiPrvMenuRow(cnv, bootGifOption), 10, "BOOT GIF:", -1);
			cnv->foreColor = 15;
			uiPuts(cnv, uiPrvMenuRow(cnv, bootGifOption), 111,
				settings->screenSaverGifPath[0] ? "SELECTED" : "SELECT GIF", -1);
		}

		if (selOption >= numOptions)
			selOption = numOptions - 1;
		selOption = uiPrvMenu(cnv, selOption, numOptions, &button);
		if (uiPrvToolExitRequested() || button == KEY_BIT_B || selOption == doneOption) {
			(void)settingsSet(settings);
			return;
		}
		if (selOption == enabledOption) {
			if (button == KEY_BIT_LEFT || button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
				settings->themeEnabled = !settings->themeEnabled;
				uiPrvApplyTheme(settings);
			}
		}
		else if (selOption == redOption)
			uiPrvThemeAdjustColor(settings, &settings->ledRed, button);
		else if (selOption == greenOption)
			uiPrvThemeAdjustColor(settings, &settings->ledGreen, button);
		else if (selOption == blueOption)
			uiPrvThemeAdjustColor(settings, &settings->ledBlue, button);
		else if (selOption == bootOption) {
			if (button == KEY_BIT_LEFT)
				settings->bootMenuCustomGif = !settings->bootMenuCustomGif;
			else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A)
				settings->bootMenuCustomGif = !settings->bootMenuCustomGif;
		}
		else if (selOption == bootGifOption && button == KEY_BIT_A)
			uiPrvPickBootGif(cnv, settings);
	}
}

static void uiPrvPickScreenSaverImage(struct Canvas *cnv, struct Settings *settings)
{
	struct FatfsVol *vol = uiPrvMountCard(cnv, false);
	struct FatFileLocator locator;
	char name[FATFS_NAME_BUF_LEN];
	char parentPath[UI_PICK_FILE_PATH_BUF_SZ];
	uint32_t parentLen, nameLen;

	if (!vol)
		return;
	if (uiPrvPickFile(cnv, vol, "/", imageViewerFileName, "No compatible images found on the SD card", false,
			&locator, name, sizeof(name), parentPath, sizeof(parentPath), NULL, false, NULL)) {
		parentLen = !strcmp(parentPath, "/") ? 0u : strlen(parentPath);
		nameLen = strlen(name);
		if (parentLen + 1u + nameLen >= sizeof(settings->screenSaverGifPath))
			uiAlert(cnv, "Selected image path is too long", DialogTypeOk);
		else {
			if (parentLen)
				memcpy(settings->screenSaverGifPath, parentPath, parentLen);
			settings->screenSaverGifPath[parentLen] = '/';
			memcpy(settings->screenSaverGifPath + parentLen + 1u, name, nameLen + 1u);
		}
	}
	(void)uiPrvCardPreUnmount();
	(void)fatfsUnmount(vol);
}

static bool uiPrvScreenSaverUsesSelectedImage(uint8_t saver)
{
	return saver == ScreenSaverDvdBounce || saver == ScreenSaverScrollPattern;
}

static const char *uiPrvScreenSaverImageBaseName(const char *path)
{
	const char *slash = strrchr(path, '/');

	return slash && slash[1] ? slash + 1 : "SELECT IMAGE";
}

static void __attribute__((noinline)) uiPrvScreenSettings(struct Canvas *cnv, struct Settings *settings)
{
	int_fast8_t selOption = 0;

	uiPowerSetActiveBrightness(settings->brightness);
	uiPowerApplySettings(settings);
	uiPrvSetHeaderTitle("Display");
	uiPrvReset(cnv, false);

	while (1) {

		int_fast8_t numOptions = 0, doneOption, contrastOption = -1, brightnessOption = -1, rotationOption,
			powerSaveOption, powerSaveTimeoutOption, powerSaveBrightnessOption;
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;

		uiPrvReset(cnv, false);

		doneOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, doneOption), 10, "DONE", -1);

	#ifdef DISP_CONTRAST_ADJUSTABLE
		contrastOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, contrastOption), 10, "CONTRAST:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, uiPrvMenuRow(cnv, contrastOption), 111, "%u         ", settings->contrast);
	#endif

		rotationOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, rotationOption), 10, "ROTATION:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, uiPrvMenuRow(cnv, rotationOption), 111, settings->rotation ? "FLIPPED  " : "NORMAL   ", -1);

	#ifdef DISP_BRIGHTNESS_ADJUSTABLE
		brightnessOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, brightnessOption), 10, "BRIGHTNESS:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, uiPrvMenuRow(cnv, brightnessOption), 111, "%u         ", settings->brightness);
	#endif

		powerSaveOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, powerSaveOption), 10, "POWER SAVE:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, uiPrvMenuRow(cnv, powerSaveOption), 111, settings->powerSaveEnabled ? "ON " : "OFF", -1);

		powerSaveTimeoutOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, powerSaveTimeoutOption), 10, "DIM AFTER:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, uiPrvMenuRow(cnv, powerSaveTimeoutOption), 111, "%us", settings->powerSaveTimeout);

		powerSaveBrightnessOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, powerSaveBrightnessOption), 10, "DIM LEVEL:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, uiPrvMenuRow(cnv, powerSaveBrightnessOption), 111, "%u", settings->powerSaveBrightness);

		selOption = uiPrvMenu(cnv, selOption, numOptions, &button);
		if (uiPrvToolExitRequested())
			return;
		if (button == KEY_BIT_B || selOption == doneOption)
			return;

		if (selOption == rotationOption) {
			settings->rotation = !settings->rotation;
		}
		if (selOption == contrastOption) {

			if (button == KEY_BIT_LEFT) {
				if (settings->contrast)
					settings->contrast--;
				else
					continue;
			}
			else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
				if (settings->contrast != 0x1f)
					settings->contrast++;
				else
					continue;
			}

			dispSetContrast(settings->contrast);
		}

		if (selOption == brightnessOption) {

			if (button == KEY_BIT_LEFT) {
				if (settings->brightness)
					settings->brightness--;
				else
					continue;
			}
			else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
				if (settings->brightness != 0x1f)
					settings->brightness++;
				else
					continue;
			}

			dispSetBrightness(settings->brightness);
			uiPowerSetActiveBrightness(settings->brightness);
		}

		if (selOption == powerSaveOption) {
			settings->powerSaveEnabled = !settings->powerSaveEnabled;
			uiPowerApplySettings(settings);
		}
		if (selOption == powerSaveTimeoutOption) {
			settings->powerSaveTimeout = uiPrvDisplayTimeoutAdjust(settings->powerSaveTimeout,
				button == KEY_BIT_LEFT ? -1 : 1);
			uiPowerApplySettings(settings);
		}
		if (selOption == powerSaveBrightnessOption) {
			if (button == KEY_BIT_LEFT && settings->powerSaveBrightness)
				settings->powerSaveBrightness--;
			else if (button != KEY_BIT_LEFT && settings->powerSaveBrightness < 0x1f)
				settings->powerSaveBrightness++;
			uiPowerApplySettings(settings);
		}
	}
}

static void uiPrvScreenSaverSettings(struct Canvas *cnv, struct Settings *settings)
{
	int_fast8_t selOption = 0;

	uiPrvSetHeaderTitle("Screensaver");
	uiPrvReset(cnv, false);
	while (1) {
		int_fast8_t numOptions = 0, doneOption, typeOption, imageOption = -1, flipOption,
			brightnessOption, timeoutOption;
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;

		uiPrvReset(cnv, false);
		doneOption = numOptions++;
		typeOption = numOptions++;
		if (uiPrvScreenSaverUsesSelectedImage(settings->screenSaver))
			imageOption = numOptions++;
		flipOption = numOptions++;
		brightnessOption = numOptions++;
		timeoutOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, doneOption), 10, "DONE", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, typeOption), 10, "SCREENSAVER:", -1);
		if (imageOption >= 0)
			uiPuts(cnv, uiPrvMenuRow(cnv, imageOption), 10, "SAVER IMAGE:", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, flipOption), 10, "SAVER FLIP:", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, brightnessOption), 10, "SAVER LEVEL:", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, timeoutOption), 10, "SAVER AFTER:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, uiPrvMenuRow(cnv, typeOption), 111, uiPrvScreenSaverName(settings->screenSaver), -1);
		if (imageOption >= 0)
			uiPrvDrawTruncText(cnv, uiPrvMenuRow(cnv, imageOption), 111, cnv->w - 121,
				uiPrvScreenSaverImageBaseName(settings->screenSaverGifPath));
		uiPuts(cnv, uiPrvMenuRow(cnv, flipOption), 111,
			settings->screenSaverRotation ? "FLIPPED  " : "NORMAL   ", -1);
		uiPrintf(cnv, uiPrvMenuRow(cnv, brightnessOption), 111, "%u", settings->screenSaverBrightness);
		uiPrintf(cnv, uiPrvMenuRow(cnv, timeoutOption), 111, "%us", settings->screenSaverTimeout);

		if (selOption >= numOptions)
			selOption = 0;
		selOption = uiPrvMenu(cnv, selOption, numOptions, &button);
		if (uiPrvToolExitRequested() || button == KEY_BIT_B || selOption == doneOption)
			return;
		if (selOption == typeOption) {
			if (button == KEY_BIT_A && settings->screenSaver == ScreenSaverGif)
				uiPrvPickScreenSaverMedia(cnv, settings, false);
			else if (button == KEY_BIT_A && settings->screenSaver == ScreenSaverImageFolder)
				uiPrvPickScreenSaverMedia(cnv, settings, true);
			else if (button == KEY_BIT_A && uiPrvScreenSaverUsesSelectedImage(settings->screenSaver))
				uiPrvPickScreenSaverImage(cnv, settings);
			else if (button == KEY_BIT_LEFT)
				settings->screenSaver = settings->screenSaver ? settings->screenSaver - 1u : ScreenSaverNumModes - 1u;
			else
				settings->screenSaver = (settings->screenSaver + 1u) % ScreenSaverNumModes;
			uiPowerApplySettings(settings);
		}
		else if (selOption == imageOption) {
			uiPrvPickScreenSaverImage(cnv, settings);
			uiPowerApplySettings(settings);
		}
		else if (selOption == flipOption) {
			settings->screenSaverRotation = !settings->screenSaverRotation;
			uiPowerApplySettings(settings);
		}
		else if (selOption == brightnessOption) {
			if (button == KEY_BIT_LEFT && settings->screenSaverBrightness)
				settings->screenSaverBrightness--;
			else if (button != KEY_BIT_LEFT && settings->screenSaverBrightness < 0x1f)
				settings->screenSaverBrightness++;
			uiPowerApplySettings(settings);
		}
		else if (selOption == timeoutOption) {
			settings->screenSaverTimeout = uiPrvDisplayTimeoutAdjust(settings->screenSaverTimeout,
				button == KEY_BIT_LEFT ? -1 : 1);
			uiPowerApplySettings(settings);
		}
	}
}

static void uiPrvApplyAudioSettings(const struct Settings *settings)
{
	audioPwmSetMasterVolume(settings->audioMuted ? 0 : settings->audioVolume);
}

static void __attribute__((noinline)) uiPrvAudioSettings(struct Canvas *cnv, struct Settings *settings)
{
	enum {
		AudioSettingDone,
		AudioSettingMute,
		AudioSettingVolume,
		AudioSettingNum,
	};
	uint_fast8_t selOption = 0;

	while (1) {
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;

		uiPrvSetHeaderTitle("Audio");
		uiPrvReset(cnv, false);
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, AudioSettingDone), 10, "DONE", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, AudioSettingMute), 10, "MUTE:", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, AudioSettingVolume), 10, "VOLUME:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, uiPrvMenuRow(cnv, AudioSettingMute), 111,
			settings->audioMuted ? "ON " : "OFF", -1);
		uiPrintf(cnv, uiPrvMenuRow(cnv, AudioSettingVolume), 111, "%u  ",
			(unsigned)settings->audioVolume);

		selOption = uiPrvMenu(cnv, selOption, AudioSettingNum, &button);
		if (uiPrvToolExitRequested() || button == KEY_BIT_B || selOption == AudioSettingDone)
			return;
		if (selOption == AudioSettingMute)
			settings->audioMuted = !settings->audioMuted;
		else if (selOption == AudioSettingVolume) {
			if (button == KEY_BIT_LEFT) {
				if (settings->audioVolume)
					settings->audioVolume--;
			}
			else if (settings->audioVolume < AUDIO_PWM_VOLUME_MAX)
				settings->audioVolume++;
		}
		uiPrvApplyAudioSettings(settings);
	}
}

static bool uiPrvEmulatorHasPalette(enum UiEmulatorConsole console)
{
	return console == UiEmulatorConsoleGameboy;
}

static bool uiPrvEmulatorHasDisplayOptions(enum UiEmulatorConsole console)
{
	return console == UiEmulatorConsoleGameboy ||
		console == UiEmulatorConsoleGameboyColor ||
		console == UiEmulatorConsoleNes;
}

static uint_fast8_t uiPrvEmulatorSpeed(const struct Settings *settings, enum UiEmulatorConsole console)
{
	switch (console) {
		case UiEmulatorConsoleGameboy:
			return settings->gbSpeed;
		case UiEmulatorConsoleGameboyColor:
			return settings->gbcSpeed;
		case UiEmulatorConsoleNes:
			return settings->nesSpeed;
		default:
			return settings->speed;
	}
}

static void uiPrvEmulatorSetSpeed(struct Settings *settings, enum UiEmulatorConsole console, uint_fast8_t speed)
{
	switch (console) {
		case UiEmulatorConsoleGameboy:
			settings->gbSpeed = speed;
			settings->speed = speed;
			break;
		case UiEmulatorConsoleGameboyColor:
			settings->gbcSpeed = speed;
			break;
		case UiEmulatorConsoleNes:
			settings->nesSpeed = speed;
			break;
		default:
			settings->speed = speed;
			break;
	}
}

static bool uiPrvEmulatorUpscale(const struct Settings *settings, enum UiEmulatorConsole console)
{
	switch (console) {
		case UiEmulatorConsoleGameboy:
			return settings->gbUpscale;
		case UiEmulatorConsoleGameboyColor:
			return settings->gbcUpscale;
		case UiEmulatorConsoleNes:
			return settings->nesUpscale;
		default:
			return false;
	}
}

static void uiPrvEmulatorSetUpscale(struct Settings *settings, enum UiEmulatorConsole console, bool upscale)
{
	switch (console) {
		case UiEmulatorConsoleGameboy:
			settings->gbUpscale = upscale;
			settings->upscale = upscale;
			break;
		case UiEmulatorConsoleGameboyColor:
			settings->gbcUpscale = upscale;
			break;
		case UiEmulatorConsoleNes:
			settings->nesUpscale = upscale;
			break;
		default:
			break;
	}
}

static bool __attribute__((noinline)) uiPrvGameSettings(struct Canvas *cnv, struct Settings *settings, enum UiEmulatorConsole console)
{
	bool restartCurGame = false;
	int_fast8_t selOption = 0;

	uiPrvSetHeaderTitle(uiPrvEmulatorSettingsName(console));
	uiPrvReset(cnv, false);

	while (1) {

		int_fast8_t numOptions = 0, doneOption, paletteOption = -1, upscaleOption = -1, speedOption = -1;
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;
		static const char speedNames[][8] = DISP_SPEED_NAMES;
		static const uint8_t speedSettings[] = DISP_SPEED_SETTINGS;
		static const char *const paletteNames[GameBoyPaletteNumPalettes] = {
			[GameBoyPaletteBw] = "Black & White",
			[GameBoyPaletteDmg] = "Original Game Boy",
			[GameBoyPaletteGbpocket] = "Game Boy Pocket",
			[GameBoyPaletteBgb] = "BGB Emulator",
			[GameBoyPaletteGbli] = "Game Boy Light",
			[GameBoyPaletteGrafixkidgray] = "Grafixkid Gray",
			[GameBoyPaletteGrafixkidgreen] = "Grafixkid Green",
			[GameBoyPaletteBlackzero] = "Game Boy (Black Zero) palette",
			[GameBoyPaletteGbcjp] = "Pocket Camera JP",
			[GameBoyPaletteGbcu] = "Brown",
			[GameBoyPaletteGbcua] = "Red",
			[GameBoyPaletteGbcub] = "Dark Brown",
			[GameBoyPaletteGbcl] = "Blue",
			[GameBoyPaletteGbcla] = "Dark Blue",
			[GameBoyPaletteGbclb] = "Gray",
			[GameBoyPaletteGbcd] = "Pale Yellow",
			[GameBoyPaletteGbcda] = "Orange",
			[GameBoyPaletteGbcdb] = "Yellow",
			[GameBoyPaletteGbcr] = "Green",
			[GameBoyPaletteGbceuus] = "Dark Green",
			[GameBoyPaletteGbcrb] = "Reverse",
			[GameBoyPaletteGbcPreferred] = "GBC Preferred",
		};
		uint_fast8_t numSpeeds = sizeof(speedSettings) / sizeof(*speedSettings);
		uint_fast8_t speed = uiPrvEmulatorSpeed(settings, console);

		settings->actLikeGBC = 1;
		if (speed >= numSpeeds)
			speed = 1;
		if (settings->gbPalette >= GameBoyPaletteNumPalettes)
			settings->gbPalette = GameBoyPaletteBw;
		uiPrvEmulatorSetSpeed(settings, console, speed);

		uiPrvSetHeaderTitle(uiPrvEmulatorSettingsName(console));
		uiPrvReset(cnv, false);

		doneOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, doneOption), 10, "DONE", -1);

		if (uiPrvEmulatorHasPalette(console)) {
			paletteOption = numOptions++;
			cnv->foreColor = 11;
			uiPuts(cnv, uiPrvMenuRow(cnv, paletteOption), 10, "PALETTE:", -1);
			cnv->foreColor = 15;
			uiPrvDrawTruncText(cnv, uiPrvMenuRow(cnv, paletteOption), 85, cnv->w - 85, paletteNames[settings->gbPalette]);
		}

		if (uiPrvEmulatorHasDisplayOptions(console)) {
			upscaleOption = numOptions++;
			cnv->foreColor = 11;
			uiPuts(cnv, uiPrvMenuRow(cnv, upscaleOption), 10, "UPSCALE:", -1);
			cnv->foreColor = 15;
			uiPuts(cnv, uiPrvMenuRow(cnv, upscaleOption), 111, uiPrvEmulatorUpscale(settings, console) ? "YES       " : "NO       ", -1);

			speedOption = numOptions++;
			cnv->foreColor = 11;
			uiPuts(cnv, uiPrvMenuRow(cnv, speedOption), 10, "SPEED:", -1);
			cnv->foreColor = 15;
			uiPuts(cnv, uiPrvMenuRow(cnv, speedOption), 111, speedNames[speed], sizeof(speedNames[speed]));
		}

		selOption = uiPrvMenu(cnv, selOption, numOptions, &button);
		if (uiPrvToolExitRequested())
			return restartCurGame;
		if (button == KEY_BIT_B || selOption == doneOption)
			return restartCurGame;

		if (selOption == speedOption) {

			if (button == KEY_BIT_LEFT) {
				if (speed)
					speed--;
				else
					speed = numSpeeds - 1;
			}
			else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
				if ((uint_fast8_t)(speed + 1) < numSpeeds)
					speed++;
				else
					speed = 0;
			}

			uiPrvEmulatorSetSpeed(settings, console, speed);
			if (console == UiEmulatorConsoleGameboy || console == UiEmulatorConsoleGameboyColor) {
				dispOff();
				dispInit(speedSettings[speed]);
				dispSetContrast(settings->contrast);		//must be reset
			}
		}

		if (selOption == upscaleOption)
			uiPrvEmulatorSetUpscale(settings, console, !uiPrvEmulatorUpscale(settings, console));

		if (selOption == paletteOption) {

			restartCurGame = true;
			if (button == KEY_BIT_LEFT)
				settings->gbPalette = settings->gbPalette ? settings->gbPalette - 1 : GameBoyPaletteNumPalettes - 1;
			else
				settings->gbPalette = (settings->gbPalette + 1) % GameBoyPaletteNumPalettes;
		}
	}
}

enum ClockSettingsOption {
	ClockSettingSet,
	ClockSettingYear,
	ClockSettingMonth,
	ClockSettingDay,
	ClockSettingHour,
	ClockSettingMinute,
	ClockSettingSecond,
	ClockSettingCancel,
	ClockSettingNum,
};

static uint_fast8_t uiPrvClockHour12(const struct BadgeRtcDateTime *time)
{
	uint_fast8_t hour = time->hour % 12u;

	return hour ? hour : 12;
}

static void uiPrvClockClampDay(struct BadgeRtcDateTime *time)
{
	uint_fast8_t maxDay = badgeRtcDaysInMonth(time->year, time->month);

	if (time->day < 1)
		time->day = 1;
	if (time->day > maxDay)
		time->day = maxDay;
}

static uint_fast16_t uiPrvClockWrap(uint_fast16_t val, uint_fast16_t min, uint_fast16_t max, bool inc)
{
	if (inc)
		return val < max ? val + 1 : min;
	return val > min ? val - 1 : max;
}

static void uiPrvClockAdjust(struct BadgeRtcDateTime *time, enum ClockSettingsOption option, bool inc)
{
	switch (option) {
		case ClockSettingYear:
			time->year = uiPrvClockWrap(time->year, BADGE_RTC_MIN_YEAR, BADGE_RTC_MAX_YEAR, inc);
			uiPrvClockClampDay(time);
			break;

		case ClockSettingMonth:
			time->month = uiPrvClockWrap(time->month, 1, 12, inc);
			uiPrvClockClampDay(time);
			break;

		case ClockSettingDay:
			time->day = uiPrvClockWrap(time->day, 1, badgeRtcDaysInMonth(time->year, time->month), inc);
			break;

		case ClockSettingHour:
			time->hour = uiPrvClockWrap(time->hour, 0, 23, inc);
			break;

		case ClockSettingMinute:
			time->minute = uiPrvClockWrap(time->minute, 0, 59, inc);
			break;

		case ClockSettingSecond:
			time->second = uiPrvClockWrap(time->second, 0, 59, inc);
			break;

		default:
			break;
	}
}

static void uiPrvClockDefault(struct BadgeRtcDateTime *time)
{
	if (badgeRtcGetDateTime(time) || badgeRtcReadHardware(time))
		return;

	time->year = 2026;
	time->month = 1;
	time->day = 1;
	time->hour = 12;
	time->minute = 0;
	time->second = 0;
}

static void __attribute__((noinline)) uiPrvClockSettings(struct Canvas *cnv)
{
	struct BadgeRtcDateTime time;
	int_fast8_t selOption = 0;

	uiPrvClockDefault(&time);
	uiPrvClockClampDay(&time);
	uiPrvSetHeaderTitle("Clock");
	uiPrvReset(cnv, false);

	while (1) {
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;

		uiPrvReset(cnv, false);

		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, ClockSettingSet), 10, "Set Clock", -1);

		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, ClockSettingYear), 10, "YEAR:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, uiPrvMenuRow(cnv, ClockSettingYear), 111, "%u       ", (unsigned)time.year);

		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, ClockSettingMonth), 10, "MONTH:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, uiPrvMenuRow(cnv, ClockSettingMonth), 111, "%02u        ", (unsigned)time.month);

		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, ClockSettingDay), 10, "DAY:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, uiPrvMenuRow(cnv, ClockSettingDay), 111, "%02u        ", (unsigned)time.day);

		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, ClockSettingHour), 10, "HOUR:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, uiPrvMenuRow(cnv, ClockSettingHour), 111, "%u %s      ", (unsigned)uiPrvClockHour12(&time), time.hour >= 12 ? "PM" : "AM");

		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, ClockSettingMinute), 10, "MINUTE:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, uiPrvMenuRow(cnv, ClockSettingMinute), 111, "%02u        ", (unsigned)time.minute);

		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, ClockSettingSecond), 10, "SECOND:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, uiPrvMenuRow(cnv, ClockSettingSecond), 111, "%02u        ", (unsigned)time.second);

		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, ClockSettingCancel), 10, "Cancel", -1);

		selOption = uiPrvMenu(cnv, selOption, ClockSettingNum, &button);
		if (uiPrvToolExitRequested() || button == KEY_BIT_B || (selOption == ClockSettingCancel && button == KEY_BIT_A))
			return;
		if (selOption == ClockSettingSet) {
			if (button != KEY_BIT_A)
				continue;
			if (badgeRtcSetDateTime(&time)) {
				uiAlert(cnv, "Clock set", DialogTypeOk);
				return;
			}
			uiAlert(cnv, "RTC write failed", DialogTypeOk);
			continue;
		}

		if (button == KEY_BIT_LEFT || button == KEY_BIT_RIGHT)
			uiPrvClockAdjust(&time, (enum ClockSettingsOption)selOption, button == KEY_BIT_RIGHT);
	}
}

static void uiPrvUsbSettingsDefaults(struct Settings *settings)
{
	settings->badUsbVid = USB_HID_DEFAULT_VID;
	settings->badUsbPid = USB_HID_DEFAULT_PID;
	strcpy(settings->badUsbManufacturer, "DC32");
	strcpy(settings->badUsbProduct, "DC32 BadUSB");
}

static void uiPrvUsbEditHex16(struct Canvas *cnv, const char *title, uint16_t *valP)
{
	uint_fast8_t nibble = 0;

	while (1) {
		uint_fast16_t key;
		char msg[64];

		uiPrvSetHeaderTitle(title);
		uiPrvReset(cnv, false);
		cnv->font = FontMedium;
		(void)sprintf(msg, "%04X", (unsigned)*valP);
		uiPuts(cnv, uiPrvContentTop(cnv), 10, msg, -1);
		uiPuts(cnv, uiPrvContentTop(cnv) + uiPrvGlyphHeight(cnv) + 1, 10 + nibble * uiPrvCharsWidth(cnv, "0", 1), "^", -1);
		uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "Up/Down Edit  A Done", -1);

		key = uiPrvRecvMenuKeypress(cnv);
		if (key == KEY_BIT_A || key == KEY_BIT_B || key == UI_KEY_BIT_CENTER)
			return;
		if (key == KEY_BIT_LEFT) {
			if (nibble)
				nibble--;
			continue;
		}
		if (key == KEY_BIT_RIGHT) {
			if (nibble < 3)
				nibble++;
			continue;
		}
		if (key == KEY_BIT_UP || key == KEY_BIT_DOWN) {
			uint_fast8_t shift = (3 - nibble) * 4;
			uint16_t mask = (uint16_t)(0xfu << shift);
			uint_fast8_t digit = (uint_fast8_t)((*valP >> shift) & 0xf);

			if (key == KEY_BIT_UP)
				digit = (digit + 1) & 0xf;
			else
				digit = (digit + 15) & 0xf;
			*valP = (uint16_t)((*valP & ~mask) | ((uint16_t)digit << shift));
		}
	}
}

static int_fast8_t uiPrvUsbTextCharIndex(char ch)
{
	static const char chars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.";
	uint_fast8_t i;

	for (i = 0; chars[i]; i++)
		if (chars[i] == ch)
			return i;
	return 0;
}

static char uiPrvUsbTextCharAt(uint_fast8_t idx)
{
	static const char chars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.";
	uint_fast8_t len = sizeof(chars) - 1;

	return chars[idx % len];
}

static void uiPrvUsbEditText(struct Canvas *cnv, const char *title, char *text, uint32_t textLen)
{
	uint32_t cursor = 0;

	if (textLen < 2)
		return;
	text[textLen - 1] = 0;
	while (1) {
		uint32_t len = strlen(text), maxCursor = len < textLen - 1 ? len : textLen - 2;
		uint_fast16_t key;
		char msg[64];

		if (cursor > maxCursor)
			cursor = maxCursor;
		uiPrvSetHeaderTitle(title);
		uiPrvReset(cnv, false);
		cnv->font = FontMedium;
		uiPrvDrawTruncText(cnv, uiPrvContentTop(cnv), 10, cnv->w - 20, text);
		(void)sprintf(msg, "Pos %u/%u", (unsigned)(cursor + 1), (unsigned)(textLen - 1));
		uiPuts(cnv, uiPrvMenuRow(cnv, 2), 10, msg, -1);
		if (text[cursor])
			(void)sprintf(msg, "Char %c", text[cursor]);
		else
			strcpy(msg, "Char END");
		uiPuts(cnv, uiPrvMenuRow(cnv, 3), 10, msg, -1);
		uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "A Save B Back Sel Del", -1);

		key = uiPrvRecvMenuKeypress(cnv);
		if (key == KEY_BIT_A || key == KEY_BIT_B || key == UI_KEY_BIT_CENTER)
			return;
		if (key == KEY_BIT_LEFT) {
			if (cursor)
				cursor--;
			continue;
		}
		if (key == KEY_BIT_RIGHT) {
			if (cursor < maxCursor)
				cursor++;
			continue;
		}
		if (key == KEY_BIT_SEL) {
			if (cursor < len)
				memmove(text + cursor, text + cursor + 1, len - cursor);
			continue;
		}
		if (key == KEY_BIT_UP || key == KEY_BIT_DOWN) {
			int_fast8_t idx = uiPrvUsbTextCharIndex(text[cursor] ? text[cursor] : ' ');

			if (key == KEY_BIT_UP)
				idx++;
			else
				idx--;
			if (idx < 0)
				idx = (int_fast8_t)(sizeof(" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.") - 2);
			text[cursor] = uiPrvUsbTextCharAt((uint_fast8_t)idx);
			text[textLen - 1] = 0;
		}
	}
}

static void __attribute__((noinline)) uiPrvUsbSettings(struct Canvas *cnv, struct Settings *settings)
{
	uint_fast8_t selOption = 0;

	while (1) {
		enum {
			UsbSettingDone,
			UsbSettingVid,
			UsbSettingPid,
			UsbSettingManufacturer,
			UsbSettingProduct,
			UsbSettingReset,
			UsbSettingNum,
		};
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;
		char msg[64];

		uiPrvSetHeaderTitle("USB");
		uiPrvReset(cnv, false);
		cnv->foreColor = 11;
		uiPuts(cnv, uiPrvMenuRow(cnv, UsbSettingDone), 10, "DONE", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, UsbSettingVid), 10, "VID:", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, UsbSettingPid), 10, "PID:", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, UsbSettingManufacturer), 10, "MFR:", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, UsbSettingProduct), 10, "PRODUCT:", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, UsbSettingReset), 10, "Reset USB defaults", -1);
		cnv->foreColor = 15;
		(void)sprintf(msg, "%04X", (unsigned)settings->badUsbVid);
		uiPuts(cnv, uiPrvMenuRow(cnv, UsbSettingVid), 75, msg, -1);
		(void)sprintf(msg, "%04X", (unsigned)settings->badUsbPid);
		uiPuts(cnv, uiPrvMenuRow(cnv, UsbSettingPid), 75, msg, -1);
		uiPrvDrawTruncText(cnv, uiPrvMenuRow(cnv, UsbSettingManufacturer), 75, cnv->w - 75, settings->badUsbManufacturer);
		uiPrvDrawTruncText(cnv, uiPrvMenuRow(cnv, UsbSettingProduct), 75, cnv->w - 75, settings->badUsbProduct);

		selOption = uiPrvMenu(cnv, selOption, UsbSettingNum, &button);
		if (uiPrvToolExitRequested() || button == KEY_BIT_B || selOption == UsbSettingDone)
			return;
		if (selOption == UsbSettingVid)
			uiPrvUsbEditHex16(cnv, "USB VID", &settings->badUsbVid);
		else if (selOption == UsbSettingPid)
			uiPrvUsbEditHex16(cnv, "USB PID", &settings->badUsbPid);
		else if (selOption == UsbSettingManufacturer)
			uiPrvUsbEditText(cnv, "USB MFR", settings->badUsbManufacturer, sizeof(settings->badUsbManufacturer));
		else if (selOption == UsbSettingProduct)
			uiPrvUsbEditText(cnv, "USB Product", settings->badUsbProduct, sizeof(settings->badUsbProduct));
		else if (selOption == UsbSettingReset)
			uiPrvUsbSettingsDefaults(settings);
	}
}

static bool __attribute__((noinline)) uiPrvEmulatorSettings(struct Canvas *cnv, struct Settings *settings)
{
	bool restartCurGame = false;
	uint_fast8_t selOption = 0;
	static const enum UiEmulatorConsole consoles[] = {
		UiEmulatorConsoleArduboy,
		UiEmulatorConsoleGameboy,
		UiEmulatorConsoleGameboyColor,
		UiEmulatorConsoleNes,
	};
	enum { NumConsoles = sizeof(consoles) / sizeof(*consoles) };

	while (1) {
		uint_fast8_t i;
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;

		uiPrvSetHeaderTitle("Emulators");
		uiPrvReset(cnv, false);
		for (i = 0; i < NumConsoles; i++)
			uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, uiPrvEmulatorSettingsName(consoles[i]), -1);

		selOption = uiPrvMenu(cnv, selOption, NumConsoles, &button);
		if (uiPrvToolExitRequested() || button == KEY_BIT_B)
			return restartCurGame;
		restartCurGame = uiPrvGameSettings(cnv, settings, consoles[selOption]) || restartCurGame;
		if (uiPrvToolExitRequested())
			return restartCurGame;
	}
}

static void uiPrvFileBrowserAppSettings(struct Canvas *cnv, struct Settings *settings);
static void uiPrvMusicAppSettings(struct Canvas *cnv, struct Settings *settings);
static void uiPrvAutoclickerAppSettings(struct Canvas *cnv, struct Settings *settings);
static void uiPrvPongAppSettings(struct Canvas *cnv, struct Settings *settings);
static void uiPrvTetrisAppSettings(struct Canvas *cnv, struct Settings *settings);

static bool uiPrvAppSettings(struct Canvas *cnv, struct Settings *settings)
{
	enum {
		AppSettingsFileBrowser,
		AppSettingsMusic,
		AppSettingsAutoclicker,
		AppSettingsBadUsb,
		AppSettingsEmulators,
		AppSettingsPong,
		AppSettingsTetris,
		AppSettingsBack,
	};
	static const char *const labels[] = {
		"File Manager", "Music", "Autoclicker", "BadUSB", "Emulators", "Pong", "Tetris", "Back",
	};
	bool restartCurGame = false;
	uint_fast8_t selected = 0;

	while (1) {
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;

		uiPrvSetHeaderTitle("App Settings");
		uiPrvReset(cnv, false);
		for (uint_fast8_t i = 0; i < sizeof(labels) / sizeof(*labels); i++)
			uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, labels[i], -1);
		selected = uiPrvMenu(cnv, selected, sizeof(labels) / sizeof(*labels), &button);
		if (uiPrvToolExitRequested() || button == KEY_BIT_B || selected == AppSettingsBack)
			return restartCurGame;
		switch (selected) {
		case AppSettingsFileBrowser:
			uiPrvFileBrowserAppSettings(cnv, settings);
			break;
		case AppSettingsMusic:
			uiPrvMusicAppSettings(cnv, settings);
			break;
		case AppSettingsAutoclicker:
			uiPrvAutoclickerAppSettings(cnv, settings);
			break;
		case AppSettingsBadUsb:
			uiPrvUsbSettings(cnv, settings);
			break;
		case AppSettingsEmulators:
			restartCurGame = uiPrvEmulatorSettings(cnv, settings) || restartCurGame;
			break;
		case AppSettingsPong:
			uiPrvPongAppSettings(cnv, settings);
			break;
		case AppSettingsTetris:
			uiPrvTetrisAppSettings(cnv, settings);
			break;
		default:
			break;
		}
		if (uiPrvToolExitRequested())
			return restartCurGame;
	}
}

static bool __attribute__((noinline)) uiPrvSettings(struct Canvas *cnv, bool exitOnDone)		//return true if anything for the current game may have changes
{
	bool restartCurGame = false;
	struct Settings settings;
	uint_fast8_t numOptions = exitOnDone ? 9 : 8;

	uiPrvSetHeaderTitle("Settings");
	settingsGet(&settings);
	uiPrvApplyTheme(&settings);
	if (settings.ledMode >= LedModeNumModes)
		settings.ledMode = LedModeOff;
	if (settings.ledSpeed < LED_SPEED_MENU_MIN || settings.ledSpeed > LED_SPEED_MENU_MAX)
		settings.ledSpeed = 4;

	uiPrvReset(cnv, false);

	while (1) {
		uint_fast8_t doneOption = 0, appSettingsOption = exitOnDone ? 1 : 0, clockOption = appSettingsOption + 1, audioOption = clockOption + 1, ledSettingsOption = audioOption + 1, themeOption = ledSettingsOption + 1, screenOption = themeOption + 1, screenSaverOption = screenOption + 1, usbOption = screenSaverOption + 1, selOption;
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;

		uiPrvSetHeaderTitle("Settings");
		uiPrvReset(cnv, false);
		if (exitOnDone)
			uiPuts(cnv, uiPrvMenuRow(cnv, doneOption), 10, "DONE", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, appSettingsOption), 10, "App Settings", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, clockOption), 10, "Clock", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, audioOption), 10, "Audio", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, ledSettingsOption), 10, "LEDs", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, themeOption), 10, "Theme", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, screenOption), 10, "Display", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, screenSaverOption), 10, "Screensaver", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, usbOption), 10, "USB", -1);

		selOption = uiPrvMenu(cnv, 0, numOptions, &button);
		if (uiPrvToolExitRequested()) {
			settingsSet(&settings);
			return restartCurGame;
		}
		if (button == KEY_BIT_B || (exitOnDone && selOption == doneOption)) {
			settingsSet(&settings);
			return restartCurGame;
		}
		if (selOption == screenOption)
			uiPrvScreenSettings(cnv, &settings);
		else if (selOption == screenSaverOption)
			uiPrvScreenSaverSettings(cnv, &settings);
		else if (selOption == appSettingsOption)
			restartCurGame = uiPrvAppSettings(cnv, &settings) || restartCurGame;
		else if (selOption == clockOption)
			uiPrvClockSettings(cnv);
		else if (selOption == audioOption)
			uiPrvAudioSettings(cnv, &settings);
		else if (selOption == ledSettingsOption)
			uiPrvLedSettings(cnv, &settings);
		else if (selOption == themeOption)
			uiPrvThemeSettings(cnv, &settings);
		else if (selOption == usbOption)
			uiPrvUsbSettings(cnv, &settings);
		if (uiPrvToolExitRequested()) {
			settingsSet(&settings);
			return restartCurGame;
		}
	}
}

static bool __attribute__((noinline)) uiPrvEditGameSettings(struct Canvas *cnv, enum UiEmulatorConsole console)
{
	struct Settings settings;
	bool restartCurGame;

	settingsGet(&settings);
	restartCurGame = uiPrvGameSettings(cnv, &settings, console);
	settingsSet(&settings);
	return restartCurGame;
}

static void uiPrvLoadSavestate(void)
{
	struct GameSelection selection;

	if (uiGetGameSelection(&selection)) {
		(void)uiPrvHydrateSaveRamFromFlash(selection.runtime, selection.romSize, selection.saveRamSize,
			(const char*)QSPI_FILENAME_START, "load selected", true);
		return;
	}

	pr("Savegame hydrate: no valid selected game; clearing cart RAM\n");
	uiPrvCartRamOwnerClear("no selected game");
	memset(CART_RAM_ADDR_IN_RAM, 0xff, QSPI_RAM_SIZE_MAX);
}

bool uiSaveSavestate(void)
{
	struct GameSelection selection;
	uint32_t ramSzExpected;

	if (!uiGetGameSelection(&selection))
		return false;

	ramSzExpected = selection.saveRamSize;
	if (ramSzExpected > QSPI_RAM_SIZE_MAX)
		return false;
	if (!ramSzExpected)
		return true;

	if (!uiPrvCartRamOwnerMatchesSelection(&selection)) {
		pr("Savegame cache: cart RAM is not owned by selected %s game; keeping QSPI copy (ram=%lu qspi=%08lx cart=%08lx ownerValid=%u)\n",
			uiPrvRuntimeName(selection.runtime), (unsigned long)ramSzExpected,
			(unsigned long)uiPrvSaveFingerprint((const void*)QSPI_RAM_COPY_START, ramSzExpected),
			(unsigned long)uiPrvSaveFingerprint(CART_RAM_ADDR_IN_RAM, ramSzExpected),
			mCartRamOwner.valid ? 1u : 0u);
		return true;
	}

	if (ramSzExpected && memcmp((const void*)QSPI_RAM_COPY_START, CART_RAM_ADDR_IN_RAM, ramSzExpected)) {
		uint32_t writeSz = (ramSzExpected + QSPI_WRITE_GRANULARITY - 1) / QSPI_WRITE_GRANULARITY * QSPI_WRITE_GRANULARITY;
		uint32_t erzSz = (ramSzExpected + QSPI_ERASE_GRANULARITY - 1) / QSPI_ERASE_GRANULARITY * QSPI_ERASE_GRANULARITY;

		pr("Savegame cache: updating QSPI from cart RAM (%lu bytes, qspi=%08lx cart=%08lx)\n",
			(unsigned long)ramSzExpected,
			(unsigned long)uiPrvSaveFingerprint((const void*)QSPI_RAM_COPY_START, ramSzExpected),
			(unsigned long)uiPrvSaveFingerprint(CART_RAM_ADDR_IN_RAM, ramSzExpected));
		if (!flashWrite(QSPI_RAM_COPY_START, erzSz, CART_RAM_ADDR_IN_RAM, writeSz))
			return false;
		pr("Savegame cache: QSPI updated (qspi=%08lx)\n",
			(unsigned long)uiPrvSaveFingerprint((const void*)QSPI_RAM_COPY_START, ramSzExpected));
	}
	
	return true;
}

#ifndef NO_SD_CARD
	#define IR_POWER_FILE			"/IR/POWER.IR"
	#define IR_FLIPPER_TV_FILE		"/IR/tv.ir"
	#define IR_FLIPPER_AC_FILE		"/IR/ac.ir"
	#define IR_FLIPPER_AUDIO_FILE	"/IR/audio.ir"
	#define IR_FLIPPER_PROJECTORS_FILE	"/IR/projectors.ir"
	#define IR_FLIPPER_FANS_FILE	"/IR/fans.ir"
	#define IR_FLIPPER_LEDS_FILE	"/IR/leds.ir"
	#define IR_FLIPPER_MONITOR_FILE	"/IR/monitor.ir"
	#define IR_FLIPPER_BLURAY_DVD_FILE	"/IR/bluray_dvd.ir"
	#define IR_FLIPPER_DIGITAL_SIGN_FILE	"/IR/digital_sign.ir"
	#define IR_FILE_MAGIC			"DC32IR1"
	#define IR_DEFAULT_CARRIER		38000
	#define IR_DEFAULT_REPEAT		1
	#define IR_LINE_BUF_SZ			32768
	#define IR_NAME_BUF_SZ			64
	#define IR_PROTOCOL_BUF_SZ		12
	#define IR_MIN_CARRIER			1000
	#define IR_MAX_CARRIER			100000
	#define IR_MAX_REPEAT			50
	#define IR_MAX_RAW_DURATION		1000000
	#define IR_MAX_RAW_DURATIONS	4096
	#define IR_MAX_RAW_REPEAT_USEC	10000000
	#define IR_PARSED_RECORD_TIMEOUT_USEC 1000000
	#define IR_RAW_CANCEL_CHUNK_USEC	1000
	struct IrBlastStats {
		uint32_t sent;
		uint32_t skipped;
		uint32_t malformed;
		uint32_t lineNo;
		uint32_t bytesRead;
		uint32_t fileSize;
		uint32_t recordIndex;
		uint32_t stalledCode;
		uint32_t stalledRecord;
		uint32_t stalledLine;
		char stalledName[IR_NAME_BUF_SZ];
		char stalledDetail[IR_PROTOCOL_BUF_SZ];
		bool cancelled;
		bool lineTooLong;
		bool timedOut;
	};

	enum FlipperIrType {
		FlipperIrTypeNone,
		FlipperIrTypeRaw,
		FlipperIrTypeParsed,
	};

	struct FlipperIrRecord {
		char name[IR_NAME_BUF_SZ];
		char protocol[IR_PROTOCOL_BUF_SZ];
		enum FlipperIrType type;
		uint32_t frequency;
		uint32_t address;
		uint32_t command;
		uint32_t lineNo;
		uint32_t recordIndex;
		bool hasName;
		bool hasFrequency;
		bool hasProtocol;
		bool hasAddress;
		bool hasCommand;
		bool sentRaw;
	};

	struct IrSendCtx {
		struct IrBlastStats *stats;
		uint64_t deadline;
		uint32_t codeIdx;
		uint32_t recordIndex;
		uint32_t lineNo;
		const char *name;
		const char *detail;
	};

	struct IrUniversalRemote {
		const char *name;
		const char *path;
		const char *const *buttons;
		uint_fast8_t numButtons;
	};

	static const char *const mIrTvButtons[] = {"Power", "Vol_up", "Vol_dn", "Ch_next", "Ch_prev", "Mute"};
	static const char *const mIrAcButtons[] = {"Off", "Cool_hi", "Cool_lo", "Heat_hi", "Heat_lo", "Dh"};
	static const char *const mIrAudioButtons[] = {"Power", "Vol_up", "Vol_dn", "Mute", "Play", "Pause", "Next", "Prev"};
	static const char *const mIrProjectorsButtons[] = {"Power", "Vol_up", "Vol_dn", "Mute", "Play", "Pause"};
	static const char *const mIrFansButtons[] = {"Power", "Speed_up", "Speed_dn", "Mode", "Rotate", "Timer"};
	static const char *const mIrLedsButtons[] = {"Power_on", "Power_off", "Brightness_up", "Brightness_dn", "White", "Red", "Green", "Blue"};
	static const char *const mIrMonitorButtons[] = {"POWER", "SOURCE", "MENU", "EXIT"};
	static const char *const mIrBlurayDvdButtons[] = {"Power", "Play", "Pause", "Ok", "Eject", "Fast_fo", "Fast_ba", "Subtitle"};
	static const char *const mIrDigitalSignButtons[] = {"POWER", "SOURCE", "PLAY", "STOP"};

	static const struct IrUniversalRemote mIrUniversalRemotes[] = {
		{"TV", IR_FLIPPER_TV_FILE, mIrTvButtons, sizeof(mIrTvButtons) / sizeof(*mIrTvButtons)},
		{"A/C", IR_FLIPPER_AC_FILE, mIrAcButtons, sizeof(mIrAcButtons) / sizeof(*mIrAcButtons)},
		{"Audio", IR_FLIPPER_AUDIO_FILE, mIrAudioButtons, sizeof(mIrAudioButtons) / sizeof(*mIrAudioButtons)},
		{"Projectors", IR_FLIPPER_PROJECTORS_FILE, mIrProjectorsButtons, sizeof(mIrProjectorsButtons) / sizeof(*mIrProjectorsButtons)},
		{"Fans", IR_FLIPPER_FANS_FILE, mIrFansButtons, sizeof(mIrFansButtons) / sizeof(*mIrFansButtons)},
		{"LEDs", IR_FLIPPER_LEDS_FILE, mIrLedsButtons, sizeof(mIrLedsButtons) / sizeof(*mIrLedsButtons)},
		{"Monitor", IR_FLIPPER_MONITOR_FILE, mIrMonitorButtons, sizeof(mIrMonitorButtons) / sizeof(*mIrMonitorButtons)},
		{"Blu-ray/DVD", IR_FLIPPER_BLURAY_DVD_FILE, mIrBlurayDvdButtons, sizeof(mIrBlurayDvdButtons) / sizeof(*mIrBlurayDvdButtons)},
		{"Digital Sign", IR_FLIPPER_DIGITAL_SIGN_FILE, mIrDigitalSignButtons, sizeof(mIrDigitalSignButtons) / sizeof(*mIrDigitalSignButtons)},
	};

	static const char *uiPrvBaseName(const char *path)
	{
		const char *base = path;

		while (*path) {
			if (*path == '/' || *path == '\\')
				base = path + 1;
			path++;
		}

		return base;
	}

	static void uiPrvRomStemBounds(const char *path, const char **baseP, const char **endP)
	{
		const char *base = uiPrvBaseName(path);
		const char *end = base + strlen(base);
		const char *dot;

		for (dot = end; dot != base; dot--) {
			if (dot[-1] == '.') {
				end = dot - 1;
				break;
			}
		}

		*baseP = base;
		*endP = end;
	}

	static void uiPrvCopyRomStem(char *dst, uint32_t dstLen, const char *path)
	{
		const char *base, *end, *src;
		uint32_t pos = 0;

		if (!dstLen)
			return;

		uiPrvRomStemBounds(path, &base, &end);
		for (src = base; src != end && pos + 1 < dstLen; src++)
			dst[pos++] = *src;
		dst[pos] = 0;
	}

	static void uiPrvCleanGameTitleFromPath(char *dst, uint32_t dstLen, const char *path, const char *fallbackName)
	{
		const char *base, *end, *src;
		uint32_t outLen = 0, parenDepth = 0;
		bool pendingSpace = false;

		if (!dstLen)
			return;
		if (!fallbackName)
			fallbackName = "";

		uiPrvRomStemBounds(path, &base, &end);
		while (base != end && (*base == ' ' || *base == '\t'))
			base++;
		while (end != base && (end[-1] == ' ' || end[-1] == '\t'))
			end--;

		for (src = base; src != end; src++) {
			char ch = *src;

			if (ch == '(') {
				parenDepth++;
				continue;
			}
			if (ch == ')') {
				if (parenDepth)
					parenDepth--;
				continue;
			}
			if (parenDepth)
				continue;

			if (ch == ' ' || ch == '\t') {
				if (outLen)
					pendingSpace = true;
				continue;
			}

			if (pendingSpace) {
				if (outLen + 2 >= dstLen)
					break;
				dst[outLen++] = ' ';
			}
			else if (outLen + 1 >= dstLen)
				break;

			pendingSpace = false;
			dst[outLen++] = ch;
		}

		dst[outLen] = 0;
		if (!outLen)
			uiPrvCopyStr(dst, dstLen, fallbackName);
	}

	static void uiPrvRomDisplayName(char *dst, uint32_t dstLen, const char *name)
	{
		char fallbackStem[UI_PICK_FILE_NAME_BUF_SZ];

		uiPrvCopyRomStem(fallbackStem, sizeof(fallbackStem), name);
		uiPrvCleanGameTitleFromPath(dst, dstLen, name, fallbackStem);
	}

	static bool uiPrvSaveNameKindValid(uint32_t kind)
	{
		return kind == SaveNameKindAuto ||
			kind == SaveNameKindClean ||
			kind == SaveNameKindFull ||
			kind == SaveNameKindFallback;
	}

	static enum SaveNameKind uiPrvSelectedSaveNameKind(void)
	{
		const struct GameSelectionFlashMeta *meta = (const struct GameSelectionFlashMeta*)(QSPI_FILENAME_START + GAME_META_OFFSET);

		if (meta->magic == GAME_META_MAGIC && uiPrvSaveNameKindValid(meta->reserved[0]))
			return (enum SaveNameKind)meta->reserved[0];
		return SaveNameKindAuto;
	}

	static bool uiPrvEraseGamePath(void)		//this will also mark the ROM as invalid
	{
		return flashWrite(QSPI_FILENAME_START, QSPI_FILENAME_MAXLEN, NULL, 0);
	}
	
	static bool uiPrvSetGamePath(const char *buf, enum GameRuntime runtime, uint32_t romSize, uint32_t saveRamSize, enum SaveNameKind saveNameKind)		//also marks ROM as possibly valid
	{
		uint8_t pathBuf[GAME_META_OFFSET];
		uint8_t metaBuf[QSPI_WRITE_GRANULARITY];
		struct GameSelectionFlashMeta *meta = (struct GameSelectionFlashMeta*)metaBuf;
		uint32_t pathLen = 0, writeSz;
		bool ret;

		memset(pathBuf, 0xff, sizeof(pathBuf));
		memset(metaBuf, 0xff, sizeof(metaBuf));
		*meta = (struct GameSelectionFlashMeta){
			.magic = GAME_META_MAGIC,
			.runtime = runtime,
			.romSize = romSize,
			.saveRamSize = saveRamSize,
		};
		meta->reserved[0] = saveNameKind;

		while (pathLen + 1 < sizeof(pathBuf) && buf[pathLen]) {
			pathBuf[pathLen] = buf[pathLen];
			pathLen++;
		}
		pathBuf[pathLen] = 0;
		writeSz = (pathLen + 1 + QSPI_WRITE_GRANULARITY - 1) / QSPI_WRITE_GRANULARITY * QSPI_WRITE_GRANULARITY;

		ret = flashWrite(QSPI_FILENAME_START, QSPI_FILENAME_MAXLEN, pathBuf, writeSz);
		if (!ret)
			return false;
		return flashWrite(QSPI_FILENAME_START + GAME_META_OFFSET, 0, metaBuf, QSPI_WRITE_GRANULARITY);
	}
	
	static bool uiPrvLoadFileWithOptions(struct Canvas *cnv, struct FatfsFil *fil, uint32_t flashAddr, const char *nameStr,
		uint32_t maxSize, uint8_t padByte, uint32_t preEraseSize)
	{
		uint32_t now, nowDone, pos, totalSz = fatfsFileGetSize(fil), bufSz = 32768, baseFlashAddr = flashAddr;
		struct ToolWorkspaceSpan bufMem;
		struct DcAppLoadingState loading = {
			.appName = "File transfer",
			.title = "Loading data",
			.detail = nameStr,
			.total = preEraseSize ? 0u : totalSz,
		};
		uint8_t *buf;
		bool ret = false;
		char msg[96];

		if (totalSz > maxSize) {
			(void)sprintf(msg, "%s is too large for this firmware. Max: %lu bytes", nameStr, (unsigned long)maxSize);
			uiAlert(cnv, msg, DialogTypeOk);
			return false;
		}

		if (!toolWorkspaceAcquire(ToolWorkspaceWram, ToolWorkspaceOwnerTransfer, &bufMem)) {
			uiAlert(cnv, "Tool workspace is busy; cannot load file", DialogTypeOk);
			return false;
		}
		buf = bufMem.ptr;

		if (bufSz > bufMem.size)
			bufSz = bufMem.size;
		bufSz &=~ (QSPI_ERASE_GRANULARITY - 1);
		dispPrvWaitForScanoutStart();
		dcAppDrawLoadingCanvas(cnv, &loading);

		if (preEraseSize && !flashWrite(baseFlashAddr, preEraseSize, NULL, 0)) {
			uiAlert(cnv, "Flash erase failure", DialogTypeOk);
			goto out_release;
		}
		if (preEraseSize) {
			loading.total = totalSz;
			dispPrvWaitForScanoutStart();
			dcAppDrawLoadingCanvas(cnv, &loading);
		}
		
		for (pos = 0; pos < totalSz; pos += now, flashAddr += now) {
			
			now = totalSz - pos;
			if (now > bufSz)
				now = bufSz;
			
			if (!fatfsFileRead(fil, buf, now, &nowDone) || now != nowDone) {
				uiAlert(cnv, "File reading failure", DialogTypeOk);
				goto out_release;
			}
	
			//over-erasing up to boundary is safe, same for writing
			nowDone = (now + QSPI_WRITE_GRANULARITY - 1) / QSPI_WRITE_GRANULARITY * QSPI_WRITE_GRANULARITY;
			if (nowDone != now)
				memset(buf + now, padByte, nowDone - now);
			if (!flashWrite(flashAddr, preEraseSize ? 0 : (now + QSPI_ERASE_GRANULARITY - 1) / QSPI_ERASE_GRANULARITY * QSPI_ERASE_GRANULARITY, buf, nowDone)) {
				uiAlert(cnv, "Flash writing failure", DialogTypeOk);
				goto out_release;
			}
			loading.done = pos + now;
			loading.animationStep++;
			dispPrvWaitForScanoutStart();
			dcAppDrawLoadingCanvas(cnv, &loading);
		}
		
		ret = true;
	out_release:
		toolWorkspaceRelease(ToolWorkspaceWram, ToolWorkspaceOwnerTransfer);
		return ret;
	}

	static bool uiPrvLoadFile(struct Canvas *cnv, struct FatfsFil *fil, uint32_t flashAddr, const char *nameStr, uint32_t maxSize)
	{
		return uiPrvLoadFileWithOptions(cnv, fil, flashAddr, nameStr, maxSize, 0x00, 0);
	}

	static bool uiPrvVerifySaveFileInFlash(struct Canvas *cnv, struct FatfsFil *fil, uint32_t expectedSize)
	{
		struct ToolWorkspaceSpan bufMem;
		uint8_t *buf;
		uint32_t pos = 0;
		bool ret = false;

		if (!fatfsFileSeek(fil, 0)) {
			uiAlert(cnv, "Cannot seek save file for verification", DialogTypeOk);
			return false;
		}

		if (!toolWorkspaceAcquire(ToolWorkspaceWram, ToolWorkspaceOwnerTransfer, &bufMem)) {
			uiAlert(cnv, "Tool workspace is busy; cannot verify save", DialogTypeOk);
			return false;
		}
		buf = bufMem.ptr;

		while (pos < expectedSize) {
			uint32_t now = expectedSize - pos, numRead = 0;

			if (now > bufMem.size)
				now = bufMem.size;
			if (!fatfsFileRead(fil, buf, now, &numRead) || numRead != now) {
				uiAlert(cnv, "Save verification read failure", DialogTypeOk);
				goto out_release;
			}
			if (memcmp(buf, (const uint8_t*)QSPI_RAM_COPY_START + pos, now)) {
				uiAlert(cnv, "Save verification mismatch", DialogTypeOk);
				goto out_release;
			}
			pos += now;
		}

		ret = true;
	out_release:
		toolWorkspaceRelease(ToolWorkspaceWram, ToolWorkspaceOwnerTransfer);
		return ret;
	}

	static bool uiPrvLoadSaveFile(struct Canvas *cnv, struct FatfsFil *fil, uint32_t expectedSize)
	{
		if (fatfsFileGetSize(fil) != expectedSize) {
			uiAlert(cnv, "Savegame file size does not match the expected size", DialogTypeOk);
			return false;
		}
		if (!uiPrvLoadFileWithOptions(cnv, fil, QSPI_RAM_COPY_START, "SAVE", QSPI_RAM_SIZE_MAX, 0xff, QSPI_RAM_SIZE_MAX))
			return false;
		if (!uiPrvVerifySaveFileInFlash(cnv, fil, expectedSize))
			return false;
		pr("Savegame import: loaded %lu bytes into QSPI cache (hash=%08lx)\n",
			(unsigned long)expectedSize, (unsigned long)uiPrvSaveFingerprint((const void*)QSPI_RAM_COPY_START, expectedSize));
		return true;
	}

	static bool uiPrvCleanSaveStem(char *dst, uint32_t dstLen, const char *stem)
	{
		uint32_t outLen = 0;
		bool pendingSpace = false;

		if (!dstLen)
			return false;
		if (!stem)
			stem = "";

		while (*stem) {
			uint8_t ch = (uint8_t)*stem++;
			bool valid = ch >= 0x20 && ch != 0x7f &&
				ch != '\\' && ch != '/' && ch != ':' && ch != '*' &&
				ch != '?' && ch != '"' && ch != '<' && ch != '>' &&
				ch != '|';

			if (!valid || ch == ' ' || ch == '\t') {
				if (outLen)
					pendingSpace = true;
				continue;
			}

			if (pendingSpace) {
				if (outLen + 2 >= dstLen)
					break;
				dst[outLen++] = ' ';
				pendingSpace = false;
			}
			else if (outLen + 1 >= dstLen)
				break;

			dst[outLen++] = (char)ch;
		}
		dst[outLen] = 0;
		return outLen != 0;
	}

	static bool uiPrvDetectedSaveTitle(enum GameRuntime runtime, const char *detectedName, char *dst, uint32_t dstLen)
	{
		char stem[UI_PICK_FILE_NAME_BUF_SZ];

		if (!uiPrvCleanSaveStem(stem, sizeof(stem), detectedName))
			return false;
		if (runtime == GameRuntimeNes && !strncmp(stem, "NES M", 5))
			return false;
		if (runtime == GameRuntimeArduboy && !strcmp(stem, "ARDUBOY"))
			return false;
		uiPrvCopyStr(dst, dstLen, stem);
		return true;
	}

	static void uiPrvSaveNameFromStem(const char *stem, char *dst, uint32_t dstLen);

	static void uiPrvSaveFileName(const char *romName, enum GameRuntime runtime, const char *detectedName, char *dst, uint32_t dstLen)
	{
		char stem[UI_PICK_FILE_NAME_BUF_SZ];

		(void)runtime;
		(void)detectedName;
		uiPrvCopyRomStem(stem, sizeof(stem), romName);
		uiPrvSaveNameFromStem(stem, dst, dstLen);
	}

	static void uiPrvCleanSaveFileName(const char *romName, enum GameRuntime runtime, const char *detectedName, char *dst, uint32_t dstLen)
	{
		char stem[UI_PICK_FILE_NAME_BUF_SZ];
		char fallbackStem[UI_PICK_FILE_NAME_BUF_SZ];

		if (!uiPrvDetectedSaveTitle(runtime, detectedName, stem, sizeof(stem))) {
			uiPrvCopyRomStem(fallbackStem, sizeof(fallbackStem), romName);
			uiPrvCleanGameTitleFromPath(stem, sizeof(stem), romName, fallbackStem);
		}
		uiPrvSaveNameFromStem(stem, dst, dstLen);
	}

	static void uiPrvSaveNameFromStem(const char *stem, char *dst, uint32_t dstLen)
	{
		char cleanStem[UI_PICK_FILE_NAME_BUF_SZ];
		uint32_t pos = 0;

		if (!dstLen)
			return;

		if (!uiPrvCleanSaveStem(cleanStem, sizeof(cleanStem), stem))
			uiPrvCopyStr(cleanStem, sizeof(cleanStem), "SAVE");

		stem = cleanStem;
		while (*stem && pos + 5 < dstLen)
			dst[pos++] = *stem++;
		if (pos + 4 < dstLen) {
			dst[pos++] = '.';
			dst[pos++] = 's';
			dst[pos++] = 'a';
			dst[pos++] = 'v';
		}
		dst[pos] = 0;
	}

	static uint8_t uiPrvSaveFallbackChar(char c)
	{
		if (c >= 'a' && c <= 'z')
			c += 'A' - 'a';
		if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
			return (uint8_t)c;
		return 0;
	}

	static void uiPrvSaveFallbackFileName(const char *romName, enum GameRuntime runtime, char *dst, uint32_t dstLen)
	{
		static const char hex[] = "0123456789ABCDEF";
		const char *base, *dot = NULL, *src;
		uint32_t hash = 2166136261u, pos = 0;
		uint16_t hash12;

		(void)runtime;
		if (!dstLen)
			return;

		base = uiPrvBaseName(romName);
		for (src = base; *src; src++) {
			if (*src == '.')
				dot = src;
		}
		if (!dot)
			dot = src;

		if (dstLen < 13) {
			dst[0] = 0;
			return;
		}

		for (src = base; src != dot; src++) {
			hash ^= (uint8_t)*src;
			hash *= 16777619u;
		}
		hash12 = (uint16_t)((hash ^ (hash >> 12) ^ (hash >> 24)) & 0x0fffu);

		for (src = base; src != dot && pos < 5; src++) {
			uint8_t c = uiPrvSaveFallbackChar(*src);

			if (c)
				dst[pos++] = (char)c;
		}
		while (pos < 5)
			dst[pos++] = 'X';
		dst[pos++] = hex[(hash12 >> 8) & 0x0f];
		dst[pos++] = hex[(hash12 >> 4) & 0x0f];
		dst[pos++] = hex[hash12 & 0x0f];
		dst[pos++] = '.';
		dst[pos++] = 'S';
		dst[pos++] = 'A';
		dst[pos++] = 'V';
		dst[pos] = 0;
	}

	static struct FatfsFil *uiPrvOpenSaveFileIfSizeMatches(struct FatfsDir *dir, const char *saveName, uint32_t expectedSize, bool *badSizeP)
	{
		struct FatfsFil *fil = fatfsFileOpenAt(dir, saveName, OPEN_MODE_READ);

		if (!fil)
			return NULL;
		if (fatfsFileGetSize(fil) == expectedSize)
			return fil;

		pr("Savegame file size does not match the expected size. It will not be loaded. Expected %u, file is %u\n", expectedSize, fatfsFileGetSize(fil));
		if (badSizeP)
			*badSizeP = true;
		fatfsFileClose(fil);
		return NULL;
	}

	static struct FatfsFil *uiPrvOpenSaveFile(struct FatfsVol *vol, const char *romName, enum GameRuntime runtime,
		enum UiEmulatorConsole console, const char *detectedName, uint32_t expectedSize,
		enum SaveNameKind *saveNameKindP, bool *badSizeP)
	{
		struct FatfsDir *dir;
		struct FatfsFil *fil = NULL;
		char saveName[UI_PICK_FILE_NAME_BUF_SZ];
		char cleanSaveName[UI_PICK_FILE_NAME_BUF_SZ];
		char saveDirPath[16];
		char fallbackName[13];
		const char *foundName = NULL;
		enum SaveNameKind foundKind = SaveNameKindAuto;

		if (!expectedSize)
			return NULL;

		uiPrvSaveDirPathForConsole(console, saveDirPath, sizeof(saveDirPath));
		dir = uiPrvOpenSaveDirForConsole(vol, console, false);
		if (!dir) {
			pr("Savegame import: no save folder %s for %s (%lu bytes expected)\n",
				saveDirPath, romName, (unsigned long)expectedSize);
			return NULL;
		}

		uiPrvSaveFileName(romName, runtime, detectedName, saveName, sizeof(saveName));
		fil = uiPrvOpenSaveFileIfSizeMatches(dir, saveName, expectedSize, badSizeP);
		if (fil) {
			foundName = saveName;
			foundKind = SaveNameKindFull;
			if (saveNameKindP)
				*saveNameKindP = foundKind;
		}
		uiPrvCleanSaveFileName(romName, runtime, detectedName, cleanSaveName, sizeof(cleanSaveName));
		if (!fil && cleanSaveName[0] && strcmp(saveName, cleanSaveName)) {
			fil = uiPrvOpenSaveFileIfSizeMatches(dir, cleanSaveName, expectedSize, badSizeP);
			if (fil) {
				foundName = cleanSaveName;
				foundKind = SaveNameKindClean;
				if (saveNameKindP)
					*saveNameKindP = foundKind;
			}
		}
		uiPrvSaveFallbackFileName(romName, runtime, fallbackName, sizeof(fallbackName));
		if (!fil && fallbackName[0] && strcmp(saveName, fallbackName) && strcmp(cleanSaveName, fallbackName)) {
			fil = uiPrvOpenSaveFileIfSizeMatches(dir, fallbackName, expectedSize, badSizeP);
			if (fil) {
				foundName = fallbackName;
				foundKind = SaveNameKindFallback;
				if (saveNameKindP)
					*saveNameKindP = foundKind;
			}
		}

		if (fil)
			pr("Savegame import: found %s/%s (%lu bytes, source=%s) for %s\n",
				saveDirPath, foundName, (unsigned long)expectedSize, uiPrvSaveNameKindName(foundKind), romName);
		else
			pr("Savegame import: no matching save found in %s for %s (%lu bytes expected)\n",
				saveDirPath, romName, (unsigned long)expectedSize);
		fatfsDirClose(dir);
		return fil;
	}

	static void uiPrvDetectedSaveTitleForSelection(const struct GameSelection *selection, char *dst, uint32_t dstLen)
	{
		if (!dstLen)
			return;
		dst[0] = 0;
		if (!selection)
			return;

		if (selection->runtime == GameRuntimeGb) {
			char name[ROM_NAME_LEN + 1];
			uint32_t romSize, saveRamSize;
			enum RomColorSupport colorSupport;

			name[ROM_NAME_LEN] = 0;
			if (mbcRomAnalyze((const void*)QSPI_ROM_START, &romSize, &saveRamSize, &colorSupport, name))
				uiPrvCopyStr(dst, dstLen, name);
		}
		else if (selection->runtime == GameRuntimeNes) {
			struct NesRomInfo info;

			if (nesAnalyzeRom((const void*)QSPI_ROM_START, selection->romSize ? selection->romSize : QSPI_ROM_SIZE_MAX, &info))
				uiPrvCopyStr(dst, dstLen, info.name);
		}
		else if (selection->runtime == GameRuntimeArduboy) {
			struct ArduboyRomInfo info;

			if (arduboyAnalyzeRom((const void*)QSPI_ROM_START, selection->romSize ? selection->romSize : QSPI_ROM_SIZE_MAX, &info) && !info.isPackage)
				uiPrvCopyStr(dst, dstLen, info.name);
		}
	}

	enum SaveExportStatus {
		SaveExportStatusOk,
		SaveExportStatusMountFailed,
		SaveExportStatusSaveDirFailed,
		SaveExportStatusCreateFailed,
		SaveExportStatusOpenExistingFailed,
		SaveExportStatusOpenCreatedFailed,
		SaveExportStatusWriteFailed,
		SaveExportStatusTruncateFailed,
		SaveExportStatusFileCloseFailed,
		SaveExportStatusDirCloseFailed,
		SaveExportStatusStreamCloseFailed,
		SaveExportStatusUnmountFailed,
		SaveExportStatusVerifyMountFailed,
		SaveExportStatusVerifySaveDirFailed,
		SaveExportStatusVerifyOpenFailed,
		SaveExportStatusVerifySizeMismatch,
		SaveExportStatusVerifyReadFailed,
		SaveExportStatusVerifyMismatch,
		SaveExportStatusVerifyFileCloseFailed,
		SaveExportStatusVerifyDirCloseFailed,
		SaveExportStatusVerifyStreamCloseFailed,
		SaveExportStatusVerifyUnmountFailed,
	};

#define SAVE_EXPORT_MSG_BUF_SZ	1024

	struct SaveExportResult {
		enum SaveExportStatus status;
		enum UiEmulatorConsole saveConsole;
		char saveDirPath[16];
		char saveName[UI_PICK_FILE_NAME_BUF_SZ];
		char primarySaveName[UI_PICK_FILE_NAME_BUF_SZ];
		char cleanSaveName[UI_PICK_FILE_NAME_BUF_SZ];
		char fallbackSaveName[13];
		const char *openPath;
		uint32_t bytesExpected;
		uint32_t bytesDone;
		uint32_t verifyOffset;
		enum SaveNameKind preferredSaveNameKind;
		enum SaveNameKind chosenSaveNameKind;
		enum FatfsCreateError fatCreateError;
		const char *fatCreateErrorName;
		enum FatfsCreateError primaryCreateError;
		const char *primaryCreateErrorName;
		enum FatfsCreateError fallbackCreateError;
		const char *fallbackCreateErrorName;
		uint8_t verifyExpected;
		uint8_t verifyActual;
		uint8_t sdFlags;
		uint8_t attemptNo;
		bool flashCacheFailed;
		bool cardReportedReadOnly;
		bool fallbackUsed;
		bool forceRequested;
		bool forceReinitUsed;
		bool retryAttempted;
		bool manualRequest;
		bool nothingToExport;
		struct SdLastError sdError;
	};

	static bool uiPrvFlushCurrentSaveToCardEx(bool force, struct SaveExportResult *result);
	static bool uiPrvExportCurrentSavestate(struct Canvas *cnv, bool manual);
	static bool uiPrvExportCurrentSavestateToMountedCard(struct Canvas *cnv, struct FatfsVol *vol, bool manual);
	static bool uiPrvExportCurrentSavestateWithOptions(struct Canvas *cnv, bool force, bool showSuccess);
	static bool uiPrvExportCurrentSavestateToMountedCardWithOptions(struct Canvas *cnv, struct FatfsVol *vol, bool force, bool showSuccess);

	static bool uiPrvSelectionMatchesCachedGame(const char *romName, enum GameRuntime runtime, uint32_t romSize, uint32_t saveRamSize)
	{
		struct GameSelection selection;

		if (!uiGetGameSelection(&selection))
			return false;
		return selection.runtime == runtime &&
			selection.romSize == romSize &&
			selection.saveRamSize == saveRamSize &&
			!strcmp((const char*)QSPI_FILENAME_START, romName);
	}

	static bool uiPrvPrepareForRomReplacement(struct Canvas *cnv, struct FatfsVol *vol, const char *reason)
	{
		struct GameSelection selection;
		uint32_t savegameExportSz = uiGetGameSelection(&selection) ? selection.saveRamSize : 0;

		uiPrvSetHeaderTitle("Select Game");
		if (savegameExportSz) {
			pr("Game selection: exporting current save before ROM replacement (%s, %lu bytes)\n",
				reason ? reason : "unknown", (unsigned long)savegameExportSz);
			if (!(vol ? uiPrvExportCurrentSavestateToMountedCardWithOptions(cnv, vol, false, true) :
				uiPrvExportCurrentSavestateWithOptions(cnv, false, true))) {
				uiPrvSetHeaderTitle("Select Game");
				return false;
			}
		}
		uiPrvCartRamOwnerClear(reason ? reason : "ROM replacement");
		uiPrvSetHeaderTitle("Select Game");
		return true;
	}

	static bool __attribute__((noinline)) uiPrvConfirmRomSelection(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *romLocator, const char *romName)
	{
		uint32_t numRead, fileSz, romSzExpected, ramSzExpected, col = 1, row = 17;
		char internalName[ROM_NAME_LEN + 1];
		struct FatfsFil *filR, *filS = NULL;
		enum RomColorSupport colorSupport;
		enum GameRuntime runtime = uiPrvRuntimeForName(romName);
		enum SaveNameKind saveNameKind = SaveNameKindFull;
		struct NesRomInfo nesInfo;
		struct ArduboyRomInfo arduboyInfo;
		struct FatfsDir *dir;
		struct CartHeader hdr;
		enum UiEmulatorConsole saveConsole;
		bool ret = false, badSaveSize = false, preserveCachedSave = false, arduboyPackage = false;
		
		static const char colorTypes[][10] = {
			[RomNoColor] = "B&W",
			[RomColorEnhanced] = "Supported",
			[RomColorRequired] = "Required",
		};
			
		if (romLocator)
			filR = fatfsFileOpenWithLocator(vol, romLocator, OPEN_MODE_READ);
		else {
			dir = fatfsDirOpen(vol, "/ROMS");
			if (!dir)
				goto out;
			filR = fatfsFileOpenAt(dir, romName, OPEN_MODE_READ);
			fatfsDirClose(dir);
			dir = NULL;
		}
		if (!filR) {
			uiAlert(cnv, "Cannot open ROM file", DialogTypeOk);
			goto out;
		}
		
		fileSz = fatfsFileGetSize(filR);
		
		if (runtime == GameRuntimeNes) {
			uint8_t nesHdr[16];

			if (fileSz < sizeof(nesHdr) || !fatfsFileRead(filR, nesHdr, sizeof(nesHdr), &numRead) || numRead != sizeof(nesHdr)) {
				uiAlert(cnv, "Cannot read ROM file", DialogTypeOk);
				goto out_close_file;
			}
			if (!nesAnalyzeRom(nesHdr, fileSz, &nesInfo)) {
				uiAlert(cnv, "Does not appear to be a valid NES ROM file", DialogTypeOk);
				goto out_close_file;
			}
			romSzExpected = nesInfo.romSize;
			ramSzExpected = nesInfo.saveRamSize;
			colorSupport = RomNoColor;
			(void)sprintf(internalName, "%s", nesInfo.name);
		}
		else if (runtime == GameRuntimeArduboy) {
			memset(&arduboyInfo, 0, sizeof(arduboyInfo));
			arduboyPackage = uiPrvStrEndsWithNoCase(romName, ".arduboy");
			romSzExpected = fileSz;
			ramSzExpected = ARDUBOY_SAVE_RAM_SIZE;
			colorSupport = RomNoColor;
			(void)sprintf(internalName, "ARDUBOY");
		}
		else if (fileSz < sizeof(hdr) || !fatfsFileRead(filR, &hdr, sizeof(hdr), &numRead) || numRead != sizeof(hdr)) {
			
			uiAlert(cnv, "Cannot read ROM file", DialogTypeOk);
			goto out_close_file;
		}
		
		else if (!mbcRomAnalyze(&hdr, &romSzExpected, &ramSzExpected, &colorSupport, internalName)) {
			uiAlert(cnv, "Does not appear to be a valid ROM file", DialogTypeOk);
			goto out_close_file;
		}
		internalName[ROM_NAME_LEN] = 0;

		if (romSzExpected > QSPI_ROM_SIZE_MAX) {
			char msg[96];

			(void)sprintf(msg, "ROM is too large for this firmware. Max: %lu bytes", (unsigned long)QSPI_ROM_SIZE_MAX);
			uiAlert(cnv, msg, DialogTypeOk);
			goto out_close_file;
		}

		if (ramSzExpected > QSPI_RAM_SIZE_MAX) {
			char msg[96];

			(void)sprintf(msg, "Cart save RAM is too large for this firmware. Max: %lu bytes", (unsigned long)QSPI_RAM_SIZE_MAX);
			uiAlert(cnv, msg, DialogTypeOk);
			goto out_close_file;
		}
	
		if (fileSz != romSzExpected) {
		
			uiAlert(cnv, "ROM file size does not match the expected size", DialogTypeOk);
			goto out_close_file;
		}

		saveConsole = uiPrvSaveConsoleForGame(runtime, colorSupport, romName);
		filS = uiPrvOpenSaveFile(vol, romName, runtime, saveConsole, internalName, ramSzExpected, &saveNameKind, &badSaveSize);
		if (!filS && badSaveSize) {
			uiAlert(cnv, "Savegame file size does not match the expected size. It will not be loaded.", DialogTypeOk);
			goto out_close_file;
		}
		preserveCachedSave = !filS && ramSzExpected && uiPrvSelectionMatchesCachedGame(romName, runtime, romSzExpected, ramSzExpected);
		if (!preserveCachedSave && !filS && ramSzExpected && runtime == GameRuntimeArduboy && arduboyPackage) {
			struct GameSelection cachedSelection;

			preserveCachedSave = uiGetGameSelection(&cachedSelection) &&
				cachedSelection.runtime == runtime &&
				cachedSelection.saveRamSize == ramSzExpected &&
				!strcmp((const char*)QSPI_FILENAME_START, romName);
		}
		if (preserveCachedSave)
			saveNameKind = uiPrvSelectedSaveNameKind();
	
		if (!fatfsFileSeek(filR, 0)) {
			
			uiAlert(cnv, "Cannot seek ROM file", DialogTypeOk);
			goto out_close_file;
		}

		uiPrvSetHeaderTitle("Select Game");
		cnv->font = FontLarge;
		row = uiPrvGlyphHeight(cnv) + 1;
		uiPrvReset(cnv, false);
		
		cnv->foreColor = 10;
		uiPuts(cnv, row, col, "ROM:", -1);
		cnv->foreColor = 15;
		row += 1 + uiPrvGlyphHeight(cnv) * uiPrvDrawWrappedString(cnv, romName, row, col + 55);
		
		cnv->foreColor = 10;
		uiPuts(cnv, row, col, "SIZE:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, row, col + 55, "%u bytes", fileSz);
		row += 1 + uiPrvGlyphHeight(cnv);
		
		if (runtime == GameRuntimeNes) {
			cnv->foreColor = 10;
			uiPuts(cnv, row, col, "SYSTEM:", -1);
			cnv->foreColor = 15;
			uiPuts(cnv, row, col + 55, "NES", -1);
			row += 1 + uiPrvGlyphHeight(cnv);

			cnv->foreColor = 10;
			uiPuts(cnv, row, col, "MAPPER:", -1);
			cnv->foreColor = 15;
			uiPrintf(cnv, row, col + 55, "%u", nesInfo.mapper);
			row += 1 + uiPrvGlyphHeight(cnv);

			cnv->foreColor = 10;
			uiPuts(cnv, row, col, "REGION:", -1);
			cnv->foreColor = 15;
			uiPuts(cnv, row, col + 55, uiPrvNesRegionName(nesInfo.region), -1);
			row += 1 + uiPrvGlyphHeight(cnv);
		}
		else if (runtime == GameRuntimeArduboy) {
			cnv->foreColor = 10;
			uiPuts(cnv, row, col, "SYSTEM:", -1);
			cnv->foreColor = 15;
			uiPuts(cnv, row, col + 55, "ARDUBOY", -1);
			row += 1 + uiPrvGlyphHeight(cnv);

			cnv->foreColor = 10;
			uiPuts(cnv, row, col, "TYPE:", -1);
			cnv->foreColor = 15;
			uiPuts(cnv, row, col + 55, arduboyPackage ? "PACKAGE" : "HEX", -1);
			row += 1 + uiPrvGlyphHeight(cnv);
		}
		else {
			cnv->foreColor = 10;
			uiPuts(cnv, row, col, "NAME:", -1);
			cnv->foreColor = 15;
			uiPrintf(cnv, row, col + 55, "%s", internalName);
			row += 1 + uiPrvGlyphHeight(cnv);
			
			cnv->foreColor = 10;
			uiPuts(cnv, row, col, "COLOR:", -1);
			cnv->foreColor = 15;
			uiPuts(cnv, row, col + 55, colorTypes[colorSupport], -1);
			row += 1 + uiPrvGlyphHeight(cnv);
		}
		
		cnv->foreColor = 10;
		uiPuts(cnv, row, col, "SAVE:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, row, col + 55, filS ? "FOUND" : "NOT FOUND", -1);
		row += 1 + uiPrvGlyphHeight(cnv);
		
		if (uiPrvGetSimpleAnswer(cnv, DialogTypeYesNo)) {
			
			//erase old path and thus mark the ROM as invalid. This will prevent a poweroff mid-load from causing us to try to play a half-loaded ROM
			uiPrvCartRamOwnerClear("selecting new ROM");
			(void)uiPrvEraseGamePath();
			
			if (runtime == GameRuntimeArduboy && arduboyPackage) {
				enum {
					ArduboyPackageScratchAddr = QSPI_ROM_START + QSPI_ROM_SIZE_MAX / 2,
					ArduboyPackageScratchSize = QSPI_ROM_SIZE_MAX / 2,
				};
				struct ToolWorkspaceSpan inflateDict = {0}, writeBuf = {0};
				uint32_t extractedHexSize = 0, extractedFxSize = 0;

				if (fileSz > ArduboyPackageScratchSize) {
					uiAlert(cnv, "Arduboy package is too large for this firmware", DialogTypeOk);
					ret = false;
				}
				else {
					ret = uiPrvLoadFile(cnv, filR, ArduboyPackageScratchAddr, "PKG", ArduboyPackageScratchSize);
					if (ret && (!toolWorkspaceAcquire(ToolWorkspaceWram, ToolWorkspaceOwnerTransfer, &inflateDict) ||
						!toolWorkspaceAcquire(ToolWorkspaceVram, ToolWorkspaceOwnerTransfer, &writeBuf))) {
						uiAlert(cnv, "Tool workspace is busy; cannot extract package", DialogTypeOk);
						ret = false;
					}
					if (ret) {
						ret = arduboyExtractPackageToFlash((const void*)ArduboyPackageScratchAddr, fileSz, QSPI_ROM_START,
							ArduboyPackageScratchSize, inflateDict.ptr, inflateDict.size, writeBuf.ptr, writeBuf.size,
							&extractedHexSize, &extractedFxSize);
						if (!ret)
							uiAlert(cnv, "Cannot extract Arduboy package", DialogTypeOk);
						else if (extractedFxSize)
							pr("Arduboy package: cached %lu bytes of FX data\n", (unsigned long)extractedFxSize);
					}
					if (inflateDict.ptr)
						toolWorkspaceRelease(ToolWorkspaceWram, ToolWorkspaceOwnerTransfer);
					if (writeBuf.ptr)
						toolWorkspaceRelease(ToolWorkspaceVram, ToolWorkspaceOwnerTransfer);
					if (ret) {
						romSzExpected = extractedHexSize;
						ret = arduboyAnalyzeRom((const void*)QSPI_ROM_START, romSzExpected, &arduboyInfo);
						if (!ret)
							uiAlert(cnv, "Extracted Arduboy HEX is invalid", DialogTypeOk);
					}
				}
			}
			else {
				if (runtime == GameRuntimeArduboy && !arduboyClearFxCache()) {
					uiAlert(cnv, "Cannot clear cached Arduboy FX data", DialogTypeOk);
					ret = false;
					goto out_close_file;
				}
				ret = uiPrvLoadFile(cnv, filR, QSPI_ROM_START, "ROM", QSPI_ROM_SIZE_MAX);
				if (ret && runtime == GameRuntimeArduboy) {
					ret = arduboyAnalyzeRom((const void*)QSPI_ROM_START, romSzExpected, &arduboyInfo);
					if (!ret)
						uiAlert(cnv, "Does not appear to be a valid Arduboy HEX file", DialogTypeOk);
				}
			}
			
			if (!ret) {
				//erase rom header if we failed to load the ROM fully
				(void)flashWrite(QSPI_ROM_START, QSPI_ERASE_GRANULARITY, NULL, 0);
				goto out_close_file;
			}
			
			if (filS) {
				ret = uiPrvLoadSaveFile(cnv, filS, ramSzExpected) && ret;
				fatfsFileClose(filS);
				filS = NULL;
			}
			else if (!preserveCachedSave)
				ret = flashWrite(QSPI_RAM_COPY_START, QSPI_RAM_SIZE_MAX, NULL, 0) && ret;

			if (ret)
				ret = uiPrvHydrateSaveRamFromFlash(runtime, romSzExpected, ramSzExpected, romName, "selection import", false);

			if (!ret) {
				(void)flashWrite(QSPI_ROM_START, QSPI_ERASE_GRANULARITY, NULL, 0);
				uiPrvCartRamOwnerClear("selection load failed");
				goto out_close_file;
			}

			ret = uiPrvSetGamePath(romName, runtime, romSzExpected, ramSzExpected, saveNameKind);
			if (!ret) {
				uiAlert(cnv, "Failed to store selected game metadata", DialogTypeOk);
				uiPrvCartRamOwnerClear("selection metadata failed");
			}
			else {
				uiPrvCartRamOwnerSet(runtime, romSzExpected, ramSzExpected, romName, "selection finalized");
				pr("Game selection: finalized %s save source=%s saveHash=%08lx\n",
					romName, uiPrvSaveNameKindName(saveNameKind),
					(unsigned long)uiPrvSaveFingerprint((const void*)QSPI_RAM_COPY_START, ramSzExpected));
			}
			
			uiPrvReset(cnv, false);
		}
		
	out_close_file:
		fatfsFileClose(filR);
		if (filS)
			fatfsFileClose(filS);
		
	out:
		return ret;
	}

static void uiPrvSaveExportResultInit(struct SaveExportResult *result)
{
	memset(result, 0, sizeof(*result));
	result->status = SaveExportStatusOk;
	result->sdError.name = "none";
	result->fatCreateErrorName = "none";
	result->primaryCreateErrorName = "none";
	result->fallbackCreateErrorName = "none";
	result->openPath = "none";
}

static void uiPrvSaveExportBeginAttempt(struct SaveExportResult *result)
{
	enum UiEmulatorConsole saveConsole = result->saveConsole;
	char saveDirPath[16];
	char primarySaveName[UI_PICK_FILE_NAME_BUF_SZ];
	char cleanSaveName[UI_PICK_FILE_NAME_BUF_SZ];
	char fallbackSaveName[13];
	uint32_t bytesExpected = result->bytesExpected;
	uint8_t attemptNo = result->attemptNo;
	enum SaveNameKind preferredSaveNameKind = result->preferredSaveNameKind;
	bool flashCacheFailed = result->flashCacheFailed;
	bool forceRequested = result->forceRequested;
	bool retryAttempted = result->retryAttempted;
	bool manualRequest = result->manualRequest;

	uiPrvCopyStr(saveDirPath, sizeof(saveDirPath), result->saveDirPath);
	uiPrvCopyStr(primarySaveName, sizeof(primarySaveName), result->primarySaveName);
	uiPrvCopyStr(cleanSaveName, sizeof(cleanSaveName), result->cleanSaveName);
	uiPrvCopyStr(fallbackSaveName, sizeof(fallbackSaveName), result->fallbackSaveName);
	uiPrvSaveExportResultInit(result);
	result->saveConsole = saveConsole;
	uiPrvCopyStr(result->saveDirPath, sizeof(result->saveDirPath), saveDirPath);
	uiPrvCopyStr(result->primarySaveName, sizeof(result->primarySaveName), primarySaveName);
	uiPrvCopyStr(result->cleanSaveName, sizeof(result->cleanSaveName), cleanSaveName);
	uiPrvCopyStr(result->fallbackSaveName, sizeof(result->fallbackSaveName), fallbackSaveName);
	uiPrvCopyStr(result->saveName, sizeof(result->saveName), primarySaveName);
	result->bytesExpected = bytesExpected;
	result->attemptNo = attemptNo;
	result->preferredSaveNameKind = preferredSaveNameKind;
	result->flashCacheFailed = flashCacheFailed;
	result->forceRequested = forceRequested;
	result->retryAttempted = retryAttempted;
	result->manualRequest = manualRequest;
}

static void uiPrvSaveExportSetStatus(struct SaveExportResult *result, enum SaveExportStatus status)
{
	if (result->status != SaveExportStatusOk)
		return;

	result->status = status;
	sdGetLastError(&result->sdError);
}

static void uiPrvSaveExportRecordFatCreateError(struct SaveExportResult *result)
{
	result->fatCreateError = fatfsLastCreateError();
	result->fatCreateErrorName = fatfsLastCreateErrorName();
}

static void uiPrvSaveExportRecordPrimaryCreateError(struct SaveExportResult *result)
{
	result->primaryCreateError = fatfsLastCreateError();
	result->primaryCreateErrorName = fatfsLastCreateErrorName();
	uiPrvSaveExportRecordFatCreateError(result);
}

static void uiPrvSaveExportRecordFallbackCreateError(struct SaveExportResult *result)
{
	result->fallbackCreateError = fatfsLastCreateError();
	result->fallbackCreateErrorName = fatfsLastCreateErrorName();
	uiPrvSaveExportRecordFatCreateError(result);
}

static bool uiPrvSaveExportNameExists(struct FatfsDir *saveDir, const char *name)
{
	struct FatFileLocator loc;

	return name && name[0] && fatfsFindFileAt(saveDir, name, &loc);
}

static void uiPrvSaveExportSetTarget(struct SaveExportResult *result, const char *name, enum SaveNameKind kind)
{
	uiPrvCopyStr(result->saveName, sizeof(result->saveName), name);
	result->chosenSaveNameKind = kind;
	result->fallbackUsed = kind == SaveNameKindFallback && strcmp(result->primarySaveName, name);
}

static bool uiPrvSaveExportNameMatches(const char *a, const char *b)
{
	return a && b && a[0] && b[0] && !strcmp(a, b);
}

static void uiPrvChooseSaveExportTarget(struct FatfsDir *saveDir, struct SaveExportResult *result)
{
	if (uiPrvSaveExportNameExists(saveDir, result->primarySaveName)) {
		uiPrvSaveExportSetTarget(result, result->primarySaveName, SaveNameKindFull);
		return;
	}

	switch (result->preferredSaveNameKind) {
		case SaveNameKindClean:
			if (uiPrvSaveExportNameExists(saveDir, result->cleanSaveName)) {
				uiPrvSaveExportSetTarget(result, result->cleanSaveName, SaveNameKindClean);
				return;
			}
			break;

		case SaveNameKindFallback:
			if (uiPrvSaveExportNameExists(saveDir, result->fallbackSaveName)) {
				uiPrvSaveExportSetTarget(result, result->fallbackSaveName, SaveNameKindFallback);
				return;
			}
			break;

		case SaveNameKindAuto:
			if (uiPrvSaveExportNameExists(saveDir, result->cleanSaveName)) {
				uiPrvSaveExportSetTarget(result, result->cleanSaveName, SaveNameKindClean);
				return;
			}
			if (uiPrvSaveExportNameExists(saveDir, result->fallbackSaveName)) {
				uiPrvSaveExportSetTarget(result, result->fallbackSaveName, SaveNameKindFallback);
				return;
			}
			break;

		case SaveNameKindFull:
		default:
			break;
	}

	if (result->preferredSaveNameKind != SaveNameKindFull) {
		if (uiPrvSaveExportNameExists(saveDir, result->cleanSaveName)) {
			uiPrvSaveExportSetTarget(result, result->cleanSaveName, SaveNameKindClean);
			return;
		}
		if (uiPrvSaveExportNameExists(saveDir, result->fallbackSaveName)) {
			uiPrvSaveExportSetTarget(result, result->fallbackSaveName, SaveNameKindFallback);
			return;
		}
	}

	uiPrvSaveExportSetTarget(result, result->primarySaveName, SaveNameKindFull);
}

static bool uiPrvWriteExportedSavestate(struct FatfsVol *vol, struct SaveExportResult *result)
{
	struct FatfsDir *saveDir = NULL;
	struct FatfsFil *fil = NULL;
	struct FatFileLocator loc;
	union SdFlags flags;
	bool ret;

	flags.value = sdGetFlags();
	result->sdFlags = flags.value;
	result->cardReportedReadOnly = flags.RO;
	if (flags.RO)
		pr("Savegame export: card reports write protect; attempting verified write anyway\n");

	saveDir = uiPrvOpenSaveDirForConsole(vol, result->saveConsole, true);
	if (!saveDir) {
		pr("Savegame export: cannot create/open %s\n", result->saveDirPath);
		uiPrvSaveExportSetStatus(result, SaveExportStatusSaveDirFailed);
		return false;
	}

	uiPrvChooseSaveExportTarget(saveDir, result);
	pr("Savegame export: preparing %s/%s (%u bytes, preferred=%u chosen=%u)\n",
		result->saveDirPath, result->saveName, result->bytesExpected,
		(unsigned)result->preferredSaveNameKind, (unsigned)result->chosenSaveNameKind);
	pr("Savegame export: primary save name is %s/%s\n", result->saveDirPath, result->primarySaveName);
	if (result->cleanSaveName[0] && strcmp(result->primarySaveName, result->cleanSaveName))
		pr("Savegame export: clean save name is %s/%s\n", result->saveDirPath, result->cleanSaveName);
	if (result->fallbackSaveName[0])
		pr("Savegame export: fallback save name is %s/%s\n", result->saveDirPath, result->fallbackSaveName);

	if (fatfsFindFileAt(saveDir, result->saveName, &loc)) {
		result->openPath = "chosen existing";
		fil = fatfsFileOpenWithLocator(vol, &loc, OPEN_MODE_WRITE);
		if (!fil) {
			pr("Savegame export: cannot open existing %s/%s for writing\n", result->saveDirPath, result->saveName);
			uiPrvSaveExportSetStatus(result, SaveExportStatusOpenExistingFailed);
			goto out_close_dir;
		}
	}
	else {
		pr("Savegame export: %s/%s missing, creating it\n", result->saveDirPath, result->saveName);
		fatfsClearLastCreateError();
		if (fatfsFileCreateAt(saveDir, result->saveName, &loc)) {
			result->openPath = "chosen created";
			fil = fatfsFileOpenWithLocator(vol, &loc, OPEN_MODE_WRITE);
			if (!fil) {
				pr("Savegame export: created %s/%s but cannot reopen for writing\n", result->saveDirPath, result->saveName);
				uiPrvSaveExportSetStatus(result, SaveExportStatusOpenCreatedFailed);
				goto out_close_dir;
			}
		}
		else {
			if (uiPrvSaveExportNameMatches(result->saveName, result->fallbackSaveName))
				uiPrvSaveExportRecordFallbackCreateError(result);
			else
				uiPrvSaveExportRecordPrimaryCreateError(result);
			pr("Savegame export: cannot create %s/%s (FAT stage: %s)\n", result->saveDirPath, result->saveName, result->fatCreateErrorName);
			if (!result->fallbackSaveName[0] || uiPrvSaveExportNameMatches(result->saveName, result->fallbackSaveName)) {
				uiPrvSaveExportSetStatus(result, SaveExportStatusCreateFailed);
				goto out_close_dir;
			}

			uiPrvCopyStr(result->saveName, sizeof(result->saveName), result->fallbackSaveName);
			result->chosenSaveNameKind = SaveNameKindFallback;
			result->fallbackUsed = true;
			pr("Savegame export: trying fallback %s/%s\n", result->saveDirPath, result->fallbackSaveName);

			if (fatfsFindFileAt(saveDir, result->fallbackSaveName, &loc)) {
				result->openPath = "fallback existing";
				fil = fatfsFileOpenWithLocator(vol, &loc, OPEN_MODE_WRITE);
				if (!fil) {
					pr("Savegame export: cannot open existing fallback %s/%s for writing\n", result->saveDirPath, result->fallbackSaveName);
					uiPrvSaveExportSetStatus(result, SaveExportStatusOpenExistingFailed);
					goto out_close_dir;
				}
			}
			else {
				pr("Savegame export: %s/%s missing, creating fallback\n", result->saveDirPath, result->fallbackSaveName);
				fatfsClearLastCreateError();
				if (!fatfsFileCreateAt(saveDir, result->fallbackSaveName, &loc)) {
					uiPrvSaveExportRecordFallbackCreateError(result);
					pr("Savegame export: cannot create fallback %s/%s (FAT stage: %s)\n",
						result->saveDirPath, result->fallbackSaveName, result->fallbackCreateErrorName);
					uiPrvSaveExportSetStatus(result, SaveExportStatusCreateFailed);
					goto out_close_dir;
				}
				result->openPath = "fallback created";
				fil = fatfsFileOpenWithLocator(vol, &loc, OPEN_MODE_WRITE);
				if (!fil) {
					pr("Savegame export: created fallback %s/%s but cannot reopen for writing\n", result->saveDirPath, result->fallbackSaveName);
					uiPrvSaveExportSetStatus(result, SaveExportStatusOpenCreatedFailed);
					goto out_close_dir;
				}
			}
		}
	}

	pr("Savegame export: writing %s/%s (hash=%08lx)\n", result->saveDirPath, result->saveName,
		(unsigned long)uiPrvSaveFingerprint((const void*)QSPI_RAM_COPY_START, result->bytesExpected));
	ret = fatfsFileWrite(fil, (const void*)QSPI_RAM_COPY_START, result->bytesExpected, &result->bytesDone);
	if (!ret || result->bytesDone != result->bytesExpected) {
		pr("Savegame export: write failed for %s/%s (%u/%u)\n", result->saveDirPath, result->saveName, result->bytesDone, result->bytesExpected);
		uiPrvSaveExportSetStatus(result, SaveExportStatusWriteFailed);
	}
	else if (!fatfsFileTruncate(fil, result->bytesExpected)) {
		pr("Savegame export: truncate failed for %s/%s to %u bytes\n", result->saveDirPath, result->saveName, result->bytesExpected);
		uiPrvSaveExportSetStatus(result, SaveExportStatusTruncateFailed);
	}

	if (!fatfsFileClose(fil)) {
		pr("Savegame export: close failed for %s/%s\n", result->saveDirPath, result->saveName);
		uiPrvSaveExportSetStatus(result, SaveExportStatusFileCloseFailed);
	}

out_close_dir:
	if (!fatfsDirClose(saveDir)) {
		pr("Savegame export: close failed for %s\n", result->saveDirPath);
		uiPrvSaveExportSetStatus(result, SaveExportStatusDirCloseFailed);
	}

	return result->status == SaveExportStatusOk;
}

static bool uiPrvVerifyExportedSavestate(struct FatfsVol *vol, struct SaveExportResult *result)
{
	struct FatfsDir *saveDir;
	struct FatfsFil *fil;
	const uint8_t *expected = (const uint8_t*)QSPI_RAM_COPY_START;
	uint8_t buf[512];
	uint32_t pos = 0;

	saveDir = uiPrvOpenSaveDirForConsole(vol, result->saveConsole, false);
	if (!saveDir) {
		pr("Savegame export: verify cannot open %s\n", result->saveDirPath);
		uiPrvSaveExportSetStatus(result, SaveExportStatusVerifySaveDirFailed);
		return false;
	}

	fil = fatfsFileOpenAt(saveDir, result->saveName, OPEN_MODE_READ);
	if (!fil) {
		pr("Savegame export: verify cannot reopen %s/%s\n", result->saveDirPath, result->saveName);
		uiPrvSaveExportSetStatus(result, SaveExportStatusVerifyOpenFailed);
		goto out_close_dir;
	}

	if (fatfsFileGetSize(fil) != result->bytesExpected) {
		pr("Savegame export: verify size mismatch for %s/%s (%u/%u)\n",
			result->saveDirPath, result->saveName, fatfsFileGetSize(fil), result->bytesExpected);
		result->bytesDone = fatfsFileGetSize(fil);
		uiPrvSaveExportSetStatus(result, SaveExportStatusVerifySizeMismatch);
		goto out_close_file;
	}

	while (pos < result->bytesExpected) {
		uint32_t numRead = 0, now = result->bytesExpected - pos;

		if (now > sizeof(buf))
			now = sizeof(buf);
		if (!fatfsFileRead(fil, buf, now, &numRead) || numRead != now) {
			pr("Savegame export: verify read failed for %s/%s at %u (%u/%u)\n",
				result->saveDirPath, result->saveName, pos, numRead, now);
			result->bytesDone = pos + numRead;
			result->verifyOffset = pos;
			uiPrvSaveExportSetStatus(result, SaveExportStatusVerifyReadFailed);
			goto out_close_file;
		}
		if (memcmp(buf, expected + pos, now)) {
			uint32_t i;

			for (i = 0; i < now; i++) {
				if (buf[i] != expected[pos + i]) {
					result->verifyOffset = pos + i;
					result->verifyActual = buf[i];
					result->verifyExpected = expected[pos + i];
					pr("Savegame export: verify mismatch for %s/%s at %u (sd=%02x flash=%02x)\n",
						result->saveDirPath, result->saveName, result->verifyOffset, result->verifyActual, result->verifyExpected);
					break;
				}
			}
			uiPrvSaveExportSetStatus(result, SaveExportStatusVerifyMismatch);
			goto out_close_file;
		}
		pos += now;
	}

out_close_file:
	if (!fatfsFileClose(fil)) {
		pr("Savegame export: verify close failed for %s/%s\n", result->saveDirPath, result->saveName);
		uiPrvSaveExportSetStatus(result, SaveExportStatusVerifyFileCloseFailed);
	}

out_close_dir:
	if (!fatfsDirClose(saveDir)) {
		pr("Savegame export: verify close failed for %s\n", result->saveDirPath);
		uiPrvSaveExportSetStatus(result, SaveExportStatusVerifyDirCloseFailed);
	}

	if (result->status == SaveExportStatusOk)
		pr("Savegame export: verified %s/%s (%u bytes, hash=%08lx)\n",
			result->saveDirPath, result->saveName, result->bytesExpected,
			(unsigned long)uiPrvSaveFingerprint((const void*)QSPI_RAM_COPY_START, result->bytesExpected));
	return result->status == SaveExportStatusOk;
}

static bool uiPrvFlushCurrentSaveToCardAttempt(bool forceReinit, struct SaveExportResult *result)
{
	struct FatfsVol *vol;

	uiPrvSaveExportBeginAttempt(result);
	result->attemptNo++;
	result->forceReinitUsed = forceReinit;
	sdClearLastError();
	vol = uiPrvMountCardEx(NULL, true, forceReinit);
	if (!vol) {
		pr("Savegame export: cannot mount SD card\n");
		result->sdFlags = sdGetFlags();
		uiPrvSaveExportSetStatus(result, SaveExportStatusMountFailed);
		return false;
	}

	(void)uiPrvWriteExportedSavestate(vol, result);
	if (!uiPrvCardPreUnmount()) {
		pr("Savegame export: SD stream close failed\n");
		uiPrvSaveExportSetStatus(result, SaveExportStatusStreamCloseFailed);
	}
	if (!fatfsUnmount(vol)) {
		pr("Savegame export: FAT unmount failed\n");
		uiPrvSaveExportSetStatus(result, SaveExportStatusUnmountFailed);
	}
	if (result->status != SaveExportStatusOk)
		return false;

	sdClearLastError();
	vol = uiPrvMountCardEx(NULL, true, false);
	if (!vol) {
		pr("Savegame export: cannot remount SD card for verification\n");
		uiPrvSaveExportSetStatus(result, SaveExportStatusVerifyMountFailed);
		return false;
	}

	(void)uiPrvVerifyExportedSavestate(vol, result);
	if (!uiPrvCardPreUnmount()) {
		pr("Savegame export: verify SD stream close failed\n");
		uiPrvSaveExportSetStatus(result, SaveExportStatusVerifyStreamCloseFailed);
	}
	if (!fatfsUnmount(vol)) {
		pr("Savegame export: verify FAT unmount failed\n");
		uiPrvSaveExportSetStatus(result, SaveExportStatusVerifyUnmountFailed);
	}

	return result->status == SaveExportStatusOk;
}

static bool uiPrvPrepareSaveExportResult(bool force, struct SaveExportResult *result)
{
	struct GameSelection selection;
	char detectedName[ROM_NAME_LEN + 1];
	enum RomColorSupport colorSupport = RomNoColor;

	uiPrvSaveExportResultInit(result);
	if (!uiGetGameSelection(&selection) || !selection.saveRamSize)
	{
		result->nothingToExport = true;
		return false;
	}

	result->forceRequested = force;
	result->bytesExpected = selection.saveRamSize;
	result->preferredSaveNameKind = uiPrvSelectedSaveNameKind();
	uiPrvDetectedSaveTitleForSelection(&selection, detectedName, sizeof(detectedName));
	if (selection.runtime == GameRuntimeGb) {
		char name[ROM_NAME_LEN + 1];
		uint32_t romSize, saveRamSize;

		name[ROM_NAME_LEN] = 0;
		(void)mbcRomAnalyze((const void*)QSPI_ROM_START, &romSize, &saveRamSize, &colorSupport, name);
	}
	result->saveConsole = uiPrvSaveConsoleForGame(selection.runtime, colorSupport, (const char*)QSPI_FILENAME_START);
	uiPrvSaveDirPathForConsole(result->saveConsole, result->saveDirPath, sizeof(result->saveDirPath));
	uiPrvSaveFileName((const char*)QSPI_FILENAME_START, selection.runtime, detectedName, result->primarySaveName, sizeof(result->primarySaveName));
	uiPrvCleanSaveFileName((const char*)QSPI_FILENAME_START, selection.runtime, detectedName, result->cleanSaveName, sizeof(result->cleanSaveName));
	uiPrvSaveFallbackFileName((const char*)QSPI_FILENAME_START, selection.runtime, result->fallbackSaveName, sizeof(result->fallbackSaveName));
	uiPrvCopyStr(result->saveName, sizeof(result->saveName), result->primarySaveName);
	if (!uiSaveSavestate()) {
		result->flashCacheFailed = true;
		pr("Savegame export: failed to copy current save RAM to flash; exporting cached flash copy anyway\n");
	}
	pr("Savegame export: cache ready for %s (%lu bytes, qspi=%08lx cart=%08lx owner=%u preferred=%s title=%s)\n",
		(const char*)QSPI_FILENAME_START, (unsigned long)result->bytesExpected,
		(unsigned long)uiPrvSaveFingerprint((const void*)QSPI_RAM_COPY_START, result->bytesExpected),
		(unsigned long)uiPrvSaveFingerprint(CART_RAM_ADDR_IN_RAM, result->bytesExpected),
		uiPrvCartRamOwnerMatchesSelection(&selection) ? 1u : 0u,
		uiPrvSaveNameKindName(result->preferredSaveNameKind), detectedName[0] ? detectedName : "(filename)");

	pr("Savegame export: flushing %s/%s to SD (force=%u)\n", result->saveDirPath, result->saveName, force ? 1u : 0u);
	return true;
}

static bool uiPrvFlushCurrentSaveToCardEx(bool force, struct SaveExportResult *result)
{
	if (!uiPrvPrepareSaveExportResult(force, result))
		return true;

	if (uiPrvFlushCurrentSaveToCardAttempt(force, result))
		return true;

	if (!force) {
		pr("Savegame export: retrying with forced SD reinit\n");
		result->retryAttempted = true;
		if (uiPrvFlushCurrentSaveToCardAttempt(true, result))
			return true;
	}

	return false;
}

static bool uiPrvFlushCurrentSaveToMountedCardEx(struct FatfsVol *vol, bool force, struct SaveExportResult *result)
{
	if (!uiPrvPrepareSaveExportResult(force, result))
		return true;

	pr("Savegame export: using already-mounted SD volume\n");
	sdClearLastError();
	(void)uiPrvWriteExportedSavestate(vol, result);
	if (result->status == SaveExportStatusOk)
		(void)uiPrvVerifyExportedSavestate(vol, result);
	return result->status == SaveExportStatusOk;
}

bool uiFlushCurrentSaveToCard(bool force)
{
	struct SaveExportResult result;

	return uiPrvFlushCurrentSaveToCardEx(force, &result);
}
	
	static bool uiPrvSelectRomForConsole(struct Canvas *cnv, bool forceSdReinit, enum UiEmulatorConsole console)	//corrupts GB's RAM, returns true if a selection was made
	{
		struct FatFileLocator locator;
		struct FatfsVol *vol;
		char name[UI_PICK_FILE_NAME_BUF_SZ];
		char emptyMsg[128];
		const char *rootPath = uiPrvRomRootForConsole(console);
		bool ret = false;
		
		// ROM selection reuses cartridge RAM, so export the current save through the normal SD pipeline first.
		if (!uiPrvPrepareForRomReplacement(cnv, NULL, "ROM picker"))
			return false;

		vol = uiPrvMountCardEx(cnv, false, forceSdReinit);
		if (!vol)
			return false;

		(void)sprintf(emptyMsg, "No %s games found in %s", uiPrvEmulatorConsoleName(console), rootPath);
		if (uiPrvPickFile(cnv, vol, rootPath, uiPrvRomFilterForConsole(console), emptyMsg, false, &locator, name, sizeof(name), NULL, 0, uiPrvRomDisplayName, false, NULL))
			ret = uiPrvConfirmRomSelection(cnv, vol, &locator, name);
	
		(void)uiPrvCardPreUnmount();
		fatfsUnmount(vol);
		return ret;
	}

	static bool uiPrvSelectRom(struct Canvas *cnv, bool forceSdReinit)
	{
		return uiPrvSelectRomForConsole(cnv, forceSdReinit, uiPrvCurrentGameConsole());
	}

static const char* uiPrvSaveExportStatusText(enum SaveExportStatus status)
{
	switch (status) {
		case SaveExportStatusMountFailed:
			return "Could not mount the SD card.";
		case SaveExportStatusSaveDirFailed:
			return "Could not create or open the save folder.";
		case SaveExportStatusCreateFailed:
			return "Could not create the missing save file.";
		case SaveExportStatusOpenExistingFailed:
			return "Could not open the existing save file for writing.";
		case SaveExportStatusOpenCreatedFailed:
			return "Created the save file but could not reopen it for writing.";
		case SaveExportStatusWriteFailed:
			return "The FAT write did not complete.";
		case SaveExportStatusTruncateFailed:
			return "Could not truncate the save file to the exact size.";
		case SaveExportStatusFileCloseFailed:
			return "The save file did not close cleanly.";
		case SaveExportStatusDirCloseFailed:
			return "The save folder did not close cleanly.";
		case SaveExportStatusStreamCloseFailed:
			return "The SD write stream did not close cleanly.";
		case SaveExportStatusUnmountFailed:
			return "The FAT volume did not unmount cleanly after writing.";
		case SaveExportStatusVerifyMountFailed:
			return "Could not remount the SD card for verification.";
		case SaveExportStatusVerifySaveDirFailed:
			return "Could not reopen the save folder for verification.";
		case SaveExportStatusVerifyOpenFailed:
			return "Could not reopen the save file for verification.";
		case SaveExportStatusVerifySizeMismatch:
			return "Verification found the wrong save file size.";
		case SaveExportStatusVerifyReadFailed:
			return "Verification could not read the save file back.";
		case SaveExportStatusVerifyMismatch:
			return "Verification found bytes that differ from flash.";
		case SaveExportStatusVerifyFileCloseFailed:
			return "The verified save file did not close cleanly.";
		case SaveExportStatusVerifyDirCloseFailed:
			return "The verified save folder did not close cleanly.";
		case SaveExportStatusVerifyStreamCloseFailed:
			return "The SD read stream did not close cleanly after verification.";
		case SaveExportStatusVerifyUnmountFailed:
			return "The FAT volume did not unmount cleanly after verification.";
		case SaveExportStatusOk:
		default:
			return "No SD save error was reported.";
	}
}

static const char* uiPrvSaveExportStatusName(enum SaveExportStatus status)
{
	switch (status) {
		case SaveExportStatusMountFailed:
			return "MountFailed";
		case SaveExportStatusSaveDirFailed:
			return "SaveDirFailed";
		case SaveExportStatusCreateFailed:
			return "CreateFailed";
		case SaveExportStatusOpenExistingFailed:
			return "OpenExistingFailed";
		case SaveExportStatusOpenCreatedFailed:
			return "OpenCreatedFailed";
		case SaveExportStatusWriteFailed:
			return "WriteFailed";
		case SaveExportStatusTruncateFailed:
			return "TruncateFailed";
		case SaveExportStatusFileCloseFailed:
			return "FileCloseFailed";
		case SaveExportStatusDirCloseFailed:
			return "DirCloseFailed";
		case SaveExportStatusStreamCloseFailed:
			return "StreamCloseFailed";
		case SaveExportStatusUnmountFailed:
			return "UnmountFailed";
		case SaveExportStatusVerifyMountFailed:
			return "VerifyMountFailed";
		case SaveExportStatusVerifySaveDirFailed:
			return "VerifySaveDirFailed";
		case SaveExportStatusVerifyOpenFailed:
			return "VerifyOpenFailed";
		case SaveExportStatusVerifySizeMismatch:
			return "VerifySizeMismatch";
		case SaveExportStatusVerifyReadFailed:
			return "VerifyReadFailed";
		case SaveExportStatusVerifyMismatch:
			return "VerifyMismatch";
		case SaveExportStatusVerifyFileCloseFailed:
			return "VerifyFileCloseFailed";
		case SaveExportStatusVerifyDirCloseFailed:
			return "VerifyDirCloseFailed";
		case SaveExportStatusVerifyStreamCloseFailed:
			return "VerifyStreamCloseFailed";
		case SaveExportStatusVerifyUnmountFailed:
			return "VerifyUnmountFailed";
		case SaveExportStatusOk:
		default:
			return "Ok";
	}
}

static void uiPrvSaveExportAppend(char msg[static SAVE_EXPORT_MSG_BUF_SZ], const char *str)
{
	uint32_t pos = strlen(msg);

	if (pos >= SAVE_EXPORT_MSG_BUF_SZ - 1)
		return;
	uiPrvCopyStr(msg + pos, SAVE_EXPORT_MSG_BUF_SZ - pos, str);
}

static void uiPrvSaveExportPath(const struct SaveExportResult *result, const char *name, char *dst, uint32_t dstLen)
{
	if (!dstLen)
		return;
	if (name && name[0]) {
		(void)sprintf(dst, "%s/%s", result->saveDirPath[0] ? result->saveDirPath : "/SAVE", name);
		return;
	}
	uiPrvCopyStr(dst, dstLen, result->saveDirPath[0] ? result->saveDirPath : "/SAVE");
}

static void uiPrvSaveExportAppendPath(char msg[static SAVE_EXPORT_MSG_BUF_SZ], const char *label,
	const struct SaveExportResult *result, const char *name)
{
	uiPrvSaveExportAppend(msg, "\n");
	uiPrvSaveExportAppend(msg, label);
	if (name && name[0]) {
		char path[96];

		uiPrvSaveExportPath(result, name, path, sizeof(path));
		uiPrvSaveExportAppend(msg, path);
	}
	else
		uiPrvSaveExportAppend(msg, "(none)");
}

static void uiPrvSaveExportAppendDiagLine(char msg[static SAVE_EXPORT_MSG_BUF_SZ], const char *label, const char *value)
{
	uiPrvSaveExportAppend(msg, "\n");
	uiPrvSaveExportAppend(msg, label);
	uiPrvSaveExportAppend(msg, value && value[0] ? value : "none");
}

static void uiPrvSaveExportFormatFailure(const struct SaveExportResult *result, char msg[static SAVE_EXPORT_MSG_BUF_SZ])
{
	char tmp[128];

	(void)sprintf(msg, "SD save failed; flash cache preserved.\n%s", uiPrvSaveExportStatusText(result->status));
	(void)sprintf(tmp, "\nStatus:%u %s", (unsigned)result->status, uiPrvSaveExportStatusName(result->status));
	uiPrvSaveExportAppend(msg, tmp);
	(void)sprintf(tmp, "\nAttempt:%u force_req=%u reinit=%u retry=%u manual=%u",
		(unsigned)result->attemptNo, result->forceRequested ? 1u : 0u, result->forceReinitUsed ? 1u : 0u,
		result->retryAttempted ? 1u : 0u, result->manualRequest ? 1u : 0u);
	uiPrvSaveExportAppend(msg, tmp);
	(void)sprintf(tmp, "\nBytes:%lu/%lu", (unsigned long)result->bytesDone, (unsigned long)result->bytesExpected);
	uiPrvSaveExportAppend(msg, tmp);
	(void)sprintf(tmp, "\nFlags:0x%02x RO=%u flash=%s fallback=%u",
		result->sdFlags, result->cardReportedReadOnly ? 1u : 0u, result->flashCacheFailed ? "cached" : "fresh",
		result->fallbackUsed ? 1u : 0u);
	uiPrvSaveExportAppend(msg, tmp);
	(void)sprintf(tmp, "\nSave kind:%u->%u", (unsigned)result->preferredSaveNameKind, (unsigned)result->chosenSaveNameKind);
	uiPrvSaveExportAppend(msg, tmp);
	uiPrvSaveExportAppendDiagLine(msg, "Open:", result->openPath);
	uiPrvSaveExportAppendDiagLine(msg, "FAT primary:", result->primaryCreateErrorName);
	uiPrvSaveExportAppendDiagLine(msg, "FAT fallback:", result->fallbackCreateErrorName);
	uiPrvSaveExportAppendDiagLine(msg, "FAT last:", result->fatCreateErrorName);

	if (result->status == SaveExportStatusVerifyReadFailed)
		(void)sprintf(tmp, "\nRead through:%lu/%lu", (unsigned long)result->bytesDone, (unsigned long)result->bytesExpected);
	else if (result->status == SaveExportStatusVerifyMismatch)
		(void)sprintf(tmp, "\nMismatch:%lu sd=%02x flash=%02x", (unsigned long)result->verifyOffset, result->verifyActual, result->verifyExpected);
	else if (result->status == SaveExportStatusVerifySizeMismatch)
		(void)sprintf(tmp, "\nVerify size:%lu/%lu", (unsigned long)result->bytesDone, (unsigned long)result->bytesExpected);
	else
		tmp[0] = 0;
	if (tmp[0])
		uiPrvSaveExportAppend(msg, tmp);

	(void)sprintf(tmp, "\nSD:%u %s LBA=%lu data=0x%08lx", result->sdError.type,
		result->sdError.name ? result->sdError.name : "unknown",
		(unsigned long)result->sdError.sector, (unsigned long)result->sdError.data);
	uiPrvSaveExportAppend(msg, tmp);
	uiPrvSaveExportAppendPath(msg, "Chosen:", result, result->saveName);
	uiPrvSaveExportAppendPath(msg, "Primary:", result, result->primarySaveName);
	if (result->cleanSaveName[0] && strcmp(result->primarySaveName, result->cleanSaveName))
		uiPrvSaveExportAppendPath(msg, "Clean:", result, result->cleanSaveName);
	uiPrvSaveExportAppendPath(msg, "Fallback:", result, result->fallbackSaveName);
}

static void uiPrvSaveExportShowSuccess(struct Canvas *cnv, const struct SaveExportResult *result, bool showSuccess,
	char msg[static SAVE_EXPORT_MSG_BUF_SZ])
{
	char path[96];

	uiPrvSaveExportPath(result, result->saveName, path, sizeof(path));
	if (result->flashCacheFailed) {
		if (result->saveName[0])
			(void)sprintf(msg, "Save written to %s from cached flash.\nLatest emulator RAM could not be copied first.", path);
		else
			(void)sprintf(msg, "Save written to %s from cached flash.\nLatest emulator RAM could not be copied first.", path);
		uiAlert(cnv, msg, DialogTypeOk);
		return;
	}

	if (!showSuccess)
		return;

	if (result->nothingToExport) {
		if (result->manualRequest)
			uiAlert(cnv, "This game has no battery save to export.", DialogTypeOk);
		return;
	}

	if (result->saveName[0])
		(void)sprintf(msg, "Save written to %s", path);
	else
		(void)sprintf(msg, "Save written to %s", path);
	uiAlert(cnv, msg, DialogTypeOk);
}

static bool uiPrvExportCurrentSavestateWithOptions(struct Canvas *cnv, bool force, bool showSuccess)
{
	struct SaveExportResult result;
	char msg[SAVE_EXPORT_MSG_BUF_SZ];

	pr("Savegame export: %s save requested\n", showSuccess ? "confirmed" : "safe-exit");
	if (!uiPrvFlushCurrentSaveToCardEx(force, &result)) {
		result.manualRequest = showSuccess;
		uiPrvSaveExportFormatFailure(&result, msg);
		uiAlert(cnv, msg, DialogTypeOk);
		return false;
	}

	result.manualRequest = force;
	uiPrvSaveExportShowSuccess(cnv, &result, showSuccess, msg);
	return true;
}

static bool uiPrvExportCurrentSavestate(struct Canvas *cnv, bool manual)
{
	return uiPrvExportCurrentSavestateWithOptions(cnv, manual, manual);
}

static bool uiPrvExportCurrentSavestateToMountedCardWithOptions(struct Canvas *cnv, struct FatfsVol *vol, bool force,
	bool showSuccess)
{
	struct SaveExportResult result;
	char msg[SAVE_EXPORT_MSG_BUF_SZ];

	pr("Savegame export: %s save requested on mounted SD volume\n", showSuccess ? "confirmed" : "safe-exit");
	if (!uiPrvFlushCurrentSaveToMountedCardEx(vol, force, &result)) {
		result.manualRequest = showSuccess;
		uiPrvSaveExportFormatFailure(&result, msg);
		uiAlert(cnv, msg, DialogTypeOk);
		return false;
	}

	result.manualRequest = force;
	uiPrvSaveExportShowSuccess(cnv, &result, showSuccess, msg);
	return true;
}

static bool uiPrvExportCurrentSavestateToMountedCard(struct Canvas *cnv, struct FatfsVol *vol, bool manual)
{
	return uiPrvExportCurrentSavestateToMountedCardWithOptions(cnv, vol, manual, manual);
}

	static bool uiPrvHaveGamePath(void)
	{
		const uint8_t *ptr = (const uint8_t*)QSPI_FILENAME_START;
		uint32_t len = 0;
		
		if (*ptr == 0x00 || *ptr == 0xff)
			return false;
		
		while (*ptr++) {
			if (++len == GAME_META_OFFSET)
				return false;
		}
		
		return true;
	}

bool uiGetGameSelection(struct GameSelection *selectionP)
{
	const struct GameSelectionFlashMeta *meta = (const struct GameSelectionFlashMeta*)(QSPI_FILENAME_START + GAME_META_OFFSET);
	struct GameSelection selection = {0};

	if (!uiPrvHaveGamePath())
		return false;

	if (meta->magic == GAME_META_MAGIC &&
		(meta->runtime == GameRuntimeGb || meta->runtime == GameRuntimeNes || meta->runtime == GameRuntimeArduboy)) {
		selection.runtime = (enum GameRuntime)meta->runtime;
		selection.romSize = meta->romSize;
		selection.saveRamSize = meta->saveRamSize;
	}
	else {
		selection.runtime = uiPrvRuntimeForName((const char*)QSPI_FILENAME_START);
	}

	if (selection.runtime == GameRuntimeGb) {
		uint32_t romSzExpected, ramSzExpected;

		if (!mbcRomAnalyze((const void*)QSPI_ROM_START, &romSzExpected, &ramSzExpected, NULL, NULL))
			return false;
		selection.romSize = romSzExpected;
		selection.saveRamSize = ramSzExpected;
	}
	else if (selection.runtime == GameRuntimeNes) {
		struct NesRomInfo info;

		if (!nesAnalyzeRom((const void*)QSPI_ROM_START, selection.romSize ? selection.romSize : QSPI_ROM_SIZE_MAX, &info))
			return false;
		if (!selection.romSize)
			selection.romSize = info.romSize;
		if (!selection.saveRamSize)
			selection.saveRamSize = info.saveRamSize;
	}
	else if (selection.runtime == GameRuntimeArduboy) {
		struct ArduboyRomInfo info;

		if (!arduboyAnalyzeRom((const void*)QSPI_ROM_START, selection.romSize ? selection.romSize : QSPI_ROM_SIZE_MAX, &info))
			return false;
		if (info.isPackage)
			return false;
		if (!selection.romSize)
			selection.romSize = info.romSize;
		if (!selection.saveRamSize)
			selection.saveRamSize = info.saveRamSize;
	}
	else
		return false;

	if (selectionP)
		*selectionP = selection;
	return true;
}

	static char* uiPrvTrim(char *str)
	{
		char *end;

		while (*str == ' ' || *str == '\t')
			str++;

		end = str + strlen(str);
		while (end != str && (end[-1] == ' ' || end[-1] == '\t')) {
			end--;
			*end = 0;
		}

		return str;
	}

	static bool uiPrvStartsWith(const char *str, const char *prefix)
	{
		while (*prefix) {
			if (*str++ != *prefix++)
				return false;
		}

		return true;
	}

	static void uiPrvCopyStr(char *dst, uint32_t dstLen, const char *src)
	{
		if (!dstLen)
			return;

		while (--dstLen && *src)
			*dst++ = *src++;
		*dst = 0;
	}

	static bool uiPrvParseU32(const char **strP, uint32_t *valP)
	{
		const char *str = *strP;
		uint32_t val = 0;

		while (*str == ' ' || *str == '\t')
			str++;

		if (*str < '0' || *str > '9')
			return false;

		do {
			uint32_t digit = *str++ - '0';

			if (val > (0xfffffffful - digit) / 10)
				return false;
			val = val * 10 + digit;
		} while (*str >= '0' && *str <= '9');

		while (*str == ' ' || *str == '\t')
			str++;

		*strP = str;
		*valP = val;

		return true;
	}

	static bool uiPrvParseHexByte(const char **strP, uint8_t *valP)
	{
		const char *str = *strP;
		uint_fast8_t i;
		uint8_t val = 0;

		while (*str == ' ' || *str == '\t')
			str++;

		for (i = 0; i < 2; i++) {
			uint8_t nibble;

			if (*str >= '0' && *str <= '9')
				nibble = *str - '0';
			else if (*str >= 'a' && *str <= 'f')
				nibble = *str - 'a' + 10;
			else if (*str >= 'A' && *str <= 'F')
				nibble = *str - 'A' + 10;
			else
				return false;

			val = (val << 4) | nibble;
			str++;
		}

		while (*str == ' ' || *str == '\t')
			str++;

		*strP = str;
		*valP = val;

		return true;
	}

	static bool uiPrvParseFlipperU32Bytes(const char *str, uint32_t *valP)
	{
		uint32_t val = 0;
		uint_fast8_t i;

		for (i = 0; i < 4; i++) {
			uint8_t byte;

			if (!uiPrvParseHexByte(&str, &byte))
				return false;

			val |= ((uint32_t)byte) << (8 * i);
		}

		*valP = val;

		return !*str;
	}

	static bool uiPrvIrCancelRequested(void);

	static bool uiPrvReadLine(struct FatfsFil *fil, char *buf, uint32_t bufSz, uint32_t *bytesReadP, uint32_t fileSize, bool *truncatedP, bool *cancelledP)
	{
		uint32_t pos = 0;

		*truncatedP = false;
		if (cancelledP)
			*cancelledP = false;
		if (!bufSz)
			return false;

		while (1) {
			char ch;
			uint32_t numRead;

			if (bytesReadP && !(*bytesReadP & 0xff) && uiPrvIrCancelRequested()) {
				if (cancelledP)
					*cancelledP = true;
				return false;
			}
			if (!fatfsFileRead(fil, &ch, 1, &numRead))
				return false;

			if (!numRead) {
				buf[pos] = 0;
				return pos != 0 || *truncatedP;
			}
			if (bytesReadP)
				(*bytesReadP)++;

			if (ch == '\n') {
				buf[pos] = 0;
				return true;
			}

			if (ch == '\r') {
				if (bytesReadP && fileSize && *bytesReadP >= fileSize) {
					buf[pos] = 0;
					return pos != 0 || *truncatedP;
				}
				continue;
			}

			if (pos + 1 < bufSz)
				buf[pos++] = ch;
			else
				*truncatedP = true;
			if (bytesReadP && fileSize && *bytesReadP >= fileSize) {
				buf[pos] = 0;
				return pos != 0 || *truncatedP;
			}
		}
	}

	static uint64_t uiPrvIrTicksFromUsec(uint32_t usec)
	{
		uint64_t ticks = ((uint64_t)usec * TICKS_PER_SECOND + 999999) / 1000000;

		return ticks ? ticks : 1;
	}

	static void uiPrvIrDrawProgress(struct Canvas *cnv, const char *title, const char *name, const char *detail, uint32_t codeIdx, uint32_t recordIdx, uint32_t lineNo, uint32_t repeatIdx, uint32_t numRepeats)
	{
		uint32_t row, glyphH;

		uiPrvSetHeaderTitle(title);
		uiPrvReset(cnv, false);
		cnv->font = FontMedium;
		row = uiPrvContentTop(cnv);
		glyphH = uiPrvGlyphHeight(cnv);
		cnv->foreColor = 11;
		if (recordIdx)
			uiPrintf(cnv, row, 10, "Code %u Rec %u", codeIdx, recordIdx);
		else
			uiPrintf(cnv, row, 10, "Code %u", codeIdx);
		row += glyphH + 1;
		cnv->foreColor = 15;
		uiPrvDrawWrappedString(cnv, name, row, 10);
		row = cnv->h - 3 * (glyphH + 1);
		if (lineNo) {
			if (detail && *detail)
				uiPrintf(cnv, row, 10, "Line %u %s", lineNo, detail);
			else
				uiPrintf(cnv, row, 10, "Line %u", lineNo);
		}
		uiPrintf(cnv, cnv->h - 2 * (glyphH + 1), 10, "Repeat %u/%u", repeatIdx, numRepeats);
		uiPuts(cnv, cnv->h - glyphH - 1, 10, "Hold B to cancel", -1);
	}

	static bool uiPrvIrCancelRequested(void)
	{
		if (uiPrvCenterExitPressedRaw())
			return true;

		return !!(uiGetKeysRaw() & KEY_BIT_B);
	}

	static void uiPrvIrSendCtxInit(struct IrSendCtx *ctx, struct IrBlastStats *stats, uint32_t codeIdx, uint32_t recordIndex, uint32_t lineNo, const char *name, const char *detail, uint32_t timeoutUsec)
	{
		memset(ctx, 0, sizeof(*ctx));
		ctx->stats = stats;
		ctx->deadline = getTime() + uiPrvIrTicksFromUsec(timeoutUsec);
		ctx->codeIdx = codeIdx;
		ctx->recordIndex = recordIndex;
		ctx->lineNo = lineNo;
		ctx->name = name ? name : "IR code";
		ctx->detail = detail ? detail : "";
	}

	static void uiPrvIrSendCtxStalled(struct IrSendCtx *ctx)
	{
		struct IrBlastStats *stats = ctx->stats;

		if (!stats || stats->timedOut)
			return;

		stats->timedOut = true;
		stats->stalledCode = ctx->codeIdx;
		stats->stalledRecord = ctx->recordIndex;
		stats->stalledLine = ctx->lineNo ? ctx->lineNo : stats->lineNo;
		uiPrvCopyStr(stats->stalledName, sizeof(stats->stalledName), ctx->name);
		uiPrvCopyStr(stats->stalledDetail, sizeof(stats->stalledDetail), ctx->detail);
	}

	static bool uiPrvIrSendCtxKeepGoing(struct IrSendCtx *ctx)
	{
		if (uiPrvIrCancelRequested()) {
			if (ctx->stats)
				ctx->stats->cancelled = true;
			return false;
		}
		if (getTime() >= ctx->deadline) {
			uiPrvIrSendCtxStalled(ctx);
			return false;
		}
		return true;
	}

	static bool uiPrvIrSpaceCancellable(uint32_t usec, struct IrSendCtx *ctx)
	{
		while (usec) {
			uint32_t now = usec > IR_RAW_CANCEL_CHUNK_USEC ? IR_RAW_CANCEL_CHUNK_USEC : usec;

			if (!uiPrvIrSendCtxKeepGoing(ctx))
				return false;
			irRemoteSpaceUsec(now);
			usec -= now;
		}
		return uiPrvIrSendCtxKeepGoing(ctx);
	}

	static bool uiPrvIrMarkCancellable(uint32_t carrier, uint32_t usec, struct IrSendCtx *ctx)
	{
		while (usec) {
			uint32_t now = usec > IR_RAW_CANCEL_CHUNK_USEC ? IR_RAW_CANCEL_CHUNK_USEC : usec;

			if (!uiPrvIrSendCtxKeepGoing(ctx))
				return false;
			irRemoteMarkUsec(carrier, now);
			usec -= now;
		}
		return uiPrvIrSendCtxKeepGoing(ctx);
	}

	static bool uiPrvIrSendBitPulseDistance(uint32_t carrier, bool bit, uint32_t mark, uint32_t zeroSpace, uint32_t oneSpace, struct IrSendCtx *ctx)
	{
		return uiPrvIrMarkCancellable(carrier, mark, ctx) && uiPrvIrSpaceCancellable(bit ? oneSpace : zeroSpace, ctx);
	}

	static bool uiPrvIrSendBitsPulseDistance(uint32_t carrier, uint32_t data, uint_fast8_t nBits, bool msbFirst, uint32_t mark, uint32_t zeroSpace, uint32_t oneSpace, struct IrSendCtx *ctx)
	{
		uint_fast8_t i;

		for (i = 0; i < nBits; i++) {
			bool bit;

			if (msbFirst)
				bit = !!(data & (1ul << (nBits - 1 - i)));
			else
				bit = !!(data & (1ul << i));

			if (!uiPrvIrSendBitPulseDistance(carrier, bit, mark, zeroSpace, oneSpace, ctx))
				return false;
		}
		return true;
	}

	static bool uiPrvIrSendBitMarkEncoded(uint32_t carrier, bool bit, uint32_t bitMark, uint32_t zeroMark, uint32_t oneMark, uint32_t space, struct IrSendCtx *ctx)
	{
		(void)bitMark;
		return uiPrvIrMarkCancellable(carrier, bit ? oneMark : zeroMark, ctx) && uiPrvIrSpaceCancellable(space, ctx);
	}

	static bool uiPrvIrSendBitsMarkEncoded(uint32_t carrier, uint32_t data, uint_fast8_t nBits, uint32_t bitMark, uint32_t zeroMark, uint32_t oneMark, uint32_t space, struct IrSendCtx *ctx)
	{
		uint_fast8_t i;

		for (i = 0; i < nBits; i++)
			if (!uiPrvIrSendBitMarkEncoded(carrier, !!(data & (1ul << i)), bitMark, zeroMark, oneMark, space, ctx))
				return false;
		return true;
	}

	static bool uiPrvIrManchesterHalf(uint32_t carrier, bool mark, uint32_t usec, struct IrSendCtx *ctx)
	{
		if (mark)
			return uiPrvIrMarkCancellable(carrier, usec, ctx);
		return uiPrvIrSpaceCancellable(usec, ctx);
	}

	static bool uiPrvIrSendManchesterBit(uint32_t carrier, bool bit, uint32_t halfUsec, struct IrSendCtx *ctx)
	{
		return uiPrvIrManchesterHalf(carrier, !bit, halfUsec, ctx) && uiPrvIrManchesterHalf(carrier, bit, halfUsec, ctx);
	}

	static bool uiPrvIrSendNecLike(uint32_t carrier, uint32_t address, uint32_t command, uint_fast8_t addrBits, uint_fast8_t cmdBits, bool useComplements, struct IrSendCtx *ctx)
	{
		if (!uiPrvIrMarkCancellable(carrier, 9000, ctx) || !uiPrvIrSpaceCancellable(4500, ctx))
			return false;
		if (!uiPrvIrSendBitsPulseDistance(carrier, address, addrBits, false, 560, 560, 1690, ctx))
			return false;
		if (useComplements)
			if (!uiPrvIrSendBitsPulseDistance(carrier, ~address, addrBits, false, 560, 560, 1690, ctx))
				return false;
		if (!uiPrvIrSendBitsPulseDistance(carrier, command, cmdBits, false, 560, 560, 1690, ctx))
			return false;
		if (useComplements)
			if (!uiPrvIrSendBitsPulseDistance(carrier, ~command, cmdBits, false, 560, 560, 1690, ctx))
				return false;
		return uiPrvIrMarkCancellable(carrier, 560, ctx);
	}

	static bool uiPrvIrSendParsed(const char *protocol, uint32_t address, uint32_t command, struct IrSendCtx *ctx)
	{
		uint32_t carrier = IR_DEFAULT_CARRIER;

		if (!strcmp(protocol, "NEC")) {
			if (!uiPrvIrSendNecLike(carrier, address, command, 8, 8, true, ctx))
				return false;
		}
		else if (!strcmp(protocol, "NECext")) {
			if (!uiPrvIrSendNecLike(carrier, address, command, 16, 16, false, ctx))
				return false;
		}
		else if (!strcmp(protocol, "NEC42")) {
			if (!uiPrvIrSendNecLike(carrier, address, command, 13, 8, true, ctx))
				return false;
		}
		else if (!strcmp(protocol, "NEC42ext")) {
			if (!uiPrvIrSendNecLike(carrier, address, command, 26, 16, false, ctx))
				return false;
		}
		else if (!strcmp(protocol, "Samsung32")) {
			if (!uiPrvIrMarkCancellable(carrier, 4500, ctx) || !uiPrvIrSpaceCancellable(4500, ctx) ||
				!uiPrvIrSendBitsPulseDistance(carrier, address, 16, false, 560, 560, 1690, ctx) ||
				!uiPrvIrSendBitsPulseDistance(carrier, command, 8, false, 560, 560, 1690, ctx) ||
				!uiPrvIrSendBitsPulseDistance(carrier, ~command, 8, false, 560, 560, 1690, ctx) ||
				!uiPrvIrMarkCancellable(carrier, 560, ctx))
				return false;
		}
		else if (!strcmp(protocol, "SIRC") || !strcmp(protocol, "SIRC15") || !strcmp(protocol, "SIRC20")) {
			uint_fast8_t addrBits = !strcmp(protocol, "SIRC") ? 5 : (!strcmp(protocol, "SIRC15") ? 8 : 13), rep;

			carrier = 40000;
			for (rep = 0; rep < 3; rep++) {
				if (!uiPrvIrMarkCancellable(carrier, 2400, ctx) || !uiPrvIrSpaceCancellable(600, ctx) ||
					!uiPrvIrSendBitsMarkEncoded(carrier, command, 7, 600, 600, 1200, 600, ctx) ||
					!uiPrvIrSendBitsMarkEncoded(carrier, address, addrBits, 600, 600, 1200, 600, ctx) ||
					!uiPrvIrSpaceCancellable(25000, ctx))
					return false;
			}
		}
		else if (!strcmp(protocol, "RCA")) {
			if (!uiPrvIrMarkCancellable(carrier, 4000, ctx) || !uiPrvIrSpaceCancellable(4000, ctx) ||
				!uiPrvIrSendBitsPulseDistance(carrier, address, 4, false, 500, 1000, 2000, ctx) ||
				!uiPrvIrSendBitsPulseDistance(carrier, command, 8, false, 500, 1000, 2000, ctx) ||
				!uiPrvIrSendBitsPulseDistance(carrier, ~address, 4, false, 500, 1000, 2000, ctx) ||
				!uiPrvIrSendBitsPulseDistance(carrier, ~command, 8, false, 500, 1000, 2000, ctx) ||
				!uiPrvIrMarkCancellable(carrier, 500, ctx))
				return false;
		}
		else if (!strcmp(protocol, "RC5") || !strcmp(protocol, "RC5X")) {
			uint32_t cmd = command & (!strcmp(protocol, "RC5X") ? 0x7f : 0x3f);
			uint_fast8_t i;

			carrier = 36000;
			if (!uiPrvIrSendManchesterBit(carrier, true, 889, ctx) ||
				!uiPrvIrSendManchesterBit(carrier, !(cmd & 0x40), 889, ctx) ||
				!uiPrvIrSendManchesterBit(carrier, false, 889, ctx))
				return false;
			for (i = 0; i < 5; i++)
				if (!uiPrvIrSendManchesterBit(carrier, !!(address & (1 << (4 - i))), 889, ctx))
					return false;
			for (i = 0; i < 6; i++)
				if (!uiPrvIrSendManchesterBit(carrier, !!(cmd & (1 << (5 - i))), 889, ctx))
					return false;
		}
		else if (!strcmp(protocol, "RC6")) {
			uint_fast8_t i;

			carrier = 36000;
			if (!uiPrvIrMarkCancellable(carrier, 2666, ctx) || !uiPrvIrSpaceCancellable(889, ctx) ||
				!uiPrvIrSendManchesterBit(carrier, true, 444, ctx) ||
				!uiPrvIrSendManchesterBit(carrier, false, 444, ctx) ||
				!uiPrvIrSendManchesterBit(carrier, false, 444, ctx) ||
				!uiPrvIrSendManchesterBit(carrier, false, 444, ctx) ||
				!uiPrvIrSendManchesterBit(carrier, false, 889, ctx))
				return false;
			for (i = 0; i < 8; i++)
				if (!uiPrvIrSendManchesterBit(carrier, !!(address & (1 << (7 - i))), 444, ctx))
					return false;
			for (i = 0; i < 8; i++)
				if (!uiPrvIrSendManchesterBit(carrier, !!(command & (1 << (7 - i))), 444, ctx))
					return false;
		}
		else if (!strcmp(protocol, "Kaseikyo")) {
			uint32_t vendor = address & 0xffff;
			uint32_t payload = ((address >> 16) & 0x03ff) | ((command & 0x03ff) << 10);
			uint8_t parity = (vendor ^ (vendor >> 8)) & 0x0f;

			if (!uiPrvIrMarkCancellable(carrier, 3360, ctx) || !uiPrvIrSpaceCancellable(1650, ctx) ||
				!uiPrvIrSendBitsPulseDistance(carrier, vendor, 16, false, 432, 432, 1296, ctx) ||
				!uiPrvIrSendBitsPulseDistance(carrier, parity, 4, false, 432, 432, 1296, ctx) ||
				!uiPrvIrSendBitsPulseDistance(carrier, payload, 20, false, 432, 432, 1296, ctx) ||
				!uiPrvIrSendBitsPulseDistance(carrier, (payload ^ (payload >> 8) ^ (payload >> 16)) & 0xff, 8, false, 432, 432, 1296, ctx) ||
				!uiPrvIrMarkCancellable(carrier, 432, ctx))
				return false;
		}
		else {
			return false;
		}

		return uiPrvIrSpaceCancellable(45000, ctx);
	}

	static bool uiPrvIrSendCodeLine(struct Canvas *cnv, const char *title, const char *codeStr, const char *name, uint32_t carrier, uint32_t repeat, uint32_t codeIdx, uint32_t recordIndex, uint32_t lineNo, struct IrBlastStats *stats, bool *malformedP)
	{
		uint32_t rep;

		*malformedP = false;

		if (!repeat)
			repeat = 1;
		if (repeat > IR_MAX_REPEAT) {
			*malformedP = true;
			return false;
		}
		if (!carrier)
			carrier = IR_DEFAULT_CARRIER;
		if (carrier < IR_MIN_CARRIER || carrier > IR_MAX_CARRIER) {
			*malformedP = true;
			return false;
		}

		for (rep = 0; rep < repeat; rep++) {
			struct IrSendCtx ctx;
			const char *str = codeStr;
			bool mark = true;
			uint32_t numDurations = 0;
			uint32_t totalUsec = 0;

			uiPrvIrSendCtxInit(&ctx, stats, codeIdx, recordIndex, lineNo, name, "raw", IR_MAX_RAW_REPEAT_USEC);
			uiPrvIrDrawProgress(cnv, title, name, "raw", codeIdx, recordIndex, lineNo, rep + 1, repeat);

			while (1) {
				uint32_t duration;

				if (!uiPrvParseU32(&str, &duration)) {
					*malformedP = true;
					return false;
				}
				if (!duration || duration > IR_MAX_RAW_DURATION || numDurations >= IR_MAX_RAW_DURATIONS) {
					*malformedP = true;
					return false;
				}
				if (duration > IR_MAX_RAW_REPEAT_USEC - totalUsec) {
					*malformedP = true;
					return false;
				}
				totalUsec += duration;

				if (mark) {
					if (!uiPrvIrMarkCancellable(carrier, duration, &ctx))
						return false;
				}
				else if (!uiPrvIrSpaceCancellable(duration, &ctx))
					return false;

				numDurations++;
				mark = !mark;

				while (*str == ' ' || *str == '\t')
					str++;

				if (!*str)
					break;
				if (*str == ',') {
					str++;
					continue;
				}
				if (*str >= '0' && *str <= '9')
					continue;

				*malformedP = true;
				return false;
			}

			if (!numDurations) {
				*malformedP = true;
				return false;
			}

			if (!uiPrvIrSpaceCancellable(45000, &ctx))
				return false;
		}

		return true;
	}

	static void uiPrvIrAlertStalled(struct Canvas *cnv, const char *title, const struct IrBlastStats *stats)
	{
		char msg[160];
		const char *name = stats->stalledName[0] ? stats->stalledName : "IR code";
		const char *detail = stats->stalledDetail[0] ? stats->stalledDetail : "record";

		(void)sprintf(msg, "%s stalled\nCode %u Rec %u\nLine %u %s\n%s",
			title,
			(unsigned)stats->stalledCode,
			(unsigned)stats->stalledRecord,
			(unsigned)stats->stalledLine,
			detail,
			name);
		uiAlert(cnv, msg, DialogTypeOk);
	}


	static char uiPrvUpper(char c)
	{
		if (c >= 'a' && c <= 'z')
			return c - 'a' + 'A';
		return c;
	}

	static bool uiPrvEqNoCase(const char *a, const char *b)
	{
		while (*a && *b) {
			if (uiPrvUpper(*a++) != uiPrvUpper(*b++))
				return false;
		}
		return !*a && !*b;
	}
	static bool uiPrvIrIsPowerName(const char *name)
	{
		return uiPrvEqNoCase(name, "Power") || uiPrvEqNoCase(name, "Pwr") || uiPrvEqNoCase(name, "POWER_OFF") || uiPrvEqNoCase(name, "Power_off");
	}

	static bool uiPrvIrNameMatches(const char *name, const char *wantedName)
	{
		if (wantedName)
			return uiPrvEqNoCase(name, wantedName) || (uiPrvEqNoCase(wantedName, "Mute") && uiPrvEqNoCase(name, "MUTE"));

		return uiPrvIrIsPowerName(name);
	}

	static void uiPrvFlipperRecordInit(struct FlipperIrRecord *rec)
	{
		memset(rec, 0, sizeof(*rec));
		rec->frequency = IR_DEFAULT_CARRIER;
	}

	static void uiPrvFlipperRecordFinish(struct Canvas *cnv, struct FlipperIrRecord *rec, struct IrBlastStats *stats, const char *wantedName, const char *title)
	{
		struct IrSendCtx ctx;

		if (stats->cancelled || stats->timedOut)
			return;

		if (!rec->hasName)
			return;

		if (!uiPrvIrNameMatches(rec->name, wantedName))
			return;

		if (rec->type == FlipperIrTypeRaw) {
			if (!rec->sentRaw)
				stats->skipped++;
			return;
		}

		if (rec->type != FlipperIrTypeParsed || !rec->hasProtocol || !rec->hasAddress || !rec->hasCommand) {
			stats->malformed++;
			return;
		}

		uiPrvIrSendCtxInit(&ctx, stats, stats->sent + 1, rec->recordIndex, rec->lineNo, rec->name, rec->protocol, IR_PARSED_RECORD_TIMEOUT_USEC);
		uiPrvIrDrawProgress(cnv, title, rec->name, rec->protocol, stats->sent + 1, rec->recordIndex, rec->lineNo, 1, 1);
		if (uiPrvIrSendParsed(rec->protocol, rec->address, rec->command, &ctx))
			stats->sent++;
		else if (!stats->cancelled && !stats->timedOut)
			stats->skipped++;
	}

	static bool uiPrvIrReadLineStat(struct FatfsFil *fil, char *line, struct IrBlastStats *stats)
	{
		bool truncated, cancelled;

		if (!stats->fileSize)
			stats->fileSize = fatfsFileGetSize(fil);
		if (!uiPrvReadLine(fil, line, IR_LINE_BUF_SZ, &stats->bytesRead, stats->fileSize, &truncated, &cancelled)) {
			if (cancelled)
				stats->cancelled = true;
			if (truncated)
				stats->lineTooLong = true;
			return false;
		}

		stats->lineNo++;
		if (truncated)
			stats->lineTooLong = true;

		return !stats->lineTooLong;
	}

	static bool uiPrvIrOpenPowerFile(struct FatfsVol *vol, const char **pathP, struct FatfsFil **filP)
	{
		struct FatfsFil *fil = fatfsFileOpen(vol, IR_FLIPPER_TV_FILE, OPEN_MODE_READ);

		if (fil) {
			*pathP = IR_FLIPPER_TV_FILE;
			*filP = fil;
			return true;
		}

		fil = fatfsFileOpen(vol, IR_POWER_FILE, OPEN_MODE_READ);
		if (fil) {
			*pathP = IR_POWER_FILE;
			*filP = fil;
			return true;
		}

		return false;
	}

	static bool uiPrvIrAcquireLineWorkspace(struct Canvas *cnv, const char *message, struct ToolWorkspaceSpan *lineMem)
	{
		bool acquired = toolWorkspaceAcquire(ToolWorkspaceCartRamUpper, ToolWorkspaceOwnerIr, lineMem);

		if (!acquired || lineMem->size < IR_LINE_BUF_SZ) {
			if (acquired)
				toolWorkspaceRelease(ToolWorkspaceCartRamUpper, ToolWorkspaceOwnerIr);
			uiAlert(cnv, message, DialogTypeOk);
			return false;
		}
		return true;
	}

	static bool uiPrvIrFileIsFlipper(const char *trimmed)
	{
		return !strcmp(trimmed, "Filetype: IR signals file") || !strcmp(trimmed, "Filetype: IR library file");
	}

	static bool uiPrvIrRewindRead(struct FatfsFil *fil, struct IrBlastStats *stats)
	{
		if (!fatfsFileSeek(fil, 0))
			return false;
		if (stats) {
			stats->lineNo = 0;
			stats->bytesRead = 0;
			stats->fileSize = fatfsFileGetSize(fil);
		}
		return true;
	}

	static bool uiPrvIrDetectFormat(struct FatfsFil *fil, char *line, struct IrBlastStats *stats, bool *isFlipperP)
	{
		while (uiPrvIrReadLineStat(fil, line, stats)) {
			char *trimmed = uiPrvTrim(line);

			if (!*trimmed || *trimmed == '#')
				continue;

			if (!strcmp(trimmed, IR_FILE_MAGIC)) {
				*isFlipperP = false;
				return uiPrvIrRewindRead(fil, stats);
			}

			if (uiPrvIrFileIsFlipper(trimmed)) {
				*isFlipperP = true;
				return uiPrvIrRewindRead(fil, stats);
			}

			stats->malformed++;
			return false;
		}

		return false;
	}

	static bool uiPrvIrPowerBlastDc32(struct Canvas *cnv, struct FatfsFil *fil, char *line, struct IrBlastStats *stats)
	{
		char name[IR_NAME_BUF_SZ];
		uint32_t carrier = IR_DEFAULT_CARRIER, repeat = IR_DEFAULT_REPEAT, codeIdx = 0;
		bool haveMagic = false;

		uiPrvCopyStr(name, sizeof(name), "IR code");

		while (uiPrvIrReadLineStat(fil, line, stats)) {
			char *trimmed;

			trimmed = uiPrvTrim(line);
			if (!*trimmed || *trimmed == '#')
				continue;

			if (strcmp(trimmed, IR_FILE_MAGIC)) {
				stats->malformed++;
				return false;
			}

			haveMagic = true;
			break;
		}

		if (!haveMagic)
			return false;

		while (uiPrvIrReadLineStat(fil, line, stats)) {
			char *trimmed;

			trimmed = uiPrvTrim(line);
			if (!*trimmed || *trimmed == '#')
				continue;

			if (uiPrvStartsWith(trimmed, "name=")) {
				uiPrvCopyStr(name, sizeof(name), uiPrvTrim(trimmed + 5));
			}
			else if (uiPrvStartsWith(trimmed, "carrier=")) {
				const char *str = trimmed + 8;

				if (!uiPrvParseU32(&str, &carrier) || *str)
					stats->malformed++;
			}
			else if (uiPrvStartsWith(trimmed, "repeat=")) {
				const char *str = trimmed + 7;

				if (!uiPrvParseU32(&str, &repeat) || *str)
					stats->malformed++;
			}
			else if (uiPrvStartsWith(trimmed, "code=")) {
				bool malformed = false;

				stats->recordIndex++;
				codeIdx++;
				if (uiPrvIrSendCodeLine(cnv, "Power", uiPrvTrim(trimmed + 5), name, carrier, repeat, codeIdx, stats->recordIndex, stats->lineNo, stats, &malformed))
					stats->sent++;
				if (malformed)
					stats->malformed++;
				if (malformed || stats->cancelled || stats->timedOut)
					return false;

				uiPrvCopyStr(name, sizeof(name), "IR code");
				carrier = IR_DEFAULT_CARRIER;
				repeat = IR_DEFAULT_REPEAT;
			}
			else {
				stats->malformed++;
				return false;
			}
		}

		return stats->sent != 0;
	}

	static bool uiPrvIrBlastFlipper(struct Canvas *cnv, struct FatfsFil *fil, char *line, struct IrBlastStats *stats, const char *wantedName, const char *title, uint32_t maxSent)
	{
		struct FlipperIrRecord rec;
		bool haveHeader = false, haveVersion = false, recDirty = false;

		uiPrvFlipperRecordInit(&rec);

		while (uiPrvIrReadLineStat(fil, line, stats) && !stats->cancelled && !stats->timedOut && (!maxSent || stats->sent < maxSent)) {
			char *trimmed = uiPrvTrim(line);

			if (!*trimmed)
				continue;

			if (*trimmed == '#') {
				if (recDirty) {
					uiPrvFlipperRecordFinish(cnv, &rec, stats, wantedName, title);
					uiPrvFlipperRecordInit(&rec);
					recDirty = false;
				}
				continue;
			}

			if (uiPrvStartsWith(trimmed, "Filetype:")) {
				if (!uiPrvIrFileIsFlipper(trimmed))
					stats->malformed++;
				else
					haveHeader = true;
				continue;
			}

			if (uiPrvStartsWith(trimmed, "Version:")) {
				const char *str = uiPrvTrim(trimmed + 8);
				uint32_t version;

				if (!uiPrvParseU32(&str, &version) || version != 1)
					stats->malformed++;
				else
					haveVersion = true;
				continue;
			}

			if (uiPrvStartsWith(trimmed, "name:")) {
				if (recDirty) {
					uiPrvFlipperRecordFinish(cnv, &rec, stats, wantedName, title);
					uiPrvFlipperRecordInit(&rec);
					if (stats->cancelled || stats->timedOut)
						break;
				}
				stats->recordIndex++;
				rec.recordIndex = stats->recordIndex;
				rec.lineNo = stats->lineNo;
				uiPrvCopyStr(rec.name, sizeof(rec.name), uiPrvTrim(trimmed + 5));
				rec.hasName = true;
				recDirty = true;
			}
			else if (uiPrvStartsWith(trimmed, "type:")) {
				char *type = uiPrvTrim(trimmed + 5);

				recDirty = true;
				if (!strcmp(type, "raw"))
					rec.type = FlipperIrTypeRaw;
				else if (!strcmp(type, "parsed"))
					rec.type = FlipperIrTypeParsed;
				else
					stats->malformed++;
			}
			else if (uiPrvStartsWith(trimmed, "protocol:")) {
				char *protocol = uiPrvTrim(trimmed + 9);

				recDirty = true;
				uiPrvCopyStr(rec.protocol, sizeof(rec.protocol), protocol);
				rec.hasProtocol = !!*protocol;
				if (!rec.hasProtocol)
					stats->malformed++;
			}
			else if (uiPrvStartsWith(trimmed, "address:")) {
				recDirty = true;
				rec.hasAddress = uiPrvParseFlipperU32Bytes(uiPrvTrim(trimmed + 8), &rec.address);
				if (!rec.hasAddress)
					stats->malformed++;
			}
			else if (uiPrvStartsWith(trimmed, "command:")) {
				recDirty = true;
				rec.hasCommand = uiPrvParseFlipperU32Bytes(uiPrvTrim(trimmed + 8), &rec.command);
				if (!rec.hasCommand)
					stats->malformed++;
			}
			else if (uiPrvStartsWith(trimmed, "frequency:")) {
				const char *str = uiPrvTrim(trimmed + 10);

				recDirty = true;
				rec.hasFrequency = uiPrvParseU32(&str, &rec.frequency) && !*str;
				if (!rec.hasFrequency)
					stats->malformed++;
			}
			else if (uiPrvStartsWith(trimmed, "duty_cycle:")) {
				//ignored for now
				recDirty = true;
			}
			else if (uiPrvStartsWith(trimmed, "data:")) {
				recDirty = true;
				if (rec.hasName && uiPrvIrNameMatches(rec.name, wantedName) && rec.type == FlipperIrTypeRaw) {
					bool malformed = false;

					if (uiPrvIrSendCodeLine(cnv, title, uiPrvTrim(trimmed + 5), rec.name, rec.frequency, 1, stats->sent + 1, rec.recordIndex, stats->lineNo, stats, &malformed)) {
						rec.sentRaw = true;
						stats->sent++;
					}
					if (malformed)
						stats->malformed++;
				}
			}
			else {
				recDirty = true;
				stats->malformed++;
			}
		}

		if (recDirty && !stats->lineTooLong && !stats->cancelled && !stats->timedOut && (!maxSent || stats->sent < maxSent))
			uiPrvFlipperRecordFinish(cnv, &rec, stats, wantedName, title);

		if (!haveHeader || !haveVersion)
			stats->malformed++;

		return stats->sent != 0;
	}

	static bool uiPrvIrPowerBlast(struct Canvas *cnv)
	{
		struct FatfsVol *vol = NULL;
		struct FatfsFil *fil = NULL;
		const char *path = NULL;
		struct ToolWorkspaceSpan lineMem;
		char *line;
		struct IrBlastStats stats;
		bool ret = false, isFlipper = false, irStarted = false;

		memset(&stats, 0, sizeof(stats));
		if (!uiPrvIrAcquireLineWorkspace(cnv, "Tool workspace is too small for Universal Remote", &lineMem))
			return false;
		line = (char*)lineMem.ptr;

		vol = uiPrvMountCard(cnv, false);
		if (!vol)
			goto out_release;

		if (!uiPrvIrOpenPowerFile(vol, &path, &fil)) {
			uiAlert(cnv, "Cannot find /IR/tv.ir or /IR/POWER.IR on the SD card", DialogTypeOk);
			goto out;
		}

		if (!uiPrvIrDetectFormat(fil, line, &stats, &isFlipper))
			goto out;

		memset(&stats, 0, sizeof(stats));
		irRemoteBegin();
		irStarted = true;
		ret = isFlipper ? uiPrvIrBlastFlipper(cnv, fil, line, &stats, NULL, "Power", 0) : uiPrvIrPowerBlastDc32(cnv, fil, line, &stats);

	out:
		if (irStarted)
			irRemoteEnd();
		if (fil)
			fatfsFileClose(fil);
		(void)uiPrvCardPreUnmount();
		fatfsUnmount(vol);

		if (stats.lineTooLong) {
			uiAlert(cnv, "A line in the IR file is too long", DialogTypeOk);
		}
		else if (stats.timedOut) {
			uiPrvIrAlertStalled(cnv, "Universal Remote spam", &stats);
		}
		else if (stats.malformed && !stats.sent) {
			char msg[80];

			(void)sprintf(msg, "Malformed IR file near line %u", (unsigned)stats.lineNo);
			uiAlert(cnv, msg, DialogTypeOk);
		}
		else if (stats.cancelled) {
			if (!uiPrvToolExitRequested())
				uiAlert(cnv, "Universal Remote spam cancelled", DialogTypeOk);
		}
		else if (ret) {
			char msg[96];

			(void)sprintf(msg, "Universal Remote spam complete\nSent: %u\nSkipped: %u\nMalformed: %u", (unsigned)stats.sent, (unsigned)stats.skipped, (unsigned)stats.malformed);
			uiAlert(cnv, msg, DialogTypeOk);
		}
		else if (path) {
			uiAlert(cnv, "No power codes found in the IR file", DialogTypeOk);
		}

	out_release:
		toolWorkspaceRelease(ToolWorkspaceCartRamUpper, ToolWorkspaceOwnerIr);
		return ret;
	}

	static bool uiPrvIrMuteBlast(struct Canvas *cnv)
	{
		struct FatfsVol *vol = NULL;
		struct FatfsFil *fil = NULL;
		struct ToolWorkspaceSpan lineMem;
		char *line;
		struct IrBlastStats stats;
		bool ret = false, isFlipper = false, irStarted = false;

		memset(&stats, 0, sizeof(stats));
		if (!uiPrvIrAcquireLineWorkspace(cnv, "Tool workspace is too small for Universal Remote", &lineMem))
			return false;
		line = (char*)lineMem.ptr;

		vol = uiPrvMountCard(cnv, false);
		if (!vol)
			goto out_release;

		fil = fatfsFileOpen(vol, IR_FLIPPER_TV_FILE, OPEN_MODE_READ);
		if (!fil) {
			uiAlert(cnv, "Cannot find /IR/tv.ir on the SD card", DialogTypeOk);
			goto out;
		}

		if (!uiPrvIrDetectFormat(fil, line, &stats, &isFlipper) || !isFlipper)
			goto out;

		memset(&stats, 0, sizeof(stats));
		irRemoteBegin();
		irStarted = true;
		ret = uiPrvIrBlastFlipper(cnv, fil, line, &stats, "Mute", "Mute", 0);

	out:
		if (irStarted)
			irRemoteEnd();
		if (fil)
			fatfsFileClose(fil);
		(void)uiPrvCardPreUnmount();
		fatfsUnmount(vol);

		if (stats.lineTooLong) {
			uiAlert(cnv, "A line in /IR/tv.ir is too long", DialogTypeOk);
		}
		else if (stats.timedOut) {
			uiPrvIrAlertStalled(cnv, "Mute", &stats);
		}
		else if (stats.malformed && !stats.sent) {
			char msg[80];

			(void)sprintf(msg, "Malformed /IR/tv.ir near line %u", (unsigned)stats.lineNo);
			uiAlert(cnv, msg, DialogTypeOk);
		}
		else if (stats.cancelled) {
			if (!uiPrvToolExitRequested())
				uiAlert(cnv, "Mute cancelled", DialogTypeOk);
		}
		else if (ret) {
			char msg[96];

			(void)sprintf(msg, "Mute complete\nSent: %u\nSkipped: %u\nMalformed: %u", (unsigned)stats.sent, (unsigned)stats.skipped, (unsigned)stats.malformed);
			uiAlert(cnv, msg, DialogTypeOk);
		}
		else if (fil) {
			uiAlert(cnv, "No mute codes found in /IR/tv.ir", DialogTypeOk);
		}

	out_release:
		toolWorkspaceRelease(ToolWorkspaceCartRamUpper, ToolWorkspaceOwnerIr);
		return ret;
	}

	static bool uiPrvIrBlastLocator(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *locator, const char *wantedName, const char *title)
	{
		struct FatfsFil *fil = NULL;
		struct ToolWorkspaceSpan lineMem;
		char *line;
		struct IrBlastStats stats;
		bool ret = false, isFlipper = false, irStarted = false;

		memset(&stats, 0, sizeof(stats));
		if (!uiPrvIrAcquireLineWorkspace(cnv, "Tool workspace is too small for Universal Remote", &lineMem))
			return false;
		line = (char*)lineMem.ptr;

		fil = fatfsFileOpenWithLocator(vol, locator, OPEN_MODE_READ);
		if (!fil) {
			uiAlert(cnv, "Cannot open IR file", DialogTypeOk);
			goto out_report;
		}

		if (!uiPrvIrDetectFormat(fil, line, &stats, &isFlipper))
			goto out_close;
		if (wantedName && !isFlipper) {
			uiAlert(cnv, "Selected IR action requires a Flipper IR file", DialogTypeOk);
			goto out_close;
		}

		memset(&stats, 0, sizeof(stats));
		irRemoteBegin();
		irStarted = true;
		ret = isFlipper ? uiPrvIrBlastFlipper(cnv, fil, line, &stats, wantedName, title, 0) : uiPrvIrPowerBlastDc32(cnv, fil, line, &stats);

	out_close:
		if (irStarted)
			irRemoteEnd();
		if (fil)
			fatfsFileClose(fil);
	out_report:
		if (stats.lineTooLong) {
			uiAlert(cnv, "A line in the IR file is too long", DialogTypeOk);
		}
		else if (stats.timedOut) {
			uiPrvIrAlertStalled(cnv, title, &stats);
		}
		else if (stats.malformed && !stats.sent) {
			char msg[80];

			(void)sprintf(msg, "Malformed IR file near line %u", (unsigned)stats.lineNo);
			uiAlert(cnv, msg, DialogTypeOk);
		}
		else if (stats.cancelled) {
			if (!uiPrvToolExitRequested())
				uiAlert(cnv, "Universal Remote action cancelled", DialogTypeOk);
		}
		else if (ret) {
			char msg[96];

			(void)sprintf(msg, "Universal Remote action complete\nSent: %u\nSkipped: %u\nMalformed: %u", (unsigned)stats.sent, (unsigned)stats.skipped, (unsigned)stats.malformed);
			uiAlert(cnv, msg, DialogTypeOk);
		}
		else if (fil) {
			uiAlert(cnv, "No matching IR codes found in the selected file", DialogTypeOk);
		}

		toolWorkspaceRelease(ToolWorkspaceCartRamUpper, ToolWorkspaceOwnerIr);
		return ret;
	}

	static bool uiPrvIrRemoteFileName(const char *fname)
	{
		return uiPrvStrEndsWithNoCase(fname, ".ir");
	}

	static const struct IrUniversalRemote *uiPrvIrKnownRemoteForName(const char *fname)
	{
		const char *baseName = uiPrvBaseName(fname);
		uint_fast8_t i;

		for (i = 0; i < sizeof(mIrUniversalRemotes) / sizeof(*mIrUniversalRemotes); i++)
			if (uiPrvEqNoCase(baseName, uiPrvBaseName(mIrUniversalRemotes[i].path)))
				return &mIrUniversalRemotes[i];

		return NULL;
	}

	static bool uiPrvIrButtonListContains(struct MusicOption *head, const char *name)
	{
		while (head) {
			if (uiPrvEqNoCase(head->name, name))
				return true;
			head = head->next;
		}

		return false;
	}

	static bool uiPrvIrButtonListAppend(struct UiFileListCtx *ctx, const char *name)
	{
		struct FatFileLocator loc;

		if (uiPrvIrButtonListContains(ctx->head, name))
			return true;

		memset(&loc, 0, sizeof(loc));
		return uiPrvFileListAppendTail(ctx, name, 0, false, &loc);
	}

	static uint32_t uiPrvIrListButtons(struct FatfsFil *fil, char *line, struct MusicOption **headP, bool *overflowP, bool *lineTooLongP)
	{
		struct UiFileListCtx ctx;
		struct IrBlastStats stats;
		bool isFlipper = false;
		struct ToolWorkspaceSpan listMem = toolWorkspaceGet(ToolWorkspaceCartRamLower);

		memset(&ctx, 0, sizeof(ctx));
		*lineTooLongP = false;
		if (!listMem.ptr || listMem.size < sizeof(struct MusicOption))
			goto out;
		ctx.nextAvail = (struct MusicOption*)listMem.ptr;
		ctx.spaceAvail = listMem.size;
		memset(&stats, 0, sizeof(stats));

		if (!uiPrvIrDetectFormat(fil, line, &stats, &isFlipper) || !isFlipper)
			goto out;

		memset(&stats, 0, sizeof(stats));
		while (uiPrvIrReadLineStat(fil, line, &stats)) {
			char *trimmed = uiPrvTrim(line);

			if (uiPrvStartsWith(trimmed, "name:"))
				if (!uiPrvIrButtonListAppend(&ctx, uiPrvTrim(trimmed + 5)))
					break;
		}

		*lineTooLongP = stats.lineTooLong;

	out:
		*headP = ctx.head;
		if (overflowP)
			*overflowP = ctx.overflow;
		return ctx.count;
	}

	static struct MusicOption *uiPrvChooseFlatOption(struct Canvas *cnv, struct MusicOption *head, uint32_t numItems, const char *title)
	{
		uint32_t topItem = 0, selectedItem = 0, prevTopItem, prevSelOnscreenItem;
		uint_fast8_t itemHeight = uiPrvGlyphHeight(cnv) + 1, pathTop, listTop, itemsOnscreen, itemLeft;
		struct FontGlyphInfo gi;

		if (!numItems)
			return NULL;

		itemLeft = fontGetGlyphInfo(&gi, cnv->font, MENU_SELECTION_CHAR) ? gi.width + 2 : 10;
		pathTop = fontGetHeight(FontLarge) + fontGetHeight(FontMedium) / 4;
		listTop = pathTop + itemHeight;
		itemsOnscreen = (cnv->h - listTop - 2u * itemHeight) / itemHeight;
		if (!itemsOnscreen) {
			uiAlert(cnv, "Display area too small for chooser", DialogTypeOk);
			return NULL;
		}
		if (itemsOnscreen > numItems)
			itemsOnscreen = numItems;
		prevTopItem = topItem + 1;
		prevSelOnscreenItem = selectedItem - topItem + 1;

		while (1) {
			struct MusicOption *draw = head;
			uint32_t i, selectedOnscreenItem = selectedItem - topItem;

			if (prevTopItem != topItem) {
				uint_fast8_t scrollWidth;

				uiPrvReset(cnv, false);
				uiPrvDrawTruncText(cnv, pathTop, 10, cnv->w - 10, title);
				scrollWidth = numItems > itemsOnscreen ? uiPrvDrawScrollbar(cnv, listTop, numItems, topItem, itemsOnscreen) : 0;
				for (i = 0; i < topItem && draw; i++)
					draw = draw->next;
				cnv->foreColor = 12;
				for (i = 0; i < itemsOnscreen && draw; i++, draw = draw->next)
					uiPrvDrawTruncText(cnv, listTop + i * itemHeight, itemLeft, cnv->w - scrollWidth - itemLeft, draw->name);
				uiPrvDrawMenuFooter(cnv, KEY_BIT_A | KEY_BIT_B);
				prevSelOnscreenItem = selectedOnscreenItem + 1;
			}
			prevTopItem = topItem;
			if (prevSelOnscreenItem != selectedOnscreenItem) {
				if (prevSelOnscreenItem < itemsOnscreen) {
					cnv->foreColor = 0;
					uiPrvDrawOneChar(cnv, listTop + itemHeight * prevSelOnscreenItem, 1, MENU_SELECTION_CHAR);
				}
				cnv->foreColor = 15;
				uiPrvDrawOneChar(cnv, listTop + itemHeight * selectedOnscreenItem, 1, MENU_SELECTION_CHAR);
			}
			prevSelOnscreenItem = selectedOnscreenItem;

			switch (uiPrvRecvMenuKeypress(cnv)) {
				case UI_KEY_BIT_CENTER:
					uiPrvRequestToolExit();
					return NULL;

				case KEY_BIT_A:
					return uiPrvFileOptionAt(head, 0, selectedItem);

				case KEY_BIT_B:
					return NULL;

				case KEY_BIT_DOWN:
					if (selectedItem + 1 < numItems) {
						selectedItem++;
						if (selectedItem >= topItem + itemsOnscreen) {
							topItem += itemsOnscreen;
							if (topItem + itemsOnscreen > numItems)
								topItem = numItems > itemsOnscreen ? numItems - itemsOnscreen : 0;
						}
					}
					else if (!mUiKeyRepeated) {
						selectedItem = 0;
						topItem = 0;
					}
					break;

				case KEY_BIT_UP:
					if (selectedItem) {
						selectedItem--;
						if (selectedItem < topItem) {
							if (topItem > itemsOnscreen)
								topItem -= itemsOnscreen;
							else
								topItem = 0;
							if (selectedItem < topItem)
								topItem = selectedItem;
						}
					}
					else if (!mUiKeyRepeated) {
						selectedItem = numItems - 1;
						topItem = numItems > itemsOnscreen ? numItems - itemsOnscreen : 0;
					}
					break;
			}
		}
	}

	static bool uiPrvIrSendButtonSpamFile(struct Canvas *cnv, struct FatfsFil *fil, const char *buttonName)
	{
		struct ToolWorkspaceSpan lineMem;
		char *line;
		struct IrBlastStats stats;
		bool ret = false, isFlipper = false, irStarted = false;

		memset(&stats, 0, sizeof(stats));
		if (!uiPrvIrAcquireLineWorkspace(cnv, "Tool workspace is too small for Universal Remote button spam", &lineMem))
			return false;
		line = (char*)lineMem.ptr;

		if (!uiPrvIrRewindRead(fil, &stats))
			goto out_report;
		if (!uiPrvIrDetectFormat(fil, line, &stats, &isFlipper) || !isFlipper)
			goto out_report;

		memset(&stats, 0, sizeof(stats));
		irRemoteBegin();
		irStarted = true;
		ret = uiPrvIrBlastFlipper(cnv, fil, line, &stats, buttonName, buttonName && *buttonName ? buttonName : "IR Button", 0);

	out_report:
		if (irStarted)
			irRemoteEnd();
		if (stats.lineTooLong) {
			uiAlert(cnv, "A line in the IR file is too long", DialogTypeOk);
		}
		else if (stats.timedOut) {
			uiPrvIrAlertStalled(cnv, "Universal Remote button spam", &stats);
		}
		else if (stats.cancelled) {
			if (!uiPrvToolExitRequested())
				uiAlert(cnv, "Universal Remote button spam cancelled", DialogTypeOk);
		}
		else if (ret) {
			char msg[96];

			(void)sprintf(msg, "Universal Remote button spam complete\nSent: %u\nSkipped: %u\nMalformed: %u", (unsigned)stats.sent, (unsigned)stats.skipped, (unsigned)stats.malformed);
			uiAlert(cnv, msg, DialogTypeOk);
		}
		else if (isFlipper) {
			uiAlert(cnv, "Selected IR button could not be sent", DialogTypeOk);
		}

		toolWorkspaceRelease(ToolWorkspaceCartRamUpper, ToolWorkspaceOwnerIr);
		return ret;
	}

	static const char *uiPrvIrKnownActionMenu(struct Canvas *cnv, const struct IrUniversalRemote *remote)
	{
		uint_fast8_t i, selOption;
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;

		if (!remote->numButtons)
			return NULL;

		uiPrvSetHeaderTitle(remote->name);
		uiPrvReset(cnv, false);
		for (i = 0; i < remote->numButtons; i++)
			uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, remote->buttons[i], -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, remote->numButtons), 10, "Back", -1);

		selOption = uiPrvMenu(cnv, 0, remote->numButtons + 1, &button);
		if (uiPrvToolExitRequested())
			return NULL;
		if (button == KEY_BIT_B || selOption == remote->numButtons)
			return NULL;

		return remote->buttons[selOption];
	}

	static bool uiPrvIrKnownActionFile(struct Canvas *cnv, struct FatfsFil *fil, const struct IrUniversalRemote *remote)
	{
		const char *buttonName = uiPrvIrKnownActionMenu(cnv, remote);

		if (!buttonName)
			return false;

		return uiPrvIrSendButtonSpamFile(cnv, fil, buttonName);
	}

	static bool uiPrvIrKnownActionLocator(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *locator, const struct IrUniversalRemote *remote)
	{
		struct FatfsFil *fil = fatfsFileOpenWithLocator(vol, locator, OPEN_MODE_READ);
		bool ret = false;

		if (!fil) {
			uiAlert(cnv, "Cannot open IR file", DialogTypeOk);
			return false;
		}

		ret = uiPrvIrKnownActionFile(cnv, fil, remote);
		fatfsFileClose(fil);
		return ret;
	}

	static bool uiPrvIrButtonSpamFile(struct Canvas *cnv, struct FatfsFil *fil, const char *fileName)
	{
		struct MusicOption *buttons = NULL, *button;
		char buttonName[IR_NAME_BUF_SZ];
		struct ToolWorkspaceSpan lineMem, listMem;
		char *line;
		bool ret = false, overflow = false, lineTooLong = false;
		uint32_t numButtons;

		if (!uiPrvIrAcquireLineWorkspace(cnv, "Tool workspace is too small for Universal Remote button spam", &lineMem))
			return false;
		if (!toolWorkspaceAcquire(ToolWorkspaceCartRamLower, ToolWorkspaceOwnerIr, &listMem)) {
			uiAlert(cnv, "Tool workspace is too small for Universal Remote button spam", DialogTypeOk);
			toolWorkspaceRelease(ToolWorkspaceCartRamUpper, ToolWorkspaceOwnerIr);
			return false;
		}
		line = (char*)lineMem.ptr;

		numButtons = uiPrvIrListButtons(fil, line, &buttons, &overflow, &lineTooLong);
		if (lineTooLong) {
			uiAlert(cnv, "A line in the IR file is too long", DialogTypeOk);
			goto out_close;
		}
		if (overflow)
			uiAlert(cnv, "Universal Remote file has too many buttons; showing what fits", DialogTypeOk);
		if (!numButtons) {
			uiAlert(cnv, "No Flipper buttons found in that IR file", DialogTypeOk);
			goto out_close;
		}

		button = uiPrvChooseFlatOption(cnv, buttons, numButtons, fileName);
		if (!button)
			goto out_close;
		uiPrvCopyStr(buttonName, sizeof(buttonName), button->name);
		toolWorkspaceRelease(ToolWorkspaceCartRamLower, ToolWorkspaceOwnerIr);
		toolWorkspaceRelease(ToolWorkspaceCartRamUpper, ToolWorkspaceOwnerIr);
		return uiPrvIrSendButtonSpamFile(cnv, fil, buttonName);

	out_close:
		toolWorkspaceRelease(ToolWorkspaceCartRamLower, ToolWorkspaceOwnerIr);
		toolWorkspaceRelease(ToolWorkspaceCartRamUpper, ToolWorkspaceOwnerIr);
		return ret;
	}

	static bool uiPrvIrButtonSpamLocator(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *locator, const char *fileName)
	{
		struct FatfsFil *fil = fatfsFileOpenWithLocator(vol, locator, OPEN_MODE_READ);
		bool ret = false;

		if (!fil) {
			uiAlert(cnv, "Cannot open IR file", DialogTypeOk);
			return false;
		}

		ret = uiPrvIrButtonSpamFile(cnv, fil, fileName);
		fatfsFileClose(fil);
		return ret;
	}

	static bool uiPrvIrUniversalRemote(struct Canvas *cnv, const struct IrUniversalRemote *remote)
	{
		struct FatfsVol *vol = NULL;
		struct FatfsFil *fil = NULL;
		bool ret = false;
		char msg[96];

		vol = uiPrvMountCard(cnv, false);
		if (!vol)
			return false;

		fil = fatfsFileOpen(vol, remote->path, OPEN_MODE_READ);
		if (!fil) {
			(void)sprintf(msg, "Cannot find %s on the SD card", remote->path);
			uiAlert(cnv, msg, DialogTypeOk);
			goto out_unmount;
		}

		ret = uiPrvIrKnownActionFile(cnv, fil, remote);

	out_unmount:
		if (fil)
			fatfsFileClose(fil);
		(void)uiPrvCardPreUnmount();
		fatfsUnmount(vol);
		return ret;
	}

	static bool uiPrvIrButtonSpam(struct Canvas *cnv)
	{
		struct FatfsVol *vol = NULL;
		struct FatFileLocator locator;
		char fileName[UI_PICK_FILE_NAME_BUF_SZ];
		bool ret = false;

		vol = uiPrvMountCard(cnv, false);
		if (!vol)
			return false;

		if (!uiPrvPickFile(cnv, vol, "/IR", uiPrvIrRemoteFileName, "No .ir files found in /IR", true, &locator, fileName, sizeof(fileName), NULL, 0, NULL, false, NULL))
			goto out_unmount;

		ret = uiPrvIrButtonSpamLocator(cnv, vol, &locator, fileName);

	out_unmount:
		(void)uiPrvCardPreUnmount();
		fatfsUnmount(vol);
		return ret;
	}

	static bool uiPrvIrTools(struct Canvas *cnv)
	{
		while (1) {
			uint_fast8_t selOption, i;
			uint_fast8_t numRemotes = sizeof(mIrUniversalRemotes) / sizeof(*mIrUniversalRemotes);
			uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;

			uiPrvSetHeaderTitle("Universal Remote");
			uiPrvReset(cnv, false);

			for (i = 0; i < numRemotes; i++)
				uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, mIrUniversalRemotes[i].name, -1);

			selOption = uiPrvMenu(cnv, 0, numRemotes, &button);
			if (uiPrvToolExitRequested())
				return false;
			if (button == KEY_BIT_B) {
				uiPrvRequestToolExit();
				return false;
			}
			(void)uiPrvIrUniversalRemote(cnv, &mIrUniversalRemotes[selOption]);
			if (uiPrvToolExitRequested())
				return false;
		}
	}

	static bool uiPrvMusicPlayableName(const char *fname)
	{
		return uiPrvStrEndsWithNoCase(fname, ".rtttl") || uiPrvStrEndsWithNoCase(fname, ".txt") ||
			uiPrvStrEndsWithNoCase(fname, ".abc") || uiPrvStrEndsWithNoCase(fname, ".mid") ||
			uiPrvStrEndsWithNoCase(fname, ".midi");
	}

	static bool uiPrvMusicBatteryOkToLaunch(struct Canvas *cnv)
	{
		struct BadgePowerStatus status;
		bool haveStatus = badgePowerReadNow(&status);

		if (!haveStatus)
			haveStatus = badgePowerGetCached(&status);
		if (haveStatus && status.valid && status.lowBatt && status.mode != BadgePowerModeCharging) {
			audioPwmStop();
			uiAlert(cnv, "Battery too low for Music.\nConnect USB or charge before playback.", DialogTypeOk);
			return false;
		}
		return true;
	}

	struct MusicListCtx {
		struct FatfsVol *vol;
		struct MusicOption *nextAvail;
		struct MusicOption *head;
		struct MusicOption *tail;
		uint32_t spaceAvail;
		uint32_t count;
		bool overflow;
		char fname[FATFS_NAME_BUF_LEN];
	};

	static bool uiPrvMusicListAppend(struct MusicListCtx *ctx, const char *fname, uint32_t fileSz, bool isDir, const struct FatFileLocator *locator)
	{
		uint32_t nameLen = strlen(fname), spaceNeeded;
		struct MusicOption *option, *after = NULL, *before = ctx->head;

		spaceNeeded = (sizeof(struct MusicOption) + nameLen + 1 + 3) &~ 3;
		if (spaceNeeded > ctx->spaceAvail) {
			ctx->overflow = true;
			return false;
		}

		option = ctx->nextAvail;
		option->locator = *locator;
		option->size = fileSz;
		option->isDir = isDir;
		memcpy(option->name, fname, nameLen + 1);

		while (before && uiPrvFileListComesBefore(before, fname, isDir)) {
			after = before;
			before = before->next;
		}

		option->prev = after;
		option->next = before;
		if (after)
			after->next = option;
		else
			ctx->head = option;
		if (before)
			before->prev = option;
		else
			ctx->tail = option;

		ctx->nextAvail = (struct MusicOption*)(((uint8_t*)ctx->nextAvail) + spaceNeeded);
		ctx->spaceAvail -= spaceNeeded;
		ctx->count++;
		return true;
	}

	static void uiPrvListMusicDir(struct MusicListCtx *ctx, struct FatfsDir *dir)
	{
		uint32_t fileSz;
		uint8_t attrs;
		struct FatFileLocator locator;
		uint32_t seen = 0;

		while (fatfsDirRead(dir, ctx->fname, &fileSz, &attrs, &locator)) {
			if (!(++seen & 0x0f))
				badgeLedsTick();
			if ((attrs & FATFS_ATTR_VOL_LBL) || uiPrvHiddenEntry(ctx->fname, attrs))
				continue;

			if (attrs & FATFS_ATTR_DIR) {
				if (!uiPrvMusicListAppend(ctx, ctx->fname, fileSz, true, &locator))
					break;
				continue;
			}

			if (uiPrvMusicPlayableName(ctx->fname))
				if (!uiPrvMusicListAppend(ctx, ctx->fname, fileSz, false, &locator))
					break;
		}
	}

	static uint32_t uiPrvListMusic(struct FatfsVol *vol, const struct FatFileLocator *dirLoc, struct MusicOption **headP, struct MusicOption **tailP, bool *overflowP)
	{
		struct MusicListCtx ctx;
		struct FatfsDir *dir;
		struct ToolWorkspaceSpan listMem = toolWorkspaceGet(ToolWorkspaceCartRamLower);

		memset(&ctx, 0, sizeof(ctx));
		ctx.vol = vol;
		ctx.nextAvail = (struct MusicOption*)listMem.ptr;
		ctx.spaceAvail = listMem.size;

		dir = dirLoc ? fatfsDirOpenWithLocator(vol, dirLoc) : fatfsDirOpen(vol, "/MUSIC");
		if (!dir)
			goto out;

		uiPrvListMusicDir(&ctx, dir);
		fatfsDirClose(dir);

	out:
		*headP = ctx.head;
		*tailP = ctx.tail;
		if (overflowP)
			*overflowP = ctx.overflow;
		return ctx.count;
	}

	static struct MusicOption *uiPrvMusicNextSong(struct MusicOption *song)
	{
		while (song && song->isDir)
			song = song->next;
		return song;
	}

	static struct MusicOption *uiPrvMusicPrevSong(struct MusicOption *song)
	{
		while (song && song->isDir)
			song = song->prev;
		return song;
	}

	static struct MusicOption *uiPrvMusicOptionAt(struct MusicOption *head, uint32_t depth, uint32_t selectedItem)
	{
		uint32_t skip;

		if (depth && !selectedItem)
			return NULL;
		skip = selectedItem - (depth ? 1 : 0);
		while (head && skip--)
			head = head->next;
		return head;
	}

	struct MusicUiData {
		struct Canvas *cnv;
		struct Settings *settings;
		const char *name;
		uint_fast16_t prevKeys;
		uint8_t focus;
		uint8_t lastPct;
		uint32_t lastProgressFillRight;
		uint64_t lastDraw;
		bool forceDraw;
		bool lastPaused;
		bool settingsDirty;
	};

	static void uiPrvMusicSanitizeSettings(struct Settings *settings)
	{
		if (settings->audioVolume > AUDIO_PWM_VOLUME_MAX)
			settings->audioVolume = 11;
	}

	static void uiPrvMusicApplySettings(struct MusicUiData *data)
	{
		uiPrvApplyAudioSettings(data->settings);
		data->settingsDirty = true;
		data->forceDraw = true;
	}

	static void uiPrvMusicAdjustVolume(struct MusicUiData *data, int_fast8_t delta)
	{
		uint8_t volume = data->settings->audioVolume;

		if (delta < 0) {
			if (!volume)
				return;
			volume--;
		}
		else {
			if (volume >= AUDIO_PWM_VOLUME_MAX)
				return;
			volume++;
		}

		data->settings->audioVolume = volume;
		uiPrvMusicApplySettings(data);
	}

	static uint32_t uiPrvMusicProgressFillRight(struct Canvas *cnv, const struct MusicPlayerStatus *status)
	{
		uint32_t left = 10, right = cnv->w - 11;
		uint32_t bytesPlayed = status->bytesPlayed;

		if (!status->fileSize)
			return left;
		if (bytesPlayed > status->fileSize)
			bytesPlayed = status->fileSize;
		return left + (uint32_t)(((uint64_t)(right - left + 1) * bytesPlayed) / status->fileSize);
	}

	static void uiPrvMusicDrawProgress(struct Canvas *cnv, uint32_t row, uint32_t fillRight)
	{
		uint32_t left = 10, right = cnv->w - 11;
		int8_t fore = cnv->foreColor;

		cnv->foreColor = 4;
		uiPrvFillRect(cnv, left, row, right, row + 4);
		if (fillRight > left) {
			cnv->foreColor = 13;
			uiPrvFillRect(cnv, left, row, fillRight - 1, row + 4);
		}
		cnv->foreColor = fore;
	}

	static void uiPrvMusicDrawControls(struct Canvas *cnv, struct MusicUiData *data, const struct MusicPlayerStatus *status)
	{
		static const char *labels[MusicPlaybackControlNum] = {"Prev", "Play", "Next", "Loop", "Track", "Vol"};
		uint32_t cellW = cnv->w / MusicPlaybackControlNum, row = cnv->h - uiPrvGlyphHeight(cnv) - 2;
		uint_fast8_t i;
		int8_t fore = cnv->foreColor;

		for (i = 0; i < MusicPlaybackControlNum; i++) {
			char buf[20];
			const char *label = labels[i];
			uint32_t left = i * cellW, right = (i == MusicPlaybackControlNum - 1) ? cnv->w - 1 : (i + 1) * cellW - 1;
			uint32_t width;

			if (i == MusicPlaybackControlPlay)
				label = status->paused ? "Play" : "Pause";
			else if (i == MusicPlaybackControlLoop) {
				(void)sprintf(buf, data->settings->musicLoopTrack ? "Loop*" : "Loop");
				label = buf;
			}
			else if (i == MusicPlaybackControlVol) {
				(void)sprintf(buf, "Vol%u", data->settings->audioVolume);
				label = buf;
			}
			else if (i == MusicPlaybackControlTrack) {
				(void)sprintf(buf, "Track %u/%u", (unsigned)status->track + 1u, (unsigned)(status->trackCount ? status->trackCount : 1u));
				label = buf;
			}

			cnv->foreColor = i == data->focus ? 13 : 7;
			if (i == data->focus)
				uiPrvFillRect(cnv, left + 2, row - 3, right - 2, row - 1);
			cnv->foreColor = i == data->focus ? 15 : 10;
			width = uiPrvCharsWidth(cnv, label, strlen(label));
			uiPuts(cnv, row, left + (cellW > width ? (cellW - width) / 2 : 1), label, -1);
		}

		cnv->foreColor = fore;
	}

	static void uiPrvMusicDrawStatus(struct Canvas *cnv, struct MusicUiData *data, const struct MusicPlayerStatus *status, uint32_t row, uint32_t pct)
	{
		int8_t fore = cnv->foreColor;

		cnv->foreColor = 0;
		uiPrvFillRect(cnv, 10, row, cnv->w - 1, row + uiPrvGlyphHeight(cnv));
		cnv->foreColor = 15;
		uiPrintf(cnv, row, 10, "%s %u%% %uBPM", status->paused ? "Paused" : "Playing", pct,
			status->bpm);
		cnv->foreColor = fore;
	}

	static enum MusicPlayerControl uiPrvMusicControl(void *userData, const struct MusicPlayerStatus *status)
	{
		struct MusicUiData *data = (struct MusicUiData*)userData;
		uint_fast16_t keys = uiGetUiKeysRaw(), pressed = keys &~ data->prevKeys;
		uint64_t now = getTime();

		uiPrvRefreshHeaderClock(data->cnv);
		data->prevKeys = keys;
		if (pressed & UI_KEY_BIT_CENTER) {
			uiPrvRequestToolExit();
			return MusicPlayerControlStop;
		}
		if (pressed & KEY_BIT_LEFT) {
			if (data->focus)
				data->focus--;
			else
				data->focus = MusicPlaybackControlNum - 1;
			data->forceDraw = true;
		}
		if (pressed & KEY_BIT_RIGHT) {
			if (data->focus < MusicPlaybackControlNum - 1)
				data->focus++;
			else
				data->focus = 0;
			data->forceDraw = true;
		}
		if (pressed & KEY_BIT_UP) {
			if (data->focus == MusicPlaybackControlTrack)
				return MusicPlayerControlTrackNext;
			uiPrvMusicAdjustVolume(data, 1);
		}
		if (pressed & KEY_BIT_DOWN) {
			if (data->focus == MusicPlaybackControlTrack)
				return MusicPlayerControlTrackPrev;
			uiPrvMusicAdjustVolume(data, -1);
		}

		{
			struct Canvas *cnv = data->cnv;
			uint32_t row = uiPrvContentTop(cnv);
			uint32_t pct = status->fileSize ? status->bytesPlayed * 100 / status->fileSize : 0;
			uint32_t progressFillRight = uiPrvMusicProgressFillRight(cnv, status);
			bool fullDraw = data->forceDraw || data->lastPct > 100 || data->lastPaused != status->paused;
			bool progressDraw = pct != data->lastPct || progressFillRight != data->lastProgressFillRight;

			if (fullDraw || ((now - data->lastDraw > TICKS_PER_SECOND / 4) && progressDraw)) {
				uiPrvBeginPacedRedraw();
				if (fullDraw) {
					uiPrvReset(cnv, false);
					uiPrvDrawTruncText(cnv, row, 10, cnv->w - 20, data->name);
					uiPrvMusicDrawControls(cnv, data, status);
				}
				uiPrvMusicDrawStatus(cnv, data, status, row + uiPrvGlyphHeight(cnv) + 2, pct);
				uiPrvMusicDrawProgress(cnv, row + 2 * uiPrvGlyphHeight(cnv) + 8, progressFillRight);
				data->lastPct = pct;
				data->lastProgressFillRight = progressFillRight;
				data->lastPaused = status->paused;
				data->lastDraw = now;
				data->forceDraw = false;
			}
		}

		if (pressed & KEY_BIT_A) {
			if (data->focus == MusicPlaybackControlPrev)
				return MusicPlayerControlPrev;
			if (data->focus == MusicPlaybackControlPlay)
				return MusicPlayerControlPause;
			if (data->focus == MusicPlaybackControlNext)
				return MusicPlayerControlNext;
			if (data->focus == MusicPlaybackControlLoop) {
				data->settings->musicLoopTrack = !data->settings->musicLoopTrack;
				uiPrvMusicApplySettings(data);
			}
			if (data->focus == MusicPlaybackControlVol)
				uiPrvMusicAdjustVolume(data, 1);
		}
		if (pressed & KEY_BIT_B)
			return MusicPlayerControlStop;
		return MusicPlayerControlNone;
	}

	static enum MusicPlayerResult uiPrvPlayMusicLocator(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *locator, const char *name, struct Settings *settings, uint8_t *focusP)
	{
		struct FatfsFil *fil;
		struct MusicUiData data = {.cnv = cnv, .settings = settings, .name = name, .focus = focusP ? *focusP : MusicPlaybackControlPlay, .lastPct = 0xff, .forceDraw = true};
		enum MusicPlayerResult ret;

		audioPwmSetVolume(AUDIO_PWM_VOLUME_MAX);
		uiPrvApplyAudioSettings(settings);
		fil = fatfsFileOpenWithLocator(vol, locator, OPEN_MODE_READ);
		if (!fil) {
			audioPwmStop();
			uiAlert(cnv, "Cannot open music file", DialogTypeOk);
			return MusicPlayerResultStopped;
		}

		data.prevKeys = uiGetUiKeysRaw();
		data.lastDraw = getTime() - TICKS_PER_SECOND;
		bool midi = uiPrvStrEndsWithNoCase(name, ".mid") || uiPrvStrEndsWithNoCase(name, ".midi");
		if (midi)
			ret = midiPlayerPlayFile(fil, NULL, 0, uiPrvMusicControl, &data);
		else if (uiPrvStrEndsWithNoCase(name, ".abc"))
			ret = abcPlayerPlayFile(fil, uiPrvMusicControl, &data);
		else
			ret = rtttlPlayerPlayFile(fil, uiPrvMusicControl, &data);
		if (focusP)
			*focusP = data.focus;
		audioPwmStop();
		fatfsFileClose(fil);
		if (data.settingsDirty)
			settingsSet(settings);
		if (ret == MusicPlayerResultStopped)
			uiPrvWaitKeysReleased();
		if (ret == MusicPlayerResultFileError)
			uiAlert(cnv, "Music read failed", DialogTypeOk);
		else if (ret == MusicPlayerResultUnsupported)
			uiAlert(cnv, midi ? "Unsupported MIDI file" : "Unsupported ABC file", DialogTypeOk);
		else if (ret == MusicPlayerResultDecodeError)
			uiAlert(cnv, midi ? "Bad MIDI file" : (uiPrvStrEndsWithNoCase(name, ".abc") ? "Bad ABC file" : "Bad RTTTL file"), DialogTypeOk);

		return ret;
	}

	static enum MusicPlayerResult uiPrvPlayMusic(struct Canvas *cnv, struct FatfsVol *vol, struct MusicOption *song, struct Settings *settings, uint8_t *focusP)
	{
		return uiPrvPlayMusicLocator(cnv, vol, &song->locator, song->name, settings, focusP);
	}

	static void uiPrvMusicPlayer(struct Canvas *cnv)
	{
		struct FatfsVol *vol;
		struct MusicOption *head = NULL, *tail = NULL, *cur = NULL;
		struct FatFileLocator dirStack[UI_BROWSER_MAX_DEPTH];
		uint16_t pathLenStack[UI_BROWSER_MAX_DEPTH];
		char path[sizeof("/MUSIC/") + FATFS_NAME_BUF_LEN * 2];
		struct Settings settings;
		uint32_t numItems, topItem = 0, selectedItem = 0, depth = 0, prevTopItem, prevSelOnscreenItem;
		uint_fast8_t itemHeight, itemsOnscreen, pathTop, listTop, itemLeft;
		bool overflow = false, haveDirLoc = false;
		struct FontGlyphInfo gi;

		uiPrvSetHeaderTitle("Music");
		if (!uiPrvMusicBatteryOkToLaunch(cnv))
			return;
		settingsGet(&settings);
		uiPrvMusicSanitizeSettings(&settings);
		audioPwmSetVolume(AUDIO_PWM_VOLUME_MAX);
		uiPrvApplyAudioSettings(&settings);

		vol = uiPrvMountCard(cnv, false);
		if (!vol)
			return;
		if (!toolWorkspaceAcquire(ToolWorkspaceCartRamLower, ToolWorkspaceOwnerMusic, NULL)) {
			uiAlert(cnv, "Tool workspace is busy; cannot list music", DialogTypeOk);
			goto out_unmount;
		}

		strcpy(path, "/MUSIC");
reload_dir:
		numItems = uiPrvListMusic(vol, haveDirLoc ? &dirStack[depth - 1] : NULL, &head, &tail, &overflow);
		if (!numItems && !depth) {
			uiAlert(cnv, "No music files found in /MUSIC", DialogTypeOk);
			goto out_unmount;
		}
		if (overflow)
			uiAlert(cnv, "Folder has too many entries; showing what fits", DialogTypeOk);

		cur = head;
		topItem = 0;
		selectedItem = 0;
		itemHeight = uiPrvGlyphHeight(cnv) + 1;
		itemLeft = fontGetGlyphInfo(&gi, cnv->font, MENU_SELECTION_CHAR) ? gi.width + 2 : 10;
		pathTop = uiPrvContentTop(cnv);
		listTop = pathTop + itemHeight;
		itemsOnscreen = (cnv->h - listTop - 2u * itemHeight) / itemHeight;
		if (!itemsOnscreen) {
			uiAlert(cnv, "Display area too small for music browser", DialogTypeOk);
			goto out_unmount;
		}
		if (itemsOnscreen > numItems + (depth ? 1 : 0))
			itemsOnscreen = numItems + (depth ? 1 : 0);
		prevTopItem = topItem + 1;
		prevSelOnscreenItem = selectedItem - topItem + 1;

		while (1) {
			struct MusicOption *draw = head;
			uint32_t i, totalItems = numItems + (depth ? 1 : 0), selectedOnscreenItem = selectedItem - topItem;

			cur = uiPrvMusicOptionAt(head, depth, selectedItem);
			if (prevTopItem != topItem) {
				uint_fast8_t firstRow = 0, scrollWidth;
				uint32_t skipItems;

				uiPrvSetHeaderTitle("Music");
				uiPrvReset(cnv, false);
				uiPrvDrawTruncText(cnv, pathTop, 10, cnv->w - 10, path);
				scrollWidth = totalItems > itemsOnscreen ? uiPrvDrawScrollbar(cnv, listTop, totalItems, topItem, itemsOnscreen) : 0;

				if (depth && !topItem) {
					cnv->foreColor = 12;
					uiPrvDrawDirLabel(cnv, listTop, itemLeft, cnv->w - scrollWidth - itemLeft, "...");
					firstRow = 1;
					skipItems = 0;
				}
				else
					skipItems = topItem - (depth ? 1 : 0);

				for (i = 0; i < skipItems && draw; i++)
					draw = draw->next;

				cnv->foreColor = 12;
				for (i = firstRow; i < itemsOnscreen && draw; i++, draw = draw->next) {
					if (draw->isDir)
						uiPrvDrawDirLabel(cnv, listTop + i * itemHeight, itemLeft, cnv->w - scrollWidth - itemLeft, draw->name);
					else
						uiPrvDrawTruncText(cnv, listTop + i * itemHeight, itemLeft, cnv->w - scrollWidth - itemLeft, draw->name);
				}
				uiPrvDrawMenuFooter(cnv, KEY_BIT_A | KEY_BIT_B);

				prevSelOnscreenItem = selectedOnscreenItem + 1;
			}
			prevTopItem = topItem;
			if (prevSelOnscreenItem != selectedOnscreenItem) {
				if (prevSelOnscreenItem < itemsOnscreen) {
					cnv->foreColor = 0;
					uiPrvDrawOneChar(cnv, listTop + itemHeight * prevSelOnscreenItem, 1, MENU_SELECTION_CHAR);
				}
				cnv->foreColor = 15;
				uiPrvDrawOneChar(cnv, listTop + itemHeight * selectedOnscreenItem, 1, MENU_SELECTION_CHAR);
			}
			prevSelOnscreenItem = selectedOnscreenItem;

			switch (uiPrvRecvMenuKeypress(cnv)) {
				case UI_KEY_BIT_CENTER:
					uiPrvRequestToolExit();
					goto out_unmount;

				case KEY_BIT_A:
					if (depth && selectedItem == 0) {
						depth--;
						haveDirLoc = depth != 0;
						path[pathLenStack[depth]] = 0;
						goto reload_dir;
					}
					if (cur && cur->isDir) {
						if (depth < UI_BROWSER_MAX_DEPTH) {
							pathLenStack[depth] = strlen(path);
							dirStack[depth++] = cur->locator;
							haveDirLoc = true;
							uiPrvAppendPathComponent(path, sizeof(path), cur->name);
							goto reload_dir;
						}
						uiAlert(cnv, "Folder nesting too deep", DialogTypeOk);
						prevTopItem = topItem + 1;
						break;
					}
					{
						uint8_t playbackFocus = MusicPlaybackControlPlay;

						while (cur && !cur->isDir) {
							enum MusicPlayerResult playRet = uiPrvPlayMusic(cnv, vol, cur, &settings, &playbackFocus);

							if (playRet == MusicPlayerResultDone && settings.musicLoopTrack)
								continue;

							if ((playRet == MusicPlayerResultNext || playRet == MusicPlayerResultDone) && uiPrvMusicNextSong(cur->next)) {
								do {
									cur = cur->next;
									selectedItem++;
								} while (cur && cur->isDir);
							}
							else if (playRet == MusicPlayerResultPrev && uiPrvMusicPrevSong(cur->prev)) {
								do {
									cur = cur->prev;
									selectedItem--;
								} while (cur && cur->isDir);
							}
							else {
								break;
							}

							if (selectedItem < topItem)
								topItem = selectedItem;
							if (selectedItem >= topItem + itemsOnscreen)
								topItem = selectedItem + 1 - itemsOnscreen;
						}
					}
					prevTopItem = topItem + 1;
					break;

				case KEY_BIT_B:
					if (depth) {
						depth--;
						haveDirLoc = depth != 0;
						path[pathLenStack[depth]] = 0;
						goto reload_dir;
					}
					uiPrvRequestToolExit();
					goto out_unmount;

				case KEY_BIT_DOWN:
					if (selectedItem + 1 < totalItems) {
						selectedItem++;
						if (selectedItem >= topItem + itemsOnscreen) {
							topItem += itemsOnscreen;
							if (topItem + itemsOnscreen > totalItems)
								topItem = totalItems > itemsOnscreen ? totalItems - itemsOnscreen : 0;
						}
						cur = uiPrvMusicOptionAt(head, depth, selectedItem);
					}
					else if (!mUiKeyRepeated && totalItems) {
						selectedItem = 0;
						topItem = 0;
						cur = uiPrvMusicOptionAt(head, depth, selectedItem);
					}
					break;

				case KEY_BIT_UP:
					if (selectedItem) {
						selectedItem--;
						if (selectedItem < topItem) {
							if (topItem > itemsOnscreen)
								topItem -= itemsOnscreen;
							else
								topItem = 0;
							if (selectedItem < topItem)
								topItem = selectedItem;
						}
						cur = uiPrvMusicOptionAt(head, depth, selectedItem);
					}
					else if (!mUiKeyRepeated && totalItems) {
						selectedItem = totalItems - 1;
						topItem = totalItems > itemsOnscreen ? totalItems - itemsOnscreen : 0;
						cur = uiPrvMusicOptionAt(head, depth, selectedItem);
					}
					break;

				case KEY_BIT_RIGHT:
					uiPrvPageSelection(&selectedItem, &topItem, totalItems, itemsOnscreen, true);
					cur = uiPrvMusicOptionAt(head, depth, selectedItem);
					break;

				case KEY_BIT_LEFT:
					uiPrvPageSelection(&selectedItem, &topItem, totalItems, itemsOnscreen, false);
					cur = uiPrvMusicOptionAt(head, depth, selectedItem);
					break;
			}
		}

	out_unmount:
		toolWorkspaceRelease(ToolWorkspaceCartRamLower, ToolWorkspaceOwnerMusic);
		(void)uiPrvCardPreUnmount();
		fatfsUnmount(vol);
	}

	struct BadUsbUiData {
		struct Canvas *cnv;
		const char *name;
		uint32_t lastLine;
		uint32_t lastProgressPct;
		uint32_t lastDelayRemainSec;
		enum BadUsbWorkerState lastState;
		uint32_t lastUnsupportedCommands;
		struct BadUsbStatus lastStatus;
		bool forceDraw;
		bool haveStatus;
	};

	#define BADUSB_UI_ENUM_WAIT_MS	5000
	#define BADUSB_UI_KEY_RELEASE_WAIT_MS 300

	static const char *uiPrvBadUsbStateName(enum BadUsbWorkerState state)
	{
		switch (state) {
			case BadUsbStateInit: return "Init";
			case BadUsbStateNotConnected: return "Connect USB";
			case BadUsbStateIdle: return "Ready";
			case BadUsbStateWillRun: return "Starting";
			case BadUsbStateRunning: return "Running";
			case BadUsbStateDelay: return "Delay";
			case BadUsbStatePaused: return "Paused";
			case BadUsbStateWaitForButton: return "Waiting";
			case BadUsbStateDone: return "Done";
			case BadUsbStateScriptError: return "Script error";
			case BadUsbStateFileError: return "File error";
			case BadUsbStateUsbError: return "USB error";
			default: return "BadUSB";
		}
	}

	static uint32_t uiPrvBadUsbProgressPct(const struct BadUsbStatus *status)
	{
		uint32_t pct;

		if (status->state == BadUsbStateDone)
			return 100;
		if (status->state == BadUsbStateInit && status->fileSize)
			pct = status->bytesRead >= status->fileSize ? 100 : status->bytesRead * 100 / status->fileSize;
		else if (status->lineTotal)
			pct = status->lineNo >= status->lineTotal ? 100 : status->lineNo * 100 / status->lineTotal;
		else if (status->fileSize)
			pct = status->bytesRead >= status->fileSize ? 100 : status->bytesRead * 100 / status->fileSize;
		else
			pct = 0;
		return pct > 100 ? 100 : pct;
	}

	static bool uiPrvBadUsbStatusSane(const struct BadUsbStatus *status)
	{
		if (!status)
			return false;
		if (status->fileSize && status->bytesRead > status->fileSize)
			return false;
		if (status->fileSize && status->lineTotal > status->fileSize + 1)
			return false;
		if (status->lineTotal)
			return status->lineNo <= status->lineTotal;
		return true;
	}

	static void uiPrvBadUsbDrawStatus(struct BadUsbUiData *data, const struct BadUsbStatus *status, enum BadUsbWorkerState state, const char *message)
	{
		struct Canvas *cnv = data->cnv;
		uint32_t row, pct = uiPrvBadUsbProgressPct(status);
		char msg[96];

		uiPrvSetHeaderTitle("BadUSB");
		uiPrvReset(cnv, false);
		cnv->font = FontMedium;
		row = uiPrvContentTop(cnv);
		uiPrvDrawTruncText(cnv, row, 10, cnv->w - 20, data->name);
		row += uiPrvGlyphHeight(cnv) + 1;
		(void)sprintf(msg, "%s  %u%%", uiPrvBadUsbStateName(state), (unsigned)pct);
		uiPrvDrawTruncText(cnv, row, 10, cnv->w - 20, msg);
		row += uiPrvGlyphHeight(cnv) + 1;
		if (state == BadUsbStateInit)
			(void)sprintf(msg, "Lines %u", (unsigned)status->lineTotal);
		else if (status->lineTotal)
			(void)sprintf(msg, "Line %u/%u", (unsigned)status->lineNo, (unsigned)status->lineTotal);
		else
			(void)sprintf(msg, "Line %u", (unsigned)status->lineNo);
		uiPrvDrawTruncText(cnv, row, 10, cnv->w - 20, msg);
		if (status->delayRemainSec)
			(void)sprintf(msg, "%us", (unsigned)status->delayRemainSec);
		if (status->delayRemainSec)
			uiPuts(cnv, row, cnv->w - 45, msg, -1);
		row += uiPrvGlyphHeight(cnv) + 1;

		if (state == BadUsbStateScriptError && status->error[0]) {
			(void)sprintf(msg, "L%u: %s", (unsigned)status->errorLine, status->error);
			uiPrvDrawTruncText(cnv, row, 10, cnv->w - 20, msg);
		}
		else if (status->unsupportedCommands) {
			(void)sprintf(msg, "Warn %u: %s L%u", (unsigned)status->unsupportedCommands, status->lastUnsupportedCommand, (unsigned)status->lastUnsupportedLine);
			uiPrvDrawTruncText(cnv, row, 10, cnv->w - 20, msg);
		}
		else if (message)
			uiPrvDrawTruncText(cnv, row, 10, cnv->w - 20, message);

		if (state == BadUsbStatePaused)
			uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "A = Resume  B = Stop", -1);
		else if (state == BadUsbStateRunning || state == BadUsbStateDelay)
			uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "A = Pause  B = Stop", -1);
		else if (state == BadUsbStateWaitForButton)
			uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "A = Continue  B = Stop", -1);
		else
			uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "Hold B to cancel", -1);
	}

	static bool uiPrvBadUsbPause(struct BadUsbUiData *data, const struct BadUsbStatus *status)
	{
		struct BadUsbStatus pausedStatus = *status;

		pausedStatus.state = BadUsbStatePaused;
		pausedStatus.message = "Paused";
		uiPrvWaitKeysReleased();
		while (1) {
			uint_fast16_t key;

			uiPrvBadUsbDrawStatus(data, &pausedStatus, BadUsbStatePaused, "Paused");
			key = uiPrvRecvKeypress();
			if (key & KEY_BIT_A) {
				data->forceDraw = true;
				return true;
			}
			if (key & UI_KEY_BIT_CENTER) {
				uiPrvRequestToolExit();
				return false;
			}
			if (key & KEY_BIT_B)
				return false;
		}
	}

	static void uiPrvBadUsbWaitKeysReleasedBounded(struct Canvas *cnv)
	{
		uint64_t end = getTime() + (uint64_t)BADUSB_UI_KEY_RELEASE_WAIT_MS * (TICKS_PER_SECOND / 1000);

		while (uiGetUiKeys() && getTime() < end)
			uiPrvRefreshHeaderClock(cnv);
	}

	static bool uiPrvBadUsbStatus(void *userData, const struct BadUsbStatus *status)
	{
		struct BadUsbUiData *data = (struct BadUsbUiData*)userData;
		struct BadUsbStatus badStatus;
		uiPrvRefreshHeaderClock(data->cnv);
		if (uiPrvCenterExitPressedRaw())
			return false;
		if (uiGetKeysRaw() & KEY_BIT_B)
			return false;
		if (!uiPrvBadUsbStatusSane(status)) {
			memset(&badStatus, 0, sizeof(badStatus));
			badStatus.state = BadUsbStateScriptError;
			badStatus.message = "Status invalid";
			strcpy(badStatus.error, "Status invalid");
			uiPrvBeginPacedRedraw();
			uiPrvBadUsbDrawStatus(data, &badStatus, BadUsbStateScriptError, "Status invalid");
			return false;
		}
		data->lastStatus = *status;
		data->haveStatus = true;
		if ((status->state == BadUsbStateRunning || status->state == BadUsbStateDelay) && (uiGetKeysRaw() & KEY_BIT_A))
			return uiPrvBadUsbPause(data, &data->lastStatus);

		if (data->forceDraw || status->lineNo != data->lastLine || status->state != data->lastState ||
				uiPrvBadUsbProgressPct(status) != data->lastProgressPct ||
				status->delayRemainSec != data->lastDelayRemainSec ||
				status->unsupportedCommands != data->lastUnsupportedCommands) {
			uiPrvBeginPacedRedraw();
			uiPrvBadUsbDrawStatus(data, &data->lastStatus, status->state, status->message);
			data->lastLine = status->lineNo;
			data->lastProgressPct = uiPrvBadUsbProgressPct(status);
			data->lastDelayRemainSec = status->delayRemainSec;
			data->lastState = status->state;
			data->lastUnsupportedCommands = status->unsupportedCommands;
			data->forceDraw = false;
		}
		return true;
	}

	static bool uiPrvBadUsbWaitButton(void *userData, const struct BadUsbStatus *status)
	{
		struct BadUsbUiData *data = (struct BadUsbUiData*)userData;
		struct BadUsbStatus waitStatus = *status;

		waitStatus.state = BadUsbStateWaitForButton;
		waitStatus.message = "Waiting for button";
		uiPrvBadUsbDrawStatus(data, &waitStatus, BadUsbStateWaitForButton, "Waiting for button");
		while (1) {
			uint_fast16_t key = uiPrvRecvKeypress();

			if (key & KEY_BIT_A) {
				data->forceDraw = true;
				return true;
			}
			if (key & UI_KEY_BIT_CENTER) {
				uiPrvRequestToolExit();
				return false;
			}
			if (key & KEY_BIT_B)
				return false;
		}
	}

	static bool uiPrvBadUsbFileName(const char *fname)
	{
		return uiPrvStrEndsWithNoCase(fname, ".txt") || uiPrvStrEndsWithNoCase(fname, ".badusb");
	}

	static void uiPrvBadUsbShowState(struct BadUsbUiData *data, const struct BadUsbPreload *preload, enum BadUsbWorkerState state, const char *message)
	{
		struct BadUsbStatus status;

		if (data->haveStatus)
			status = data->lastStatus;
		else
			memset(&status, 0, sizeof(status));
		if (preload) {
			status.fileSize = preload->fileSize;
			status.lineTotal = preload->lineTotal;
		}
		status.state = state;
		status.message = message;
		status.delayRemainSec = 0;
		uiPrvBadUsbDrawStatus(data, &status, state, message);
		data->lastStatus = status;
		data->haveStatus = true;
		data->lastState = state;
	}

	static void uiPrvBadUsbPreloadForDisplay(struct BadUsbPreload *preload, struct FatfsFil *fil, const struct UsbHidDeviceInfo *info)
	{
		memset(preload, 0, sizeof(*preload));
		if (info)
			preload->info = *info;
		else
			usbHidDefaultInfo(&preload->info);
		preload->fileSize = fatfsFileGetSize(fil);
	}

	static bool uiPrvBadUsbWaitReady(struct BadUsbUiData *data, const struct BadUsbPreload *preload)
	{
		uint64_t end = getTime() + (uint64_t)BADUSB_UI_ENUM_WAIT_MS * (TICKS_PER_SECOND / 1000);

		uiPrvBeginPacedRedraw();
		uiPrvBadUsbShowState(data, preload, BadUsbStateNotConnected, "Waiting for USB");

		while (getTime() < end) {
			usbHidTask();
			uiPrvRefreshHeaderClock(data->cnv);
			uiPrvRefreshHeaderClock(data->cnv);
			if (uiPrvCenterExitPressedRaw()) {
				uiPrvRequestToolExit();
				return false;
			}
			if (uiGetKeysRaw() & KEY_BIT_B)
				return false;
			if (usbHidReady())
				return true;
		}
		return usbHidReady();
	}

	static bool uiPrvRunBadUsbLocator(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *locator, const char *name)
	{
		struct FatfsFil *fil = NULL;
		struct BadUsbUiData data;
		struct BadUsbPreload preload;
		struct UsbHidDeviceInfo hidInfo;
		struct UsbHidDeviceInfo defaultHidInfo;
		struct Settings settings;
		struct ToolWorkspaceSpan scratchMem = {0};
		enum BadUsbResult ret = BadUsbResultFileError;
		char msg[96];
		bool ok = false, reportsEnabled = false, usbStarted = false, scratchAcquired = false, reportResult = false;

		uiPrvSetHeaderTitle("BadUSB");
		fil = fatfsFileOpenWithLocator(vol, locator, OPEN_MODE_READ);
		if (!fil) {
			uiAlert(cnv, "Cannot open BadUSB script", DialogTypeOk);
			return false;
		}

		(void)sprintf(msg, "Run BadUSB script?\n%s", name);
		if (!uiAlert(cnv, msg, DialogTypeYesNo))
			goto out_close;

		memset(&data, 0, sizeof(data));
		data.cnv = cnv;
		data.name = name;
		data.forceDraw = true;
		uiPrvBadUsbWaitKeysReleasedBounded(cnv);

		scratchAcquired = toolWorkspaceAcquire(ToolWorkspaceWram, ToolWorkspaceOwnerBadUsb, &scratchMem);
		if (!scratchAcquired || scratchMem.size < badUsbScratchSize()) {
			if (scratchAcquired) {
				toolWorkspaceRelease(ToolWorkspaceWram, ToolWorkspaceOwnerBadUsb);
				scratchAcquired = false;
			}
			uiAlert(cnv, "Tool workspace is too small for BadUSB", DialogTypeOk);
			goto out_close;
		}

		settingsGet(&settings);
		memset(&defaultHidInfo, 0, sizeof(defaultHidInfo));
		defaultHidInfo.vid = settings.badUsbVid;
		defaultHidInfo.pid = settings.badUsbPid;
		strcpy(defaultHidInfo.manufacturer, settings.badUsbManufacturer);
		strcpy(defaultHidInfo.product, settings.badUsbProduct);

		reportResult = true;
		if (!badUsbReadDeviceInfoWithDefaultScratch(fil, &defaultHidInfo, &hidInfo, scratchMem.ptr, scratchMem.size))
			goto out_close;
		uiPrvBadUsbPreloadForDisplay(&preload, fil, &hidInfo);

		uiPrvBadUsbShowState(&data, &preload, BadUsbStateWillRun, "Starting USB");
		if (!usbHidBeginReportSet(&hidInfo, UsbHidReportSetKeyboardMouseConsumer)) {
			struct BadUsbStatus usbStatus;

			memset(&usbStatus, 0, sizeof(usbStatus));
			usbStatus.state = BadUsbStateUsbError;
			usbStatus.fileSize = preload.fileSize;
			usbStatus.lineTotal = preload.lineTotal;
			usbStatus.message = usbHidLastError();
			uiPrvBadUsbDrawStatus(&data, &usbStatus, BadUsbStateUsbError, usbHidLastError());
			ret = BadUsbResultUsbError;
			goto out_close;
		}
		usbStarted = true;
		if (!uiPrvBadUsbWaitReady(&data, &preload)) {
			ret = (uiGetKeysRaw() & KEY_BIT_B) || uiPrvToolExitRequested() ? BadUsbResultCancelled : BadUsbResultUsbError;
			goto out_usb;
		}
		usbHidSetReportsEnabled(true);
		reportsEnabled = true;
		uiPrvBadUsbWaitKeysReleasedBounded(cnv);
		uiPrvBadUsbShowState(&data, &preload, BadUsbStateRunning, "Running");
		ret = badUsbRunPreparedFileWithScratch(fil, NULL, uiPrvBadUsbStatus, uiPrvBadUsbWaitButton, &data, scratchMem.ptr, scratchMem.size);

	out_usb:
		if (reportsEnabled) {
			/* BadUSB EOF/error cleanup must prefer immediate detach over a final release report. */
			reportsEnabled = false;
		}
		if (usbStarted) {
			usbHidDropNow();
			usbStarted = false;
		}

	out_close:
		if (scratchAcquired) {
			toolWorkspaceRelease(ToolWorkspaceWram, ToolWorkspaceOwnerBadUsb);
			scratchAcquired = false;
		}
		if (fil) {
			fatfsFileClose(fil);
			fil = NULL;
		}
		if (!reportResult)
			return ok;

		if (ret == BadUsbResultDone) {
			uiAlert(cnv, "BadUSB script complete", DialogTypeOk);
			ok = true;
		}
		else if (ret == BadUsbResultCancelled) {
			if (!uiPrvToolExitRequested())
				uiAlert(cnv, "BadUSB script cancelled", DialogTypeOk);
		}
		else if (ret == BadUsbResultUsbError)
			uiAlert(cnv, "BadUSB USB device failed to enumerate", DialogTypeOk);
		else if (ret == BadUsbResultFileError)
			uiAlert(cnv, "BadUSB script read failed", DialogTypeOk);
		else
			uiAlert(cnv, "BadUSB script has an unsupported or malformed command", DialogTypeOk);
		return ok;
	}

	static bool uiPrvBadUsbTool(struct Canvas *cnv)
	{
		struct FatfsVol *vol;
		struct FatFileLocator locator;
		char name[UI_PICK_FILE_NAME_BUF_SZ];
		bool ok = false;

		uiPrvSetHeaderTitle("BadUSB");
		vol = uiPrvMountCard(cnv, false);
		if (!vol)
			return false;

		while (!uiPrvToolExitRequested()) {
			if (!uiPrvPickFile(cnv, vol, "/BADUSB", uiPrvBadUsbFileName, "No BadUSB scripts found in /BADUSB", false, &locator, name, sizeof(name), NULL, 0, NULL, false, NULL))
				break;

			ok = uiPrvRunBadUsbLocator(cnv, vol, &locator, name) || ok;
		}

		(void)uiPrvCardPreUnmount();
		fatfsUnmount(vol);
		return ok;
	}

#endif //NO_SD_CARD

#ifdef NO_SD_CARD
bool uiFlushCurrentSaveToCard(bool force)
{
	(void)force;
	return uiSaveSavestate();
}
#endif

#define USB_KEYBOARD_ENUM_WAIT_MS	5000
#define USB_KEYBOARD_KEY_DELAY_MS	12

enum UiUsbKeyboardAction {
	UiUsbKeyboardActionKey,
	UiUsbKeyboardActionString,
	UiUsbKeyboardActionReleaseAll,
};

enum UiUsbKeyboardCategory {
	UiUsbKeyboardCategoryBasic,
	UiUsbKeyboardCategoryNavigation,
	UiUsbKeyboardCategoryEditing,
	UiUsbKeyboardCategoryWindows,
	UiUsbKeyboardCategoryLinux,
	UiUsbKeyboardCategoryMac,
	UiUsbKeyboardCategoryBrowser,
	UiUsbKeyboardCategoryMedia,
	UiUsbKeyboardCategoryPresentation,
	UiUsbKeyboardCategoryFunction,
	UiUsbKeyboardCategoryType,
	UiUsbKeyboardCategoryNum,
};

struct UiUsbKeyboardCategoryInfo {
	const char *label;
};

struct UiUsbKeyboardCommand {
	enum UiUsbKeyboardCategory category;
	const char *label;
	enum UiUsbKeyboardAction action;
	uint8_t mods;
	uint8_t usage;
	const char *text;
};

static const struct UiUsbKeyboardCategoryInfo mUsbKeyboardCategories[UiUsbKeyboardCategoryNum] = {
	[UiUsbKeyboardCategoryBasic] = {"Basic Keys"},
	[UiUsbKeyboardCategoryNavigation] = {"Navigation"},
	[UiUsbKeyboardCategoryEditing] = {"Editing"},
	[UiUsbKeyboardCategoryWindows] = {"Windows"},
	[UiUsbKeyboardCategoryLinux] = {"Linux"},
	[UiUsbKeyboardCategoryMac] = {"Mac"},
	[UiUsbKeyboardCategoryBrowser] = {"Browser"},
	[UiUsbKeyboardCategoryMedia] = {"Media / YouTube"},
	[UiUsbKeyboardCategoryPresentation] = {"Presentation"},
	[UiUsbKeyboardCategoryFunction] = {"Function Keys"},
	[UiUsbKeyboardCategoryType] = {"Type/Test"},
};

static const struct UiUsbKeyboardCommand mUsbKeyboardCommands[] = {
	{UiUsbKeyboardCategoryBasic, "Release all keys", UiUsbKeyboardActionReleaseAll, 0, 0, NULL},
	{UiUsbKeyboardCategoryBasic, "Enter", UiUsbKeyboardActionKey, 0, 0x28, NULL},
	{UiUsbKeyboardCategoryBasic, "Tab", UiUsbKeyboardActionKey, 0, 0x2b, NULL},
	{UiUsbKeyboardCategoryBasic, "Esc", UiUsbKeyboardActionKey, 0, 0x29, NULL},
	{UiUsbKeyboardCategoryBasic, "Backspace", UiUsbKeyboardActionKey, 0, 0x2a, NULL},
	{UiUsbKeyboardCategoryBasic, "Delete", UiUsbKeyboardActionKey, 0, 0x4c, NULL},
	{UiUsbKeyboardCategoryBasic, "Space", UiUsbKeyboardActionKey, 0, 0x2c, NULL},
	{UiUsbKeyboardCategoryBasic, "Menu / App key", UiUsbKeyboardActionKey, 0, 0x65, NULL},

	{UiUsbKeyboardCategoryNavigation, "Up", UiUsbKeyboardActionKey, 0, 0x52, NULL},
	{UiUsbKeyboardCategoryNavigation, "Down", UiUsbKeyboardActionKey, 0, 0x51, NULL},
	{UiUsbKeyboardCategoryNavigation, "Left", UiUsbKeyboardActionKey, 0, 0x50, NULL},
	{UiUsbKeyboardCategoryNavigation, "Right", UiUsbKeyboardActionKey, 0, 0x4f, NULL},
	{UiUsbKeyboardCategoryNavigation, "Home", UiUsbKeyboardActionKey, 0, 0x4a, NULL},
	{UiUsbKeyboardCategoryNavigation, "End", UiUsbKeyboardActionKey, 0, 0x4d, NULL},
	{UiUsbKeyboardCategoryNavigation, "Page Up", UiUsbKeyboardActionKey, 0, 0x4b, NULL},
	{UiUsbKeyboardCategoryNavigation, "Page Down", UiUsbKeyboardActionKey, 0, 0x4e, NULL},

	{UiUsbKeyboardCategoryEditing, "Copy (Ctrl+C)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x06, NULL},
	{UiUsbKeyboardCategoryEditing, "Paste (Ctrl+V)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x19, NULL},
	{UiUsbKeyboardCategoryEditing, "Cut (Ctrl+X)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x1b, NULL},
	{UiUsbKeyboardCategoryEditing, "Undo (Ctrl+Z)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x1d, NULL},
	{UiUsbKeyboardCategoryEditing, "Redo (Ctrl+Y)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x1c, NULL},
	{UiUsbKeyboardCategoryEditing, "Select All (Ctrl+A)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x04, NULL},
	{UiUsbKeyboardCategoryEditing, "Find (Ctrl+F)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x09, NULL},
	{UiUsbKeyboardCategoryEditing, "New (Ctrl+N)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x11, NULL},
	{UiUsbKeyboardCategoryEditing, "Open (Ctrl+O)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x12, NULL},
	{UiUsbKeyboardCategoryEditing, "Save (Ctrl+S)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x16, NULL},
	{UiUsbKeyboardCategoryEditing, "Print (Ctrl+P)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x13, NULL},
	{UiUsbKeyboardCategoryEditing, "Paste Plain (Ctrl+Shift+V)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL | USB_HID_MOD_LSHIFT, 0x19, NULL},

	{UiUsbKeyboardCategoryWindows, "Run (Win+R)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x15, NULL},
	{UiUsbKeyboardCategoryWindows, "Explorer (Win+E)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x08, NULL},
	{UiUsbKeyboardCategoryWindows, "Show Desktop (Win+D)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x07, NULL},
	{UiUsbKeyboardCategoryWindows, "Lock (Win+L)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x0f, NULL},
	{UiUsbKeyboardCategoryWindows, "Search (Win+S)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x16, NULL},
	{UiUsbKeyboardCategoryWindows, "Settings (Win+I)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x0c, NULL},
	{UiUsbKeyboardCategoryWindows, "Power User (Win+X)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x1b, NULL},
	{UiUsbKeyboardCategoryWindows, "Clipboard (Win+V)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x19, NULL},
	{UiUsbKeyboardCategoryWindows, "Emoji Panel (Win+.)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x37, NULL},
	{UiUsbKeyboardCategoryWindows, "Project Display (Win+P)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x13, NULL},
	{UiUsbKeyboardCategoryWindows, "Task Switch (Alt+Tab)", UiUsbKeyboardActionKey, USB_HID_MOD_LALT, 0x2b, NULL},
	{UiUsbKeyboardCategoryWindows, "Task View (Win+Tab)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x2b, NULL},
	{UiUsbKeyboardCategoryWindows, "Task Manager (Ctrl+Shift+Esc)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL | USB_HID_MOD_LSHIFT, 0x29, NULL},
	{UiUsbKeyboardCategoryWindows, "Close App (Alt+F4)", UiUsbKeyboardActionKey, USB_HID_MOD_LALT, 0x3d, NULL},
	{UiUsbKeyboardCategoryWindows, "Ctrl+Alt+Del", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL | USB_HID_MOD_LALT, 0x4c, NULL},
	{UiUsbKeyboardCategoryWindows, "Snipping (Win+Shift+S)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI | USB_HID_MOD_LSHIFT, 0x16, NULL},
	{UiUsbKeyboardCategoryWindows, "Print Screen", UiUsbKeyboardActionKey, 0, 0x46, NULL},
	{UiUsbKeyboardCategoryWindows, "Active Screenshot (Alt+PrtSc)", UiUsbKeyboardActionKey, USB_HID_MOD_LALT, 0x46, NULL},
	{UiUsbKeyboardCategoryWindows, "Reset Graphics (Win+Ctrl+Shift+B)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI | USB_HID_MOD_LCTRL | USB_HID_MOD_LSHIFT, 0x05, NULL},

	{UiUsbKeyboardCategoryLinux, "Terminal (Ctrl+Alt+T)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL | USB_HID_MOD_LALT, 0x17, NULL},
	{UiUsbKeyboardCategoryLinux, "Lock (Ctrl+Alt+L)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL | USB_HID_MOD_LALT, 0x0f, NULL},

	{UiUsbKeyboardCategoryMac, "Spotlight (Cmd+Space)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x2c, NULL},
	{UiUsbKeyboardCategoryMac, "App Switch (Cmd+Tab)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x2b, NULL},
	{UiUsbKeyboardCategoryMac, "Force Quit (Cmd+Opt+Esc)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI | USB_HID_MOD_LALT, 0x29, NULL},
	{UiUsbKeyboardCategoryMac, "Hide App (Cmd+H)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x0b, NULL},
	{UiUsbKeyboardCategoryMac, "Quit App (Cmd+Q)", UiUsbKeyboardActionKey, USB_HID_MOD_LGUI, 0x14, NULL},

	{UiUsbKeyboardCategoryBrowser, "Address Bar (Ctrl+L)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x0f, NULL},
	{UiUsbKeyboardCategoryBrowser, "New Tab (Ctrl+T)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x17, NULL},
	{UiUsbKeyboardCategoryBrowser, "Close Tab (Ctrl+W)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x1a, NULL},
	{UiUsbKeyboardCategoryBrowser, "Reopen Tab (Ctrl+Shift+T)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL | USB_HID_MOD_LSHIFT, 0x17, NULL},
	{UiUsbKeyboardCategoryBrowser, "Private Window (Ctrl+Shift+N)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL | USB_HID_MOD_LSHIFT, 0x11, NULL},
	{UiUsbKeyboardCategoryBrowser, "Refresh (Ctrl+R)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x15, NULL},
	{UiUsbKeyboardCategoryBrowser, "Full Screen (F11)", UiUsbKeyboardActionKey, 0, 0x44, NULL},
	{UiUsbKeyboardCategoryBrowser, "Back (Alt+Left)", UiUsbKeyboardActionKey, USB_HID_MOD_LALT, 0x50, NULL},
	{UiUsbKeyboardCategoryBrowser, "Forward (Alt+Right)", UiUsbKeyboardActionKey, USB_HID_MOD_LALT, 0x4f, NULL},
	{UiUsbKeyboardCategoryBrowser, "Next Tab (Ctrl+Tab)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x2b, NULL},
	{UiUsbKeyboardCategoryBrowser, "Prev Tab (Ctrl+Shift+Tab)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL | USB_HID_MOD_LSHIFT, 0x2b, NULL},
	{UiUsbKeyboardCategoryBrowser, "Zoom In (Ctrl+=)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x2e, NULL},
	{UiUsbKeyboardCategoryBrowser, "Zoom Out (Ctrl+-)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x2d, NULL},
	{UiUsbKeyboardCategoryBrowser, "Zoom Reset (Ctrl+0)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL, 0x27, NULL},
	{UiUsbKeyboardCategoryBrowser, "Dev Tools (Ctrl+Shift+I)", UiUsbKeyboardActionKey, USB_HID_MOD_LCTRL | USB_HID_MOD_LSHIFT, 0x0c, NULL},

	{UiUsbKeyboardCategoryMedia, "Play/Pause (Space)", UiUsbKeyboardActionKey, 0, 0x2c, NULL},
	{UiUsbKeyboardCategoryMedia, "YouTube Play/Pause (K)", UiUsbKeyboardActionKey, 0, 0x0e, NULL},
	{UiUsbKeyboardCategoryMedia, "YouTube Mute (M)", UiUsbKeyboardActionKey, 0, 0x10, NULL},
	{UiUsbKeyboardCategoryMedia, "YouTube Fullscreen (F)", UiUsbKeyboardActionKey, 0, 0x09, NULL},
	{UiUsbKeyboardCategoryMedia, "YouTube Captions (C)", UiUsbKeyboardActionKey, 0, 0x06, NULL},
	{UiUsbKeyboardCategoryMedia, "YouTube Vol Up (Up)", UiUsbKeyboardActionKey, 0, 0x52, NULL},
	{UiUsbKeyboardCategoryMedia, "YouTube Vol Down (Down)", UiUsbKeyboardActionKey, 0, 0x51, NULL},
	{UiUsbKeyboardCategoryMedia, "YouTube Back 10s (J)", UiUsbKeyboardActionKey, 0, 0x0d, NULL},
	{UiUsbKeyboardCategoryMedia, "YouTube Forward 10s (L)", UiUsbKeyboardActionKey, 0, 0x0f, NULL},
	{UiUsbKeyboardCategoryMedia, "YouTube Previous (Shift+P)", UiUsbKeyboardActionKey, USB_HID_MOD_LSHIFT, 0x13, NULL},
	{UiUsbKeyboardCategoryMedia, "YouTube Next (Shift+N)", UiUsbKeyboardActionKey, USB_HID_MOD_LSHIFT, 0x11, NULL},

	{UiUsbKeyboardCategoryPresentation, "Start (F5)", UiUsbKeyboardActionKey, 0, 0x3e, NULL},
	{UiUsbKeyboardCategoryPresentation, "Current Slide (Shift+F5)", UiUsbKeyboardActionKey, USB_HID_MOD_LSHIFT, 0x3e, NULL},
	{UiUsbKeyboardCategoryPresentation, "Next", UiUsbKeyboardActionKey, 0, 0x4f, NULL},
	{UiUsbKeyboardCategoryPresentation, "Previous", UiUsbKeyboardActionKey, 0, 0x50, NULL},
	{UiUsbKeyboardCategoryPresentation, "Black Screen (B)", UiUsbKeyboardActionKey, 0, 0x05, NULL},

	{UiUsbKeyboardCategoryFunction, "F1 Help", UiUsbKeyboardActionKey, 0, 0x3a, NULL},
	{UiUsbKeyboardCategoryFunction, "F2 Rename", UiUsbKeyboardActionKey, 0, 0x3b, NULL},
	{UiUsbKeyboardCategoryFunction, "F5 Refresh", UiUsbKeyboardActionKey, 0, 0x3e, NULL},
	{UiUsbKeyboardCategoryFunction, "F12 Dev Tools", UiUsbKeyboardActionKey, 0, 0x45, NULL},

	{UiUsbKeyboardCategoryType, "DC32 test line", UiUsbKeyboardActionString, 0, 0, "DC32 USB Keyboard\n"},
};

static bool uiPrvUsbKeyboardDelay(uint32_t msec)
{
	uint64_t end = getTime() + (uint64_t)msec * (TICKS_PER_SECOND / 1000);

	while (getTime() < end) {
		usbHidTask();
		if ((uiGetKeysRaw() & KEY_BIT_B) || uiPrvCenterExitPressedRaw())
			return false;
	}
	return true;
}

static bool uiPrvUsbKeyboardWaitReady(struct Canvas *cnv)
{
	uint64_t end = getTime() + (uint64_t)USB_KEYBOARD_ENUM_WAIT_MS * (TICKS_PER_SECOND / 1000);

	uiPrvBeginPacedRedraw();
	uiPrvSetHeaderTitle("USB Keyboard");
	uiPrvReset(cnv, false);
	cnv->font = FontMedium;
	uiPuts(cnv, uiPrvContentTop(cnv), 10, "Waiting for USB", -1);
	uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "Hold B to cancel", -1);

	while (getTime() < end) {
		usbHidTask();
		uiPrvRefreshHeaderClock(cnv);
		if (uiGetKeysRaw() & KEY_BIT_B)
			return false;
		if (usbHidReady())
			return true;
	}
	return usbHidReady();
}

static bool uiPrvUsbKeyboardAsciiKey(char ch, uint8_t *usageP, uint8_t *modsP)
{
	*modsP = 0;
	if (ch >= 'a' && ch <= 'z') {
		*usageP = 0x04 + ch - 'a';
		return true;
	}
	if (ch >= 'A' && ch <= 'Z') {
		*usageP = 0x04 + ch - 'A';
		*modsP = USB_HID_MOD_LSHIFT;
		return true;
	}
	if (ch >= '1' && ch <= '9') {
		*usageP = 0x1e + ch - '1';
		return true;
	}
	if (ch == '0') {
		*usageP = 0x27;
		return true;
	}
	switch (ch) {
		case ' ': *usageP = 0x2c; return true;
		case '\t': *usageP = 0x2b; return true;
		case '\n': *usageP = 0x28; return true;
		case '!': *usageP = 0x1e; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '@': *usageP = 0x1f; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '#': *usageP = 0x20; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '$': *usageP = 0x21; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '%': *usageP = 0x22; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '^': *usageP = 0x23; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '&': *usageP = 0x24; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '*': *usageP = 0x25; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '(': *usageP = 0x26; *modsP = USB_HID_MOD_LSHIFT; return true;
		case ')': *usageP = 0x27; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '-': *usageP = 0x2d; return true;
		case '_': *usageP = 0x2d; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '=': *usageP = 0x2e; return true;
		case '+': *usageP = 0x2e; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '[': *usageP = 0x2f; return true;
		case '{': *usageP = 0x2f; *modsP = USB_HID_MOD_LSHIFT; return true;
		case ']': *usageP = 0x30; return true;
		case '}': *usageP = 0x30; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '\\': *usageP = 0x31; return true;
		case '|': *usageP = 0x31; *modsP = USB_HID_MOD_LSHIFT; return true;
		case ';': *usageP = 0x33; return true;
		case ':': *usageP = 0x33; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '\'': *usageP = 0x34; return true;
		case '"': *usageP = 0x34; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '`': *usageP = 0x35; return true;
		case '~': *usageP = 0x35; *modsP = USB_HID_MOD_LSHIFT; return true;
		case ',': *usageP = 0x36; return true;
		case '<': *usageP = 0x36; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '.': *usageP = 0x37; return true;
		case '>': *usageP = 0x37; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '/': *usageP = 0x38; return true;
		case '?': *usageP = 0x38; *modsP = USB_HID_MOD_LSHIFT; return true;
		default: return false;
	}
}

static bool uiPrvUsbKeyboardSendKey(uint8_t mods, uint8_t usage)
{
	uint8_t keys[6] = {usage, 0, 0, 0, 0, 0};
	uint8_t release[6] = {0};
	bool ok = true;

	if (!usbHidKeyboardReport(mods, keys))
		return false;
	if (!uiPrvUsbKeyboardDelay(USB_KEYBOARD_KEY_DELAY_MS))
		ok = false;
	if (!usbHidKeyboardReport(0, release))
		ok = false;
	return ok;
}

static bool uiPrvUsbKeyboardSendString(const char *str)
{
	while (*str) {
		uint8_t usage, mods;

		if (uiPrvUsbKeyboardAsciiKey(*str++, &usage, &mods)) {
			if (!uiPrvUsbKeyboardSendKey(mods, usage))
				return false;
		}
	}
	return true;
}

typedef const char *(*UiUsbKeyboardLabelF)(void *userData, uint32_t index);

struct UiUsbKeyboardCommandListCtx {
	enum UiUsbKeyboardCategory category;
};

static const char *uiPrvUsbKeyboardCategoryLabel(void *userData, uint32_t index)
{
	(void)userData;
	return mUsbKeyboardCategories[index].label;
}

static uint32_t uiPrvUsbKeyboardCommandCount(enum UiUsbKeyboardCategory category)
{
	uint32_t i, count = 0;

	for (i = 0; i < sizeof(mUsbKeyboardCommands) / sizeof(*mUsbKeyboardCommands); i++)
		if (mUsbKeyboardCommands[i].category == category)
			count++;
	return count;
}

static uint32_t uiPrvUsbKeyboardCommandTableIndex(enum UiUsbKeyboardCategory category, uint32_t visibleIndex)
{
	uint32_t i;

	for (i = 0; i < sizeof(mUsbKeyboardCommands) / sizeof(*mUsbKeyboardCommands); i++) {
		if (mUsbKeyboardCommands[i].category != category)
			continue;
		if (!visibleIndex--)
			return i;
	}
	return sizeof(mUsbKeyboardCommands) / sizeof(*mUsbKeyboardCommands);
}

static const char *uiPrvUsbKeyboardCommandLabel(void *userData, uint32_t index)
{
	struct UiUsbKeyboardCommandListCtx *ctx = (struct UiUsbKeyboardCommandListCtx*)userData;
	uint32_t tableIndex = uiPrvUsbKeyboardCommandTableIndex(ctx->category, index);

	if (tableIndex >= sizeof(mUsbKeyboardCommands) / sizeof(*mUsbKeyboardCommands))
		return "";
	return mUsbKeyboardCommands[tableIndex].label;
}

static bool uiPrvUsbKeyboardSelectList(struct Canvas *cnv, const char *title, const char *footer,
	const char *status, uint32_t numItems, UiUsbKeyboardLabelF labelF, void *labelData, uint32_t *selectedP, uint32_t *choiceP)
{
	uint32_t selected = *selectedP, topItem, prevTopItem, prevSelOnscreenItem;
	uint_fast8_t itemHeight = uiPrvMenuItemHeight(cnv), itemsOnscreen, listTop, itemLeft, footerRows;
	struct FontGlyphInfo gi;

	if (!numItems)
		return false;
	if (selected >= numItems)
		selected = 0;

	itemLeft = fontGetGlyphInfo(&gi, cnv->font, MENU_SELECTION_CHAR) ? gi.width + 2 : 10;
	listTop = uiPrvContentTop(cnv);
	footerRows = status ? 2 : 1;
	itemsOnscreen = (cnv->h - listTop - footerRows * itemHeight) / itemHeight;
	if (!itemsOnscreen) {
		uiAlert(cnv, "Display area too small for USB Keyboard", DialogTypeOk);
		return false;
	}
	if (itemsOnscreen > numItems)
		itemsOnscreen = numItems;
	topItem = selected >= itemsOnscreen ? selected + 1 - itemsOnscreen : 0;
	prevTopItem = topItem + 1;
	prevSelOnscreenItem = selected - topItem + 1;

	while (1) {
		uint32_t i, selectedOnscreenItem = selected - topItem;

		if (prevTopItem != topItem) {
			uint_fast8_t scrollWidth;

			uiPrvSetHeaderTitle(title);
			uiPrvReset(cnv, false);
			scrollWidth = numItems > itemsOnscreen ? uiPrvDrawScrollbar(cnv, listTop, numItems, topItem, itemsOnscreen) : 0;
			cnv->foreColor = 12;
			for (i = 0; i < itemsOnscreen && topItem + i < numItems; i++)
				uiPrvDrawTruncText(cnv, listTop + i * itemHeight, itemLeft, cnv->w - scrollWidth - itemLeft,
					labelF(labelData, topItem + i));
			if (status)
				uiPrvDrawTruncText(cnv, cnv->h - 2 * itemHeight, 10, cnv->w - 10, status);
			uiPuts(cnv, cnv->h - itemHeight, 10, footer, -1);
			prevSelOnscreenItem = selectedOnscreenItem + 1;
		}
		prevTopItem = topItem;
		if (prevSelOnscreenItem != selectedOnscreenItem) {
			if (prevSelOnscreenItem < itemsOnscreen) {
				cnv->foreColor = 0;
				uiPrvDrawOneChar(cnv, listTop + itemHeight * prevSelOnscreenItem, 1, MENU_SELECTION_CHAR);
			}
			cnv->foreColor = 15;
			uiPrvDrawOneChar(cnv, listTop + itemHeight * selectedOnscreenItem, 1, MENU_SELECTION_CHAR);
		}
		prevSelOnscreenItem = selectedOnscreenItem;

		switch (uiPrvRecvMenuKeypress(cnv)) {
			case UI_KEY_BIT_CENTER:
				uiPrvRequestToolExit();
				return false;

			case KEY_BIT_A:
				*selectedP = selected;
				*choiceP = selected;
				return true;

			case KEY_BIT_B:
				*selectedP = selected;
				return false;

			case KEY_BIT_DOWN:
				if (selected + 1 < numItems) {
					selected++;
					if (selected >= topItem + itemsOnscreen) {
						topItem++;
						if (topItem + itemsOnscreen > numItems)
							topItem = numItems > itemsOnscreen ? numItems - itemsOnscreen : 0;
					}
				}
				else if (!mUiKeyRepeated) {
					selected = 0;
					topItem = 0;
				}
				break;

			case KEY_BIT_UP:
				if (selected) {
					selected--;
					if (selected < topItem)
						topItem = selected;
				}
				else if (!mUiKeyRepeated) {
					selected = numItems - 1;
					topItem = numItems > itemsOnscreen ? numItems - itemsOnscreen : 0;
				}
				break;

			case KEY_BIT_RIGHT:
				if (topItem + itemsOnscreen < numItems) {
					uint32_t offset = selected - topItem;

					topItem += itemsOnscreen;
					if (topItem + itemsOnscreen > numItems)
						topItem = numItems > itemsOnscreen ? numItems - itemsOnscreen : 0;
					selected = topItem + offset;
					if (selected >= numItems)
						selected = numItems - 1;
				}
				break;

			case KEY_BIT_LEFT:
				if (topItem) {
					uint32_t offset = selected - topItem;

					topItem = topItem > itemsOnscreen ? topItem - itemsOnscreen : 0;
					selected = topItem + offset;
					if (selected >= numItems)
						selected = numItems - 1;
				}
				break;
		}
	}
}

static bool uiPrvUsbKeyboardSelectCategory(struct Canvas *cnv, uint32_t *selectedP, const char *status, enum UiUsbKeyboardCategory *categoryP)
{
	uint32_t choice;

	if (!uiPrvUsbKeyboardSelectList(cnv, "USB Keyboard", "A = Open   B = Quit", status,
		UiUsbKeyboardCategoryNum, uiPrvUsbKeyboardCategoryLabel, NULL, selectedP, &choice))
		return false;
	*categoryP = (enum UiUsbKeyboardCategory)choice;
	return true;
}

static bool uiPrvUsbKeyboardSelectCommand(struct Canvas *cnv, enum UiUsbKeyboardCategory category,
	uint32_t *selectedP, const char *status, uint32_t *choiceP)
{
	struct UiUsbKeyboardCommandListCtx ctx = {category};
	uint32_t visibleChoice, numCommands = uiPrvUsbKeyboardCommandCount(category);

	if (!uiPrvUsbKeyboardSelectList(cnv, mUsbKeyboardCategories[category].label, "A = Send   B = Back", status,
		numCommands, uiPrvUsbKeyboardCommandLabel, &ctx, selectedP, &visibleChoice))
		return false;
	*choiceP = uiPrvUsbKeyboardCommandTableIndex(category, visibleChoice);
	return *choiceP < sizeof(mUsbKeyboardCommands) / sizeof(*mUsbKeyboardCommands);
}

static bool uiPrvUsbKeyboardSendCommand(const struct UiUsbKeyboardCommand *cmd)
{
	switch (cmd->action) {
	case UiUsbKeyboardActionString:
		return uiPrvUsbKeyboardSendString(cmd->text);

	case UiUsbKeyboardActionReleaseAll:
		usbHidReleaseAll();
		return true;

	case UiUsbKeyboardActionKey:
	default:
		return uiPrvUsbKeyboardSendKey(cmd->mods, cmd->usage);
	}
}

static void uiPrvUsbKeyboardTool(struct Canvas *cnv)
{
	uint32_t selectedCategory = 0;
	uint32_t selectedCommand[UiUsbKeyboardCategoryNum] = {0};
	char lastStatus[96] = {0};
	bool reportsEnabled = false;
	struct UsbHidDeviceInfo info;

	uiPrvSetHeaderTitle("USB Keyboard");
	if (!uiAlert(cnv, "Start USB Keyboard?\nConnect to a host, then send keys from the badge.", DialogTypeYesNo))
		return;
	usbHidDefaultInfo(&info);
	strcpy(info.product, "DC32 USB Keyboard");
	if (!usbHidBegin(&info)) {
		char msg[96];

		(void)sprintf(msg, "USB Keyboard failed to start\n%s", usbHidLastError());
		uiAlert(cnv, msg, DialogTypeOk);
		return;
	}
	if (!uiPrvUsbKeyboardWaitReady(cnv)) {
		usbHidEnd();
		if (!(uiGetKeysRaw() & KEY_BIT_B))
			uiAlert(cnv, "USB Keyboard failed to enumerate", DialogTypeOk);
		uiPrvWaitKeysReleased();
		return;
	}

	usbHidSetReportsEnabled(true);
	reportsEnabled = true;
	uiPrvWaitKeysReleased();

	while (!uiPrvToolExitRequested()) {
		enum UiUsbKeyboardCategory category;

		if (!uiPrvUsbKeyboardSelectCategory(cnv, &selectedCategory, lastStatus[0] ? lastStatus : NULL, &category))
			break;
		while (!uiPrvToolExitRequested()) {
			uint32_t choice;
			const struct UiUsbKeyboardCommand *cmd;

			if (!uiPrvUsbKeyboardSelectCommand(cnv, category, &selectedCommand[category], lastStatus[0] ? lastStatus : NULL, &choice))
				break;
			cmd = &mUsbKeyboardCommands[choice];
			if (!uiPrvUsbKeyboardSendCommand(cmd)) {
				if (!uiPrvToolExitRequested())
					uiAlert(cnv, "USB Keyboard report failed", DialogTypeOk);
				goto out;
			}
			(void)snprintf(lastStatus, sizeof(lastStatus), "Sent: %s/%s", mUsbKeyboardCategories[category].label, cmd->label);
		}
	}

out:
	if (reportsEnabled) {
		usbHidReleaseAll();
		usbHidSetReportsEnabled(false);
	}
	usbHidEnd();
	uiPrvWaitKeysReleased();
}

#define USB_HID_TOOL_ENUM_WAIT_MS	5000
#define AUTOCLICKER_PRESS_MS		5
#define AUTOCLICKER_MIN_CPS		1
#define AUTOCLICKER_MAX_CPS		50
#define AUTOCLICKER_A_LOCKOUT_TICKS	((uint64_t)TICKS_PER_SECOND * 150u / 1000u)

static bool uiPrvUsbHidWaitReady(struct Canvas *cnv, const char *title)
{
	uint64_t end = getTime() + (uint64_t)USB_HID_TOOL_ENUM_WAIT_MS * (TICKS_PER_SECOND / 1000);

	uiPrvBeginPacedRedraw();
	uiPrvSetHeaderTitle(title);
	uiPrvReset(cnv, false);
	cnv->font = FontMedium;
	uiPuts(cnv, uiPrvContentTop(cnv), 10, "Waiting for USB", -1);
	uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "FN = Exit", -1);

	while (getTime() < end) {
		usbHidTask();
		uiPrvRefreshHeaderClock(cnv);
		if (uiPrvCenterExitPressedRaw()) {
			uiPrvRequestToolExit();
			return false;
		}
		if (usbHidReady())
			return true;
	}
	return usbHidReady();
}

static const char *uiPrvAutoclickerButtonName(uint8_t button)
{
	switch (button) {
	case AutoclickerButtonRight:
		return "Right";
	case AutoclickerButtonMiddle:
		return "Middle";
	case AutoclickerButtonLeft:
	default:
		return "Left";
	}
}

static uint8_t uiPrvAutoclickerButtonMask(uint8_t button)
{
	switch (button) {
	case AutoclickerButtonRight:
		return USB_HID_MOUSE_BUTTON_RIGHT;
	case AutoclickerButtonMiddle:
		return USB_HID_MOUSE_BUTTON_MIDDLE;
	case AutoclickerButtonLeft:
	default:
		return USB_HID_MOUSE_BUTTON_LEFT;
	}
}

static bool uiPrvAutoclickerAdjustClicks(struct Settings *settings, int_fast8_t delta)
{
	uint8_t old = settings->autoclickerCps;

	if (delta > 0) {
		if (settings->autoclickerCps < AUTOCLICKER_MAX_CPS)
			settings->autoclickerCps++;
	}
	else if (delta < 0 && settings->autoclickerCps > AUTOCLICKER_MIN_CPS) {
		settings->autoclickerCps--;
	}
	return old != settings->autoclickerCps;
}

static void uiPrvAutoclickerAdjustButton(struct Settings *settings, bool next)
{
	if (next)
		settings->autoclickerButton = (settings->autoclickerButton + 1) % AutoclickerButtonNumButtons;
	else
		settings->autoclickerButton = settings->autoclickerButton ? settings->autoclickerButton - 1 : AutoclickerButtonNumButtons - 1;
}

static void uiPrvFileBrowserAppSettings(struct Canvas *cnv, struct Settings *settings)
{
	uint_fast8_t selected = 0;

	while (1) {
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;

		uiPrvSetHeaderTitle("File Manager");
		uiPrvReset(cnv, false);
		uiPuts(cnv, uiPrvMenuRow(cnv, 0), 10, "Start in Favorites", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, 0), 135,
			settings->fileBrowserStartFavorites ? "ON" : "OFF", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, 1), 10, "Back", -1);
		selected = uiPrvMenu(cnv, selected, 2, &button);
		if (uiPrvToolExitRequested() || button == KEY_BIT_B || selected == 1)
			return;
		settings->fileBrowserStartFavorites = !settings->fileBrowserStartFavorites;
	}
}

static void uiPrvMusicAppSettings(struct Canvas *cnv, struct Settings *settings)
{
	uint_fast8_t selected = 0;

	while (1) {
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;

		uiPrvSetHeaderTitle("Music");
		uiPrvReset(cnv, false);
		uiPuts(cnv, uiPrvMenuRow(cnv, 0), 10, "Loop track", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, 0), 135, settings->musicLoopTrack ? "ON" : "OFF", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, 1), 10, "Back", -1);
		selected = uiPrvMenu(cnv, selected, 2, &button);
		if (uiPrvToolExitRequested() || button == KEY_BIT_B || selected == 1)
			return;
		settings->musicLoopTrack = !settings->musicLoopTrack;
	}
}

static void uiPrvAutoclickerAppSettings(struct Canvas *cnv, struct Settings *settings)
{
	uint_fast8_t selected = 0;

	while (1) {
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;
		char value[24];

		uiPrvSetHeaderTitle("Autoclicker");
		uiPrvReset(cnv, false);
		uiPuts(cnv, uiPrvMenuRow(cnv, 0), 10, "Button", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, 0), 135, uiPrvAutoclickerButtonName(settings->autoclickerButton), -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, 1), 10, "Clicks / sec", -1);
		(void)sprintf(value, "%u", (unsigned)settings->autoclickerCps);
		uiPuts(cnv, uiPrvMenuRow(cnv, 1), 135, value, -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, 2), 10, "Back", -1);
		selected = uiPrvMenu(cnv, selected, 3, &button);
		if (uiPrvToolExitRequested() || button == KEY_BIT_B || selected == 2)
			return;
		if (selected == 0)
			uiPrvAutoclickerAdjustButton(settings, button != KEY_BIT_LEFT);
		else
			(void)uiPrvAutoclickerAdjustClicks(settings, button == KEY_BIT_LEFT ? -1 : 1);
	}
}

static void uiPrvPongAppSettings(struct Canvas *cnv, struct Settings *settings)
{
	static const char *const themes[] = {"Classic", "Teams", "Rainbow"};
	uint_fast8_t selected = 0;

	while (1) {
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;

		uiPrvSetHeaderTitle("Pong");
		uiPrvReset(cnv, false);
		uiPuts(cnv, uiPrvMenuRow(cnv, 0), 10, "Colors", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, 0), 135, themes[settings->pongColorTheme], -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, 1), 10, "Back", -1);
		selected = uiPrvMenu(cnv, selected, 2, &button);
		if (uiPrvToolExitRequested() || button == KEY_BIT_B || selected == 1)
			return;
		settings->pongColorTheme = button == KEY_BIT_LEFT ?
			(settings->pongColorTheme + 2u) % 3u : (settings->pongColorTheme + 1u) % 3u;
	}
}

static void uiPrvTetrisAppSettings(struct Canvas *cnv, struct Settings *settings)
{
	static const char *const modes[] = {"Marathon", "Line Race", "Ultra"};
	static const char *const rules[] = {"Standard", "Standard Fast B", "Nintendo R"};
	uint_fast8_t selected = 0;

	while (1) {
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;

		uiPrvSetHeaderTitle("Tetris");
		uiPrvReset(cnv, false);
		uiPuts(cnv, uiPrvMenuRow(cnv, 0), 10, "Mode", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, 0), 135, modes[settings->tetrisMode], -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, 1), 10, "Ruleset", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, 1), 135, rules[settings->tetrisRule], -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, 2), 10, "Back", -1);
		selected = uiPrvMenu(cnv, selected, 3, &button);
		if (uiPrvToolExitRequested() || button == KEY_BIT_B || selected == 2)
			return;
		if (selected == 0)
			settings->tetrisMode = button == KEY_BIT_LEFT ? (settings->tetrisMode + 2u) % 3u : (settings->tetrisMode + 1u) % 3u;
		else
			settings->tetrisRule = button == KEY_BIT_LEFT ? (settings->tetrisRule + 2u) % 3u : (settings->tetrisRule + 1u) % 3u;
		settings->portSettingsInitialized = true;
	}
}

static void uiPrvAutoclickerDraw(struct Canvas *cnv, const struct Settings *settings, bool running)
{
	char msg[64];

	uiPrvSetHeaderTitle("Autoclicker");
	uiPrvReset(cnv, false);
	cnv->font = FontMedium;
	uiPuts(cnv, uiPrvContentTop(cnv), 10, running ? "Running" : "Stopped", -1);
	(void)sprintf(msg, "Button: %s", uiPrvAutoclickerButtonName(settings->autoclickerButton));
	uiPuts(cnv, uiPrvMenuRow(cnv, 2), 10, msg, -1);
	(void)sprintf(msg, "Clicks /s: %u", (unsigned)settings->autoclickerCps);
	uiPuts(cnv, uiPrvMenuRow(cnv, 3), 10, msg, -1);
	cnv->foreColor = 15;
	uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, running ? "A/B Stop" : "A Start  B Exit", -1);
}

static bool uiPrvAutoclickerDelay(uint32_t msec)
{
	uint64_t end = getTime() + (uint64_t)msec * (TICKS_PER_SECOND / 1000);

	while (getTime() < end) {
		usbHidTask();
		timebaseIdleWaitMsec(1u);
	}
	return true;
}

static void uiPrvAutoclickerTool(struct Canvas *cnv)
{
	struct Settings settings;
	struct UsbHidDeviceInfo info;
	uint_fast16_t prevKeys = 0;
	uint_fast16_t repeatKey = 0;
	uint64_t nextClick = 0;
	uint64_t repeatNext = 0, aRearmTicks = 0;
	bool running = false, failed = false, reportsEnabled = false, redraw = true;
	bool aToggleArmed = true;

	uiPrvSetHeaderTitle("Autoclicker");
	settingsGet(&settings);
	usbHidDefaultInfo(&info);
	strcpy(info.product, "DC32 Autoclicker");
	if (!usbHidBeginReportSet(&info, UsbHidReportSetMouse)) {
		char msg[96];

		(void)sprintf(msg, "Autoclicker failed to start\n%s", usbHidLastError());
		uiAlert(cnv, msg, DialogTypeOk);
		return;
	}
	if (!uiPrvUsbHidWaitReady(cnv, "Autoclicker")) {
		usbHidEnd();
		if (!uiPrvToolExitRequested())
			uiAlert(cnv, "Autoclicker failed to enumerate", DialogTypeOk);
		uiPrvWaitKeysReleased();
		return;
	}
	usbHidSetReportsEnabled(true);
	reportsEnabled = true;
	usbHidReleaseAll();
	uiPrvWaitKeysReleased();

	while (!uiPrvToolExitRequested()) {
		uint64_t now = getTime();
		uint_fast16_t keys = uiGetUiKeysRaw();
		uint_fast16_t pressed = keys & ~prevKeys;
		uint_fast16_t speedKey = 0;

		usbHidTask();
		uiPrvRefreshHeaderClock(cnv);
		if (pressed & UI_KEY_BIT_CENTER) {
			uiPrvRequestToolExit();
			break;
		}
		if (pressed & KEY_BIT_B) {
			if (running) {
				running = false;
				nextClick = 0;
				redraw = true;
				usbHidReleaseAll();
			}
			else
				break;
		}
		if (!(keys & KEY_BIT_A) && now >= aRearmTicks)
			aToggleArmed = true;
		if ((pressed & KEY_BIT_A) && aToggleArmed) {
			bool wasRunning = running;

			running = !running;
			nextClick = 0;
			redraw = true;
			if (wasRunning && !running)
				usbHidReleaseAll();
			aToggleArmed = false;
			aRearmTicks = now + AUTOCLICKER_A_LOCKOUT_TICKS;
		}
		if (pressed & KEY_BIT_LEFT) {
			uiPrvAutoclickerAdjustButton(&settings, false);
			redraw = true;
		}
		else if (pressed & KEY_BIT_RIGHT) {
			uiPrvAutoclickerAdjustButton(&settings, true);
			redraw = true;
		}
		if (keys & KEY_BIT_UP)
			speedKey = KEY_BIT_UP;
		else if (keys & KEY_BIT_DOWN)
			speedKey = KEY_BIT_DOWN;
		if (speedKey != repeatKey) {
			repeatKey = speedKey;
			repeatNext = speedKey ? now + UI_KEY_REPEAT_INITIAL_TICKS : 0;
			if (speedKey && uiPrvAutoclickerAdjustClicks(&settings, speedKey == KEY_BIT_UP ? 1 : -1))
				redraw = true;
		}
		else if (speedKey && now >= repeatNext) {
			repeatNext = now + UI_KEY_REPEAT_INTERVAL_TICKS;
			if (uiPrvAutoclickerAdjustClicks(&settings, speedKey == KEY_BIT_UP ? 1 : -1))
				redraw = true;
		}
		if (running && (!nextClick || now >= nextClick)) {
			uint8_t button = uiPrvAutoclickerButtonMask(settings.autoclickerButton);
			uint64_t interval = TICKS_PER_SECOND / settings.autoclickerCps;

			if (!usbHidMouseReport(button, 0, 0, 0, 0) || !uiPrvAutoclickerDelay(AUTOCLICKER_PRESS_MS) || !usbHidMouseReport(0, 0, 0, 0, 0)) {
				failed = true;
				break;
			}
			nextClick = now + interval;
		}
		if (redraw) {
			uiPrvBeginPacedRedraw();
			uiPrvAutoclickerDraw(cnv, &settings, running);
			redraw = false;
		}
		prevKeys = keys;
	}

	settingsSet(&settings);
	if (reportsEnabled) {
		usbHidReleaseAll();
		usbHidSetReportsEnabled(false);
	}
	usbHidEnd();
	uiPrvWaitKeysReleased();
	if (failed && !uiPrvToolExitRequested())
		uiAlert(cnv, "Autoclicker report failed", DialogTypeOk);
}

#define USB_GAMEPAD_REPORT_INTERVAL_TICKS	((uint64_t)TICKS_PER_SECOND / 50u)
#define USB_GAMEPAD_FEEDBACK_INTERVAL_TICKS	((uint64_t)TICKS_PER_SECOND / 30u)
#define USB_GAMEPAD_PING_TICKS			((uint64_t)TICKS_PER_SECOND * 90u / 1000u)
#define USB_GAMEPAD_PING_FREQ			1319u
#define USB_GAMEPAD_PING_VOLUME			8u
#define USB_GAMEPAD_HOME_EXIT_TICKS		((uint64_t)TICKS_PER_SECOND * 1200u / 1000u)
#define USB_GAMEPAD_TOUCH_CLICK_ZONE_WIDTH_DIV	5u
#define USB_GAMEPAD_TOUCH_CLICK_ZONE_HEIGHT_DIV	5u

enum UiGamepadProfile {
	UiGamepadProfilePs4,
	UiGamepadProfileXbox360,
};

enum UiGamepadTouchZone {
	UiGamepadTouchZoneNone,
	UiGamepadTouchZoneLeftClick,
	UiGamepadTouchZoneRightClick,
};

struct UiGamepadFeedback {
	uint8_t rumbleLeft;
	uint8_t rumbleRight;
	uint8_t lightRed;
	uint8_t lightGreen;
	uint8_t lightBlue;
	uint32_t pingSeq;
};

static const char *uiPrvGamepadProfileName(enum UiGamepadProfile profile)
{
	switch (profile) {
	case UiGamepadProfileXbox360:
		return "Xbox 360 Controller";
	case UiGamepadProfilePs4:
	default:
		return "PS4 Controller";
	}
}

static bool uiPrvGamepadChooseProfile(struct Canvas *cnv, enum UiGamepadProfile *profileP)
{
	enum {
		GamepadChoicePs4,
		GamepadChoiceXbox360,
		GamepadChoiceCancel,
		GamepadChoiceNum,
	};
	uint_fast8_t selection = GamepadChoicePs4;
	uint_fast16_t button;

	while (!uiPrvToolExitRequested()) {
		uiPrvSetHeaderTitle("USB Gamepad");
		uiPrvReset(cnv, false);
		cnv->font = FontMedium;
		uiPuts(cnv, uiPrvMenuRow(cnv, GamepadChoicePs4), 10, "PS4 Controller", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, GamepadChoiceXbox360), 10, "Xbox 360 Controller", -1);
		uiPuts(cnv, uiPrvMenuRow(cnv, GamepadChoiceCancel), 10, "Cancel", -1);
		uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "A = Start  B = Back", -1);

		button = KEY_BIT_A | KEY_BIT_B;
		selection = uiPrvMenu(cnv, selection, GamepadChoiceNum, &button);
		if (button == KEY_BIT_B || uiPrvToolExitRequested() || selection == GamepadChoiceCancel)
			return false;
		if (profileP)
			*profileP = selection == GamepadChoiceXbox360 ? UiGamepadProfileXbox360 : UiGamepadProfilePs4;
		return true;
	}
	return false;
}

static void uiPrvGamepadTask(enum UiGamepadProfile profile)
{
	if (profile == UiGamepadProfileXbox360)
		usbXinputTask();
	else
		usbHidTask();
}

static bool uiPrvGamepadReady(enum UiGamepadProfile profile)
{
	return profile == UiGamepadProfileXbox360 ? usbXinputReady() : usbHidReady();
}

static bool uiPrvGamepadStart(enum UiGamepadProfile profile)
{
	if (profile == UiGamepadProfileXbox360)
		return usbXinputBegin();
	else {
		struct UsbHidDeviceInfo info;

		usbHidDefaultInfo(&info);
		info.vid = USB_HID_DS4_VID;
		info.pid = USB_HID_DS4_PID;
		strcpy(info.manufacturer, "Sony Computer Entertainment");
		strcpy(info.product, "Wireless Controller");
		return usbHidBeginReportSet(&info, UsbHidReportSetGamepad);
	}
}

static const char *uiPrvGamepadLastError(enum UiGamepadProfile profile)
{
	return profile == UiGamepadProfileXbox360 ? usbXinputLastError() : usbHidLastError();
}

static void uiPrvGamepadSetReportsEnabled(enum UiGamepadProfile profile, bool enabled)
{
	if (profile == UiGamepadProfileXbox360)
		usbXinputSetReportsEnabled(enabled);
	else
		usbHidSetReportsEnabled(enabled);
}

static void uiPrvGamepadReleaseAll(enum UiGamepadProfile profile)
{
	if (profile == UiGamepadProfileXbox360)
		usbXinputReleaseAll();
	else
		(void)usbHidGamepadReport(0, 0, 0, 0, 0, 0, UsbHidGamepadHatCentered, 0);
}

static void uiPrvGamepadEnd(enum UiGamepadProfile profile)
{
	if (profile == UiGamepadProfileXbox360)
		usbXinputEnd();
	else
		usbHidEnd();
}

static bool uiPrvGamepadWaitReady(struct Canvas *cnv, enum UiGamepadProfile profile)
{
	uint64_t end = getTime() + (uint64_t)USB_HID_TOOL_ENUM_WAIT_MS * (TICKS_PER_SECOND / 1000);
	uint64_t homeHoldStart = 0;

	uiPrvBeginPacedRedraw();
	uiPrvSetHeaderTitle("USB Gamepad");
	uiPrvReset(cnv, false);
	cnv->font = FontMedium;
	uiPuts(cnv, uiPrvContentTop(cnv), 10, "Waiting for USB", -1);
	uiPrvDrawTruncText(cnv, uiPrvMenuRow(cnv, 1), 10, cnv->w - 20, uiPrvGamepadProfileName(profile));
	uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "Hold Home = Exit", -1);

	while (getTime() < end) {
		uint64_t now = getTime();

		uiPrvGamepadTask(profile);
		uiPrvRefreshHeaderClock(cnv);
		if (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER) {
			if (!homeHoldStart)
				homeHoldStart = now;
			else if (now - homeHoldStart >= USB_GAMEPAD_HOME_EXIT_TICKS) {
				uiPrvRequestToolExit();
				return false;
			}
		}
		else
			homeHoldStart = 0;
		if (uiPrvGamepadReady(profile))
			return true;
	}
	return uiPrvGamepadReady(profile);
}

static bool uiPrvGamepadSendReport(enum UiGamepadProfile profile, uint8_t hat, uint32_t buttons, const struct UsbHidGamepadTouch *touch)
{
	if (profile == UiGamepadProfileXbox360)
		return usbXinputReport(0, 0, 0, 0, 0, 0, hat, buttons);
	return usbHidGamepadReportTouch(0, 0, 0, 0, 0, 0, hat, buttons, touch);
}

static void uiPrvGamepadReadFeedback(enum UiGamepadProfile profile, struct UiGamepadFeedback *feedback)
{
	memset(feedback, 0, sizeof(*feedback));
	if (profile == UiGamepadProfileXbox360) {
		struct UsbXinputFeedback xinputFeedback;

		usbXinputFeedback(&xinputFeedback);
		feedback->rumbleLeft = xinputFeedback.rumbleLeft;
		feedback->rumbleRight = xinputFeedback.rumbleRight;
		if (xinputFeedback.ledPattern)
			feedback->lightGreen = 24 + ((xinputFeedback.ledPattern & 0x0f) * 8);
		feedback->pingSeq = xinputFeedback.pingSeq;
	}
	else {
		struct UsbHidGamepadFeedback hidFeedback;

		usbHidGamepadFeedback(&hidFeedback);
		feedback->rumbleLeft = hidFeedback.rumbleLeft;
		feedback->rumbleRight = hidFeedback.rumbleRight;
		feedback->lightRed = hidFeedback.lightbarRed / 4;
		feedback->lightGreen = hidFeedback.lightbarGreen / 4;
		feedback->lightBlue = hidFeedback.lightbarBlue / 4;
		feedback->pingSeq = hidFeedback.pingSeq;
	}
}

static uint8_t uiPrvGamepadVisibleRumble(uint8_t value)
{
	if (value && value < 32)
		return 32;
	return value;
}

static void uiPrvGamepadApplyFeedbackLeds(const struct UiGamepadFeedback *feedback)
{
	if (feedback->rumbleLeft || feedback->rumbleRight) {
		uint8_t red = uiPrvGamepadVisibleRumble(feedback->rumbleLeft);
		uint8_t blue = uiPrvGamepadVisibleRumble(feedback->rumbleRight);
		uint8_t green = red < blue ? red / 4 : blue / 4;

		badgeLedsOverrideRgb(red, green, blue);
	}
	else
		badgeLedsOverrideRgb(0, 0, 0);
}

static void uiPrvGamepadUpdatePing(const struct UiGamepadFeedback *feedback, uint32_t *lastPingSeqP, uint64_t *pingEndTicksP, uint_fast8_t restoreVolume)
{
	uint64_t now = getTime();

	if (feedback->pingSeq != *lastPingSeqP) {
		*lastPingSeqP = feedback->pingSeq;
		if (feedback->pingSeq) {
			audioPwmSetVolume(restoreVolume ? restoreVolume : USB_GAMEPAD_PING_VOLUME);
			if (audioPwmTone(USB_GAMEPAD_PING_FREQ))
				*pingEndTicksP = now + USB_GAMEPAD_PING_TICKS;
			else
				audioPwmSetVolume(restoreVolume);
		}
	}
	if (*pingEndTicksP && now >= *pingEndTicksP) {
		audioPwmStop();
		audioPwmSetVolume(restoreVolume);
		*pingEndTicksP = 0;
	}
}

static void uiPrvGamepadRestoreFeedback(const struct Settings *settings, uint_fast8_t audioVolume)
{
	audioPwmStop();
	audioPwmSetVolume(audioVolume);
	badgeLedsOverrideRgb(0, 0, 0);
	badgeLedsApplySettings(settings, true);
}

static uint8_t uiPrvGamepadHat(uint_fast8_t keys)
{
	bool up = !!(keys & KEY_BIT_UP), down = !!(keys & KEY_BIT_DOWN);
	bool left = !!(keys & KEY_BIT_LEFT), right = !!(keys & KEY_BIT_RIGHT);

	if (up && !down && right && !left)
		return UsbHidGamepadHatUpRight;
	if (down && !up && right && !left)
		return UsbHidGamepadHatDownRight;
	if (down && !up && left && !right)
		return UsbHidGamepadHatDownLeft;
	if (up && !down && left && !right)
		return UsbHidGamepadHatUpLeft;
	if (up && !down)
		return UsbHidGamepadHatUp;
	if (right && !left)
		return UsbHidGamepadHatRight;
	if (down && !up)
		return UsbHidGamepadHatDown;
	if (left && !right)
		return UsbHidGamepadHatLeft;
	return UsbHidGamepadHatCentered;
}

static uint32_t uiPrvGamepadButtons(uint_fast16_t keys)
{
	uint32_t buttons = 0;

	if (keys & KEY_BIT_A)
		buttons |= USB_HID_GAMEPAD_BUTTON_A;
	if (keys & KEY_BIT_B)
		buttons |= USB_HID_GAMEPAD_BUTTON_B;
	if (keys & KEY_BIT_SEL)
		buttons |= USB_HID_GAMEPAD_BUTTON_SELECT;
	if (keys & KEY_BIT_START)
		buttons |= USB_HID_GAMEPAD_BUTTON_START;
	if (keys & UI_KEY_BIT_CENTER)
		buttons |= USB_HID_GAMEPAD_BUTTON_HOME;
	return buttons;
}

static int32_t uiPrvGamepadClampCoord(int32_t val, int32_t max)
{
	if (val < 0)
		return 0;
	if (val > max)
		return max;
	return val;
}

static void uiPrvGamepadPackTouchCoord(struct UsbHidGamepadTouch *touch, struct Canvas *cnv, int32_t screenX, int32_t screenY)
{
	uint32_t width = cnv->w > 1 ? cnv->w - 1 : 1;
	uint32_t height = cnv->h > 1 ? cnv->h - 1 : 1;

	screenX = uiPrvGamepadClampCoord(screenX, cnv->w - 1);
	screenY = uiPrvGamepadClampCoord(screenY, cnv->h - 1);
	touch->x = (uint16_t)((uint32_t)screenX * (USB_HID_DS4_TOUCHPAD_WIDTH - 1u) / width);
	touch->y = (uint16_t)((uint32_t)screenY * (USB_HID_DS4_TOUCHPAD_HEIGHT - 1u) / height);
}

static bool uiPrvGamepadReadTouch(struct Canvas *cnv, enum UiGamepadProfile profile, struct UsbHidGamepadTouch *touch, enum UiGamepadTouchZone *zoneP)
{
	struct UiTouchSample sample;
	int32_t rawScreenX, rawScreenY, screenX, screenY;
	uint32_t zoneWidth, zoneHeight;

	memset(touch, 0, sizeof(*touch));
	if (zoneP)
		*zoneP = UiGamepadTouchZoneNone;
	if (profile != UiGamepadProfilePs4)
		return false;
	if (!uiReadTouchRaw(&sample) || !sample.penDown)
		return false;

	rawScreenX = 350 - (int32_t)sample.x * 94 / 1024;
	rawScreenY = 260 - (int32_t)sample.y * 71 / 1024;
	screenX = rawScreenX;
	screenY = rawScreenY;
	touch->active = true;
	uiPrvGamepadPackTouchCoord(touch, cnv, screenX, screenY);

	zoneWidth = cnv->w / USB_GAMEPAD_TOUCH_CLICK_ZONE_WIDTH_DIV;
	zoneHeight = cnv->h / USB_GAMEPAD_TOUCH_CLICK_ZONE_HEIGHT_DIV;
	if (zoneWidth < 1)
		zoneWidth = 1;
	if (zoneHeight < 1)
		zoneHeight = 1;
	screenX = uiPrvGamepadClampCoord(screenX, cnv->w - 1);
	screenY = uiPrvGamepadClampCoord(screenY, cnv->h - 1);
	if ((uint32_t)screenY >= cnv->h - zoneHeight) {
		if ((uint32_t)screenX < zoneWidth) {
			touch->click = true;
			if (zoneP)
				*zoneP = UiGamepadTouchZoneLeftClick;
		}
		else if ((uint32_t)screenX >= cnv->w - zoneWidth) {
			touch->click = true;
			if (zoneP)
				*zoneP = UiGamepadTouchZoneRightClick;
		}
	}
	return true;
}

static bool uiPrvGamepadTouchChanged(const struct UsbHidGamepadTouch *cur, const struct UsbHidGamepadTouch *prev)
{
	return cur->active != prev->active ||
		cur->click != prev->click ||
		cur->x != prev->x ||
		cur->y != prev->y;
}

static bool uiPrvGamepadFeedbackChanged(const struct UiGamepadFeedback *cur,
	const struct UiGamepadFeedback *prev)
{
	return cur->rumbleLeft != prev->rumbleLeft ||
		cur->rumbleRight != prev->rumbleRight ||
		cur->lightRed != prev->lightRed ||
		cur->lightGreen != prev->lightGreen ||
		cur->lightBlue != prev->lightBlue ||
		cur->pingSeq != prev->pingSeq;
}

static const char *uiPrvGamepadTouchZoneName(enum UiGamepadTouchZone zone)
{
	switch (zone) {
	case UiGamepadTouchZoneLeftClick:
		return "Left click";
	case UiGamepadTouchZoneRightClick:
		return "Right click";
	case UiGamepadTouchZoneNone:
	default:
		return "";
	}
}

static void uiPrvGamepadDraw(struct Canvas *cnv, enum UiGamepadProfile profile, uint_fast16_t uiKeys, const struct UiGamepadFeedback *feedback, const struct UsbHidGamepadTouch *touch, enum UiGamepadTouchZone touchZone)
{
	char msg[64];
	uint_fast8_t keys = (uint_fast8_t)uiKeys;

	uiPrvSetHeaderTitle("USB Gamepad");
	uiPrvReset(cnv, false);
	cnv->font = FontMedium;
	uiPrvDrawTruncText(cnv, uiPrvContentTop(cnv), 10, cnv->w - 20, uiPrvGamepadProfileName(profile));
	uiPuts(cnv, uiPrvMenuRow(cnv, 1), 10, "USB connected", -1);
	(void)sprintf(msg, "Hat %u Buttons %08x", (unsigned)uiPrvGamepadHat(keys), (unsigned)uiPrvGamepadButtons(uiKeys));
	uiPrvDrawTruncText(cnv, uiPrvMenuRow(cnv, 2), 10, cnv->w - 20, msg);
	(void)sprintf(msg, "Rumble L%03u R%03u", (unsigned)feedback->rumbleLeft, (unsigned)feedback->rumbleRight);
	uiPrvDrawTruncText(cnv, uiPrvMenuRow(cnv, 3), 10, cnv->w - 20, msg);
	if (profile == UiGamepadProfilePs4) {
		if (touch->active)
			(void)sprintf(msg, "Touch %04u,%03u %s", (unsigned)touch->x, (unsigned)touch->y, uiPrvGamepadTouchZoneName(touchZone));
		else
			(void)strcpy(msg, "Touch up");
		uiPrvDrawTruncText(cnv, uiPrvMenuRow(cnv, 4), 10, cnv->w - 20, msg);
	}
	uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "Hold Home = Exit", -1);
}

static void uiPrvGamepadTool(struct Canvas *cnv)
{
	enum UiGamepadProfile profile;
	struct Settings ledSettings;
	struct UiGamepadFeedback feedback, drawnFeedback = {0};
	struct UsbHidGamepadTouch touch = {0}, prevTouch = {0}, drawnTouch = {0};
	enum UiGamepadTouchZone touchZone = UiGamepadTouchZoneNone, prevTouchZone = UiGamepadTouchZoneNone, drawnTouchZone = UiGamepadTouchZoneNone;
	uint_fast16_t prevUiKeys = 0xffff, drawnUiKeys = 0;
	uint_fast8_t prevKeys = 0xff;
	uint64_t lastReport = 0, lastFeedback = 0, pingEndTicks = 0, homeHoldStart = 0;
	uint_fast8_t audioVolume;
	uint32_t lastPingSeq = 0;
	bool reportsEnabled = false, failed = false, haveDrawn = false;

	uiPrvSetHeaderTitle("USB Gamepad");
	if (!uiPrvGamepadChooseProfile(cnv, &profile))
		return;
	settingsGet(&ledSettings);
	audioVolume = audioPwmGetVolume();
	badgeLedsOverrideRgb(0, 0, 0);

	if (!uiPrvGamepadStart(profile)) {
		char msg[96];

		uiPrvGamepadRestoreFeedback(&ledSettings, audioVolume);
		(void)sprintf(msg, "USB Gamepad failed to start\n%s", uiPrvGamepadLastError(profile));
		uiAlert(cnv, msg, DialogTypeOk);
		return;
	}
	if (!uiPrvGamepadWaitReady(cnv, profile)) {
		uiPrvGamepadEnd(profile);
		uiPrvGamepadRestoreFeedback(&ledSettings, audioVolume);
		if (!uiPrvToolExitRequested())
			uiAlert(cnv, "USB Gamepad failed to enumerate", DialogTypeOk);
		uiPrvWaitKeysReleased();
		return;
	}
	uiPrvGamepadSetReportsEnabled(profile, true);
	reportsEnabled = true;
	uiPrvWaitKeysReleased();
	uiPrvGamepadReadFeedback(profile, &feedback);
	lastPingSeq = feedback.pingSeq;

	while (!uiPrvToolExitRequested()) {
		uint64_t now = getTime();
		uint_fast16_t uiKeys = uiGetUiKeysRaw();
		uint_fast8_t keys = (uint_fast8_t)uiKeys;
		uint8_t hat = uiPrvGamepadHat(keys);
		uint32_t buttons = uiPrvGamepadButtons(uiKeys);

		(void)uiPrvGamepadReadTouch(cnv, profile, &touch, &touchZone);
		uiPrvGamepadTask(profile);
		uiPrvRefreshHeaderClock(cnv);
		if (uiKeys & UI_KEY_BIT_CENTER) {
			if (!homeHoldStart)
				homeHoldStart = now;
			else if (now - homeHoldStart >= USB_GAMEPAD_HOME_EXIT_TICKS) {
				uiPrvRequestToolExit();
				break;
			}
		}
		else
			homeHoldStart = 0;
		if (keys != prevKeys || uiKeys != prevUiKeys || touchZone != prevTouchZone ||
				uiPrvGamepadTouchChanged(&touch, &prevTouch) || !lastReport ||
				now - lastReport >= USB_GAMEPAD_REPORT_INTERVAL_TICKS) {
			if (!uiPrvGamepadSendReport(profile, hat, buttons, &touch)) {
				failed = true;
				break;
			}
			prevKeys = keys;
			prevUiKeys = uiKeys;
			prevTouch = touch;
			prevTouchZone = touchZone;
			lastReport = now;
		}
		if (!lastFeedback || now - lastFeedback >= USB_GAMEPAD_FEEDBACK_INTERVAL_TICKS) {
			uiPrvGamepadReadFeedback(profile, &feedback);
			uiPrvGamepadApplyFeedbackLeds(&feedback);
			uiPrvGamepadUpdatePing(&feedback, &lastPingSeq, &pingEndTicks, audioVolume);
			lastFeedback = now;
		}
		else
			uiPrvGamepadUpdatePing(&feedback, &lastPingSeq, &pingEndTicks, audioVolume);
		if (!haveDrawn || uiKeys != drawnUiKeys || touchZone != drawnTouchZone ||
				uiPrvGamepadTouchChanged(&touch, &drawnTouch) ||
				uiPrvGamepadFeedbackChanged(&feedback, &drawnFeedback)) {
			uiPrvBeginPacedRedraw();
			uiPrvGamepadDraw(cnv, profile, uiKeys, &feedback, &touch, touchZone);
			drawnUiKeys = uiKeys;
			drawnFeedback = feedback;
			drawnTouch = touch;
			drawnTouchZone = touchZone;
			haveDrawn = true;
		}
		timebaseIdleWaitMsec(1u);
	}

	if (reportsEnabled) {
		uiPrvGamepadReleaseAll(profile);
		uiPrvGamepadSetReportsEnabled(profile, false);
	}
	uiPrvGamepadEnd(profile);
	uiPrvGamepadRestoreFeedback(&ledSettings, audioVolume);
	uiPrvWaitKeysReleased();
	if (failed && !uiPrvToolExitRequested())
		uiAlert(cnv, "USB Gamepad report failed", DialogTypeOk);
}

static bool uiPrvHaveValidRom(char *romNameOutP, enum RomColorSupport *romColorSupportP, uint32_t *ramSzExpectedP)
{
	const struct CartHeader *hdr = (const struct CartHeader*)QSPI_ROM_START;
	uint32_t romSzExpected, ramSzExpected;
	enum RomColorSupport romColorSupport;
	struct GameSelection selection;

	#ifndef NO_SD_CARD
		//in systems with an SD card, the file path is mandatory
		if (!uiPrvHaveGamePath())
			return false;
	#endif

	if (romNameOutP)
		romNameOutP[ROM_NAME_LEN] = 0;

	if (!uiGetGameSelection(&selection))
		return false;

	if (selection.runtime == GameRuntimeNes) {
		struct NesRomInfo info;

		if (!nesAnalyzeRom((const void*)QSPI_ROM_START, selection.romSize, &info))
			return false;
		if (romNameOutP)
			(void)sprintf(romNameOutP, "%s", info.name);
		if (romColorSupportP)
			*romColorSupportP = RomNoColor;
		if (ramSzExpectedP)
			*ramSzExpectedP = info.saveRamSize;
		return true;
	}
	if (selection.runtime == GameRuntimeArduboy) {
		struct ArduboyRomInfo info;

		if (!arduboyAnalyzeRom((const void*)QSPI_ROM_START, selection.romSize, &info) || info.isPackage)
			return false;
		if (romNameOutP)
			(void)sprintf(romNameOutP, "%s", info.name);
		if (romColorSupportP)
			*romColorSupportP = RomNoColor;
		if (ramSzExpectedP)
			*ramSzExpectedP = info.saveRamSize;
		return true;
	}
	
	if (!mbcRomAnalyze(hdr, &romSzExpected, &ramSzExpected, &romColorSupport, romNameOutP))
		return false;

	if (romSzExpected > QSPI_ROM_SIZE_MAX || ramSzExpected > QSPI_RAM_SIZE_MAX)
		return false;

	if (romColorSupportP)
		*romColorSupportP = romColorSupport;
	if (ramSzExpectedP)
		*ramSzExpectedP = ramSzExpected;

	return true;
}

static void uiPrvCopyGameTitleFallback(char *dst, uint32_t dstLen, const char *fallbackName)
{
	if (!dstLen)
		return;

	if (!fallbackName)
		fallbackName = "";

	while (--dstLen && *fallbackName)
		*dst++ = *fallbackName++;
	*dst = 0;
}

#ifndef NO_SD_CARD
static void uiPrvTitleFromGamePath(char *dst, uint32_t dstLen, const char *path, const char *fallbackName)
{
	uiPrvCleanGameTitleFromPath(dst, dstLen, path, fallbackName);
}
#endif

static void uiPrvCurrentGameTitle(char *dst, uint32_t dstLen, const char *fallbackName)
{
#ifndef NO_SD_CARD
	if (uiPrvHaveGamePath()) {
		uiPrvTitleFromGamePath(dst, dstLen, (const char*)QSPI_FILENAME_START, fallbackName);
		return;
	}
#endif

	uiPrvCopyGameTitleFallback(dst, dstLen, fallbackName);
}

static enum UiEmulatorConsole uiPrvCurrentGameConsole(void)
{
	struct GameSelection selection;
	enum RomColorSupport colorSupport;

	if (uiGetGameSelection(&selection)) {
		if (selection.runtime == GameRuntimeNes)
			return UiEmulatorConsoleNes;
		if (selection.runtime == GameRuntimeArduboy)
			return UiEmulatorConsoleArduboy;
		if (selection.runtime == GameRuntimeGb) {
			if (uiPrvStrEndsWithNoCase((const char*)QSPI_FILENAME_START, ".gbc"))
				return UiEmulatorConsoleGameboyColor;
			if (!uiPrvHaveValidRom(NULL, &colorSupport, NULL))
				return UiEmulatorConsoleGameboy;
			if (colorSupport == RomColorRequired)
				return UiEmulatorConsoleGameboyColor;
			if (colorSupport == RomColorEnhanced)
				return UiEmulatorConsoleGameboyColor;
			if (colorSupport == RomNoColor)
				return UiEmulatorConsoleGameboy;
			return UiEmulatorConsoleGameboy;
		}
	}

	return UiEmulatorConsoleGameboy;
}

static const char *uiPrvCurrentGameConsoleName(void)
{
	switch (uiPrvCurrentGameConsole()) {
		case UiEmulatorConsoleNes:
			return "NES";
		case UiEmulatorConsoleArduboy:
			return "ARDUBOY";
		case UiEmulatorConsoleGameboyColor:
			return "GBC";
		case UiEmulatorConsoleGameboy:
		default:
			return "GB";
	}
}

static void uiPrvDrawGameAction(struct Canvas *cnv, uint32_t row, const char *title, bool resume)
{
	char label[96];

	if (resume)
		(void)sprintf(label, "Resume %s", title);
	else
		(void)sprintf(label, "Play '%s'", title);

	uiPrvDrawTruncText(cnv, row, 10, cnv->w - 10, label);
}

enum UiToolId {
	UiToolBrowser,
	UiToolInfrared,
	UiToolIr,
	UiToolUsbStorage,
	UiToolBadUsb,
	UiToolUsb,
	UiToolHidTest,
	UiToolAutoclicker,
	UiToolGamepad,
	UiToolMedia,
	UiToolMusic,
	UiToolImage,
	UiToolGames,
	UiToolPorts,
	UiToolGame,
	UiToolSettings,
	UiToolPowerOff,
	UiToolNum,
	UiToolRunGame,
	UiToolRefresh,
};

static const char *uiPrvToolHeaderTitle(enum UiToolId tool)
{
	switch (tool) {
		case UiToolBrowser: return "File Manager";
		case UiToolInfrared: return "Infrared";
		case UiToolIr: return "Universal Remote";
		case UiToolUsbStorage: return "USB Storage";
		case UiToolBadUsb: return "BadUSB";
		case UiToolUsb: return "USB";
		case UiToolHidTest: return "USB Keyboard";
		case UiToolAutoclicker: return "Autoclicker";
		case UiToolGamepad: return "USB Gamepad";
		case UiToolMedia: return "Media";
		case UiToolMusic: return "Music";
		case UiToolImage: return "Image Viewer";
		case UiToolGames: return "Games";
		case UiToolPorts: return "Ports";
		case UiToolGame:
		case UiToolRunGame:
			return "Emulators";
		case UiToolSettings: return "Settings";
		case UiToolPowerOff: return "Power Off";
		default: return "Main Menu";
	}
}

static enum BootGuardMode uiPrvBootGuardModeForTool(enum UiToolId tool)
{
	switch (tool) {
		case UiToolGame:
		case UiToolGames:
		case UiToolPorts:
		case UiToolRunGame:
			return BootGuardModeGame;

		case UiToolInfrared:
		case UiToolIr:
			return BootGuardModeIr;

		case UiToolBadUsb:
			return BootGuardModeBadUsb;

		case UiToolMusic:
			return BootGuardModeMusic;

		case UiToolBrowser:
		case UiToolImage:
		case UiToolUsbStorage:
		case UiToolUsb:
		case UiToolHidTest:
		case UiToolAutoclicker:
		case UiToolGamepad:
		case UiToolMedia:
		case UiToolSettings:
		case UiToolPowerOff:
		default:
			return BootGuardModeTool;
	}
}

static void uiPrvEnterTool(enum UiToolId tool)
{
	uiPowerSetIdleInhibited(true);
	if (tool == UiToolBadUsb || tool == UiToolHidTest || tool == UiToolAutoclicker || tool == UiToolGamepad)
		audioPwmStop();
	bootGuardEnter(uiPrvBootGuardModeForTool(tool));
}

static void uiPrvExitTool(enum UiToolId tool)
{
	usbHidReleaseAll();
	usbHidSetReportsEnabled(false);
	usbXinputSetReportsEnabled(false);
	usbXinputEnd();
#ifndef NO_SD_CARD
	usbMscEnd();
#endif
	audioPwmStop();
	bootGuardExit(uiPrvBootGuardModeForTool(tool));
	uiPowerSetIdleInhibited(false);
}

static const char *uiPrvBootGuardModeName(enum BootGuardMode mode)
{
	switch (mode) {
		case BootGuardModeGame: return "Game";
		case BootGuardModeIr: return "Universal Remote";
		case BootGuardModeBadUsb: return "BadUSB";
		case BootGuardModeMusic: return "Music";
		case BootGuardModeTool: return "Tool";
		case BootGuardModeHardFault: return "HardFault";
		case BootGuardModeNone:
		default:
			return "None";
	}
}

static void uiPrvShowBootRecovery(struct Canvas *cnv)
{
	enum BootGuardMode mode = bootGuardRecoveredMode();
	char msg[224];

	if (mode == BootGuardModeNone)
		return;

	if (mode == BootGuardModeHardFault) {
		struct BootGuardCrashInfo info;

		bootGuardRecoveredCrashInfo(&info);
		(void)sprintf(msg, "Recovered from a crash.\nMode %s in %s\nPC %08x LR %08x\nxPSR %08x SP %08x\nCFSR %08x HFSR %08x\nBFAR %08x",
			uiPrvBootGuardModeName((enum BootGuardMode)info.mode),
			uiPrvBootGuardModeName((enum BootGuardMode)info.originalMode),
			(unsigned)info.pc, (unsigned)info.lr, (unsigned)info.xpsr,
			(unsigned)info.sp, (unsigned)info.cfsr, (unsigned)info.hfsr, (unsigned)info.bfar);
	}
	else {
		(void)sprintf(msg, "Recovered after reset in %s.\nStarting Main Menu.",
			uiPrvBootGuardModeName(mode));
	}

	uiAlert(cnv, msg, DialogTypeOk);
	bootGuardClear();
}

#ifndef NO_SD_CARD
struct UiFileRef {
	struct FatFileLocator locator;
	uint32_t size;
	bool isDir;
	const char *name;
	const char *parentPath;
};

enum UiBrowserOpenWithId {
	UiBrowserOpenNone,
	UiBrowserOpenCancelled,
	UiBrowserOpenIrButtonSpam,
	UiBrowserOpenBadUsb,
	UiBrowserOpenMusic,
	UiBrowserOpenGame,
	UiBrowserOpenImage,
	UiBrowserOpenDcApp,
};

static void uiPrvImageAlert(struct Canvas *cnv, enum ImageViewerResult result)
{
	switch (result) {
	case ImageViewerResultOpenError:
		uiAlert(cnv, "Cannot open image file", DialogTypeOk);
		break;
	case ImageViewerResultReadError:
		uiAlert(cnv, "Image read failed", DialogTypeOk);
		break;
	case ImageViewerResultDecodeError:
		uiAlert(cnv, "Cannot decode image file", DialogTypeOk);
		break;
	case ImageViewerResultIncompatibleGif:
		uiAlert(cnv, "This GIF is not DC32-compatible. Run /IMAGES/image_converter.py to create a supported .gif.", DialogTypeOk);
		break;
	case ImageViewerResultUnsupported:
		uiAlert(cnv, "Open .gif, .jpg, .jpeg, or uncompressed .bmp. Use /IMAGES/image_converter.py for DC32-compatible GIFs.", DialogTypeOk);
		break;
	case ImageViewerResultTooLarge:
		uiAlert(cnv, "Image is too large for this firmware", DialogTypeOk);
		break;
	case ImageViewerResultNoMemory:
		uiAlert(cnv, "Not enough workspace memory for image viewer", DialogTypeOk);
		break;
	default:
		break;
	}
}

static bool uiPrvImageFileName(const char *name)
{
	return uiPrvStrEndsWithNoCase(name, ".gif") || uiPrvStrEndsWithNoCase(name, ".dci") || uiPrvStrEndsWithNoCase(name, ".dca") ||
		uiPrvStrEndsWithNoCase(name, ".jpg") || uiPrvStrEndsWithNoCase(name, ".jpeg") ||
		uiPrvStrEndsWithNoCase(name, ".bmp");
}

static bool uiPrvImageResultCanSkip(enum ImageViewerResult result)
{
	return result == ImageViewerResultOpenError || result == ImageViewerResultReadError ||
		result == ImageViewerResultDecodeError || result == ImageViewerResultIncompatibleGif ||
		result == ImageViewerResultUnsupported || result == ImageViewerResultTooLarge;
}

static bool uiPrvFindAdjacentImage(struct FatfsVol *vol, const char *path, const char *curName, bool forward, struct FatFileLocator *locatorOut, char *nameOut, uint32_t nameOutSz)
{
	struct FatfsDir *dir;
	char fname[FATFS_NAME_BUF_LEN];
	uint32_t fileSz;
	uint8_t attrs;
	struct FatFileLocator locator, firstLoc, lastLoc, candidateLoc;
	char firstName[UI_PICK_FILE_NAME_BUF_SZ], lastName[UI_PICK_FILE_NAME_BUF_SZ], candidateName[UI_PICK_FILE_NAME_BUF_SZ];
	bool haveFirst = false, haveLast = false, haveCandidate = false;

	dir = fatfsDirOpen(vol, path);
	if (!dir)
		return false;

	while (fatfsDirRead(dir, fname, &fileSz, &attrs, &locator)) {
		int curCmp;

		if ((attrs & (FATFS_ATTR_VOL_LBL | FATFS_ATTR_DIR)) || uiPrvHiddenEntry(fname, attrs) || !uiPrvImageFileVisibleInDir(vol, path, NULL, fname))
			continue;
		if (!haveFirst || strsCaselesslyCompareUtf(fname, firstName, 0xffffffff) < 0) {
			firstLoc = locator;
			uiPrvCopyStr(firstName, sizeof(firstName), fname);
			haveFirst = true;
		}
		if (!haveLast || strsCaselesslyCompareUtf(fname, lastName, 0xffffffff) > 0) {
			lastLoc = locator;
			uiPrvCopyStr(lastName, sizeof(lastName), fname);
			haveLast = true;
		}
		curCmp = strsCaselesslyCompareUtf(fname, curName, 0xffffffff);
		if (forward) {
			if (curCmp > 0 && (!haveCandidate || strsCaselesslyCompareUtf(fname, candidateName, 0xffffffff) < 0)) {
				candidateLoc = locator;
				uiPrvCopyStr(candidateName, sizeof(candidateName), fname);
				haveCandidate = true;
			}
		}
		else if (curCmp < 0 && (!haveCandidate || strsCaselesslyCompareUtf(fname, candidateName, 0xffffffff) > 0)) {
			candidateLoc = locator;
			uiPrvCopyStr(candidateName, sizeof(candidateName), fname);
			haveCandidate = true;
		}
	}
	fatfsDirClose(dir);

	if (!haveFirst)
		return false;
	if (haveCandidate) {
		*locatorOut = candidateLoc;
		uiPrvCopyStr(nameOut, nameOutSz, candidateName);
		return true;
	}
	if (forward) {
		*locatorOut = firstLoc;
		uiPrvCopyStr(nameOut, nameOutSz, firstName);
	}
	else {
		*locatorOut = lastLoc;
		uiPrvCopyStr(nameOut, nameOutSz, lastName);
	}
	return true;
}

enum UiImageMenuAction {
	UiImageMenuActionReturn,
	UiImageMenuActionSelect,
	UiImageMenuActionSettings,
	UiImageMenuActionMainMenu,
};

static enum UiImageMenuAction uiPrvImageViewerMenu(struct Canvas *cnv)
{
	enum {
		ImageMenuOptionReturn,
		ImageMenuOptionSelect,
		ImageMenuOptionSettings,
		ImageMenuOptionMainMenu,
		ImageMenuOptionNum,
	};
	static const char *labels[ImageMenuOptionNum] = {
		[ImageMenuOptionReturn] = "Return",
		[ImageMenuOptionSelect] = "Select image",
		[ImageMenuOptionSettings] = "Settings",
		[ImageMenuOptionMainMenu] = "Main Menu",
	};
	uint_fast8_t i, selOption;
	uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;

	uiPrvSetHeaderTitle("Image Viewer");
	uiPrvReset(cnv, false);
	for (i = 0; i < ImageMenuOptionNum; i++)
		uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, labels[i], -1);

	selOption = uiPrvMenu(cnv, 0, ImageMenuOptionNum, &button);
	if (button == KEY_BIT_B || selOption == ImageMenuOptionReturn)
		return UiImageMenuActionReturn;
	if (selOption == ImageMenuOptionSelect)
		return UiImageMenuActionSelect;
	if (selOption == ImageMenuOptionSettings)
		return UiImageMenuActionSettings;
	return UiImageMenuActionMainMenu;
}

static void uiPrvRunImageSequence(struct Canvas *cnv, struct FatfsVol *vol, const char *parentPath, const struct FatFileLocator *locator, const char *name)
{
	struct FatFileLocator curLocator = *locator;
	char curName[UI_PICK_FILE_NAME_BUF_SZ];
	char curPath[UI_PICK_FILE_PATH_BUF_SZ];
	char firstBadName[UI_PICK_FILE_NAME_BUF_SZ];
	bool skippingBadImages = false;

	uiPrvCopyStr(curName, sizeof(curName), name);
	uiPrvCopyStr(curPath, sizeof(curPath), parentPath && parentPath[0] ? parentPath : "/");

	while (1) {
		enum ImageViewerResult result;
		struct Settings settings;
		struct Canvas viewerCanvas = *cnv;

		settingsGet(&settings);
		viewerCanvas.flipped = settings.rotation;
		uiPrvSetHeaderTitle("Image Viewer");
		uiPowerSetScreenSaverContentActive(true);
		result = imageViewerRun(&viewerCanvas, vol, curPath, &curLocator, curName);
		uiPowerSetScreenSaverContentActive(false);
		if (uiPrvImageResultCanSkip(result)) {
			struct FatFileLocator nextLocator;
			char nextName[UI_PICK_FILE_NAME_BUF_SZ];

			if (!skippingBadImages) {
				uiPrvCopyStr(firstBadName, sizeof(firstBadName), curName);
				skippingBadImages = true;
			}
			if (uiPrvFindAdjacentImage(vol, curPath, curName, true, &nextLocator, nextName, sizeof(nextName)) &&
					strsCaselesslyCompareUtf(nextName, firstBadName, 0xffffffff)) {
				curLocator = nextLocator;
				uiPrvCopyStr(curName, sizeof(curName), nextName);
				continue;
			}
			uiAlert(cnv, "No readable images found in this folder", DialogTypeOk);
			return;
		}
		skippingBadImages = false;
		if (result == ImageViewerResultMenu) {
			enum UiImageMenuAction action;

			uiPrvWaitKeysReleased();
			action = uiPrvImageViewerMenu(cnv);
			if (action == UiImageMenuActionReturn)
				continue;
			if (action == UiImageMenuActionSelect)
				return;
			if (action == UiImageMenuActionSettings) {
				(void)uiPrvSettings(cnv, true);
				if (uiPrvToolExitRequested())
					return;
				continue;
			}
			uiPrvRequestToolExit();
			return;
		}
		if (result == ImageViewerResultExit) {
			uiPrvRequestToolExit();
			return;
		}
		if (result == ImageViewerResultBack)
			return;
		if (result == ImageViewerResultPrev || result == ImageViewerResultNext) {
			struct FatFileLocator nextLocator;
			char nextName[UI_PICK_FILE_NAME_BUF_SZ];

			if (uiPrvFindAdjacentImage(vol, curPath, curName, result == ImageViewerResultNext, &nextLocator, nextName, sizeof(nextName))) {
				curLocator = nextLocator;
				uiPrvCopyStr(curName, sizeof(curName), nextName);
				continue;
			}
			uiAlert(cnv, "No other images found in this folder", DialogTypeOk);
			continue;
		}
		uiPrvImageAlert(cnv, result);
		return;
	}
}

static enum UiToolId uiPrvToolSwitcher(struct Canvas *cnv, enum UiToolId curTool)
{
	static const char *names[UiToolNum] = {
		[UiToolBrowser] = "File Manager",
		[UiToolInfrared] = "Infrared",
		[UiToolIr] = "Universal Remote",
		[UiToolUsbStorage] = "USB Storage",
		[UiToolBadUsb] = "BadUSB",
		[UiToolUsb] = "USB",
		[UiToolHidTest] = "USB Keyboard",
		[UiToolAutoclicker] = "Autoclicker",
		[UiToolGamepad] = "USB Gamepad",
		[UiToolMedia] = "Media",
		[UiToolMusic] = "Music",
		[UiToolImage] = "Image Viewer",
		[UiToolGames] = "Games",
		[UiToolPorts] = "Ports",
		[UiToolGame] = "Emulators",
		[UiToolSettings] = "Settings",
		[UiToolPowerOff] = "Power Off",
	};
	static const enum UiToolId toolOrder[] = {
		UiToolBrowser,
		UiToolInfrared,
		UiToolUsb,
		UiToolMedia,
		UiToolGames,
		UiToolSettings,
		UiToolPowerOff,
	};
	uint_fast8_t itemHeight, i, selOption, numTools = sizeof(toolOrder) / sizeof(*toolOrder);
	uint_fast8_t curOption = 0;
	uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;

	mToolExitEnabled = false;
	uiPrvClearToolExit();
	if (curTool == UiToolIr)
		curTool = UiToolInfrared;

	uiPrvSetHeaderTitle("Main Menu");
	uiPrvReset(cnv, false);
	itemHeight = uiPrvGlyphHeight(cnv) + 1;
	for (i = 0; i < numTools; i++) {
		if (toolOrder[i] == curTool)
			curOption = i;
		uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, names[toolOrder[i]], -1);
	}

	selOption = uiPrvMenu(cnv, curOption, numTools, &button);
	if (uiPowerConsumeScreenSaverWake())
		return UiToolRefresh;
	if (button == KEY_BIT_B)
		return curTool;
	return toolOrder[selOption];
}

static enum UiBrowserOpenWithId uiPrvBrowserOpenWith(struct Canvas *cnv, const struct UiFileRef *ref)
{
	enum UiBrowserOpenWithId ids[8];
	const char *labels[8];
	uint_fast8_t numOptions = 0, i, selOption;
	uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;

	if (uiPrvIrRemoteFileName(ref->name)) {
		ids[numOptions] = UiBrowserOpenIrButtonSpam;
		labels[numOptions++] = "Universal Remote";
	}
	if (uiPrvStrEndsWithNoCase(ref->name, ".txt") || uiPrvStrEndsWithNoCase(ref->name, ".badusb")) {
		ids[numOptions] = UiBrowserOpenBadUsb;
		labels[numOptions++] = "BadUSB";
	}
	if (uiPrvStrEndsWithNoCase(ref->name, ".txt") || uiPrvStrEndsWithNoCase(ref->name, ".rtttl") ||
		uiPrvStrEndsWithNoCase(ref->name, ".abc") || uiPrvStrEndsWithNoCase(ref->name, ".mid") ||
		uiPrvStrEndsWithNoCase(ref->name, ".midi")) {
		ids[numOptions] = UiBrowserOpenMusic;
		labels[numOptions++] = "Music";
	}
	if (uiPrvRomFileName(ref->name)) {
		ids[numOptions] = UiBrowserOpenGame;
		labels[numOptions++] = "Emulators";
	}
	if (uiPrvImageFileName(ref->name)) {
		ids[numOptions] = UiBrowserOpenImage;
		labels[numOptions++] = "Image Viewer";
	}
	if (uiPrvStrEndsWithNoCase(ref->name, ".DC32")) {
		ids[numOptions] = UiBrowserOpenDcApp;
		labels[numOptions++] = "DC32 App";
	}

	if (!numOptions)
		return UiBrowserOpenNone;
	if (numOptions == 1)
		return ids[0];

	uiPrvSetHeaderTitle("Open With");
	uiPrvReset(cnv, false);
	for (i = 0; i < numOptions; i++)
		uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, labels[i], -1);
	uiPuts(cnv, uiPrvMenuRow(cnv, numOptions), 10, "Cancel", -1);

	selOption = uiPrvMenu(cnv, 0, numOptions + 1, &button);
	if (uiPrvToolExitRequested())
		return UiBrowserOpenCancelled;
	if (button == KEY_BIT_B || selOption == numOptions)
		return UiBrowserOpenCancelled;
	return ids[selOption];
}

static enum UiToolId uiPrvBrowserLaunchedToolReturn(enum UiToolId launchedTool)
{
	return uiPrvToolExitRequested() ? launchedTool : UiToolBrowser;
}

static const struct DcAppCatalogEntry *uiPrvBrowserFindDcApp(const struct UiFileRef *ref)
{
	char path[UI_PICK_FILE_PATH_BUF_SZ];
	const struct DcAppCatalogEntry *entries;
	uint_fast8_t count, i;

	uiPrvCopyStr(path, sizeof(path), ref->parentPath && ref->parentPath[0] ? ref->parentPath : "/");
	uiPrvAppendPathComponent(path, sizeof(path), ref->name);
	entries = dcAppCatalogEntries(&count);
	for (i = 0; i < count; i++)
		if (!strsCaselesslyCompareUtf(path, entries[i].path, 0xffffffff))
			return &entries[i];
	return NULL;
}

static bool uiPrvBrowserDcAppIsRuntimeEngine(enum DcAppId appId)
{
	return appId == DcAppIdGameGb || appId == DcAppIdGameNes || appId == DcAppIdGameArduboy;
}

static enum UiToolId uiPrvBrowserToolForDcApp(const struct DcAppCatalogEntry *entry)
{
	switch (entry->appId) {
	case DcAppIdToolIr:
		return UiToolIr;
	case DcAppIdToolLaserTag:
		return UiToolInfrared;
	case DcAppIdToolImage:
		return UiToolImage;
	case DcAppIdToolMusic:
		return UiToolMusic;
	case DcAppIdToolBadUsb:
		return UiToolBadUsb;
	case DcAppIdToolAutoclicker:
		return UiToolAutoclicker;
	case DcAppIdToolGamepad:
		return UiToolGamepad;
	case DcAppIdToolRaspyJack:
	case DcAppIdToolPwnagotchi:
		return UiToolUsb;
	case DcAppIdStarfield:
	case DcAppIdSpiro:
	case DcAppIdCube:
	case DcAppIdDvdBounce:
	case DcAppIdScrollPattern:
		return UiToolMedia;
	case DcAppIdPong:
	case DcAppIdTetris:
	case DcAppIdArkanoid:
	case DcAppIdFlappy:
	case DcAppIdTrex:
	case DcAppIdDoom:
	case DcAppIdChips:
	case DcAppIdScorch:
	case DcAppIdPipe:
	case DcAppIdSokoban:
	case DcAppIdOpenJazz:
	case DcAppIdSoccer:
		return UiToolPorts;
	default:
		return entry->launcherVisible ? UiToolGames : UiToolBrowser;
	}
}

static bool uiPrvRunSdApp(struct Canvas *cnv, enum DcAppId appId, enum DcAppToolAction action,
	struct FatfsVol *vol, const struct FatFileLocator *locator, const char *name, const char *parentPath)
{
	struct Settings settings;
	struct DcAppRunArgs args = {
		.toolAction = action,
		.canvas = cnv,
		.vol = vol,
		.locator = locator,
		.name = name,
		.parentPath = parentPath,
	};
	enum DcAppResult result;

	settingsGet(&settings);
	if (appId >= DcAppIdPong && appId <= DcAppIdSoccer)
		args.rotate = settings.rotation;
	result = dcAppRunTool(appId, &args);

	if (result != DcAppResultOk) {
		if (dcAppLastError()[0])
			uiAlert(cnv, dcAppLastError(), DialogTypeOk);
		else
			uiAlert(cnv, dcAppResultName(result), DialogTypeOk);
		dcAppClearError();
		return false;
	}
	return true;
}

static enum UiToolId uiPrvLaunchBrowserFile(struct Canvas *cnv, struct FatfsVol *vol, const struct UiFileRef *ref)
{
	switch (uiPrvBrowserOpenWith(cnv, ref)) {
	case UiBrowserOpenIrButtonSpam:
		uiPrvSetHeaderTitle("Universal Remote");
		(void)uiPrvRunSdApp(cnv, DcAppIdToolIr, DcAppToolActionIrButton, vol, &ref->locator, ref->name, ref->parentPath);
		return uiPrvBrowserLaunchedToolReturn(UiToolIr);

	case UiBrowserOpenBadUsb:
		uiPrvSetHeaderTitle("BadUSB");
		(void)uiPrvRunSdApp(cnv, DcAppIdToolBadUsb, DcAppToolActionBadUsbFile, vol, &ref->locator, ref->name, ref->parentPath);
		return uiPrvBrowserLaunchedToolReturn(UiToolBadUsb);

	case UiBrowserOpenMusic:
		uiPrvSetHeaderTitle("Music");
		if (!uiPrvMusicBatteryOkToLaunch(cnv))
			return UiToolBrowser;
		(void)uiPrvRunSdApp(cnv, DcAppIdToolMusic, DcAppToolActionMusicFile, vol, &ref->locator, ref->name, ref->parentPath);
		return uiPrvBrowserLaunchedToolReturn(UiToolMusic);

	case UiBrowserOpenGame:
		if (!uiPrvPrepareForRomReplacement(cnv, vol, "browser game open"))
			return UiToolBrowser;
		if (uiPrvConfirmRomSelection(cnv, vol, &ref->locator, ref->name))
			return UiToolRunGame;
		return UiToolBrowser;

	case UiBrowserOpenImage:
		(void)uiPrvRunSdApp(cnv, DcAppIdToolImage, DcAppToolActionImageFile, vol, &ref->locator, ref->name, ref->parentPath);
		return uiPrvBrowserLaunchedToolReturn(UiToolImage);

	case UiBrowserOpenDcApp:
	{
		const struct DcAppCatalogEntry *entry = uiPrvBrowserFindDcApp(ref);
		enum UiToolId launchedTool;

		if (!entry) {
			uiAlert(cnv, "That .DC32 app is not registered.", DialogTypeOk);
			return UiToolBrowser;
		}
		if (uiPrvBrowserDcAppIsRuntimeEngine(entry->appId)) {
			uiAlert(cnv, "Open Emulators from Games to run ROMs.", DialogTypeOk);
			return UiToolBrowser;
		}
		if (entry->appId == DcAppIdToolMusic && !uiPrvMusicBatteryOkToLaunch(cnv))
			return UiToolBrowser;
		launchedTool = uiPrvBrowserToolForDcApp(entry);
		uiPrvSetHeaderTitle(entry->name);
		(void)uiPrvRunSdApp(cnv, entry->appId, DcAppToolActionMain, vol, NULL, NULL, NULL);
		return uiPrvBrowserLaunchedToolReturn(launchedTool);
	}

	case UiBrowserOpenNone:
		uiAlert(cnv, "No tool is registered for that file type", DialogTypeOk);
		return UiToolBrowser;

	case UiBrowserOpenCancelled:
	default:
		break;
	}

	return UiToolBrowser;
}

static enum UiToolId uiPrvBrowserTool(struct Canvas *cnv, UiRunGameF runGameF, void *userData)
{
	struct FatfsVol *vol;
	enum UiToolId nextTool = UiToolBrowser;
	struct FatFileLocator locator;
	char name[UI_PICK_FILE_NAME_BUF_SZ];
	struct ToolWorkspaceSpan pathMem = toolWorkspaceGet(ToolWorkspaceCartRamUpper);
	char *browserPath = (char*)pathMem.ptr;
	struct UiFileRef ref;
	struct UiBrowserOps browserOps;
	struct Settings settings;

	(void)runGameF;
	(void)userData;

	if (!browserPath || pathMem.size < UI_PICK_FILE_PATH_BUF_SZ) {
		uiAlert(cnv, "Tool workspace is too small for File Manager", DialogTypeOk);
		return UiToolBrowser;
	}
	uiPrvCartRamOwnerClear("file manager workspace");
	browserPath[0] = '/';
	browserPath[1] = 0;
	memset(&browserOps, 0, sizeof(browserOps));

	uiPrvSetHeaderTitle("File Manager");
	uiPrvReset(cnv, false);
	uiPrvDrawWrappedString(cnv, "Opening File Manager...", 32, 10);
	vol = uiPrvMountCard(cnv, false);
	if (!vol)
		return UiToolBrowser;
	uiPrvBrowserFavoritesLoad(vol, &browserOps.favorites);
	settingsGet(&settings);
	if (settings.fileBrowserStartFavorites && !browserOps.favorites.count) {
		settings.fileBrowserStartFavorites = false;
		(void)settingsSet(&settings);
	}
	browserOps.showFavorites = settings.fileBrowserStartFavorites && browserOps.favorites.count;

	while (1) {
		if (browserOps.showFavorites) {
			struct FatfsDir *favoriteDir;

			browserOps.showFavorites = false;
			if (!uiPrvBrowserChooseLocation(cnv, vol, &browserOps, browserPath, UI_PICK_FILE_PATH_BUF_SZ))
				break;
			favoriteDir = fatfsDirOpen(vol, browserPath);
			if (!favoriteDir) {
				char parentPath[UI_BROWSER_ACTION_PATH_MAX];
				struct FatfsDir *parentDir = NULL;

				if (!uiPrvBrowserSplitPath(browserPath, parentPath, sizeof(parentPath), name, sizeof(name)) ||
						!(parentDir = fatfsDirOpen(vol, parentPath)) ||
						!fatfsFindFileAt(parentDir, name, &locator)) {
					if (parentDir) (void)fatfsDirClose(parentDir);
					uiAlert(cnv, "Favorite is no longer available", DialogTypeOk);
					continue;
				}
				(void)fatfsDirClose(parentDir);
				memset(&ref, 0, sizeof(ref));
				ref.locator = locator;
				ref.name = name;
				ref.parentPath = parentPath;
				nextTool = uiPrvLaunchBrowserFile(cnv, vol, &ref);
				if (uiPrvToolExitRequested() || nextTool != UiToolBrowser)
					break;
				browserOps.showFavorites = true;
				continue;
			}
			(void)fatfsDirClose(favoriteDir);
		}
		uiPrvSetHeaderTitle("File Manager");
		if (!uiPrvPickFile(cnv, vol, browserPath, NULL, "No files found on the SD card", false, &locator, name, sizeof(name), browserPath, UI_PICK_FILE_PATH_BUF_SZ, NULL, false, &browserOps)) {
			if (browserOps.showFavorites)
				continue;
			break;
		}
		memset(&ref, 0, sizeof(ref));
		ref.locator = locator;
		ref.name = name;
		ref.parentPath = browserPath;
		nextTool = uiPrvLaunchBrowserFile(cnv, vol, &ref);
		if (uiPrvToolExitRequested())
			break;
		if (nextTool != UiToolBrowser)
			break;
	}

	(void)uiPrvCardPreUnmount();
	fatfsUnmount(vol);
	return nextTool;
}

static void uiPrvImageViewerTool(struct Canvas *cnv)
{
	struct FatfsVol *vol;
	struct FatFileLocator locator;
	char name[UI_PICK_FILE_NAME_BUF_SZ];
	char parentPath[UI_PICK_FILE_PATH_BUF_SZ];

	uiPrvSetHeaderTitle("Image Viewer");
	uiPrvReset(cnv, false);
	uiPrvDrawWrappedString(cnv, "Opening Image Viewer...", 32, 10);
	vol = uiPrvMountCard(cnv, false);
	if (!vol)
		return;

	while (!uiPrvToolExitRequested()) {
		uiPrvSetHeaderTitle("Image Viewer");
		if (!uiPrvPickFile(cnv, vol, "/IMAGES", uiPrvImageFileName, "No image files found in /IMAGES", false, &locator, name, sizeof(name), parentPath, sizeof(parentPath), NULL, false, NULL))
			break;
		uiPrvRunImageSequence(cnv, vol, parentPath, &locator, name);
		uiPrvWaitKeysReleased();
	}

	(void)uiPrvCardPreUnmount();
	fatfsUnmount(vol);
}

#ifdef DCAPP_TOOL_BUILD
int uiDcAppRunIr(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	(void)host;
	if (!args || !args->canvas)
		return -1;
	switch (args->toolAction) {
	case DcAppToolActionMain:
		(void)uiPrvIrTools(args->canvas);
		return 0;
	case DcAppToolActionIrButton:
		if (!args->vol || !args->locator || !args->name)
			return -1;
		(void)uiPrvIrButtonSpamLocator(args->canvas, args->vol, args->locator, args->name);
		return 0;
	case DcAppToolActionIrPower:
		if (!args->vol || !args->locator)
			return -1;
		(void)uiPrvIrBlastLocator(args->canvas, args->vol, args->locator, NULL, "Power");
		return 0;
	case DcAppToolActionIrMute:
		if (!args->vol || !args->locator)
			return -1;
		(void)uiPrvIrBlastLocator(args->canvas, args->vol, args->locator, "Mute", "Mute");
		return 0;
	default:
		return -1;
	}
}

int uiDcAppRunImage(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	(void)host;
	if (!args || !args->canvas)
		return -1;
	if (args->toolAction == DcAppToolActionMain) {
		uiPrvImageViewerTool(args->canvas);
		return 0;
	}
	if (args->toolAction == DcAppToolActionImageFile && args->vol && args->locator && args->name) {
		uiPrvRunImageSequence(args->canvas, args->vol, args->parentPath, args->locator, args->name);
		uiPrvWaitKeysReleased();
		return 0;
	}
	return -1;
}

int uiDcAppRunMusic(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	(void)host;
	if (!args || !args->canvas)
		return -1;
	if (args->toolAction == DcAppToolActionMain) {
		if (!uiPrvMusicBatteryOkToLaunch(args->canvas))
			return 0;
		uiPrvMusicPlayer(args->canvas);
		return 0;
	}
	if (args->toolAction == DcAppToolActionMusicFile && args->vol && args->locator && args->name) {
		struct Settings settings;

		if (!uiPrvMusicBatteryOkToLaunch(args->canvas))
			return 0;
		settingsGet(&settings);
		uiPrvMusicSanitizeSettings(&settings);
		(void)uiPrvPlayMusicLocator(args->canvas, args->vol, args->locator, args->name, &settings, NULL);
		settingsSet(&settings);
		return 0;
	}
	return -1;
}

int uiDcAppRunBadUsb(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	(void)host;
	if (!args || !args->canvas)
		return -1;
	if (args->toolAction == DcAppToolActionMain) {
		(void)uiPrvBadUsbTool(args->canvas);
		return 0;
	}
	if (args->toolAction == DcAppToolActionBadUsbFile && args->vol && args->locator && args->name) {
		(void)uiPrvRunBadUsbLocator(args->canvas, args->vol, args->locator, args->name);
		return 0;
	}
	return -1;
}

int uiDcAppRunAutoclicker(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	(void)host;
	if (!args || !args->canvas)
		return -1;
	if (args->toolAction == DcAppToolActionMain) {
		uiPrvAutoclickerTool(args->canvas);
		return 0;
	}
	return -1;
}

int uiDcAppRunGamepad(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	(void)host;
	if (!args || !args->canvas)
		return -1;
	if (args->toolAction == DcAppToolActionMain) {
		uiPrvGamepadTool(args->canvas);
		return 0;
	}
	return -1;
}
#endif

struct UiUsbStorageState {
	uint32_t blocks;
	uint32_t lba;
	uint32_t bytes;
	bool mounted;
	bool ejected;
	bool writable;
	bool speedLimited;
	char op[24];
	char error[80];
};

static void uiPrvUsbStorageReadState(struct UiUsbStorageState *state)
{
	struct UsbMscStatus status;

	memset(state, 0, sizeof(*state));
	usbMscGetStatus(&status);
	state->blocks = sdGetNumSecs();
	state->lba = status.lba;
	state->bytes = status.bytes;
	state->mounted = usbMscMounted();
	state->ejected = usbMscEjected();
	state->writable = usbMscWritable();
	state->speedLimited = status.speedLimited;
	uiPrvCopyStr(state->op, sizeof(state->op), status.op ? status.op : "");
	uiPrvCopyStr(state->error, sizeof(state->error), status.error ? status.error : "");
}

static void uiPrvUsbStorageClearArea(struct Canvas *cnv, uint32_t top, uint32_t bottom)
{
	int8_t foreColor = cnv->foreColor;

	cnv->foreColor = cnv->backColor;
	uiPrvFillRect(cnv, 0, top, cnv->w - 1, bottom);
	cnv->foreColor = foreColor;
}

static void uiPrvUsbStorageClearRow(struct Canvas *cnv, uint_fast8_t row)
{
	uint32_t top = row ? uiPrvMenuRow(cnv, row) : uiPrvContentTop(cnv);

	uiPrvUsbStorageClearArea(cnv, top, top + uiPrvMenuItemHeight(cnv) - 1u);
}

static void uiPrvUsbStorageDrawConnection(struct Canvas *cnv, const struct UiUsbStorageState *state)
{
	uiPuts(cnv, uiPrvContentTop(cnv), 10, state->mounted ? "USB connected" : "Waiting for host", -1);
}

static void uiPrvUsbStorageDrawWritable(struct Canvas *cnv, const struct UiUsbStorageState *state)
{
	uiPuts(cnv, uiPrvMenuRow(cnv, 2), 10, state->writable ? "microSD is read-write" : "microSD is read-only", -1);
}

static void uiPrvUsbStorageDrawCapacity(struct Canvas *cnv, const struct UiUsbStorageState *state)
{
	char msg[96];

	(void)sprintf(msg, "%u MiB exposed", (unsigned)(state->blocks / 2048));
	uiPuts(cnv, uiPrvMenuRow(cnv, 3), 10, msg, -1);
}

static void uiPrvUsbStorageDrawOperation(struct Canvas *cnv, const struct UiUsbStorageState *state)
{
	char msg[96];

	(void)sprintf(msg, "%s LBA %u", state->op, (unsigned)state->lba);
	uiPuts(cnv, uiPrvMenuRow(cnv, 4), 10, msg, -1);
}

static void uiPrvUsbStorageDrawBytes(struct Canvas *cnv, const struct UiUsbStorageState *state)
{
	char msg[96];

	(void)sprintf(msg, "%u bytes %s", (unsigned)state->bytes, state->speedLimited ? "safe speed" : "fast speed");
	uiPuts(cnv, uiPrvMenuRow(cnv, 5), 10, msg, -1);
}

static void uiPrvUsbStorageDrawMessage(struct Canvas *cnv, const struct UiUsbStorageState *state)
{
	if (state->error[0] && strcmp(state->error, "none")) {
		uiPrvDrawWrappedString(cnv, state->error, uiPrvMenuRow(cnv, 6), 10);
	}
	else if (state->ejected)
		uiPuts(cnv, uiPrvMenuRow(cnv, 6), 10, "Host ejected the card", -1);
	else
		uiPrvDrawWrappedString(cnv, "Eject/unmount on the host before exiting.", uiPrvMenuRow(cnv, 6), 10);
}

static void uiPrvUsbStorageDraw(struct Canvas *cnv, const struct UiUsbStorageState *state)
{
	uiPrvSetHeaderTitle("USB Storage");
	uiPrvReset(cnv, false);
	cnv->font = FontMedium;

	uiPrvUsbStorageDrawConnection(cnv, state);
	uiPrvUsbStorageDrawWritable(cnv, state);
	uiPrvUsbStorageDrawCapacity(cnv, state);
	uiPrvUsbStorageDrawOperation(cnv, state);
	uiPrvUsbStorageDrawBytes(cnv, state);
	uiPrvUsbStorageDrawMessage(cnv, state);
	uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "B = Exit", -1);
}

static void uiPrvUsbStorageUpdate(struct Canvas *cnv, const struct UiUsbStorageState *oldState, const struct UiUsbStorageState *state)
{
	uint32_t messageTop = uiPrvMenuRow(cnv, 6);
	uint32_t footerTop = cnv->h - uiPrvGlyphHeight(cnv) - 1u;

	if (oldState->mounted != state->mounted) {
		uiPrvUsbStorageClearRow(cnv, 0);
		uiPrvUsbStorageDrawConnection(cnv, state);
	}
	if (oldState->writable != state->writable) {
		uiPrvUsbStorageClearRow(cnv, 2);
		uiPrvUsbStorageDrawWritable(cnv, state);
	}
	if (oldState->blocks != state->blocks) {
		uiPrvUsbStorageClearRow(cnv, 3);
		uiPrvUsbStorageDrawCapacity(cnv, state);
	}
	if (oldState->lba != state->lba || strcmp(oldState->op, state->op)) {
		uiPrvUsbStorageClearRow(cnv, 4);
		uiPrvUsbStorageDrawOperation(cnv, state);
	}
	if (oldState->bytes != state->bytes || oldState->speedLimited != state->speedLimited) {
		uiPrvUsbStorageClearRow(cnv, 5);
		uiPrvUsbStorageDrawBytes(cnv, state);
	}
	if (oldState->ejected != state->ejected || strcmp(oldState->error, state->error)) {
		uiPrvUsbStorageClearArea(cnv, messageTop, footerTop - 1u);
		uiPrvUsbStorageDrawMessage(cnv, state);
	}
}

static void uiPrvUsbStorageTool(struct Canvas *cnv)
{
	struct UiUsbStorageState state, drawnState;
	bool haveDrawn = false;
	union SdFlags flags;

	uiPrvSetHeaderTitle("USB Storage");
	if (!uiAlert(cnv, "Expose microSD card to the USB host?", DialogTypeYesNo))
		return;

	uiPrvReset(cnv, false);
	uiPrvDrawWrappedString(cnv, "Starting USB Storage...", 32, 10);

	uiPrvCardStreamReset();
	if (!sdGetNumSecs() && (!sdCardInit() || !sdGetNumSecs())) {
		(void)uiPrvCardPreUnmount();
		sdReportLastError();
		uiAlert(cnv, "Insert an SD card, or check that the card can be read", DialogTypeOk);
		return;
	}
	(void)uiPrvCardPreUnmount();

	flags.value = sdGetFlags();
	if (!usbMscBegin(!flags.RO)) {
		char msg[96];

		(void)sprintf(msg, "USB Storage failed to start\n%s", usbMscLastError());
		uiAlert(cnv, msg, DialogTypeOk);
		return;
	}
	uiPrvWaitKeysReleased();

	while (!uiPrvToolExitRequested()) {
		usbMscTask();
		uiPrvRefreshHeaderClock(cnv);
		if ((uiGetKeysRaw() & KEY_BIT_B) || uiPrvCenterExitPressedRaw()) {
			uiPrvWaitKeysReleased();
			if (!usbMscEjected() && !uiAlert(cnv, "The host has not ejected the SD card.\nDisconnect anyway?", DialogTypeYesNo)) {
				uiPrvClearToolExit();
				uiPrvWaitKeysReleased();
				haveDrawn = false;
				continue;
			}
			break;
		}
		uiPrvUsbStorageReadState(&state);
		if (!haveDrawn || memcmp(&state, &drawnState, sizeof(state))) {
			uiPrvBeginPacedRedraw();
			if (haveDrawn)
				uiPrvUsbStorageUpdate(cnv, &drawnState, &state);
			else
				uiPrvUsbStorageDraw(cnv, &state);
			drawnState = state;
			haveDrawn = true;
		}
		timebaseIdleWaitMsec(1u);
	}

	usbMscEnd();
	uiPrvWaitKeysReleased();
}
#endif

static enum UiGameAction mLastGameMenuAction = UiGameActionResume;
static bool mDeferredGameSelect;

static bool uiPrvCurrentGameDefersSelect(void)
{
	struct GameSelection selection;

	return uiGetGameSelection(&selection) &&
		(selection.runtime == GameRuntimeGb ||
		 selection.runtime == GameRuntimeNes ||
		 selection.runtime == GameRuntimeArduboy);
}

static enum UiGameAction uiPrvRunLoadedGame(struct Canvas *cnv, UiRunGameF runGameF, void *userData)
{
	while (1) {
		mLastGameMenuAction = UiGameActionResume;
		mDeferredGameSelect = false;
		uiPrvEnterTool(UiToolGame);
		toolWorkspaceEnd();
		uiPrvLoadSavestate();
		runGameF(userData);
		if (dcAppLastError()[0]) {
			uiAlert(cnv, dcAppLastError(), DialogTypeOk);
			dcAppClearError();
		}
		if (!uiSaveSavestate())
			uiAlert(cnv, "Failed to save state to flash", DialogTypeOk);
		toolWorkspaceBegin();
		uiPrvExitTool(UiToolGame);

	#ifndef NO_SD_CARD
		if (!mDeferredGameSelect && mLastGameMenuAction == UiGameActionSwitchTool) {
			uiPrvExportCurrentSavestateWithOptions(cnv, false, true);
			uiPrvCartRamOwnerClear("switching away from game");
		}

		if (mDeferredGameSelect && mLastGameMenuAction == UiGameActionSelectGame) {
			if (uiPrvSelectRom(cnv, true)) {
				uiPrvLoadSavestate();
				continue;
			}
			if (uiPrvHaveValidRom(NULL, NULL, NULL)) {
				uiPrvLoadSavestate();
				continue;
			}
			return UiGameActionSwitchTool;
		}
	#endif

		return mLastGameMenuAction;
	}
}

static enum UiToolId uiPrvGameTool(struct Canvas *cnv, UiRunGameF runGameF, void *userData)
{
	while (1) {
		enum {
			GameToolOptionRun,
			GameToolOptionConsole,
		};
		static const enum UiEmulatorConsole consoles[] = {
			UiEmulatorConsoleArduboy,
			UiEmulatorConsoleGameboy,
			UiEmulatorConsoleGameboyColor,
			UiEmulatorConsoleNes,
		};
		uint_fast8_t optionIds[6], numOptions = 0, selOption, i;
		enum UiEmulatorConsole optionConsoles[6];
		const char *labels[6];
		char name[ROM_NAME_LEN + 1];
		char title[UI_GAME_TITLE_BUF_SZ];
		bool validRom = uiPrvHaveValidRom(name, NULL, NULL);
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;

		if (uiPrvToolExitRequested())
			return UiToolGame;

		uiPrvSetHeaderTitle("Emulators");
		uiPrvReset(cnv, false);

		if (validRom) {
			labels[numOptions] = NULL;
			optionConsoles[numOptions] = uiPrvCurrentGameConsole();
			optionIds[numOptions++] = GameToolOptionRun;
		}
	#ifndef NO_SD_CARD
		for (i = 0; i < sizeof(consoles) / sizeof(*consoles); i++) {
			labels[numOptions] = uiPrvEmulatorConsoleName(consoles[i]);
			optionConsoles[numOptions] = consoles[i];
			optionIds[numOptions++] = GameToolOptionConsole;
		}
	#endif

		for (i = 0; i < numOptions; i++) {
			if (optionIds[i] == GameToolOptionRun) {
				uiPrvCurrentGameTitle(title, sizeof(title), name);
				uiPrvDrawGameAction(cnv, uiPrvMenuRow(cnv, i), title, true);
			}
			else
				uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, labels[i], -1);
		}

		selOption = uiPrvMenu(cnv, 0, numOptions, &button);
		if (uiPrvToolExitRequested())
			return UiToolGame;
		if (button == KEY_BIT_B)
			return UiToolGame;
		if (optionIds[selOption] == GameToolOptionRun) {
			if (uiPrvRunLoadedGame(cnv, runGameF, userData) == UiGameActionSwitchTool)
				return UiToolGame;
		}
	#ifndef NO_SD_CARD
		else if (optionIds[selOption] == GameToolOptionConsole) {
			if (uiPrvSelectRomForConsole(cnv, false, optionConsoles[selOption])) {
				if (uiPrvRunLoadedGame(cnv, runGameF, userData) == UiGameActionSwitchTool)
					return UiToolGame;
			}
			if (uiPrvToolExitRequested())
				return UiToolGame;
		}
	#endif
	}
}

enum UiGameAction uiGameMenu(void)
{
	struct Canvas canvas = CANVAS_INITIALIZER, *cnv = &canvas;
	enum {
		GameMenuOptionRun,
		GameMenuOptionSelect,
		GameMenuOptionSaveToSd,
		GameMenuOptionSettings,
		GameMenuOptionAudio,
		GameMenuOptionScreen,
		GameMenuOptionLeds,
		GameMenuOptionTheme,
		GameMenuOptionSwitch,
	};
	uint_fast8_t optionIds[9], numOptions = 0, selOption, i;
	const char *labels[9];
	char name[ROM_NAME_LEN + 1];
	char title[UI_GAME_TITLE_BUF_SZ];
	bool validRom = uiPrvHaveValidRom(name, NULL, NULL);
	uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;
	
	uiPrvSetHeaderTitle("FN Menu");
	uiPrvReset(cnv, false);

	if (validRom && !uiSaveSavestate())
		uiAlert(cnv, "Failed to save state to flash", DialogTypeOk);

	if (validRom) {
		labels[numOptions] = NULL;
		optionIds[numOptions++] = GameMenuOptionRun;
	}
#ifndef NO_SD_CARD
	labels[numOptions] = validRom ? "Select game" : "Load game";
	optionIds[numOptions++] = GameMenuOptionSelect;
	if (validRom) {
		labels[numOptions] = "Save to SD";
		optionIds[numOptions++] = GameMenuOptionSaveToSd;
	}
#endif
	labels[numOptions] = "App Settings";
	optionIds[numOptions++] = GameMenuOptionSettings;
	labels[numOptions] = "Audio";
	optionIds[numOptions++] = GameMenuOptionAudio;
	labels[numOptions] = "Display";
	optionIds[numOptions++] = GameMenuOptionScreen;
	labels[numOptions] = "LEDs";
	optionIds[numOptions++] = GameMenuOptionLeds;
	labels[numOptions] = "Theme";
	optionIds[numOptions++] = GameMenuOptionTheme;
	labels[numOptions] = "Exit to Main Menu";
	optionIds[numOptions++] = GameMenuOptionSwitch;

	for (i = 0; i < numOptions; i++) {
		if (optionIds[i] == GameMenuOptionRun) {
			uiPrvCurrentGameTitle(title, sizeof(title), name);
			uiPrvDrawGameAction(cnv, uiPrvMenuRow(cnv, i), title, true);
		}
		else
			uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, labels[i], -1);
	}

	selOption = uiPrvMenu(cnv, 0, numOptions, &button);
	if (uiPrvToolExitRequested()) {
		mLastGameMenuAction = UiGameActionSwitchTool;
		return mLastGameMenuAction;
	}
	if (button == KEY_BIT_B || optionIds[selOption] == GameMenuOptionRun)
		return UiGameActionResume;
#ifndef NO_SD_CARD
	if (optionIds[selOption] == GameMenuOptionSelect) {
		bool selected;

		if (uiPrvCurrentGameDefersSelect()) {
			mDeferredGameSelect = true;
			mLastGameMenuAction = UiGameActionSelectGame;
			return mLastGameMenuAction;
		}

		toolWorkspaceBegin();
		selected = uiPrvSelectRom(cnv, false);
		if (uiPrvToolExitRequested()) {
			toolWorkspaceEnd();
			mLastGameMenuAction = UiGameActionSwitchTool;
			return mLastGameMenuAction;
		}
		if (selected) {
			uiPrvLoadSavestate();
			mLastGameMenuAction = UiGameActionSelectGame;
		}
		else if (uiPrvHaveValidRom(NULL, NULL, NULL)) {
			uiPrvLoadSavestate();
			mLastGameMenuAction = UiGameActionResume;
		}
		else
			mLastGameMenuAction = UiGameActionSwitchTool;
		toolWorkspaceEnd();
		return mLastGameMenuAction;
	}
	if (optionIds[selOption] == GameMenuOptionSaveToSd) {
		uiPrvExportCurrentSavestate(cnv, true);
		mLastGameMenuAction = UiGameActionResume;
		return mLastGameMenuAction;
	}
#endif
	if (optionIds[selOption] == GameMenuOptionSettings) {
		mLastGameMenuAction = uiPrvEditGameSettings(cnv, uiPrvCurrentGameConsole()) ? UiGameActionRestart : UiGameActionResume;
		if (uiPrvToolExitRequested())
			mLastGameMenuAction = UiGameActionSwitchTool;
		return mLastGameMenuAction;
	}
	if (optionIds[selOption] == GameMenuOptionAudio ||
			optionIds[selOption] == GameMenuOptionScreen ||
			optionIds[selOption] == GameMenuOptionLeds ||
			optionIds[selOption] == GameMenuOptionTheme) {
		struct Settings settings;

		settingsGet(&settings);
		if (optionIds[selOption] == GameMenuOptionAudio)
			uiPrvAudioSettings(cnv, &settings);
		else if (optionIds[selOption] == GameMenuOptionScreen)
			uiPrvScreenSettings(cnv, &settings);
		else if (optionIds[selOption] == GameMenuOptionLeds)
			uiPrvLedSettings(cnv, &settings);
		else
			uiPrvThemeSettings(cnv, &settings);
		(void)settingsSet(&settings);
		mLastGameMenuAction = uiPrvToolExitRequested() ?
			UiGameActionSwitchTool : UiGameActionResume;
		return mLastGameMenuAction;
	}
	if (optionIds[selOption] == GameMenuOptionSwitch) {
		mLastGameMenuAction = UiGameActionSwitchTool;
		return mLastGameMenuAction;
	}

	return UiGameActionResume;
}

static void uiPrvFnAppSettingsMenu(struct Canvas *cnv)
{
	const struct UiFnSettings *settings = mUiFnSettings;
	uint_fast8_t selected = 0;

	if (!settings || !settings->count || !settings->labels || !settings->value ||
		!settings->adjust)
		return;
	while (1) {
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;

		uiPrvSetHeaderTitle(settings->title && settings->title[0] ?
			settings->title : "App Settings");
		uiPrvReset(cnv, false);
		for (uint_fast8_t i = 0; i < settings->count; i++) {
			char value[40];

			value[0] = 0;
			settings->value(mUiFnSettingsContext, i, value, sizeof(value));
			uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, settings->labels[i], -1);
			uiPrvDrawTruncText(cnv, uiPrvMenuRow(cnv, i), 135, cnv->w - 145, value);
		}
		uiPuts(cnv, uiPrvMenuRow(cnv, settings->count), 10, "Back", -1);
		selected = uiPrvMenu(cnv, selected, settings->count + 1u, &button);
		if (uiPrvToolExitRequested() || button == KEY_BIT_B || selected == settings->count)
			return;
		if (button == KEY_BIT_LEFT)
			settings->adjust(mUiFnSettingsContext, selected, -1);
		else
			settings->adjust(mUiFnSettingsContext, selected, 1);
	}
}

bool uiPortMenu(struct Canvas *activeCanvas)
{
	struct Canvas canvas = CANVAS_INITIALIZER, *cnv = &canvas;
	enum {
		PortMenuOptionResume,
		PortMenuOptionAppSettings,
		PortMenuOptionAudio,
		PortMenuOptionScreen,
		PortMenuOptionLeds,
		PortMenuOptionMain,
	};
	uint_fast8_t selOption = PortMenuOptionResume;

	uiPrvWaitKeysReleased();
	while (1) {
		const char *labels[6];
		uint_fast8_t optionIds[6], numOptions = 0;
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;

		uiPrvSetHeaderTitle("FN Menu");
		uiPrvReset(cnv, false);
		labels[numOptions] = "Resume";
		optionIds[numOptions++] = PortMenuOptionResume;
		if (mUiFnSettings) {
			labels[numOptions] = "App Settings";
			optionIds[numOptions++] = PortMenuOptionAppSettings;
		}
		labels[numOptions] = "Audio";
		optionIds[numOptions++] = PortMenuOptionAudio;
		labels[numOptions] = "Display";
		optionIds[numOptions++] = PortMenuOptionScreen;
		labels[numOptions] = "LEDs";
		optionIds[numOptions++] = PortMenuOptionLeds;
		labels[numOptions] = "Exit to Main Menu";
		optionIds[numOptions++] = PortMenuOptionMain;
		for (uint_fast8_t i = 0; i < numOptions; i++)
			uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, labels[i], -1);
		if (selOption >= numOptions)
			selOption = 0;
		selOption = uiPrvMenu(cnv, selOption, numOptions, &button);
		if (uiPrvToolExitRequested())
			return false;
		if (button == KEY_BIT_B || optionIds[selOption] == PortMenuOptionResume)
			return true;
		if (optionIds[selOption] == PortMenuOptionMain)
			return false;
		if (optionIds[selOption] == PortMenuOptionAppSettings) {
			uiPrvFnAppSettingsMenu(cnv);
			if (uiPrvToolExitRequested())
				return false;
		}
		if (optionIds[selOption] == PortMenuOptionAudio) {
			struct Settings settings;

			settingsGet(&settings);
			uiPrvAudioSettings(cnv, &settings);
			(void)settingsSet(&settings);
			if (uiPrvToolExitRequested())
				return false;
		}
		if (optionIds[selOption] == PortMenuOptionScreen) {
			struct Settings settings;

			settingsGet(&settings);
			uiPrvScreenSettings(cnv, &settings);
			(void)settingsSet(&settings);
			if (activeCanvas)
				activeCanvas->flipped = settings.rotation;
			if (uiPrvToolExitRequested())
				return false;
		}
		if (optionIds[selOption] == PortMenuOptionLeds) {
			struct Settings settings;

			settingsGet(&settings);
			uiPrvLedSettings(cnv, &settings);
			(void)settingsSet(&settings);
			if (uiPrvToolExitRequested())
				return false;
		}
	}
}

enum UiCategoryEntryKind {
	UiCategoryEntryTool,
	UiCategoryEntrySdApp,
	UiCategoryEntryDemos,
};

struct UiCategoryEntry {
	const char *label;
	enum UiCategoryEntryKind kind;
	enum UiToolId tool;
	enum DcAppId appId;
	const char *detail;
};

static enum UiToolId uiPrvPortsCategoryTool(struct Canvas *cnv, UiRunGameF runGameF, void *userData);
static enum UiToolId uiPrvDemosCategoryTool(struct Canvas *cnv, UiRunGameF runGameF, void *userData);

static void uiPrvCategoryReturnFence(void)
{
	uiPrvClearToolExit();
	uiPrvWaitKeysReleased();
}

static void uiPrvRunCategoryToolEntry(struct Canvas *cnv, enum UiToolId tool, UiRunGameF runGameF, void *userData)
{
	uiPrvSetHeaderTitle(uiPrvToolHeaderTitle(tool));

	switch (tool) {
		case UiToolIr:
		#ifndef NO_SD_CARD
			uiPrvEnterTool(tool);
			(void)uiPrvRunSdApp(cnv, DcAppIdToolIr, DcAppToolActionMain, NULL, NULL, NULL, NULL);
			uiPrvExitTool(tool);
		#else
			uiAlert(cnv, "Universal Remote requires SD card support", DialogTypeOk);
		#endif
			break;

		case UiToolUsbStorage:
		#ifndef NO_SD_CARD
			uiPrvEnterTool(tool);
			uiPrvUsbStorageTool(cnv);
			uiPrvExitTool(tool);
		#else
			uiAlert(cnv, "USB Storage requires SD card support", DialogTypeOk);
		#endif
			break;

		case UiToolHidTest:
			uiPrvEnterTool(tool);
			uiPrvUsbKeyboardTool(cnv);
			uiPrvExitTool(tool);
			break;

		case UiToolAutoclicker:
	#ifndef NO_SD_CARD
			uiPrvEnterTool(tool);
			(void)uiPrvRunSdApp(cnv, DcAppIdToolAutoclicker, DcAppToolActionMain, NULL, NULL, NULL, NULL);
			uiPrvExitTool(tool);
		#else
			uiAlert(cnv, "Autoclicker requires SD card support", DialogTypeOk);
		#endif
			break;

		case UiToolBadUsb:
	#ifndef NO_SD_CARD
			uiPrvEnterTool(tool);
			(void)uiPrvRunSdApp(cnv, DcAppIdToolBadUsb, DcAppToolActionMain, NULL, NULL, NULL, NULL);
			uiPrvExitTool(tool);
	#else
			uiAlert(cnv, "BadUSB requires SD card support", DialogTypeOk);
	#endif
			break;

		case UiToolGamepad:
	#ifndef NO_SD_CARD
			uiPrvEnterTool(tool);
			(void)uiPrvRunSdApp(cnv, DcAppIdToolGamepad, DcAppToolActionMain, NULL, NULL, NULL, NULL);
			uiPrvExitTool(tool);
		#else
			uiAlert(cnv, "USB Gamepad requires SD card support", DialogTypeOk);
		#endif
			break;

		case UiToolMusic:
		#ifndef NO_SD_CARD
			if (!uiPrvMusicBatteryOkToLaunch(cnv))
				break;
			uiPrvEnterTool(tool);
			(void)uiPrvRunSdApp(cnv, DcAppIdToolMusic, DcAppToolActionMain, NULL, NULL, NULL, NULL);
			uiPrvExitTool(tool);
		#else
			uiAlert(cnv, "Music requires SD card support", DialogTypeOk);
		#endif
			break;

		case UiToolImage:
		#ifndef NO_SD_CARD
			uiPrvEnterTool(tool);
			(void)uiPrvRunSdApp(cnv, DcAppIdToolImage, DcAppToolActionMain, NULL, NULL, NULL, NULL);
			uiPrvExitTool(tool);
		#else
			uiAlert(cnv, "Image Viewer requires SD card support", DialogTypeOk);
		#endif
			break;

		case UiToolGame:
			uiPrvEnterTool(tool);
			(void)uiPrvGameTool(cnv, runGameF, userData);
			uiPrvExitTool(tool);
			break;

		case UiToolPorts:
			(void)uiPrvPortsCategoryTool(cnv, runGameF, userData);
			break;

		default:
			break;
	}

	uiPrvCategoryReturnFence();
}

static void uiPrvRunCategorySdAppEntry(struct Canvas *cnv, enum UiToolId ownerTool, const char *label, enum DcAppId appId)
{
#ifndef NO_SD_CARD
	struct FatfsVol *vol;
	struct Settings settings;

	if (appId == DcAppIdDvdBounce || appId == DcAppIdScrollPattern) {
		settingsGet(&settings);
		if (!settings.screenSaverGifPath[0]) {
			uiPrvPickScreenSaverImage(cnv, &settings);
			(void)settingsSet(&settings);
			if (!settings.screenSaverGifPath[0]) {
				uiPrvCategoryReturnFence();
				return;
			}
		}
	}
	vol = uiPrvMountCard(cnv, false);

	if (!vol)
		return;
	uiPrvSetHeaderTitle(label);
	uiPrvEnterTool(ownerTool);
	(void)uiPrvRunSdApp(cnv, appId, DcAppToolActionMain, vol, NULL, NULL, NULL);
	uiPrvExitTool(ownerTool);
	(void)fatfsUnmount(vol);
#else
	(void)appId;
	uiAlert(cnv, "This app requires SD card support", DialogTypeOk);
#endif
	uiPrvCategoryReturnFence();
}

static enum UiToolId uiPrvCategoryTool(struct Canvas *cnv, enum UiToolId categoryTool, const char *title,
	const struct UiCategoryEntry *entries, uint_fast8_t numEntries, uint_fast8_t entryColor,
	UiRunGameF runGameF, void *userData)
{
	while (1) {
		uint_fast8_t i, selOption;
		uint_fast16_t button = KEY_BIT_A | KEY_BIT_B;

		uiPrvSetHeaderTitle(title);
		uiPrvReset(cnv, false);
		for (i = 0; i < numEntries; i++) {
			if (entries[i].detail) {
				uint32_t titleWidth;

				cnv->foreColor = 15;
				titleWidth = uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, entries[i].label, -1);
				cnv->foreColor = 11;
				uiPuts(cnv, uiPrvMenuRow(cnv, i), 10 + titleWidth, entries[i].detail, -1);
			}
			else {
				cnv->foreColor = entryColor;
				uiPuts(cnv, uiPrvMenuRow(cnv, i), 10, entries[i].label, -1);
			}
		}
		cnv->foreColor = entryColor;

		selOption = uiPrvMenu(cnv, 0, numEntries, &button);
		if (uiPrvToolExitRequested() || button == KEY_BIT_B) {
			uiPrvCategoryReturnFence();
			return categoryTool;
		}

		if (entries[selOption].kind == UiCategoryEntryTool)
			uiPrvRunCategoryToolEntry(cnv, entries[selOption].tool, runGameF, userData);
		else if (entries[selOption].kind == UiCategoryEntryDemos)
			(void)uiPrvDemosCategoryTool(cnv, runGameF, userData);
		else
			uiPrvRunCategorySdAppEntry(cnv, categoryTool, entries[selOption].label, entries[selOption].appId);
	}
}

static enum UiToolId uiPrvUsbCategoryTool(struct Canvas *cnv, UiRunGameF runGameF, void *userData)
{
	static const struct UiCategoryEntry entries[] = {
		{"USB Storage", UiCategoryEntryTool, UiToolUsbStorage, 0},
		{"USB Keyboard", UiCategoryEntryTool, UiToolHidTest, 0},
		{"BadUSB", UiCategoryEntryTool, UiToolBadUsb, 0},
		{"Autoclicker", UiCategoryEntryTool, UiToolAutoclicker, 0},
		{"USB Gamepad", UiCategoryEntryTool, UiToolGamepad, 0},
		{"RaspyJack Remote", UiCategoryEntrySdApp, UiToolUsb, DcAppIdToolRaspyJack},
		{"Pwnagotchi Remote", UiCategoryEntrySdApp, UiToolUsb, DcAppIdToolPwnagotchi},
	};

	return uiPrvCategoryTool(cnv, UiToolUsb, "USB", entries, sizeof(entries) / sizeof(*entries),
		15, runGameF, userData);
}

static enum UiToolId uiPrvInfraredCategoryTool(struct Canvas *cnv, UiRunGameF runGameF, void *userData)
{
	static const struct UiCategoryEntry entries[] = {
		{"Universal Remote", UiCategoryEntryTool, UiToolIr, 0},
		{"Laser Tag", UiCategoryEntrySdApp, UiToolInfrared, DcAppIdToolLaserTag},
	};

	return uiPrvCategoryTool(cnv, UiToolInfrared, "Infrared", entries,
		sizeof(entries) / sizeof(*entries), 15, runGameF, userData);
}

static enum UiToolId uiPrvMediaCategoryTool(struct Canvas *cnv, UiRunGameF runGameF, void *userData)
{
	static const struct UiCategoryEntry entries[] = {
		{"Music", UiCategoryEntryTool, UiToolMusic, 0},
		{"Image Viewer", UiCategoryEntryTool, UiToolImage, 0},
		{"Demos", UiCategoryEntryDemos, UiToolMedia, 0},
	};

	return uiPrvCategoryTool(cnv, UiToolMedia, "Media", entries, sizeof(entries) / sizeof(*entries),
		15, runGameF, userData);
}

static enum UiToolId uiPrvDemosCategoryTool(struct Canvas *cnv, UiRunGameF runGameF, void *userData)
{
	static const struct UiCategoryEntry entries[] = {
		{"Starfield", UiCategoryEntrySdApp, UiToolMedia, DcAppIdStarfield},
		{"Spiro", UiCategoryEntrySdApp, UiToolMedia, DcAppIdSpiro},
		{"Cube", UiCategoryEntrySdApp, UiToolMedia, DcAppIdCube},
		{"DVD Bounce", UiCategoryEntrySdApp, UiToolMedia, DcAppIdDvdBounce},
		{"Scrolling Pattern", UiCategoryEntrySdApp, UiToolMedia, DcAppIdScrollPattern},
	};

	return uiPrvCategoryTool(cnv, UiToolMedia, "Demos", entries, sizeof(entries) / sizeof(*entries),
		15, runGameF, userData);
}

static enum UiToolId uiPrvPortsCategoryTool(struct Canvas *cnv, UiRunGameF runGameF, void *userData)
{
	static const struct UiCategoryEntry entries[] = {
		{"Pong", UiCategoryEntrySdApp, UiToolPorts, DcAppIdPong, NULL},
		{"Tetris", UiCategoryEntrySdApp, UiToolPorts, DcAppIdTetris, " (NullpoMino)"},
		{"Arkanoid", UiCategoryEntrySdApp, UiToolPorts, DcAppIdArkanoid, " (wkeeling)"},
		{"Flappy Bird", UiCategoryEntrySdApp, UiToolPorts, DcAppIdFlappy, " (VadimBoev)"},
		{"T-Rex Runner", UiCategoryEntrySdApp, UiToolPorts, DcAppIdTrex, " (wayou)"},
		{"DOOM", UiCategoryEntrySdApp, UiToolPorts, DcAppIdDoom, " (rp2040-doom)"},
		{"Chip's Challenge", UiCategoryEntrySdApp, UiToolPorts, DcAppIdChips, " (Tile World)"},
		{"Scorched Earth", UiCategoryEntrySdApp, UiToolPorts, DcAppIdScorch, " (xscorch)"},
		{"Pipe Dream", UiCategoryEntrySdApp, UiToolPorts, DcAppIdPipe, " (PipeDreamer)"},
		{"Sokoban", UiCategoryEntrySdApp, UiToolPorts, DcAppIdSokoban, " (XSokoban)"},
		{"Jazz Jackrabbit", UiCategoryEntrySdApp, UiToolPorts, DcAppIdOpenJazz, " (OpenJazz)"},
		{"Sensible Soccer", UiCategoryEntrySdApp, UiToolPorts, DcAppIdSoccer, " (YSoccer)"},
	};

	return uiPrvCategoryTool(cnv, UiToolPorts, "Ports", entries, sizeof(entries) / sizeof(*entries),
		15, runGameF, userData);
}

static enum UiToolId uiPrvGamesCategoryTool(struct Canvas *cnv, UiRunGameF runGameF, void *userData)
{
	static const struct UiCategoryEntry entries[] = {
		{"Emulators", UiCategoryEntryTool, UiToolGame, 0},
		{"Ports", UiCategoryEntryTool, UiToolPorts, 0},
	};

	return uiPrvCategoryTool(cnv, UiToolGames, "Games", entries, sizeof(entries) / sizeof(*entries),
		15, runGameF, userData);
}

void uiRunToolShell(UiRunGameF runGameF, void *userData)
{
	struct Canvas canvas = CANVAS_INITIALIZER, *cnv = &canvas;
	struct Settings settings;
	enum UiToolId activeTool = UiToolBrowser;

	settingsGet(&settings);
	uiPrvApplyTheme(&settings);
	uiPrvBootSplash(cnv);
	uiPrvSetHeaderTitle("Main Menu");
	uiPrvReset(cnv, false);
	toolWorkspaceBegin();
	if (bootGuardRecoveredMode() != BootGuardModeNone) {
		pr("boot guard recovered mode %u; starting tool shell\n", bootGuardRecoveredMode());
		uiPrvShowBootRecovery(cnv);
	}
	while (1) {
		activeTool = uiPrvToolSwitcher(cnv, activeTool);
		uiPrvSetHeaderTitle(uiPrvToolHeaderTitle(activeTool));
		mToolExitEnabled = true;
		uiPrvClearToolExit();

		switch (activeTool) {
			case UiToolBrowser:
			#ifndef NO_SD_CARD
				uiPrvEnterTool(activeTool);
				activeTool = uiPrvBrowserTool(cnv, runGameF, userData);
				uiPrvExitTool(UiToolBrowser);
				if (activeTool == UiToolRunGame) {
					(void)uiPrvRunLoadedGame(cnv, runGameF, userData);
					activeTool = UiToolGame;
				}
			#else
				uiAlert(cnv, "File Manager requires SD card support", DialogTypeOk);
			#endif
				break;

			case UiToolIr:
			#ifndef NO_SD_CARD
				uiPrvEnterTool(activeTool);
				(void)uiPrvRunSdApp(cnv, DcAppIdToolIr, DcAppToolActionMain, NULL, NULL, NULL, NULL);
				uiPrvExitTool(activeTool);
			#endif
				break;

			case UiToolInfrared:
				activeTool = uiPrvInfraredCategoryTool(cnv, runGameF, userData);
				break;

			case UiToolUsbStorage:
			#ifndef NO_SD_CARD
				uiPrvEnterTool(activeTool);
				uiPrvUsbStorageTool(cnv);
				uiPrvExitTool(activeTool);
			#else
				uiAlert(cnv, "USB Storage requires SD card support", DialogTypeOk);
			#endif
				break;

			case UiToolBadUsb:
			#ifndef NO_SD_CARD
				uiPrvEnterTool(activeTool);
				(void)uiPrvRunSdApp(cnv, DcAppIdToolBadUsb, DcAppToolActionMain, NULL, NULL, NULL, NULL);
				uiPrvExitTool(activeTool);
			#endif
				break;

			case UiToolUsb:
				activeTool = uiPrvUsbCategoryTool(cnv, runGameF, userData);
				break;

			case UiToolHidTest:
				uiPrvEnterTool(activeTool);
				uiPrvUsbKeyboardTool(cnv);
				uiPrvExitTool(activeTool);
				break;

			case UiToolAutoclicker:
			#ifndef NO_SD_CARD
				uiPrvEnterTool(activeTool);
				(void)uiPrvRunSdApp(cnv, DcAppIdToolAutoclicker, DcAppToolActionMain, NULL, NULL, NULL, NULL);
				uiPrvExitTool(activeTool);
			#else
				uiAlert(cnv, "Autoclicker requires SD card support", DialogTypeOk);
			#endif
				break;

			case UiToolGamepad:
			#ifndef NO_SD_CARD
				uiPrvEnterTool(activeTool);
				(void)uiPrvRunSdApp(cnv, DcAppIdToolGamepad, DcAppToolActionMain, NULL, NULL, NULL, NULL);
				uiPrvExitTool(activeTool);
			#else
				uiAlert(cnv, "USB Gamepad requires SD card support", DialogTypeOk);
			#endif
				break;

			case UiToolMedia:
				activeTool = uiPrvMediaCategoryTool(cnv, runGameF, userData);
				break;

			case UiToolMusic:
			#ifndef NO_SD_CARD
				uiPrvEnterTool(activeTool);
				(void)uiPrvRunSdApp(cnv, DcAppIdToolMusic, DcAppToolActionMain, NULL, NULL, NULL, NULL);
				uiPrvExitTool(activeTool);
			#endif
				break;

			case UiToolImage:
			#ifndef NO_SD_CARD
				uiPrvEnterTool(activeTool);
				(void)uiPrvRunSdApp(cnv, DcAppIdToolImage, DcAppToolActionMain, NULL, NULL, NULL, NULL);
				uiPrvExitTool(activeTool);
			#else
				uiAlert(cnv, "Image Viewer requires SD card support", DialogTypeOk);
			#endif
				break;

			case UiToolGames:
				activeTool = uiPrvGamesCategoryTool(cnv, runGameF, userData);
				break;

			case UiToolPorts:
				activeTool = uiPrvPortsCategoryTool(cnv, runGameF, userData);
				break;

			case UiToolGame:
				uiPrvEnterTool(activeTool);
				activeTool = uiPrvGameTool(cnv, runGameF, userData);
				uiPrvExitTool(UiToolGame);
				break;

			case UiToolRunGame:
				(void)uiPrvRunLoadedGame(cnv, runGameF, userData);
				activeTool = UiToolGame;
				break;

			case UiToolSettings:
				uiPrvEnterTool(activeTool);
				(void)uiPrvSettings(cnv, false);
				uiPrvExitTool(activeTool);
				break;

			case UiToolPowerOff:
				doSleep();
				break;

			case UiToolRefresh:
				activeTool = UiToolBrowser;
				break;

			default:
				activeTool = UiToolBrowser;
				break;
		}
		mToolExitEnabled = false;
		uiPrvClearToolExit();
	}
	
	//we might have changed FPS - reset the counter
	dispPrvFrameCtrReset();
}

void uiSelfTestInit(struct Canvas *cnv, bool inverted, bool flipped)
{
	struct Canvas init = CANVAS_INITIALIZER;

	*cnv = init;
	cnv->flipped = flipped;

	uiPrvReset(cnv, inverted);
}

void uiSelfTestSetText(struct Canvas *cnv, unsigned r, unsigned c, const char *fmt, ...)
{
	va_list vl;

	va_start(vl, fmt);
	uiVprintf(cnv, r, c, fmt, vl);
	va_end(vl);
}

void uiSelfTestSetMarks(struct Canvas *cnv, uint8_t passMask, uint8_t failMask)
{
	uint_fast8_t idx = 0;
	uint32_t eachSpacing = (cnv->w - 9) / 8, eachWidth = eachSpacing - 2;


	for (idx = 0; idx < 8; idx++, passMask >>= 1, failMask >>= 1) {

		bool fail = !!(failMask & 1);
		bool pass = !!(passMask & 1);
		uint32_t color;

		if (fail && pass)
			color = 0b0111101111100000;	//dark yellow
		else if (fail)
			color = 0b0111100000000000;	//dark red
		else if (pass)
			color = 0b0000001111100011;	//blueinsh green
		else
			color = uiPrvGreyToColor(cnv->backColor);

		uiPrvFillRectEx(cnv, eachSpacing * idx, cnv->h * 3 / 4, idx == 7 ? cnv->w : eachSpacing * idx + eachWidth, cnv->h, color);
	}
}
