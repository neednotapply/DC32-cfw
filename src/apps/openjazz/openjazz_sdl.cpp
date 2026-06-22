#include "SDL.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern "C" {
#include "dcApp.h"
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "gb.h"
}
#include "apps/port/port_runtime.h"
#include "apps/openjazz/openjazz_install.h"

static const struct DcAppHostApi *gOjHost;
static struct Canvas gOjCanvas;
static bool gOjQuitRequested;
static uint64_t gOjStartTicks;
static SDL_Surface *gOjScreen;
static SDL_Surface gOjScreenStorage;
static SDL_PixelFormat gOjScreenFormat;
static SDL_Palette gOjScreenPalette;
static SDL_Palette gOjSharedPalette;
static bool gOjSharedPaletteReady;
struct OjReadOnlyFormat {
	SDL_PixelFormat format;
	Uint8 colorKey;
	Uint8 colorKeyEnabled;
	Uint8 used;
};
static OjReadOnlyFormat gOjReadOnlyFormats[8];
static uint_fast16_t gOjPrevKeys;
static const char *gOjError = "ok";

struct OjQueuedKey {
	Uint32 type;
	SDLKey key;
};

static struct OjQueuedKey gOjQueue[16];
static uint8_t gOjQueueRead;
static uint8_t gOjQueueWrite;

static uint32_t ojDisplayIndex(const struct Canvas *cnv, uint32_t x, uint32_t y)
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

static uint16_t ojRgb565(const SDL_Color *color)
{
	return dcAppDrawRgb565(color->r, color->g, color->b);
}

static void ojInitPalette(SDL_Palette *palette)
{
	if (!palette)
		return;
	palette->ncolors = 256;
	for (uint32_t i = 0; i < 256u; i++) {
		palette->colors[i].r = (Uint8)i;
		palette->colors[i].g = (Uint8)i;
		palette->colors[i].b = (Uint8)i;
		palette->colors[i].a = 0xff;
	}
}

static SDL_PixelFormat *ojReadOnlyFormat(Uint8 colorKey, bool colorKeyEnabled)
{
	OjReadOnlyFormat *freeSlot = NULL;

	if (!gOjSharedPaletteReady) {
		ojInitPalette(&gOjSharedPalette);
		gOjSharedPaletteReady = true;
	}
	for (uint32_t i = 0; i < sizeof(gOjReadOnlyFormats) / sizeof(gOjReadOnlyFormats[0]); i++) {
		OjReadOnlyFormat *slot = gOjReadOnlyFormats + i;

		if (slot->used && slot->colorKey == colorKey &&
				slot->colorKeyEnabled == (Uint8)colorKeyEnabled)
			return &slot->format;
		if (!slot->used && !freeSlot)
			freeSlot = slot;
	}
	if (!freeSlot)
		return NULL;
	memset(freeSlot, 0, sizeof(*freeSlot));
	freeSlot->format.palette = &gOjSharedPalette;
	freeSlot->format.BitsPerPixel = 8;
	freeSlot->format.BytesPerPixel = 1;
	freeSlot->format.colorkey = colorKey;
	freeSlot->colorKey = colorKey;
	freeSlot->colorKeyEnabled = colorKeyEnabled ? 1u : 0u;
	freeSlot->used = 1u;
	return &freeSlot->format;
}

SDL_Surface *dc32OjSdlWrapReadOnlySurface(const void *pixels, int width, int height,
	int pitch, Uint8 colorKey, bool colorKeyEnabled)
{
	SDL_Surface *surface;

	if (!pixels || width <= 0 || height <= 0 || pitch < width)
		return NULL;
	surface = (SDL_Surface*)dc32PortCalloc(1, sizeof(*surface));
	if (!surface || !dc32OjSdlInitReadOnlySurface(surface, pixels, width, height,
			pitch, colorKey, colorKeyEnabled)) {
		dc32PortFree(surface);
		return NULL;
	}
	return surface;
}

