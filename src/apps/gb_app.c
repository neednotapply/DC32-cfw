#include "dcApp.h"
#include "gb.h"
#include "mbc.h"

const struct DcAppImageHeader dcAppImageHeader __attribute__((section(".dcapp_header"), used, aligned(256))) = {
	.magic = DCAPP_MAGIC,
	.headerSize = DCAPP_HEADER_SIZE,
	.abiVersion = DCAPP_ABI_VERSION,
	.runtime = GameRuntimeGb,
	.loadAddr = 0x10080000u,
	.appRamStart = 0x2005F000u,
	.appRamSize = 0x00014000u,
};

int dcAppEntry(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	uint32_t romSize, saveRamSize;

	(void)host;
	if (!args)
		return -1;
	romSize = args->romSize;
	saveRamSize = args->saveRamSize;
	if (!mbcInit(args->rom, &romSize, args->saveRam, &saveRamSize))
		return -1;
	if (saveRamSize > args->saveRamSize)
		return -1;
	gbSetFrameDithering(1);
	gbSetDmgPaletteForRom(args->rom, args->romSize, args->gbPalette);
	gbRun(args->presentAsCgb);
	return 0;
}

void dcAppAbort(void)
{
	gbAbort();
}

void dcAppRefreshDisplayOptions(void)
{
}
