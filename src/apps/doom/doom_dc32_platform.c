#include "doom_dc32.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gb.h"
#include "picodoom.h"
#include "pico/sem.h"
#include "doom/doomstat.h"
#include "doom/r_data.h"
#include "doomkeys.h"
#include "d_event.h"
#include "i_sound.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_config.h"
#include "qspi.h"
#include "tables.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

uint8_t (*frame_buffer)[SCREENWIDTH * MAIN_VIEWHEIGHT];
uint8_t *doomDc32ListBuffer;
uint32_t doomDc32ListBufferSize;

static uint8_t mZone[DOOM_DC32_ZONE_BYTES] __attribute__((aligned(8)));
static uint8_t *mHeapBase;
static uint32_t mHeapSize;
static uint32_t mHeapUsed;
static bool mVideoInitialized;

struct DoomDc32AllocHeader {
	uint32_t size;
};

boolean screenvisible = true;
boolean screensaver_mode = false;
isb_int8_t usegamma = 0;
unsigned int joywait = 0;
uint8_t restart_song_state;
pixel_t *I_VideoBuffer;
should_be_const constcharstar video_driver = "";
should_be_const constcharstar window_position = "center";
int screen_width = SCREENWIDTH;
int screen_height = SCREENHEIGHT;
int fullscreen = true;
int aspect_ratio_correct = false;
int integer_scaling = false;
int vga_porch_flash = false;
int force_software_renderer = false;

static uint16_t mPalette[256];
static uint16_t mSharedPal[NUM_SHARED_PALETTES][16];
static int8_t mNextPalette = 0;
static const patch_t *mStatusBar;

semaphore_t render_frame_ready;
semaphore_t display_frame_freed;
uint8_t display_frame_index;
uint8_t display_overlay_index;
uint8_t display_video_type;
uint8_t next_video_type;
uint8_t next_frame_index;
uint8_t next_overlay_index;
uint8_t *wipe_yoffsets;
int16_t *wipe_yoffsets_raw;
uint32_t *wipe_linelookup;
volatile uint8_t wipe_min;
#if !DEMO1_ONLY
uint8_t *next_video_scroll;
uint8_t *video_scroll;
#endif

static uint16_t doomDc32Rgb565(uint32_t r, uint32_t g, uint32_t b)
{
	return (uint16_t)(((r & 0xf8u) << 8) | ((g & 0xfcu) << 3) | (b >> 3));
}

static uint32_t doomDc32AlignUp(uint32_t value, uint32_t align)
{
	return (value + align - 1u) & ~(align - 1u);
}

static void doomDc32InitScratch(void)
{
	uint8_t *p = DOOM_DC32_CART_SCRATCH;

	frame_buffer = (uint8_t (*)[SCREENWIDTH * MAIN_VIEWHEIGHT])p;
	p += DOOM_DC32_FRAME_BYTES;
	p = (uint8_t*)doomDc32AlignUp((uint32_t)(uintptr_t)p, 4u);
	doomDc32ListBuffer = p;
	doomDc32ListBufferSize = DOOM_DC32_LIST_BYTES;
	p += doomDc32ListBufferSize;
	p = (uint8_t*)doomDc32AlignUp((uint32_t)(uintptr_t)p, 8u);
	mHeapBase = p;
	mHeapSize = DOOM_DC32_CART_SCRATCH_SIZE - (uint32_t)(p - DOOM_DC32_CART_SCRATCH);
	mHeapUsed = 0;
	memset(DOOM_DC32_CART_SCRATCH, 0, DOOM_DC32_FRAME_BYTES + doomDc32ListBufferSize);
}

void *malloc(size_t size)
{
	struct DoomDc32AllocHeader *hdr;
	uint32_t pos;
	uint32_t total;

	if (!size)
		size = 1;
	if (size > UINT32_MAX - sizeof(*hdr) - 7u)
		return NULL;
	total = doomDc32AlignUp((uint32_t)size + sizeof(*hdr), 8u);
	if (!mHeapBase || mHeapUsed + total > mHeapSize)
		return NULL;
	pos = mHeapUsed;
	mHeapUsed += total;
	hdr = (struct DoomDc32AllocHeader*)(mHeapBase + pos);
	hdr->size = (uint32_t)size;
	return hdr + 1;
}

void *calloc(size_t nmemb, size_t size)
{
	size_t total = nmemb * size;
	void *ptr = malloc(total);

	if (ptr)
		memset(ptr, 0, total);
	return ptr;
}

void free(void *ptr)
{
	(void)ptr;
}

