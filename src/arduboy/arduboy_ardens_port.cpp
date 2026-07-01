#include "arduboy/arduboy.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <new>

extern "C" {
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "memMap.h"
#include "printf.h"
#include "qspi.h"
#include "timebase.h"
#include "ui.h"
}

#include "miniz.h"
#include "absim.hpp"

#define ARDUBOY_CPU_FREQ 16000000u
#define ARDUBOY_FRAME_CYCLES (ARDUBOY_CPU_FREQ / ARDUBOY_FRAME_RATE)
#define ARDUBOY_CONTROL_POLL_CYCLES 4096u
#define ARDUBOY_EEPROM_SYNC_TICKS TICKS_PER_SECOND
#define ARDUBOY_BOOT_WATCHDOG_TICKS (TICKS_PER_SECOND * 5u)
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
#define ARDUBOY_AVR_PINB 0x23u
#define ARDUBOY_AVR_PINE 0x2cu
#define ARDUBOY_AVR_PINF 0x2fu
#define ARDUBOY_AVR_PORTD 0x2bu

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

enum ArduboyArdensFailReason {
	ArduboyArdensFailNone,
	ArduboyArdensFailInvalidRom,
	ArduboyArdensFailInvalidPc,
	ArduboyArdensFailBootWatchdog,
	ArduboyArdensFailNoDisplayIo,
};

struct ArduboyArdensRuntime {
	absim::atmega32u4_t cpu;
	absim::display_t display;
	uint8_t *saveRam;
	uint32_t saveRamSize;
	uint16_t presentSrcX[DISP_HEIGHT];
	uint8_t presentSrcYPage[DISP_WIDTH];
	uint8_t presentSrcYMask[DISP_WIDTH];
	uint8_t presentSrcYPageFlipped[DISP_WIDTH];
	uint8_t presentSrcYMaskFlipped[DISP_WIDTH];
	uint64_t startTicks;
	uint64_t lastBootLogTicks;
	uint64_t lastEepromSyncTicks;
	uint64_t lastFrameTicks;
	uint64_t frameCount;
	uint64_t executedCycles;
	uint64_t advanceCalls;
	uint32_t flashUsed;
	uint32_t spiWrites;
	uint32_t commandWrites;
	uint32_t dataWrites;
	uint32_t ignoredSpiWrites;
	uint32_t displayUpdates;
	uint32_t eepromSyncs;
	uint16_t lastPc;
	uint16_t lastOpcode;
	uint8_t lastSpiByte;
	uint8_t lastPortD;
	bool abortRequested;
	bool displayDirty;
	bool displayEverOn;
	bool visibleFrameSeen;
	bool failed;
	uint8_t failReason;
};

static_assert(sizeof(ArduboyArdensRuntime) + ARDUBOY_SAVE_RAM_SIZE <= 0x2b000u,
	"Ardens runtime must fit in the reserved Arduboy scratch window");

extern "C" const uint32_t gArduboyArdensRuntimeSize = sizeof(ArduboyArdensRuntime);

static ArduboyArdensRuntime *mArdensPtr;
static uint16_t mPresentRowFirst, mPresentRowEnd;
static uint16_t mPresentColFirst, mPresentColEnd;
static uint8_t mPresentLastState = ARDUBOY_PRESENT_STATE_INVALID;
static bool mPresentFlipped;

void arduboySetRotation(bool flipped)
{
	mPresentFlipped = flipped;
	mPresentLastState = ARDUBOY_PRESENT_STATE_INVALID;
	if (mArdensPtr)
		mArdensPtr->displayDirty = true;
}

static ArduboyArdensRuntime *arduboyPrvRuntime(void)
{
	if (!mArdensPtr) {
		uintptr_t p = (uintptr_t)CART_RAM_ADDR_IN_RAM + ARDUBOY_SAVE_RAM_SIZE;
		uintptr_t align = alignof(ArduboyArdensRuntime);

		p = (p + align - 1u) & ~(align - 1u);
		mArdensPtr = new ((void*)p) ArduboyArdensRuntime;
	}
	return mArdensPtr;
}

#define mArdens (*mArdensPtr)

