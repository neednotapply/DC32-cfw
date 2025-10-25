#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include "gbCartHeader.h"
#include "settings.h"
#include "memMap.h"
#include "printf.h"
#include "sleep.h"
#include "fonts.h"
#include "fatfs.h"
#include "qspi.h"
#include "cpu.h"
#include "mbc.h"
#include "utf.h"
#include "ui.h"
#include "sd.h"
#include "gb.h"

extern void defconApplyLedSettingsFromStruct(const struct Settings *settings);
extern void defconOrientationApplySettings(const struct Settings *settings);


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
	
	return prevVal;
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
		
		if (key == KEY_BIT_A)
			return true;
		
		if (key == KEY_BIT_B)
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
					uiAlert(cnv, "Cannot initialize SD card", DialogTypeOk);
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

static const char *uiPrvLedModeName(uint8_t mode)
{
        switch (mode) {
                case LedModeManual:
                        return "ON";
                case LedModeAllOff:
                        return "OFF";
                default:
                        return "ON";
        }
}

static void uiPrvLedColorAdjust(struct Canvas *cnv, struct Settings *settings, uint_fast8_t ledIdx)
{
	int_fast8_t selOption = 0;
	uint_fast8_t itemHeight = uiPrvGlyphHeight(cnv) + 1;

	uiPrvReset(cnv, false);

	while (1) {
		int_fast8_t numOptions = 0;
		int_fast8_t doneOption, redOption, greenOption, blueOption;
		uint8_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;

		cnv->foreColor = 11;
		uiPrintf(cnv, itemHeight, 10, "LED %u COLORS", (unsigned)(ledIdx + 1));

		doneOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "BACK", -1);

		redOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "RED:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, cnv->h - numOptions * itemHeight, 111, "%3u       ", settings->ledColors[ledIdx].red);

		greenOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "GREEN:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, cnv->h - numOptions * itemHeight, 111, "%3u       ", settings->ledColors[ledIdx].green);

		blueOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "BLUE:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, cnv->h - numOptions * itemHeight, 111, "%3u       ", settings->ledColors[ledIdx].blue);

		selOption = numOptions - 1 - uiPrvMenu(cnv, numOptions - 1 - selOption, numOptions, &button);

		if (button == KEY_BIT_B || selOption == doneOption)
			return;

		if (selOption == redOption || selOption == greenOption || selOption == blueOption) {
			uint8_t *component;

			if (selOption == redOption)
				component = &settings->ledColors[ledIdx].red;
			else if (selOption == greenOption)
				component = &settings->ledColors[ledIdx].green;
			else
				component = &settings->ledColors[ledIdx].blue;

			if (button == KEY_BIT_LEFT) {
				if (!*component)
					continue;
				(*component)--;
			}
			else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
				if (*component == 0xff)
					continue;
				(*component)++;
			}
			else {
				continue;
			}

			(void)settingsSet(settings);
			defconApplyLedSettingsFromStruct(settings);
		}
	}
}

static bool uiPrvLedMenuOptionRow(int_fast8_t option, int_fast8_t firstLedOption, uint_fast8_t itemHeight,
        uint_fast16_t listTop, uint_fast16_t bottomStart, uint_fast8_t topLedIndex, uint_fast8_t ledItemsOnscreen, int32_t *rowOut)
{
        if (option < firstLedOption) {
                *rowOut = bottomStart + (firstLedOption - 1 - option) * itemHeight;
                return true;
        }

        uint_fast8_t ledIndex = option - firstLedOption;

        if (ledIndex < topLedIndex || ledIndex >= topLedIndex + ledItemsOnscreen)
                return false;

        *rowOut = listTop + (ledIndex - topLedIndex) * itemHeight;
        return true;
}

