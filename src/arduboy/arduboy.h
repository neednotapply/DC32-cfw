#ifndef ARDUBOY_H
#define ARDUBOY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARDUBOY_SAVE_RAM_SIZE 0x400u
#define ARDUBOY_DISPLAY_WIDTH 128u
#define ARDUBOY_DISPLAY_HEIGHT 64u
#define ARDUBOY_FRAME_RATE 60u
#define ARDUBOY_FLASH_SIZE 0x8000u

struct ArduboyRomInfo {
	char name[16];
	uint32_t romSize;
	uint32_t saveRamSize;
	uint32_t flashUsed;
	bool isPackage;
};

bool arduboyAnalyzeRom(const void *rom, uint32_t size, struct ArduboyRomInfo *info);
bool arduboyExtractPackageToFlash(const void *packageData, uint32_t packageSize,
	uint32_t flashAddr, uint32_t maxSize, void *inflateDict, uint32_t inflateDictSize,
	void *writeBuf, uint32_t writeBufSize, uint32_t *hexSizeP);
void arduboyRun(const void *rom, uint32_t romSize, void *saveRam, uint32_t saveRamSize);
void arduboyAbort(void);
void arduboySetRotation(bool flipped);
void arduboyRefreshDisplayOptions(void);
void arduboyPortInGameMenu(void);

#ifdef __cplusplus
}
#endif

#endif