SDL_Surface *dc32OjSdlWrapWritableSurface(void *pixels, int width, int height,
	int pitch)
{
	SDL_Surface *surface;
	SDL_PixelFormat *format;

	if (!pixels || width <= 0 || height <= 0 || pitch < width)
		return NULL;
	if (!gOjSharedPaletteReady) {
		ojInitPalette(&gOjSharedPalette);
		gOjSharedPaletteReady = true;
	}
	surface = (SDL_Surface*)dc32PortCalloc(1, sizeof(*surface));
	format = (SDL_PixelFormat*)dc32PortCalloc(1, sizeof(*format));
	if (!surface || !format) {
		dc32PortFree(format);
		dc32PortFree(surface);
		return NULL;
	}
	format->palette = &gOjSharedPalette;
	format->BitsPerPixel = 8;
	format->BytesPerPixel = 1;
	surface->format = format;
	surface->w = width;
	surface->h = height;
	surface->pitch = (Uint16)pitch;
	surface->pixels = pixels;
	surface->clip_rect.w = (Uint16)width;
	surface->clip_rect.h = (Uint16)height;
	surface->ownsFormat = 1u;
	return surface;
}

bool dc32OjSdlInitReadOnlySurface(SDL_Surface *surface, const void *pixels,
	int width, int height, int pitch, Uint8 colorKey, bool colorKeyEnabled)
{
	SDL_PixelFormat *format;

	if (!surface || !pixels || width <= 0 || height <= 0 || pitch < width)
		return false;
	format = ojReadOnlyFormat(colorKey, colorKeyEnabled);
	if (!format)
		return false;
	memset(surface, 0, sizeof(*surface));
	surface->format = format;
	surface->w = width;
	surface->h = height;
	surface->pitch = (Uint16)pitch;
	surface->pixels = const_cast<void*>(pixels);
	surface->clip_rect.w = (Uint16)width;
	surface->clip_rect.h = (Uint16)height;
	surface->ownsFormat = 0u;
	surface->colorKeyEnabled = colorKeyEnabled ? 1u : 0u;
	surface->readOnlyPixels = 1u;
	return true;
}

static SDL_Surface *ojAllocSurface(int width, int height, bool privatePalette)
{
	SDL_Surface *surface;
	SDL_PixelFormat *format;
	SDL_Palette *palette = NULL;
	uint32_t pixelsSize;

	if (width <= 0 || height <= 0)
		return NULL;
	pixelsSize = (uint32_t)width * (uint32_t)height;
	surface = (SDL_Surface*)dc32PortCalloc(1, sizeof(*surface));
	format = (SDL_PixelFormat*)dc32PortCalloc(1, sizeof(*format));
	if (!surface || !format)
		goto fail;
	if (privatePalette) {
		palette = (SDL_Palette*)dc32PortCalloc(1, sizeof(*palette));
		if (!palette)
			goto fail;
		ojInitPalette(palette);
		surface->ownsPalette = 1;
	} else {
		if (!gOjSharedPaletteReady) {
			ojInitPalette(&gOjSharedPalette);
			gOjSharedPaletteReady = true;
		}
		palette = &gOjSharedPalette;
	}
	surface->pixels = dc32PortCalloc(1, pixelsSize);
	if (!surface->pixels)
		goto fail;
	format->palette = palette;
	format->BitsPerPixel = 8;
	format->BytesPerPixel = 1;
	surface->format = format;
	surface->w = width;
	surface->h = height;
	surface->pitch = (Uint16)width;
	surface->clip_rect.x = 0;
	surface->clip_rect.y = 0;
	surface->clip_rect.w = (Uint16)width;
	surface->clip_rect.h = (Uint16)height;
	surface->ownsPixels = 1;
	surface->ownsFormat = 1;
	return surface;

fail:
	if (palette && surface && surface->ownsPalette)
		dc32PortFree(palette);
	if (format)
		dc32PortFree(format);
	if (surface) {
		dc32PortFree(surface->pixels);
		dc32PortFree(surface);
	}
	gOjError = "out of memory";
	return NULL;
}