void *realloc(void *ptr, size_t size)
{
	struct DoomDc32AllocHeader *hdr;
	void *next;
	uint32_t oldSize;

	if (!ptr)
		return malloc(size);
	next = malloc(size);
	if (next && size) {
		hdr = ((struct DoomDc32AllocHeader*)ptr) - 1;
		oldSize = hdr->size;
		memcpy(next, ptr, oldSize < size ? oldSize : size);
	}
	return next;
}

void panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	doomDc32SetErrorV(fmt ? fmt : "panic", ap);
	va_end(ap);
	doomDc32Exit(1);
}

void panic_unsupported(void)
{
	panic("unsupported");
}

int __fast_mul(int a, int b)
{
	return a * b;
}

void exit(int status)
{
	doomDc32Exit(status);
}

static uint64_t doomDc32HostTicks(void)
{
	return doomDc32Host && doomDc32Host->getTime ? doomDc32Host->getTime() : 0;
}

static uint64_t doomDc32TicksToUsec(uint64_t ticks)
{
	return (ticks / TICKS_PER_SECOND) * 1000000ull +
		((ticks % TICKS_PER_SECOND) * 1000000ull) / TICKS_PER_SECOND;
}

uint64_t time_us_64(void)
{
	return doomDc32TicksToUsec(doomDc32HostTicks());
}

void sleep_ms(uint32_t ms)
{
	if (doomDc32Host && doomDc32Host->delayMsec)
		doomDc32Host->delayMsec(ms);
}

void I_AtExit(atexit_func_t func, boolean run_on_error)
{
	(void)func;
	(void)run_on_error;
}

void I_Tactile(int on, int off, int total)
{
	(void)on;
	(void)off;
	(void)total;
}

byte *I_ZoneBase(int *size)
{
	*size = sizeof(mZone);
	return mZone;
}

void I_PrintBanner(const char *msg)
{
	if (doomDc32Host && doomDc32Host->log)
		doomDc32Host->log("%s\n", msg);
}

void I_PrintDivider(void)
{
	if (doomDc32Host && doomDc32Host->log)
		doomDc32Host->log("========================================\n");
}

void I_PrintStartupBanner(const char *gamedescription)
{
	I_PrintDivider();
	I_PrintBanner(gamedescription);
	I_PrintDivider();
}

boolean I_ConsoleStdout(void)
{
	return false;
}

void I_Init(void)
{
	doomDc32InitScratch();
}

void I_Quit(void)
{
	doomDc32Exit(0);
}

void I_Error(const char *error, ...)
{
	va_list ap;

	va_start(ap, error);
	doomDc32SetErrorV(error ? error : "I_Error", ap);
	va_end(ap);
	doomDc32Exit(1);
}

void __assert_func(const char *file, int line, const char *func, const char *failedexpr)
{
	(void)func;
	doomDc32SetError("Assert %s:%d %s", file ? file : "?", line, failedexpr ? failedexpr : "?");
	doomDc32Exit(1);
}

void __assert(const char *file, int line, const char *failedexpr)
{
	__assert_func(file, line, NULL, failedexpr);
}

int printf(const char *fmt, ...)
{
	(void)fmt;
	return 0;
}

int vprintf(const char *fmt, va_list ap)
{
	(void)fmt;
	(void)ap;
	return 0;
}

static void doomDc32FmtPut(char **dstP, size_t *remainP, int *countP, char ch)
{
	(*countP)++;
	if (*remainP > 1u) {
		**dstP = ch;
		(*dstP)++;
		(*remainP)--;
	}
}

static void doomDc32FmtStr(char **dstP, size_t *remainP, int *countP, const char *str)
{
	if (!str)
		str = "(null)";
	while (*str)
		doomDc32FmtPut(dstP, remainP, countP, *str++);
}

static void doomDc32FmtU32(char **dstP, size_t *remainP, int *countP,
	uint32_t value, uint32_t base, int width, bool padZero, bool neg)
{
	char tmp[11];
	int n = 0;

	if (!value)
		tmp[n++] = '0';
	while (value && n < (int)sizeof(tmp)) {
		uint32_t digit = value % base;

		tmp[n++] = (char)(digit < 10u ? '0' + digit : 'a' + digit - 10u);
		value /= base;
	}
	if (neg)
		width--;
	while (width > n)
		doomDc32FmtPut(dstP, remainP, countP, padZero ? '0' : ' ');
	if (neg)
		doomDc32FmtPut(dstP, remainP, countP, '-');
	while (n)
		doomDc32FmtPut(dstP, remainP, countP, tmp[--n]);
}

