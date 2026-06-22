#include <new>
#include <setjmp.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "SDL.h"
#include "apps/openjazz/openjazz_cache.h"
#include "apps/openjazz/openjazz_heap.h"
#include "apps/openjazz/openjazz_install.h"
#include "apps/openjazz/openjazz_memory.h"
#include "apps/openjazz/openjazz_pack.h"
#include "apps/port/port_runtime.h"

extern "C" {
#include "dcApp.h"
#include "dcAppDraw.h"
#include "toolWorkspace.h"
}

#ifndef DCAPP_RUNTIME_ID
#error "DCAPP_RUNTIME_ID must be provided by the app target"
#endif

extern int openjazz_main(int argc, char *argv[]);

typedef void (*OjCtor)(void);

extern "C" {
extern OjCtor __preinit_array_start[];
extern OjCtor __preinit_array_end[];
extern OjCtor __init_array_start[];
extern OjCtor __init_array_end[];
}

static jmp_buf gOjFatalJump;
static bool gOjFatalReady;
static size_t gOjFailedAllocSize;
static uint32_t gOjFailedLargestBlock;
static uint32_t gOjFailedHeapError;

extern "C" const struct DcAppImageHeader dcAppImageHeader __attribute__((section(".dcapp_header"), used, aligned(256))) = {
	.magic = DCAPP_MAGIC,
	.headerSize = DCAPP_HEADER_SIZE,
	.abiVersion = DCAPP_ABI_VERSION,
	.runtime = DCAPP_RUNTIME_ID,
	.flags = 0,
	.loadAddr = 0x10080000u,
	.appRamStart = 0x2005F000u,
	.appRamSize = 0x00014000u,
};

static void ojFatalAlloc(size_t size)
{
	dc32OjSdlRequestQuit();
	gOjFailedAllocSize = size;
	gOjFailedLargestBlock = dc32PortHeapLargestFreeBlock();
	gOjFailedHeapError = (uint32_t)dc32OjHeapLastError();

	if (gOjFatalReady)
		longjmp(gOjFatalJump, 1);
	__builtin_trap();
}

extern "C" void dc32OjFatalAllocation(size_t size)
{
	ojFatalAlloc(size);
}

void *operator new(size_t size)
{
	void *ptr = dc32PortMalloc(size ? size : 1u);

	if (!ptr)
		ojFatalAlloc(size);
	return ptr;
}

void *operator new[](size_t size)
{
	void *ptr = dc32PortMalloc(size ? size : 1u);

	if (!ptr)
		ojFatalAlloc(size);
	return ptr;
}

void *operator new(size_t size, const std::nothrow_t&) noexcept
{
	return dc32PortMalloc(size ? size : 1u);
}

void *operator new[](size_t size, const std::nothrow_t&) noexcept
{
	return dc32PortMalloc(size ? size : 1u);
}

void operator delete(void *ptr) noexcept
{
	dc32PortFree(ptr);
}

void operator delete[](void *ptr) noexcept
{
	dc32PortFree(ptr);
}

void operator delete(void *ptr, size_t) noexcept
{
	dc32PortFree(ptr);
}

void operator delete[](void *ptr, size_t) noexcept
{
	dc32PortFree(ptr);
}

void operator delete(void *ptr, const std::nothrow_t&) noexcept
{
	dc32PortFree(ptr);
}

void operator delete[](void *ptr, const std::nothrow_t&) noexcept
{
	dc32PortFree(ptr);
}

extern "C" void __cxa_pure_virtual(void)
{
	ojFatalAlloc(0);
}

static void ojCallConstructors(OjCtor *begin, OjCtor *end)
{
	for (OjCtor *ctor = begin; ctor < end; ++ctor) {
		if (*ctor)
			(*ctor)();
	}
}

static void ojInitCppRuntime()
{
	static bool initialized;

	if (initialized)
		return;
	ojCallConstructors(__preinit_array_start, __preinit_array_end);
	ojCallConstructors(__init_array_start, __init_array_end);
	initialized = true;
}