static SDLKey ojKeyForBit(uint_fast16_t bit)
{
	switch (bit) {
		case KEY_BIT_UP:
			return SDLK_UP;
		case KEY_BIT_DOWN:
			return SDLK_DOWN;
		case KEY_BIT_LEFT:
			return SDLK_LEFT;
		case KEY_BIT_RIGHT:
			return SDLK_RIGHT;
		case KEY_BIT_A:
			return SDLK_SPACE;
		case KEY_BIT_B:
			return SDLK_LALT;
		case KEY_BIT_START:
			return SDLK_p;
		case KEY_BIT_SEL:
			return SDLK_RCTRL;
		case UI_KEY_BIT_CENTER:
			return SDLK_ESCAPE;
		default:
			return SDLK_UNKNOWN;
	}
}

static void ojQueueKey(Uint32 type, SDLKey key)
{
	uint8_t next;

	if (key == SDLK_UNKNOWN)
		return;
	next = (uint8_t)((gOjQueueWrite + 1u) % (sizeof(gOjQueue) / sizeof(gOjQueue[0])));
	if (next == gOjQueueRead)
		return;
	gOjQueue[gOjQueueWrite].type = type;
	gOjQueue[gOjQueueWrite].key = key;
	gOjQueueWrite = next;
}

static bool ojPopKey(SDL_Event *event)
{
	struct OjQueuedKey queued;

	if (gOjQueueRead == gOjQueueWrite)
		return false;
	queued = gOjQueue[gOjQueueRead];
	gOjQueueRead = (uint8_t)((gOjQueueRead + 1u) % (sizeof(gOjQueue) / sizeof(gOjQueue[0])));
	memset(event, 0, sizeof(*event));
	event->type = queued.type;
	event->key.type = queued.type;
	event->key.keysym.sym = queued.key;
	return true;
}

static void ojQueueChangedKey(uint_fast16_t bit, uint_fast16_t keys)
{
	ojQueueKey((keys & bit) ? SDL_KEYDOWN : SDL_KEYUP, ojKeyForBit(bit));
}

static void ojProcessKeys(uint_fast16_t keys)
{
	static const uint_fast16_t directions[] = {
		KEY_BIT_UP, KEY_BIT_DOWN, KEY_BIT_LEFT, KEY_BIT_RIGHT,
	};
	static const uint_fast16_t actions[] = {
		KEY_BIT_A, KEY_BIT_B, KEY_BIT_START, KEY_BIT_SEL, UI_KEY_BIT_CENTER,
	};
	uint_fast16_t changed = keys ^ gOjPrevKeys;

	for (uint32_t i = 0; i < sizeof(directions) / sizeof(directions[0]); i++)
		if (changed & directions[i])
			ojQueueChangedKey(directions[i], keys);
	for (uint32_t i = 0; i < sizeof(actions) / sizeof(actions[0]); i++)
		if (changed & actions[i])
			ojQueueChangedKey(actions[i], keys);
	gOjPrevKeys = keys;
}

void dc32OjSdlSetHost(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	memset(&gOjCanvas, 0, sizeof(gOjCanvas));
	gOjHost = host;
	if (args && args->canvas)
		gOjCanvas = *args->canvas;
	if (!gOjCanvas.framebuffer && host && host->displayFb)
		gOjCanvas.framebuffer = host->displayFb();
	if (!gOjCanvas.w)
		gOjCanvas.w = 320u;
	if (!gOjCanvas.h)
		gOjCanvas.h = 240u;
	if (!gOjCanvas.bpp)
		gOjCanvas.bpp = 16u;
	if (!args || !args->canvas)
		gOjCanvas.rotated = 1u;
	if (args && args->rotate)
		gOjCanvas.flipped = 1u;
	gOjQuitRequested = false;
	gOjStartTicks = host && host->getTime ? host->getTime() : 0;
	gOjPrevKeys = host && host->uiKeysRaw ? host->uiKeysRaw() : 0;
	gOjQueueRead = 0;
	gOjQueueWrite = 0;
}

