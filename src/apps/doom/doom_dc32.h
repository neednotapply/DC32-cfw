#ifndef DC32_DOOM_PORT_H
#define DC32_DOOM_PORT_H

#include <stdarg.h>
#include <stdint.h>
#include "dcApp.h"
#include "i_video.h"
#include "memMap.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DOOM_DC32_SCREEN_W 320u
#define DOOM_DC32_SCREEN_H 200u
#define DOOM_DC32_PRESENT_H 240u
#define DOOM_DC32_CART_SCRATCH ((uint8_t*)0x20000000u)
#define DOOM_DC32_CART_SCRATCH_SIZE 0x0002b000u
#define DOOM_DC32_FRAME_BYTES (2u * SCREENWIDTH * MAIN_VIEWHEIGHT)
#define DOOM_DC32_RENDER_COL_MAX 3200u
#define DOOM_DC32_LIST_BYTES (DOOM_DC32_RENDER_COL_MAX * 12u + 64u * 64u)
#define DOOM_DC32_ZONE_BYTES (40u * 1024u)
#define DOOM_DC32_WHX_PATH "/APPS/doom1.whx"
#define DOOM_DC32_WHX_FLASH_ADDR QSPI_ROM_START
#define DOOM_DC32_WHX_LEGACY_VPATCH_COUNT 384u

extern const struct DcAppHostApi *doomDc32Host;
extern struct Canvas doomDc32Canvas;
extern bool doomDc32CanvasValid;
extern uint8_t (*frame_buffer)[SCREENWIDTH * MAIN_VIEWHEIGHT];
extern uint8_t *doomDc32ListBuffer;
extern uint32_t doomDc32ListBufferSize;

void doomDc32PresentFrame(void);
void *doomDc32VpatchLists(void);
void doomDc32Status(const char *title, const char *detail, uint32_t done, uint32_t total);
void doomDc32WaitForCenter(const char *title, const char *detail);
void doomDc32SetError(const char *fmt, ...);
void doomDc32SetErrorV(const char *fmt, va_list ap);
void doomDc32SoundStopAll(void);
void doomDc32Exit(int code) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif
