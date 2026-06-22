#include "apps/openjazz/openjazz_heap.h"

#include <limits.h>
#include <string.h>

#define OJ_HEAP_HEADER_MAGIC 0x4f4a4844u
#define OJ_HEAP_FREE_MAGIC   0x46524545u
#define OJ_HEAP_USED_MAGIC   0x55534544u
#define OJ_HEAP_FOOT_MAGIC   0x4f4a4654u
#define OJ_HEAP_SIZE_XOR     0xa55a39c3u
#define OJ_HEAP_ALIGN        8u
#define OJ_HEAP_MIN_PAYLOAD  16u

struct OjHeapBlock {
	uint32_t magic;
	uint32_t size;
	struct OjHeapBlock *next;
	uint32_t state;
};

struct OjHeapFooter {
	uint32_t magic;
	uint32_t sizeXor;
};

struct OjHeapRegion {
	struct OjHeapBlock *head;
	uintptr_t start;
	uintptr_t end;
	uint32_t used;
	uint32_t peak;
};

static struct OjHeapRegion gOjHeaps[DC32_OJ_HEAP_COUNT];
static enum Dc32OjHeapId gOjHeapSelected = DC32_OJ_HEAP_PERSISTENT;
static enum Dc32OjHeapError gOjHeapError;

static uintptr_t ojAlignUp(uintptr_t value)
{
	return (value + OJ_HEAP_ALIGN - 1u) & ~(uintptr_t)(OJ_HEAP_ALIGN - 1u);
}

static uintptr_t ojAlignDown(uintptr_t value)
{
	return value & ~(uintptr_t)(OJ_HEAP_ALIGN - 1u);
}

static uint32_t ojAlignSize(uint32_t value)
{
	return (value + OJ_HEAP_ALIGN - 1u) & ~(OJ_HEAP_ALIGN - 1u);
}

static struct OjHeapFooter *ojFooter(struct OjHeapBlock *block)
{
	return (struct OjHeapFooter*)((uint8_t*)(block + 1) + block->size);
}

static uintptr_t ojBlockEnd(struct OjHeapBlock *block)
{
	return (uintptr_t)(ojFooter(block) + 1);
}

static void ojWriteFooter(struct OjHeapBlock *block)
{
	struct OjHeapFooter *footer = ojFooter(block);

	footer->magic = OJ_HEAP_FOOT_MAGIC ^ block->state;
	footer->sizeXor = block->size ^ OJ_HEAP_SIZE_XOR;
}

static __attribute__((noinline, noclone)) bool ojInitRegion(
	enum Dc32OjHeapId id, void *base, uint32_t size)
{
	struct OjHeapRegion *region = &gOjHeaps[id];
	uintptr_t start = ojAlignUp((uintptr_t)base);
	uintptr_t end = ojAlignDown((uintptr_t)base + size);
	struct OjHeapBlock *block;
	uint32_t overhead = sizeof(*block) + sizeof(struct OjHeapFooter);

	memset(region, 0, sizeof(*region));
	if (!base || end <= start || end - start <= overhead + OJ_HEAP_MIN_PAYLOAD) {
		gOjHeapError = DC32_OJ_HEAP_BAD_REGION;
		return false;
	}
	block = (struct OjHeapBlock*)start;
	block->magic = OJ_HEAP_HEADER_MAGIC;
	block->size = (uint32_t)(end - start - overhead);
	block->size &= ~(OJ_HEAP_ALIGN - 1u);
	block->next = NULL;
	block->state = OJ_HEAP_FREE_MAGIC;
	region->head = block;
	region->start = start;
	region->end = end;
	ojWriteFooter(block);
	return true;
}

static bool ojValidateBlock(struct OjHeapRegion *region,
	struct OjHeapBlock *block)
{
	uintptr_t address = (uintptr_t)block;
	struct OjHeapFooter *footer;
	uintptr_t end;