void dc32OjSdlRequestQuit(void)
{
	gOjQuitRequested = true;
}

int SDL_Init(Uint32 flags)
{
	(void)flags;
	gOjStartTicks = gOjHost && gOjHost->getTime ? gOjHost->getTime() : 0;
	return 0;
}

void SDL_Quit(void)
{
}

Uint32 SDL_GetTicks(void)
{
	uint64_t now = gOjHost && gOjHost->getTime ? gOjHost->getTime() : 0;

	if (now < gOjStartTicks)
		return 0;
	return (Uint32)((now - gOjStartTicks) / (TICKS_PER_SECOND / 1000u));
}

void SDL_Delay(Uint32 ms)
{
	if (gOjHost && gOjHost->delayMsec)
		gOjHost->delayMsec(ms);
}

int SDL_PollEvent(SDL_Event *event)
{
	uint_fast16_t keys;
	if (!event)
		return 0;
	if (gOjQuitRequested) {
		memset(event, 0, sizeof(*event));
		event->type = SDL_QUIT;
		gOjQuitRequested = false;
		return 1;
	}
	if (ojPopKey(event))
		return 1;
	keys = gOjHost && gOjHost->uiKeysRaw ? gOjHost->uiKeysRaw() : 0;
	ojProcessKeys(keys);
	return ojPopKey(event) ? 1 : 0;
}

const char *SDL_GetError(void)
{
	return gOjError;
}

char *SDL_GetKeyName(SDLKey key)
{
	const char *name;

	switch (key) {
		case SDLK_UP:
			name = "Up";
			break;
		case SDLK_DOWN:
			name = "Down";
			break;
		case SDLK_LEFT:
			name = "Left";
			break;
		case SDLK_RIGHT:
			name = "Right";
			break;
		case SDLK_SPACE:
			name = "A";
			break;
		case SDLK_LALT:
			name = "B";
			break;
		case SDLK_RETURN:
			name = "Enter";
			break;
		case SDLK_ESCAPE:
			name = "FN";
			break;
		case SDLK_RCTRL:
			name = "Select";
			break;
		case SDLK_F9:
			name = "Stats";
			break;
		case SDLK_p:
			name = "Start";
			break;
		default:
			name = "?";
			break;
	}
	return const_cast<char*>(name);
}

const SDL_version *SDL_Linked_Version(void)
{
	static const SDL_version version = {1, 2, 15};

	return &version;
}

SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags)
{
	(void)bpp;
	(void)flags;
	if (width != (int)DC32_PORT_SCREEN_W || height != (int)DC32_PORT_SCREEN_H) {
		gOjError = "unsupported fixed canvas size";
		return NULL;
	}
	memset(&gOjScreenStorage, 0, sizeof(gOjScreenStorage));
	memset(&gOjScreenFormat, 0, sizeof(gOjScreenFormat));
	ojInitPalette(&gOjScreenPalette);
	gOjScreenFormat.palette = &gOjScreenPalette;
	gOjScreenFormat.BitsPerPixel = 8;
	gOjScreenFormat.BytesPerPixel = 1;
	gOjScreenStorage.format = &gOjScreenFormat;
	gOjScreenStorage.w = width;
	gOjScreenStorage.h = height;
	gOjScreenStorage.pitch = (Uint16)width;
	gOjScreenStorage.pixels = DC32_PORT_OPENJAZZ_CANVAS_START;
	gOjScreenStorage.clip_rect.w = (Uint16)width;
	gOjScreenStorage.clip_rect.h = (Uint16)height;
	memset(gOjScreenStorage.pixels, 0, DC32_PORT_OPENJAZZ_CANVAS_SIZE);
	gOjScreen = &gOjScreenStorage;
	return gOjScreen;
}

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth,
	Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask)
{
	(void)flags;
	(void)depth;
	(void)rmask;
	(void)gmask;
	(void)bmask;
	(void)amask;
	return ojAllocSurface(width, height, false);
}

