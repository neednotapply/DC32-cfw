#ifndef DC32_OPENJAZZ_MEMORY_H
#define DC32_OPENJAZZ_MEMORY_H

#include <stddef.h>
#include <stdint.h>

#define DC32_OJ_TRANSIENT_MIN        (56u * 1024u)
#define DC32_OJ_LEVEL_OBJECT_LIMIT   0x00016000u
#define DC32_OJ_PALETTE_SLOT_SIZE    32u
#define DC32_OJ_PALETTE_SLOT_COUNT   288u

void *dc32OjLevelArenaAcquire(size_t size);
void dc32OjLevelArenaRelease(void *ptr);
bool dc32OjLevelArenaOwns(const void *ptr);
bool dc32OjCacheWorkspaceAcquire(void **table, size_t tableSize,
	void **buffer, size_t bufferSize, void **decode, size_t decodeSize);
void dc32OjCacheWorkspaceRelease(void);
void dc32OjBeginLevelMemory(void);
void dc32OjEndLevelMemory(void);
void dc32OjRequireGameplayHeap(void);
void *dc32OjPaletteEffectAlloc(size_t size);
void dc32OjPaletteEffectFree(void *ptr);
bool dc32OjPaletteEffectOwns(const void *ptr);
uint32_t dc32OjPaletteEffectSlotsUsed(void);

extern "C" void dc32OjFatalAllocation(size_t size);

#endif
