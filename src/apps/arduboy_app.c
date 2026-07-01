#include "arduboy/arduboy.h"
#include "dcApp.h"
#include "settings.h"

const struct DcAppImageHeader dcAppImageHeader __attribute__((section(".dcapp_header"), used, aligned(256))) = {
	.magic = DCAPP_MAGIC,
	.headerSize = DCAPP_HEADER_SIZE,
	.abiVersion = DCAPP_ABI_VERSION,
	.runtime = GameRuntimeArduboy,
	.loadAddr = 0x10080000u,
	.appRamStart = 0x2005F000u,
	.appRamSize = 0x00014000u,
};

int dcAppEntry(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	(void)host;
	if (!args)
		return -1;
	arduboySetRotation(args->rotate);
	arduboyRun(args->rom, args->romSize, args->saveRam, args->saveRamSize);
	return 0;
}

void dcAppAbort(void)
{
	arduboyAbort();
}

void dcAppRefreshDisplayOptions(void)
{
	struct Settings settings;

	settingsGet(&settings);
	arduboySetRotation(settings.rotation);
	arduboyRefreshDisplayOptions();
}