static uint16_t arduboyPrvRd16(const uint8_t *p)
{
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t arduboyPrvRd32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t arduboyPrvArdensWord(uint16_t pc)
{
	uint32_t addr = (uint32_t)pc * 2u;

	if (addr + 1u >= mArdens.cpu.prog.size())
		return 0xffffu;
	return (uint16_t)mArdens.cpu.prog[addr] | ((uint16_t)mArdens.cpu.prog[addr + 1u] << 8);
}

static bool arduboyPrvRawDirectCallTarget(uint16_t pc, uint16_t *targetP, uint8_t *wordsP)
{
	uint16_t w0 = arduboyPrvArdensWord(pc);

	if ((w0 & 0xe000u) == 0xd000u) {
		int16_t off = (int16_t)(w0 & 0x0fffu);

		if (off & 0x0800)
			off |= (int16_t)0xf000;
		*targetP = (uint16_t)(pc + off + 1);
		if (wordsP)
			*wordsP = 1;
		return true;
	}
	if ((w0 & 0xfe0cu) == 0x940cu && (w0 & 0x0002u)) {
		*targetP = (uint16_t)(arduboyPrvArdensWord((uint16_t)(pc + 1u)) & 0x3fffu);
		if (wordsP)
			*wordsP = 2;
		return true;
	}
	return false;
}

static uint8_t arduboyPrvRawInstrWords(uint16_t pc)
{
	uint16_t w0 = arduboyPrvArdensWord(pc);

	if (((w0 & 0xfe0cu) == 0x940cu) ||
		((w0 & 0xfe0fu) == 0x9000u) ||
		((w0 & 0xfe0fu) == 0x9200u))
		return 2;
	return 1;
}

static bool arduboyPrvRawImm8(uint16_t pc, uint8_t *immP)
{
	uint16_t w0 = arduboyPrvArdensWord(pc);

	if ((w0 & 0xf000u) != 0xe000u && (w0 & 0xf000u) != 0x3000u)
		return false;
	*immP = (uint8_t)(((w0 >> 4) & 0xf0u) | (w0 & 0x0fu));
	return true;
}

static bool arduboyPrvRawLds(uint16_t pc, uint8_t *regP, uint16_t *addrP)
{
	uint16_t w0 = arduboyPrvArdensWord(pc);

	if ((w0 & 0xfe0fu) != 0x9000u)
		return false;
	if (regP)
		*regP = (uint8_t)((w0 >> 4) & 0x1fu);
	if (addrP)
		*addrP = arduboyPrvArdensWord((uint16_t)(pc + 1u));
	return true;
}

static bool arduboyPrvRawSts(uint16_t pc, uint8_t *regP, uint16_t *addrP)
{
	uint16_t w0 = arduboyPrvArdensWord(pc);

	if ((w0 & 0xfe0fu) != 0x9200u)
		return false;
	if (regP)
		*regP = (uint8_t)((w0 >> 4) & 0x1fu);
	if (addrP)
		*addrP = arduboyPrvArdensWord((uint16_t)(pc + 1u));
	return true;
}

static bool arduboyPrvRawBranchOrJump(uint16_t pc)
{
	uint16_t w0 = arduboyPrvArdensWord(pc);

	return (w0 & 0xe000u) == 0xc000u ||
		(w0 & 0xfc00u) == 0xf000u ||
		(w0 & 0xfc00u) == 0xf400u;
}

static bool arduboyPrvRawUnconditionalJumpTarget(uint16_t pc, uint16_t *targetP)
{
	uint16_t w0 = arduboyPrvArdensWord(pc);

	if ((w0 & 0xf000u) == 0xc000u) {
		int16_t off = (int16_t)(w0 & 0x0fffu);

		if (off & 0x0800)
			off |= (int16_t)0xf000;
		*targetP = (uint16_t)(pc + off + 1);
		return true;
	}
	if ((w0 & 0xfe0cu) == 0x940cu && !(w0 & 0x0002u)) {
		*targetP = (uint16_t)(arduboyPrvArdensWord((uint16_t)(pc + 1u)) & 0x3fffu);
		return true;
	}
	return false;
}

static uint8_t arduboyPrvScoreArduinoDelayTarget(uint16_t target, uint16_t *microsTargetP)
{
	uint16_t pc = target;
	uint16_t repeatedTarget = 0xffffu;
	uint16_t visited[48];
	uint8_t visitedCount = 0;
	uint8_t repeatedCalls = 0;
	uint8_t totalCalls = 0;
	bool hasE8 = false;
	bool has03 = false;
	bool hasBranch = false;

	if ((uint32_t)target * 2u >= mArdens.flashUsed)
		return 0;
	for (uint32_t n = 0; n < 160u && (uint32_t)pc * 2u < mArdens.flashUsed; n++) {
		uint16_t w0 = arduboyPrvArdensWord(pc);
		uint16_t callTarget;
		uint16_t jumpTarget;
		uint8_t words;
		uint8_t imm;
		bool seen = false;

		for (uint8_t i = 0; i < visitedCount; i++) {
			if (visited[i] == pc) {
				seen = true;
				break;
			}
		}
		if (seen)
			break;
		if (visitedCount < sizeof(visited) / sizeof(visited[0]))
			visited[visitedCount++] = pc;

		if (w0 == 0x9508u)
			break;
		if (w0 == 0xffffu)
			return 0;
		if (arduboyPrvRawImm8(pc, &imm)) {
			if (imm == 0xe8u)
				hasE8 = true;
			else if (imm == 0x03u)
				has03 = true;
		}
		if (arduboyPrvRawDirectCallTarget(pc, &callTarget, &words)) {
			totalCalls++;
			if (repeatedTarget == callTarget)
				repeatedCalls++;
			else if (repeatedCalls < 2u) {
				repeatedTarget = callTarget;
				repeatedCalls = 1;
			}
			pc = (uint16_t)(pc + words);
			continue;
		}
		if (arduboyPrvRawBranchOrJump(pc))
			hasBranch = true;
		if (arduboyPrvRawUnconditionalJumpTarget(pc, &jumpTarget)) {
			if ((uint32_t)jumpTarget * 2u >= mArdens.flashUsed)
				break;
			pc = jumpTarget;
			continue;
		}
		pc = (uint16_t)(pc + arduboyPrvRawInstrWords(pc));
	}

	if (!hasE8 || !has03 || repeatedCalls < 2u || totalCalls < 2u || !hasBranch)
		return 0;
	if (microsTargetP)
		*microsTargetP = repeatedTarget;
	return (uint8_t)(8u + (repeatedCalls > 3u ? 3u : repeatedCalls));
}

static uint16_t arduboyPrvDetectTimer0OverflowAddr(uint16_t microsPc)
{
	for (uint16_t pc = microsPc; (uint32_t)pc * 2u < mArdens.flashUsed; pc = (uint16_t)(pc + arduboyPrvRawInstrWords(pc))) {
		uint8_t reg0, reg1, reg2, reg3;
		uint16_t addr0, addr1, addr2, addr3;

		if (arduboyPrvArdensWord(pc) == 0x9508u)
			break;
		if (arduboyPrvRawLds(pc, &reg0, &addr0) &&
			arduboyPrvRawLds((uint16_t)(pc + 2u), &reg1, &addr1) &&
			arduboyPrvRawLds((uint16_t)(pc + 4u), &reg2, &addr2) &&
			arduboyPrvRawLds((uint16_t)(pc + 6u), &reg3, &addr3) &&
			reg0 == 24u && reg1 == 25u && reg2 == 26u && reg3 == 27u &&
			addr1 == addr0 + 1u && addr2 == addr0 + 2u && addr3 == addr0 + 3u)
			return addr0;
	}
	return absim::atmega32u4_t::EMBEDDED_HLE_NONE;
}

static bool arduboyPrvFindTimer0IsrVars(uint16_t overflowAddr, uint16_t *millisAddrP, uint16_t *fractAddrP)
{
	uint32_t words = (mArdens.flashUsed + 1u) / 2u;

	for (uint32_t start = 0; start < words; start++) {
		uint16_t millisAddr = absim::atmega32u4_t::EMBEDDED_HLE_NONE;
		uint16_t fractAddr = absim::atmega32u4_t::EMBEDDED_HLE_NONE;
		bool hasOverflowStore = false;
		bool hasMillisStore = false;
		bool hasFractStore = false;

		for (uint16_t pc = (uint16_t)start; (uint32_t)pc * 2u < mArdens.flashUsed; pc = (uint16_t)(pc + arduboyPrvRawInstrWords(pc))) {
			uint16_t w0 = arduboyPrvArdensWord(pc);
			uint8_t reg0, reg1, reg2, reg3;
			uint16_t addr0, addr1, addr2, addr3;

			if ((uint32_t)(pc - start) > 120u)
				break;
			if (w0 == 0x9518u) {
				if (hasOverflowStore && hasMillisStore && hasFractStore) {
					if (millisAddrP)
						*millisAddrP = millisAddr;
					if (fractAddrP)
						*fractAddrP = fractAddr;
					return true;
				}
				break;
			}
			if (w0 == 0x9508u)
				break;
			if (arduboyPrvRawSts(pc, &reg0, &addr0)) {
				if (addr0 >= overflowAddr && addr0 < overflowAddr + 4u)
					hasOverflowStore = true;
				if (reg0 == 18u) {
					fractAddr = addr0;
					hasFractStore = true;
				}
			}
			if (arduboyPrvRawLds(pc, &reg0, &addr0) &&
				arduboyPrvRawLds((uint16_t)(pc + 2u), &reg1, &addr1) &&
				arduboyPrvRawLds((uint16_t)(pc + 4u), &reg2, &addr2) &&
				arduboyPrvRawLds((uint16_t)(pc + 6u), &reg3, &addr3) &&
				reg0 == 24u && reg1 == 25u && reg2 == 26u && reg3 == 27u &&
				addr1 == addr0 + 1u && addr2 == addr0 + 2u && addr3 == addr0 + 3u &&
				addr0 != overflowAddr) {
				millisAddr = addr0;
			}
			if (millisAddr != absim::atmega32u4_t::EMBEDDED_HLE_NONE &&
				arduboyPrvRawSts(pc, &reg0, &addr0) &&
				arduboyPrvRawSts((uint16_t)(pc + 2u), &reg1, &addr1) &&
				arduboyPrvRawSts((uint16_t)(pc + 4u), &reg2, &addr2) &&
				arduboyPrvRawSts((uint16_t)(pc + 6u), &reg3, &addr3) &&
				reg0 == 24u && reg1 == 25u && reg2 == 26u && reg3 == 27u &&
				addr0 == millisAddr && addr1 == millisAddr + 1u &&
				addr2 == millisAddr + 2u && addr3 == millisAddr + 3u) {
				hasMillisStore = true;
			}
		}
	}
	return false;
}

static void arduboyPrvDetectEmbeddedHleTargets(void)
{
	uint16_t bestTarget = absim::atmega32u4_t::EMBEDDED_HLE_NONE;
	uint16_t bestMicros = absim::atmega32u4_t::EMBEDDED_HLE_NONE;
	uint8_t bestScore = 0;
	uint32_t words = (mArdens.flashUsed + 1u) / 2u;

	for (uint32_t pc = 0; pc < words && pc < ARDUBOY_FLASH_SIZE / 2u; pc++) {
		uint16_t target;
		uint16_t microsTarget = absim::atmega32u4_t::EMBEDDED_HLE_NONE;
		uint8_t callWords;
		uint8_t score;

		if (!arduboyPrvRawDirectCallTarget((uint16_t)pc, &target, &callWords))
			continue;
		if ((uint32_t)target * 2u >= mArdens.flashUsed)
			continue;
		score = arduboyPrvScoreArduinoDelayTarget(target, &microsTarget);
		if (score > bestScore) {
			bestScore = score;
			bestTarget = target;
			bestMicros = microsTarget;
		}
		pc += callWords - 1u;
	}

	if (bestScore >= 10u) {
		mArdens.cpu.embedded_delay_pc = bestTarget;
		mArdens.cpu.embedded_micros_pc = bestMicros;
		mArdens.cpu.embedded_timer0_overflow_addr =
			arduboyPrvDetectTimer0OverflowAddr(bestMicros);
		if (mArdens.cpu.embedded_timer0_overflow_addr != absim::atmega32u4_t::EMBEDDED_HLE_NONE) {
			(void)arduboyPrvFindTimer0IsrVars(mArdens.cpu.embedded_timer0_overflow_addr,
				&mArdens.cpu.embedded_timer0_millis_addr,
				&mArdens.cpu.embedded_timer0_fract_addr);
		}
	}
	pr("arduboy ardens hle: delay=%04x micros=%04x score=%u ms=%04x fract=%04x ovf=%04x\n",
		(unsigned)mArdens.cpu.embedded_delay_pc,
		(unsigned)mArdens.cpu.embedded_micros_pc,
		(unsigned)bestScore,
		(unsigned)mArdens.cpu.embedded_timer0_millis_addr,
		(unsigned)mArdens.cpu.embedded_timer0_fract_addr,
		(unsigned)mArdens.cpu.embedded_timer0_overflow_addr);
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

static bool arduboyPrvLoadHexToArdens(uint32_t addr, const uint8_t *data, uint32_t len, void *userData)
{
	ArduboyArdensRuntime *rt = (ArduboyArdensRuntime*)userData;

	if (!rt || addr > ARDUBOY_FLASH_SIZE || len > ARDUBOY_FLASH_SIZE - addr)
		return false;
	memcpy(rt->cpu.prog.data() + addr, data, len);
	if (rt->flashUsed < addr + len)
		rt->flashUsed = addr + len;
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
		mArdens.presentSrcX[y] = srcX;
	}

	mPresentColFirst = DISP_WIDTH;
	mPresentColEnd = 0;
	for (uint32_t x = 0; x < DISP_WIDTH; x++) {
		uint32_t logicalY = DISP_WIDTH - 1u - x;
		uint8_t page = ARDUBOY_PRESENT_Y_INVALID;
		uint8_t mask = 0;
		uint8_t pageFlipped = ARDUBOY_PRESENT_Y_INVALID;
		uint8_t maskFlipped = 0;

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
		mArdens.presentSrcYPage[x] = page;
		mArdens.presentSrcYMask[x] = mask;
		mArdens.presentSrcYPageFlipped[x] = pageFlipped;
		mArdens.presentSrcYMaskFlipped[x] = maskFlipped;
	}

	memset(dispGetFb(), 0, DISP_WIDTH * DISP_HEIGHT * DISP_BPP / 8);
	mPresentLastState = ARDUBOY_PRESENT_STATE_INVALID;
}

void arduboyRefreshDisplayOptions(void)
{
	(void)arduboyPrvRuntime();
	dispSetFramerate(ARDUBOY_FRAME_RATE);
	arduboyPrvUpdatePresentMap();
	mArdens.displayDirty = true;
}

static bool arduboyPrvVramHasNonzero(void)
{
	for (uint32_t i = 0; i < mArdens.display.ram.size(); i++)
		if (mArdens.display.ram[i])
			return true;
	return false;
}

static void arduboyPrvPresentFrame(bool force)
{
	uint16_t *fb = (uint16_t*)dispGetFb();
	bool displayOn = mArdens.display.display_on;
	bool inverted = mArdens.display.inverse_display;
	bool comScanReverse = mArdens.display.com_scan_direction;
	const uint8_t *srcYPage = comScanReverse ? mArdens.presentSrcYPage : mArdens.presentSrcYPageFlipped;
	const uint8_t *srcYMask = comScanReverse ? mArdens.presentSrcYMask : mArdens.presentSrcYMaskFlipped;
	uint8_t presentState = (displayOn ? ARDUBOY_PRESENT_STATE_DISPLAY_ON : 0u) |
		(inverted ? ARDUBOY_PRESENT_STATE_INVERTED : 0u) |
		(comScanReverse ? 0x04u : 0u);
	bool clearFrame = force || presentState != mPresentLastState;
	uint16_t onColor = inverted ? 0x0000u : 0xffffu;
	uint16_t offColor = inverted ? 0xffffu : 0x0000u;

	if (!force && !clearFrame && !mArdens.displayDirty)
		return;
	if (clearFrame) {
		arduboyPrvFill(fb, DISP_WIDTH * DISP_HEIGHT, offColor);
		mPresentLastState = presentState;
	}
	if (!displayOn && !clearFrame)
		return;

	for (uint32_t y = mPresentRowFirst; y < mPresentRowEnd; y++) {
		uint16_t srcX = mArdens.presentSrcX[y];

		if (srcX == ARDUBOY_PRESENT_INVALID)
			continue;
		srcX = ARDUBOY_DISPLAY_WIDTH - 1u - srcX;
		for (uint32_t x = mPresentColFirst; x < mPresentColEnd; x++) {
			uint8_t page = srcYPage[x];
			uint8_t mask = srcYMask[x];
			bool pixelOn = displayOn && page != ARDUBOY_PRESENT_Y_INVALID &&
				(mArdens.display.ram[(uint32_t)page * ARDUBOY_DISPLAY_WIDTH + srcX] & mask);

			uint32_t dst = y * DISP_WIDTH + x;

			if (mPresentFlipped)
				dst = DISP_WIDTH * DISP_HEIGHT - 1u - dst;
			fb[dst] = pixelOn ? onColor : offColor;
		}
	}

	mArdens.displayDirty = false;
	mArdens.displayUpdates++;
	if (displayOn && arduboyPrvVramHasNonzero()) {
		mArdens.visibleFrameSeen = true;
		mArdens.displayEverOn = true;
	}
	else if (displayOn) {
		mArdens.displayEverOn = true;
	}
}

static void arduboyPrvDebugPutPixel(uint16_t *fb, uint32_t x, uint32_t y, uint16_t color)
{
	if (!fb || x >= ARDUBOY_DEBUG_LOGICAL_WIDTH || y >= ARDUBOY_DEBUG_LOGICAL_HEIGHT)
		return;
	uint32_t dst = x * DISP_WIDTH + (DISP_WIDTH - 1u - y);

	if (mPresentFlipped)
		dst = DISP_WIDTH * DISP_HEIGHT - 1u - dst;
	fb[dst] = color;
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

static const char *arduboyPrvFailReasonText(uint8_t reason)
{
	switch (reason) {
		case ArduboyArdensFailInvalidRom:
			return "INVALID ROM";
		case ArduboyArdensFailInvalidPc:
			return "INVALID PC";
		case ArduboyArdensFailBootWatchdog:
			return "BOOT WATCHDOG";
		case ArduboyArdensFailNoDisplayIo:
			return "NO DISPLAY IO";
		default:
			return "UNKNOWN";
	}
}

static void arduboyPrvShowFailureScreen(const char *reason)
{
	uint16_t *fb = (uint16_t*)dispGetFb();
	char line[96];
	uint64_t now = getTime();
	uint64_t elapsed = now >= mArdens.startTicks ? now - mArdens.startTicks : 0;
	uint32_t cps = elapsed ? (uint32_t)((mArdens.cpu.cycle_count * (uint64_t)TICKS_PER_SECOND) / elapsed) : 0;

	arduboyPrvDebugFillRect(fb, 0, 0, ARDUBOY_DEBUG_LOGICAL_WIDTH, ARDUBOY_DEBUG_LOGICAL_HEIGHT, 0x0000u);
	arduboyPrvDebugFillRect(fb, 0, 0, ARDUBOY_DEBUG_LOGICAL_WIDTH, 2, 0xf800u);
	arduboyPrvDebugFillRect(fb, 0, ARDUBOY_DEBUG_LOGICAL_HEIGHT - 2u, ARDUBOY_DEBUG_LOGICAL_WIDTH, 2, 0xf800u);
	arduboyPrvDebugFillRect(fb, 0, 0, 2, ARDUBOY_DEBUG_LOGICAL_HEIGHT, 0xf800u);
	arduboyPrvDebugFillRect(fb, ARDUBOY_DEBUG_LOGICAL_WIDTH - 2u, 0, 2, ARDUBOY_DEBUG_LOGICAL_HEIGHT, 0xf800u);
	arduboyPrvDebugDrawText(fb, 14, 14, "ARDUBOY ARDENS STOP", 0xffe0u);
	(void)sprintf(line, "REASON: %s", reason);
	arduboyPrvDebugDrawText(fb, 14, 34, line, 0xffffu);
	(void)sprintf(line, "PC:%04x OP:%04x CYC:%08lx",
		(unsigned)mArdens.cpu.pc, (unsigned)mArdens.lastOpcode,
		(unsigned long)mArdens.cpu.cycle_count);
	arduboyPrvDebugDrawText(fb, 14, 54, line, 0xffffu);
	(void)sprintf(line, "SPI:%lu CMD:%lu DATA:%lu IGN:%lu",
		(unsigned long)mArdens.spiWrites, (unsigned long)mArdens.commandWrites,
		(unsigned long)mArdens.dataWrites, (unsigned long)mArdens.ignoredSpiWrites);
	arduboyPrvDebugDrawText(fb, 14, 74, line, 0xffffu);
	(void)sprintf(line, "LCD:%u INV:%u COM:%u LAST:%02x",
		mArdens.display.display_on ? 1u : 0u, mArdens.display.inverse_display ? 1u : 0u,
		mArdens.display.com_scan_direction ? 1u : 0u, (unsigned)mArdens.lastSpiByte);
	arduboyPrvDebugDrawText(fb, 14, 94, line, 0xffffu);
	(void)sprintf(line, "FR:%lu DISP:%lu EE:%lu",
		(unsigned long)mArdens.frameCount, (unsigned long)mArdens.displayUpdates,
		(unsigned long)mArdens.eepromSyncs);
	arduboyPrvDebugDrawText(fb, 14, 114, line, 0xffffu);
	(void)sprintf(line, "GUEST:%lu/s HOST:%lums",
		(unsigned long)cps, (unsigned long)(elapsed / (TICKS_PER_SECOND / 1000u)));
	arduboyPrvDebugDrawText(fb, 14, 134, line, 0xffffu);
	(void)sprintf(line, "PORTD:%02x PINS:%02x/%02x/%02x",
		(unsigned)mArdens.lastPortD, (unsigned)mArdens.cpu.data[ARDUBOY_AVR_PINB],
		(unsigned)mArdens.cpu.data[ARDUBOY_AVR_PINE], (unsigned)mArdens.cpu.data[ARDUBOY_AVR_PINF]);
	arduboyPrvDebugDrawText(fb, 14, 154, line, 0xffffu);
	(void)sprintf(line, "HLE:DELAY PC:%04x HIT:%lu LAST:%lu",
		(unsigned)mArdens.cpu.embedded_delay_pc,
		(unsigned long)mArdens.cpu.embedded_delay_hits,
		(unsigned long)mArdens.cpu.embedded_delay_last_ms);
	arduboyPrvDebugDrawText(fb, 14, 174, line, 0xffffu);
	(void)sprintf(line, "FASTSPI:%lu",
		(unsigned long)mArdens.cpu.embedded_spi_fast_writes);
	arduboyPrvDebugDrawText(fb, 190, 174, line, 0xffffu);
	(void)sprintf(line, "T0:%lu %04x/%04x/%04x",
		(unsigned long)mArdens.cpu.embedded_timer0_fast_updates,
		(unsigned)mArdens.cpu.embedded_timer0_millis_addr,
		(unsigned)mArdens.cpu.embedded_timer0_fract_addr,
		(unsigned)mArdens.cpu.embedded_timer0_overflow_addr);
	arduboyPrvDebugDrawText(fb, 14, 188, line, 0xffffu);
	arduboyPrvDebugDrawText(fb, 14, 208, "CENTER: MENU  B: EXIT FROM MENU", 0x07e0u);
	dispPrvWaitForScanoutStart();
}

static void arduboyPrvFailureLoop(const char *reason)
{
	mArdens.failed = true;
	while (!mArdens.abortRequested) {
		arduboyPrvShowFailureScreen(reason);
		if (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER) {
			arduboyPortInGameMenu();
			while (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER)
				;
		}
		if (uiGetKeysRaw() & KEY_BIT_B)
			break;
		dispPrvFrameCtrWait();
	}
}

static void arduboyPrvApplyButtons(void)
{
	uint_fast8_t keys = uiGetKeysRaw();

	mArdens.cpu.data[ARDUBOY_AVR_PINB] =
		(uint8_t)((mArdens.cpu.data[ARDUBOY_AVR_PINB] & 0xefu) | ((keys & KEY_BIT_B) ? 0u : 0x10u));
	mArdens.cpu.data[ARDUBOY_AVR_PINE] =
		(uint8_t)((mArdens.cpu.data[ARDUBOY_AVR_PINE] & 0xbfu) | ((keys & KEY_BIT_A) ? 0u : 0x40u));
	mArdens.cpu.data[ARDUBOY_AVR_PINF] = (uint8_t)((mArdens.cpu.data[ARDUBOY_AVR_PINF] & 0x0fu) |
		((keys & KEY_BIT_UP) ? 0u : 0x80u) |
		((keys & KEY_BIT_RIGHT) ? 0u : 0x40u) |
		((keys & KEY_BIT_LEFT) ? 0u : 0x20u) |
		((keys & KEY_BIT_DOWN) ? 0u : 0x10u));
}

static void arduboyPrvSyncEepromToSave(bool force)
{
	uint64_t now = getTime();

	if (!mArdens.saveRam || mArdens.saveRamSize < ARDUBOY_SAVE_RAM_SIZE)
		return;
	if (!force && !mArdens.cpu.eeprom_dirty && now - mArdens.lastEepromSyncTicks < ARDUBOY_EEPROM_SYNC_TICKS)
		return;
	memcpy(mArdens.saveRam, mArdens.cpu.eeprom.data(), ARDUBOY_SAVE_RAM_SIZE);
	mArdens.cpu.eeprom_dirty = false;
	mArdens.lastEepromSyncTicks = now;
	mArdens.eepromSyncs++;
}

static bool arduboyPrvServiceRuntimeControls(void)
{
	if (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER) {
		arduboyPrvSyncEepromToSave(true);
		arduboyPortInGameMenu();
		while (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER)
			;
		return !mArdens.abortRequested;
	}
	return true;
}

static void arduboyPrvLogBootProgress(uint64_t now)
{
	uint64_t elapsed = now >= mArdens.startTicks ? now - mArdens.startTicks : 0;
	uint32_t cps = elapsed ? (uint32_t)((mArdens.cpu.cycle_count * (uint64_t)TICKS_PER_SECOND) / elapsed) : 0;
	uint32_t ms = (uint32_t)(elapsed / (TICKS_PER_SECOND / 1000u));

	if (now - mArdens.lastBootLogTicks < ARDUBOY_BOOT_LOG_TICKS)
		return;
	pr("arduboy ardens boot: wall=%lums guest=%lu/s pc=%04x op=%04x frame=%lu spi=%lu fastSpi=%lu cmd=%lu data=%lu ign=%lu disp=%lu visible=%u hleD=%lu lastMs=%lu t0=%lu\n",
		(unsigned long)ms, (unsigned long)cps, (unsigned)mArdens.cpu.pc,
		(unsigned)mArdens.lastOpcode, (unsigned long)mArdens.frameCount,
		(unsigned long)mArdens.spiWrites,
		(unsigned long)mArdens.cpu.embedded_spi_fast_writes,
		(unsigned long)mArdens.commandWrites,
		(unsigned long)mArdens.dataWrites, (unsigned long)mArdens.ignoredSpiWrites,
		(unsigned long)mArdens.displayUpdates, mArdens.visibleFrameSeen ? 1u : 0u,
		(unsigned long)mArdens.cpu.embedded_delay_hits,
		(unsigned long)mArdens.cpu.embedded_delay_last_ms,
		(unsigned long)mArdens.cpu.embedded_timer0_fast_updates);
	mArdens.lastBootLogTicks = now;
}

static void arduboyPrvStop(uint8_t reason)
{
	mArdens.failReason = reason;
	pr("arduboy ardens stop: reason=%u pc=%04x op=%04x cyc=%lu spi=%lu fastSpi=%lu cmd=%lu data=%lu ign=%lu frames=%lu disp=%lu hleD=%lu lastMs=%lu t0=%lu\n",
		(unsigned)reason, (unsigned)mArdens.cpu.pc, (unsigned)mArdens.lastOpcode,
		(unsigned long)mArdens.cpu.cycle_count, (unsigned long)mArdens.spiWrites,
		(unsigned long)mArdens.cpu.embedded_spi_fast_writes,
		(unsigned long)mArdens.commandWrites, (unsigned long)mArdens.dataWrites,
		(unsigned long)mArdens.ignoredSpiWrites, (unsigned long)mArdens.frameCount,
		(unsigned long)mArdens.displayUpdates,
		(unsigned long)mArdens.cpu.embedded_delay_hits,
		(unsigned long)mArdens.cpu.embedded_delay_last_ms,
		(unsigned long)mArdens.cpu.embedded_timer0_fast_updates);
	arduboyPrvFailureLoop(arduboyPrvFailReasonText(reason));
}

static void arduboyPrvCaptureCompletedSpiByte(uint8_t displayPort)
{
	if (mArdens.cpu.cycle_count < mArdens.cpu.spi_done_cycle)
		return;

	uint8_t byte = mArdens.cpu.spi_data_byte;

	mArdens.spiWrites++;
	mArdens.lastSpiByte = byte;
	mArdens.lastPortD = displayPort;
	if (!(displayPort & (1u << 6))) {
		if (displayPort & (1u << 4)) {
			mArdens.display.send_data(byte);
			mArdens.dataWrites++;
		}
		else {
			mArdens.display.send_command(byte);
			mArdens.commandWrites++;
		}
		mArdens.displayDirty = true;
	}
	else {
		mArdens.ignoredSpiWrites++;
	}

	mArdens.cpu.spi_datain_byte = 0xffu;
	mArdens.cpu.spi_done_cycle = UINT64_MAX;
}

void arduboyAbort(void)
{
	(void)arduboyPrvRuntime();
	mArdens.abortRequested = true;
}

void arduboyRun(const void *rom, uint32_t romSize, void *saveRam, uint32_t saveRamSize)
{
	ArduboyRomInfo info;
	uint64_t nextControlCycle;

	if (!arduboyAnalyzeRom(rom, romSize, &info) || info.isPackage)
		return;

	(void)arduboyPrvRuntime();
	memset(&mArdens, 0, sizeof(mArdens));
	mArdens.saveRam = (uint8_t*)saveRam;
	mArdens.saveRamSize = saveRamSize;
	mArdens.startTicks = getTime();
	mArdens.lastBootLogTicks = mArdens.startTicks;
	mArdens.lastEepromSyncTicks = mArdens.startTicks;
	mArdens.lastFrameTicks = mArdens.startTicks;

	arduboyRefreshDisplayOptions();
	memset(mArdens.cpu.prog.data(), 0xff, mArdens.cpu.prog.size());

	mArdens.cpu.lock = 0xff;
	mArdens.cpu.fuse_lo = 0xff;
	mArdens.cpu.fuse_hi = 0xd3;
	mArdens.cpu.fuse_ext = 0xcb;
	mArdens.cpu.reset();
	mArdens.display.reset();
	mArdens.display.type = absim::display_t::SSD1306;
	mArdens.cpu.data[ARDUBOY_AVR_PINB] = 0x10u;
	mArdens.cpu.data[ARDUBOY_AVR_PINE] = 0x40u;
	mArdens.cpu.data[ARDUBOY_AVR_PINF] = 0xf0u;

	if (saveRam && saveRamSize >= ARDUBOY_SAVE_RAM_SIZE)
		memcpy(mArdens.cpu.eeprom.data(), saveRam, ARDUBOY_SAVE_RAM_SIZE);

	if (!arduboyPrvHexParse(rom, romSize, arduboyPrvLoadHexToArdens, &mArdens, NULL)) {
		arduboyPrvStop(ArduboyArdensFailInvalidRom);
		return;
	}
	mArdens.cpu.program_loaded = true;
	mArdens.cpu.last_addr = (uint16_t)((mArdens.flashUsed + 1u) & ~1u);
	mArdens.cpu.decode();
	arduboyPrvDetectEmbeddedHleTargets();
	mArdens.cpu.pc = 0;
	mArdens.cpu.executing_instr_pc = 0;
	mArdens.cpu.spi_datain_byte = 0xffu;
	nextControlCycle = ARDUBOY_CONTROL_POLL_CYCLES;

	pr("arduboy ardens start: flash=%lu save=%lu backend=ardens audio=disabled\n",
		(unsigned long)mArdens.flashUsed, (unsigned long)saveRamSize);

	while (!mArdens.abortRequested) {
		uint64_t frameEnd = mArdens.cpu.cycle_count + ARDUBOY_FRAME_CYCLES;
		uint64_t now;

		arduboyPrvApplyButtons();
		while (!mArdens.abortRequested && mArdens.cpu.cycle_count < frameEnd) {
			uint8_t displayPort = mArdens.cpu.data[ARDUBOY_AVR_PORTD];
			uint64_t before = mArdens.cpu.cycle_count;

			if (mArdens.cpu.pc >= ARDUBOY_FLASH_SIZE / 2u) {
				arduboyPrvStop(ArduboyArdensFailInvalidPc);
				break;
			}
			mArdens.lastPc = mArdens.cpu.pc;
			mArdens.lastOpcode = mArdens.cpu.decoded_prog[mArdens.cpu.pc].word;
			mArdens.cpu.advance_cycle();
			arduboyPrvCaptureCompletedSpiByte(displayPort);
			mArdens.advanceCalls++;
			if (mArdens.cpu.cycle_count > before)
				mArdens.executedCycles += mArdens.cpu.cycle_count - before;
			if (mArdens.cpu.cycle_count >= nextControlCycle) {
				nextControlCycle = mArdens.cpu.cycle_count + ARDUBOY_CONTROL_POLL_CYCLES;
				if (!arduboyPrvServiceRuntimeControls())
					break;
				arduboyPrvApplyButtons();
			}
		}
		if (mArdens.abortRequested || mArdens.failed)
			break;

		arduboyPrvSyncEepromToSave(false);
		arduboyPrvPresentFrame(mArdens.frameCount == 0);
		mArdens.frameCount++;

		now = getTime();
		arduboyPrvLogBootProgress(now);
		if (!mArdens.visibleFrameSeen && now - mArdens.startTicks >= ARDUBOY_BOOT_WATCHDOG_TICKS) {
			arduboyPrvStop((mArdens.spiWrites || mArdens.commandWrites || mArdens.dataWrites) ?
				ArduboyArdensFailBootWatchdog : ArduboyArdensFailNoDisplayIo);
			break;
		}

		dispPrvFrameCtrWait();
	}

	arduboyPrvSyncEepromToSave(true);
	dispSetFramerate(ARDUBOY_FRAME_RATE);
}
