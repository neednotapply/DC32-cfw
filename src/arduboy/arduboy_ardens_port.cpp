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
#define ARDUBOY_PRESENT_RATE 60u
#define ARDUBOY_CONTROL_POLL_CYCLES 16384u
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
#define ARDUBOY_PRESENT_STATE_INVALID 0xffu
#define ARDUBOY_PRESENT_STATE_DISPLAY_ON 0x01u
#define ARDUBOY_PRESENT_STATE_INVERTED 0x02u
#define ARDUBOY_DEBUG_LOGICAL_WIDTH DISP_HEIGHT
#define ARDUBOY_DEBUG_LOGICAL_HEIGHT DISP_WIDTH
#define ARDUBOY_AVR_PINB 0x23u
#define ARDUBOY_AVR_PINE 0x2cu
#define ARDUBOY_AVR_PINF 0x2fu
#define ARDUBOY_AVR_DDRD 0x2au
#define ARDUBOY_AVR_PORTD 0x2bu
#define ARDUBOY_AVR_DDRE 0x2du
#define ARDUBOY_AVR_PORTE 0x2eu
#define ARDUBOY_FX_CACHE_META_SIZE QSPI_WRITE_GRANULARITY
#define ARDUBOY_FX_CACHE_REGION_SIZE (QSPI_ROM_SIZE_MAX / 2u)
#define ARDUBOY_FX_CACHE_META_ADDR \
	(QSPI_ROM_START + ARDUBOY_FX_CACHE_REGION_SIZE - ARDUBOY_FX_CACHE_META_SIZE)
#define ARDUBOY_FX_ADDRESS_SPACE 0x01000000u
#define ARDUBOY_EXPANDED_PAGE_PIXELS 20u
#define ARDUBOY_EXPANDED_PAGE_COUNT 8u
#define ARDUBOY_EXPANDED_LINE_PIXELS (ARDUBOY_EXPANDED_PAGE_PIXELS * ARDUBOY_EXPANDED_PAGE_COUNT)
#define ARDUBOY_FRAMEBUFFER_SIZE (128u * 64u / 8u)

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

enum ArduboyFxSpiState {
	ArduboyFxSpiIdle,
	ArduboyFxSpiReadAddress,
	ArduboyFxSpiReadData,
	ArduboyFxSpiReadStatus,
	ArduboyFxSpiReadJedec,
};

enum ArduboyHleKind {
	ArduboyHleNone,
	ArduboyHleArdBitmap203,
	ArduboyHleSpritesB,
};

struct ArduboyHleDescriptor {
	uint16_t targetPc;
	uint16_t framebufferAddr;
	uint8_t kind;
	uint8_t reserved;
};

struct ArduboyArdensRuntime {
	absim::atmega32u4_t cpu;
	absim::display_t display;
	uint8_t *saveRam;
	uint32_t saveRamSize;
	void (*ledsTick)(void);
	uint16_t presentSrcX[DISP_HEIGHT];
	uint64_t startTicks;
	uint64_t lastBootLogTicks;
	uint64_t lastEepromSyncTicks;
	uint64_t frameCount;
	uint32_t flashUsed;
	uint32_t spiWrites;
	uint32_t commandWrites;
	uint32_t dataWrites;
	uint32_t ignoredSpiWrites;
	uint32_t displayUpdates;
	uint32_t eepromSyncs;
	const uint8_t *fxData;
	uint32_t fxDataSize;
	uint32_t fxGuestBase;
	uint32_t fxAddress;
	uint16_t lastOpcode;
	uint8_t lastSpiByte;
	uint8_t lastPortD;
	bool abortRequested;
	bool displayDirty;
	bool displayEverOn;
	bool visibleFrameSeen;
	bool failed;
	uint8_t failReason;
	uint8_t fxSpiState;
	uint8_t fxAddressBytes;
	uint8_t fxJedecByte;
	bool fxSelected;
	bool fxAwake;
	ArduboyHleDescriptor graphicsHle;
	ArduboyHleDescriptor spritesHle;
	uint32_t graphicsHleHits;
	uint32_t graphicsHleFallbacks;
	uint32_t graphicsHleGuestCycles;
	uint32_t bulkDisplayHits;
};

static_assert(sizeof(ArduboyArdensRuntime) + ARDUBOY_SAVE_RAM_SIZE <= 0x2b000u,
	"Ardens runtime must fit in the reserved Arduboy scratch window");

extern "C" const uint32_t gArduboyArdensRuntimeSize = sizeof(ArduboyArdensRuntime);

static ArduboyArdensRuntime *mArdensPtr;
static uint16_t mPresentRowFirst, mPresentRowEnd;
static uint16_t mPresentColFirst, mPresentColEnd;
static uint8_t mPresentLastState = ARDUBOY_PRESENT_STATE_INVALID;
static bool mPresentFlipped;
static uint8_t mExpandBits[4][ARDUBOY_EXPANDED_PAGE_COUNT][ARDUBOY_EXPANDED_PAGE_PIXELS];
static uint8_t mExpandPages[4][ARDUBOY_EXPANDED_PAGE_COUNT];
static bool mExpandPixelsReady;


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

static bool arduboyPrvRawRegImm8(uint16_t pc, uint16_t opcode, uint8_t reg, uint8_t *immP)
{
	uint16_t w0 = arduboyPrvArdensWord(pc);

	if ((w0 & 0xf000u) != opcode || (uint8_t)(16u + ((w0 >> 4) & 0x0fu)) != reg)
		return false;
	if (immP)
		*immP = (uint8_t)(((w0 >> 4) & 0xf0u) | (w0 & 0x0fu));
	return true;
}

static uint32_t arduboyPrvDirectCallCount(uint16_t target)
{
	uint32_t calls = 0;
	uint32_t words = (mArdens.flashUsed + 1u) / 2u;

	for (uint32_t pc = 0; pc < words; pc++) {
		uint16_t callTarget;
		uint8_t callWords = 0;

		if (arduboyPrvRawDirectCallTarget((uint16_t)pc, &callTarget, &callWords) && callTarget == target)
			calls++;
		if (callWords == 2u)
			pc++;
	}
	return calls;
}

