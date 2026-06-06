#include "nes.h"

#include <string.h>

#define NES_MAGIC0 'N'
#define NES_MAGIC1 'E'
#define NES_MAGIC2 'S'
#define NES_MAGIC3 0x1a
#define NES_HEADER_SIZE 16u
#define NES_FRAME_BASE_NTSC 60u
#define NES_FRAME_BASE_PAL 50u

static enum NesRegion nesAnalyzeDetectRegion(const uint8_t *bytes)
{
	if ((bytes[7] & 0x0c) == 0x08) {
		switch (bytes[12] & 0x03) {
			case 1:
				return NesRegionPal;
			case 3:
				return NesRegionDendy;
			default:
				return NesRegionNtsc;
		}
	}
	return (bytes[9] & 0x01) ? NesRegionPal : NesRegionNtsc;
}

bool nesAnalyzeRom(const void *rom, uint32_t size, struct NesRomInfo *info)
{
	const uint8_t *bytes = (const uint8_t*)rom;
	uint32_t prgSize, chrSize, trainerSize, expectedSize;
	uint8_t mapper;

	(void)NES_FRAME_BASE_NTSC;
	(void)NES_FRAME_BASE_PAL;
	if (!bytes || size < NES_HEADER_SIZE)
		return false;
	if (bytes[0] != NES_MAGIC0 || bytes[1] != NES_MAGIC1 ||
			bytes[2] != NES_MAGIC2 || bytes[3] != NES_MAGIC3)
		return false;

	prgSize = (uint32_t)bytes[4] * 0x4000u;
	chrSize = (uint32_t)bytes[5] * 0x2000u;
	trainerSize = (bytes[6] & 0x04) ? 512u : 0u;
	expectedSize = NES_HEADER_SIZE + trainerSize + prgSize + chrSize;
	if (!prgSize || expectedSize > size)
		return false;

	mapper = (uint8_t)((bytes[6] >> 4) | (bytes[7] & 0xf0));
	if (mapper != 0 && mapper != 1 && mapper != 2 && mapper != 3 && mapper != 4 && mapper != 7)
		return false;

	if (info) {
		memset(info, 0, sizeof(*info));
		info->romSize = expectedSize;
		info->saveRamSize = NES_SAVE_RAM_SIZE;
		info->mapper = mapper;
		info->region = nesAnalyzeDetectRegion(bytes);
		info->hasSaveRam = (bytes[6] & 0x02) != 0;
		info->name[0] = 'N';
		info->name[1] = 'E';
		info->name[2] = 'S';
		info->name[3] = ' ';
		info->name[4] = 'M';
		if (mapper >= 100) {
			info->name[5] = (char)('0' + mapper / 100);
			info->name[6] = (char)('0' + (mapper / 10) % 10);
			info->name[7] = (char)('0' + mapper % 10);
			info->name[8] = 0;
		}
		else if (mapper >= 10) {
			info->name[5] = (char)('0' + mapper / 10);
			info->name[6] = (char)('0' + mapper % 10);
			info->name[7] = 0;
		}
		else {
			info->name[5] = (char)('0' + mapper);
			info->name[6] = 0;
		}
	}
	return true;
}