extern "C" int dcAppEntry(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	int ret;

	if (!host || !host->displayFb || !host->uiKeysRaw)
		return -1;
	{
		struct ToolWorkspaceSpan scratch = {};
		const char *reason = NULL;

		if (!dcAppGetActiveScratch(&scratch) || !scratch.ptr)
			reason = "Scratch missing";
		else if (scratch.size < DC32_OJ_TRANSIENT_MIN)
			reason = "Scratch too small";
		else if (!dc32OjHeapInit(DC32_PORT_OPENJAZZ_AUX_START,
				DC32_PORT_OPENJAZZ_AUX_SIZE,
				scratch.ptr, scratch.size))
			reason = "Heap init failed";
		if (reason) {
			char detail[48];

			snprintf(detail, sizeof(detail), "Have %u; need %u",
				(unsigned int)scratch.size,
				(unsigned int)DC32_OJ_TRANSIENT_MIN);
			dc32OjShowMessage(host, args, "OpenJazz memory error",
				reason, detail, true);
			return -1;
		}
		dc32OjHeapSelect(DC32_OJ_HEAP_PERSISTENT);
	}
	if (setjmp(gOjFatalJump)) {
		char detail[72];

		gOjFatalReady = false;
		dc32OjCacheClose();
		dc32OjPackClose();
		if (host->log)
			host->log("OpenJazz fatal memory failure: peak=%u used=%u free=%u block=%u request=%u heap=%s asset=%s\n",
				dc32PortHeapPeakBytesUsed(), dc32PortHeapBytesUsed(), dc32PortHeapBytesFree(),
				gOjFailedLargestBlock, (uint32_t)gOjFailedAllocSize,
				gOjFailedHeapError ? dc32OjHeapErrorName(
					(enum Dc32OjHeapError)gOjFailedHeapError) : "none",
				dc32OjLoadingContext());
		if (gOjFailedHeapError)
			snprintf(detail, sizeof(detail), "Heap: %s",
				dc32OjHeapErrorName(
					(enum Dc32OjHeapError)gOjFailedHeapError));
		else
			snprintf(detail, sizeof(detail), "Need %u; block %u; free %u",
				(unsigned int)gOjFailedAllocSize,
				(unsigned int)gOjFailedLargestBlock,
				(unsigned int)dc32PortHeapBytesFree());
		dc32OjShowMessage(host, args,
			gOjFailedHeapError ? "Memory corruption" : "Not enough memory",
			dc32OjLoadingContext(), detail, true);
		return -1;
	}
	gOjFatalReady = true;
	ojInitCppRuntime();
	dc32OjSdlSetHost(host, args);
	dc32OjLoadingStart(host, args, "Checking game data");
	if (!args || !args->vol || !dc32OjPrepareData(host, args)) {
		dc32OjLoadingFinish();
		gOjFatalReady = false;
		return -1;
	}
	if (!dc32OjCachePrepare(host, args)) {
		dc32OjLoadingFinish();
		gOjFatalReady = false;
		dc32OjPackClose();
		return -1;
	}
	dc32OjLoadingStage("Starting engine");
	if (host->log)
		host->log("OpenJazz starting: heap used=%u free=%u\n",
			dc32PortHeapBytesUsed(), dc32PortHeapBytesFree());
	ret = openjazz_main(0, NULL);
	if (ret < 0 && dc32OjCacheLastError()[0]) {
		if (host->log)
			host->log("OpenJazz cache failure: %s asset=%s\n",
				dc32OjCacheLastError(), dc32OjLoadingContext());
		dc32OjShowMessage(host, args, "OpenJazz cache failed",
			dc32OjLoadingContext(), dc32OjCacheLastError(), true);
	}
	if (host->log)
		host->log("OpenJazz stopped: heap peak=%u used=%u free=%u\n",
			dc32PortHeapPeakBytesUsed(), dc32PortHeapBytesUsed(), dc32PortHeapBytesFree());
	dc32OjCacheClose();
	dc32OjPackClose();
	gOjFatalReady = false;
	return ret;
}

extern "C" void dcAppAbort(void)
{
	dc32OjSdlRequestQuit();
}

extern "C" void dcAppRefreshDisplayOptions(void)
{
}