static bool arduboyPrvMatchesArdBitmap203(uint16_t target, uint16_t *framebufferAddrP)
{
	static const uint16_t prologue[] = {
		0x922fu, 0x923fu, 0x924fu, 0x925fu, 0x926fu, 0x927fu, 0x928fu,
		0x929fu, 0x92afu, 0x92bfu, 0x92cfu, 0x92dfu, 0x92efu, 0x92ffu,
		0x930fu, 0x931fu, 0x93cfu, 0x93dfu, 0xb7cdu, 0xb7deu, 0x9761u,
		0xb60fu, 0x94f8u, 0xbfdeu, 0xbe0fu, 0xbfcdu, 0x012cu, 0x011au,
		0x872bu, 0x01fau, 0x9134u, 0x9631u, 0x9124u,
	};
	uint8_t low, high;

	if ((uint32_t)target + sizeof(prologue) / sizeof(prologue[0]) >= ARDUBOY_FLASH_SIZE / 2u)
		return false;
	for (uint32_t i = 0; i < sizeof(prologue) / sizeof(prologue[0]); i++)
		if (arduboyPrvArdensWord((uint16_t)(target + i)) != prologue[i])
			return false;
	/* The 2.0.3 routine is 0x1f6 words long. These independent checks make the
	 * long common AVR save-register prologue insufficient to trigger HLE. */
	if (arduboyPrvArdensWord((uint16_t)(target + 0x1f5u)) != 0x9508u ||
		arduboyPrvArdensWord((uint16_t)(target + 0x124u)) != 0x01fbu ||
		!arduboyPrvRawRegImm8((uint16_t)(target + 0x122u), 0x5000u, 22u, &low) ||
		!arduboyPrvRawRegImm8((uint16_t)(target + 0x123u), 0x4000u, 23u, &high))
		return false;
	uint16_t framebufferAddr = (uint16_t)(0u - ((uint16_t)high << 8) - low);

	if (framebufferAddr < 0x100u ||
		(uint32_t)framebufferAddr + ARDUBOY_FRAMEBUFFER_SIZE > mArdens.cpu.data.size())
		return false;
	if (framebufferAddrP)
		*framebufferAddrP = framebufferAddr;
	return true;
}

static bool arduboyPrvMatchesSpritesB(uint16_t target, uint16_t *framebufferAddrP)
{
	static const uint16_t prologue[] = {
		0x922fu, 0x923fu, 0x924fu, 0x925fu, 0x926fu, 0x927fu, 0x928fu,
		0x929fu, 0x92afu, 0x92bfu, 0x92cfu, 0x92dfu, 0x92efu, 0x92ffu,
		0x930fu, 0x931fu, 0x93cfu, 0x93dfu, 0xd000u, 0xd000u, 0xb7cdu,
		0xb7deu, 0x2e60u,
	};
	uint8_t low, high;

	for (uint32_t i = 0; i < sizeof(prologue) / sizeof(prologue[0]); i++)
		if (arduboyPrvArdensWord((uint16_t)(target + i)) != prologue[i])
			return false;
	if (arduboyPrvArdensWord((uint16_t)(target + 0x153u)) != 0x9508u ||
		arduboyPrvArdensWord((uint16_t)(target + 0x1bu)) != 0x01fau ||
		!arduboyPrvRawRegImm8((uint16_t)(target + 0xe1u), 0x5000u, 26u, &low) ||
		!arduboyPrvRawRegImm8((uint16_t)(target + 0xe2u), 0x4000u, 27u, &high))
		return false;
	uint16_t framebufferAddr = (uint16_t)(0u - ((uint16_t)high << 8) - low);
	if (framebufferAddr < 0x100u ||
		(uint32_t)framebufferAddr + ARDUBOY_FRAMEBUFFER_SIZE > mArdens.cpu.data.size())
		return false;
	if (framebufferAddrP)
		*framebufferAddrP = framebufferAddr;
	return true;
}

static void arduboyPrvDetectGraphicsHle(void)
{
	mArdens.graphicsHle.targetPc = absim::atmega32u4_t::EMBEDDED_HLE_NONE;
	mArdens.graphicsHle.kind = ArduboyHleNone;
	mArdens.spritesHle.targetPc = absim::atmega32u4_t::EMBEDDED_HLE_NONE;
	mArdens.spritesHle.kind = ArduboyHleNone;
#ifdef ARDUBOY_HLE_ENABLED
	uint32_t words = (mArdens.flashUsed + 1u) / 2u;
	for (uint32_t pc = 0; pc < words; pc++) {
		uint16_t target, framebufferAddr;
		uint8_t callWords = 0;

		if (!arduboyPrvRawDirectCallTarget((uint16_t)pc, &target, &callWords))
			continue;
		if (target >= words)
			continue;
		if (mArdens.graphicsHle.kind == ArduboyHleNone &&
			arduboyPrvMatchesArdBitmap203(target, &framebufferAddr) &&
			arduboyPrvDirectCallCount(target) >= 4u) {
			mArdens.graphicsHle.targetPc = target;
			mArdens.graphicsHle.framebufferAddr = framebufferAddr;
			mArdens.graphicsHle.kind = ArduboyHleArdBitmap203;
		}
		if (mArdens.spritesHle.kind == ArduboyHleNone &&
			arduboyPrvMatchesSpritesB(target, &framebufferAddr) &&
			arduboyPrvDirectCallCount(target) >= 2u) {
			mArdens.spritesHle.targetPc = target;
			mArdens.spritesHle.framebufferAddr = framebufferAddr;
			mArdens.spritesHle.kind = ArduboyHleSpritesB;
		}
		if (mArdens.graphicsHle.kind != ArduboyHleNone && mArdens.spritesHle.kind != ArduboyHleNone)
			break;
		pc += callWords - 1u;
	}
#endif
	pr("arduboy graphics hle: bitmap=%u/%04x/%04x sprites=%u/%04x/%04x\n",
		(unsigned)mArdens.graphicsHle.kind, (unsigned)mArdens.graphicsHle.targetPc,
		(unsigned)mArdens.graphicsHle.framebufferAddr,
		(unsigned)mArdens.spritesHle.kind, (unsigned)mArdens.spritesHle.targetPc,
		(unsigned)mArdens.spritesHle.framebufferAddr);
}