	if (address < region->start || address > region->end - sizeof(*block) ||
			(address & (OJ_HEAP_ALIGN - 1u))) {
		gOjHeapError = DC32_OJ_HEAP_BAD_LINK;
		return false;
	}
	if (block->magic != OJ_HEAP_HEADER_MAGIC ||
			(block->state != OJ_HEAP_FREE_MAGIC &&
				block->state != OJ_HEAP_USED_MAGIC) ||
			(block->size & (OJ_HEAP_ALIGN - 1u))) {
		gOjHeapError = DC32_OJ_HEAP_BAD_HEADER;
		return false;
	}
	end = address + sizeof(*block) + block->size +
		sizeof(struct OjHeapFooter);
	if (end < address || end > region->end) {
		gOjHeapError = DC32_OJ_HEAP_BAD_HEADER;
		return false;
	}
	footer = ojFooter(block);
	if (footer->magic != (OJ_HEAP_FOOT_MAGIC ^ block->state) ||
			footer->sizeXor != (block->size ^ OJ_HEAP_SIZE_XOR)) {
		gOjHeapError = DC32_OJ_HEAP_BAD_FOOTER;
		return false;
	}
	if (block->next && ((uintptr_t)block->next != end ||
			(uintptr_t)block->next <= address)) {
		gOjHeapError = DC32_OJ_HEAP_BAD_LINK;
		return false;
	}
	return true;
}

bool dc32OjHeapValidate(enum Dc32OjHeapId id)
{
	struct OjHeapRegion *region;
	struct OjHeapBlock *block;
	uint32_t limit;

	if (gOjHeapError == DC32_OJ_HEAP_DOUBLE_FREE ||
			gOjHeapError == DC32_OJ_HEAP_INVALID_FREE)
		return false;
	if ((unsigned int)id >= DC32_OJ_HEAP_COUNT) {
		gOjHeapError = DC32_OJ_HEAP_BAD_REGION;
		return false;
	}
	region = &gOjHeaps[id];
	if (!region->head || region->end <= region->start) {
		gOjHeapError = DC32_OJ_HEAP_BAD_REGION;
		return false;
	}
	block = region->head;
	limit = (uint32_t)((region->end - region->start) /
		(sizeof(*block) + sizeof(struct OjHeapFooter))) + 1u;
	while (block && limit--) {
		if (!ojValidateBlock(region, block))
			return false;
		block = block->next;
	}
	if (block || !limit) {
		gOjHeapError = DC32_OJ_HEAP_BAD_LINK;
		return false;
	}
	gOjHeapError = DC32_OJ_HEAP_OK;
	return true;
}

bool dc32OjHeapInit(void *persistent, uint32_t persistentSize,
	void *transient, uint32_t transientSize)
{
	memset(gOjHeaps, 0, sizeof(gOjHeaps));
	gOjHeapError = DC32_OJ_HEAP_OK;
	gOjHeapSelected = DC32_OJ_HEAP_PERSISTENT;
	if (!ojInitRegion(DC32_OJ_HEAP_PERSISTENT, persistent, persistentSize))
		return false;
	if (!ojInitRegion(DC32_OJ_HEAP_TRANSIENT, transient, transientSize))
		return false;
	gOjHeapError = DC32_OJ_HEAP_OK;
	return true;
}

void dc32OjHeapSelect(enum Dc32OjHeapId heap)
{
	if ((unsigned int)heap < DC32_OJ_HEAP_COUNT)
		gOjHeapSelected = heap;
}

enum Dc32OjHeapId dc32OjHeapSelected(void)
{
	return gOjHeapSelected;
}

bool dc32OjHeapResetTransient(void)
{
	struct OjHeapRegion *region = &gOjHeaps[DC32_OJ_HEAP_TRANSIENT];
	void *base = (void*)region->start;
	uint32_t size = (uint32_t)(region->end - region->start);

	return ojInitRegion(DC32_OJ_HEAP_TRANSIENT, base, size);
}