int vsnprintf(char *dst, size_t dstLen, const char *fmt, va_list ap)
{
	char *out = dst;
	size_t remain = dstLen;
	int count = 0;

	if (!fmt)
		fmt = "";
	if (dst && dstLen)
		*dst = 0;
	while (*fmt) {
		char ch = *fmt++;
		bool padZero = false;
		int width = 0;
		int precision = -1;
		bool longArg = false;

		if (ch != '%') {
			doomDc32FmtPut(&out, &remain, &count, ch);
			continue;
		}
		ch = *fmt++;
		while (ch == '-' || ch == '+' || ch == ' ' || ch == '#') {
			ch = *fmt++;
		}
		if (ch == '0') {
			padZero = true;
			ch = *fmt++;
		}
		while (ch >= '0' && ch <= '9') {
			width = width * 10 + ch - '0';
			ch = *fmt++;
		}
		if (ch == '.') {
			precision = 0;
			ch = *fmt++;
			while (ch >= '0' && ch <= '9') {
				precision = precision * 10 + ch - '0';
				ch = *fmt++;
			}
		}
		if (precision > width) {
			width = precision;
			padZero = true;
		}
		if (ch == 'l') {
			longArg = true;
			ch = *fmt++;
			if (ch == 'l')
				ch = *fmt++;
		}
		switch (ch) {
			case '%':
				doomDc32FmtPut(&out, &remain, &count, '%');
				break;
			case 'c':
				doomDc32FmtPut(&out, &remain, &count, (char)va_arg(ap, int));
				break;
			case 's':
				doomDc32FmtStr(&out, &remain, &count, va_arg(ap, const char*));
				break;
			case 'd':
			case 'i': {
				int32_t value = longArg ? (int32_t)va_arg(ap, long) : va_arg(ap, int);
				uint32_t mag = value < 0 ? (uint32_t)(-value) : (uint32_t)value;

				doomDc32FmtU32(&out, &remain, &count, mag, 10u, width, padZero, value < 0);
				break;
			}
			case 'u':
				doomDc32FmtU32(&out, &remain, &count,
					longArg ? (uint32_t)va_arg(ap, unsigned long) : va_arg(ap, uint32_t),
					10u, width, padZero, false);
				break;
			case 'p':
				doomDc32FmtStr(&out, &remain, &count, "0x");
				doomDc32FmtU32(&out, &remain, &count, (uint32_t)(uintptr_t)va_arg(ap, void*),
					16u, width, padZero, false);
				break;
			case 'x':
			case 'X':
				doomDc32FmtU32(&out, &remain, &count,
					longArg ? (uint32_t)va_arg(ap, unsigned long) : va_arg(ap, uint32_t),
					16u, width, padZero, false);
				break;
			default:
				doomDc32FmtPut(&out, &remain, &count, ch ? ch : '?');
				break;
		}
	}
	if (dst && dstLen)
		*out = 0;
	return count;
}

int snprintf(char *dst, size_t dstLen, const char *fmt, ...)
{
	va_list ap;
	int result;

	va_start(ap, fmt);
	result = vsnprintf(dst, dstLen, fmt, ap);
	va_end(ap);
	return result;
}

int sprintf(char *dst, const char *fmt, ...)
{
	va_list ap;
	int result;

	va_start(ap, fmt);
	result = vsnprintf(dst, (size_t)-1, fmt, ap);
	va_end(ap);
	return result;
}

int fprintf(FILE *stream, const char *fmt, ...)
{
	(void)stream;
	(void)fmt;
	return 0;
}

int vfprintf(FILE *stream, const char *fmt, va_list ap)
{
	(void)stream;
	(void)fmt;
	(void)ap;
	return 0;
}

int puts(const char *s)
{
	(void)s;
	return 0;
}

int putchar(int c)
{
	return c;
}

void perror(const char *s)
{
	(void)s;
}

void *I_Realloc(void *ptr, size_t size)
{
	void *result = realloc(ptr, size);

	if (!result && size)
		I_Error("I_Realloc failed");
	return result;
}

int I_GetTime(void)
{
	return (int)((doomDc32HostTicks() * TICRATE) / TICKS_PER_SECOND);
}

int I_GetTimeMS(void)
{
	return (int)((doomDc32HostTicks() * 1000ull) / TICKS_PER_SECOND);
}

void I_Sleep(int ms)
{
	sleep_ms((uint32_t)ms);
}

void I_WaitVBL(int count)
{
	I_Sleep((count * 1000) / 70);
}

void I_InitTimer(void)
{
}