struct ArduboyHleBitmapInfo {
	uint16_t bitmapAddr;
	uint16_t decodedBits;
	uint16_t spans;
	uint8_t width;
	uint8_t height;
	uint8_t firstColor;
	bool scanMode;
	bool scanZigZag;
};

static bool arduboyPrvHleReadBit(const ArduboyArdensRuntime *runtime, uint16_t bitmapAddr,
	uint32_t bitPos, uint8_t *valueP)
{
	uint32_t addr = (uint32_t)bitmapAddr + bitPos / 8u;

	if (addr >= runtime->flashUsed)
		return false;
	*valueP = (uint8_t)((runtime->cpu.prog[addr] >> (bitPos & 7u)) & 1u);
	return true;
}

static bool arduboyPrvHleInspectArdBitmap203(const ArduboyArdensRuntime *runtime,
	uint16_t bitmapAddr, ArduboyHleBitmapInfo *infoP)
{
	if ((uint32_t)bitmapAddr + 2u > runtime->flashUsed)
		return false;
	uint8_t byte0 = runtime->cpu.prog[bitmapAddr + 0u];
	uint8_t byte1 = runtime->cpu.prog[bitmapAddr + 1u];
	uint32_t totalBits = (uint32_t)((byte0 & 0x7fu) + 1u) * ((byte1 & 0x3fu) + 1u);
	uint32_t emitted = 0;
	uint32_t encoderPos = 16;
	uint32_t spans = 0;

	while (emitted < totalBits) {
		uint32_t sizeCounter = 1;
		uint8_t bit;

		while (true) {
			if (!arduboyPrvHleReadBit(runtime, bitmapAddr, encoderPos++, &bit))
				return false;
			if (!bit)
				break;
			if (++sizeCounter > 15u)
				return false;
		}
		uint32_t len;
		if (sizeCounter == 1u) {
			if (!arduboyPrvHleReadBit(runtime, bitmapAddr, encoderPos++, &bit))
				return false;
			len = 1u + bit;
		}
		else {
			len = (1u << (sizeCounter - 1u)) + 1u;
			for (uint32_t j = 0; j < sizeCounter - 1u; j++) {
				if (!arduboyPrvHleReadBit(runtime, bitmapAddr, encoderPos++, &bit))
					return false;
				len += (uint32_t)bit << j;
			}
		}
		if (len == 0 || len > totalBits - emitted)
			return false;
		emitted += len;
		if (++spans > totalBits)
			return false;
	}

	infoP->bitmapAddr = bitmapAddr;
	infoP->decodedBits = (uint16_t)totalBits;
	infoP->spans = (uint16_t)spans;
	infoP->width = (uint8_t)((byte0 & 0x7fu) + 1u);
	infoP->height = (uint8_t)((byte1 & 0x3fu) + 1u);
	infoP->firstColor = (uint8_t)(byte0 >> 7);
	infoP->scanMode = (byte1 & 0x40u) != 0;
	infoP->scanZigZag = (byte1 & 0x80u) != 0;
	return true;
}

static bool arduboyPrvHleDrawArdBitmap203(ArduboyArdensRuntime *runtime,
	const ArduboyHleBitmapInfo *info, int16_t sx, int16_t sy, uint8_t color,
	uint8_t align, uint8_t mirror, uint32_t *storesP)
{
	if (align & 0x02u)
		sx = (int16_t)(sx - info->width / 2u);
	else if (align & 0x01u)
		sx = (int16_t)(sx - info->width);
	if (align & 0x08u)
		sy = (int16_t)(sy - info->height / 2u);
	else if (align & 0x04u)
		sy = (int16_t)(sy - info->height);
	if ((int32_t)sx + info->width < 0 || sx > 127 ||
		(int32_t)sy + info->height < 0 || sy > 63)
		return true;

	int32_t yAbs = sy < 0 ? -(int32_t)sy : sy;
	int16_t yOffset = (int16_t)(yAbs % 8);
	int16_t sRow = (int16_t)(sy / 8);
	if (sy < 0 && yOffset > 0) {
		sRow--;
		yOffset = (int16_t)(8 - yOffset);
	}
	int16_t rows = (int16_t)((info->height + 7u) / 8u);
	int16_t row = 0;
	int16_t column = 0;
	int16_t columnOffset = (mirror & 0x01u) ? (int16_t)(info->width - 1u) : 0;
	bool scanMode = info->scanMode;
	if (mirror & 0x02u) {
		row = (int16_t)(rows - 1);
		scanMode = !scanMode;
	}

	uint32_t encoderPos = 16;
	uint32_t emitted = 0;
	uint8_t spanColor = info->firstColor;
	uint8_t decodedByte = 0;
	uint8_t characterPos = 7;
	uint8_t *framebuffer = runtime->cpu.data.data() + runtime->graphicsHle.framebufferAddr;

	while (emitted < info->decodedBits) {
		uint32_t sizeCounter = 1;
		uint8_t bit;
		do {
			if (!arduboyPrvHleReadBit(runtime, info->bitmapAddr, encoderPos++, &bit))
				return false;
			if (bit)
				sizeCounter++;
		} while (bit);
		uint32_t len;
		if (sizeCounter == 1u) {
			if (!arduboyPrvHleReadBit(runtime, info->bitmapAddr, encoderPos++, &bit))
				return false;
			len = 1u + bit;
		}
		else {
			len = (1u << (sizeCounter - 1u)) + 1u;
			for (uint32_t j = 0; j < sizeCounter - 1u; j++) {
				if (!arduboyPrvHleReadBit(runtime, info->bitmapAddr, encoderPos++, &bit))
					return false;
				len += (uint32_t)bit << j;
			}
		}

		for (uint32_t i = 0; i < len; i++, emitted++) {
			if (spanColor)
				decodedByte |= (uint8_t)(1u << (scanMode ? characterPos : 7u - characterPos));
			characterPos--;
			if (characterPos != 0xffu)
				continue;

			int16_t bufferRow = (int16_t)(sRow + row);
			int16_t screenX = (int16_t)(sx + columnOffset);
			if (decodedByte && bufferRow < 8 && screenX >= 0 && screenX < 128) {
				uint16_t bitmapData = (uint16_t)decodedByte << yOffset;
				if (bufferRow >= 0) {
					uint8_t *dst = framebuffer + bufferRow * 128 + screenX;
					*dst = color ? (uint8_t)(*dst | bitmapData) :
						(uint8_t)(*dst & ~(uint8_t)bitmapData);
					(*storesP)++;
				}
				if (yOffset && bufferRow < 7 && bufferRow > -2) {
					uint8_t *dst = framebuffer + (bufferRow + 1) * 128 + screenX;
					uint8_t upper = (uint8_t)(bitmapData >> 8);
					*dst = color ? (uint8_t)(*dst | upper) : (uint8_t)(*dst & ~upper);
					(*storesP)++;
				}
			}
			if (info->scanZigZag)
				scanMode = !scanMode;
			column++;
			columnOffset += (mirror & 0x01u) ? -1 : 1;
			if (column >= info->width) {
				column = 0;
				row += (mirror & 0x02u) ? -1 : 1;
				columnOffset = (mirror & 0x01u) ? (int16_t)(info->width - 1u) : 0;
			}
			decodedByte = 0;
			characterPos = 7;
		}
		spanColor ^= 1u;
	}
	return true;
}

