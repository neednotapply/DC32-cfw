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
};

struct ToolWorkspaceSpan {
	void *ptr;
	uint32_t size;
};

void toolWorkspaceBegin(void);
void toolWorkspaceEnd(void);
bool toolWorkspaceActive(void);
struct ToolWorkspaceSpan toolWorkspaceGet(enum ToolWorkspaceSpanId spanId);

#endif
