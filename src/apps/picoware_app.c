#include "apps/picoware_app.h"

#ifndef DCAPP_RUNTIME_ID
#error "DCAPP_RUNTIME_ID must be provided by the app target"
#endif

const struct DcAppImageHeader dcAppImageHeader __attribute__((section(".dcapp_header"), used, aligned(256))) = {
	.magic = DCAPP_MAGIC,
	.headerSize = DCAPP_HEADER_SIZE,
	.abiVersion = DCAPP_ABI_VERSION,
	.runtime = DCAPP_RUNTIME_ID,
	.loadAddr = 0x10080000u,
	.appRamStart = 0x2005F000u,
	.appRamSize = 0x00014000u,
};

int dcAppEntry(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	return picowareAppRun(host, args);
}

void dcAppAbort(void)
{
}

void dcAppRefreshDisplayOptions(void)
{
}