static bool arduboyPrvHleDrawSpritesB(ArduboyArdensRuntime *runtime,
	int16_t x, int16_t y, uint16_t bitmapAddr, uint8_t frame, uint8_t mode,
	uint32_t *sourceBytesP, uint32_t *storesP)
{
	if (mode != 2u && mode != 3u && mode != 250u && mode != 251u)
		return false;
	if ((uint32_t)bitmapAddr + 2u > runtime->flashUsed)
		return false;
	uint8_t width = runtime->cpu.prog[bitmapAddr + 0u];
	uint8_t height = runtime->cpu.prog[bitmapAddr + 1u];
	if (!width || !height)
		return false;
	uint32_t rows = (height + 7u) / 8u;
	uint32_t step = mode == 3u ? 2u : 1u;
	uint32_t frameBytes = (uint32_t)width * rows * step;
	uint32_t dataAddr = (uint32_t)bitmapAddr + 2u + (uint32_t)frame * frameBytes;
	if (dataAddr + frameBytes > runtime->flashUsed)
		return false;
	if ((int32_t)x + width <= 0 || x > 127 || (int32_t)y + height <= 0 || y > 63)
		return true;

	int16_t yOffset = (int16_t)((uint16_t)y & 7u);
	int16_t startRow = (int16_t)(y / 8);
	if (y < 0 && yOffset > 0)
		startRow--;
	uint16_t xOffset = x < 0 ? (uint16_t)(-(int32_t)x) : 0u;
	uint16_t renderedWidth = (int32_t)x + width > 127 ?
		(uint16_t)(128 - x - xOffset) : (uint16_t)(width - xOffset);
	uint16_t startHeight = startRow < -1 ? (uint16_t)(-startRow - 1) : 0u;
	int16_t loopHeight = (int16_t)rows;
	if (startRow + loopHeight > 8)
		loopHeight = (int16_t)(8 - startRow);
	loopHeight = (int16_t)(loopHeight - startHeight);
	if (loopHeight <= 0 || renderedWidth == 0)
		return true;

	startRow = (int16_t)(startRow + startHeight);
	int32_t framebufferOffset = (int32_t)startRow * 128 + x + xOffset;
	uint32_t sourceOffset = ((uint32_t)startHeight * width + xOffset) * step;
	uint32_t stride = ((uint32_t)width - renderedWidth) * step;
	uint8_t *framebuffer = runtime->cpu.data.data() + runtime->spritesHle.framebufferAddr;
	uint16_t shift = (uint16_t)(1u << yOffset);

	for (int16_t row = 0; row < loopHeight; row++) {
		for (uint16_t column = 0; column < renderedWidth; column++) {
			uint16_t bitmapData = (uint16_t)runtime->cpu.prog[dataAddr + sourceOffset] * shift;
			uint16_t maskData = (uint16_t)~bitmapData;
			if (mode == 2u)
				maskData = (uint16_t)~(0xffu * shift);
			else if (mode == 251u)
				bitmapData = 0;
			else
				maskData = (uint16_t)~((uint16_t)runtime->cpu.prog[dataAddr + sourceOffset + step - 1u] * shift);

			if (startRow >= 0 && framebufferOffset >= 0 && framebufferOffset < (int32_t)ARDUBOY_FRAMEBUFFER_SIZE) {
				uint8_t *dst = framebuffer + framebufferOffset;
				*dst = (uint8_t)((*dst & (uint8_t)maskData) | (uint8_t)bitmapData);
				(*storesP)++;
			}
			if (yOffset && startRow < 7 && framebufferOffset + 128 >= 0 &&
				framebufferOffset + 128 < (int32_t)ARDUBOY_FRAMEBUFFER_SIZE) {
				uint8_t *dst = framebuffer + framebufferOffset + 128;
				*dst = (uint8_t)((*dst & (uint8_t)(maskData >> 8)) | (uint8_t)(bitmapData >> 8));
				(*storesP)++;
			}
			framebufferOffset++;
			sourceOffset += step;
		}
		startRow++;
		sourceOffset += stride;
		framebufferOffset += 128 - renderedWidth;
	}
	*sourceBytesP = (uint32_t)loopHeight * renderedWidth * step;
	return true;
}

static bool arduboyPrvHleCanAdvance(absim::atmega32u4_t& cpu, uint32_t cycles)
{
	cpu.update_all();
	return cpu.embedded_can_advance_timer0_delay(cycles);
}

static void arduboyPrvHleAdvance(absim::atmega32u4_t& cpu, uint32_t cycles)
{
	cpu.cycle_count += cycles;
	(void)cpu.embedded_advance_timer0_delay(cycles);
}