static void doomDc32PostKey(int key, bool down)
{
	event_t event;

	if (!key)
		return;
	memset(&event, 0, sizeof(event));
	event.type = down ? ev_keydown : ev_keyup;
	event.data1 = key;
	event.data2 = key;
	event.data3 = key >= 32 && key < 127 ? key : 0;
	D_PostEvent(&event);
}

static void doomDc32PollKeys(void)
{
	static uint_fast16_t prevKeys;
	uint_fast16_t keys = doomDc32Host && doomDc32Host->uiKeysRaw ? doomDc32Host->uiKeysRaw() : 0;
	uint_fast16_t changed = keys ^ prevKeys;
	bool pauseDown = (keys & (KEY_BIT_START | KEY_BIT_SEL)) == (KEY_BIT_START | KEY_BIT_SEL);
	bool prevPauseDown = (prevKeys & (KEY_BIT_START | KEY_BIT_SEL)) == (KEY_BIT_START | KEY_BIT_SEL);

	if ((keys & UI_KEY_BIT_CENTER) && !(prevKeys & UI_KEY_BIT_CENTER) &&
			doomDc32Host && doomDc32Host->portMenu) {
		if (!doomDc32Host->portMenu(&doomDc32Canvas))
			doomDc32Exit(0);
		keys = doomDc32Host->uiKeysRaw ? doomDc32Host->uiKeysRaw() : 0;
		changed = keys ^ prevKeys;
	}
	if (changed & KEY_BIT_LEFT)
		doomDc32PostKey(KEY_LEFTARROW, (keys & KEY_BIT_LEFT) != 0);
	if (changed & KEY_BIT_RIGHT)
		doomDc32PostKey(KEY_RIGHTARROW, (keys & KEY_BIT_RIGHT) != 0);
	if (changed & KEY_BIT_UP)
		doomDc32PostKey(KEY_UPARROW, (keys & KEY_BIT_UP) != 0);
	if (changed & KEY_BIT_DOWN)
		doomDc32PostKey(KEY_DOWNARROW, (keys & KEY_BIT_DOWN) != 0);
	if (pauseDown != prevPauseDown) {
		if (prevKeys & KEY_BIT_START)
			doomDc32PostKey(KEY_ENTER, false);
		if (prevKeys & KEY_BIT_SEL)
			doomDc32PostKey(KEY_ESCAPE, false);
		doomDc32PostKey(KEY_PAUSE, pauseDown);
	} else if (!pauseDown) {
		if (changed & KEY_BIT_START)
			doomDc32PostKey(KEY_ENTER, (keys & KEY_BIT_START) != 0);
		if (changed & KEY_BIT_SEL)
			doomDc32PostKey(KEY_ESCAPE, (keys & KEY_BIT_SEL) != 0);
	}
	if (changed & KEY_BIT_A)
		doomDc32PostKey(KEY_RCTRL, (keys & KEY_BIT_A) != 0);
	if (changed & KEY_BIT_B)
		doomDc32PostKey(' ', (keys & KEY_BIT_B) != 0);
	prevKeys = keys;
}

void I_StartTextInput(int x1, int y1, int x2, int y2)
{
	(void)x1;
	(void)y1;
	(void)x2;
	(void)y2;
}

void I_StopTextInput(void)
{
}

void I_BindInputVariables(void)
{
}

void I_GetEvent(void)
{
	doomDc32PollKeys();
}

void I_GetEventTimeout(int key_timeout)
{
	(void)key_timeout;
	doomDc32PollKeys();
}

void *doomDc32VpatchLists(void)
{
	static vpatchlists_t lists;

	return &lists;
}