static void uiPrvLedColorsMenu(struct Canvas *cnv, struct Settings *settings)
{
        const int_fast8_t doneOption = 0;
        const int_fast8_t modeOption = 1;
        const int_fast8_t brightnessOption = 2;
        const int_fast8_t firstLedOption = 3;
        const int_fast8_t totalOptions = firstLedOption + NUM_WS2812s;
        int_fast8_t selOption = doneOption;
        uint_fast8_t itemHeight;
        uint_fast16_t listTop, bottomStart, listHeight;
        uint_fast8_t ledItemsOnscreen, topLedIndex = 0, prevTopLedIndex = UINT8_MAX;
        int_fast8_t prevSelOption = -1;
        bool redraw = true;

        cnv->font = FontLarge;
        itemHeight = uiPrvGlyphHeight(cnv) + 1;

        listTop = itemHeight * 2;
        bottomStart = cnv->h - firstLedOption * itemHeight;
        if (bottomStart <= listTop)
                bottomStart = listTop + itemHeight;

        listHeight = (bottomStart > listTop) ? (bottomStart - listTop) : itemHeight;
        ledItemsOnscreen = listHeight / itemHeight;
        if (!ledItemsOnscreen)
                ledItemsOnscreen = 1;
        if (ledItemsOnscreen > NUM_WS2812s)
                ledItemsOnscreen = NUM_WS2812s;

        while (1) {
                if (redraw || prevTopLedIndex != topLedIndex) {
                        uint_fast8_t i;
                        uint_fast8_t scrollWidth;

                        uiPrvReset(cnv, false);
                        cnv->font = FontLarge;

                        cnv->foreColor = 11;
                        uiPuts(cnv, itemHeight, 10, "LED SETTINGS", -1);

                        cnv->foreColor = 11;
                        uiPuts(cnv, bottomStart + 2 * itemHeight, 10, "BACK", -1);

                        cnv->foreColor = 11;
                        uiPuts(cnv, bottomStart + itemHeight, 10, "MODE:", -1);
                        cnv->foreColor = 15;
                        {
                                const char *modeName = uiPrvLedModeName(settings->ledMode);
                                char modeBuf[9];
                                size_t len = strlen(modeName);

                                if (len > sizeof(modeBuf) - 1)
                                        len = sizeof(modeBuf) - 1;

                                memcpy(modeBuf, modeName, len);
                                while (len < sizeof(modeBuf) - 1)
                                        modeBuf[len++] = ' ';
                                modeBuf[len] = '\0';

                                uiPuts(cnv, bottomStart + itemHeight, 111, modeBuf, -1);
                        }

                        cnv->foreColor = 11;
                        uiPuts(cnv, bottomStart, 10, "BRIGHTNESS:", -1);
                        cnv->foreColor = 15;
                        uiPrintf(cnv, bottomStart, 111, "%3u       ", settings->ledGlobalBrightness);

                        scrollWidth = uiPrvDrawScrollbar(cnv, listTop, NUM_WS2812s, topLedIndex, ledItemsOnscreen);
                        (void)scrollWidth;

                        for (i = 0; i < ledItemsOnscreen; i++) {
                                uint_fast8_t ledIndex = topLedIndex + i;
                                const struct SettingsLedColor *led = &settings->ledColors[ledIndex];

                                cnv->foreColor = 11;
                                uiPrintf(cnv, listTop + i * itemHeight, 10, "LED %u:", (unsigned)(ledIndex + 1));
                                cnv->foreColor = 15;
                                uiPrintf(cnv, listTop + i * itemHeight, 111, "R:%3u G:%3u B:%3u   ", led->red, led->green, led->blue);
                        }

                        prevTopLedIndex = topLedIndex;
                        redraw = false;
                        prevSelOption = -1;
                }

                if (prevSelOption != selOption) {
                        int32_t row;

                        if (prevSelOption >= 0 && uiPrvLedMenuOptionRow(prevSelOption, firstLedOption, itemHeight, listTop,
                                        bottomStart, topLedIndex, ledItemsOnscreen, &row)) {
                                cnv->foreColor = 0;
                                uiPrvDrawOneChar(cnv, row, 1, MENU_SELECTION_CHAR);
                        }

                        if (uiPrvLedMenuOptionRow(selOption, firstLedOption, itemHeight, listTop, bottomStart, topLedIndex,
                                        ledItemsOnscreen, &row)) {
                                cnv->foreColor = 15;
                                uiPrvDrawOneChar(cnv, row, 1, MENU_SELECTION_CHAR);
                        }

                        prevSelOption = selOption;
                }

                switch (uiPrvRecvKeypress()) {
                        case KEY_BIT_DOWN:
                                if (selOption < totalOptions - 1) {
                                        selOption++;
                                        if (selOption >= firstLedOption) {
                                                uint_fast8_t ledIndex = selOption - firstLedOption;

                                                if (ledIndex >= topLedIndex + ledItemsOnscreen) {
                                                        topLedIndex = ledIndex - ledItemsOnscreen + 1;
                                                        if (topLedIndex > NUM_WS2812s - ledItemsOnscreen)
                                                                topLedIndex = NUM_WS2812s - ledItemsOnscreen;
                                                }
                                        }
                                }
                                break;

                        case KEY_BIT_UP:
                                if (selOption > 0) {
                                        selOption--;
                                        if (selOption >= firstLedOption) {
                                                uint_fast8_t ledIndex = selOption - firstLedOption;

                                                if (ledIndex < topLedIndex)
                                                        topLedIndex = ledIndex;
                                        }
                                }
                                break;

                        case KEY_BIT_B:
                                return;

                        case KEY_BIT_LEFT:
                                if (selOption == doneOption) {
                                        return;
                                }
                                else if (selOption == modeOption) {
                                        uint8_t previous = settings->ledMode;

                                        if (settings->ledMode == 0)
                                                settings->ledMode = LedModeCount - 1;
                                        else
                                                settings->ledMode--;

                                        if (settings->ledMode != previous) {
                                                (void)settingsSet(settings);
                                                defconApplyLedSettingsFromStruct(settings);
                                                redraw = true;
                                        }
                                }
                                else if (selOption == brightnessOption) {
                                        if (settings->ledGlobalBrightness) {
                                                settings->ledGlobalBrightness--;
                                                (void)settingsSet(settings);
                                                defconApplyLedSettingsFromStruct(settings);
                                                redraw = true;
                                        }
                                }
                                else if (selOption >= firstLedOption) {
                                        uiPrvLedColorAdjust(cnv, settings, selOption - firstLedOption);
                                        redraw = true;
                                        prevTopLedIndex = UINT8_MAX;
                                }
                                break;

                        case KEY_BIT_RIGHT:
                        case KEY_BIT_A:
                                if (selOption == doneOption) {
                                        return;
                                }
                                else if (selOption == modeOption) {
                                        settings->ledMode = (settings->ledMode + 1) % LedModeCount;
                                        (void)settingsSet(settings);
                                        defconApplyLedSettingsFromStruct(settings);
                                        redraw = true;
                                }
                                else if (selOption == brightnessOption) {
                                        if (settings->ledGlobalBrightness != 0xff) {
                                                settings->ledGlobalBrightness++;
                                                (void)settingsSet(settings);
                                                defconApplyLedSettingsFromStruct(settings);
                                                redraw = true;
                                        }
                                }
                                else if (selOption >= firstLedOption) {
                                        uiPrvLedColorAdjust(cnv, settings, selOption - firstLedOption);
                                        redraw = true;
                                        prevTopLedIndex = UINT8_MAX;
                                }
                                break;
                }
        }
}

