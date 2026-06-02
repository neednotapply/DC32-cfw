#include "arduboy/arduboy.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

extern "C" {
#include "audioPwm.h"
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "memMap.h"
#include "printf.h"
#include "qspi.h"
#include "timebase.h"
#include "ui.h"

#include "avr_eeprom.h"
#include "avr_ioport.h"
#include "avr_spi.h"
#include "sim_avr.h"
#include "ssd1306_virt.h"
}

#include "miniz.h"

#define ARDUBOY_CPU_FREQ 16000000u
#define ARDUBOY_FRAME_CYCLES (ARDUBOY_CPU_FREQ / ARDUBOY_FRAME_RATE)
#define ARDUBOY_BOOT_FAST_FRAME_CYCLES (ARDUBOY_FRAME_CYCLES * 16u)
#define ARDUBOY_SIM_RUN_CYCLE_LIMIT 65536u
#define ARDUBOY_SIM_CONTROL_POLL_RUNS 64u
#define ARDUBOY_SIM_CONTROL_POLL_TICKS (TICKS_PER_SECOND / 30u)
#define ARDUBOY_EEPROM_SYNC_TICKS TICKS_PER_SECOND
#define ARDUBOY_BOOT_WATCHDOG_TICKS ((TICKS_PER_SECOND * 3u) / 2u)
#define ARDUBOY_BOOT_LOG_TICKS TICKS_PER_SECOND
#define ARDUBOY_ZIP_LOCAL_SIG 0x04034b50u
#define ARDUBOY_ZIP_CENTRAL_SIG 0x02014b50u
#define ARDUBOY_ZIP_EOCD_SIG 0x06054b50u
#define ARDUBOY_ZIP_METHOD_STORED 0u
#define ARDUBOY_ZIP_METHOD_DEFLATE 8u
#define ARDUBOY_ZIP_GP_ENCRYPTED 0x0001u
#define ARDUBOY_ZIP64_SENTINEL 0xffffffffu
#define ARDUBOY_FLASH_WRITE_CHUNK QSPI_WRITE_GRANULARITY
#define ARDUBOY_PRESENT_INVALID 0xffffu
#define ARDUBOY_PRESENT_Y_INVALID 0xffu
#define ARDUBOY_PRESENT_STATE_INVALID 0xffu
#define ARDUBOY_PRESENT_STATE_DISPLAY_ON 0x01u
#define ARDUBOY_PRESENT_STATE_INVERTED 0x02u
#define ARDUBOY_DEBUG_LOGICAL_WIDTH DISP_HEIGHT
#define ARDUBOY_DEBUG_LOGICAL_HEIGHT DISP_WIDTH
#define ARDUBOY_SIM_ALLOC_MAGIC 0x41525653u
#define ARDUBOY_SIM_ALLOC_ALIGN 8u
#ifndef ARDUBOY_SIM_SCRATCH_SIZE
#define ARDUBOY_SIM_SCRATCH_SIZE 0x18000u
#endif
#define ARDUBOY_SIM_ARENA_LOW_BYTES 8192u

#if ARDUBOY_SIM_SCRATCH_SIZE <= ARDUBOY_SAVE_RAM_SIZE
#error "ARDUBOY_SIM_SCRATCH_SIZE must leave room for EEPROM save RAM"
#endif

#define AVR_PINB 0x23u
#define AVR_PINE 0x2cu
#define AVR_PINF 0x2fu

struct ArduboyZipMember {
	uint32_t dataOffset;
	uint32_t compressedSize;
	uint32_t uncompressedSize;
	uint16_t method;
};

struct ArduboyFlashWriter {
	uint32_t addr;
	uint32_t pos;
	uint32_t maxSize;
	uint32_t fill;
	uint8_t *buf;
};

struct ArduboySimAllocHeader {
	uint32_t magic;
	uint32_t size;
	uint32_t offset;
	uint32_t total;
};

struct ArduboySimRuntime {
	avr_t *cpu;
	ssd1306_t *screen;
	uint8_t *saveRam;
	uint32_t saveRamSize;
	uint16_t *presentSrcX;
	uint8_t *presentSrcYPage;
	uint8_t *presentSrcYMask;
	uint8_t *presentSrcYPageFlipped;
	uint8_t *presentSrcYMaskFlipped;
	uint8_t *arenaBase;
	uint32_t arenaSize;
	uint32_t arenaPos;
	uint32_t arenaHighWater;
	uint32_t arenaInitHighWater;
	bool arenaOverflow;
	bool abortRequested;
	bool failed;
	int failState;
	uint64_t startTicks;
	uint64_t lastEepromSyncTicks;
	uint64_t lastBootLogTicks;
	uint64_t bootWatchdogLastTicks;
	uint64_t lastControlPollTicks;
	uint64_t lastFrameTicks;
	uint64_t frameCount;
	uint64_t runCalls;
	uint64_t runCycles;
	avr_flashaddr_t bootWatchdogLastPc;
	avr_cycle_count_t bootWatchdogLastCycle;
	uint32_t bootWatchdogLastSpiWrites;
	uint32_t zeroCycleRuns;
	uint8_t bootWatchdogStalls;
	bool displayEverOn;
	uint32_t visibleFrames;
	uint32_t spiWrites;
	uint32_t commandWrites;
	uint32_t dataWrites;
	uint32_t allocFailures;
	uint32_t corruptPointers;
	uint8_t lastSpiByte;
	const char *initStage;
	const char *allocFailureWhere;
};

static ArduboySimRuntime mSim;
static uint16_t mPresentRowFirst, mPresentRowEnd;
static uint16_t mPresentColFirst, mPresentColEnd;
static uint8_t mPresentLastState = ARDUBOY_PRESENT_STATE_INVALID;

extern "C" void *arduboySimMalloc(size_t size);
extern "C" void *arduboySimCalloc(size_t count, size_t size);
extern "C" void *arduboySimRealloc(void *ptr, size_t size);
extern "C" void arduboySimFree(void *ptr);
extern "C" int arduboySimPrintf(const char *fmt, ...);
extern "C" int arduboySimFprintf(FILE *file, const char *fmt, ...);
extern "C" void arduboySimAbort(void);
extern "C" void arduboySimExit(int code);
extern "C" int arduboySimUsleep(unsigned usec);
extern "C" int arduboySimGetPid(void);
extern "C" long arduboySimRandom(void);
extern "C" int arduboySimClockGettime(clockid_t clkId, struct timespec *tp);
extern "C" int arduboySimGettimeofday(struct timeval *tv, void *tz);
extern "C" FILE *arduboySimFopen(const char *path, const char *mode);
extern "C" int arduboySimFclose(FILE *file);
extern "C" int arduboySimFflush(FILE *file);
extern "C" char *arduboySimStrdup(const char *str);
extern "C" int arduboySimPtrInArena(const void *ptr);
extern "C" int arduboySimPtrRangeInArena(const void *ptr, uint32_t bytes);
extern "C" void arduboySimAllocationFailure(const char *where);

static void arduboyPrvTrackArenaInitHighWater(void)
{
	if (mSim.arenaInitHighWater < mSim.arenaHighWater)
		mSim.arenaInitHighWater = mSim.arenaHighWater;
}

