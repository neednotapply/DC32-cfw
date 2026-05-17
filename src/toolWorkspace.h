#ifndef _TOOL_WORKSPACE_H_
#define _TOOL_WORKSPACE_H_

#include <stdbool.h>
#include <stdint.h>

enum ToolWorkspaceSpanId {
	ToolWorkspaceCartRam,
	ToolWorkspaceCartRamLower,
	ToolWorkspaceCartRamUpper,
	ToolWorkspaceWram,
	ToolWorkspaceVram,
	ToolWorkspaceSpanNum,
};

enum ToolWorkspaceOwner {
	ToolWorkspaceOwnerNone = 0,
	ToolWorkspaceOwnerFileBrowser,
	ToolWorkspaceOwnerMusic,
	ToolWorkspaceOwnerIr,
	ToolWorkspaceOwnerBadUsb,
	ToolWorkspaceOwnerTransfer,
	ToolWorkspaceOwnerImage,
};

struct ToolWorkspaceSpan {
	void *ptr;
	uint32_t size;
};

void toolWorkspaceBegin(void);
void toolWorkspaceEnd(void);
bool toolWorkspaceActive(void);
void toolWorkspaceReleaseAll(void);
bool toolWorkspaceAcquire(enum ToolWorkspaceSpanId spanId, enum ToolWorkspaceOwner owner, struct ToolWorkspaceSpan *spanP);
void toolWorkspaceRelease(enum ToolWorkspaceSpanId spanId, enum ToolWorkspaceOwner owner);
struct ToolWorkspaceSpan toolWorkspaceGet(enum ToolWorkspaceSpanId spanId);

#endif