static uint32_t __attribute__((section(".fastcode.arduboy_hle"), noinline, noclone))
	arduboyPrvGraphicsHle(void *context, absim::atmega32u4_t& cpu,
	uint16_t target, uint16_t retAddr, uint32_t baseCycles)
{
	ArduboyArdensRuntime *runtime = (ArduboyArdensRuntime*)context;
	if (!runtime)
		return 0;
	if (runtime->spritesHle.kind == ArduboyHleSpritesB && target == runtime->spritesHle.targetPc) {
		uint16_t bitmapAddr = cpu.gpr_word(20);
		uint8_t mode = cpu.gpr(16);
		if ((uint32_t)bitmapAddr + 2u > runtime->flashUsed ||
			(mode != 2u && mode != 3u && mode != 250u && mode != 251u)) {
			runtime->graphicsHleFallbacks++;
			return 0;
		}
		uint8_t width = cpu.prog[bitmapAddr + 0u];
		uint8_t height = cpu.prog[bitmapAddr + 1u];
		uint32_t rows = (height + 7u) / 8u;
		uint32_t step = mode == 3u ? 2u : 1u;
		uint32_t frameBytes = (uint32_t)width * rows * step;
		uint32_t dataAddr = (uint32_t)bitmapAddr + 2u + (uint32_t)cpu.gpr(18) * frameBytes;
		if (!width || !height || dataAddr + frameBytes > runtime->flashUsed) {
			runtime->graphicsHleFallbacks++;
			return 0;
		}
		uint32_t maxCycles = 300u + frameBytes * 80u +
			(uint32_t)width * (rows + 1u) * 35u;
		if (!arduboyPrvHleCanAdvance(cpu, maxCycles)) {
			runtime->graphicsHleFallbacks++;
			return 0;
		}
		uint32_t sourceBytes = 0, stores = 0;
		if (!arduboyPrvHleDrawSpritesB(runtime,
			(int16_t)cpu.gpr_word(24), (int16_t)cpu.gpr_word(22), cpu.gpr_word(20),
			cpu.gpr(18), cpu.gpr(16), &sourceBytes, &stores)) {
			runtime->graphicsHleFallbacks++;
			return 0;
		}
		uint32_t cycles = 300u + sourceBytes * 80u + stores * 35u;
		arduboyPrvHleAdvance(cpu, cycles);
		cpu.pc = retAddr;
		runtime->graphicsHleHits++;
		runtime->graphicsHleGuestCycles += cycles;
		return baseCycles;
	}
	if (runtime->graphicsHle.kind != ArduboyHleArdBitmap203 ||
		target != runtime->graphicsHle.targetPc)
		return 0;

	ArduboyHleBitmapInfo info;
	uint16_t bitmapAddr = cpu.gpr_word(20);
	if (!arduboyPrvHleInspectArdBitmap203(runtime, bitmapAddr, &info)) {
		runtime->graphicsHleFallbacks++;
		return 0;
	}
	uint32_t maxCycles = 400u + (uint32_t)info.decodedBits * 18u +
		(uint32_t)info.spans * 55u + (uint32_t)info.decodedBits * 2u * 25u;
	if (!arduboyPrvHleCanAdvance(cpu, maxCycles)) {
		runtime->graphicsHleFallbacks++;
		return 0;
	}
	uint32_t stores = 0;
	if (!arduboyPrvHleDrawArdBitmap203(runtime, &info,
		(int16_t)cpu.gpr_word(24), (int16_t)cpu.gpr_word(22), cpu.gpr(18),
		cpu.gpr(16), cpu.gpr(14), &stores)) {
		runtime->graphicsHleFallbacks++;
		return 0;
	}

	uint32_t cycles = 400u + (uint32_t)info.decodedBits * 18u +
		(uint32_t)info.spans * 55u + stores * 25u;
	arduboyPrvHleAdvance(cpu, cycles);
	cpu.pc = retAddr;
	runtime->graphicsHleHits++;
	runtime->graphicsHleGuestCycles += cycles;
	return baseCycles;
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
	arduboyPrvDetectGraphicsHle();
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
	void *writeBuf, uint32_t writeBufSize, uint32_t *hexSizeP, uint32_t *fxDataSizeP)
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
	if (fxDataSizeP)
		*fxDataSizeP = 0;
	return true;
}

static void arduboyPrvFill(uint16_t *dst, uint32_t count, uint16_t color)
{
	while (count--)
		*dst++ = color;
}

static void arduboyPrvBuildExpandPixels(void)
{
	if (mExpandPixelsReady)
		return;
	for (uint32_t state = 0; state < 4; state++) {
		bool comScanReverse = (state & 1u) != 0;
		bool flipped = (state & 2u) != 0;
		for (uint32_t group = 0; group < ARDUBOY_EXPANDED_PAGE_COUNT; group++) {
			for (uint32_t x = 0; x < ARDUBOY_EXPANDED_PAGE_PIXELS; x++) {
				uint32_t output = group * ARDUBOY_EXPANDED_PAGE_PIXELS + x;
				uint32_t original = flipped ? ARDUBOY_EXPANDED_LINE_PIXELS - 1u - output : output;
				uint32_t sourceY = ((ARDUBOY_EXPANDED_LINE_PIXELS - 1u - original) *
					ARDUBOY_DISPLAY_HEIGHT) / ARDUBOY_EXPANDED_LINE_PIXELS;
				if (!comScanReverse)
					sourceY = ARDUBOY_DISPLAY_HEIGHT - 1u - sourceY;
				if (x == 0)
					mExpandPages[state][group] = (uint8_t)(sourceY >> 3);
				mExpandBits[state][group][x] = (uint8_t)(1u << (sourceY & 7u));
			}
		}
	}
	mExpandPixelsReady = true;
}

static void arduboyPrvUpdatePresentMap(void)
{
	uint32_t logicalW = DISP_HEIGHT;
	uint32_t logicalH = DISP_WIDTH;
	uint32_t dstW, dstH, dstX;

	if ((uint64_t)logicalW * ARDUBOY_DISPLAY_HEIGHT <= (uint64_t)logicalH * ARDUBOY_DISPLAY_WIDTH) {
		dstW = logicalW;
		dstH = (logicalW * ARDUBOY_DISPLAY_HEIGHT) / ARDUBOY_DISPLAY_WIDTH;
	}
	else {
		dstH = logicalH;
		dstW = (logicalH * ARDUBOY_DISPLAY_WIDTH) / ARDUBOY_DISPLAY_HEIGHT;
	}
	dstX = (logicalW - dstW) / 2u;

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

	static_assert(ARDUBOY_EXPANDED_LINE_PIXELS == 160u,
		"Arduboy expansion table must cover the 160-pixel scaled display height");
	mPresentColFirst = (uint16_t)((DISP_WIDTH - dstH) / 2u);
	mPresentColEnd = (uint16_t)(mPresentColFirst + dstH);
	arduboyPrvBuildExpandPixels();

	memset(dispGetFb(), 0, DISP_WIDTH * DISP_HEIGHT * DISP_BPP / 8);
	mPresentLastState = ARDUBOY_PRESENT_STATE_INVALID;
}

