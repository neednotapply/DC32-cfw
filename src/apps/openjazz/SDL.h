#ifndef DC32_OPENJAZZ_SDL_H
#define DC32_OPENJAZZ_SDL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t Uint8;
typedef int8_t Sint8;
typedef uint16_t Uint16;
typedef int16_t Sint16;
typedef uint32_t Uint32;
typedef int32_t Sint32;
typedef int SDLKey;

typedef struct SDL_Color {
	Uint8 r;
	Uint8 g;
	Uint8 b;
	Uint8 a;
} SDL_Color;

typedef struct SDL_Palette {
	int ncolors;
	SDL_Color colors[256];
} SDL_Palette;

typedef struct SDL_PixelFormat {
	SDL_Palette *palette;
	Uint8 BitsPerPixel;
	Uint8 BytesPerPixel;
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	Uint32 Amask;
	Uint32 colorkey;
} SDL_PixelFormat;

typedef struct SDL_Rect {
	Sint16 x;
	Sint16 y;
	Uint16 w;
	Uint16 h;
} SDL_Rect;

typedef struct SDL_Surface {
	Uint32 flags;
	SDL_PixelFormat *format;
	int w;
	int h;
	Uint16 pitch;
	void *pixels;
	SDL_Rect clip_rect;
	Uint8 ownsPixels;
	Uint8 ownsPalette;
	Uint8 ownsFormat;
	Uint8 colorKeyEnabled;
	Uint8 readOnlyPixels;
} SDL_Surface;

typedef struct SDL_keysym {
	SDLKey sym;
	Uint16 mod;
} SDL_keysym;

typedef struct SDL_KeyboardEvent {
	Uint32 type;
	SDL_keysym keysym;
} SDL_KeyboardEvent;

typedef struct SDL_JoyButtonEvent {
	Uint32 type;
	Uint8 button;
} SDL_JoyButtonEvent;

typedef struct SDL_JoyAxisEvent {
	Uint32 type;
	Uint8 axis;
	Sint16 value;
} SDL_JoyAxisEvent;

typedef struct SDL_JoyHatEvent {
	Uint32 type;
	Uint8 hat;
	Uint8 value;
} SDL_JoyHatEvent;

typedef struct SDL_MouseMotionEvent {
	Uint32 type;
	Uint16 state;
	Sint16 x;
	Sint16 y;
} SDL_MouseMotionEvent;

typedef struct SDL_MouseButtonEvent {
	Uint32 type;
	Uint8 button;
	Sint16 x;
	Sint16 y;
} SDL_MouseButtonEvent;

typedef struct SDL_ResizeEvent {
	Uint32 type;
	int w;
	int h;
} SDL_ResizeEvent;

typedef union SDL_Event {
	Uint32 type;
	SDL_KeyboardEvent key;
	SDL_JoyButtonEvent jbutton;
	SDL_JoyAxisEvent jaxis;
	SDL_JoyHatEvent jhat;
	SDL_MouseMotionEvent motion;
	SDL_MouseButtonEvent button;
	SDL_ResizeEvent resize;
} SDL_Event;

typedef struct SDL_version {
	Uint8 major;
	Uint8 minor;
	Uint8 patch;
} SDL_version;

#define SDL_INIT_TIMER      0x00000001u
#define SDL_INIT_AUDIO      0x00000010u
#define SDL_INIT_VIDEO      0x00000020u
#define SDL_INIT_JOYSTICK   0x00000200u

#define SDL_SWSURFACE       0x00000000u
#define SDL_HWSURFACE       0x00000001u
#define SDL_DOUBLEBUF       0x40000000u
#define SDL_HWPALETTE       0x20000000u
#define SDL_RESIZABLE       0x00000010u
#define SDL_FULLSCREEN      0x80000000u
#define SDL_SRCCOLORKEY     0x00001000u

#define SDL_DISABLE 0
#define SDL_ENABLE 1