static void doomDc32UpdatePalette(void)
{
	static const uint8_t *playpal;
	static bool calculatePalettes;

	if (mNextPalette == -1)
		return;
	if (!playpal) {
		lumpindex_t l = W_GetNumForName("PLAYPAL");

		playpal = W_CacheLumpNum(l, PU_STATIC);
		calculatePalettes = W_LumpLength(l) == 768;
	}
	if (!calculatePalettes || !mNextPalette) {
		const uint8_t *doompalette = playpal + mNextPalette * 768;

		for (int i = 0; i < 256; i++) {
			int r = *doompalette++;
			int g = *doompalette++;
			int b = *doompalette++;

			if (usegamma) {
				r = gammatable[usegamma - 1][r];
				g = gammatable[usegamma - 1][g];
				b = gammatable[usegamma - 1][b];
			}
			mPalette[i] = doomDc32Rgb565((uint32_t)r, (uint32_t)g, (uint32_t)b);
		}
	}
	else {
		const uint8_t *doompalette = playpal;
		int mul, r0, g0, b0;

		if (mNextPalette < 9) {
			mul = mNextPalette * 65536 / 9;
			r0 = 255;
			g0 = b0 = 0;
		}
		else if (mNextPalette < 13) {
			mul = (mNextPalette - 8) * 65536 / 8;
			r0 = 215;
			g0 = 186;
			b0 = 69;
		}
		else {
			mul = 65536 / 8;
			r0 = b0 = 0;
			g0 = 256;
		}
		for (int i = 0; i < 256; i++) {
			int r = *doompalette++;
			int g = *doompalette++;
			int b = *doompalette++;

			r += ((r0 - r) * mul) >> 16;
			g += ((g0 - g) * mul) >> 16;
			b += ((b0 - b) * mul) >> 16;
			mPalette[i] = doomDc32Rgb565((uint32_t)r, (uint32_t)g, (uint32_t)b);
		}
	}
	mNextPalette = -1;
	for (int i = 0; i < NUM_SHARED_PALETTES; i++) {
		patch_t *patch = resolve_vpatch_handle(vpatch_for_shared_palette[i]);

		if (!patch || !vpatch_has_shared_palette(patch))
			continue;
		for (int j = 0; j < 16; j++)
			mSharedPal[i][j] = mPalette[vpatch_palette(patch)[j]];
	}
}

static uint32_t doomDc32DrawVpatch(uint16_t *dest, patch_t *patch, vpatchlist_t *vp, uint32_t off)
{
	int repeat = vp->entry.repeat;
	int w = vpatch_width(patch);
	const uint8_t *data0 = vpatch_data(patch);
	const uint8_t *data = data0 + off;

	dest += vp->entry.x;
	if (!vpatch_has_shared_palette(patch)) {
		const uint8_t *pal = vpatch_palette(patch);

		switch (vpatch_type(patch)) {
			case vp4_runs: {
				uint16_t *p = dest;
				uint16_t *pend = dest + w;
				uint8_t gap;

				while (0xff != (gap = *data++)) {
					p += gap;
					int len = *data++;
					for (int i = 1; i < len; i += 2) {
						uint32_t v = *data++;
						*p++ = mPalette[pal[v & 0xf]];
						*p++ = mPalette[pal[v >> 4]];
					}
					if (len & 1)
						*p++ = mPalette[pal[(*data++) & 0xf]];
					if (p >= pend)
						break;
				}
				break;
			}
			case vp4_alpha: {
				uint16_t *p = dest;

				for (int i = 0; i < w / 2; i++) {
					uint32_t v = *data++;
					if (v & 0xf)
						p[0] = mPalette[pal[v & 0xf]];
					if (v >> 4)
						p[1] = mPalette[pal[v >> 4]];
					p += 2;
				}
				if (w & 1) {
					uint32_t v = *data++;
					if (v & 0xf)
						p[0] = mPalette[pal[v & 0xf]];
				}
				break;
			}
			case vp4_solid: {
				uint16_t *p = dest;

				for (int i = 0; i < w / 2; i++) {
					uint32_t v = *data++;
					p[0] = mPalette[pal[v & 0xf]];
					p[1] = mPalette[pal[v >> 4]];
					p += 2;
				}
				if (w & 1) {
					uint32_t v = *data++;
					p[0] = mPalette[pal[v & 0xf]];
				}
				break;
			}
			case vp6_runs: {
				uint16_t *p = dest;
				uint16_t *pend = dest + w;
				uint8_t gap;

				while (0xff != (gap = *data++)) {
					p += gap;
					int len = *data++;
					for (int i = 3; i < len; i += 4) {
						uint32_t v = *data++;
						v |= (uint32_t)(*data++) << 8;
						v |= (uint32_t)(*data++) << 16;
						*p++ = mPalette[pal[v & 0x3f]];
						*p++ = mPalette[pal[(v >> 6) & 0x3f]];
						*p++ = mPalette[pal[(v >> 12) & 0x3f]];
						*p++ = mPalette[pal[(v >> 18) & 0x3f]];
					}
					len &= 3;
					if (len--) {
						uint32_t v = *data++;
						*p++ = mPalette[pal[v & 0x3f]];
						if (len--) {
							v >>= 6;
							v |= (uint32_t)(*data++) << 2;
							*p++ = mPalette[pal[v & 0x3f]];
							if (len--) {
								v >>= 6;
								v |= (uint32_t)(*data++) << 4;
								*p++ = mPalette[pal[v & 0x3f]];
							}
						}
					}
					if (p >= pend)
						break;
				}
				break;
			}
			case vp8_runs: {
				uint16_t *p = dest;
				uint16_t *pend = dest + w;
				uint8_t gap;

				while (0xff != (gap = *data++)) {
					p += gap;
					int len = *data++;
					for (int i = 0; i < len; i++)
						*p++ = mPalette[pal[*data++]];
					if (p >= pend)
						break;
				}
				break;
			}
			case vp_border: {
				dest[0] = mPalette[*data++];
				uint16_t col = mPalette[*data++];

				for (int i = 1; i < w - 1; i++)
					dest[i] = col;
				dest[w - 1] = mPalette[*data++];
				break;
			}
			default:
				break;
		}
	}
	else {
		uint32_t sp = vpatch_shared_palette(patch);
		uint16_t *pal16 = mSharedPal[sp];

		if (sp >= NUM_SHARED_PALETTES)
			return (uint32_t)(data - data0);
		switch (vpatch_type(patch)) {
			case vp4_solid: {
				uint16_t *p = dest;

				for (int i = 0; i < w / 2; i++) {
					uint32_t v = *data++;
					p[0] = pal16[v & 0xf];
					p[1] = pal16[v >> 4];
					p += 2;
				}
				if (w & 1) {
					uint32_t v = *data++;
					p[0] = pal16[v & 0xf];
				}
				break;
			}
			case vp4_alpha: {
				uint16_t *p = dest;

				for (int i = 0; i < w / 2; i++) {
					uint32_t v = *data++;
					if (v & 0xf)
						p[0] = pal16[v & 0xf];
					if (v >> 4)
						p[1] = pal16[v >> 4];
					p += 2;
				}
				if (w & 1) {
					uint32_t v = *data++;
					if (v & 0xf)
						p[0] = pal16[v & 0xf];
				}
				break;
			}
			default:
				break;
		}
	}
	if (repeat) {
		if (vp->entry.patch_handle == VPATCH_M_THERMM)
			w--;
		for (int i = 0; i < repeat * w; i++)
			dest[w + i] = dest[i];
	}
	return (uint32_t)(data - data0);
}

