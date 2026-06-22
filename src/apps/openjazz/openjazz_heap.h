#ifndef DC32_OPENJAZZ_HEAP_H
#define DC32_OPENJAZZ_HEAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum Dc32OjHeapId {
	DC32_OJ_HEAP_PERSISTENT = 0,
	DC32_OJ_HEAP_TRANSIENT = 1,
	DC32_OJ_HEAP_COUNT = 2
};

enum Dc32OjHeapError {
	DC32_OJ_HEAP_OK = 0,
	DC32_OJ_HEAP_BAD_REGION,
	DC32_OJ_HEAP_BAD_HEADER,
	DC32_OJ_HEAP_BAD_FOOTER,
	DC32_OJ_HEAP_BAD_LINK,
	DC32_OJ_HEAP_DOUBLE_FREE,
	DC32_OJ_HEAP_INVALID_FREE
};

bool dc32OjHeapInit(void *persistent, uint32_t persistentSize,
	void *transient, uint32_t transientSize);
void dc32OjHeapSelect(enum Dc32OjHeapId heap);
enum Dc32OjHeapId dc32OjHeapSelected(void);
bool dc32OjHeapResetTransient(void);
bool dc32OjHeapValidate(enum Dc32OjHeapId heap);
bool dc32OjHeapOwns(enum Dc32OjHeapId heap, const void *ptr);
void *dc32OjHeapMalloc(size_t size);
void *dc32OjHeapCalloc(size_t count, size_t size);
void *dc32OjHeapRealloc(void *ptr, size_t size);
void dc32OjHeapFree(void *ptr);
uint32_t dc32OjHeapBytesUsed(enum Dc32OjHeapId heap);
uint32_t dc32OjHeapPeakBytesUsed(enum Dc32OjHeapId heap);
uint32_t dc32OjHeapBytesFree(enum Dc32OjHeapId heap);
uint32_t dc32OjHeapLargestFreeBlock(enum Dc32OjHeapId heap);
enum Dc32OjHeapError dc32OjHeapLastError(void);
const char *dc32OjHeapErrorName(enum Dc32OjHeapError error);

#ifdef __cplusplus
}
#endif

#endif