static bool __attribute__((noinline)) uiPrvSettings(struct Canvas *cnv)		//return true if anything for the current game may have changes
{
	bool restartCurGame = false;
	int_fast8_t selOption = 0;
	struct Settings settings;
        static const char rotationNames[][10] = {
                "GAME    ",
                "BADGE   ",
                "AUTO    ",
	};
	uint_fast8_t itemHeight;
	
	settingsGet(&settings);
	
	if (settings.rotationMode >= RotationModeCount)
		settings.rotationMode = RotationModeAuto;
	
	uiPrvReset(cnv, false);
	itemHeight = uiPrvGlyphHeight(cnv) + 1;
	
	while (1) {
		
		int_fast8_t numOptions = 0, doneOption, cgbOption, speedOption, contrastOption = -1, brightnessOption = -1, rotationOption, upscaleOption, ledOption = -1;
		uint8_t button = KEY_BIT_A | KEY_BIT_B | KEY_BIT_LEFT | KEY_BIT_RIGHT;
		static const char speeds[][8] = DISP_SPEED_NAMES;
		
		doneOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "DONE", -1);
		
		cgbOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "COLOR:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 111, settings.actLikeGBC ? "YES       " : "NO       ", -1);
	
		upscaleOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "UPSCALE:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 111, settings.upscale ? "YES       " : "NO       ", -1);
	
		speedOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "SPEED:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 111, speeds[settings.speed], sizeof(speeds[settings.speed]));
	
	#ifdef DISP_CONTRAST_ADJUSTABLE
		contrastOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "CONTRAST:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, cnv->h - numOptions * itemHeight, 111, "%u         ", settings.contrast);
	#endif

	#ifdef DISP_BRIGHTNESS_ADJUSTABLE
		brightnessOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "BRIGHTNESS:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, cnv->h - numOptions * itemHeight, 111, "%u         ", settings.brightness);
	#endif

		rotationOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "ROTATION:", -1);
		cnv->foreColor = 15;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 111, rotationNames[settings.rotationMode], sizeof(rotationNames[0]));

		ledOption = numOptions++;
		cnv->foreColor = 11;
		uiPuts(cnv, cnv->h - numOptions * itemHeight, 10, "LED COLORS:", -1);
		cnv->foreColor = 15;
		uiPrintf(cnv, cnv->h - numOptions * itemHeight, 111, "%-6s B:%3u   ", uiPrvLedModeName(settings.ledMode), settings.ledGlobalBrightness);

		selOption = numOptions - 1 - uiPrvMenu(cnv,  numOptions - 1 - selOption, numOptions, &button);
		if (button == KEY_BIT_B || selOption == doneOption){
			
			settingsSet(&settings);
			return restartCurGame;
		}
		
		if (selOption == upscaleOption) {
			
			settings.upscale = !settings.upscale;
		}
		
		if (selOption == cgbOption) {
			
			restartCurGame = true;
			settings.actLikeGBC = !settings.actLikeGBC;
		}
		
		if (selOption == speedOption) {
			
			static const uint8_t speeds[] = DISP_SPEED_SETTINGS;
			
			if (button == KEY_BIT_LEFT) {
				if (settings.speed)
					settings.speed--;
				else
					continue;
			}
			else if (button == KEY_BIT_RIGHT) {
				if (settings.speed != sizeof(speeds) / sizeof(*speeds) - 1)
					settings.speed++;
				else continue;
			}
			else if (button == KEY_BIT_A)
				settings.speed++;
			
			dispOff();
			dispInit(speeds[settings.speed]);
			dispSetContrast(settings.contrast);		//must be reset
		}
		
		if (selOption == contrastOption) {
			
			if (button == KEY_BIT_LEFT) {
				if (settings.contrast)
					settings.contrast--;
				else
					continue;
			}
			else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
				if (settings.contrast != 0x1f)
					settings.contrast++;
				else
					continue;
			}
				
			dispSetContrast(settings.contrast);
		}

                if (selOption == rotationOption) {

                        if (button == KEY_BIT_LEFT) {
                                settings.rotationMode = (settings.rotationMode + RotationModeCount - 1) % RotationModeCount;
                        }
                        else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
                                settings.rotationMode = (settings.rotationMode + 1) % RotationModeCount;
                        }

                        defconOrientationApplySettings(&settings);
                        continue;
                }

		if (selOption == ledOption) {
			uiPrvLedColorsMenu(cnv, &settings);
			uiPrvReset(cnv, false);
			continue;
		}

		if (selOption == brightnessOption) {
			
			if (button == KEY_BIT_LEFT) {
				if (settings.brightness)
					settings.brightness--;
				else
					continue;
			}
			else if (button == KEY_BIT_RIGHT || button == KEY_BIT_A) {
				if (settings.brightness != 0x1f)
					settings.brightness++;
				else
					continue;
			}
				
			dispSetBrightness(settings.brightness);
		}
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
		uint8_t *buf = mbcPrvGetWramBuf();
		
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
	
	static bool __attribute__((noinline)) uiPrvConfirmRomSelection(struct Canvas *cnv, struct FatfsVol *vol, struct RomOption *rom)
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
		filR = fatfsFileOpenAt(dir, rom->name, OPEN_MODE_READ);
		fatfsDirClose(dir);
		dir = NULL;
		if (!filR) {
			uiAlert(cnv, "Cannot open ROM file", DialogTypeOk);
			goto out;
		}
		
		dir = fatfsDirOpen(vol, "/SAVE");
		if (dir) {
			filS = fatfsFileOpenAt(dir, rom->name, OPEN_MODE_READ);
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
		row += 1 + uiPrvGlyphHeight(cnv) * uiPrvDrawWrappedString(cnv, rom->name, row, col + 55);
		
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
			(void)uiPrvSetGamePath(rom->name);
			
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
					if (uiPrvConfirmRomSelection(cnv, vol, cur)) {
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

static bool __attribute__((noinline)) uiPrvCommon(void)		//return true if emulator needs to restart
{
	struct Canvas canvas = CANVAS_INITIALIZER, *cnv = &canvas;
	bool ret = false;
	

	uiPrvReset(cnv, false);
	uiPrvFwUpdate(cnv, true);

	while (1) {
		
		int_fast8_t playOption = -1, selectGameOption = -1, aboutOption, settingsOption, fwUpdateOption, rollOption, powerOffOption, numOptions = 0, selOption, itemHeight;
		enum RomColorSupport romColorSupport;
		char name[ROM_NAME_LEN + 1];
		uint32_t ramSz = 0;
		bool validRom;
				
		uiPrvReset(cnv, false);
		itemHeight = uiPrvGlyphHeight(cnv) + 1;
		
		validRom = uiPrvHaveValidRom(name, &romColorSupport, &ramSz);
		
		if (validRom) {
			
			uint_fast8_t height;
			
		#ifdef NO_SD_CARD
				height = 4;
		#else
				height = 6;
		#endif
			
			static const char colorTypes[][5] = {
				[RomNoColor] = "DMG ",
				[RomColorEnhanced] = "",
				[RomColorRequired] = "CGB ",
			};
			
			uiPrintf(cnv, cnv->h - height * itemHeight, 10, "Play %sgame '%s'?", colorTypes[romColorSupport], name);
			playOption = numOptions++;
		}
		#ifndef NO_SD_CARD
			selectGameOption = numOptions++;
			uiPuts(cnv, cnv->h - 5 * itemHeight, 10, validRom ? "Select Another Game" : "Select a Game", -1);
			fwUpdateOption = numOptions++;
			uiPuts(cnv, cnv->h - 4 * itemHeight, 10, "Firmware Update", -1);
		#endif
		settingsOption = numOptions++;
		uiPuts(cnv, cnv->h - 3 * itemHeight, 10, "Settings", -1);
		aboutOption = numOptions++;
		uiPuts(cnv, cnv->h - 2 * itemHeight, 10, "About...", -1);
		powerOffOption = numOptions++;
		uiPuts(cnv, cnv->h - 1 * itemHeight, 10, "Power Off", -1);

		selOption = uiPrvMenu(cnv, 0, numOptions, NULL);
		if (selOption == playOption) {
			
			uiPrvLoadSavestate();
			uiPrvReset(cnv, false);
			break;
		}
	#ifndef NO_SD_CARD
		else if (selOption == selectGameOption) {
			
				if (uiPrvSelectRom(cnv, validRom ? ramSz : 0))
					ret = true;
		}
		else if (selOption == fwUpdateOption) {
			
			uiPrvReset(cnv, false);
			uiPrvFwUpdate(cnv, false);
		}
	#endif
		else if (selOption == aboutOption) {
			
			uiAlert(cnv, 
					"uGB/TWISTED NEMATIC WITH MONULAR BEEPS\n"
					"HW: Rachel, MP, George, Matt, Louis\n"
					"SW: dmitryGR\n"
					"\n"
					"\xC2\xA9 2024\0""2024"
				, DialogTypeOk);
		}
		else if (selOption == settingsOption) {
			
			if (uiPrvSettings(cnv))
				ret = true;
		}
		else if (selOption == powerOffOption) {
			
			doSleep();
		}
	}
	
	return ret;
}

void uiPreGame(void)
{
	struct Canvas canvas = CANVAS_INITIALIZER, *cnv = &canvas;

	uiPrvReset(cnv, false);
	uiPrvFwUpdate(cnv, true);


	if (!uiPrvHaveValidRom(NULL, NULL, NULL))
		(void)uiPrvCommon();
	else
		uiPrvLoadSavestate();
}

void uiInGame(void)
{
	struct Canvas canvas = CANVAS_INITIALIZER, *cnv = &canvas;
	uint32_t romSzExpected, ramSzExpected;
	
	uiPrvReset(cnv, false);

	//we do not expect issues with analyzing rom here... we were just playing it...
	if (!uiSaveSavestate())
		uiAlert(cnv, "Failed to save state to flash", DialogTypeOk);

	if (uiPrvCommon())
		gbAbort();
	
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