static uint32_t doomDc32DisplayIndex(const struct Canvas *cnv, uint32_t x, uint32_t y)
{
	uint32_t rowItems = cnv->rotated ? cnv->h : cnv->w;

	if (cnv->flipped) {
		x = cnv->w - 1u - x;
		y = cnv->h - 1u - y;
	}
	if (cnv->rotated)
		return x * rowItems + (cnv->h - 1u - y);
	return y * rowItems + x;
}

static void doomDc32WriteLogicalLine(uint16_t *fb, const struct Canvas *cnv,
	uint32_t y, const uint16_t *line, uint32_t width)
{
	if (cnv->rotated && !cnv->flipped) {
		for (uint32_t x = 0; x < width; x++)
			fb[x * cnv->h + (cnv->h - 1u - y)] = line[x];
		return;
	}
	for (uint32_t x = 0; x < width; x++)
		fb[doomDc32DisplayIndex(cnv, x, y)] = line[x];
}

static void doomDc32PaletteLine(uint16_t *line, const uint8_t *src)
{
	for (int x = 0; x < SCREENWIDTH; x++)
		line[x] = mPalette[src[x]];
}

static void doomDc32BlackLine(uint16_t *line)
{
	for (int x = 0; x < SCREENWIDTH; x++)
		line[x] = 0;
}

static void doomDc32WipeLine(uint16_t *line, int scanline)
{
	const uint8_t *src;

	if (scanline < MAIN_VIEWHEIGHT)
		src = frame_buffer[display_frame_index];
	else
		src = frame_buffer[display_frame_index ^ 1] - 32 * SCREENWIDTH;
	src += scanline * SCREENWIDTH;
	if (!wipe_yoffsets || !wipe_linelookup) {
		doomDc32PaletteLine(line, src);
		return;
	}
	for (int x = 0; x < SCREENWIDTH; x++) {
		int rel = scanline - wipe_yoffsets[x];

		if (rel < 0) {
			line[x] = mPalette[src[x]];
		} else {
			const uint8_t *flip = (const uint8_t*)(uintptr_t)wipe_linelookup[rel];

			if (flip >= &frame_buffer[0][0] && flip < &frame_buffer[0][0] + DOOM_DC32_FRAME_BYTES)
				line[x] = mPalette[flip[x]];
			else
				line[x] = mPalette[src[x]];
		}
	}
}

