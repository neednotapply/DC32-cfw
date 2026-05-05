#pragma GCC optimize ("Os")
#include <stdarg.h>
#include <string.h>
#include "gbCartHeader.h"
#include "settings.h"
#include "badgeLeds.h"
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
#include "musicPlayer.h"
#include "rtttlPlayer.h"
#include "audioPwm.h"
#include "timebase.h"
#include "toolWorkspace.h"
#include "utf.h"
#include "ui.h"
#include "sd.h"
#include "gb.h"


#define MENU_SELECTION_CHAR				0xBB /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK */

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

		
		return (ret5 << 11) + (ret6 << 5) + ret5;
	}

#endif


static uint_fast8_t uiPrvGlyphHeight(const struct Canvas *cnv)
{
	return fontGetHeight((enum Font)cnv->font);
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

static uint_fast8_t uiPrvRecvKeypress(void)
{
	uint_fast8_t val, prevVal = 0;
	
	while (!uiGetKeys());
	while ((val = uiGetKeys()) != 0)
		prevVal = val;

	if (prevVal & KEY_BIT_A)
		return KEY_BIT_A;
	if (prevVal & KEY_BIT_B)
		return KEY_BIT_B;
	if (prevVal & KEY_BIT_UP)
		return KEY_BIT_UP;
	if (prevVal & KEY_BIT_DOWN)
		return KEY_BIT_DOWN;
	if (prevVal & KEY_BIT_LEFT)
		return KEY_BIT_LEFT;
	if (prevVal & KEY_BIT_RIGHT)
		return KEY_BIT_RIGHT;
	
	return prevVal;
}

static void uiPrvWaitKeysReleased(void)
{
	while (uiGetKeys());
}

static uint_fast8_t uiPrvMenu(struct Canvas *cnv, uint_fast8_t curChoice, uint_fast8_t numChoices, uint8_t *btnsMaskP /* if passed in, return val is the button that was pressed */)
{
	uint_fast8_t i, itemHeight = uiPrvGlyphHeight(cnv) + 1, row = cnv->h - numChoices * itemHeight, fore = cnv->foreColor, back = cnv->backColor, gotKey;
	uint8_t btnsMask = btnsMaskP ? *btnsMaskP : KEY_BIT_A;
	
	while (1) {
		
		for (i = 0; i < numChoices; i++) {
			
			cnv->foreColor = (i == curChoice) ? fore : back;
			uiPrvDrawOneChar(cnv, row + itemHeight * i, 1, MENU_SELECTION_CHAR);
		}
		
		switch (gotKey = uiPrvRecvKeypress()) {
			case KEY_BIT_DOWN:
				if (curChoice < numChoices - 1)
					curChoice++;
				break;
			
			case KEY_BIT_UP:
				if (curChoice)
					curChoice--;
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
	static const char windowTitle[] = "uGB v1.5.0";
	memset(cnv->framebuffer, invert ? 0xff : 0, cnv->w * cnv->h * DISP_BPP / 8);
	
	//draw title
	cnv->foreColor = invert ? 0 : 9;
	cnv->font = FontLarge;
	uiPrvFillRect(cnv, 0, 0, cnv->w - 1, uiPrvGlyphHeight(cnv));
	cnv->foreColor = invert ? 9 : 0;
	cnv->backColor = invert ? 0 : 9;
	uiPrintf(cnv, 0, (cnv->w - uiPrvCharsWidth(cnv, windowTitle, sizeof(windowTitle) - 1)) / 2, windowTitle);

	//set colors for ui
	cnv->font = FontMedium;
	cnv->foreColor = invert ? 0 : 15;
	cnv->backColor = invert ? 15 : 0;
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
	uint_fast8_t key;

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
	static bool uiPrvCardReadSec(void *diskUserData, uint32_t sec, uint32_t numSec, void *dstP)
	{
		uint8_t *dst = (uint8_t*)dstP;
		
		if (!numSec)
			return false;
		
		if (mCurCardOp == CurrentlyWriting) {
			
			if (!sdWriteStop())
				return false;
			
			mCurCardOp = CurentlyIdle;
		}
		else if (mCurCardOp == CurrentlyReading && mCurCardSec != sec) {
			
			if (!sdReadStop())
				return false;
			
			mCurCardOp = CurentlyIdle;
		}
		
		if (mCurCardOp == CurentlyIdle) {
			
			if (!sdReadStart(sec, 0))
				return false;
			
			mCurCardSec = sec;
			mCurCardOp = CurrentlyReading;
		}
		
		//we are now in the reading state
		while (numSec) {
			
			if (!sdReadNext(dst))
				return false;
			
			dst += SD_BLOCK_SIZE;
			numSec--;
			mCurCardSec++;
		}
		
		return true;
	}
	
	static bool uiPrvCardWriteSec(void *diskUserData, uint32_t sec, uint32_t numSec, const void *srcP)
	{
		const uint8_t *src = (const uint8_t*)srcP;
		
		if (!numSec)
			return false;
		
		if (mCurCardOp == CurrentlyReading) {
			
			if (!sdReadStop())
				return false;
			
			mCurCardOp = CurentlyIdle;
		}
		else if (mCurCardOp == CurrentlyWriting && mCurCardSec != sec) {
			
			if (!sdWriteStop())
				return false;
			
			mCurCardOp = CurentlyIdle;
		}
		
		if (mCurCardOp == CurentlyIdle) {
			
			if (!sdWriteStart(sec, 0))
				return false;
			
			mCurCardSec = sec;
			mCurCardOp = CurrentlyWriting;
		}
		
		//we are now in the writing state
		while (numSec) {
			
			if (!sdWriteNext(src))
				return false;
			
			src += SD_BLOCK_SIZE;
			numSec--;
			mCurCardSec++;
		}
		
		return true;
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
	
	static struct FatfsVol* uiPrvMountCard(struct Canvas *cnv, bool quiet)
	{
		struct FatfsVol *vol;
		
		if (!sdGetNumSecs()) {
			
			if (!sdCardInit() || !sdGetNumSecs()) {
				if (!quiet)
					uiAlert(cnv, "Insert an SD card, or check that the card can be read", DialogTypeOk);
				return NULL;	
			}
		}
		
		vol = fatfsMount(uiPrvCardReadSec, uiPrvCardWriteSec, NULL);
		if (vol)
			return vol;
		
		if (!quiet)
			uiAlert(cnv, "Cannot find a valid FAT filesystem on the SD card", DialogTypeOk);
		return NULL;
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
			
			ac = utfToCaseless(ac);
			bc = utfToCaseless(bc);
			
			if (ac != bc)
				return ac - bc;
		}
		return 0;
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
		
		dir = fatfsDirOpen(vol, "/ROM");
		
		if (!dir) {
			
			uiAlert(cnv, "Cannot find a ROM directory on the SD card", DialogTypeOk);
			return 0;
		}
		
		while (fatfsDirRead(dir, fname, &fileSz, &attrs, NULL)) {
			
			unsigned nameLen;
			
			//pr(" file '%s', %lu bytes, attr %02xh\n", fname, fileSz, attrs);
			
			if (attrs & (FATFS_ATTR_VOL_LBL | FATFS_ATTR_DIR))
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
			uiAlert(cnv, "No ROMs found in /ROM on the card", DialogTypeOk);
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

#ifndef NO_SD_CARD
	static void uiPrvCopyStr(char *dst, uint32_t dstLen, const char *src);

	typedef bool (*UiFileNameFilterF)(const char *name);

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

	static bool uiPrvIsDotDir(const char *name)
	{
		return name[0] == '.' && (!name[1] || (name[1] == '.' && !name[2]));
	}

	static bool uiPrvFileListAppend(struct UiFileListCtx *ctx, const char *fname, uint32_t fileSz, bool isDir, const struct FatFileLocator *locator)
	{
		uint32_t nameLen = strlen(fname), spaceNeeded = (sizeof(struct MusicOption) + nameLen + 1 + 3) &~ 3;

		if (spaceNeeded > ctx->spaceAvail) {
			ctx->overflow = true;
			return false;
		}

		ctx->nextAvail->prev = ctx->tail;
		ctx->nextAvail->next = NULL;
		ctx->nextAvail->locator = *locator;
		ctx->nextAvail->size = fileSz;
		ctx->nextAvail->isDir = isDir;
		memcpy(ctx->nextAvail->name, fname, nameLen + 1);
		if (ctx->tail)
			ctx->tail->next = ctx->nextAvail;
		else
			ctx->head = ctx->nextAvail;
		ctx->tail = ctx->nextAvail;
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
		uint32_t openWidth, closeWidth, stringLen, nameMaxWidth, nameWidth, numCharsFit;
		static const char open[] = "[", close[] = "]", truncInd[] = "...";

		openWidth = uiPrvCharsWidth(cnv, open, sizeof(open) - 1);
		closeWidth = uiPrvCharsWidth(cnv, close, sizeof(close) - 1);
		if (maxWidth <= openWidth + closeWidth)
			return;

		uiPuts(cnv, r, c, open, sizeof(open) - 1);
		c += openWidth;
		nameMaxWidth = maxWidth - openWidth - closeWidth;
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
		uiPuts(cnv, r, c, close, sizeof(close) - 1);
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

	static bool uiPrvPickEntryRead(struct FatfsDir *dir, UiFileNameFilterF filterF, struct UiPickEntry *entry)
	{
		uint8_t attrs;

		while (fatfsDirRead(dir, entry->name, &entry->size, &attrs, &entry->locator)) {
			if (attrs & FATFS_ATTR_VOL_LBL)
				continue;
			if (attrs & FATFS_ATTR_DIR) {
				if (!uiPrvIsDotDir(entry->name)) {
					entry->isDir = true;
					return true;
				}
				continue;
			}
			if (!filterF || filterF(entry->name)) {
				entry->isDir = false;
				return true;
			}
		}

		return false;
	}

	static uint32_t uiPrvPickEntryCount(struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *dirLoc, UiFileNameFilterF filterF, struct UiPickEntry *entry)
	{
		struct FatfsDir *dir = uiPrvPickDirOpen(vol, rootPath, dirLoc);
		uint32_t count = 0;

		if (!dir)
			return 0;

		while (uiPrvPickEntryRead(dir, filterF, entry)) {
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

		while (uiPrvPickEntryRead(dir, filterF, entry)) {
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

	static bool uiPrvPickFile(struct Canvas *cnv, struct FatfsVol *vol, const char *rootPath, UiFileNameFilterF filterF, const char *emptyMsg, struct FatFileLocator *locatorOut, char *nameOut, uint32_t nameOutSz, char *parentPathOut, uint32_t parentPathOutSz)
	{
		struct FatFileLocator dirStack[UI_BROWSER_MAX_DEPTH];
		uint16_t pathLenStack[UI_BROWSER_MAX_DEPTH];
		struct ToolWorkspaceSpan pathMem = toolWorkspaceGet(ToolWorkspaceCartRamUpper);
		struct ToolWorkspaceSpan entryMem = toolWorkspaceGet(ToolWorkspaceCartRamLower);
		char *path = (char*)pathMem.ptr;
		struct UiPickEntry *entry = (struct UiPickEntry*)entryMem.ptr;
		uint32_t numItems, topItem = 0, selectedItem = 0, depth = 0, prevTopItem, prevSelOnscreenItem;
		uint_fast8_t itemHeight, itemsOnscreen, pathTop, listTop, itemLeft;
		bool haveDirLoc = false;
		struct FontGlyphInfo gi;

		if (!path || pathMem.size < UI_PICK_FILE_PATH_BUF_SZ || !entry || entryMem.size < sizeof(*entry)) {
			uiAlert(cnv, "Tool workspace is too small for file browser", DialogTypeOk);
			return false;
		}

		uiPrvCopyStr(path, UI_PICK_FILE_PATH_BUF_SZ, rootPath);
reload_dir:
		numItems = uiPrvPickEntryCount(vol, rootPath, haveDirLoc ? &dirStack[depth - 1] : NULL, filterF, entry);
		if (!numItems && !depth) {
			uiAlert(cnv, emptyMsg, DialogTypeOk);
			return false;
		}

		topItem = 0;
		selectedItem = 0;
		itemHeight = uiPrvGlyphHeight(cnv) + 1;
		itemLeft = fontGetGlyphInfo(&gi, cnv->font, MENU_SELECTION_CHAR) ? gi.width + 2 : 10;
		pathTop = fontGetHeight(FontLarge) + fontGetHeight(FontMedium) / 4;
		listTop = pathTop + itemHeight;
		itemsOnscreen = (cnv->h - listTop) / itemHeight;
		if (!itemsOnscreen) {
			uiAlert(cnv, "Display area too small for file browser", DialogTypeOk);
			return false;
		}
		if (itemsOnscreen > numItems + (depth ? 1 : 0))
			itemsOnscreen = numItems + (depth ? 1 : 0);
		prevTopItem = topItem + 1;
		prevSelOnscreenItem = selectedItem - topItem + 1;

		while (1) {
			uint32_t i, totalItems = numItems + (depth ? 1 : 0), selectedOnscreenItem = selectedItem - topItem;

			if (prevTopItem != topItem) {
				uint_fast8_t firstRow = 0, scrollWidth;
				uint32_t skipItems;

				uiPrvReset(cnv, false);
				uiPrvDrawTruncText(cnv, pathTop, 10, cnv->w - 10, path);
				scrollWidth = totalItems > itemsOnscreen ? uiPrvDrawScrollbar(cnv, listTop, totalItems, topItem, itemsOnscreen) : 0;

				if (depth && !topItem) {
					cnv->foreColor = 12;
					uiPrvDrawTruncText(cnv, listTop, itemLeft, cnv->w - scrollWidth - itemLeft, "[..]");
					firstRow = 1;
					skipItems = 0;
				}
				else
					skipItems = topItem - (depth ? 1 : 0);

				cnv->foreColor = 12;
				{
					struct FatfsDir *drawDir = uiPrvPickDirOpen(vol, rootPath, haveDirLoc ? &dirStack[depth - 1] : NULL);

					if (drawDir) {
						for (i = 0; i < skipItems && uiPrvPickEntryRead(drawDir, filterF, entry); i++) {
						}
						for (i = firstRow; i < itemsOnscreen && uiPrvPickEntryRead(drawDir, filterF, entry); i++) {
							if (entry->isDir)
								uiPrvDrawDirLabel(cnv, listTop + i * itemHeight, itemLeft, cnv->w - scrollWidth - itemLeft, entry->name);
							else
								uiPrvDrawTruncText(cnv, listTop + i * itemHeight, itemLeft, cnv->w - scrollWidth - itemLeft, entry->name);
						}
						fatfsDirClose(drawDir);
					}
				}

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

			switch (uiPrvRecvKeypress()) {
				case KEY_BIT_A:
					if (depth && selectedItem == 0) {
						depth--;
						haveDirLoc = depth != 0;
						path[pathLenStack[depth]] = 0;
						goto reload_dir;
					}
					if (!uiPrvPickEntryAt(vol, rootPath, haveDirLoc ? &dirStack[depth - 1] : NULL, filterF, selectedItem - (depth ? 1 : 0), entry))
						break;
					if (entry->isDir) {
						uint32_t pathLen = strlen(path), nameLen = strlen(entry->name);

						if (depth < UI_BROWSER_MAX_DEPTH) {
							pathLenStack[depth] = pathLen;
							dirStack[depth++] = entry->locator;
							haveDirLoc = true;
							if (pathLen + 1 < UI_PICK_FILE_PATH_BUF_SZ) {
								path[pathLen++] = '/';
								if (pathLen + nameLen < UI_PICK_FILE_PATH_BUF_SZ)
									memcpy(path + pathLen, entry->name, nameLen + 1);
								else if (pathLen + 4 <= UI_PICK_FILE_PATH_BUF_SZ)
									strcpy(path + pathLen, "...");
								else
									path[pathLen - 1] = 0;
							}
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
					break;

				case KEY_BIT_B:
					if (depth) {
						depth--;
						haveDirLoc = depth != 0;
						path[pathLenStack[depth]] = 0;
						goto reload_dir;
					}
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
	uint_fast8_t itemHeight;

	if (settings->ledMode >= LedModeNumModes)
		settings->ledMode = LedModeOff;
	if (settings->ledSpeed < LED_SPEED_MENU_MIN || settings->ledSpeed > LED_SPEED_MENU_MAX)
		settings->ledSpeed = 4;
	settings->ledBrightness = uiPrvLedBrightnessFromMenu(uiPrvLedBrightnessToMenu(settings->ledBrightness));

	uiPrvReset(cnv, false);
	itemHeight = uiPrvGlyphHeight(cnv) + 1;

	while (1) {

		int_fast8_t numOptions = 0, doneOption, ledsOption, ledRedOption, ledGreenOption, ledBlueOption, ledSpeedOption, ledBrightnessOption;
		uint8_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;

		uiPrvReset(cnv, false);

		doneOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "DONE", -1);

		ledsOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "LEDS:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, cnv->h - numOptions * itemHeight, 111, "%s        ", badgeLedsModeName(settings->ledMode));

		ledRedOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "LED RED:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, cnv->h - numOptions * itemHeight, 111, "%u         ", uiPrvLedColorToMenu(settings->ledRed));

		ledGreenOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "LED GREEN:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, cnv->h - numOptions * itemHeight, 111, "%u         ", uiPrvLedColorToMenu(settings->ledGreen));

		ledBlueOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "LED BLUE:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, cnv->h - numOptions * itemHeight, 111, "%u         ", uiPrvLedColorToMenu(settings->ledBlue));

		ledSpeedOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "LED SPEED:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, cnv->h - numOptions * itemHeight, 111, "%u         ", settings->ledSpeed);

		ledBrightnessOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "LED BRIGHT:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, cnv->h - numOptions * itemHeight, 111, "%u         ", uiPrvLedBrightnessToMenu(settings->ledBrightness));

		selOption = numOptions - 1 - uiPrvMenu(cnv,  numOptions - 1 - selOption, numOptions, &button);
		if (button == KEY_BIT_B || selOption == doneOption)
			return;

		if (selOption == ledsOption) {

			if (button == KEY_BIT_LEFT) {
				if (settings->ledMode)
					settings->ledMode--;
				else
					continue;
			}
			else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
				if (settings->ledMode < LedModeNumModes - 1)
					settings->ledMode++;
				else
					settings->ledMode = LedModeOff;
			}
			badgeLedsApplySettings(settings, true);
		}

		if (selOption == ledRedOption || selOption == ledGreenOption || selOption == ledBlueOption) {
			uint8_t *valP = (selOption == ledRedOption) ? &settings->ledRed : ((selOption == ledGreenOption) ? &settings->ledGreen : &settings->ledBlue);
			uint_fast8_t color = uiPrvLedColorToMenu(*valP);

			if (button == KEY_BIT_LEFT) {
				if (color)
					color--;
				else
					continue;
			}
			else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
				if (color < LED_COLOR_MENU_MAX)
					color++;
				else
					continue;
			}

			*valP = uiPrvLedColorFromMenu(color);
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

static void __attribute__((noinline)) uiPrvScreenSettings(struct Canvas *cnv, struct Settings *settings)
{
	int_fast8_t selOption = 0;
	uint_fast8_t itemHeight;

	uiPrvReset(cnv, false);
	itemHeight = uiPrvGlyphHeight(cnv) + 1;

	while (1) {

		int_fast8_t numOptions = 0, doneOption, contrastOption = -1, brightnessOption = -1, rotationOption;
		uint8_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;

		uiPrvReset(cnv, false);

		doneOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "DONE", -1);

	#ifdef DISP_CONTRAST_ADJUSTABLE
		contrastOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "CONTRAST:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, cnv->h - numOptions * itemHeight, 111, "%u         ", settings->contrast);
	#endif

		rotationOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "ROTATION:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 111, settings->rotation ? "FLIPPED  " : "NORMAL   ", -1);

	#ifdef DISP_BRIGHTNESS_ADJUSTABLE
		brightnessOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "BRIGHTNESS:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, cnv->h - numOptions * itemHeight, 111, "%u         ", settings->brightness);
	#endif

		selOption = numOptions - 1 - uiPrvMenu(cnv,  numOptions - 1 - selOption, numOptions, &button);
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
		}
	}
}

static bool __attribute__((noinline)) uiPrvGameSettings(struct Canvas *cnv, struct Settings *settings)
{
	bool restartCurGame = false;
	int_fast8_t selOption = 0;
	uint_fast8_t itemHeight;

	uiPrvReset(cnv, false);
	itemHeight = uiPrvGlyphHeight(cnv) + 1;

	while (1) {

		int_fast8_t numOptions = 0, doneOption, cgbOption, upscaleOption, speedOption;
		uint8_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;
		static const char speedNames[][8] = DISP_SPEED_NAMES;
		static const uint8_t speedSettings[] = DISP_SPEED_SETTINGS;
		uint_fast8_t numSpeeds = sizeof(speedSettings) / sizeof(*speedSettings);

		if (settings->speed >= numSpeeds)
			settings->speed = 1;

		uiPrvReset(cnv, false);

		doneOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "DONE", -1);

		cgbOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "COLOR:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 111, settings->actLikeGBC ? "YES       " : "NO       ", -1);

		upscaleOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "UPSCALE:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 111, settings->upscale ? "YES       " : "NO       ", -1);

		speedOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "SPEED:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 111, speedNames[settings->speed], sizeof(speedNames[settings->speed]));

		selOption = numOptions - 1 - uiPrvMenu(cnv,  numOptions - 1 - selOption, numOptions, &button);
		if (button == KEY_BIT_B || selOption == doneOption)
			return restartCurGame;

		if (selOption == speedOption) {

			if (button == KEY_BIT_LEFT) {
				if (settings->speed)
					settings->speed--;
				else
					settings->speed = numSpeeds - 1;
			}
			else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
				if ((uint_fast8_t)(settings->speed + 1) < numSpeeds)
					settings->speed++;
				else
					settings->speed = 0;
			}

			dispOff();
			dispInit(speedSettings[settings->speed]);
			dispSetContrast(settings->contrast);		//must be reset
		}

		if (selOption == upscaleOption) {

			settings->upscale = !settings->upscale;
		}

		if (selOption == cgbOption) {

			restartCurGame = true;
			settings->actLikeGBC = !settings->actLikeGBC;
		}
	}
}

static bool __attribute__((noinline)) uiPrvSettings(struct Canvas *cnv)		//return true if anything for the current game may have changes
{
	bool restartCurGame = false;
	struct Settings settings;
	uint_fast8_t itemHeight;

	settingsGet(&settings);
	if (settings.ledMode >= LedModeNumModes)
		settings.ledMode = LedModeOff;
	if (settings.ledSpeed < LED_SPEED_MENU_MIN || settings.ledSpeed > LED_SPEED_MENU_MAX)
		settings.ledSpeed = 4;

	uiPrvReset(cnv, false);
	itemHeight = uiPrvGlyphHeight(cnv) + 1;

	while (1) {
		uint_fast8_t screenOption = 0, gameOption = 1, ledSettingsOption = 2, doneOption = 3, selOption;
		uint8_t button = KEY_BIT_A | KEY_BIT_B;

		uiPrvReset(cnv, false);
		uiPuts(cnv, cnv->h - 4 * itemHeight, 10, "Screen", -1);
		uiPuts(cnv, cnv->h - 3 * itemHeight, 10, "Game", -1);
		uiPuts(cnv, cnv->h - 2 * itemHeight, 10, "LEDs", -1);
		uiPuts(cnv, cnv->h - 1 * itemHeight, 10, "DONE", -1);

		selOption = uiPrvMenu(cnv, 0, 4, &button);
		if (button == KEY_BIT_B || selOption == doneOption) {
			settingsSet(&settings);
			return restartCurGame;
		}
		if (selOption == screenOption)
			uiPrvScreenSettings(cnv, &settings);
		else if (selOption == gameOption)
			restartCurGame = uiPrvGameSettings(cnv, &settings) || restartCurGame;
		else if (selOption == ledSettingsOption)
			uiPrvLedSettings(cnv, &settings);
	}
}

static void uiPrvLoadSavestate(void)
{
	uint32_t romSzExpected, ramSzExpected;
	
	if (mbcRomAnalyze((const void*)QSPI_ROM_START, &romSzExpected, &ramSzExpected, NULL, NULL) && ramSzExpected < QSPI_RAM_SIZE_MAX)
		memcpy(CART_RAM_ADDR_IN_RAM, (const void*)QSPI_RAM_COPY_START, ramSzExpected);
	else
		memset(CART_RAM_ADDR_IN_RAM, 0xff, QSPI_RAM_SIZE_MAX);
}

bool uiSaveSavestate(void)
{
	uint32_t romSzExpected, ramSzExpected;
	
	if (!mbcRomAnalyze((const void*)QSPI_ROM_START, &romSzExpected, &ramSzExpected, NULL, NULL))
		return false;
	
	if (ramSzExpected && memcmp((const void*)QSPI_RAM_COPY_START, CART_RAM_ADDR_IN_RAM, ramSzExpected)) {
		
		uint32_t writeSz = (ramSzExpected + QSPI_WRITE_GRANULARITY - 1) / QSPI_WRITE_GRANULARITY * QSPI_WRITE_GRANULARITY;
		uint32_t erzSz = (ramSzExpected + QSPI_ERASE_GRANULARITY - 1) / QSPI_ERASE_GRANULARITY * QSPI_ERASE_GRANULARITY;
		
		if (!flashWrite(QSPI_RAM_COPY_START, erzSz, CART_RAM_ADDR_IN_RAM, writeSz))
			return false;
	}
	
	return true;
}

#ifndef NO_SD_CARD
	#define IR_POWER_FILE			"/IR/POWER.IR"
	#define IR_FLIPPER_TV_FILE		"/IR/tv.ir"
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

	struct IrBlastStats {
		uint32_t sent;
		uint32_t skipped;
		uint32_t malformed;
		uint32_t lineNo;
		bool cancelled;
		bool lineTooLong;
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
		bool hasName;
		bool hasFrequency;
		bool hasAddress;
		bool hasCommand;
		bool sentRaw;
	};

	static bool uiPrvEraseGamePath(void)		//this will also mark the ROM as invalid
	{
		return flashWrite(QSPI_FILENAME_START, QSPI_FILENAME_MAXLEN, NULL, 0);
	}
	
	static bool uiPrvSetGamePath(const char *buf)		//buf may be over-read, also marks ROM as possibly valid
	{
		return flashWrite(QSPI_FILENAME_START, QSPI_FILENAME_MAXLEN, buf, (strlen(buf) + 1 + QSPI_WRITE_GRANULARITY - 1) / QSPI_WRITE_GRANULARITY * QSPI_WRITE_GRANULARITY);
	}
	
	static bool uiPrvLoadFile(struct Canvas *cnv, struct FatfsFil *fil, uint32_t flashAddr, const char *nameStr)
	{
		uint32_t row, now, nowDone, pos, totalSz = fatfsFileGetSize(fil), bufSz = 32768;
		struct ToolWorkspaceSpan bufMem = toolWorkspaceGet(ToolWorkspaceWram);
		uint8_t *buf = bufMem.ptr;

		if (bufSz > bufMem.size)
			bufSz = bufMem.size;
		
		cnv->font = FontLarge;
		row = uiPrvGlyphHeight(cnv) + 1;
		uiPrvReset(cnv, false);
		
		uiPrintf(cnv, row, 10, "Loading %s...", nameStr);
		cnv->foreColor = 15;
		uiPrvFillRect(cnv, row + 8, 50, 131, 60);
		cnv->foreColor = 0;
		uiPrvFillRect(cnv, row + 9, 51, 130, 59);
		cnv->foreColor = 10;
		
		for (pos = 0; pos < totalSz; pos += now, flashAddr += now) {
			
			now = totalSz - pos;
			if (now > bufSz)
				now = bufSz;
			
			if (now < QSPI_WRITE_GRANULARITY)	//only possible at the end - do not leak garbage
				memset(buf + now, 0, QSPI_WRITE_GRANULARITY - now);

			if (!fatfsFileRead(fil, buf, now, &nowDone) || now != nowDone) {
				uiAlert(cnv, "File reading failure", DialogTypeOk);
				return false;
			}
	
			//over-erasing up to boundary is safe, same for writing
			if (!flashWrite(flashAddr, (now + QSPI_ERASE_GRANULARITY - 1) / QSPI_ERASE_GRANULARITY * QSPI_ERASE_GRANULARITY, buf, (now + QSPI_WRITE_GRANULARITY - 1) / QSPI_WRITE_GRANULARITY * QSPI_WRITE_GRANULARITY)) {
				uiAlert(cnv, "Flash writing failure", DialogTypeOk);
				return false;
			}
	
			uiPrvFillRect(cnv, row + 10, 52, 30 + pos * 100 / totalSz, 58);
		}
		
		return true;
	}
	
	static bool __attribute__((noinline)) uiPrvConfirmRomSelection(struct Canvas *cnv, struct FatfsVol *vol, const char *romName)
	{
		uint32_t numRead, fileSz, romSzExpected, ramSzExpected, col = 1, row = 17;
		char internalName[ROM_NAME_LEN + 1];
		struct FatfsFil *filR, *filS = NULL;
		enum RomColorSupport colorSupport;
		struct FatfsDir *dir;
		struct CartHeader hdr;
		bool ret = false;
		
		static const char colorTypes[][10] = {
			[RomNoColor] = "B&W",
			[RomColorEnhanced] = "Supported",
			[RomColorRequired] = "Required",
		};
			
		
		dir = fatfsDirOpen(vol, "/ROM");
		if (!dir)
			goto out;
		filR = fatfsFileOpenAt(dir, romName, OPEN_MODE_READ);
		fatfsDirClose(dir);
		dir = NULL;
		if (!filR) {
			uiAlert(cnv, "Cannot open ROM file", DialogTypeOk);
			goto out;
		}
		
		dir = fatfsDirOpen(vol, "/SAVE");
		if (dir) {
			filS = fatfsFileOpenAt(dir, romName, OPEN_MODE_READ);
			fatfsDirClose(dir);
		}
		
		fileSz = fatfsFileGetSize(filR);
		
		if (fileSz < sizeof(hdr) || !fatfsFileRead(filR, &hdr, sizeof(hdr), &numRead) || numRead != sizeof(hdr)) {
			
			uiAlert(cnv, "Cannot read ROM file", DialogTypeOk);
			goto out_close_file;
		}
		
		if (!mbcRomAnalyze(&hdr, &romSzExpected, &ramSzExpected, &colorSupport, internalName)) {
			uiAlert(cnv, "Does not appear to be a valid ROM file", DialogTypeOk);
			goto out_close_file;
		}
	
		if (fileSz != romSzExpected) {
		
			uiAlert(cnv, "ROM file size does not match the expected size", DialogTypeOk);
			goto out_close_file;
		}
		
		if (filS && fatfsFileGetSize(filS) != ramSzExpected) {
			
			pr("Savegame file size does not match the expected size. It will not be loaded. Expected %u, file is %u\n", ramSzExpected, fatfsFileGetSize(filS));
			uiAlert(cnv, "Savegame file size does not match the expected size. It will not be loaded.", DialogTypeOk);
			fatfsFileClose(filS);
			filS = NULL;
		}
	
		if (!fatfsFileSeek(filR, 0)) {
			
			uiAlert(cnv, "Cannot seek ROM file", DialogTypeOk);
			goto out_close_file;
		}

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
		
		cnv->foreColor = 10;
		uiPuts(cnv, row, col, "SAVE:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, row, col + 55, filS ? "FOUND" : "NOT FOUND", -1);
		row += 1 + uiPrvGlyphHeight(cnv);
		
		if (uiPrvGetSimpleAnswer(cnv, DialogTypeYesNo)) {
			
			//erase old path and thus mark the ROM as invalid. This will prevent a poweroff mid-load from causing us to try to play a half-loaded ROM
			(void)uiPrvEraseGamePath();
			
			ret = uiPrvLoadFile(cnv, filR, QSPI_ROM_START, "ROM");
			
			if (!ret) {
				//erase rom header if we failed to load the ROM fully
				(void)flashWrite(QSPI_ROM_START, QSPI_ERASE_GRANULARITY, NULL, 0);
				goto out_close_file;
			}
			
			//ROM is loaded
			(void)uiPrvSetGamePath(romName);
			
			if (filS)
				ret = uiPrvLoadFile(cnv, filS, QSPI_RAM_COPY_START, "SAVE") && ret;
			else
				ret = flashWrite(QSPI_RAM_COPY_START, QSPI_RAM_SIZE_MAX, NULL, 0) && ret;
			
			uiPrvReset(cnv, false);
		}
		
	out_close_file:
		fatfsFileClose(filR);
		if (filS)
			fatfsFileClose(filS);
		
	out:
		return ret;
	}

	static bool uiPrvExportSavestate(struct FatfsVol *vol, uint32_t savegameExportSz)
	{
		struct FatfsDir *saveDir = fatfsDirOpen(vol, "/SAVE");
		struct FatfsFil *fil;
		bool ret = true;
	
		if (!saveDir) {		//we might have ti make the savedir
			
			struct FatFileLocator loc;
			
			if (!fatfsDirCreate(vol, "/SAVE", &loc) || !(saveDir = fatfsDirOpenWithLocator(vol, &loc)))
				return false;
		}
		
		fil = fatfsFileOpenAt(saveDir, (const char*)QSPI_FILENAME_START, OPEN_MODE_WRITE | OPEN_MODE_CREATE);
		if (fil) {
			
			uint32_t bytesWritten;
					
			if (fatfsFileGetSize(fil) > savegameExportSz)
				ret = ret && fatfsFileTruncate(fil, savegameExportSz);
					
			ret = ret && fatfsFileWrite(fil, (const void*)QSPI_RAM_COPY_START, savegameExportSz, &bytesWritten) && bytesWritten == savegameExportSz;
			
			ret = fatfsFileClose(fil) && ret;
		}
		
		fatfsDirClose(saveDir);
		
		return ret;
	}
	
	static bool uiPrvSelectRom(struct Canvas *cnv, uint32_t savegameExportSz)	//corrupts GB's RAM, returns true if a selection was made
	{
		uint32_t i, listTop, itemLeft, numRoms, topItem = 0, selectedOnscreenItem = 0, prevTopItem = 1, prevSelOnscreenItem = 1, curGlobalIdx;
		uint_fast8_t itemHeight, itemsOnscreen;
		struct RomOption *head, *tail, *cur;
		struct FontGlyphInfo gi;
		struct FatfsVol *vol;
		bool ret = false;
		
		vol = uiPrvMountCard(cnv, false);
		if (!vol)
			return false;
		
		cnv->font = FontLarge;
		listTop = uiPrvGlyphHeight(cnv) + 1;
		uiPrvReset(cnv, false);
		itemLeft = fontGetGlyphInfo(&gi, cnv->font, MENU_SELECTION_CHAR) ? gi.width + 2 : 10;

		//rom selection will corrupt our cart RAM, save it
		if (savegameExportSz && !uiPrvExportSavestate(vol, savegameExportSz))
			uiAlert(cnv, "Failed to write current savegame out to card. If you load another game, it will be lost", DialogTypeOk);
		
		itemHeight = uiPrvGlyphHeight(cnv) + 1;
		itemsOnscreen = (cnv->h - listTop) / itemHeight;
		
		numRoms = uiPrvListRoms(cnv, vol, &head, &tail);
		if (!numRoms)
			return false;
		
		sortNames(&head, &tail, numRoms);
			
		if (itemsOnscreen > numRoms)
			itemsOnscreen = numRoms;
		
		while (1) {
			
			uint_fast8_t scrollWidth;
			
			curGlobalIdx = topItem + selectedOnscreenItem;
			for (cur = head, i = 0; i < topItem; i++, cur = cur->next);
			
			if (prevTopItem != topItem) {
				
				uiPrvReset(cnv, false);
				
				scrollWidth = uiPrvDrawScrollbar(cnv, listTop, numRoms, topItem, itemsOnscreen);
				
				cnv->foreColor = 12;
				for (i = 0; i < itemsOnscreen; i++, cur = cur->next)
					uiPrvDrawTruncText(cnv, listTop + i * itemHeight, itemLeft, cnv->w - scrollWidth - itemLeft, cur->name);
				
				prevSelOnscreenItem = selectedOnscreenItem + 1;		//force redraw of arrow
			}
			prevTopItem = topItem;
			if (prevSelOnscreenItem != selectedOnscreenItem) {
				
				cnv->foreColor = 0;
				uiPrvDrawOneChar(cnv, listTop + itemHeight * prevSelOnscreenItem, 1, MENU_SELECTION_CHAR);
				cnv->foreColor = 15;
				uiPrvDrawOneChar(cnv, listTop + itemHeight * selectedOnscreenItem, 1, MENU_SELECTION_CHAR);
			}
			prevSelOnscreenItem = selectedOnscreenItem;
			switch (uiPrvRecvKeypress()) {
				case KEY_BIT_A:
					for (cur = head, i = 0; i < curGlobalIdx; i++, cur = cur->next);
					if (uiPrvConfirmRomSelection(cnv, vol, cur->name)) {
						ret = true;
						goto out;
					}
					prevTopItem = topItem + 1;	//force a redraw
					break;
					
				case KEY_BIT_B:		//no rom selected
					goto out;
				
				case KEY_BIT_DOWN:
					if (curGlobalIdx < numRoms - 1) {
						curGlobalIdx++;
						selectedOnscreenItem++;
						if (selectedOnscreenItem == itemsOnscreen) {
							
							topItem += itemsOnscreen;
							if (topItem > numRoms - itemsOnscreen)	
								topItem = numRoms - itemsOnscreen;
							selectedOnscreenItem = curGlobalIdx - topItem;
						}
					}
					break;
				
				case KEY_BIT_UP:
					if (curGlobalIdx > 0) {
						curGlobalIdx--;
						if (!selectedOnscreenItem--) {
							
							if (topItem < itemsOnscreen)
								topItem = 0;
							else
								topItem -= itemsOnscreen;
							selectedOnscreenItem = curGlobalIdx - topItem;
						}
					}
					break;
			}
		}
	
	out:
		(void)uiPrvCardPreUnmount();
		fatfsUnmount(vol);
		return ret;
	}

	static bool uiPrvHaveGamePath(void)
	{
		const uint8_t *ptr = (const uint8_t*)QSPI_FILENAME_START;
		uint32_t len = 0;
		
		if (*ptr == 0x00 || *ptr == 0xff)
			return false;
		
		while (*ptr++) {
			if (++len == QSPI_FILENAME_MAXLEN)
				return false;
		}
		
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

	static bool uiPrvReadLine(struct FatfsFil *fil, char *buf, uint32_t bufSz, bool *truncatedP)
	{
		uint32_t pos = 0;

		*truncatedP = false;
		if (!bufSz)
			return false;

		while (1) {
			char ch;
			uint32_t numRead;

			if (!fatfsFileRead(fil, &ch, 1, &numRead))
				return false;

			if (!numRead) {
				buf[pos] = 0;
				return pos != 0 || *truncatedP;
			}

			if (ch == '\n') {
				buf[pos] = 0;
				return true;
			}

			if (ch == '\r')
				continue;

			if (pos + 1 < bufSz)
				buf[pos++] = ch;
			else
				*truncatedP = true;
		}
	}

	static void uiPrvIrDrawProgress(struct Canvas *cnv, const char *title, const char *name, uint32_t codeIdx, uint32_t repeatIdx, uint32_t numRepeats)
	{
		uint32_t row;

		uiPrvReset(cnv, false);
		cnv->font = FontLarge;
		row = uiPrvGlyphHeight(cnv) + 1;
		uiPuts(cnv, row, 10, title, -1);
		row += uiPrvGlyphHeight(cnv) + 2;

		cnv->font = FontMedium;
		cnv->foreColor = 11;
		uiPrintf(cnv, row, 10, "Code %u", codeIdx);
		row += uiPrvGlyphHeight(cnv) + 1;
		cnv->foreColor = 15;
		uiPrvDrawWrappedString(cnv, name, row, 10);
		uiPrintf(cnv, cnv->h - 2 * (uiPrvGlyphHeight(cnv) + 1), 10, "Repeat %u/%u", repeatIdx, numRepeats);
		uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "Hold B to cancel", -1);
	}

	static bool uiPrvIrCancelRequested(void)
	{
		return !!(uiGetKeysRaw() & KEY_BIT_B);
	}

	static bool uiPrvIrSpaceCancellable(uint32_t usec, bool *cancelledP)
	{
		while (usec) {
			uint32_t now = usec > 5000 ? 5000 : usec;

			if (uiPrvIrCancelRequested()) {
				*cancelledP = true;
				return false;
			}
			irRemoteSpaceUsec(now);
			usec -= now;
		}
		return true;
	}

	static bool uiPrvIrMarkCancellable(uint32_t carrier, uint32_t usec, bool *cancelledP)
	{
		if (uiPrvIrCancelRequested()) {
			*cancelledP = true;
			return false;
		}
		irRemoteMarkUsec(carrier, usec);
		return true;
	}

	static bool uiPrvIrSendBitPulseDistance(uint32_t carrier, bool bit, uint32_t mark, uint32_t zeroSpace, uint32_t oneSpace, bool *cancelledP)
	{
		return uiPrvIrMarkCancellable(carrier, mark, cancelledP) && uiPrvIrSpaceCancellable(bit ? oneSpace : zeroSpace, cancelledP);
	}

	static bool uiPrvIrSendBitsPulseDistance(uint32_t carrier, uint32_t data, uint_fast8_t nBits, bool msbFirst, uint32_t mark, uint32_t zeroSpace, uint32_t oneSpace, bool *cancelledP)
	{
		uint_fast8_t i;

		for (i = 0; i < nBits; i++) {
			bool bit;

			if (msbFirst)
				bit = !!(data & (1ul << (nBits - 1 - i)));
			else
				bit = !!(data & (1ul << i));

			if (!uiPrvIrSendBitPulseDistance(carrier, bit, mark, zeroSpace, oneSpace, cancelledP))
				return false;
		}
		return true;
	}

	static bool uiPrvIrSendBitMarkEncoded(uint32_t carrier, bool bit, uint32_t bitMark, uint32_t zeroMark, uint32_t oneMark, uint32_t space, bool *cancelledP)
	{
		(void)bitMark;
		return uiPrvIrMarkCancellable(carrier, bit ? oneMark : zeroMark, cancelledP) && uiPrvIrSpaceCancellable(space, cancelledP);
	}

	static bool uiPrvIrSendBitsMarkEncoded(uint32_t carrier, uint32_t data, uint_fast8_t nBits, uint32_t bitMark, uint32_t zeroMark, uint32_t oneMark, uint32_t space, bool *cancelledP)
	{
		uint_fast8_t i;

		for (i = 0; i < nBits; i++)
			if (!uiPrvIrSendBitMarkEncoded(carrier, !!(data & (1ul << i)), bitMark, zeroMark, oneMark, space, cancelledP))
				return false;
		return true;
	}

	static bool uiPrvIrManchesterHalf(uint32_t carrier, bool mark, uint32_t usec, bool *cancelledP)
	{
		if (mark)
			return uiPrvIrMarkCancellable(carrier, usec, cancelledP);
		return uiPrvIrSpaceCancellable(usec, cancelledP);
	}

	static bool uiPrvIrSendManchesterBit(uint32_t carrier, bool bit, uint32_t halfUsec, bool *cancelledP)
	{
		return uiPrvIrManchesterHalf(carrier, !bit, halfUsec, cancelledP) && uiPrvIrManchesterHalf(carrier, bit, halfUsec, cancelledP);
	}

	static bool uiPrvIrSendNecLike(uint32_t carrier, uint32_t address, uint32_t command, uint_fast8_t addrBits, uint_fast8_t cmdBits, bool useComplements, bool *cancelledP)
	{
		if (!uiPrvIrMarkCancellable(carrier, 9000, cancelledP) || !uiPrvIrSpaceCancellable(4500, cancelledP))
			return false;
		if (!uiPrvIrSendBitsPulseDistance(carrier, address, addrBits, false, 560, 560, 1690, cancelledP))
			return false;
		if (useComplements)
			if (!uiPrvIrSendBitsPulseDistance(carrier, ~address, addrBits, false, 560, 560, 1690, cancelledP))
				return false;
		if (!uiPrvIrSendBitsPulseDistance(carrier, command, cmdBits, false, 560, 560, 1690, cancelledP))
			return false;
		if (useComplements)
			if (!uiPrvIrSendBitsPulseDistance(carrier, ~command, cmdBits, false, 560, 560, 1690, cancelledP))
				return false;
		return uiPrvIrMarkCancellable(carrier, 560, cancelledP);
	}

	static bool uiPrvIrSendParsed(const char *protocol, uint32_t address, uint32_t command, bool *cancelledP)
	{
		uint32_t carrier = IR_DEFAULT_CARRIER;

		if (!strcmp(protocol, "NEC")) {
			if (!uiPrvIrSendNecLike(carrier, address, command, 8, 8, true, cancelledP))
				return false;
		}
		else if (!strcmp(protocol, "NECext")) {
			if (!uiPrvIrSendNecLike(carrier, address, command, 16, 16, false, cancelledP))
				return false;
		}
		else if (!strcmp(protocol, "NEC42")) {
			if (!uiPrvIrSendNecLike(carrier, address, command, 13, 8, true, cancelledP))
				return false;
		}
		else if (!strcmp(protocol, "NEC42ext")) {
			if (!uiPrvIrSendNecLike(carrier, address, command, 26, 16, false, cancelledP))
				return false;
		}
		else if (!strcmp(protocol, "Samsung32")) {
			if (!uiPrvIrMarkCancellable(carrier, 4500, cancelledP) || !uiPrvIrSpaceCancellable(4500, cancelledP) ||
				!uiPrvIrSendBitsPulseDistance(carrier, address, 16, false, 560, 560, 1690, cancelledP) ||
				!uiPrvIrSendBitsPulseDistance(carrier, command, 8, false, 560, 560, 1690, cancelledP) ||
				!uiPrvIrSendBitsPulseDistance(carrier, ~command, 8, false, 560, 560, 1690, cancelledP) ||
				!uiPrvIrMarkCancellable(carrier, 560, cancelledP))
				return false;
		}
		else if (!strcmp(protocol, "SIRC") || !strcmp(protocol, "SIRC15") || !strcmp(protocol, "SIRC20")) {
			uint_fast8_t addrBits = !strcmp(protocol, "SIRC") ? 5 : (!strcmp(protocol, "SIRC15") ? 8 : 13), rep;

			carrier = 40000;
			for (rep = 0; rep < 3; rep++) {
				if (!uiPrvIrMarkCancellable(carrier, 2400, cancelledP) || !uiPrvIrSpaceCancellable(600, cancelledP) ||
					!uiPrvIrSendBitsMarkEncoded(carrier, command, 7, 600, 600, 1200, 600, cancelledP) ||
					!uiPrvIrSendBitsMarkEncoded(carrier, address, addrBits, 600, 600, 1200, 600, cancelledP) ||
					!uiPrvIrSpaceCancellable(25000, cancelledP))
					return false;
			}
		}
		else if (!strcmp(protocol, "RCA")) {
			if (!uiPrvIrMarkCancellable(carrier, 4000, cancelledP) || !uiPrvIrSpaceCancellable(4000, cancelledP) ||
				!uiPrvIrSendBitsPulseDistance(carrier, address, 4, false, 500, 1000, 2000, cancelledP) ||
				!uiPrvIrSendBitsPulseDistance(carrier, command, 8, false, 500, 1000, 2000, cancelledP) ||
				!uiPrvIrSendBitsPulseDistance(carrier, ~address, 4, false, 500, 1000, 2000, cancelledP) ||
				!uiPrvIrSendBitsPulseDistance(carrier, ~command, 8, false, 500, 1000, 2000, cancelledP) ||
				!uiPrvIrMarkCancellable(carrier, 500, cancelledP))
				return false;
		}
		else if (!strcmp(protocol, "RC5") || !strcmp(protocol, "RC5X")) {
			uint32_t cmd = command & (!strcmp(protocol, "RC5X") ? 0x7f : 0x3f);
			uint_fast8_t i;

			carrier = 36000;
			if (!uiPrvIrSendManchesterBit(carrier, true, 889, cancelledP) ||
				!uiPrvIrSendManchesterBit(carrier, !(cmd & 0x40), 889, cancelledP) ||
				!uiPrvIrSendManchesterBit(carrier, false, 889, cancelledP))
				return false;
			for (i = 0; i < 5; i++)
				if (!uiPrvIrSendManchesterBit(carrier, !!(address & (1 << (4 - i))), 889, cancelledP))
					return false;
			for (i = 0; i < 6; i++)
				if (!uiPrvIrSendManchesterBit(carrier, !!(cmd & (1 << (5 - i))), 889, cancelledP))
					return false;
		}
		else if (!strcmp(protocol, "RC6")) {
			uint_fast8_t i;

			carrier = 36000;
			if (!uiPrvIrMarkCancellable(carrier, 2666, cancelledP) || !uiPrvIrSpaceCancellable(889, cancelledP) ||
				!uiPrvIrSendManchesterBit(carrier, true, 444, cancelledP) ||
				!uiPrvIrSendManchesterBit(carrier, false, 444, cancelledP) ||
				!uiPrvIrSendManchesterBit(carrier, false, 444, cancelledP) ||
				!uiPrvIrSendManchesterBit(carrier, false, 444, cancelledP) ||
				!uiPrvIrSendManchesterBit(carrier, false, 889, cancelledP))
				return false;
			for (i = 0; i < 8; i++)
				if (!uiPrvIrSendManchesterBit(carrier, !!(address & (1 << (7 - i))), 444, cancelledP))
					return false;
			for (i = 0; i < 8; i++)
				if (!uiPrvIrSendManchesterBit(carrier, !!(command & (1 << (7 - i))), 444, cancelledP))
					return false;
		}
		else if (!strcmp(protocol, "Kaseikyo")) {
			uint32_t vendor = address & 0xffff;
			uint32_t payload = ((address >> 16) & 0x03ff) | ((command & 0x03ff) << 10);
			uint8_t parity = (vendor ^ (vendor >> 8)) & 0x0f;

			if (!uiPrvIrMarkCancellable(carrier, 3360, cancelledP) || !uiPrvIrSpaceCancellable(1650, cancelledP) ||
				!uiPrvIrSendBitsPulseDistance(carrier, vendor, 16, false, 432, 432, 1296, cancelledP) ||
				!uiPrvIrSendBitsPulseDistance(carrier, parity, 4, false, 432, 432, 1296, cancelledP) ||
				!uiPrvIrSendBitsPulseDistance(carrier, payload, 20, false, 432, 432, 1296, cancelledP) ||
				!uiPrvIrSendBitsPulseDistance(carrier, (payload ^ (payload >> 8) ^ (payload >> 16)) & 0xff, 8, false, 432, 432, 1296, cancelledP) ||
				!uiPrvIrMarkCancellable(carrier, 432, cancelledP))
				return false;
		}
		else {
			return false;
		}

		return uiPrvIrSpaceCancellable(45000, cancelledP);
	}

	static bool uiPrvIrSendCodeLine(struct Canvas *cnv, const char *title, const char *codeStr, const char *name, uint32_t carrier, uint32_t repeat, uint32_t codeIdx, bool *malformedP, bool *cancelledP)
	{
		uint32_t rep;

		*malformedP = false;
		*cancelledP = false;

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
			const char *str = codeStr;
			bool mark = true;
			uint32_t numDurations = 0;

			uiPrvIrDrawProgress(cnv, title, name, codeIdx, rep + 1, repeat);

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

				if (mark) {
					if (!uiPrvIrMarkCancellable(carrier, duration, cancelledP))
						return false;
				}
				else if (!uiPrvIrSpaceCancellable(duration, cancelledP))
					return false;

				numDurations++;
				mark = !mark;

				while (*str == ',' || *str == ' ' || *str == '\t')
					str++;

				if (!*str)
					break;

				*malformedP = true;
				return false;
			}

			if (!numDurations) {
				*malformedP = true;
				return false;
			}

			if (!uiPrvIrSpaceCancellable(45000, cancelledP))
				return false;
		}

		return true;
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
		if (stats->cancelled)
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

		if (rec->type != FlipperIrTypeParsed || !rec->hasAddress || !rec->hasCommand) {
			stats->malformed++;
			return;
		}

		uiPrvIrDrawProgress(cnv, title, rec->name, stats->sent + 1, 1, 1);
		if (uiPrvIrSendParsed(rec->protocol, rec->address, rec->command, &stats->cancelled))
			stats->sent++;
		else
			stats->skipped++;
	}

	static bool uiPrvIrReadLineStat(struct FatfsFil *fil, char *line, struct IrBlastStats *stats)
	{
		bool truncated;

		if (!uiPrvReadLine(fil, line, IR_LINE_BUF_SZ, &truncated)) {
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

	static bool uiPrvIrFileIsFlipper(const char *trimmed)
	{
		return !strcmp(trimmed, "Filetype: IR signals file") || !strcmp(trimmed, "Filetype: IR library file");
	}

	static bool uiPrvIrDetectFormat(struct FatfsFil *fil, char *line, struct IrBlastStats *stats, bool *isFlipperP)
	{
		while (uiPrvIrReadLineStat(fil, line, stats)) {
			char *trimmed = uiPrvTrim(line);

			if (!*trimmed || *trimmed == '#')
				continue;

			if (!strcmp(trimmed, IR_FILE_MAGIC)) {
				*isFlipperP = false;
				return fatfsFileSeek(fil, 0);
			}

			if (uiPrvIrFileIsFlipper(trimmed)) {
				*isFlipperP = true;
				return fatfsFileSeek(fil, 0);
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
				bool malformed = false, cancelled = false;

				codeIdx++;
				if (uiPrvIrSendCodeLine(cnv, "IR Power Spam", uiPrvTrim(trimmed + 5), name, carrier, repeat, codeIdx, &malformed, &cancelled))
					stats->sent++;
				if (malformed)
					stats->malformed++;
				if (cancelled)
					stats->cancelled = true;
				if (malformed || cancelled)
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
		bool haveHeader = false, haveVersion = false;

		uiPrvFlipperRecordInit(&rec);

		while (uiPrvIrReadLineStat(fil, line, stats) && !stats->cancelled && (!maxSent || stats->sent < maxSent)) {
			char *trimmed = uiPrvTrim(line);

			if (!*trimmed)
				continue;

			if (*trimmed == '#') {
				uiPrvFlipperRecordFinish(cnv, &rec, stats, wantedName, title);
				uiPrvFlipperRecordInit(&rec);
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
				uiPrvCopyStr(rec.name, sizeof(rec.name), uiPrvTrim(trimmed + 5));
				rec.hasName = true;
			}
			else if (uiPrvStartsWith(trimmed, "type:")) {
				char *type = uiPrvTrim(trimmed + 5);

				if (!strcmp(type, "raw"))
					rec.type = FlipperIrTypeRaw;
				else if (!strcmp(type, "parsed"))
					rec.type = FlipperIrTypeParsed;
				else
					stats->malformed++;
			}
			else if (uiPrvStartsWith(trimmed, "protocol:")) {
				uiPrvCopyStr(rec.protocol, sizeof(rec.protocol), uiPrvTrim(trimmed + 9));
			}
			else if (uiPrvStartsWith(trimmed, "address:")) {
				rec.hasAddress = uiPrvParseFlipperU32Bytes(uiPrvTrim(trimmed + 8), &rec.address);
				if (!rec.hasAddress)
					stats->malformed++;
			}
			else if (uiPrvStartsWith(trimmed, "command:")) {
				rec.hasCommand = uiPrvParseFlipperU32Bytes(uiPrvTrim(trimmed + 8), &rec.command);
				if (!rec.hasCommand)
					stats->malformed++;
			}
			else if (uiPrvStartsWith(trimmed, "frequency:")) {
				const char *str = uiPrvTrim(trimmed + 10);

				rec.hasFrequency = uiPrvParseU32(&str, &rec.frequency) && !*str;
				if (!rec.hasFrequency)
					stats->malformed++;
			}
			else if (uiPrvStartsWith(trimmed, "duty_cycle:")) {
				//ignored for now
			}
			else if (uiPrvStartsWith(trimmed, "data:")) {
				if (rec.hasName && uiPrvIrNameMatches(rec.name, wantedName) && rec.type == FlipperIrTypeRaw) {
					bool malformed = false, cancelled = false;

					if (uiPrvIrSendCodeLine(cnv, title, uiPrvTrim(trimmed + 5), rec.name, rec.frequency, 1, stats->sent + 1, &malformed, &cancelled)) {
						rec.sentRaw = true;
						stats->sent++;
					}
					if (malformed)
						stats->malformed++;
					if (cancelled)
						stats->cancelled = true;
				}
			}
			else {
				stats->malformed++;
			}
		}

		if (!maxSent || stats->sent < maxSent)
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
		struct ToolWorkspaceSpan lineMem = toolWorkspaceGet(ToolWorkspaceCartRamUpper);
		char *line = (char*)lineMem.ptr;
		struct IrBlastStats stats;
		bool ret = false, isFlipper = false, irStarted = false;

		memset(&stats, 0, sizeof(stats));
		if (!line || lineMem.size < IR_LINE_BUF_SZ) {
			uiAlert(cnv, "Tool workspace is too small for IR", DialogTypeOk);
			return false;
		}

		vol = uiPrvMountCard(cnv, false);
		if (!vol)
			return false;

		if (!uiPrvIrOpenPowerFile(vol, &path, &fil)) {
			uiAlert(cnv, "Cannot find /IR/tv.ir or /IR/POWER.IR on the SD card", DialogTypeOk);
			goto out;
		}

		if (!uiPrvIrDetectFormat(fil, line, &stats, &isFlipper))
			goto out;

		memset(&stats, 0, sizeof(stats));
		irRemoteBegin();
		irStarted = true;
		ret = isFlipper ? uiPrvIrBlastFlipper(cnv, fil, line, &stats, NULL, "IR Power Spam", 0) : uiPrvIrPowerBlastDc32(cnv, fil, line, &stats);

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
		else if (stats.malformed && !stats.sent) {
			char msg[80];

			(void)sprintf(msg, "Malformed IR file near line %u", (unsigned)stats.lineNo);
			uiAlert(cnv, msg, DialogTypeOk);
		}
		else if (stats.cancelled) {
			uiAlert(cnv, "IR spam cancelled", DialogTypeOk);
		}
		else if (ret) {
			char msg[96];

			(void)sprintf(msg, "IR spam complete\nSent: %u\nSkipped: %u\nMalformed: %u", (unsigned)stats.sent, (unsigned)stats.skipped, (unsigned)stats.malformed);
			uiAlert(cnv, msg, DialogTypeOk);
		}
		else if (path) {
			uiAlert(cnv, "No power codes found in the IR file", DialogTypeOk);
		}

		return ret;
	}

	static bool uiPrvIrMuteBlast(struct Canvas *cnv)
	{
		struct FatfsVol *vol = NULL;
		struct FatfsFil *fil = NULL;
		struct ToolWorkspaceSpan lineMem = toolWorkspaceGet(ToolWorkspaceCartRamUpper);
		char *line = (char*)lineMem.ptr;
		struct IrBlastStats stats;
		bool ret = false, isFlipper = false, irStarted = false;

		memset(&stats, 0, sizeof(stats));
		if (!line || lineMem.size < IR_LINE_BUF_SZ) {
			uiAlert(cnv, "Tool workspace is too small for IR", DialogTypeOk);
			return false;
		}

		vol = uiPrvMountCard(cnv, false);
		if (!vol)
			return false;

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
		ret = uiPrvIrBlastFlipper(cnv, fil, line, &stats, "Mute", "IR Mute Spam", 0);

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
		else if (stats.malformed && !stats.sent) {
			char msg[80];

			(void)sprintf(msg, "Malformed /IR/tv.ir near line %u", (unsigned)stats.lineNo);
			uiAlert(cnv, msg, DialogTypeOk);
		}
		else if (stats.cancelled) {
			uiAlert(cnv, "Mute Spam cancelled", DialogTypeOk);
		}
		else if (ret) {
			char msg[96];

			(void)sprintf(msg, "Mute Spam complete\nSent: %u\nSkipped: %u\nMalformed: %u", (unsigned)stats.sent, (unsigned)stats.skipped, (unsigned)stats.malformed);
			uiAlert(cnv, msg, DialogTypeOk);
		}
		else if (fil) {
			uiAlert(cnv, "No mute codes found in /IR/tv.ir", DialogTypeOk);
		}

		return ret;
	}

	static bool uiPrvIrRemoteFileName(const char *fname)
	{
		return uiPrvStrEndsWithNoCase(fname, ".ir");
	}

	static bool uiPrvIrButtonListAppend(struct UiFileListCtx *ctx, const char *name)
	{
		struct FatFileLocator loc;

		memset(&loc, 0, sizeof(loc));
		return uiPrvFileListAppend(ctx, name, 0, false, &loc);
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
		itemsOnscreen = (cnv->h - listTop) / itemHeight;
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

			switch (uiPrvRecvKeypress()) {
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
					break;
			}
		}
	}

	static bool uiPrvIrRemoteLocator(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *locator, const char *fileName)
	{
		struct FatfsFil *fil = NULL;
		struct MusicOption *buttons = NULL, *button;
		char buttonName[IR_NAME_BUF_SZ];
		struct ToolWorkspaceSpan lineMem = toolWorkspaceGet(ToolWorkspaceCartRamUpper);
		char *line = (char*)lineMem.ptr;
		struct IrBlastStats stats;
		bool ret = false, overflow = false, lineTooLong = false, isFlipper = false, irStarted = false;
		uint32_t numButtons;

		memset(&stats, 0, sizeof(stats));
		if (!line || lineMem.size < IR_LINE_BUF_SZ) {
			uiAlert(cnv, "Tool workspace is too small for IR remote", DialogTypeOk);
			return false;
		}

		fil = fatfsFileOpenWithLocator(vol, locator, OPEN_MODE_READ);
		if (!fil) {
			uiAlert(cnv, "Cannot open IR remote file", DialogTypeOk);
			goto out_report;
		}

		numButtons = uiPrvIrListButtons(fil, line, &buttons, &overflow, &lineTooLong);
		if (lineTooLong) {
			uiAlert(cnv, "A line in the IR file is too long", DialogTypeOk);
			goto out_close;
		}
		if (overflow)
			uiAlert(cnv, "Remote has too many buttons; showing what fits", DialogTypeOk);
		if (!numButtons) {
			uiAlert(cnv, "No Flipper buttons found in that IR file", DialogTypeOk);
			goto out_close;
		}

		button = uiPrvChooseFlatOption(cnv, buttons, numButtons, fileName);
		if (!button)
			goto out_close;
		uiPrvCopyStr(buttonName, sizeof(buttonName), button->name);

		if (!fatfsFileSeek(fil, 0))
			goto out_close;
		memset(&stats, 0, sizeof(stats));
		if (!uiPrvIrDetectFormat(fil, line, &stats, &isFlipper) || !isFlipper)
			goto out_close;

		memset(&stats, 0, sizeof(stats));
		irRemoteBegin();
		irStarted = true;
		ret = uiPrvIrBlastFlipper(cnv, fil, line, &stats, buttonName, "IR Remote", 1);

	out_close:
		if (irStarted)
			irRemoteEnd();
		if (fil)
			fatfsFileClose(fil);
	out_report:
		if (stats.lineTooLong) {
			uiAlert(cnv, "A line in the IR file is too long", DialogTypeOk);
		}
		else if (stats.cancelled) {
			uiAlert(cnv, "IR remote cancelled", DialogTypeOk);
		}
		else if (ret) {
			uiAlert(cnv, "IR remote button sent", DialogTypeOk);
		}
		else if (irStarted) {
			uiAlert(cnv, "Selected IR button could not be sent", DialogTypeOk);
		}

		return ret;
	}

	static bool uiPrvIrRemote(struct Canvas *cnv)
	{
		struct FatfsVol *vol = NULL;
		struct FatFileLocator locator;
		char fileName[UI_PICK_FILE_NAME_BUF_SZ];
		bool ret = false;

		vol = uiPrvMountCard(cnv, false);
		if (!vol)
			return false;

		if (!uiPrvPickFile(cnv, vol, "/IR", uiPrvIrRemoteFileName, "No .ir files found in /IR", &locator, fileName, sizeof(fileName), NULL, 0))
			goto out_unmount;

		ret = uiPrvIrRemoteLocator(cnv, vol, &locator, fileName);

	out_unmount:
		(void)uiPrvCardPreUnmount();
		fatfsUnmount(vol);
		return ret;
	}

	static bool uiPrvIrTools(struct Canvas *cnv)
	{
		while (1) {
			uint_fast8_t itemHeight, selOption;
			uint_fast8_t remoteOption = 0, backOption = 1;
			uint8_t button = KEY_BIT_A | KEY_BIT_B;

			uiPrvReset(cnv, false);
			itemHeight = uiPrvGlyphHeight(cnv) + 1;

			uiPuts(cnv, cnv->h - 2 * itemHeight, 10, "Remote", -1);
			uiPuts(cnv, cnv->h - 1 * itemHeight, 10, "Back", -1);

			selOption = uiPrvMenu(cnv, 0, 2, &button);
			if (button == KEY_BIT_B || selOption == backOption)
				return false;
			if (selOption == remoteOption)
				(void)uiPrvIrRemote(cnv);
		}
	}

	static bool uiPrvMusicPlayableName(const char *fname)
	{
		return uiPrvStrEndsWithNoCase(fname, ".rtttl") || uiPrvStrEndsWithNoCase(fname, ".txt");
	}

	static uint_fast8_t uiPrvContentTop(struct Canvas *cnv)
	{
		return fontGetHeight(FontLarge) + fontGetHeight(FontMedium) / 4;
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

	static bool uiPrvMusicIsDotDir(const char *name)
	{
		return name[0] == '.' && (!name[1] || (name[1] == '.' && !name[2]));
	}

	static bool uiPrvMusicListAppend(struct MusicListCtx *ctx, const char *fname, uint32_t fileSz, bool isDir, const struct FatFileLocator *locator)
	{
		uint32_t nameLen = strlen(fname), spaceNeeded;

		spaceNeeded = (sizeof(struct MusicOption) + nameLen + 1 + 3) &~ 3;
		if (spaceNeeded > ctx->spaceAvail) {
			ctx->overflow = true;
			return false;
		}

		ctx->nextAvail->prev = ctx->tail;
		ctx->nextAvail->next = NULL;
		ctx->nextAvail->locator = *locator;
		ctx->nextAvail->size = fileSz;
		ctx->nextAvail->isDir = isDir;
		memcpy(ctx->nextAvail->name, fname, nameLen + 1);
		if (ctx->tail)
			ctx->tail->next = ctx->nextAvail;
		else
			ctx->head = ctx->nextAvail;
		ctx->tail = ctx->nextAvail;
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
			if (attrs & FATFS_ATTR_VOL_LBL)
				continue;

			if (attrs & FATFS_ATTR_DIR) {
				if (uiPrvMusicIsDotDir(ctx->fname))
					continue;
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
		uint8_t prevKeys;
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
		if (settings->musicVolume > AUDIO_PWM_VOLUME_MAX)
			settings->musicVolume = 11;
	}

	static void uiPrvMusicApplySettings(struct MusicUiData *data)
	{
		audioPwmSetVolume(data->settings->musicVolume);
		data->settingsDirty = true;
		data->forceDraw = true;
	}

	static void uiPrvMusicAdjustVolume(struct MusicUiData *data, int_fast8_t delta)
	{
		uint8_t volume = data->settings->musicVolume;

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

		data->settings->musicVolume = volume;
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
		static const char *labels[MusicPlaybackControlNum] = {"Prev", "Play", "Next", "Loop", "Vol"};
		uint32_t cellW = cnv->w / MusicPlaybackControlNum, row = cnv->h - uiPrvGlyphHeight(cnv) - 2;
		uint_fast8_t i;
		int8_t fore = cnv->foreColor;

		for (i = 0; i < MusicPlaybackControlNum; i++) {
			char buf[12];
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
				(void)sprintf(buf, "Vol%u", data->settings->musicVolume);
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
		uiPrintf(cnv, row, 10, "%s %u%% RTTTL %ubpm", status->paused ? "Paused" : "Playing", pct, status->sampleRate);
		cnv->foreColor = fore;
	}

	static enum MusicPlayerControl uiPrvMusicControl(void *userData, const struct MusicPlayerStatus *status)
	{
		struct MusicUiData *data = (struct MusicUiData*)userData;
		uint8_t keys = uiGetKeys(), pressed = keys &~ data->prevKeys;
		uint64_t now = getTime();

		data->prevKeys = keys;
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
		if (pressed & KEY_BIT_UP)
			uiPrvMusicAdjustVolume(data, 1);
		if (pressed & KEY_BIT_DOWN)
			uiPrvMusicAdjustVolume(data, -1);

		{
			struct Canvas *cnv = data->cnv;
			uint32_t row = uiPrvContentTop(cnv);
			uint32_t pct = status->fileSize ? status->bytesPlayed * 100 / status->fileSize : 0;
			uint32_t progressFillRight = uiPrvMusicProgressFillRight(cnv, status);
			bool fullDraw = data->forceDraw || data->lastPct > 100 || data->lastPaused != status->paused;
			bool progressDraw = pct != data->lastPct || progressFillRight != data->lastProgressFillRight;

			if (fullDraw || ((now - data->lastDraw > TICKS_PER_SECOND / 4) && progressDraw)) {
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

	static enum MusicPlayerResult uiPrvPlayMusicLocator(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *locator, const char *name, struct Settings *settings)
	{
		struct FatfsFil *fil;
		struct MusicUiData data = {.cnv = cnv, .settings = settings, .name = name, .focus = MusicPlaybackControlPlay, .lastPct = 0xff, .forceDraw = true};
		enum MusicPlayerResult ret;

		audioPwmSetVolume(settings->musicVolume);
		fil = fatfsFileOpenWithLocator(vol, locator, OPEN_MODE_READ);
		if (!fil) {
			uiAlert(cnv, "Cannot open music file", DialogTypeOk);
			return MusicPlayerResultStopped;
		}

		data.prevKeys = uiGetKeys();
		data.lastDraw = getTime() - TICKS_PER_SECOND;
		ret = rtttlPlayerPlayFile(fil, uiPrvMusicControl, &data);
		fatfsFileClose(fil);
		if (data.settingsDirty)
			settingsSet(settings);
		if (ret == MusicPlayerResultStopped)
			uiPrvWaitKeysReleased();
		if (ret == MusicPlayerResultFileError)
			uiAlert(cnv, "Music read failed", DialogTypeOk);
		else if (ret == MusicPlayerResultDecodeError)
			uiAlert(cnv, "Bad RTTTL file", DialogTypeOk);

		return ret;
	}

	static enum MusicPlayerResult uiPrvPlayMusic(struct Canvas *cnv, struct FatfsVol *vol, struct MusicOption *song, struct Settings *settings)
	{
		return uiPrvPlayMusicLocator(cnv, vol, &song->locator, song->name, settings);
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

		settingsGet(&settings);
		uiPrvMusicSanitizeSettings(&settings);
		audioPwmSetVolume(settings.musicVolume);

		vol = uiPrvMountCard(cnv, false);
		if (!vol)
			return;

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
		itemsOnscreen = (cnv->h - listTop) / itemHeight;
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

				uiPrvReset(cnv, false);
				uiPrvDrawTruncText(cnv, pathTop, 10, cnv->w - 10, path);
				scrollWidth = totalItems > itemsOnscreen ? uiPrvDrawScrollbar(cnv, listTop, totalItems, topItem, itemsOnscreen) : 0;

				if (depth && !topItem) {
					cnv->foreColor = 12;
					uiPrvDrawTruncText(cnv, listTop, itemLeft, cnv->w - scrollWidth - itemLeft, "[..]");
					firstRow = 1;
					skipItems = 0;
				}
				else
					skipItems = topItem - (depth ? 1 : 0);

				for (i = 0; i < skipItems && draw; i++)
					draw = draw->next;

				cnv->foreColor = 12;
				for (i = firstRow; i < itemsOnscreen && draw; i++, draw = draw->next) {
					char label[FATFS_NAME_BUF_LEN + 3];

					if (draw->isDir) {
						label[0] = '[';
						strcpy(label + 1, draw->name);
						strcat(label, "]");
						uiPrvDrawTruncText(cnv, listTop + i * itemHeight, itemLeft, cnv->w - scrollWidth - itemLeft, label);
					}
					else
						uiPrvDrawTruncText(cnv, listTop + i * itemHeight, itemLeft, cnv->w - scrollWidth - itemLeft, draw->name);
				}

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

			switch (uiPrvRecvKeypress()) {
				case KEY_BIT_A:
					if (depth && selectedItem == 0) {
						depth--;
						haveDirLoc = depth != 0;
						path[pathLenStack[depth]] = 0;
						goto reload_dir;
					}
					if (cur && cur->isDir) {
						uint32_t pathLen = strlen(path), nameLen = strlen(cur->name);

						if (depth < UI_BROWSER_MAX_DEPTH) {
							pathLenStack[depth] = pathLen;
							dirStack[depth++] = cur->locator;
							haveDirLoc = true;
							if (pathLen + 1 < sizeof(path)) {
								path[pathLen++] = '/';
								if (pathLen + nameLen < sizeof(path))
									memcpy(path + pathLen, cur->name, nameLen + 1);
								else if (pathLen + 4 <= sizeof(path))
									strcpy(path + pathLen, "...");
								else
									path[pathLen - 1] = 0;
							}
							goto reload_dir;
						}
						uiAlert(cnv, "Folder nesting too deep", DialogTypeOk);
						prevTopItem = topItem + 1;
						break;
					}
					while (cur && !cur->isDir) {
						enum MusicPlayerResult playRet = uiPrvPlayMusic(cnv, vol, cur, &settings);

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
					prevTopItem = topItem + 1;
					break;

				case KEY_BIT_B:
					if (depth) {
						depth--;
						haveDirLoc = depth != 0;
						path[pathLenStack[depth]] = 0;
						goto reload_dir;
					}
					else
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
					break;
			}
		}

	out_unmount:
		(void)uiPrvCardPreUnmount();
		fatfsUnmount(vol);
	}

	struct BadUsbUiData {
		struct Canvas *cnv;
		const char *name;
		uint64_t lastDraw;
		uint32_t lastLine;
		bool forceDraw;
	};

	static bool uiPrvBadUsbStatus(void *userData, const struct BadUsbStatus *status)
	{
		struct BadUsbUiData *data = (struct BadUsbUiData*)userData;
		struct Canvas *cnv = data->cnv;
		uint64_t now = getTime();

		if (uiGetKeysRaw() & KEY_BIT_B)
			return false;

		if (data->forceDraw || status->lineNo != data->lastLine || now - data->lastDraw > TICKS_PER_SECOND / 4) {
			uint32_t row, pct = status->fileSize ? status->bytesRead * 100 / status->fileSize : 0;

			uiPrvReset(cnv, false);
			cnv->font = FontLarge;
			row = uiPrvGlyphHeight(cnv) + 1;
			uiPuts(cnv, row, 10, "BadUSB", -1);
			row += uiPrvGlyphHeight(cnv) + 2;
			cnv->font = FontMedium;
			uiPrvDrawTruncText(cnv, row, 10, cnv->w - 20, data->name);
			row += uiPrvGlyphHeight(cnv) + 2;
			uiPrintf(cnv, row, 10, "Line %u  %u%%", (unsigned)status->lineNo, (unsigned)pct);
			row += uiPrvGlyphHeight(cnv) + 2;
			if (status->message)
				uiPrvDrawTruncText(cnv, row, 10, cnv->w - 20, status->message);
			uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "Hold B to cancel", -1);
			data->lastDraw = now;
			data->lastLine = status->lineNo;
			data->forceDraw = false;
		}
		return true;
	}

	static bool uiPrvBadUsbWaitButton(void *userData, const struct BadUsbStatus *status)
	{
		struct BadUsbUiData *data = (struct BadUsbUiData*)userData;
		struct Canvas *cnv = data->cnv;

		uiPrvReset(cnv, false);
		cnv->font = FontLarge;
		uiPuts(cnv, uiPrvGlyphHeight(cnv) + 1, 10, "BadUSB Paused", -1);
		cnv->font = FontMedium;
		uiPuts(cnv, cnv->h - 2 * (uiPrvGlyphHeight(cnv) + 1), 10, "A = Continue", -1);
		uiPuts(cnv, cnv->h - uiPrvGlyphHeight(cnv) - 1, 10, "B = Cancel", -1);
		while (1) {
			uint8_t key = uiPrvRecvKeypress();

			if (key & KEY_BIT_A) {
				data->forceDraw = true;
				return true;
			}
			if (key & KEY_BIT_B)
				return false;
		}
	}

	static bool uiPrvBadUsbFileName(const char *fname)
	{
		return uiPrvStrEndsWithNoCase(fname, ".txt") || uiPrvStrEndsWithNoCase(fname, ".badusb");
	}

	static bool uiPrvRunBadUsbLocator(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *locator, const char *name)
	{
		struct FatfsFil *fil = NULL;
		struct BadUsbUiData data;
		enum BadUsbResult ret;
		char msg[96];
		bool ok = false;

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
		ret = badUsbRunFile(fil, uiPrvBadUsbStatus, uiPrvBadUsbWaitButton, &data);
		if (ret == BadUsbResultDone) {
			uiAlert(cnv, "BadUSB script complete", DialogTypeOk);
			ok = true;
		}
		else if (ret == BadUsbResultCancelled)
			uiAlert(cnv, "BadUSB script cancelled", DialogTypeOk);
		else if (ret == BadUsbResultUsbError)
			uiAlert(cnv, "BadUSB USB device failed to enumerate", DialogTypeOk);
		else if (ret == BadUsbResultFileError)
			uiAlert(cnv, "BadUSB script read failed", DialogTypeOk);
		else
			uiAlert(cnv, "BadUSB script has an unsupported or malformed command", DialogTypeOk);

	out_close:
		if (fil)
			fatfsFileClose(fil);
		return ok;
	}

	static bool uiPrvBadUsbTool(struct Canvas *cnv)
	{
		struct FatfsVol *vol;
		struct FatFileLocator locator;
		char name[UI_PICK_FILE_NAME_BUF_SZ];
		bool ok = false;

		vol = uiPrvMountCard(cnv, false);
		if (!vol)
			return false;
		if (!uiPrvPickFile(cnv, vol, "/BADUSB", uiPrvBadUsbFileName, "No BadUSB scripts found in /BADUSB", &locator, name, sizeof(name), NULL, 0))
			goto out_unmount;

		ok = uiPrvRunBadUsbLocator(cnv, vol, &locator, name);

	out_unmount:
		(void)uiPrvCardPreUnmount();
		fatfsUnmount(vol);
		return ok;
	}

	static void uiPrvFwUpdate(struct Canvas *cnv, bool tryZDU)
	{
		int_fast8_t updateOption, cancelOption = -1, numOptions = 0, selOption, itemHeight;
		struct FatfsVol *vol;

		vol = uiPrvMountCard(cnv, tryZDU);
		if (!vol) {

			//uiPrvMountCard shows its own error message
			pr("%s: no card init\n", __func__);
		}
		else {

			struct FatfsFil *fil = fatfsFileOpen(vol, tryZDU ? "/UPDATE_OR_WE_ARE.FUCKED":"/FIRMWARE.BIN", OPEN_MODE_READ);

			if (!fil) {
				
				pr("%s: file\n", __func__);

				if (!tryZDU)
					uiAlert(cnv, "Cannot find /FIRMWARE.BIN. You're out of luck in the firmware department. No firmware for you.", DialogTypeOk);
			}
			else {
				uint32_t fileSz = fatfsFileGetSize(fil);

				if (fileSz < 1024) {

					if (!tryZDU)
						uiAlert(cnv, "/FIRMWARE.BIN is too small to be believable. You're out of luck in the firmware department. No firmware for you.", DialogTypeOk);
				}
				else {

					bool tryToUpdate = false;

					if (tryZDU) {
						uint8_t curForcedVer = *(volatile uint8_t*)0x10000020, availForcedVer;
						uint32_t nBytesRead;

						if (fatfsFileSeek(fil, 0x20) && fatfsFileRead(fil, &availForcedVer, 1, &nBytesRead) && nBytesRead == 1 && fatfsFileSeek(fil, 0) && curForcedVer < availForcedVer) {
							
							uiAlert(cnv, "There is a mandatory update to apply. It will be applied now. You may opt out, but the update will proceed anyways. Proceed?", DialogTypeOk);

							tryToUpdate = true;
						}
						else {

							pr("%s: update not interesting\n", __func__);
						}
					}
					else {
						uint8_t button = KEY_BIT_A | KEY_BIT_B;

						itemHeight = uiPrvGlyphHeight(cnv) + 1;
						uiPrvDrawWrappedString(cnv, "This option will replace the current firmware with whatever is in /FIRMWARE.BIN on the card. No checks are made. GL HF", 32, 10);

						updateOption = numOptions++;
						uiPuts(cnv, cnv->h - 2 * itemHeight, 10, "Proceed", -1);
						cancelOption = numOptions++;
						uiPuts(cnv, cnv->h - 1 * itemHeight, 10, "Cancel", -1);
						
						selOption = uiPrvMenu(cnv, 0, numOptions, &button);

						tryToUpdate = button == KEY_BIT_A && selOption == updateOption;
					}

					if (tryToUpdate) {

						if (!uiPrvLoadFile(cnv, fil, 0x10000000, "FIRMWARE")) {

							uiAlert(cnv, "Firmware update failed to be installed. You're out of luck in the firmware department. No firmware for you.", DialogTypeOk);
						}
						else {

							 while(1)
							 	NVIC_SystemReset();
						}
					}
				}
				(void)fatfsFileClose(fil);
			}
			(void)uiPrvCardPreUnmount();
			fatfsUnmount(vol);
		}
	}

#endif //NO_SD_CARD

static bool uiPrvHaveValidRom(char *romNameOutP, enum RomColorSupport *romColorSupportP, uint32_t *ramSzExpectedP)
{
	const struct CartHeader *hdr = (const struct CartHeader*)QSPI_ROM_START;
	uint32_t romSzExpected, ramSzExpected;
	enum RomColorSupport romColorSupport;

	#ifndef NO_SD_CARD
		//in systems with an SD card, the file path is mandatory
		if (!uiPrvHaveGamePath())
			return false;
	#endif

	if (romNameOutP)
		romNameOutP[ROM_NAME_LEN] = 0;
	
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

enum UiToolId {
	UiToolBrowser,
	UiToolIr,
	UiToolBadUsb,
	UiToolMusic,
	UiToolGame,
	UiToolSettings,
	UiToolFwUpdate,
	UiToolPowerOff,
	UiToolNum,
};

#ifndef NO_SD_CARD
struct UiFileRef {
	struct FatFileLocator locator;
	uint32_t size;
	bool isDir;
	const char *name;
	const char *parentPath;
};

static bool uiPrvPathIsFolderNoCase(const char *path, const char *folder)
{
	uint32_t pathLen = strlen(path), folderLen = strlen(folder);

	if (pathLen < folderLen)
		return false;
	if (strsCaselesslyCompareUtf(path, folder, folderLen))
		return false;
	return !path[folderLen] || path[folderLen] == '/';
}

static bool uiPrvRomFileName(const char *fname)
{
	return uiPrvStrEndsWithNoCase(fname, ".gb") || uiPrvStrEndsWithNoCase(fname, ".gbc");
}

static enum UiToolId uiPrvToolSwitcher(struct Canvas *cnv, enum UiToolId curTool)
{
	static const char *names[UiToolNum] = {
		[UiToolBrowser] = "Browser",
		[UiToolIr] = "IR",
		[UiToolBadUsb] = "BadUSB",
		[UiToolMusic] = "Music",
		[UiToolGame] = "Game",
		[UiToolSettings] = "Settings",
		[UiToolFwUpdate] = "Firmware Update",
		[UiToolPowerOff] = "Power Off",
	};
	uint_fast8_t itemHeight, i, selOption;
	uint8_t button = KEY_BIT_A | KEY_BIT_B;

	uiPrvReset(cnv, false);
	itemHeight = uiPrvGlyphHeight(cnv) + 1;
	for (i = 0; i < UiToolNum; i++)
		uiPuts(cnv, cnv->h - (UiToolNum - i) * itemHeight, 10, names[i], -1);

	selOption = uiPrvMenu(cnv, curTool, UiToolNum, &button);
	if (button == KEY_BIT_B)
		return curTool;
	return (enum UiToolId)selOption;
}

static enum UiToolId uiPrvLaunchBrowserFile(struct Canvas *cnv, struct FatfsVol *vol, const struct UiFileRef *ref, UiRunGameF runGameF, void *userData)
{
	(void)runGameF;
	(void)userData;

	if (uiPrvIrRemoteFileName(ref->name)) {
		(void)uiPrvIrRemoteLocator(cnv, vol, &ref->locator, ref->name);
		return UiToolBrowser;
	}
	if (uiPrvStrEndsWithNoCase(ref->name, ".badusb") ||
		(uiPrvStrEndsWithNoCase(ref->name, ".txt") && uiPrvPathIsFolderNoCase(ref->parentPath, "/BADUSB"))) {
		(void)uiPrvRunBadUsbLocator(cnv, vol, &ref->locator, ref->name);
		return UiToolBrowser;
	}
	if (uiPrvStrEndsWithNoCase(ref->name, ".rtttl")) {
		struct Settings settings;

		settingsGet(&settings);
		uiPrvMusicSanitizeSettings(&settings);
		(void)uiPrvPlayMusicLocator(cnv, vol, &ref->locator, ref->name, &settings);
		settingsSet(&settings);
		return UiToolBrowser;
	}
	if (uiPrvRomFileName(ref->name)) {
		if (!uiPrvPathIsFolderNoCase(ref->parentPath, "/ROM")) {
			uiAlert(cnv, "Game files must be launched from /ROM", DialogTypeOk);
			return UiToolBrowser;
		}
		if (uiPrvConfirmRomSelection(cnv, vol, ref->name))
			return UiToolGame;
		return UiToolBrowser;
	}

	uiAlert(cnv, "No tool is registered for that file type", DialogTypeOk);
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

	if (!browserPath || pathMem.size < UI_PICK_FILE_PATH_BUF_SZ) {
		uiAlert(cnv, "Tool workspace is too small for file browser", DialogTypeOk);
		return uiPrvToolSwitcher(cnv, UiToolBrowser);
	}
	browserPath[0] = '/';
	browserPath[1] = 0;

	vol = uiPrvMountCard(cnv, false);
	if (!vol)
		return uiPrvToolSwitcher(cnv, UiToolBrowser);

	while (1) {
		if (!uiPrvPickFile(cnv, vol, browserPath, NULL, "No files found on the SD card", &locator, name, sizeof(name), browserPath, UI_PICK_FILE_PATH_BUF_SZ)) {
			nextTool = uiPrvToolSwitcher(cnv, UiToolBrowser);
			if (nextTool != UiToolBrowser)
				break;
			continue;
		}
		memset(&ref, 0, sizeof(ref));
		ref.locator = locator;
		ref.name = name;
		ref.parentPath = browserPath;
		nextTool = uiPrvLaunchBrowserFile(cnv, vol, &ref, runGameF, userData);
		if (nextTool != UiToolBrowser)
			break;
		browserPath[0] = '/';
		browserPath[1] = 0;
	}

	(void)uiPrvCardPreUnmount();
	fatfsUnmount(vol);
	return nextTool;
}
#endif

static enum UiToolId uiPrvGameTool(struct Canvas *cnv, UiRunGameF runGameF, void *userData)
{
	while (1) {
		int_fast8_t runOption = -1, selectOption = -1, switchOption, numOptions = 0, selOption;
		uint_fast8_t itemHeight;
		enum RomColorSupport romColorSupport;
		char name[ROM_NAME_LEN + 1];
		uint32_t ramSz = 0;
		bool validRom = uiPrvHaveValidRom(name, &romColorSupport, &ramSz);
		uint8_t button = KEY_BIT_A | KEY_BIT_B;

		uiPrvReset(cnv, false);
		itemHeight = uiPrvGlyphHeight(cnv) + 1;

		if (validRom) {
			uiPrintf(cnv, cnv->h - 3 * itemHeight, 10, "Run game '%s'", name);
			runOption = numOptions++;
		}
	#ifndef NO_SD_CARD
		selectOption = numOptions++;
		uiPuts(cnv, cnv->h - (validRom ? 2 : 2) * itemHeight, 10, validRom ? "Select Game" : "Load Game", -1);
	#endif
		switchOption = numOptions++;
		uiPuts(cnv, cnv->h - 1 * itemHeight, 10, "Switch Tool", -1);

		selOption = uiPrvMenu(cnv, 0, numOptions, &button);
		if (button == KEY_BIT_B || selOption == switchOption)
			return uiPrvToolSwitcher(cnv, UiToolGame);
		if (selOption == runOption) {
			toolWorkspaceEnd();
			uiPrvLoadSavestate();
			runGameF(userData);
			if (!uiSaveSavestate())
				uiAlert(cnv, "Failed to save state to flash", DialogTypeOk);
			toolWorkspaceBegin();
		}
	#ifndef NO_SD_CARD
		else if (selOption == selectOption) {
			if (uiPrvSelectRom(cnv, validRom ? ramSz : 0)) {
				toolWorkspaceEnd();
				uiPrvLoadSavestate();
				runGameF(userData);
				if (!uiSaveSavestate())
					uiAlert(cnv, "Failed to save state to flash", DialogTypeOk);
				toolWorkspaceBegin();
			}
		}
	#endif
	}
}

enum UiGameAction uiGameMenu(void)
{
	struct Canvas canvas = CANVAS_INITIALIZER, *cnv = &canvas;
	uint_fast8_t itemHeight, selOption;
	uint_fast8_t resumeOption = 0, settingsOption = 1, switchOption = 2, powerOffOption = 3;
	uint8_t button = KEY_BIT_A | KEY_BIT_B;
	
	uiPrvReset(cnv, false);

	if (!uiSaveSavestate())
		uiAlert(cnv, "Failed to save state to flash", DialogTypeOk);

	itemHeight = uiPrvGlyphHeight(cnv) + 1;
	uiPuts(cnv, cnv->h - 4 * itemHeight, 10, "Resume Game", -1);
	uiPuts(cnv, cnv->h - 3 * itemHeight, 10, "Settings", -1);
	uiPuts(cnv, cnv->h - 2 * itemHeight, 10, "Switch Tool", -1);
	uiPuts(cnv, cnv->h - 1 * itemHeight, 10, "Power Off", -1);

	selOption = uiPrvMenu(cnv, 0, 4, &button);
	if (button == KEY_BIT_B || selOption == resumeOption)
		return UiGameActionResume;
	if (selOption == settingsOption)
		return uiPrvSettings(cnv) ? UiGameActionRestart : UiGameActionResume;
	if (selOption == switchOption)
		return UiGameActionSwitchTool;
	if (selOption == powerOffOption)
		doSleep();

	return UiGameActionResume;
}

void uiRunToolShell(UiRunGameF runGameF, void *userData)
{
	struct Canvas canvas = CANVAS_INITIALIZER, *cnv = &canvas;
	enum UiToolId activeTool = UiToolBrowser;

	uiPrvReset(cnv, false);
#ifndef NO_SD_CARD
	uiPrvFwUpdate(cnv, true);
#endif
	toolWorkspaceBegin();

	while (1) {
		switch (activeTool) {
			case UiToolBrowser:
			#ifndef NO_SD_CARD
				activeTool = uiPrvBrowserTool(cnv, runGameF, userData);
			#else
				activeTool = uiPrvToolSwitcher(cnv, UiToolBrowser);
			#endif
				continue;

			case UiToolIr:
			#ifndef NO_SD_CARD
				(void)uiPrvIrTools(cnv);
			#endif
				break;

			case UiToolBadUsb:
			#ifndef NO_SD_CARD
				(void)uiPrvBadUsbTool(cnv);
			#endif
				break;

			case UiToolMusic:
			#ifndef NO_SD_CARD
				uiPrvMusicPlayer(cnv);
			#endif
				break;

			case UiToolGame:
				activeTool = uiPrvGameTool(cnv, runGameF, userData);
				continue;

			case UiToolSettings:
				(void)uiPrvSettings(cnv);
				break;

			case UiToolFwUpdate:
			#ifndef NO_SD_CARD
				uiPrvFwUpdate(cnv, false);
			#endif
				break;

			case UiToolPowerOff:
				doSleep();
				break;

			default:
				activeTool = UiToolBrowser;
				continue;
		}
		activeTool = uiPrvToolSwitcher(cnv, activeTool);
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