static void ojSplitBlock(struct OjHeapBlock *block, uint32_t size)
{
	uint32_t overhead = sizeof(*block) + sizeof(struct OjHeapFooter);
	struct OjHeapBlock *next;

	if (block->size < size + overhead + OJ_HEAP_MIN_PAYLOAD)
		return;
	next = (struct OjHeapBlock*)((uint8_t*)(block + 1) + size +
		sizeof(struct OjHeapFooter));
	next->magic = OJ_HEAP_HEADER_MAGIC;
	next->size = block->size - size - overhead;
	next->next = block->next;
	next->state = OJ_HEAP_FREE_MAGIC;
	block->size = size;
	block->next = next;
	ojWriteFooter(block);
	ojWriteFooter(next);
}

__attribute__((noinline)) void *dc32OjHeapMalloc(size_t size)
{
	struct OjHeapRegion *region = &gOjHeaps[gOjHeapSelected];
	struct OjHeapBlock *block;
	uint32_t wanted;

	if (!size)
		size = 1;
	if (size > UINT32_MAX - (OJ_HEAP_ALIGN - 1u)) {
		gOjHeapError = DC32_OJ_HEAP_BAD_REGION;
		return NULL;
	}
	if (!dc32OjHeapValidate(gOjHeapSelected))
		return NULL;
	wanted = ojAlignSize((uint32_t)size);
	for (block = region->head; block; block = block->next) {
		if (block->state != OJ_HEAP_FREE_MAGIC || block->size < wanted)
			continue;
		ojSplitBlock(block, wanted);
		block->state = OJ_HEAP_USED_MAGIC;
		ojWriteFooter(block);
		region->used += block->size;
		if (region->used > region->peak)
			region->peak = region->used;
		gOjHeapError = DC32_OJ_HEAP_OK;
		return block + 1;
	}
	gOjHeapError = DC32_OJ_HEAP_OK;
	return NULL;
}

void *dc32OjHeapCalloc(size_t count, size_t size)
{
	size_t total = count * size;
	void *ptr;

	if (size && total / size != count)
		return NULL;
	ptr = dc32OjHeapMalloc(total);
	if (ptr)
		memset(ptr, 0, total);
	return ptr;
}

bool dc32OjHeapOwns(enum Dc32OjHeapId id, const void *ptr)
{
	uintptr_t address = (uintptr_t)ptr;
	struct OjHeapRegion *region;

	if ((unsigned int)id >= DC32_OJ_HEAP_COUNT)
		return false;
	region = &gOjHeaps[id];
	return ptr && address > region->start && address < region->end;
}

static enum Dc32OjHeapId ojHeapForPointer(const void *ptr)
{
	for (unsigned int id = 0; id < DC32_OJ_HEAP_COUNT; id++)
		if (dc32OjHeapOwns((enum Dc32OjHeapId)id, ptr))
			return (enum Dc32OjHeapId)id;
	return DC32_OJ_HEAP_COUNT;
}

static struct OjHeapBlock *ojFindBlock(struct OjHeapRegion *region,
	const void *ptr)
{
	struct OjHeapBlock *block = region->head;
	uint32_t limit = (uint32_t)((region->end - region->start) /
		(sizeof(*block) + sizeof(struct OjHeapFooter))) + 1u;

	while (block && limit--) {
		if (block + 1 == ptr)
			return block;
		block = block->next;
	}
	return NULL;
}

static void ojCoalesce(struct OjHeapRegion *region)
{
	struct OjHeapBlock *block = region->head;

	while (block && block->next) {
		struct OjHeapBlock *next = block->next;

		if (block->state == OJ_HEAP_FREE_MAGIC &&
				next->state == OJ_HEAP_FREE_MAGIC &&
				ojBlockEnd(block) == (uintptr_t)next) {
			block->size += sizeof(*next) + sizeof(struct OjHeapFooter) +
				next->size;
			block->next = next->next;
			ojWriteFooter(block);
		} else {
			block = next;
		}
	}
}

