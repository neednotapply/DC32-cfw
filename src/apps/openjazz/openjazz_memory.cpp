#include "apps/openjazz/openjazz_memory.h"
#include "apps/openjazz/openjazz_heap.h"
#include "apps/openjazz/openjazz_install.h"

#include <stdio.h>
#include <string.h>

extern "C" {
#include "apps/port/port_runtime.h"
}

static bool gOjLevelArenaBusy;
static uint32_t gOjPaletteSlots[
	(DC32_OJ_PALETTE_SLOT_COUNT + 31u) / 32u];

static uint8_t *ojPalettePoolStart(void)
{
	return (uint8_t*)DC32_PORT_OPENJAZZ_LEVEL_START +
		DC32_OJ_LEVEL_OBJECT_LIMIT;
}

static void ojPalettePoolReset(void)
{
	memset(gOjPaletteSlots, 0, sizeof(gOjPaletteSlots));
}

void *dc32OjLevelArenaAcquire(size_t size)
{
	if (gOjLevelArenaBusy || !size ||
			size > DC32_PORT_OPENJAZZ_LEVEL_SIZE)
		return NULL;
	gOjLevelArenaBusy = true;
	memset(DC32_PORT_OPENJAZZ_LEVEL_START, 0, DC32_PORT_OPENJAZZ_LEVEL_SIZE);
	ojPalettePoolReset();
	return DC32_PORT_OPENJAZZ_LEVEL_START;
}

void dc32OjLevelArenaRelease(void *ptr)
{
	if (ptr == DC32_PORT_OPENJAZZ_LEVEL_START) {
		gOjLevelArenaBusy = false;
		ojPalettePoolReset();
	}
}

bool dc32OjLevelArenaOwns(const void *ptr)
{
	return ptr == DC32_PORT_OPENJAZZ_LEVEL_START;
}

bool dc32OjCacheWorkspaceAcquire(void **table, size_t tableSize,
	void **buffer, size_t bufferSize, void **decode, size_t decodeSize)
{
	size_t tableAligned = (tableSize + 255u) & ~255u;
	size_t bufferAligned = (bufferSize + 255u) & ~255u;
	size_t total = tableAligned + bufferAligned + decodeSize;
	uint8_t *workspace = (uint8_t*)dc32OjLevelArenaAcquire(total);

	if (!workspace)
		return false;
	if (table)
		*table = workspace;
	if (buffer)
		*buffer = workspace + tableAligned;
	if (decode)
		*decode = workspace + tableAligned + bufferAligned;
	return true;
}

void dc32OjCacheWorkspaceRelease(void)
{
	dc32OjLevelArenaRelease(DC32_PORT_OPENJAZZ_LEVEL_START);
}

static void ojRequireHeapValid(const char *stage)
{
	if (!dc32OjHeapValidate(DC32_OJ_HEAP_PERSISTENT) ||
			!dc32OjHeapValidate(DC32_OJ_HEAP_TRANSIENT)) {
		dc32OjLoadingFailure("Heap corruption",
			dc32OjHeapErrorName(dc32OjHeapLastError()));
		dc32OjFatalAllocation(0);
	}
	if (stage)
		dc32OjLoadingStage(stage);
}

void dc32OjBeginLevelMemory(void)
{
	dc32OjHeapSelect(DC32_OJ_HEAP_PERSISTENT);
	ojRequireHeapValid("Resetting level memory");
	if (!dc32OjHeapResetTransient()) {
		dc32OjLoadingFailure("Heap reset failed",
			dc32OjHeapErrorName(dc32OjHeapLastError()));
		dc32OjFatalAllocation(0);
	}
	dc32OjHeapSelect(DC32_OJ_HEAP_TRANSIENT);
}

void dc32OjEndLevelMemory(void)
{
	ojRequireHeapValid("Resetting level memory");
	dc32OjHeapSelect(DC32_OJ_HEAP_PERSISTENT);
	if (!dc32OjHeapResetTransient() ||
			!dc32OjHeapValidate(DC32_OJ_HEAP_PERSISTENT)) {
		dc32OjLoadingFailure("Heap reset failed",
			dc32OjHeapErrorName(dc32OjHeapLastError()));
		dc32OjFatalAllocation(0);
	}
}

void dc32OjRequireGameplayHeap(void)
{
	char detail[64];
	uint32_t freeBytes;
	uint32_t largest;

	dc32OjLoadingStage("Checking gameplay memory reserve");
	ojRequireHeapValid(NULL);
	freeBytes = dc32OjHeapBytesFree(DC32_OJ_HEAP_TRANSIENT);
	largest = dc32OjHeapLargestFreeBlock(DC32_OJ_HEAP_TRANSIENT);
	snprintf(detail, sizeof(detail), "Free %u; block %u",
		(unsigned int)freeBytes, (unsigned int)largest);
	dc32OjLoadingAsset(detail);
	if (freeBytes < 48u * 1024u || largest < 32u * 1024u)
		dc32OjFatalAllocation(32u * 1024u);
}

void *dc32OjPaletteEffectAlloc(size_t size)
{
	if (!gOjLevelArenaBusy || !size || size > DC32_OJ_PALETTE_SLOT_SIZE)
		return NULL;
	for (uint32_t slot = 0; slot < DC32_OJ_PALETTE_SLOT_COUNT; slot++) {
		uint32_t word = slot >> 5;
		uint32_t mask = 1u << (slot & 31u);

		if (!(gOjPaletteSlots[word] & mask)) {
			gOjPaletteSlots[word] |= mask;
			return ojPalettePoolStart() + slot * DC32_OJ_PALETTE_SLOT_SIZE;
		}
	}
	return NULL;
}

bool dc32OjPaletteEffectOwns(const void *ptr)
{
	uintptr_t start = (uintptr_t)ojPalettePoolStart();
	uintptr_t end =
		start + DC32_OJ_PALETTE_SLOT_COUNT * DC32_OJ_PALETTE_SLOT_SIZE;
	uintptr_t value = (uintptr_t)ptr;

	return value >= start && value < end &&
		((value - start) % DC32_OJ_PALETTE_SLOT_SIZE) == 0u;
}

void dc32OjPaletteEffectFree(void *ptr)
{
	uint8_t *start = ojPalettePoolStart();
	uint32_t slot;

	if (!dc32OjPaletteEffectOwns(ptr))
		return;
	slot = (uint32_t)(((uint8_t*)ptr - start) /
		DC32_OJ_PALETTE_SLOT_SIZE);
	gOjPaletteSlots[slot >> 5] &= ~(1u << (slot & 31u));
}

uint32_t dc32OjPaletteEffectSlotsUsed(void)
{
	uint32_t used = 0;

	for (uint32_t slot = 0; slot < DC32_OJ_PALETTE_SLOT_COUNT; slot++)
		if (gOjPaletteSlots[slot >> 5] & (1u << (slot & 31u)))
			used++;
	return used;
}
