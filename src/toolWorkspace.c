#include <string.h>
#include "dcApp.h"
#include "toolWorkspace.h"
#include "memMap.h"

static bool mToolWorkspaceActive;
static enum ToolWorkspaceOwner mToolWorkspaceOwners[ToolWorkspaceSpanNum];
static uint8_t mToolWorkspaceCounts[ToolWorkspaceSpanNum];

static bool toolWorkspacePrvSpansOverlap(struct ToolWorkspaceSpan a, struct ToolWorkspaceSpan b)
{
	uintptr_t aStart = (uintptr_t)a.ptr, bStart = (uintptr_t)b.ptr;
	uintptr_t aEnd = aStart + a.size, bEnd = bStart + b.size;

	return aStart < bEnd && bStart < aEnd;
}

static struct ToolWorkspaceSpan toolWorkspacePrvActiveAppSpan(uint32_t offset, uint32_t maxSize)
{
	struct ToolWorkspaceSpan active, span = {0};

	if (!dcAppGetActiveScratch(&active))
		return span;
	if (offset >= active.size)
		return span;
	span.ptr = (uint8_t*)active.ptr + offset;
	span.size = active.size - offset;
	if (span.size > maxSize)
		span.size = maxSize;
	return span;
}

void toolWorkspaceBegin(void)
{
	mToolWorkspaceActive = true;
	toolWorkspaceReleaseAll();
}

void toolWorkspaceEnd(void)
{
	toolWorkspaceReleaseAll();
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
			if (dcAppGetActiveScratch(NULL))
				span = toolWorkspacePrvActiveAppSpan(0, DCAPP_WORKSPACE_WRAM_SIZE);
			else {
				span.ptr = (void*)DCAPP_WORKSPACE_WRAM_START;
				span.size = DCAPP_WORKSPACE_WRAM_SIZE;
			}
			break;

		case ToolWorkspaceVram:
			if (dcAppGetActiveScratch(NULL))
				span = toolWorkspacePrvActiveAppSpan(DCAPP_WORKSPACE_WRAM_SIZE, DCAPP_WORKSPACE_VRAM_SIZE);
			else {
				span.ptr = (void*)DCAPP_WORKSPACE_VRAM_START;
				span.size = DCAPP_WORKSPACE_VRAM_SIZE;
			}
			break;

		case ToolWorkspaceSpanNum:
			break;
	}

	return span;
}

void toolWorkspaceReleaseAll(void)
{
	memset(mToolWorkspaceOwners, 0, sizeof(mToolWorkspaceOwners));
	memset(mToolWorkspaceCounts, 0, sizeof(mToolWorkspaceCounts));
}

bool toolWorkspaceAcquire(enum ToolWorkspaceSpanId spanId, enum ToolWorkspaceOwner owner, struct ToolWorkspaceSpan *spanP)
{
	struct ToolWorkspaceSpan span;
	uint_fast8_t i;

	if (!mToolWorkspaceActive || spanId >= ToolWorkspaceSpanNum || owner == ToolWorkspaceOwnerNone)
		return false;

	span = toolWorkspaceGet(spanId);
	if (!span.ptr || !span.size)
		return false;

	for (i = 0; i < ToolWorkspaceSpanNum; i++) {
		struct ToolWorkspaceSpan other;

		if (!mToolWorkspaceCounts[i] || mToolWorkspaceOwners[i] == owner)
			continue;
		other = toolWorkspaceGet((enum ToolWorkspaceSpanId)i);
		if (other.ptr && other.size && toolWorkspacePrvSpansOverlap(span, other))
			return false;
	}

	mToolWorkspaceOwners[spanId] = owner;
	if (mToolWorkspaceCounts[spanId] != 0xff)
		mToolWorkspaceCounts[spanId]++;
	if (spanP)
		*spanP = span;
	return true;
}

void toolWorkspaceRelease(enum ToolWorkspaceSpanId spanId, enum ToolWorkspaceOwner owner)
{
	if (spanId >= ToolWorkspaceSpanNum)
		return;
	if (!mToolWorkspaceCounts[spanId] || mToolWorkspaceOwners[spanId] != owner)
		return;
	if (!--mToolWorkspaceCounts[spanId])
		mToolWorkspaceOwners[spanId] = ToolWorkspaceOwnerNone;
}