void dc32OjHeapFree(void *ptr)
{
	enum Dc32OjHeapId id;
	struct OjHeapRegion *region;
	struct OjHeapBlock *block;

	if (!ptr)
		return;
	id = ojHeapForPointer(ptr);
	if (id == DC32_OJ_HEAP_COUNT) {
		gOjHeapError = DC32_OJ_HEAP_INVALID_FREE;
		return;
	}
	if (!dc32OjHeapValidate(id))
		return;
	region = &gOjHeaps[id];
	block = ojFindBlock(region, ptr);
	if (!block) {
		gOjHeapError = DC32_OJ_HEAP_INVALID_FREE;
		return;
	}
	if (block->state == OJ_HEAP_FREE_MAGIC) {
		gOjHeapError = DC32_OJ_HEAP_DOUBLE_FREE;
		return;
	}
	if (region->used >= block->size)
		region->used -= block->size;
	else
		region->used = 0;
	block->state = OJ_HEAP_FREE_MAGIC;
	ojWriteFooter(block);
	ojCoalesce(region);
	gOjHeapError = DC32_OJ_HEAP_OK;
}

void *dc32OjHeapRealloc(void *ptr, size_t size)
{
	enum Dc32OjHeapId id;
	struct OjHeapBlock *block;
	enum Dc32OjHeapId previous;
	void *replacement;
	uint32_t copySize;

	if (!ptr)
		return dc32OjHeapMalloc(size);
	if (!size) {
		dc32OjHeapFree(ptr);
		return NULL;
	}
	id = ojHeapForPointer(ptr);
	if (id == DC32_OJ_HEAP_COUNT || !dc32OjHeapValidate(id)) {
		gOjHeapError = DC32_OJ_HEAP_INVALID_FREE;
		return NULL;
	}
	block = ojFindBlock(&gOjHeaps[id], ptr);
	if (!block || block->state != OJ_HEAP_USED_MAGIC) {
		gOjHeapError = DC32_OJ_HEAP_INVALID_FREE;
		return NULL;
	}
	if (block->size >= size)
		return ptr;
	previous = gOjHeapSelected;
	gOjHeapSelected = id;
	replacement = dc32OjHeapMalloc(size);
	gOjHeapSelected = previous;
	if (!replacement)
		return NULL;
	copySize = block->size;
	memcpy(replacement, ptr, copySize);
	dc32OjHeapFree(ptr);
	return replacement;
}

static __attribute__((noinline)) uint32_t ojHeapStat(
	enum Dc32OjHeapId id, bool largest)
{
	struct OjHeapBlock *block;
	uint32_t value = 0;

	if (!dc32OjHeapValidate(id))
		return 0;
	for (block = gOjHeaps[id].head; block; block = block->next)
		if (block->state == OJ_HEAP_FREE_MAGIC) {
			if (largest) {
				if (block->size > value)
					value = block->size;
			} else {
				value += block->size;
			}
		}
	return value;
}

uint32_t dc32OjHeapBytesUsed(enum Dc32OjHeapId heap)
{
	return (unsigned int)heap < DC32_OJ_HEAP_COUNT ?
		gOjHeaps[heap].used : 0;
}

uint32_t dc32OjHeapPeakBytesUsed(enum Dc32OjHeapId heap)
{
	return (unsigned int)heap < DC32_OJ_HEAP_COUNT ?
		gOjHeaps[heap].peak : 0;
}

uint32_t dc32OjHeapBytesFree(enum Dc32OjHeapId heap)
{
	return ojHeapStat(heap, false);
}

uint32_t dc32OjHeapLargestFreeBlock(enum Dc32OjHeapId heap)
{
	return ojHeapStat(heap, true);
}

enum Dc32OjHeapError dc32OjHeapLastError(void)
{
	return gOjHeapError;
}

const char *dc32OjHeapErrorName(enum Dc32OjHeapError error)
{
	switch (error) {
		case DC32_OJ_HEAP_OK: return "ok";
		case DC32_OJ_HEAP_BAD_REGION: return "region";
		case DC32_OJ_HEAP_BAD_HEADER: return "header";
		case DC32_OJ_HEAP_BAD_FOOTER: return "footer";
		case DC32_OJ_HEAP_BAD_LINK: return "link";
		case DC32_OJ_HEAP_DOUBLE_FREE: return "double free";
		case DC32_OJ_HEAP_INVALID_FREE: return "invalid free";
		default: return "unknown";
	}
}