static void doomDc32BaseLine(uint16_t *line, int scanline)
{
	const uint8_t *src;

	switch (display_video_type) {
		case VIDEO_TYPE_DOUBLE:
			if (scanline < MAIN_VIEWHEIGHT) {
				doomDc32PaletteLine(line, frame_buffer[display_frame_index] + scanline * SCREENWIDTH);
			} else {
				doomDc32BlackLine(line);
			}
			break;
		case VIDEO_TYPE_WIPE:
			doomDc32WipeLine(line, scanline);
			break;
		case VIDEO_TYPE_SINGLE:
		case VIDEO_TYPE_SAVING:
			if (scanline < MAIN_VIEWHEIGHT)
				src = frame_buffer[display_frame_index] + scanline * SCREENWIDTH;
			else
				src = frame_buffer[display_frame_index ^ 1] + (scanline - 32) * SCREENWIDTH;
			doomDc32PaletteLine(line, src);
			break;
		default:
			doomDc32BlackLine(line);
			break;
	}
}

static void doomDc32AdvanceWipe(void)
{
	bool regular = display_overlay_index != 0;
	int newWipeMin = SCREENHEIGHT;

	if (display_video_type != VIDEO_TYPE_WIPE || !wipe_yoffsets || !wipe_yoffsets_raw || wipe_min > SCREENHEIGHT)
		return;
	for (int x = 0; x < SCREENWIDTH; x++) {
		int v;

		if (wipe_yoffsets_raw[x] < 0) {
			if (regular)
				wipe_yoffsets_raw[x]++;
			v = 0;
		} else {
			int dy = wipe_yoffsets_raw[x] < 16 ? (1 + wipe_yoffsets_raw[x] + regular) / 2 : 4;

			if (wipe_yoffsets_raw[x] + dy > SCREENHEIGHT) {
				v = SCREENHEIGHT;
			} else {
				wipe_yoffsets_raw[x] += dy;
				v = wipe_yoffsets_raw[x];
			}
		}
		wipe_yoffsets[x] = (uint8_t)v;
		if (v < newWipeMin)
			newWipeMin = v;
	}
	wipe_min = (uint8_t)newWipeMin;
}

static void doomDc32BuildOverlayRows(void)
{
	if (display_video_type < FIRST_VIDEO_TYPE_WITH_OVERLAYS)
		return;
	memset(vpatchlists->vpatch_next, 0, sizeof(vpatchlists->vpatch_next));
	memset(vpatchlists->vpatch_starters, 0, sizeof(vpatchlists->vpatch_starters));
	memset(vpatchlists->vpatch_doff, 0, sizeof(vpatchlists->vpatch_doff));
	vpatchlist_t *overlays = vpatchlists->overlays[display_overlay_index];

	for (int i = overlays->header.size - 1; i > 0; i--) {
		if (overlays[i].entry.y < count_of(vpatchlists->vpatch_starters)) {
			vpatchlists->vpatch_next[i] = vpatchlists->vpatch_starters[overlays[i].entry.y];
			vpatchlists->vpatch_starters[overlays[i].entry.y] = (uint8_t)i;
		}
	}
}

static void doomDc32DrawOverlayLine(uint16_t *line, int y)
{
	vpatchlist_t *overlays;
	int prev;

	if (display_video_type < FIRST_VIDEO_TYPE_WITH_OVERLAYS)
		return;
	prev = 0;
	for (int vp = vpatchlists->vpatch_starters[y]; vp;) {
		int next = vpatchlists->vpatch_next[vp];

		while (vpatchlists->vpatch_next[prev] && vpatchlists->vpatch_next[prev] < vp)
			prev = vpatchlists->vpatch_next[prev];
		if (prev != vp && vpatchlists->vpatch_next[prev] != vp) {
			vpatchlists->vpatch_next[vp] = vpatchlists->vpatch_next[prev];
			vpatchlists->vpatch_next[prev] = (uint8_t)vp;
			prev = vp;
		}
		vp = next;
	}
	overlays = vpatchlists->overlays[display_overlay_index];
	prev = 0;
	for (int vp = vpatchlists->vpatch_next[prev]; vp; vp = vpatchlists->vpatch_next[prev]) {
		vpatchlist_t *entry = &overlays[vp];
		patch_t *patch;
		int yoff;

		if (entry->entry.patch_handle == VPATCH_NAME_INVALID) {
			vpatchlists->vpatch_next[prev] = vpatchlists->vpatch_next[vp];
			continue;
		}
		patch = resolve_vpatch_handle(entry->entry.patch_handle);
		yoff = y - entry->entry.y;
		if (patch && yoff < vpatch_height(patch)) {
			vpatchlists->vpatch_doff[vp] = (uint16_t)doomDc32DrawVpatch(line, patch,
				entry, vpatchlists->vpatch_doff[vp]);
			prev = vp;
		} else {
			vpatchlists->vpatch_next[prev] = vpatchlists->vpatch_next[vp];
		}
	}
}

