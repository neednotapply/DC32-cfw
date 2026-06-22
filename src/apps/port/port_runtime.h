#ifndef DC32_PORT_RUNTIME_H
#define DC32_PORT_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "dcApp.h"
#include "dcAppDraw.h"
#include "fatfs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DC32_PORT_SCREEN_W 320u
#define DC32_PORT_SCREEN_H 240u
#define DC32_PORT_OPENJAZZ_CANVAS_START ((void*)0x20000000u)
#define DC32_PORT_OPENJAZZ_CANVAS_SIZE  0x00012c00u
#define DC32_PORT_OPENJAZZ_LEVEL_START  ((void*)0x20012c00u)
#define DC32_PORT_OPENJAZZ_LEVEL_SIZE   0x00018400u
#define DC32_PORT_OPENJAZZ_AUX_START    ((void*)0x20056000u)
#define DC32_PORT_OPENJAZZ_AUX_SIZE     0x00009000u
#define DC32_PORT_CART_HEAP_START ((void*)0x20000000u)
#define DC32_PORT_CART_HEAP_SIZE 0x0002b000u

struct Dc32PortPak {
	struct FatfsFil *file;
	uint32_t size;
	uint32_t position;
	bool positionValid;
};

void dc32PortHeapInit(void *base, uint32_t size);
bool dc32PortHeapAddRegion(void *base, uint32_t size);
void dc32PortHeapInitDefault(void);
void *dc32PortMalloc(size_t size);
void *dc32PortCalloc(size_t nmemb, size_t size);
void *dc32PortRealloc(void *ptr, size_t size);
void dc32PortFree(void *ptr);
uint32_t dc32PortHeapBytesUsed(void);
uint32_t dc32PortHeapPeakBytesUsed(void);
uint32_t dc32PortHeapBytesFree(void);
uint32_t dc32PortHeapLargestFreeBlock(void);

uint16_t dc32PortRgb332ToRgb565(uint8_t color);
void dc32PortPresentRgb332(struct DcAppDrawCtx *draw, const uint8_t *src, uint32_t w, uint32_t h);
void dc32PortPresentIndexed8(struct DcAppDrawCtx *draw, const uint8_t *src, uint32_t w, uint32_t h, const uint8_t paletteRgb332[256]);

bool dc32PortOpenAssetPack(struct FatfsVol *vol, const char *path, struct Dc32PortPak *pak);
bool dc32PortReadAssetPack(struct Dc32PortPak *pak, uint32_t offset, void *dst, uint32_t size);
void dc32PortCloseAssetPack(struct Dc32PortPak *pak);

bool dc32PortEnsureSaveDir(struct FatfsVol *vol);
bool dc32PortSaveRead(struct FatfsVol *vol, const char *appName, void *dst, uint32_t size);
bool dc32PortSaveWrite(struct FatfsVol *vol, const char *appName, const void *src, uint32_t size);

bool dc32PortCenterExitRequested(const struct DcAppHostApi *host);
void dc32PortShowMissingData(const struct DcAppHostApi *host, const struct DcAppRunArgs *args,
	const char *title, const char *path, void *backbuffer);

#ifdef __cplusplus
}
#endif

#endif
