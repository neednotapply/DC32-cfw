#ifndef NES_H
#define NES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NES_SAVE_RAM_SIZE 0x2000u

enum NesRegion {
	NesRegionNtsc = 0,
	NesRegionPal = 1,
	NesRegionDendy = 2,
};

struct NesRomInfo {
	char name[16];
	uint32_t romSize;
	uint32_t saveRamSize;
	uint8_t mapper;
	enum NesRegion region;
	bool hasSaveRam;
};

bool nesAnalyzeRom(const void *rom, uint32_t size, struct NesRomInfo *info);
void nesRun(const void *rom, uint32_t romSize, void *saveRam, uint32_t saveRamSize,
	void (*ledsTick)(void));
void nesAbort(void);
void nesRefreshDisplayOptions(void);
void nesPortDrawLine(uint16_t line, const uint16_t *pixels256);
uint32_t nesGetLoadedRomSize(void);

#ifdef __cplusplus
}
#endif

#endif