void doomDc32PresentFrame(void)
{
	struct Canvas *cnv = doomDc32CanvasValid ? &doomDc32Canvas : NULL;
	uint16_t *fb = cnv ? (uint16_t*)cnv->framebuffer : NULL;
	uint16_t line[SCREENWIDTH];
	int previousSourceY = -1;

	if (!fb && doomDc32Host && doomDc32Host->displayFb)
		fb = doomDc32Host->displayFb();
	if (!fb)
		return;
	if (!cnv || cnv->bpp != 16u || cnv->w < SCREENWIDTH || cnv->h < SCREENHEIGHT)
		return;
	if (!sem_available(&render_frame_ready))
		return;
	sem_acquire_blocking(&render_frame_ready);
	display_video_type = next_video_type;
	display_frame_index = next_frame_index;
	display_overlay_index = next_overlay_index;
#if !DEMO1_ONLY
	video_scroll = next_video_scroll;
#endif
	sem_release(&display_frame_freed);
	doomDc32UpdatePalette();
	doomDc32BuildOverlayRows();
	doomDc32AdvanceWipe();
	// The original 320x200 framebuffer assumes 1.2:1 pixels.  Expand it to
	// the badge's 320x240 4:3 canvas so its intended geometry is preserved.
	for (uint32_t y = 0; y < cnv->h; y++) {
		int sourceY = (int)((y * SCREENHEIGHT) / cnv->h);

		/*
		 * A 200-to-240 scale duplicates source rows.  Overlays are decoded as
		 * a stateful scanline stream, so redraw each source line exactly once
		 * and copy the completed line for its duplicate display rows.
		 */
		if (sourceY != previousSourceY) {
			doomDc32BaseLine(line, sourceY);
			doomDc32DrawOverlayLine(line, sourceY);
			previousSourceY = sourceY;
		}
		doomDc32WriteLogicalLine(fb, cnv, y, line, SCREENWIDTH);
	}
}

void I_InitGraphics(void)
{
	doomDc32InitScratch();
	mStatusBar = resolve_vpatch_handle(VPATCH_STBAR);
	(void)mStatusBar;
	sem_init(&render_frame_ready, 0, 2);
	sem_init(&display_frame_freed, 1, 2);
	pd_init();
	I_VideoBuffer = frame_buffer[0];
	mVideoInitialized = true;
}

void I_ShutdownGraphics(void)
{
}

void I_CheckIsScreensaver(void)
{
}

void I_SetGrabMouseCallback(grabmouse_callback_t func)
{
	(void)func;
}

void I_DisplayFPSDots(boolean dots_on)
{
	(void)dots_on;
}

void I_BindVideoVariables(void)
{
}

void I_GraphicsCheckCommandLine(void)
{
}

void I_InitWindowTitle(void)
{
}

void I_InitWindowIcon(void)
{
}

void I_SetBrightness(uint8_t brightness)
{
	(void)brightness;
}

void I_GetWindowPosition(int *x, int *y, int w, int h)
{
	(void)w;
	(void)h;
	if (x)
		*x = 0;
	if (y)
		*y = 0;
}

void I_SetWindowTitle(const char *title)
{
	(void)title;
}

void I_SetPaletteNum(int num)
{
	mNextPalette = (int8_t)num;
}

int I_GetPaletteIndex(int r, int g, int b)
{
	(void)r;
	(void)g;
	(void)b;
	return 0;
}

void I_UpdateNoBlit(void)
{
}

void I_FinishUpdate(void)
{
}

void I_ReadScreen(pixel_t *scr)
{
	if (scr && I_VideoBuffer)
		memcpy(scr, I_VideoBuffer, SCREENWIDTH * SCREENHEIGHT);
}

void I_BeginRead(void)
{
}

void I_StartFrame(void)
{
	if (doomDc32Host && doomDc32Host->delayMsec)
		doomDc32Host->delayMsec(1);
}

void I_StartTic(void)
{
	if (mVideoInitialized)
		I_GetEvent();
}

void I_EnableLoadingDisk(int xoffs, int yoffs)
{
	(void)xoffs;
	(void)yoffs;
}