#define SDL_QUIT            0x100u
#define SDL_KEYDOWN         0x300u
#define SDL_KEYUP           0x301u
#define SDL_MOUSEMOTION     0x400u
#define SDL_MOUSEBUTTONDOWN 0x401u
#define SDL_MOUSEBUTTONUP   0x402u
#define SDL_JOYAXISMOTION   0x600u
#define SDL_JOYHATMOTION    0x602u
#define SDL_JOYBUTTONDOWN   0x603u
#define SDL_JOYBUTTONUP     0x604u
#define SDL_VIDEORESIZE     0x700u
#define SDL_VIDEOEXPOSE     0x701u

#define SDL_HAT_UP          0x01u
#define SDL_HAT_RIGHT       0x02u
#define SDL_HAT_DOWN        0x04u
#define SDL_HAT_LEFT        0x08u

#define SDL_BUTTON_LEFT     1u
#define SDL_BUTTON(X)       (1u << ((X) - 1u))

#define KMOD_ALT            0x0300u

#define SDLK_UNKNOWN        0
#define SDLK_BACKSPACE      8
#define SDLK_RETURN         13
#define SDLK_ESCAPE         27
#define SDLK_SPACE          32
#define SDLK_DELETE         127
#define SDLK_UP             273
#define SDLK_DOWN           274
#define SDLK_RIGHT          275
#define SDLK_LEFT           276
#define SDLK_LCTRL          306
#define SDLK_RCTRL          305
#define SDLK_LALT           308
#define SDLK_RALT           307
#define SDLK_F9             290
#define SDLK_1              '1'
#define SDLK_2              '2'
#define SDLK_3              '3'
#define SDLK_4              '4'
#define SDLK_5              '5'
#define SDLK_p              'p'
#define SDLK_y              'y'
#define SDLK_n              'n'

#define SDL_PHYSPAL         0x01
#define SDL_LOGPAL          0x02

#define SDL_MUSTLOCK(surface) (0)
#define SDL_VERSION(version_ptr) do { \
	(version_ptr)->major = 1; \
	(version_ptr)->minor = 2; \
	(version_ptr)->patch = 15; \
} while (0)

struct DcAppHostApi;
struct DcAppRunArgs;

void dc32OjSdlSetHost(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void dc32OjSdlRequestQuit(void);
SDL_Surface *dc32OjSdlWrapReadOnlySurface(const void *pixels, int width, int height,
	int pitch, Uint8 colorKey, bool colorKeyEnabled);
SDL_Surface *dc32OjSdlWrapWritableSurface(void *pixels, int width, int height,
	int pitch);
bool dc32OjSdlInitReadOnlySurface(SDL_Surface *surface, const void *pixels,
	int width, int height, int pitch, Uint8 colorKey, bool colorKeyEnabled);

int SDL_Init(Uint32 flags);
void SDL_Quit(void);
Uint32 SDL_GetTicks(void);
void SDL_Delay(Uint32 ms);
int SDL_PollEvent(SDL_Event *event);
const char *SDL_GetError(void);
char *SDL_GetKeyName(SDLKey key);
const SDL_version *SDL_Linked_Version(void);

SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags);
SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth,
	Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask);
void SDL_FreeSurface(SDL_Surface *surface);
int SDL_Flip(SDL_Surface *screen);
int SDL_SetPalette(SDL_Surface *surface, int flags, SDL_Color *colors, int first, int ncolors);
void SDL_WM_SetCaption(const char *title, const char *icon);
SDL_Rect **SDL_ListModes(SDL_PixelFormat *format, Uint32 flags);
int SDL_SetClipRect(SDL_Surface *surface, const SDL_Rect *rect);
int SDL_ShowCursor(int toggle);
Uint32 SDL_MapRGB(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b);
int SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color);
int SDL_SetColorKey(SDL_Surface *surface, Uint32 flag, Uint32 key);
int SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
int SDL_LockSurface(SDL_Surface *surface);
void SDL_UnlockSurface(SDL_Surface *surface);
int SDL_SaveBMP(SDL_Surface *surface, const char *file);

int SDL_NumJoysticks(void);
void *SDL_JoystickOpen(int index);
void SDL_JoystickClose(int index);

#ifdef __cplusplus
}
#endif

#endif
