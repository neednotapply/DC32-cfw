/* YSoccer 19-derived DC32 app, GPL-2.0-only. */
#include "apps/soccer/soccer_port.h"

#ifndef DCAPP_RUNTIME_ID
#error "DCAPP_RUNTIME_ID must be provided by the app target"
#endif

const struct DcAppImageHeader dcAppImageHeader __attribute__((section(".dcapp_header"), used, aligned(256))) = {
	.magic = DCAPP_MAGIC,
	.headerSize = DCAPP_HEADER_SIZE,
	.abiVersion = DCAPP_ABI_VERSION,
	.runtime = DCAPP_RUNTIME_ID,
	.loadAddr = 0x10100000u,
	.appRamStart = 0x2005F000u,
	.appRamSize = 0x00014000u,
	.flags = DCAPP_IMAGE_FLAG_LARGE_XIP,
};

int dcAppEntry(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	return soccerAppRun(host, args);
}

void dcAppAbort(void)
{
	soccerAppAbort();
}

void dcAppRefreshDisplayOptions(void)
{
}