static uint16_t arduboyPrvRd16(const uint8_t *p)
{
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t arduboyPrvRd32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void arduboyPrvSetInitStage(const char *stage)
{
	mSim.initStage = stage;
	arduboyPrvTrackArenaInitHighWater();
	pr("arduboy sim init enter: %s arena=%lu/%lu allocFail=%lu corrupt=%lu\n",
		stage ? stage : "?",
		(unsigned long)mSim.arenaHighWater, (unsigned long)mSim.arenaSize,
		(unsigned long)mSim.allocFailures, (unsigned long)mSim.corruptPointers);
}

static void arduboyPrvLogInitStageUsage(const char *stage)
{
	uint32_t freeBytes = mSim.arenaSize > mSim.arenaPos ? mSim.arenaSize - mSim.arenaPos : 0;

	arduboyPrvTrackArenaInitHighWater();
	pr("arduboy sim init done: %s arena=%lu/%lu free=%lu pos=%lu initMax=%lu allocFail=%lu corrupt=%lu last=%s\n",
		stage ? stage : "?",
		(unsigned long)mSim.arenaHighWater, (unsigned long)mSim.arenaSize,
		(unsigned long)freeBytes, (unsigned long)mSim.arenaPos,
		(unsigned long)mSim.arenaInitHighWater,
		(unsigned long)mSim.allocFailures, (unsigned long)mSim.corruptPointers,
		mSim.allocFailureWhere ? mSim.allocFailureWhere : "-");
	if (stage && !strcmp(stage, "READY") && freeBytes < ARDUBOY_SIM_ARENA_LOW_BYTES)
		pr("arduboy sim warning: ARENA LOW free=%lu threshold=%lu scratch=%lu\n",
			(unsigned long)freeBytes, (unsigned long)ARDUBOY_SIM_ARENA_LOW_BYTES,
			(unsigned long)ARDUBOY_SIM_SCRATCH_SIZE);
}

static bool arduboyPrvEndsWithNoCase(const char *str, uint32_t len, const char *suffix)
{
	uint32_t suffixLen = 0;

	while (suffix[suffixLen])
		suffixLen++;
	if (len < suffixLen)
		return false;
	str += len - suffixLen;
	for (uint32_t i = 0; i < suffixLen; i++) {
		char a = str[i], b = suffix[i];
		if (a >= 'A' && a <= 'Z')
			a = (char)(a - 'A' + 'a');
		if (b >= 'A' && b <= 'Z')
			b = (char)(b - 'A' + 'a');
		if (a != b)
			return false;
	}
	return true;
}

static int arduboyPrvHexNibble(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	return -1;
}

static bool arduboyPrvHexByte(const char *src, uint8_t *byteP)
{
	int hi = arduboyPrvHexNibble(src[0]);
	int lo = arduboyPrvHexNibble(src[1]);

	if (hi < 0 || lo < 0)
		return false;
	*byteP = (uint8_t)((hi << 4) | lo);
	return true;
}

typedef bool (*ArduboyHexDataF)(uint32_t addr, const uint8_t *data, uint32_t len, void *userData);

static bool arduboyPrvHexParse(const void *rom, uint32_t size, ArduboyHexDataF dataF,
	void *userData, uint32_t *flashUsedP)
{
	const char *bytes = (const char*)rom;
	uint32_t pos = 0, base = 0, flashUsed = 0;

	if (!bytes || !size)
		return false;
	while (pos < size) {
		uint32_t lineStart, lineEnd;
		uint8_t count, addrHi, addrLo, type, checksum, data[255];
		uint32_t addr, sum;

		while (pos < size && (bytes[pos] == '\r' || bytes[pos] == '\n' || bytes[pos] == ' ' || bytes[pos] == '\t'))
			pos++;
		if (pos >= size)
			break;
		if (bytes[pos++] != ':')
			return false;
		lineStart = pos;
		while (pos < size && bytes[pos] != '\r' && bytes[pos] != '\n')
			pos++;
		lineEnd = pos;
		if (lineEnd - lineStart < 10)
			return false;
		if (!arduboyPrvHexByte(bytes + lineStart, &count) ||
			!arduboyPrvHexByte(bytes + lineStart + 2, &addrHi) ||
			!arduboyPrvHexByte(bytes + lineStart + 4, &addrLo) ||
			!arduboyPrvHexByte(bytes + lineStart + 6, &type))
			return false;
		if (lineEnd - lineStart != 10u + (uint32_t)count * 2u)
			return false;
		sum = count + addrHi + addrLo + type;
		for (uint32_t i = 0; i < count; i++) {
			if (!arduboyPrvHexByte(bytes + lineStart + 8 + i * 2, &data[i]))
				return false;
			sum += data[i];
		}
		if (!arduboyPrvHexByte(bytes + lineStart + 8 + count * 2, &checksum))
			return false;
		sum += checksum;
		if ((sum & 0xffu) != 0)
			return false;

		addr = ((uint32_t)addrHi << 8) | addrLo;
		if (type == 0) {
			uint32_t absolute = base + addr;

			if (absolute > ARDUBOY_FLASH_SIZE || count > ARDUBOY_FLASH_SIZE - absolute)
				return false;
			if (dataF && !dataF(absolute, data, count, userData))
				return false;
			if (flashUsed < absolute + count)
				flashUsed = absolute + count;
		}
		else if (type == 1) {
			if (flashUsedP)
				*flashUsedP = flashUsed;
			return flashUsed != 0;
		}
		else if (type == 2) {
			if (count != 2)
				return false;
			base = (((uint32_t)data[0] << 8) | data[1]) << 4;
		}
		else if (type == 4) {
			if (count != 2)
				return false;
			base = (((uint32_t)data[0] << 8) | data[1]) << 16;
		}
		else {
			return false;
		}
	}
	if (flashUsedP)
		*flashUsedP = flashUsed;
	return flashUsed != 0;
}

static bool arduboyPrvLoadHexData(uint32_t addr, const uint8_t *data, uint32_t len, void *userData)
{
	uint8_t *flash = (uint8_t*)userData;

	memcpy(flash + addr, data, len);
	return true;
}

static bool arduboyPrvLoadHexToAvrFlash(uint32_t addr, const uint8_t *data, uint32_t len, void *userData)
{
	avr_t *cpu = (avr_t*)userData;

	if (!cpu || !cpu->flash || addr > cpu->flashend + 1u || len > cpu->flashend + 1u - addr)
		return false;
	memcpy(cpu->flash + addr, data, len);
	return true;
}

static bool arduboyPrvFindZipHexMember(const void *packageData, uint32_t packageSize, ArduboyZipMember *memberP)
{
	const uint8_t *zip = (const uint8_t*)packageData;
	uint32_t eocd = UINT32_MAX, entries, centralSize, centralOffset, p;

	if (!zip || packageSize < 22)
		return false;
	for (p = packageSize - 22; p != UINT32_MAX; p--) {
		if (arduboyPrvRd32(zip + p) == ARDUBOY_ZIP_EOCD_SIG) {
			eocd = p;
			break;
		}
		if (!p || packageSize - p > 66000u)
			break;
	}
	if (eocd == UINT32_MAX)
		return false;
	entries = arduboyPrvRd16(zip + eocd + 10);
	centralSize = arduboyPrvRd32(zip + eocd + 12);
	centralOffset = arduboyPrvRd32(zip + eocd + 16);
	if (centralOffset == ARDUBOY_ZIP64_SENTINEL || centralSize == ARDUBOY_ZIP64_SENTINEL ||
		centralOffset > packageSize || centralSize > packageSize - centralOffset)
		return false;
	p = centralOffset;
	for (uint32_t i = 0; i < entries && p + 46 <= centralOffset + centralSize; i++) {
		const uint8_t *cent = zip + p;
		uint16_t flags, method, nameLen, extraLen, commentLen;
		uint32_t compressedSize, uncompressedSize, localOffset, localNameLen, localExtraLen;
		const char *name;

		if (arduboyPrvRd32(cent) != ARDUBOY_ZIP_CENTRAL_SIG)
			return false;
		flags = arduboyPrvRd16(cent + 8);
		method = arduboyPrvRd16(cent + 10);
		compressedSize = arduboyPrvRd32(cent + 20);
		uncompressedSize = arduboyPrvRd32(cent + 24);
		nameLen = arduboyPrvRd16(cent + 28);
		extraLen = arduboyPrvRd16(cent + 30);
		commentLen = arduboyPrvRd16(cent + 32);
		localOffset = arduboyPrvRd32(cent + 42);
		if (p + 46u + nameLen + extraLen + commentLen > centralOffset + centralSize)
			return false;
		name = (const char*)(cent + 46);
		if (!(flags & ARDUBOY_ZIP_GP_ENCRYPTED) &&
			(method == ARDUBOY_ZIP_METHOD_STORED || method == ARDUBOY_ZIP_METHOD_DEFLATE) &&
			compressedSize != ARDUBOY_ZIP64_SENTINEL && uncompressedSize != ARDUBOY_ZIP64_SENTINEL &&
			arduboyPrvEndsWithNoCase(name, nameLen, ".hex")) {
			const uint8_t *local;
			uint32_t dataOffset;

			if (localOffset > packageSize || packageSize - localOffset < 30)
				return false;
			local = zip + localOffset;
			if (arduboyPrvRd32(local) != ARDUBOY_ZIP_LOCAL_SIG)
				return false;
			localNameLen = arduboyPrvRd16(local + 26);
			localExtraLen = arduboyPrvRd16(local + 28);
			dataOffset = localOffset + 30u + localNameLen + localExtraLen;
			if (dataOffset > packageSize || compressedSize > packageSize - dataOffset)
				return false;
			memberP->dataOffset = dataOffset;
			memberP->compressedSize = compressedSize;
			memberP->uncompressedSize = uncompressedSize;
			memberP->method = method;
			return true;
		}
		p += 46u + nameLen + extraLen + commentLen;
	}
	return false;
}

static bool arduboyPrvLooksLikePackage(const void *rom, uint32_t size)
{
	const uint8_t *bytes = (const uint8_t*)rom;

	return bytes && size >= 4 && arduboyPrvRd32(bytes) == ARDUBOY_ZIP_LOCAL_SIG;
}

bool arduboyAnalyzeRom(const void *rom, uint32_t size, ArduboyRomInfo *info)
{
	uint32_t flashUsed = 0;
	bool isPackage = false;

	if (arduboyPrvLooksLikePackage(rom, size)) {
		ArduboyZipMember member;

		isPackage = true;
		if (!arduboyPrvFindZipHexMember(rom, size, &member))
			return false;
		flashUsed = member.uncompressedSize;
	}
	else if (!arduboyPrvHexParse(rom, size, NULL, NULL, &flashUsed))
		return false;
	if (info) {
		memset(info, 0, sizeof(*info));
		memcpy(info->name, "ARDUBOY", 8);
		info->romSize = size;
		info->saveRamSize = ARDUBOY_SAVE_RAM_SIZE;
		info->flashUsed = flashUsed;
		info->isPackage = isPackage;
	}
	return true;
}

static bool arduboyPrvFlashWriterFlush(ArduboyFlashWriter *writer, bool final)
{
	if (!writer->fill)
		return true;
	if (!final && writer->fill < ARDUBOY_FLASH_WRITE_CHUNK)
		return true;
	if (writer->fill < ARDUBOY_FLASH_WRITE_CHUNK)
		memset(writer->buf + writer->fill, 0, ARDUBOY_FLASH_WRITE_CHUNK - writer->fill);
	if (!flashWrite(writer->addr + writer->pos, 0, writer->buf, ARDUBOY_FLASH_WRITE_CHUNK))
		return false;
	writer->pos += ARDUBOY_FLASH_WRITE_CHUNK;
	writer->fill = 0;
	return true;
}

static bool arduboyPrvFlashWriterWrite(ArduboyFlashWriter *writer, const uint8_t *data, uint32_t len)
{
	while (len) {
		uint32_t now = ARDUBOY_FLASH_WRITE_CHUNK - writer->fill;

		if (writer->pos + writer->fill >= writer->maxSize)
			return false;
		if (now > len)
			now = len;
		if (now > writer->maxSize - writer->pos - writer->fill)
			now = writer->maxSize - writer->pos - writer->fill;
		memcpy(writer->buf + writer->fill, data, now);
		writer->fill += now;
		data += now;
		len -= now;
		if (writer->fill == ARDUBOY_FLASH_WRITE_CHUNK && !arduboyPrvFlashWriterFlush(writer, false))
			return false;
	}
	return true;
}

bool arduboyExtractPackageToFlash(const void *packageData, uint32_t packageSize,
	uint32_t flashAddr, uint32_t maxSize, void *inflateDict, uint32_t inflateDictSize,
	void *writeBuf, uint32_t writeBufSize, uint32_t *hexSizeP)
{
	ArduboyZipMember member;
	const uint8_t *packageBytes = (const uint8_t*)packageData;
	ArduboyFlashWriter writer;

	if (!packageBytes || !writeBuf || writeBufSize < ARDUBOY_FLASH_WRITE_CHUNK)
		return false;
	if (!arduboyPrvFindZipHexMember(packageData, packageSize, &member))
		return false;
	if (member.uncompressedSize > maxSize)
		return false;

	memset(&writer, 0, sizeof(writer));
	writer.addr = flashAddr;
	writer.maxSize = maxSize;
	writer.buf = (uint8_t*)writeBuf;

	if (member.method == ARDUBOY_ZIP_METHOD_STORED) {
		if (!arduboyPrvFlashWriterWrite(&writer, packageBytes + member.dataOffset, member.uncompressedSize))
			return false;
	}
	else {
		size_t outSize;

		if (!inflateDict || inflateDictSize < 32768u)
			return false;
		outSize = tinfl_decompress_mem_to_mem(inflateDict, inflateDictSize,
			packageBytes + member.dataOffset, member.compressedSize,
			TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
		if (outSize == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED || outSize != member.uncompressedSize)
			return false;
		if (!arduboyPrvFlashWriterWrite(&writer, (const uint8_t*)inflateDict, (uint32_t)outSize))
			return false;
	}

	if (!arduboyPrvFlashWriterFlush(&writer, true))
		return false;
	if (hexSizeP)
		*hexSizeP = member.uncompressedSize;
	return true;
}

static void *arduboyPrvArenaAlloc(uint32_t size, uint32_t align)
{
	uintptr_t base = (uintptr_t)mSim.arenaBase;
	uint32_t pos;
	void *ret;

	if (align == 0 || (align & (align - 1u)) != 0)
		align = 4;
	pos = (uint32_t)(((base + mSim.arenaPos + align - 1u) & ~(uintptr_t)(align - 1u)) - base);
	size = (size + 7u) & ~7u;
	if (!mSim.arenaBase || pos > mSim.arenaSize || size > mSim.arenaSize - pos) {
		mSim.arenaOverflow = true;
		return NULL;
	}
	ret = mSim.arenaBase + pos;
	mSim.arenaPos = pos + size;
	if (mSim.arenaHighWater < mSim.arenaPos)
		mSim.arenaHighWater = mSim.arenaPos;
	return ret;
}

static bool arduboyPrvPtrInArenaRange(const void *ptr, uint32_t bytes)
{
	uintptr_t p = (uintptr_t)ptr;
	uintptr_t base = (uintptr_t)mSim.arenaBase;
	uintptr_t end = base + mSim.arenaSize;

	if (!mSim.arenaBase || p < base || p > end)
		return false;
	return bytes <= end - p;
}

static ArduboySimAllocHeader *arduboyPrvAllocHeaderFromPayload(void *ptr)
{
	ArduboySimAllocHeader *hdr;

	if (!ptr || (((uintptr_t)ptr) & (ARDUBOY_SIM_ALLOC_ALIGN - 1u)))
		return NULL;
	if (!arduboyPrvPtrInArenaRange(ptr, 1))
		return NULL;
	hdr = (ArduboySimAllocHeader*)((uint8_t*)ptr - sizeof(ArduboySimAllocHeader));
	if (!arduboyPrvPtrInArenaRange(hdr, sizeof(*hdr)) || hdr->magic != ARDUBOY_SIM_ALLOC_MAGIC)
		return NULL;
	if (!hdr->total || hdr->total > mSim.arenaSize || hdr->offset > mSim.arenaSize - hdr->total ||
		hdr->size > hdr->total - sizeof(*hdr))
		return NULL;
	if ((uint8_t*)mSim.arenaBase + hdr->offset != (uint8_t*)hdr)
		return NULL;
	return hdr;
}

static bool arduboyPrvAllocIsArenaTop(const ArduboySimAllocHeader *hdr)
{
	return hdr && hdr->offset <= mSim.arenaSize && hdr->total <= mSim.arenaSize - hdr->offset &&
		hdr->offset + hdr->total == mSim.arenaPos;
}

static bool arduboyPrvAllocRuntimeBuffers(void)
{
	mSim.presentSrcX = (uint16_t*)arduboyPrvArenaAlloc(sizeof(uint16_t) * DISP_HEIGHT, 4);
	mSim.presentSrcYPage = (uint8_t*)arduboyPrvArenaAlloc(DISP_WIDTH, 4);
	mSim.presentSrcYMask = (uint8_t*)arduboyPrvArenaAlloc(DISP_WIDTH, 4);
	mSim.presentSrcYPageFlipped = (uint8_t*)arduboyPrvArenaAlloc(DISP_WIDTH, 4);
	mSim.presentSrcYMaskFlipped = (uint8_t*)arduboyPrvArenaAlloc(DISP_WIDTH, 4);
	return mSim.presentSrcX && mSim.presentSrcYPage && mSim.presentSrcYMask &&
		mSim.presentSrcYPageFlipped && mSim.presentSrcYMaskFlipped;
}

extern "C" void *arduboySimMalloc(size_t size)
{
	uint32_t payloadSize, total;
	ArduboySimAllocHeader *hdr;

	if (size > UINT32_MAX - sizeof(ArduboySimAllocHeader) - (ARDUBOY_SIM_ALLOC_ALIGN - 1u)) {
		arduboySimAllocationFailure("malloc/size");
		return NULL;
	}
	payloadSize = (uint32_t)size;
	total = sizeof(ArduboySimAllocHeader) + ((payloadSize + (ARDUBOY_SIM_ALLOC_ALIGN - 1u)) & ~(ARDUBOY_SIM_ALLOC_ALIGN - 1u));
	hdr = (ArduboySimAllocHeader*)arduboyPrvArenaAlloc(total, ARDUBOY_SIM_ALLOC_ALIGN);
	if (!hdr) {
		arduboySimAllocationFailure("malloc/arena");
		return NULL;
	}
	hdr->magic = ARDUBOY_SIM_ALLOC_MAGIC;
	hdr->size = payloadSize;
	hdr->offset = (uint32_t)((uint8_t*)hdr - mSim.arenaBase);
	hdr->total = total;
	return (uint8_t*)hdr + sizeof(*hdr);
}

extern "C" void *arduboySimCalloc(size_t count, size_t size)
{
	size_t total;
	void *ptr;

	if (size && count > SIZE_MAX / size)
		return NULL;
	total = count * size;
	ptr = arduboySimMalloc(total);
	if (ptr)
		memset(ptr, 0, total);
	return ptr;
}

extern "C" void *arduboySimRealloc(void *ptr, size_t size)
{
	ArduboySimAllocHeader *hdr;
	uint32_t payloadSize, newTotal;
	void *newPtr;

	if (!ptr)
		return arduboySimMalloc(size);
	hdr = arduboyPrvAllocHeaderFromPayload(ptr);
	if (!hdr) {
		mSim.corruptPointers++;
		arduboySimAllocationFailure("realloc/badptr");
		return NULL;
	}
	if (size > UINT32_MAX - sizeof(ArduboySimAllocHeader) - (ARDUBOY_SIM_ALLOC_ALIGN - 1u)) {
		arduboySimAllocationFailure("realloc/size");
		return NULL;
	}
	payloadSize = (uint32_t)size;
	newTotal = sizeof(ArduboySimAllocHeader) + ((payloadSize + (ARDUBOY_SIM_ALLOC_ALIGN - 1u)) & ~(ARDUBOY_SIM_ALLOC_ALIGN - 1u));
	if (arduboyPrvAllocIsArenaTop(hdr) && newTotal <= mSim.arenaSize - hdr->offset) {
		hdr->size = payloadSize;
		hdr->total = newTotal;
		mSim.arenaPos = hdr->offset + newTotal;
		if (mSim.arenaHighWater < mSim.arenaPos)
			mSim.arenaHighWater = mSim.arenaPos;
		return ptr;
	}
	newPtr = arduboySimMalloc(size);
	if (newPtr)
		memcpy(newPtr, ptr, hdr->size < size ? hdr->size : size);
	return newPtr;
}

extern "C" void arduboySimFree(void *ptr)
{
	ArduboySimAllocHeader *hdr;

	if (!ptr || !arduboyPrvPtrInArenaRange(ptr, 1))
		return;
	hdr = arduboyPrvAllocHeaderFromPayload(ptr);
	if (!hdr) {
		mSim.corruptPointers++;
		arduboySimAllocationFailure("free/badptr");
		return;
	}
	if (arduboyPrvAllocIsArenaTop(hdr)) {
		mSim.arenaPos = hdr->offset;
		hdr->magic = 0;
	}
}

extern "C" char *arduboySimStrdup(const char *str)
{
	size_t len;
	char *copy;

	if (!str)
		return NULL;
	len = strlen(str) + 1u;
	copy = (char*)arduboySimMalloc(len);
	if (copy)
		memcpy(copy, str, len);
	return copy;
}

extern "C" int arduboySimPtrInArena(const void *ptr)
{
	return arduboyPrvPtrInArenaRange(ptr, 1) ? 1 : 0;
}

extern "C" int arduboySimPtrRangeInArena(const void *ptr, uint32_t bytes)
{
	return arduboyPrvPtrInArenaRange(ptr, bytes) ? 1 : 0;
}

extern "C" void arduboySimAllocationFailure(const char *where)
{
	mSim.allocFailures++;
	mSim.allocFailureWhere = where;
	mSim.arenaOverflow = true;
	pr("arduboy sim alloc failure: %s arena=%lu/%lu corrupt=%lu\n",
		where ? where : "?",
		(unsigned long)mSim.arenaHighWater, (unsigned long)mSim.arenaSize,
		(unsigned long)mSim.corruptPointers);
}

extern "C" int arduboySimPrintf(const char *fmt, ...)
{
	(void)fmt;
	return 0;
}

extern "C" int arduboySimFprintf(FILE *file, const char *fmt, ...)
{
	(void)file;
	(void)fmt;
	return 0;
}

extern "C" void arduboySimAbort(void)
{
	mSim.failed = true;
	mSim.failState = cpu_Crashed;
	mSim.abortRequested = true;
}

extern "C" void arduboySimExit(int code)
{
	(void)code;
	arduboySimAbort();
}

extern "C" int arduboySimUsleep(unsigned usec)
{
	(void)usec;
	return 0;
}

extern "C" int arduboySimGetPid(void)
{
	return 1;
}

extern "C" long arduboySimRandom(void)
{
	return 0x1234;
}

extern "C" int arduboySimClockGettime(clockid_t clkId, struct timespec *tp)
{
	uint64_t ticks = getTime();

	(void)clkId;
	if (!tp)
		return -1;
	tp->tv_sec = (time_t)(ticks / TICKS_PER_SECOND);
	tp->tv_nsec = (long)(((ticks % TICKS_PER_SECOND) * 1000000000ull) / TICKS_PER_SECOND);
	return 0;
}

extern "C" int arduboySimGettimeofday(struct timeval *tv, void *tz)
{
	uint64_t ticks = getTime();

	(void)tz;
	if (!tv)
		return -1;
	tv->tv_sec = (time_t)(ticks / TICKS_PER_SECOND);
	tv->tv_usec = (suseconds_t)(((ticks % TICKS_PER_SECOND) * 1000000ull) / TICKS_PER_SECOND);
	return 0;
}

extern "C" FILE *arduboySimFopen(const char *path, const char *mode)
{
	(void)path;
	(void)mode;
	return NULL;
}

extern "C" int arduboySimFclose(FILE *file)
{
	(void)file;
	return 0;
}

extern "C" int arduboySimFflush(FILE *file)
{
	(void)file;
	return 0;
}

static void arduboyPrvNoSleep(avr_t *avr, avr_cycle_count_t howLong)
{
	(void)avr;
	(void)howLong;
}

static void arduboyPrvSsdSpiMonitor(avr_irq_t *irq, uint32_t value, void *param)
{
	ArduboySimRuntime *rt = (ArduboySimRuntime*)param;

	(void)irq;
	rt->spiWrites++;
	rt->lastSpiByte = (uint8_t)value;
	if (rt->screen && !rt->screen->cs_pin) {
		if (rt->screen->di_pin == SSD1306_VIRT_DATA)
			rt->dataWrites++;
		else
			rt->commandWrites++;
	}
}

static void arduboyPrvSetButtons(ArduboySimRuntime *rt)
{
	uint_fast8_t keys = uiGetKeysRaw();

	if (!rt->cpu || !rt->cpu->data)
		return;
	rt->cpu->data[AVR_PINB] = (uint8_t)((rt->cpu->data[AVR_PINB] & 0xefu) | ((keys & KEY_BIT_B) ? 0u : 0x10u));
	rt->cpu->data[AVR_PINE] = (uint8_t)((rt->cpu->data[AVR_PINE] & 0xbfu) | ((keys & KEY_BIT_A) ? 0u : 0x40u));
	rt->cpu->data[AVR_PINF] = (uint8_t)((rt->cpu->data[AVR_PINF] & 0x0fu) |
		((keys & KEY_BIT_UP) ? 0u : 0x80u) |
		((keys & KEY_BIT_RIGHT) ? 0u : 0x40u) |
		((keys & KEY_BIT_LEFT) ? 0u : 0x20u) |
		((keys & KEY_BIT_DOWN) ? 0u : 0x10u));
}

static void arduboyPrvSyncEepromToSave(bool force)
{
	avr_eeprom_desc_t desc;
	uint64_t now = getTime();

	if (!mSim.cpu || !mSim.saveRam || mSim.saveRamSize < ARDUBOY_SAVE_RAM_SIZE)
		return;
	if (!force && now - mSim.lastEepromSyncTicks < ARDUBOY_EEPROM_SYNC_TICKS)
		return;
	memset(&desc, 0, sizeof(desc));
	desc.ee = mSim.saveRam;
	desc.offset = 0;
	desc.size = ARDUBOY_SAVE_RAM_SIZE;
	(void)avr_ioctl(mSim.cpu, AVR_IOCTL_EEPROM_GET, &desc);
	mSim.lastEepromSyncTicks = now;
}

static void arduboyPrvFill(uint16_t *dst, uint32_t count, uint16_t color)
{
	while (count--)
		*dst++ = color;
}

static void arduboyPrvUpdatePresentMap(void)
{
	uint32_t logicalW = DISP_HEIGHT;
	uint32_t logicalH = DISP_WIDTH;
	uint32_t dstW, dstH, dstX, dstY;

	if (!mSim.presentSrcX || !mSim.presentSrcYPage || !mSim.presentSrcYMask ||
		!mSim.presentSrcYPageFlipped || !mSim.presentSrcYMaskFlipped)
		return;
	if ((uint64_t)logicalW * ARDUBOY_DISPLAY_HEIGHT <= (uint64_t)logicalH * ARDUBOY_DISPLAY_WIDTH) {
		dstW = logicalW;
		dstH = (logicalW * ARDUBOY_DISPLAY_HEIGHT) / ARDUBOY_DISPLAY_WIDTH;
	}
	else {
		dstH = logicalH;
		dstW = (logicalH * ARDUBOY_DISPLAY_WIDTH) / ARDUBOY_DISPLAY_HEIGHT;
	}
	dstX = (logicalW - dstW) / 2u;
	dstY = (logicalH - dstH) / 2u;

	mPresentRowFirst = DISP_HEIGHT;
	mPresentRowEnd = 0;
	for (uint32_t y = 0; y < DISP_HEIGHT; y++) {
		uint32_t logicalX = y;
		uint16_t srcX = ARDUBOY_PRESENT_INVALID;

		if (logicalX >= dstX && logicalX < dstX + dstW) {
			srcX = (uint16_t)(((logicalX - dstX) * ARDUBOY_DISPLAY_WIDTH) / dstW);
			if (mPresentRowFirst == DISP_HEIGHT)
				mPresentRowFirst = (uint16_t)y;
			mPresentRowEnd = (uint16_t)(y + 1u);
		}
		mSim.presentSrcX[y] = srcX;
	}

	mPresentColFirst = DISP_WIDTH;
	mPresentColEnd = 0;
	for (uint32_t x = 0; x < DISP_WIDTH; x++) {
		uint32_t logicalY = DISP_WIDTH - 1u - x;
		uint8_t page = ARDUBOY_PRESENT_Y_INVALID, mask = 0;
		uint8_t pageFlipped = ARDUBOY_PRESENT_Y_INVALID, maskFlipped = 0;

		if (logicalY >= dstY && logicalY < dstY + dstH) {
			uint32_t srcY = ((logicalY - dstY) * ARDUBOY_DISPLAY_HEIGHT) / dstH;
			uint32_t srcYFlipped = ARDUBOY_DISPLAY_HEIGHT - 1u - srcY;

			page = (uint8_t)(srcY >> 3);
			mask = (uint8_t)(1u << (srcY & 7u));
			pageFlipped = (uint8_t)(srcYFlipped >> 3);
			maskFlipped = (uint8_t)(1u << (srcYFlipped & 7u));
			if (mPresentColFirst == DISP_WIDTH)
				mPresentColFirst = (uint16_t)x;
			mPresentColEnd = (uint16_t)(x + 1u);
		}
		mSim.presentSrcYPage[x] = page;
		mSim.presentSrcYMask[x] = mask;
		mSim.presentSrcYPageFlipped[x] = pageFlipped;
		mSim.presentSrcYMaskFlipped[x] = maskFlipped;
	}
	mPresentLastState = ARDUBOY_PRESENT_STATE_INVALID;
	memset(dispGetFb(), 0, DISP_WIDTH * DISP_HEIGHT * DISP_BPP / 8);
}

void arduboyRefreshDisplayOptions(void)
{
	dispSetFramerate(ARDUBOY_FRAME_RATE);
	arduboyPrvUpdatePresentMap();
}

static void arduboyPrvPresentFrame(bool force)
{
	uint16_t *fb = (uint16_t*)dispGetFb();
	bool displayOn, inverted, segmentRemap, comScanNormal;
	const uint8_t *srcYPage, *srcYMask;
	uint16_t onColor = 0xffffu, offColor = 0x0000u;
	uint8_t presentState;
	bool clearFrame, dirty;

	if (!mSim.screen || !mSim.presentSrcX || !mSim.presentSrcYPage || !mSim.presentSrcYMask ||
		!mSim.presentSrcYPageFlipped || !mSim.presentSrcYMaskFlipped)
		return;
	displayOn = ssd1306_get_flag(mSim.screen, SSD1306_FLAG_DISPLAY_ON) != 0;
	inverted = ssd1306_get_flag(mSim.screen, SSD1306_FLAG_DISPLAY_INVERTED) != 0;
	segmentRemap = ssd1306_get_flag(mSim.screen, SSD1306_FLAG_SEGMENT_REMAP_0) != 0;
	comScanNormal = ssd1306_get_flag(mSim.screen, SSD1306_FLAG_COM_SCAN_NORMAL) != 0;
	srcYPage = comScanNormal ? mSim.presentSrcYPageFlipped : mSim.presentSrcYPage;
	srcYMask = comScanNormal ? mSim.presentSrcYMaskFlipped : mSim.presentSrcYMask;
	presentState = (displayOn ? ARDUBOY_PRESENT_STATE_DISPLAY_ON : 0u) |
		(inverted ? ARDUBOY_PRESENT_STATE_INVERTED : 0u);
	clearFrame = force || presentState != mPresentLastState;
	dirty = ssd1306_get_flag(mSim.screen, SSD1306_FLAG_DIRTY) != 0;
	if (inverted) {
		onColor = 0x0000u;
		offColor = 0xffffu;
	}
	if (!clearFrame && !dirty)
		return;
	if (clearFrame || displayOn)
		dispPrvWaitForScanoutStart();
	if (clearFrame) {
		arduboyPrvFill(fb, DISP_WIDTH * DISP_HEIGHT, offColor);
		mPresentLastState = presentState;
	}
	if (!displayOn) {
		ssd1306_set_flag(mSim.screen, SSD1306_FLAG_DIRTY, 0);
		return;
	}

	for (uint32_t y = mPresentRowFirst; y < mPresentRowEnd; y++) {
		uint16_t *row = fb + y * DISP_WIDTH;
		uint16_t srcX = mSim.presentSrcX[y];

		if (srcX == ARDUBOY_PRESENT_INVALID)
			continue;
		if (segmentRemap)
			srcX = ARDUBOY_DISPLAY_WIDTH - 1u - srcX;

		for (uint32_t x = mPresentColFirst; x < mPresentColEnd; x++) {
			uint8_t page = srcYPage[x];

			row[x] = (page != ARDUBOY_PRESENT_Y_INVALID &&
				(mSim.screen->vram[page][srcX] & srcYMask[x])) ? onColor : offColor;
		}
	}
	ssd1306_set_flag(mSim.screen, SSD1306_FLAG_DIRTY, 0);
	if (displayOn && mSim.screen->nonzero_bytes)
		mSim.visibleFrames++;
}

static bool arduboyPrvHasDisplayTraffic(void)
{
	if (mSim.screen)
		return mSim.visibleFrames || mSim.screen->command_writes || mSim.screen->data_writes;
	return mSim.visibleFrames || mSim.commandWrites || mSim.dataWrites;
}

static bool arduboyPrvDisplayIsOn(void)
{
	return mSim.screen && ssd1306_get_flag(mSim.screen, SSD1306_FLAG_DISPLAY_ON) != 0;
}

static uint32_t arduboyPrvSpiWrites(void)
{
	return mSim.screen ? mSim.screen->spi_writes : mSim.spiWrites;
}

static uint32_t arduboyPrvCommandWrites(void)
{
	return mSim.screen ? mSim.screen->command_writes : mSim.commandWrites;
}

static uint32_t arduboyPrvDataWrites(void)
{
	return mSim.screen ? mSim.screen->data_writes : mSim.dataWrites;
}

static uint8_t arduboyPrvLastSpiByte(void)
{
	return mSim.screen ? mSim.screen->last_spi_byte : mSim.lastSpiByte;
}

static void arduboyPrvDebugPutPixel(uint16_t *fb, uint32_t x, uint32_t y, uint16_t color)
{
	if (!fb || x >= ARDUBOY_DEBUG_LOGICAL_WIDTH || y >= ARDUBOY_DEBUG_LOGICAL_HEIGHT)
		return;
	fb[x * DISP_WIDTH + (DISP_WIDTH - 1u - y)] = color;
}

static void arduboyPrvDebugFillRect(uint16_t *fb, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color)
{
	uint32_t xEnd = x + w, yEnd = y + h;

	if (xEnd > ARDUBOY_DEBUG_LOGICAL_WIDTH)
		xEnd = ARDUBOY_DEBUG_LOGICAL_WIDTH;
	if (yEnd > ARDUBOY_DEBUG_LOGICAL_HEIGHT)
		yEnd = ARDUBOY_DEBUG_LOGICAL_HEIGHT;
	for (uint32_t yy = y; yy < yEnd; yy++)
		for (uint32_t xx = x; xx < xEnd; xx++)
			arduboyPrvDebugPutPixel(fb, xx, yy, color);
}

static uint32_t arduboyPrvDebugDrawChar(uint16_t *fb, uint32_t x, uint32_t y, char ch, uint16_t color)
{
	FontGlyphInfo gi;

	if (!fontGetGlyphInfo(&gi, FontSmall, (unsigned char)ch))
		return 4;
	for (uint32_t yy = 0; yy < gi.height; yy++)
		for (uint32_t xx = 0; xx < gi.width; xx++)
			if (fontGetGlyphPixel(&gi, (uint_fast8_t)yy, (uint_fast8_t)xx))
				arduboyPrvDebugPutPixel(fb, x + xx, y + yy, color);
	return gi.width + 1u;
}

static void arduboyPrvDebugDrawText(uint16_t *fb, uint32_t x, uint32_t y, const char *text, uint16_t color)
{
	while (*text)
		x += arduboyPrvDebugDrawChar(fb, x, y, *text++, color);
}

static void arduboyPrvShowFailureScreen(const char *reason)
{
	uint16_t *fb = (uint16_t*)dispGetFb();
	char line[80];

	arduboyPrvDebugFillRect(fb, 0, 0, ARDUBOY_DEBUG_LOGICAL_WIDTH, ARDUBOY_DEBUG_LOGICAL_HEIGHT, 0x0000u);
	arduboyPrvDebugFillRect(fb, 0, 0, ARDUBOY_DEBUG_LOGICAL_WIDTH, 2, 0xf800u);
	arduboyPrvDebugFillRect(fb, 0, ARDUBOY_DEBUG_LOGICAL_HEIGHT - 2u, ARDUBOY_DEBUG_LOGICAL_WIDTH, 2, 0xf800u);
	arduboyPrvDebugFillRect(fb, 0, 0, 2, ARDUBOY_DEBUG_LOGICAL_HEIGHT, 0xf800u);
	arduboyPrvDebugFillRect(fb, ARDUBOY_DEBUG_LOGICAL_WIDTH - 2u, 0, 2, ARDUBOY_DEBUG_LOGICAL_HEIGHT, 0xf800u);
	arduboyPrvDebugDrawText(fb, 14, 14, "ARDUBOY SIM STOP", 0xffe0u);
	(void)sprintf(line, "REASON: %s", reason);
	arduboyPrvDebugDrawText(fb, 14, 34, line, 0xffffu);
	(void)sprintf(line, "PC:%05lx STATE:%d CYC:%08lx",
		mSim.cpu ? (unsigned long)mSim.cpu->pc : 0ul,
		mSim.cpu ? mSim.cpu->state : -1,
		mSim.cpu ? (unsigned long)mSim.cpu->cycle : 0ul);
	arduboyPrvDebugDrawText(fb, 14, 54, line, 0xffffu);
	(void)sprintf(line, "SPI:%lu CMD:%lu DATA:%lu LAST:%02x",
		(unsigned long)arduboyPrvSpiWrites(), (unsigned long)arduboyPrvCommandWrites(),
		(unsigned long)arduboyPrvDataWrites(), (unsigned)arduboyPrvLastSpiByte());
	arduboyPrvDebugDrawText(fb, 14, 74, line, 0xffffu);
	(void)sprintf(line, "SCRATCH:%lu SAVE:%u",
		(unsigned long)ARDUBOY_SIM_SCRATCH_SIZE, (unsigned)ARDUBOY_SAVE_RAM_SIZE);
	arduboyPrvDebugDrawText(fb, 14, 94, line, 0xffffu);
	(void)sprintf(line, "ARENA:%lu/%lu OV:%u",
		(unsigned long)mSim.arenaHighWater, (unsigned long)mSim.arenaSize,
		mSim.arenaOverflow ? 1u : 0u);
	arduboyPrvDebugDrawText(fb, 14, 114, line, 0xffffu);
	(void)sprintf(line, "FREE:%lu INITMAX:%lu POS:%lu",
		(unsigned long)(mSim.arenaSize > mSim.arenaPos ? mSim.arenaSize - mSim.arenaPos : 0),
		(unsigned long)mSim.arenaInitHighWater, (unsigned long)mSim.arenaPos);
	arduboyPrvDebugDrawText(fb, 14, 134, line, 0xffffu);
	(void)sprintf(line, "STAGE:%s AF:%lu BAD:%lu",
		mSim.initStage ? mSim.initStage : "?",
		(unsigned long)mSim.allocFailures, (unsigned long)mSim.corruptPointers);
	arduboyPrvDebugDrawText(fb, 14, 154, line, 0xffffu);
	(void)sprintf(line, "RUN:%lu AVG:%lu Z:%lu",
		(unsigned long)mSim.runCalls,
		(unsigned long)(mSim.runCalls ? (mSim.runCycles / mSim.runCalls) : 0u),
		(unsigned long)mSim.zeroCycleRuns);
	arduboyPrvDebugDrawText(fb, 14, 174, line, 0xffffu);
	if (mSim.allocFailureWhere) {
		(void)sprintf(line, "ALLOC:%s", mSim.allocFailureWhere);
		arduboyPrvDebugDrawText(fb, 14, 194, line, 0xffffu);
	}
	arduboyPrvDebugDrawText(fb, 14, 204, "CENTER: MENU  B: EXIT FROM MENU", 0x07e0u);
	dispPrvWaitForScanoutStart();
}

static bool arduboyPrvServiceRuntimeControls(void)
{
	if (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER) {
		arduboyPrvSyncEepromToSave(true);
		arduboyPortInGameMenu();
		while (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER)
			;
		return !mSim.abortRequested;
	}
	return true;
}

static bool arduboyPrvMaybeServiceRuntimeControls(uint32_t runGuard)
{
	uint64_t now;

	if ((runGuard & (ARDUBOY_SIM_CONTROL_POLL_RUNS - 1u)) != 0)
		return true;
	now = getTime();
	if (now - mSim.lastControlPollTicks < ARDUBOY_SIM_CONTROL_POLL_TICKS)
		return true;
	mSim.lastControlPollTicks = now;
	return arduboyPrvServiceRuntimeControls();
}

static void arduboyPrvFailureLoop(void)
{
	while (!mSim.abortRequested && mSim.failed) {
		if (!arduboyPrvServiceRuntimeControls())
			break;
		if (uiGetKeysRaw() & KEY_BIT_B)
			break;
	}
}

static bool arduboyPrvMaybeBootWatchdog(void)
{
	uint64_t now = getTime();
	avr_cycle_count_t cycle = mSim.cpu ? mSim.cpu->cycle : 0;
	avr_flashaddr_t pc = mSim.cpu ? mSim.cpu->pc : 0;

	if (arduboyPrvHasDisplayTraffic())
		return false;
	if (now - mSim.startTicks < ARDUBOY_BOOT_WATCHDOG_TICKS)
		return false;
	if (now - mSim.lastBootLogTicks < ARDUBOY_BOOT_LOG_TICKS)
		return false;
	if (cycle != mSim.bootWatchdogLastCycle ||
			pc != mSim.bootWatchdogLastPc ||
			arduboyPrvSpiWrites() != mSim.bootWatchdogLastSpiWrites) {
		uint64_t elapsed = now - mSim.bootWatchdogLastTicks;
		avr_cycle_count_t deltaCycles = cycle >= mSim.bootWatchdogLastCycle ? cycle - mSim.bootWatchdogLastCycle : 0;
		uint32_t cps = elapsed ? (uint32_t)((deltaCycles * TICKS_PER_SECOND) / elapsed) : 0;
		pr("arduboy sim: boot slow but progressing pc=%08lx state=%d cycle=%lu +%lu/s run=%lu avg=%lu spi=%lu cmd=%lu data=%lu arena=%lu/%lu free=%lu\n",
			(unsigned long)pc,
			mSim.cpu ? mSim.cpu->state : -1,
			(unsigned long)cycle, (unsigned long)cps,
			(unsigned long)mSim.runCalls,
			(unsigned long)(mSim.runCalls ? (mSim.runCycles / mSim.runCalls) : 0u),
			(unsigned long)arduboyPrvSpiWrites(), (unsigned long)arduboyPrvCommandWrites(),
			(unsigned long)arduboyPrvDataWrites(),
			(unsigned long)mSim.arenaHighWater, (unsigned long)mSim.arenaSize,
			(unsigned long)(mSim.arenaSize > mSim.arenaPos ? mSim.arenaSize - mSim.arenaPos : 0));
		mSim.bootWatchdogLastCycle = cycle;
		mSim.bootWatchdogLastPc = pc;
		mSim.bootWatchdogLastSpiWrites = arduboyPrvSpiWrites();
		mSim.bootWatchdogLastTicks = now;
		mSim.lastBootLogTicks = now;
		mSim.bootWatchdogStalls = 0;
		return false;
	}
	mSim.lastBootLogTicks = now;
	if (++mSim.bootWatchdogStalls < 3) {
		pr("arduboy sim: boot watchdog waiting for progress pc=%08lx state=%d cycle=%lu stall=%u spi=%lu cmd=%lu data=%lu\n",
			(unsigned long)pc,
			mSim.cpu ? mSim.cpu->state : -1,
			(unsigned long)cycle, mSim.bootWatchdogStalls,
			(unsigned long)arduboyPrvSpiWrites(), (unsigned long)arduboyPrvCommandWrites(),
			(unsigned long)arduboyPrvDataWrites());
		return false;
	}
	pr("arduboy sim: boot stalled pc=%08lx state=%d cycle=%lu run=%lu avg=%lu spi=%lu cmd=%lu data=%lu arena=%lu/%lu ov=%u\n",
		(unsigned long)pc,
		mSim.cpu ? mSim.cpu->state : -1,
		(unsigned long)cycle,
		(unsigned long)mSim.runCalls,
		(unsigned long)(mSim.runCalls ? (mSim.runCycles / mSim.runCalls) : 0u),
		(unsigned long)arduboyPrvSpiWrites(), (unsigned long)arduboyPrvCommandWrites(),
		(unsigned long)arduboyPrvDataWrites(),
		(unsigned long)mSim.arenaHighWater, (unsigned long)mSim.arenaSize,
		mSim.arenaOverflow ? 1u : 0u);
	arduboyPrvShowFailureScreen("BOOT STALL");
	mSim.failed = true;
	return true;
}

static void arduboyPrvPerfLog(void)
{
#ifdef ARDUBOY_PERF
	uint64_t now = getTime();
	if (now - mSim.lastBootLogTicks >= ARDUBOY_BOOT_LOG_TICKS) {
		uint64_t elapsed = now - mSim.startTicks;
		uint32_t ms = (uint32_t)((elapsed * 1000ull) / TICKS_PER_SECOND);
		uint32_t cps = elapsed ? (uint32_t)((mSim.cpu->cycle * TICKS_PER_SECOND) / elapsed) : 0;
		pr("arduboy sim perf: host=%lums guest=%lu/s frame=%lu run=%lu avg=%lu spi=%lu cmd=%lu data=%lu arena=%lu/%lu\n",
			(unsigned long)ms, (unsigned long)cps, (unsigned long)mSim.frameCount,
			(unsigned long)mSim.runCalls,
			(unsigned long)(mSim.runCalls ? (mSim.runCycles / mSim.runCalls) : 0u),
			(unsigned long)arduboyPrvSpiWrites(), (unsigned long)arduboyPrvCommandWrites(),
			(unsigned long)arduboyPrvDataWrites(), (unsigned long)mSim.arenaHighWater,
			(unsigned long)mSim.arenaSize);
		mSim.lastBootLogTicks = now;
	}
#else
	(void)mSim;
#endif
}

static bool arduboyPrvInitSim(void *saveRam, uint32_t saveRamSize)
{
	avr_eeprom_desc_t eeprom;
	avr_irq_t *spiIrq;
	int initRc;
	ssd1306_wiring_t wiring = {
		{ 'D', 6 },
		{ 'D', 4 },
		{ 'D', 7 },
	};

	arduboyPrvSetInitStage("AVR CORE");
	mSim.cpu = avr_make_mcu_by_name("atmega32u4");
	arduboyPrvLogInitStageUsage("AVR CORE");
	if (!mSim.cpu)
		return false;
	mSim.cpu->frequency = ARDUBOY_CPU_FREQ;
	arduboyPrvSetInitStage("AVR INIT");
	initRc = avr_init(mSim.cpu);
	arduboyPrvLogInitStageUsage("AVR INIT");
	if (initRc < 0)
		return false;
	if (!mSim.cpu->flash || !mSim.cpu->data || !mSim.cpu->irq || mSim.arenaOverflow)
		return false;
	mSim.cpu->frequency = ARDUBOY_CPU_FREQ;
	mSim.cpu->sleep = arduboyPrvNoSleep;
	mSim.cpu->run_cycle_limit = ARDUBOY_SIM_RUN_CYCLE_LIMIT;
	mSim.cpu->run_cycle_count = ARDUBOY_SIM_RUN_CYCLE_LIMIT;
	mSim.cpu->codeend = mSim.cpu->flashend;

	arduboyPrvSetInitStage("SSD1306 ALLOC");
	mSim.screen = (ssd1306_t*)arduboyPrvArenaAlloc(sizeof(*mSim.screen), 4);
	if (!mSim.screen)
		return false;
	arduboyPrvSetInitStage("SSD1306 INIT");
	ssd1306_init(mSim.cpu, mSim.screen, ARDUBOY_DISPLAY_WIDTH, ARDUBOY_DISPLAY_HEIGHT);
	arduboyPrvLogInitStageUsage("SSD1306 INIT");
	if (!mSim.screen->irq || mSim.arenaOverflow)
		return false;
	arduboyPrvSetInitStage("SSD1306 CONNECT");
	ssd1306_connect(mSim.screen, &wiring);
	arduboyPrvSetInitStage("SPI MONITOR");
	spiIrq = mSim.screen->irq + IRQ_SSD1306_SPI_BYTE_IN;
	if (!arduboySimPtrRangeInArena(spiIrq, sizeof(*spiIrq)))
		return false;
	avr_irq_register_notify(spiIrq, arduboyPrvSsdSpiMonitor, &mSim);
	arduboyPrvLogInitStageUsage("SPI MONITOR");
	if (mSim.arenaOverflow)
		return false;

	arduboyPrvSetInitStage("EEPROM IMPORT");
	memset(&eeprom, 0, sizeof(eeprom));
	eeprom.ee = (uint8_t*)saveRam;
	eeprom.offset = 0;
	eeprom.size = saveRamSize < ARDUBOY_SAVE_RAM_SIZE ? saveRamSize : ARDUBOY_SAVE_RAM_SIZE;
	if (eeprom.ee && eeprom.size)
		(void)avr_ioctl(mSim.cpu, AVR_IOCTL_EEPROM_SET, &eeprom);
	arduboyPrvSetInitStage("READY");
	arduboyPrvLogInitStageUsage("READY");
	return !mSim.arenaOverflow;
}

void arduboyAbort(void)
{
	mSim.abortRequested = true;
}

void arduboyRun(const void *rom, uint32_t romSize, void *saveRam, uint32_t saveRamSize)
{
	ArduboyRomInfo info;

	if (!arduboyAnalyzeRom(rom, romSize, &info) || info.isPackage)
		return;

	memset(&mSim, 0, sizeof(mSim));
	mSim.saveRam = (uint8_t*)saveRam;
	mSim.saveRamSize = saveRamSize;
	mSim.arenaBase = ((uint8_t*)CART_RAM_ADDR_IN_RAM) + ARDUBOY_SAVE_RAM_SIZE;
	mSim.arenaSize = ARDUBOY_SIM_SCRATCH_SIZE - ARDUBOY_SAVE_RAM_SIZE;
	mSim.startTicks = getTime();
	mSim.lastEepromSyncTicks = mSim.startTicks;
	mSim.lastBootLogTicks = mSim.startTicks;
	mSim.bootWatchdogLastTicks = mSim.startTicks;
	mSim.lastControlPollTicks = mSim.startTicks;
	memset(mSim.arenaBase, 0xff, mSim.arenaSize);
	audioPwmStop();
	dispPrvFrameCtrReset();

	if (!arduboyPrvInitSim(saveRam, saveRamSize) || mSim.arenaOverflow) {
		mSim.failed = true;
		arduboyPrvShowFailureScreen(mSim.allocFailureWhere ? mSim.allocFailureWhere : "INIT/ARENA");
		arduboyPrvFailureLoop();
		goto out;
	}
	arduboyPrvSetInitStage("PRESENTER MAPS");
	if (!arduboyPrvAllocRuntimeBuffers()) {
		mSim.failed = true;
		arduboyPrvShowFailureScreen(mSim.allocFailureWhere ? mSim.allocFailureWhere : "PRESENTER ARENA");
		arduboyPrvFailureLoop();
		goto out;
	}
	arduboyRefreshDisplayOptions();
	arduboyPrvSetInitStage("HEX LOAD");
	if (!arduboyPrvHexParse(rom, romSize, arduboyPrvLoadHexToAvrFlash, mSim.cpu, NULL)) {
		mSim.failed = true;
		arduboyPrvShowFailureScreen("HEX LOAD");
		arduboyPrvFailureLoop();
		goto out;
	}
	arduboyPrvSetInitStage("RUN");
	pr("arduboy sim: start arena=%lu bytes used=%lu\n",
		(unsigned long)mSim.arenaSize, (unsigned long)mSim.arenaHighWater);

	while (!mSim.abortRequested && !mSim.failed && mSim.cpu && mSim.cpu->state != cpu_Done && mSim.cpu->state != cpu_Crashed) {
		bool hadDisplayOn = mSim.displayEverOn;
		avr_cycle_count_t frameCycles = hadDisplayOn ? ARDUBOY_FRAME_CYCLES : ARDUBOY_BOOT_FAST_FRAME_CYCLES;
		avr_cycle_count_t frameEnd = mSim.cpu->cycle + frameCycles;
		uint32_t guard = 0;

		if (!arduboyPrvServiceRuntimeControls())
			break;
		arduboyPrvSetButtons(&mSim);
		while (!mSim.abortRequested && !mSim.failed && mSim.cpu->cycle < frameEnd) {
			avr_cycle_count_t beforeCycle;
			avr_cycle_count_t runCycles;
			int state;

			if (!arduboyPrvMaybeServiceRuntimeControls(guard++))
				break;
			beforeCycle = mSim.cpu->cycle;
			state = avr_run(mSim.cpu);
			runCycles = mSim.cpu->cycle >= beforeCycle ? mSim.cpu->cycle - beforeCycle : 0;
			mSim.runCalls++;
			mSim.runCycles += runCycles;
			if (!runCycles)
				mSim.zeroCycleRuns++;
			if (state == cpu_Done || state == cpu_Crashed) {
				mSim.failed = state == cpu_Crashed;
				mSim.failState = state;
				break;
			}
			if (arduboyPrvMaybeBootWatchdog())
				break;
			if (!hadDisplayOn && arduboyPrvDisplayIsOn()) {
				mSim.displayEverOn = true;
				break;
			}
		}
		arduboyPrvSyncEepromToSave(false);
		if (mSim.displayEverOn) {
			if (hadDisplayOn)
				dispPrvFrameCtrWait();
			else
				dispPrvFrameCtrReset();
			arduboyPrvPresentFrame(!hadDisplayOn);
		}
		mSim.frameCount++;
		arduboyPrvPerfLog();
	}

	if (mSim.cpu && mSim.cpu->state == cpu_Crashed)
		arduboyPrvShowFailureScreen("CPU CRASHED");
	arduboyPrvFailureLoop();

out:
	arduboyPrvSyncEepromToSave(true);
	audioPwmStop();
	if (mSim.cpu)
		avr_terminate(mSim.cpu);
	mSim.cpu = NULL;
	dispSetFramerate(ARDUBOY_FRAME_RATE);
}
