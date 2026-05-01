#include "toolWorkspace.h"
#include "memMap.h"
#include "mbc.h"

static bool mToolWorkspaceActive;

void toolWorkspaceBegin(void)
{
	mToolWorkspaceActive = true;
}

void toolWorkspaceEnd(void)
{
	mToolWorkspaceActive = false;
}

bool toolWorkspaceActive(void)
{
	return mToolWorkspaceActive;
}

struct ToolWorkspaceSpan toolWorkspaceGet(enum ToolWorkspaceSpanId spanId)
{
	struct ToolWorkspaceSpan span = {0};

	switch (spanId) {
		case ToolWorkspaceCartRam:
			span.ptr = CART_RAM_ADDR_IN_RAM;
			span.size = QSPI_RAM_SIZE_MAX;
			break;

		case ToolWorkspaceCartRamLower:
			span.ptr = CART_RAM_ADDR_IN_RAM;
			span.size = QSPI_RAM_SIZE_MAX / 2;
			break;

		case ToolWorkspaceCartRamUpper:
			span.ptr = ((uint8_t*)CART_RAM_ADDR_IN_RAM) + QSPI_RAM_SIZE_MAX / 2;
			span.size = QSPI_RAM_SIZE_MAX / 2;
			break;

		case ToolWorkspaceWram:
			span.ptr = mbcPrvGetWramBuf();
			span.size = mbcPrvGetWramBufSize();
			break;

		case ToolWorkspaceVram:
			span.ptr = mbcPrvGetVramBuf();
			span.size = mbcPrvGetVramBufSize();
			break;
	}

	return span;
}