void SDL_FreeSurface(SDL_Surface *surface)
{
	if (!surface)
		return;
	if (surface == &gOjScreenStorage) {
		gOjScreen = NULL;
		return;
	}
	if (surface == gOjScreen)
		gOjScreen = NULL;
	if (surface->format) {
		if (surface->ownsPalette)
			dc32PortFree(surface->format->palette);
		if (surface->ownsFormat)
			dc32PortFree(surface->format);
	}
	if (surface->ownsPixels)
		dc32PortFree(surface->pixels);
	dc32PortFree(surface);
}

int SDL_Flip(SDL_Surface *screen)
{
	uint16_t *dst;
	SDL_Palette *palette;

	if (!screen || !screen->pixels || !screen->format || !screen->format->palette)
		return -1;
	dc32OjLoadingPause();
	if (gOjHost && gOjHost->ledsTick)
		gOjHost->ledsTick();
	dispPrvWaitForScanoutStart();
	dst = (uint16_t*)gOjCanvas.framebuffer;
	palette = screen->format->palette;
	if (!dst || gOjCanvas.bpp != 16u)
		return -1;
	for (int y = 0; y < screen->h && y < (int)gOjCanvas.h; y++) {
		const uint8_t *src = (const uint8_t*)screen->pixels + (uint32_t)y * screen->pitch;

		for (int x = 0; x < screen->w && x < (int)gOjCanvas.w; x++)
			dst[ojDisplayIndex(&gOjCanvas, (uint32_t)x, (uint32_t)y)] = ojRgb565(&palette->colors[src[x]]);
	}
	return 0;
}

int SDL_SetPalette(SDL_Surface *surface, int flags, SDL_Color *colors, int first, int ncolors)
{
	(void)flags;
	if (!surface || !surface->format || !surface->format->palette || !colors)
		return 0;
	if (first < 0 || ncolors < 0 || first + ncolors > 256)
		return 0;
	memcpy(surface->format->palette->colors + first, colors, (size_t)ncolors * sizeof(colors[0]));
	return 1;
}

void SDL_WM_SetCaption(const char *title, const char *icon)
{
	(void)title;
	(void)icon;
}

SDL_Rect **SDL_ListModes(SDL_PixelFormat *format, Uint32 flags)
{
	static SDL_Rect mode = {0, 0, 320, 240};
	static SDL_Rect *modes[] = {&mode, NULL};

	(void)format;
	(void)flags;
	return modes;
}

int SDL_SetClipRect(SDL_Surface *surface, const SDL_Rect *rect)
{
	if (!surface)
		return 0;
	if (rect)
		surface->clip_rect = *rect;
	else {
		surface->clip_rect.x = 0;
		surface->clip_rect.y = 0;
		surface->clip_rect.w = (Uint16)surface->w;
		surface->clip_rect.h = (Uint16)surface->h;
	}
	return 1;
}

int SDL_ShowCursor(int toggle)
{
	(void)toggle;
	return 0;
}

Uint32 SDL_MapRGB(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b)
{
	(void)format;
	(void)r;
	(void)g;
	(void)b;
	return 0;
}

int SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color)
{
	int x0;
	int y0;
	int w;
	int h;

	if (!dst || !dst->pixels || dst->readOnlyPixels)
		return -1;
	x0 = dstrect ? dstrect->x : 0;
	y0 = dstrect ? dstrect->y : 0;
	w = dstrect ? dstrect->w : dst->w;
	h = dstrect ? dstrect->h : dst->h;
	if (x0 < 0) {
		w += x0;
		x0 = 0;
	}
	if (y0 < 0) {
		h += y0;
		y0 = 0;
	}
	if (x0 + w > dst->w)
		w = dst->w - x0;
	if (y0 + h > dst->h)
		h = dst->h - y0;
	if (w <= 0 || h <= 0)
		return 0;
	for (int y = 0; y < h; y++)
		memset((uint8_t*)dst->pixels + (uint32_t)(y0 + y) * dst->pitch + x0, (uint8_t)color, (size_t)w);
	return 0;
}

int SDL_SetColorKey(SDL_Surface *surface, Uint32 flag, Uint32 key)
{
	if (!surface || !surface->format)
		return -1;
	surface->colorKeyEnabled = (flag & SDL_SRCCOLORKEY) ? 1 : 0;
	surface->format->colorkey = key;
	return 0;
}

int SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
	int sx;
	int sy;
	int dx;
	int dy;
	int w;
	int h;
	int yStart;
	int yEnd;
	int yStep;

	if (!src || !dst || !src->pixels || !dst->pixels ||
			dst->readOnlyPixels)
		return -1;
	sx = srcrect ? srcrect->x : 0;
	sy = srcrect ? srcrect->y : 0;
	w = srcrect ? srcrect->w : src->w;
	h = srcrect ? srcrect->h : src->h;
	dx = dstrect ? dstrect->x : 0;
	dy = dstrect ? dstrect->y : 0;
	if (sx < 0) {
		dx -= sx;
		w += sx;
		sx = 0;
	}
	if (sy < 0) {
		dy -= sy;
		h += sy;
		sy = 0;
	}
	if (dx < 0) {
		sx -= dx;
		w += dx;
		dx = 0;
	}
	if (dy < 0) {
		sy -= dy;
		h += dy;
		dy = 0;
	}
	if (sx + w > src->w)
		w = src->w - sx;
	if (sy + h > src->h)
		h = src->h - sy;
	if (dx + w > dst->w)
		w = dst->w - dx;
	if (dy + h > dst->h)
		h = dst->h - dy;
	if (w <= 0 || h <= 0)
		return 0;
	yStart = 0;
	yEnd = h;
	yStep = 1;
	if (src == dst && dy > sy) {
		yStart = h - 1;
		yEnd = -1;
		yStep = -1;
	}
	for (int yy = yStart; yy != yEnd; yy += yStep) {
		uint8_t *srcRow = (uint8_t*)src->pixels + (uint32_t)(sy + yy) * src->pitch + sx;
		uint8_t *dstRow = (uint8_t*)dst->pixels + (uint32_t)(dy + yy) * dst->pitch + dx;

		if (!src->colorKeyEnabled) {
			memmove(dstRow, srcRow, (size_t)w);
		} else if (src == dst && dstRow > srcRow) {
			for (int x = w - 1; x >= 0; x--) {
				if (srcRow[x] != (uint8_t)src->format->colorkey)
					dstRow[x] = srcRow[x];
			}
		} else {
			for (int x = 0; x < w; x++) {
				if (srcRow[x] != (uint8_t)src->format->colorkey)
					dstRow[x] = srcRow[x];
			}
		}
	}
	return 0;
}

int SDL_LockSurface(SDL_Surface *surface)
{
	(void)surface;
	return 0;
}

void SDL_UnlockSurface(SDL_Surface *surface)
{
	(void)surface;
}

int SDL_SaveBMP(SDL_Surface *surface, const char *file)
{
	(void)surface;
	(void)file;
	return -1;
}

int SDL_NumJoysticks(void)
{
	return 0;
}

void *SDL_JoystickOpen(int index)
{
	(void)index;
	return NULL;
}

void SDL_JoystickClose(int index)
{
	(void)index;
}