void arduboyRefreshDisplayOptions(void)
{
	(void)arduboyPrvRuntime();
	dispSetFramerate(ARDUBOY_PRESENT_RATE);
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

static void __attribute__((section(".fastcode.arduboy_present"), noinline, noclone))
	arduboyPrvPresentFrame(bool force)
{
	uint16_t *fb = (uint16_t*)dispGetFb();
	bool displayOn = mArdens.display.display_on;
	bool inverted = mArdens.display.inverse_display;
	bool comScanReverse = mArdens.display.com_scan_direction;
	uint8_t presentState = (displayOn ? ARDUBOY_PRESENT_STATE_DISPLAY_ON : 0u) |
		(inverted ? ARDUBOY_PRESENT_STATE_INVERTED : 0u) |
		(comScanReverse ? 0x04u : 0u);
	bool clearFrame = force || presentState != mPresentLastState;
	uint16_t offColor = inverted ? 0xffffu : 0x0000u;

	if (!force && !clearFrame && !mArdens.displayDirty)
		return;
	if (clearFrame) {
		arduboyPrvFill(fb, DISP_WIDTH * DISP_HEIGHT, offColor);
		mPresentLastState = presentState;
	}
	if (!displayOn) {
		mArdens.displayDirty = false;
		return;
	}

	uint32_t expandState = (comScanReverse ? 1u : 0u) | (mPresentFlipped ? 2u : 0u);
	for (uint32_t y = mPresentRowFirst; y < mPresentRowEnd; y++) {
		uint16_t srcX = mArdens.presentSrcX[y];

		if (srcX == ARDUBOY_PRESENT_INVALID)
			continue;
		srcX = ARDUBOY_DISPLAY_WIDTH - 1u - srcX;
		uint32_t dstY = mPresentFlipped ? DISP_HEIGHT - 1u - y : y;
		uint16_t *dst = fb + dstY * DISP_WIDTH + mPresentColFirst;
		for (uint32_t group = 0; group < ARDUBOY_EXPANDED_PAGE_COUNT; group++) {
			uint8_t page = mExpandPages[expandState][group];
			uint8_t value = mArdens.display.ram[(uint32_t)page * ARDUBOY_DISPLAY_WIDTH + srcX];
			uint16_t *groupDst = dst + group * ARDUBOY_EXPANDED_PAGE_PIXELS;
			const uint8_t *bits = mExpandBits[expandState][group];
			uint16_t invertMask = inverted ? 0xffffu : 0u;
			for (uint32_t x = 0; x < ARDUBOY_EXPANDED_PAGE_PIXELS; x++)
				groupDst[x] = (uint16_t)-(uint16_t)((value & bits[x]) != 0u) ^ invertMask;
		}
	}

	mArdens.displayUpdates++;
	mArdens.displayDirty = false;
	mArdens.displayEverOn = true;
	if (!mArdens.visibleFrameSeen && arduboyPrvVramHasNonzero())
		mArdens.visibleFrameSeen = true;
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

static uint32_t arduboyPrvDebugDrawChar(uint16_t *fb, uint32_t x, uint32_t y,
	char ch, enum Font font, uint16_t color)
{
	FontGlyphInfo gi;

	if (!fontGetGlyphInfo(&gi, font, (unsigned char)ch))
		return 4;
	for (uint32_t yy = 0; yy < gi.height; yy++)
		for (uint32_t xx = 0; xx < gi.width; xx++)
			if (fontGetGlyphPixel(&gi, (uint_fast8_t)yy, (uint_fast8_t)xx))
				arduboyPrvDebugPutPixel(fb, x + xx, y + yy, color);
	return gi.width + 1u;
}

static void arduboyPrvDebugDrawText(uint16_t *fb, uint32_t x, uint32_t y,
	const char *text, uint16_t color)
{
	while (*text)
		x += arduboyPrvDebugDrawChar(fb, x, y, *text++, FontSmall, color);
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
	arduboyPrvDebugDrawText(fb, 14, 208, "FN: MENU  B: EXIT FROM MENU", 0x07e0u);
	dispPrvWaitForScanoutStart();
}

static void arduboyPrvFailureLoop(const char *reason)
{
	mArdens.failed = true;
	while (!mArdens.abortRequested) {
		arduboyPrvShowFailureScreen(reason);
		if (mArdens.ledsTick)
			mArdens.ledsTick();
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

static void arduboyPrvApplyButtons(uint_fast8_t keys)
{
	mArdens.cpu.data[ARDUBOY_AVR_PINB] =
		(uint8_t)((mArdens.cpu.data[ARDUBOY_AVR_PINB] & 0xefu) | ((keys & KEY_BIT_A) ? 0u : 0x10u));
	mArdens.cpu.data[ARDUBOY_AVR_PINE] =
		(uint8_t)((mArdens.cpu.data[ARDUBOY_AVR_PINE] & 0xbfu) | ((keys & KEY_BIT_B) ? 0u : 0x40u));
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

static void arduboyPrvLogBootProgress(uint64_t now)
{
	if (mArdens.visibleFrameSeen)
		return;
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

static void arduboyPrvFxSetSelected(ArduboyArdensRuntime *runtime, bool selected)
{
	if (runtime->fxSelected == selected)
		return;
	runtime->fxSelected = selected;
	if (!selected) {
		runtime->fxSpiState = ArduboyFxSpiIdle;
		runtime->fxAddressBytes = 0;
		runtime->fxJedecByte = 0;
	}
}

static void arduboyPrvFxUpdateSelected(ArduboyArdensRuntime *runtime)
{
	uint8_t ddrd = runtime->cpu.data[ARDUBOY_AVR_DDRD];
	uint8_t portd = runtime->cpu.data[ARDUBOY_AVR_PORTD];
	uint8_t ddre = runtime->cpu.data[ARDUBOY_AVR_DDRE];
	uint8_t porte = runtime->cpu.data[ARDUBOY_AVR_PORTE];
	bool selected = ((ddrd & (1u << 1)) && !(portd & (1u << 1))) ||
		 ((ddrd & (1u << 2)) && !(portd & (1u << 2))) ||
		 ((ddre & (1u << 2)) && !(porte & (1u << 2)));

	arduboyPrvFxSetSelected(runtime, selected);
}

static uint8_t arduboyPrvFxRead(const ArduboyArdensRuntime *runtime, uint32_t address)
{
	address &= ARDUBOY_FX_ADDRESS_SPACE - 1u;
	if (address == 0)
		return 0x41u;
	if (address == 1)
		return 0x52u;
	if (runtime->fxData && address >= runtime->fxGuestBase &&
			address - runtime->fxGuestBase < runtime->fxDataSize)
		return runtime->fxData[address - runtime->fxGuestBase];
	return 0xffu;
}

static uint8_t arduboyPrvFxTransfer(ArduboyArdensRuntime *runtime, uint8_t byte)
{
	if (!runtime->fxSelected)
		return 0xffu;
	if (runtime->fxSpiState == ArduboyFxSpiReadAddress) {
		runtime->fxAddress = ((runtime->fxAddress << 8) | byte) & (ARDUBOY_FX_ADDRESS_SPACE - 1u);
		if (++runtime->fxAddressBytes == 3)
			runtime->fxSpiState = ArduboyFxSpiReadData;
		return 0;
	}
	if (runtime->fxSpiState == ArduboyFxSpiReadData) {
		uint8_t result = arduboyPrvFxRead(runtime, runtime->fxAddress);

		runtime->fxAddress = (runtime->fxAddress + 1u) & (ARDUBOY_FX_ADDRESS_SPACE - 1u);
		return result;
	}
	if (runtime->fxSpiState == ArduboyFxSpiReadStatus)
		return 0;
	if (runtime->fxSpiState == ArduboyFxSpiReadJedec) {
		static const uint8_t jedec[] = {0xefu, 0x40u, 0x18u};
		uint8_t result = runtime->fxJedecByte < sizeof(jedec) ? jedec[runtime->fxJedecByte] : 0xffu;

		runtime->fxJedecByte++;
		return result;
	}

	if (!runtime->fxAwake) {
		if (byte == 0xabu)
			runtime->fxAwake = true;
		return 0;
	}
	switch (byte) {
	case 0x03u:
		runtime->fxAddress = 0;
		runtime->fxAddressBytes = 0;
		runtime->fxSpiState = ArduboyFxSpiReadAddress;
		break;
	case 0x05u:
		runtime->fxSpiState = ArduboyFxSpiReadStatus;
		break;
	case 0x9fu:
		runtime->fxJedecByte = 0;
		runtime->fxSpiState = ArduboyFxSpiReadJedec;
		break;
	case 0xb9u:
		runtime->fxAwake = false;
		break;
	default:
		break;
	}
	return 0;
}

static void arduboyPrvCapturePort(void *context, uint16_t ptr, uint8_t value)
{
	(void)ptr;
	(void)value;
	ArduboyArdensRuntime *runtime = (ArduboyArdensRuntime*)context;

	if (runtime)
		arduboyPrvFxUpdateSelected(runtime);
}

static uint8_t __attribute__((section(".fastcode.arduboy_spi_sink"), noinline, noclone)) arduboyPrvCaptureSpiByte(
	void *context, uint8_t byte, uint8_t displayPort)
{
	ArduboyArdensRuntime *runtime = (ArduboyArdensRuntime*)context;
	if (!runtime)
		return 0xffu;

	bool trackDiagnostics = !runtime->visibleFrameSeen;
	if (trackDiagnostics) {
		runtime->spiWrites++;
		runtime->lastSpiByte = byte;
		runtime->lastPortD = displayPort;
	}
	arduboyPrvFxUpdateSelected(runtime);
	if (!(displayPort & (1u << 6))) {
		if (displayPort & (1u << 4)) {
			runtime->display.send_data(byte);
			if (trackDiagnostics)
				runtime->dataWrites++;
		}
		else {
			runtime->display.send_command(byte);
			if (trackDiagnostics)
				runtime->commandWrites++;
		}
		runtime->displayDirty = true;
	}
	else if (!runtime->fxSelected && trackDiagnostics)
		runtime->ignoredSpiWrites++;
	if (runtime->fxSelected)
		return arduboyPrvFxTransfer(runtime, byte);
	return 0xffu;
}

static bool __attribute__((section(".fastcode.arduboy_spi_bulk"), noinline, noclone))
	arduboyPrvCaptureSpiBulk(void *context, const uint8_t *bytes, uint32_t count, uint8_t displayPort)
{
	ArduboyArdensRuntime *runtime = (ArduboyArdensRuntime*)context;
	if (!runtime || !bytes || count == 0)
		return false;
	arduboyPrvFxUpdateSelected(runtime);
	/* This superinstruction only represents Arduboy2's framebuffer transfer.
	 * Command traffic and FX-selected transfers retain byte-exact emulation. */
	if (runtime->fxSelected || (displayPort & (1u << 6)) || !(displayPort & (1u << 4)))
		return false;

	for (uint32_t i = 0; i < count; i++)
		runtime->display.send_data(bytes[i]);
	runtime->lastSpiByte = bytes[count - 1u];
	runtime->lastPortD = displayPort;
	runtime->displayDirty = true;
	runtime->bulkDisplayHits++;
	bool trackDiagnostics = !runtime->visibleFrameSeen;
	if (trackDiagnostics) {
		runtime->spiWrites += count;
		runtime->dataWrites += count;
	}
	return true;
}

void arduboyAbort(void)
{
	(void)arduboyPrvRuntime();
	mArdens.abortRequested = true;
}

static void arduboyPrvInitFxCache(void)
{
	const ArduboyFxCacheHeader *header = (const ArduboyFxCacheHeader*)ARDUBOY_FX_CACHE_META_ADDR;
	uint32_t metaOffset = ARDUBOY_FX_CACHE_REGION_SIZE - ARDUBOY_FX_CACHE_META_SIZE;

	if (header->magic != ARDUBOY_FX_CACHE_MAGIC || header->version != ARDUBOY_FX_CACHE_VERSION ||
			header->dataOffset >= metaOffset || header->dataSize > metaOffset - header->dataOffset ||
			header->dataSize > ARDUBOY_FX_ADDRESS_SPACE)
		return;
	mArdens.fxData = (const uint8_t*)(QSPI_ROM_START + header->dataOffset);
	mArdens.fxDataSize = header->dataSize;
	mArdens.fxGuestBase = (ARDUBOY_FX_ADDRESS_SPACE - header->dataSize) & (ARDUBOY_FX_ADDRESS_SPACE - 1u);
}

static void arduboyPrvFinalizeFxAddress(void)
{
	if (!mArdens.fxData || mArdens.flashUsed < 0x18u)
		return;
	if (mArdens.cpu.prog[0x14] == 0x18u && mArdens.cpu.prog[0x15] == 0x95u) {
		uint32_t page = ((uint32_t)mArdens.cpu.prog[0x16] << 8) | mArdens.cpu.prog[0x17];

		mArdens.fxGuestBase = page << 8;
	}
	pr("arduboy fx: data=%lu guest=%06lx\n", (unsigned long)mArdens.fxDataSize,
		(unsigned long)mArdens.fxGuestBase);
}

void arduboyRun(const void *rom, uint32_t romSize, void *saveRam, uint32_t saveRamSize,
	void (*ledsTick)(void))
{
	ArduboyRomInfo info;
	uint64_t frameBaseCycle;
	uint64_t nextControlCycle;

	if (!arduboyAnalyzeRom(rom, romSize, &info) || info.isPackage)
		return;

	(void)arduboyPrvRuntime();
	memset(&mArdens, 0, sizeof(mArdens));
	mArdens.saveRam = (uint8_t*)saveRam;
	mArdens.saveRamSize = saveRamSize;
	mArdens.ledsTick = ledsTick;
	mArdens.startTicks = getTime();
	mArdens.lastBootLogTicks = mArdens.startTicks;
	mArdens.lastEepromSyncTicks = mArdens.startTicks;
	arduboyPrvInitFxCache();

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
	arduboyPrvFinalizeFxAddress();
	mArdens.cpu.decode();
	arduboyPrvDetectEmbeddedHleTargets();
	mArdens.cpu.pc = 0;
	mArdens.cpu.executing_instr_pc = 0;
	mArdens.cpu.spi_datain_byte = 0xffu;
	mArdens.cpu.embedded_spi_sink = arduboyPrvCaptureSpiByte;
	mArdens.cpu.embedded_spi_bulk_sink = arduboyPrvCaptureSpiBulk;
	mArdens.cpu.embedded_spi_sink_context = &mArdens;
	mArdens.cpu.embedded_port_sink = arduboyPrvCapturePort;
	mArdens.cpu.embedded_port_sink_context = &mArdens;
#ifdef ARDUBOY_HLE_ENABLED
	mArdens.cpu.embedded_hle_sink = arduboyPrvGraphicsHle;
	mArdens.cpu.embedded_hle_sink_context = &mArdens;
#endif
	frameBaseCycle = mArdens.cpu.cycle_count;
	nextControlCycle = frameBaseCycle + ARDUBOY_CONTROL_POLL_CYCLES;
	dispPrvFrameCtrReset();

	pr("arduboy ardens start: flash=%lu save=%lu backend=single-core audio=disabled\n",
		(unsigned long)mArdens.flashUsed, (unsigned long)saveRamSize);

	while (!mArdens.abortRequested) {
		uint64_t frameEnd = frameBaseCycle +
			((mArdens.frameCount + 1u) * ARDUBOY_CPU_FREQ) / ARDUBOY_PRESENT_RATE;
		uint64_t now;

		if (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER) {
			arduboyPrvSyncEepromToSave(true);
			arduboyPortInGameMenu();
			while (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER)
				;
			if (mArdens.abortRequested)
				break;
			dispPrvFrameCtrReset();
		}

		arduboyPrvApplyButtons(uiGetKeysRaw());
		while (!mArdens.abortRequested && mArdens.cpu.cycle_count < frameEnd) {
			if (mArdens.cpu.pc >= ARDUBOY_FLASH_SIZE / 2u) {
				mArdens.failReason = ArduboyArdensFailInvalidPc;
				mArdens.failed = true;
				break;
			}
			mArdens.lastOpcode = mArdens.cpu.decoded_prog[mArdens.cpu.pc].word;
			mArdens.cpu.advance_cycle();
			if (mArdens.cpu.cycle_count >= nextControlCycle) {
				nextControlCycle = mArdens.cpu.cycle_count + ARDUBOY_CONTROL_POLL_CYCLES;
				arduboyPrvApplyButtons(uiGetKeysRaw());
			}
		}
		if (mArdens.abortRequested || mArdens.failed)
			break;

		arduboyPrvPresentFrame(mArdens.frameCount == 0);
		if (mArdens.ledsTick)
			mArdens.ledsTick();
		arduboyPrvSyncEepromToSave(false);
		mArdens.frameCount++;

		now = getTime();
		arduboyPrvLogBootProgress(now);
		if (!mArdens.visibleFrameSeen && now - mArdens.startTicks >= ARDUBOY_BOOT_WATCHDOG_TICKS) {
			mArdens.failReason = (mArdens.spiWrites || mArdens.commandWrites || mArdens.dataWrites) ?
				ArduboyArdensFailBootWatchdog : ArduboyArdensFailNoDisplayIo;
			mArdens.failed = true;
			break;
		}
		dispPrvFrameCtrWait();
	}

	if (mArdens.failed)
		arduboyPrvStop(mArdens.failReason);
	arduboyPrvSyncEepromToSave(true);
	dispSetFramerate(ARDUBOY_FRAME_RATE);
}
