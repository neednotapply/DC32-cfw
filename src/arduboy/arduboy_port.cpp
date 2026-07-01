#include "arduboy/arduboy.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern "C" {
#include "audioPwm.h"
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "mbc.h"
#include "memMap.h"
#include "printf.h"
#include "qspi.h"
#include "timebase.h"
#include "ui.h"
}

#include "miniz.h"

#define ARDUBOY_CPU_FREQ 16000000u
#define ARDUBOY_FRAME_CYCLES (ARDUBOY_CPU_FREQ / ARDUBOY_FRAME_RATE)
#define ARDUBOY_FRAME_STEP_GUARD (ARDUBOY_CPU_FREQ / 2u)
#define ARDUBOY_ZIP_LOCAL_SIG 0x04034b50u
#define ARDUBOY_ZIP_CENTRAL_SIG 0x02014b50u
#define ARDUBOY_ZIP_EOCD_SIG 0x06054b50u
#define ARDUBOY_ZIP_METHOD_STORED 0u
#define ARDUBOY_ZIP_METHOD_DEFLATE 8u
#define ARDUBOY_ZIP_GP_ENCRYPTED 0x0001u
#define ARDUBOY_ZIP64_SENTINEL 0xffffffffu
#define ARDUBOY_FLASH_WRITE_CHUNK QSPI_WRITE_GRANULARITY
#define ARDUBOY_DATA_SIZE 0x0b00u
#define ARDUBOY_RAM_START 0x0100u
#define ARDUBOY_RAM_END 0x0affu
#define ARDUBOY_TIMER0_OVERFLOW_CYCLES 16384u
#define ARDUBOY_VECTOR_TIMER0_OVF 0x002eu
#define ARDUBOY_BLOCK_CACHE_SIZE 128u
#define ARDUBOY_BLOCK_MAX_OPS 8u
#define ARDUBOY_HLE_TARGETS 96u
#define ARDUBOY_HLE_SCAN_CALL_TARGETS 96u
#define ARDUBOY_HLE_SCAN_MAX_WORDS 96u
#define ARDUBOY_ADDR_UNSET 0xffffu
#define ARDUBOY_COLOR_BLACK 0u
#define ARDUBOY_COLOR_WHITE 1u
#define ARDUBOY_COLOR_INVERT 2u
#define ARDUBOY_EEPROM_SYNC_CYCLES ARDUBOY_CPU_FREQ
#define ARDUBOY_NO_DISPLAY_CYCLES (ARDUBOY_CPU_FREQ * 2u)
#define ARDUBOY_BLACK_SCREEN_CYCLES (ARDUBOY_CPU_FREQ * 2u)
#define ARDUBOY_BOOT_WATCHDOG_TICKS ((TICKS_PER_SECOND * 3u) / 2u)
#define ARDUBOY_BOOT_PROGRESS_LOG_TICKS TICKS_PER_SECOND
#define ARDUBOY_SPI_HLE_HIGH_CS_DISABLE 64u
#define ARDUBOY_DISPLAY_PIN_CS 0x01u
#define ARDUBOY_DISPLAY_PIN_DC 0x02u
#define ARDUBOY_DISPLAY_PIN_RST 0x04u
#define ARDUBOY_PRESENT_INVALID 0xffffu
#define ARDUBOY_PRESENT_Y_INVALID 0xffu
#define ARDUBOY_PRESENT_STATE_INVALID 0xffu
#define ARDUBOY_PRESENT_STATE_DISPLAY_ON 0x01u
#define ARDUBOY_PRESENT_STATE_INVERTED 0x02u
#define ARDUBOY_DEBUG_LOGICAL_WIDTH DISP_HEIGHT
#define ARDUBOY_DEBUG_LOGICAL_HEIGHT DISP_WIDTH
#define ARDUBOY_IO_START 0x20u
#define ARDUBOY_IO_END 0xffu
#define ARDUBOY_FAST __attribute__((section(".fastcode")))
#define ARDUBOY_INLINE __attribute__((always_inline)) static inline
#define ARDUBOY_PERF_LOG_CYCLES ARDUBOY_CPU_FREQ

#define AVR_SREG_C 0x01u
#define AVR_SREG_Z 0x02u
#define AVR_SREG_N 0x04u
#define AVR_SREG_V 0x08u
#define AVR_SREG_S 0x10u
#define AVR_SREG_H 0x20u
#define AVR_SREG_T 0x40u
#define AVR_SREG_I 0x80u

#define AVR_PINB 0x23u
#define AVR_DDRB 0x24u
#define AVR_PORTB 0x25u
#define AVR_PIND 0x29u
#define AVR_DDRD 0x2au
#define AVR_PORTD 0x2bu
#define AVR_PINE 0x2cu
#define AVR_DDRE 0x2du
#define AVR_PORTE 0x2eu
#define AVR_PINF 0x2fu
#define AVR_DDRF 0x30u
#define AVR_PORTF 0x31u
#define AVR_TIFR0 0x35u
#define AVR_EECR 0x3fu
#define AVR_EEDR 0x40u
#define AVR_EEARL 0x41u
#define AVR_EEARH 0x42u
#define AVR_GTCCR 0x43u
#define AVR_TCCR0A 0x44u
#define AVR_TCCR0B 0x45u
#define AVR_TCNT0 0x46u
#define AVR_OCR0A 0x47u
#define AVR_OCR0B 0x48u
#define AVR_PLLCSR 0x49u
#define AVR_SPCR 0x4cu
#define AVR_SPDR 0x4eu
#define AVR_SPSR 0x4du
#define AVR_WDTCSR 0x60u
#define AVR_CLKPR 0x61u
#define AVR_PRR0 0x64u
#define AVR_PRR1 0x65u
#define AVR_SPL 0x5du
#define AVR_SPH 0x5eu
#define AVR_SREG 0x5fu
#define AVR_TIMSK0 0x6eu
#define AVR_ADCL 0x78u
#define AVR_ADCH 0x79u
#define AVR_ADCSRA 0x7au
#define AVR_ADCSRB 0x7bu
#define AVR_ADMUX 0x7cu
#define AVR_DIDR2 0x7du
#define AVR_DIDR0 0x7eu
#define AVR_DIDR1 0x7fu
#define AVR_UHWCON 0xd7u
#define AVR_USBCON 0xd8u
#define AVR_USBSTA 0xd9u
#define AVR_USBINT 0xdau
#define AVR_UDCON 0xe0u
#define AVR_UDINT 0xe1u
#define AVR_UDIEN 0xe2u
#define AVR_UEINTX 0xe8u
#define AVR_UENUM 0xe9u
#define AVR_UERST 0xeau
#define AVR_UECONX 0xebu
#define AVR_UECFG0X 0xecu
#define AVR_UECFG1X 0xedu
#define AVR_UESTA0X 0xeeu
#define AVR_UEINT 0xf4u

#define AVR_TIFR0_TOV0 0x01u
#define AVR_TIFR0_OCF0A 0x02u
#define AVR_TIFR0_OCF0B 0x04u
#define AVR_TIMSK0_TOIE0 0x01u
#define AVR_ADCSRA_ADSC 0x40u
#define AVR_ADCSRA_ADIF 0x10u
#define AVR_PLLCSR_PLOCK 0x01u
#define AVR_SPSR_SPIF 0x80u
#define AVR_USBSTA_VBUS 0x01u
#define AVR_UESTA0X_CFGOK 0x80u
#define AVR_UEINTX_TXINI 0x01u
#define AVR_UEINTX_RWAL 0x20u

#define ARDUBOY_FAIL_NONE 0u
#define ARDUBOY_FAIL_UNSUPPORTED_OPCODE 1u
#define ARDUBOY_FAIL_FRAME_GUARD 2u
#define ARDUBOY_FAIL_NO_DISPLAY_IO 3u
#define ARDUBOY_FAIL_INVALID_PC 4u
#define ARDUBOY_FAIL_HLE_BAD_RETURN 5u
#define ARDUBOY_FAIL_STACK_FAULT 6u
#define ARDUBOY_FAIL_HLE_QUARANTINED 7u
#define ARDUBOY_FAIL_BLACK_SCREEN 8u
#define ARDUBOY_FAIL_BOOT_WATCHDOG 9u
#define ARDUBOY_FAIL_SPI_NOT_CAPTURED 10u
#define ARDUBOY_HLE_CONF_HEURISTIC 1u
#define ARDUBOY_HLE_CONF_ANCHORED 2u
#define ARDUBOY_HLE_CONF_EXACT 3u
#define ARDUBOY_HLE_FLAG_DISABLED 0x01u
#define ARDUBOY_HLE_FLAG_DIRECT_CALL 0x02u
#define ARDUBOY_HLE_FLAG_SOFT_MISS 0x04u
#define ARDUBOY_STACK_OP_NONE 0u
#define ARDUBOY_STACK_OP_PUSH 1u
#define ARDUBOY_STACK_OP_POP 2u
#define ARDUBOY_STACK_OP_CALL 3u
#define ARDUBOY_STACK_OP_RET 4u
#define ARDUBOY_STACK_OP_HLE_RET 5u

#ifndef ARDUBOY_HLE_ALLOW_HEURISTIC
#define ARDUBOY_HLE_ALLOW_HEURISTIC 0
#endif

enum ArduboyAddressingMode {
	ArduboyAddrHorizontal,
	ArduboyAddrVertical,
	ArduboyAddrPage,
};

enum ArduboyHleKind {
	ArduboyHleNone,
	ArduboyHleMutedAudio,
	ArduboyHleMillis,
	ArduboyHleDelay,
	ArduboyHleDelayShort,
	ArduboyHleIdle,
	ArduboyHleNextFrame,
	ArduboyHleSetFrameRate,
	ArduboyHleSetFrameDuration,
	ArduboyHleButtonsState,
	ArduboyHleEepromRead,
	ArduboyHleEepromWrite,
	ArduboyHleEepromUpdate,
	ArduboyHleSpiTransfer,
	ArduboyHleSpiTransferRead,
	ArduboyHlePaintScreen,
	ArduboyHlePaintScreenProgmem,
	ArduboyHleDisplay,
	ArduboyHleDisplayClear,
	ArduboyHleEveryXFrames,
	ArduboyHlePollButtons,
	ArduboyHlePressed,
	ArduboyHleNotPressed,
	ArduboyHleJustPressed,
	ArduboyHleJustReleased,
	ArduboyHleClear,
	ArduboyHleFillScreen,
	ArduboyHleDrawPixel,
	ArduboyHleDrawFastHLine,
	ArduboyHleDrawFastVLine,
	ArduboyHleDrawLine,
	ArduboyHleDrawRect,
	ArduboyHleFillRect,
	ArduboyHleDrawBitmap,
	ArduboyHleSpritesDraw,
	ArduboyHleAudioEnabled,
	ArduboyHleAudioOnOff,
	ArduboyHleAudioSave,
	ArduboyHleToneNoop,
	ArduboyHleTonesNoop,
	ArduboyHleTonePlaying,
	ArduboyHleMemset,
	ArduboyHleMemcpy,
	ArduboyHleMemmove,
	ArduboyHleRgbNoop,
	ArduboyHleKindCount,
};

enum ArduboyOpKind {
	ArduboyOpUnsupported,
	ArduboyOpNop,
	ArduboyOpMovw,
	ArduboyOpMul,
	ArduboyOpMuls,
	ArduboyOpFmul,
	ArduboyOpAluReg,
	ArduboyOpImm,
	ArduboyOpLdStQ,
	ArduboyOpLdi,
	ArduboyOpIo,
	ArduboyOpRjmp,
	ArduboyOpRcall,
	ArduboyOpLds,
	ArduboyOpSts,
	ArduboyOpPop,
	ArduboyOpPush,
	ArduboyOpJmp,
	ArduboyOpCall,
	ArduboyOpAdiw,
	ArduboyOpIoBit,
	ArduboyOpBranch,
	ArduboyOpSkipRegBit,
	ArduboyOpBld,
	ArduboyOpBst,
	ArduboyOpLdPtr,
	ArduboyOpStPtr,
	ArduboyOpZMem,
	ArduboyOpUnary,
	ArduboyOpIjmp,
	ArduboyOpIcall,
	ArduboyOpRet,
	ArduboyOpReti,
	ArduboyOpLpm,
	ArduboyOpNoopSpecial,
	ArduboyOpSregBit,
};

struct ArduboyZipMember {
	uint32_t dataOffset;
	uint32_t compressedSize;
	uint32_t uncompressedSize;
	uint16_t method;
};

struct ArduboyHexParser {
	uint32_t segment;
	uint32_t highest;
	uint32_t dataBytes;
	bool eofSeen;
};

struct ArduboyFlashWriter {
	uint32_t addr;
	uint32_t pos;
	uint32_t maxSize;
	uint8_t *buf;
	uint32_t fill;
};

struct ArduboyCachedOp {
	uint16_t pc;
	uint16_t opcode;
	uint16_t next;
	uint16_t target;
	uint16_t imm16;
	uint8_t kind;
	uint8_t d;
	uint8_t r;
	uint8_t k;
	uint8_t q;
	uint8_t bit;
	uint8_t ioAddr;
	uint8_t aux;
	uint8_t cycles;
	uint8_t words;
	uint8_t control;
};

struct ArduboyBlock {
	uint16_t pc;
	uint8_t opCount;
	uint8_t valid;
	ArduboyCachedOp ops[ARDUBOY_BLOCK_MAX_OPS];
};

struct ArduboyHleTarget {
	uint16_t pc;
	uint8_t kind;
	uint8_t confidence;
	uint8_t flags;
};

struct ArduboyCallTarget {
	uint16_t pc;
	uint8_t count;
	uint8_t flags;
};

struct ArduboyMachine {
	uint8_t flash[ARDUBOY_FLASH_SIZE];
	uint8_t data[ARDUBOY_DATA_SIZE];
	uint8_t eeprom[ARDUBOY_SAVE_RAM_SIZE];
	uint8_t vram[ARDUBOY_DISPLAY_HEIGHT / 8u][ARDUBOY_DISPLAY_WIDTH];
	uint16_t pc;
	uint64_t cycles;
	uint32_t timer0Cycles;
	uint64_t nextEepromSyncCycles;
	uint32_t frameGuardTrips;
	uint32_t spiWrites;
	uint32_t spiIgnoredWrites;
	uint32_t spiSelectedWrites;
	uint32_t hleSpiWrites;
	uint32_t hleSpiIgnoredWrites;
	uint32_t displayDataWrites;
	uint32_t displayCommandWrites;
	uint32_t displayResets;
	uint32_t displayPinTransitions;
	uint32_t cacheResets;
	uint32_t blockRuns;
	uint32_t opRuns;
	uint32_t decodedBlocks;
	uint32_t cacheHits;
	uint32_t cacheMisses;
	uint32_t pollingCollapses;
	uint32_t hleQuarantines;
	uint32_t hleBadReturns;
	uint32_t hleSoftMisses;
	uint32_t hleSkippedHeuristics;
	uint16_t hotPc[4];
	uint32_t hotCount[4];
	uint16_t unsupportedOpcode;
	uint16_t unsupportedPc;
	uint16_t lastPc;
	uint16_t lastOpcode;
	uint16_t lastHlePc;
	uint16_t lastHleReturnPc;
	uint16_t lastHleEntrySp;
	uint16_t lastHleExitSp;
	uint16_t stackFaultSp;
	uint16_t stackFaultPrevSp;
	uint16_t stackFaultReturnPc;
	uint8_t displayFlags;
	uint8_t displayCs;
	uint8_t displayDi;
	uint8_t displayCommand;
	uint8_t displayRegWrites;
	uint8_t displayAddrMode;
	uint8_t cursorPage;
	uint8_t cursorColumn;
	uint8_t windowPageStart;
	uint8_t windowPageEnd;
	uint8_t windowColumnStart;
	uint8_t windowColumnEnd;
	uint8_t lastDisplayCommand;
	uint8_t lastDisplayData;
	uint8_t lastPortD;
	uint8_t lastDisplayPins;
	uint8_t timer0Pending;
	uint8_t halted;
	uint8_t unsupported;
	uint8_t abortRequested;
	uint8_t failureReason;
	uint8_t eepromDirty;
	uint8_t displayDirty;
	uint8_t displayExplicitOff;
	uint8_t hleCount;
	uint8_t lastHleKind;
	uint8_t lastHleConfidence;
	uint8_t lastHleFlags;
	uint8_t stackFaultOp;
	uint8_t frameDurationMs;
	uint16_t screenBufferAddr;
	uint16_t frameCountAddr;
	uint16_t eachFrameMillisAddr;
	uint16_t thisFrameStartAddr;
	uint16_t lastFrameDurationMsAddr;
	uint16_t justRenderedAddr;
	uint16_t currentButtonStateAddr;
	uint16_t previousButtonStateAddr;
	uint16_t cursorXAddr;
	uint16_t cursorYAddr;
	uint16_t textColorAddr;
	uint16_t textBackgroundAddr;
	uint16_t audioEnabledAddr;
	uint8_t hleCurrentButtons;
	uint8_t hlePreviousButtons;
	uint16_t hleFrameCount;
	uint8_t hleAudioEnabled;
	uint64_t bootHostTicks;
	uint64_t lastVisibleHostTicks;
	uint64_t lastBootDiagHostTicks;
	uint32_t visibleFrames;
	uint32_t bootWatchdogTrips;
	ArduboyHleTarget hleTargets[ARDUBOY_HLE_TARGETS];
	ArduboyBlock blocks[ARDUBOY_BLOCK_CACHE_SIZE];
#ifdef ARDUBOY_PERF
	uint64_t perfLastHostTicks;
	uint64_t perfLastGuestCycles;
	uint32_t perfFrames;
	uint32_t perfBlocks;
	uint32_t perfOps;
	uint32_t perfDecodedBlocks;
	uint32_t perfPresentTicks;
	uint32_t perfWaitTicks;
	uint32_t perfCacheHits;
	uint32_t perfCacheMisses;
	uint32_t perfHleCalls[ArduboyHleKindCount];
	uint32_t perfHleQuarantines;
	uint32_t perfHleMisses;
	uint16_t perfHotPc[4];
	uint32_t perfHotCount[4];
#endif
};

static_assert(sizeof(ArduboyMachine) <= QSPI_RAM_SIZE_MAX - ARDUBOY_SAVE_RAM_SIZE,
	"Arduboy hybrid machine must fit in cart RAM after EEPROM save data");

static ArduboyMachine *mMachine;
static uint8_t *mSaveRam;
static uint32_t mSaveRamSize;
static volatile bool mAbortRequested;
static uint16_t mPresentSrcX[DISP_HEIGHT];
static uint8_t mPresentSrcYPage[DISP_WIDTH];
static uint8_t mPresentSrcYMask[DISP_WIDTH];
static uint8_t mPresentSrcYPageFlipped[DISP_WIDTH];
static uint8_t mPresentSrcYMaskFlipped[DISP_WIDTH];
static uint16_t mPresentRowFirst;
static uint16_t mPresentRowEnd;
static uint16_t mPresentColFirst;
static uint16_t mPresentColEnd;
static uint8_t mPresentLastState;
static bool mPresentFlipped;

void arduboySetRotation(bool flipped)
{
	mPresentFlipped = flipped;
	mPresentLastState = ARDUBOY_PRESENT_STATE_INVALID;
	if (mMachine)
		mMachine->displayDirty = 1;
}

static bool arduboyPrvDisplayHasVisibleContent(const ArduboyMachine *m);
static void arduboyPrvSyncEepromToSave(bool force);
static void arduboyPrvStopWithRuntimeFailure(ArduboyMachine *m, uint8_t reason);

static uint16_t arduboyPrvRd16(const uint8_t *p)
{
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t arduboyPrvRd32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
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
			a = (char)(a + 'a' - 'A');
		if (b >= 'A' && b <= 'Z')
			b = (char)(b + 'a' - 'A');
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

typedef bool (*ArduboyHexDataF)(void *userData, uint32_t addr, const uint8_t *data, uint32_t len);

static bool arduboyPrvHexParse(const void *rom, uint32_t size, ArduboyHexDataF dataF, void *userData, uint32_t *flashUsedP)
{
	const char *bytes = (const char*)rom;
	uint32_t pos = 0;
	struct ArduboyHexParser parser = {0};

	while (pos < size) {
		uint32_t lineStart, lineEnd, lineLen;
		uint8_t count, addrHi, addrLo, type, checksum, sum;
		uint8_t data[255];
		uint32_t addr;

		while (pos < size && (bytes[pos] == '\r' || bytes[pos] == '\n' || bytes[pos] == ' ' || bytes[pos] == '\t'))
			pos++;
		if (pos >= size)
			break;
		if (bytes[pos] != ':')
			return false;
		lineStart = ++pos;
		lineEnd = lineStart;
		while (lineEnd < size && bytes[lineEnd] != '\r' && bytes[lineEnd] != '\n')
			lineEnd++;
		lineLen = lineEnd - lineStart;
		if (lineLen < 10 || (lineLen & 1))
			return false;
		if (!arduboyPrvHexByte(bytes + lineStart, &count) ||
			!arduboyPrvHexByte(bytes + lineStart + 2, &addrHi) ||
			!arduboyPrvHexByte(bytes + lineStart + 4, &addrLo) ||
			!arduboyPrvHexByte(bytes + lineStart + 6, &type))
			return false;
		if (lineLen != (uint32_t)(10 + count * 2))
			return false;

		sum = (uint8_t)(count + addrHi + addrLo + type);
		for (uint32_t i = 0; i < count; i++) {
			if (!arduboyPrvHexByte(bytes + lineStart + 8 + i * 2, &data[i]))
				return false;
			sum = (uint8_t)(sum + data[i]);
		}
		if (!arduboyPrvHexByte(bytes + lineStart + 8 + count * 2, &checksum))
			return false;
		sum = (uint8_t)(sum + checksum);
		if (sum)
			return false;

		addr = parser.segment + ((uint32_t)addrHi << 8) + addrLo;
		switch (type) {
			case 0x00:
				if (!count)
					break;
				if (addr >= ARDUBOY_FLASH_SIZE || count > ARDUBOY_FLASH_SIZE - addr)
					return false;
				if (dataF && !dataF(userData, addr, data, count))
					return false;
				if (parser.highest < addr + count)
					parser.highest = addr + count;
				parser.dataBytes += count;
				break;
			case 0x01:
				if (count)
					return false;
				parser.eofSeen = true;
				break;
			case 0x02:
				if (count != 2)
					return false;
				parser.segment = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4);
				break;
			case 0x03:
			case 0x05:
				break;
			case 0x04:
				if (count != 2)
					return false;
				parser.segment = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16);
				break;
			default:
				return false;
		}
		pos = lineEnd;
	}

	if (!parser.eofSeen || !parser.dataBytes)
		return false;
	if (flashUsedP)
		*flashUsedP = parser.highest;
	return true;
}

static bool arduboyPrvFindZipHexMember(const void *packageData, uint32_t packageSize, struct ArduboyZipMember *memberP)
{
	const uint8_t *zip = (const uint8_t*)packageData;
	uint32_t searchStart, eocd = UINT32_MAX, centralOffset, centralSize, pos, entries;

	if (!zip || packageSize < 22)
		return false;
	searchStart = packageSize > 22 + 0xffffu ? packageSize - 22 - 0xffffu : 0;
	for (uint32_t p = packageSize - 22; p + 1 > searchStart; p--) {
		if (arduboyPrvRd32(zip + p) == ARDUBOY_ZIP_EOCD_SIG) {
			eocd = p;
			break;
		}
		if (!p)
			break;
	}
	if (eocd == UINT32_MAX)
		return false;

	entries = arduboyPrvRd16(zip + eocd + 10);
	centralSize = arduboyPrvRd32(zip + eocd + 12);
	centralOffset = arduboyPrvRd32(zip + eocd + 16);
	if (centralOffset > packageSize || centralSize > packageSize - centralOffset)
		return false;

	pos = centralOffset;
	for (uint32_t entry = 0; entry < entries && pos + 46 <= packageSize; entry++) {
		const uint8_t *cent = zip + pos;
		uint16_t flags, method, nameLen, extraLen, commentLen;
		uint32_t compressedSize, uncompressedSize, localOffset;
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
		if (pos + 46u + nameLen + extraLen + commentLen > centralOffset + centralSize ||
			pos + 46u + nameLen + extraLen + commentLen > packageSize)
			return false;
		name = (const char*)(cent + 46);
		if (!(flags & ARDUBOY_ZIP_GP_ENCRYPTED) &&
			(method == ARDUBOY_ZIP_METHOD_STORED || method == ARDUBOY_ZIP_METHOD_DEFLATE) &&
			compressedSize != ARDUBOY_ZIP64_SENTINEL &&
			uncompressedSize != ARDUBOY_ZIP64_SENTINEL &&
			arduboyPrvEndsWithNoCase(name, nameLen, ".hex")) {
			const uint8_t *local;
			uint16_t localNameLen, localExtraLen;

			if (localOffset > packageSize || packageSize - localOffset < 30)
				return false;
			local = zip + localOffset;
			if (arduboyPrvRd32(local) != ARDUBOY_ZIP_LOCAL_SIG)
				return false;
			localNameLen = arduboyPrvRd16(local + 26);
			localExtraLen = arduboyPrvRd16(local + 28);
			if (localOffset + 30u + localNameLen + localExtraLen > packageSize ||
				compressedSize > packageSize - (localOffset + 30u + localNameLen + localExtraLen))
				return false;
			memberP->dataOffset = localOffset + 30u + localNameLen + localExtraLen;
			memberP->compressedSize = compressedSize;
			memberP->uncompressedSize = uncompressedSize;
			memberP->method = method;
			return true;
		}
		pos += 46u + nameLen + extraLen + commentLen;
	}
	return false;
}

static bool arduboyPrvLooksLikePackage(const void *rom, uint32_t size)
{
	const uint8_t *bytes = (const uint8_t*)rom;

	return bytes && size >= 4 && arduboyPrvRd32(bytes) == ARDUBOY_ZIP_LOCAL_SIG;
}

bool arduboyAnalyzeRom(const void *rom, uint32_t size, struct ArduboyRomInfo *info)
{
	uint32_t flashUsed = 0;
	bool isPackage = false;

	if (!rom || !size)
		return false;
	if (arduboyPrvLooksLikePackage(rom, size)) {
		struct ArduboyZipMember member;

		if (!arduboyPrvFindZipHexMember(rom, size, &member))
			return false;
		isPackage = true;
		flashUsed = ARDUBOY_FLASH_SIZE;
	}
	else if (!arduboyPrvHexParse(rom, size, NULL, NULL, &flashUsed))
		return false;

	if (info) {
		memset(info, 0, sizeof(*info));
		memcpy(info->name, "ARDUBOY", sizeof("ARDUBOY"));
		info->romSize = size;
		info->saveRamSize = ARDUBOY_SAVE_RAM_SIZE;
		info->flashUsed = flashUsed;
		info->isPackage = isPackage;
	}
	return true;
}

static bool arduboyPrvFlashWriterFlush(struct ArduboyFlashWriter *writer, bool final)
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

static bool arduboyPrvFlashWriterWrite(struct ArduboyFlashWriter *writer, const uint8_t *data, uint32_t len)
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
	struct ArduboyZipMember member;
	const uint8_t *packageBytes = (const uint8_t*)packageData;
	struct ArduboyFlashWriter writer;

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

static uint16_t arduboyPrvFlashWord(const ArduboyMachine *m, uint16_t pc)
{
	uint32_t addr = (uint32_t)pc << 1;

	if (addr + 1 >= ARDUBOY_FLASH_SIZE)
		return 0xffffu;
	return (uint16_t)m->flash[addr] | ((uint16_t)m->flash[addr + 1] << 8);
}

static int16_t arduboyPrvSign(uint16_t value, uint8_t bits)
{
	uint16_t mask = (uint16_t)(1u << (bits - 1u));

	value &= (uint16_t)((1u << bits) - 1u);
	return (int16_t)((value ^ mask) - mask);
}

static uint16_t arduboyPrvGetSp(const ArduboyMachine *m)
{
	return (uint16_t)m->data[AVR_SPL] | ((uint16_t)m->data[AVR_SPH] << 8);
}

static void arduboyPrvSetSp(ArduboyMachine *m, uint16_t sp)
{
	m->data[AVR_SPL] = (uint8_t)sp;
	m->data[AVR_SPH] = (uint8_t)(sp >> 8);
}

static uint16_t arduboyPrvGetPair(const ArduboyMachine *m, uint8_t reg)
{
	return (uint16_t)m->data[reg] | ((uint16_t)m->data[reg + 1u] << 8);
}

static void arduboyPrvSetPair(ArduboyMachine *m, uint8_t reg, uint16_t value)
{
	m->data[reg] = (uint8_t)value;
	m->data[reg + 1u] = (uint8_t)(value >> 8);
}

static void arduboyPrvSetSreg(ArduboyMachine *m, uint8_t mask, uint8_t value)
{
	m->data[AVR_SREG] = (uint8_t)((m->data[AVR_SREG] & ~mask) | (value & mask));
}

static void arduboyPrvStackFault(ArduboyMachine *m, uint8_t stackOp,
	uint16_t prevSp, uint16_t faultSp)
{
	if (!m->unsupported && !m->halted) {
		m->unsupported = 1;
		m->failureReason = ARDUBOY_FAIL_STACK_FAULT;
		m->unsupportedPc = m->pc;
		m->unsupportedOpcode = arduboyPrvFlashWord(m, m->pc);
		m->stackFaultOp = stackOp;
		m->stackFaultPrevSp = prevSp;
		m->stackFaultSp = faultSp;
		m->stackFaultReturnPc = 0xffffu;
		m->lastHleExitSp = faultSp;
	}
}

static uint8_t arduboyPrvReadData(ArduboyMachine *m, uint16_t addr);
static void arduboyPrvWriteData(ArduboyMachine *m, uint16_t addr, uint8_t value);
static void ARDUBOY_FAST arduboyPrvFlushCycles(ArduboyMachine *m, uint32_t *cyclesP);
static bool ARDUBOY_FAST arduboyPrvRjmpTargets(uint16_t pc, uint16_t opcode, uint16_t target);
static bool arduboyPrvDirectCallTarget(const ArduboyMachine *m, uint16_t pc, uint16_t *targetP);
static bool arduboyPrvDirectJumpTarget(const ArduboyMachine *m, uint16_t pc, uint16_t *targetP);
static void arduboyPrvApplyButtons(ArduboyMachine *m);

static void arduboyPrvPush8(ArduboyMachine *m, uint8_t value, uint8_t stackOp)
{
	uint16_t sp = arduboyPrvGetSp(m);

	if (sp >= ARDUBOY_RAM_START && sp < ARDUBOY_DATA_SIZE)
		m->data[sp] = value;
	else
		arduboyPrvStackFault(m, stackOp, sp, sp);
	arduboyPrvSetSp(m, (uint16_t)(sp - 1u));
}

static uint8_t arduboyPrvPop8(ArduboyMachine *m, uint8_t stackOp)
{
	uint16_t prevSp = arduboyPrvGetSp(m);
	uint16_t sp = (uint16_t)(prevSp + 1u);

	arduboyPrvSetSp(m, sp);
	if (sp >= ARDUBOY_RAM_START && sp < ARDUBOY_DATA_SIZE)
		return m->data[sp];
	arduboyPrvStackFault(m, stackOp, prevSp, sp);
	return 0xffu;
}

static void arduboyPrvPush16(ArduboyMachine *m, uint16_t value, uint8_t stackOp)
{
	arduboyPrvPush8(m, (uint8_t)(value >> 8), stackOp);
	arduboyPrvPush8(m, (uint8_t)value, stackOp);
}

static uint16_t arduboyPrvPop16(ArduboyMachine *m, uint8_t stackOp)
{
	uint8_t lo = arduboyPrvPop8(m, stackOp);
	uint8_t hi = arduboyPrvPop8(m, stackOp);
	uint16_t value = (uint16_t)lo | ((uint16_t)hi << 8);

	if (m->failureReason == ARDUBOY_FAIL_STACK_FAULT)
		m->stackFaultReturnPc = value;
	return value;
}

static bool arduboyPrvPcIsExecutable(const ArduboyMachine *m, uint16_t pc)
{
	return pc < (ARDUBOY_FLASH_SIZE / 2u) && arduboyPrvFlashWord(m, pc) != 0xffffu;
}

static bool arduboyPrvPeekReturnPc(const ArduboyMachine *m, uint16_t *spP, uint16_t *returnPcP)
{
	uint16_t sp = arduboyPrvGetSp(m);
	uint16_t loAddr = (uint16_t)(sp + 1u);
	uint16_t hiAddr = (uint16_t)(sp + 2u);

	if (loAddr >= ARDUBOY_DATA_SIZE || hiAddr >= ARDUBOY_DATA_SIZE)
		return false;
	if (sp < ARDUBOY_RAM_START - 1u || sp >= ARDUBOY_RAM_END)
		return false;
	*spP = sp;
	*returnPcP = (uint16_t)m->data[loAddr] | ((uint16_t)m->data[hiAddr] << 8);
	return true;
}

static bool arduboyPrvReturnWasDirectCallTo(const ArduboyMachine *m, uint16_t returnPc, uint16_t targetPc)
{
	uint16_t found;

	if (returnPc > 0u &&
		arduboyPrvDirectCallTarget(m, (uint16_t)(returnPc - 1u), &found)) {
		uint16_t wrapped;

		if (found == targetPc)
			return true;
		if (arduboyPrvDirectJumpTarget(m, found, &wrapped) && wrapped == targetPc)
			return true;
	}
	if (returnPc > 1u &&
		arduboyPrvDirectCallTarget(m, (uint16_t)(returnPc - 2u), &found)) {
		uint16_t wrapped;

		if (found == targetPc)
			return true;
		if (arduboyPrvDirectJumpTarget(m, found, &wrapped) && wrapped == targetPc)
			return true;
	}
	return false;
}

static void arduboyPrvQuarantineHleTarget(ArduboyMachine *m, ArduboyHleTarget *target,
	uint16_t entrySp, uint16_t returnPc)
{
	target->flags |= ARDUBOY_HLE_FLAG_DISABLED;
	m->lastHlePc = target->pc;
	m->lastHleKind = target->kind;
	m->lastHleConfidence = target->confidence;
	m->lastHleFlags = target->flags;
	m->lastHleEntrySp = entrySp;
	m->lastHleExitSp = arduboyPrvGetSp(m);
	m->lastHleReturnPc = returnPc;
	m->hleQuarantines++;
	if (!arduboyPrvPcIsExecutable(m, returnPc))
		m->hleBadReturns++;
#ifdef ARDUBOY_PERF
	m->perfHleQuarantines++;
#endif
}

static void arduboyPrvSkipHeuristicHleTarget(ArduboyMachine *m, ArduboyHleTarget *target)
{
	target->flags |= ARDUBOY_HLE_FLAG_DISABLED;
	m->lastHlePc = target->pc;
	m->lastHleKind = target->kind;
	m->lastHleConfidence = target->confidence;
	m->lastHleFlags = target->flags;
	m->lastHleEntrySp = arduboyPrvGetSp(m);
	m->lastHleExitSp = m->lastHleEntrySp;
	m->lastHleReturnPc = 0xffffu;
	m->hleSkippedHeuristics++;
}

static void arduboyPrvSoftMissHleTarget(ArduboyMachine *m, ArduboyHleTarget *target,
	uint16_t entrySp, uint16_t returnPc)
{
	m->lastHlePc = target->pc;
	m->lastHleKind = target->kind;
	m->lastHleConfidence = target->confidence;
	m->lastHleFlags = (uint8_t)(target->flags | ARDUBOY_HLE_FLAG_SOFT_MISS);
	m->lastHleEntrySp = entrySp;
	m->lastHleExitSp = arduboyPrvGetSp(m);
	m->lastHleReturnPc = returnPc;
	m->hleSoftMisses++;
}

static bool arduboyPrvValidateHleEntry(ArduboyMachine *m, ArduboyHleTarget *target)
{
	uint16_t entrySp = 0xffffu;
	uint16_t returnPc = 0xffffu;

	m->lastHlePc = target->pc;
	m->lastHleKind = target->kind;
	m->lastHleConfidence = target->confidence;
	m->lastHleFlags = target->flags;
	if (!arduboyPrvPeekReturnPc(m, &entrySp, &returnPc)) {
		arduboyPrvQuarantineHleTarget(m, target, entrySp, returnPc);
		return false;
	}
	m->lastHleEntrySp = entrySp;
	m->lastHleExitSp = entrySp;
	m->lastHleReturnPc = returnPc;
	if (!arduboyPrvPcIsExecutable(m, returnPc)) {
		arduboyPrvQuarantineHleTarget(m, target, entrySp, returnPc);
		return false;
	}
	if (!arduboyPrvReturnWasDirectCallTo(m, returnPc, target->pc)) {
		arduboyPrvSoftMissHleTarget(m, target, entrySp, returnPc);
		return false;
	}
	target->flags |= ARDUBOY_HLE_FLAG_DIRECT_CALL;
	return true;
}

static uint8_t arduboyPrvDisplayClamp(uint8_t value, uint8_t limit)
{
	return value < limit ? value : (uint8_t)(limit - 1u);
}

static void arduboyPrvDisplayClearCommand(ArduboyMachine *m)
{
	m->displayCommand = 0;
	m->displayRegWrites = 0;
}

static void arduboyPrvDisplayReset(ArduboyMachine *m)
{
	memset(m->vram, 0, sizeof(m->vram));
	m->displayDirty = 1;
	m->cursorPage = 0;
	m->cursorColumn = 0;
	m->windowPageStart = 0;
	m->windowColumnStart = 0;
	m->windowPageEnd = (ARDUBOY_DISPLAY_HEIGHT / 8u) - 1u;
	m->windowColumnEnd = ARDUBOY_DISPLAY_WIDTH - 1u;
	m->displayFlags = ARDUBOY_PRESENT_STATE_DISPLAY_ON;
	m->displayExplicitOff = 0;
	m->displayAddrMode = ArduboyAddrPage;
	m->displayCs = 1;
	m->displayDi = 0;
	m->lastDisplayPins = ARDUBOY_DISPLAY_PIN_CS;
	m->displayResets++;
	arduboyPrvDisplayClearCommand(m);
}

static void arduboyPrvDisplayWriteData(ArduboyMachine *m, uint8_t data)
{
	m->displayDataWrites++;
	m->lastDisplayData = data;
	if (m->cursorPage < ARDUBOY_DISPLAY_HEIGHT / 8u && m->cursorColumn < ARDUBOY_DISPLAY_WIDTH) {
		if (m->vram[m->cursorPage][m->cursorColumn] != data)
			m->displayDirty = 1;
		m->vram[m->cursorPage][m->cursorColumn] = data;
	}

	switch (m->displayAddrMode) {
		case ArduboyAddrVertical:
			if (++m->cursorPage > m->windowPageEnd || m->cursorPage >= ARDUBOY_DISPLAY_HEIGHT / 8u) {
				m->cursorPage = m->windowPageStart;
				if (++m->cursorColumn > m->windowColumnEnd || m->cursorColumn >= ARDUBOY_DISPLAY_WIDTH)
					m->cursorColumn = m->windowColumnStart;
			}
			break;
		case ArduboyAddrHorizontal:
			if (++m->cursorColumn > m->windowColumnEnd || m->cursorColumn >= ARDUBOY_DISPLAY_WIDTH) {
				m->cursorColumn = m->windowColumnStart;
				if (++m->cursorPage > m->windowPageEnd || m->cursorPage >= ARDUBOY_DISPLAY_HEIGHT / 8u)
					m->cursorPage = m->windowPageStart;
			}
			break;
		case ArduboyAddrPage:
		default:
			if (++m->cursorColumn >= ARDUBOY_DISPLAY_WIDTH)
				m->cursorColumn = 0;
			break;
	}
}

static void arduboyPrvDisplayCommandArg(ArduboyMachine *m, uint8_t data)
{
	switch (m->displayCommand) {
		case 0x20:
			if (data <= ArduboyAddrPage)
				m->displayAddrMode = data;
			arduboyPrvDisplayClearCommand(m);
			return;
		case 0x21:
			if (m->displayRegWrites == 2u) {
				m->cursorColumn = arduboyPrvDisplayClamp(data, ARDUBOY_DISPLAY_WIDTH);
				m->windowColumnStart = m->cursorColumn;
				m->displayRegWrites = 1u;
			}
			else {
				m->windowColumnEnd = arduboyPrvDisplayClamp(data, ARDUBOY_DISPLAY_WIDTH);
				arduboyPrvDisplayClearCommand(m);
			}
			return;
		case 0x22:
			if (m->displayRegWrites == 2u) {
				m->cursorPage = arduboyPrvDisplayClamp(data, ARDUBOY_DISPLAY_HEIGHT / 8u);
				m->windowPageStart = m->cursorPage;
				m->displayRegWrites = 1u;
			}
			else {
				m->windowPageEnd = arduboyPrvDisplayClamp(data, ARDUBOY_DISPLAY_HEIGHT / 8u);
				arduboyPrvDisplayClearCommand(m);
			}
			return;
		case 0x26:
		case 0x27:
		case 0x29:
		case 0x2a:
		case 0xa3:
			if (m->displayRegWrites && !--m->displayRegWrites)
				arduboyPrvDisplayClearCommand(m);
			return;
		default:
			arduboyPrvDisplayClearCommand(m);
			return;
	}
}

static void arduboyPrvDisplayWriteCommand(ArduboyMachine *m, uint8_t command)
{
	m->displayCommandWrites++;
	m->lastDisplayCommand = command;

	if (m->displayCommand) {
		arduboyPrvDisplayCommandArg(m, command);
		return;
	}

	if (command <= 0x0fu) {
		if (m->displayAddrMode == ArduboyAddrPage)
			m->cursorColumn = (uint8_t)((m->cursorColumn & 0xf0u) | command);
		return;
	}
	if (command >= 0x10u && command <= 0x1fu) {
		if (m->displayAddrMode == ArduboyAddrPage) {
			m->cursorColumn = (uint8_t)((m->cursorColumn & 0x0fu) | ((command & 0x0fu) << 4));
			m->cursorColumn = arduboyPrvDisplayClamp(m->cursorColumn, ARDUBOY_DISPLAY_WIDTH);
		}
		return;
	}
	if (command >= 0x40u && command <= 0x7fu)
		return;
	if (command >= 0xb0u && command < 0xb8u) {
		m->cursorPage = (uint8_t)(command - 0xb0u);
		return;
	}

	switch (command) {
		case 0x20:
		case 0x81:
		case 0x8d:
		case 0xa8:
		case 0xd3:
		case 0xd5:
		case 0xd9:
		case 0xda:
		case 0xdb:
			m->displayCommand = command;
			m->displayRegWrites = 1u;
			return;
		case 0x21:
		case 0x22:
			m->displayCommand = command;
			m->displayRegWrites = 2u;
			return;
		case 0x26:
		case 0x27:
			m->displayCommand = command;
			m->displayRegWrites = 6u;
			return;
		case 0x29:
		case 0x2a:
			m->displayCommand = command;
			m->displayRegWrites = 5u;
			return;
		case 0xa3:
			m->displayCommand = command;
			m->displayRegWrites = 2u;
			return;
		case 0xa6:
			if (m->displayFlags & ARDUBOY_PRESENT_STATE_INVERTED)
				m->displayDirty = 1;
			m->displayFlags &= (uint8_t)~ARDUBOY_PRESENT_STATE_INVERTED;
			return;
		case 0xa7:
			if (!(m->displayFlags & ARDUBOY_PRESENT_STATE_INVERTED))
				m->displayDirty = 1;
			m->displayFlags |= ARDUBOY_PRESENT_STATE_INVERTED;
			return;
		case 0xae:
			if (m->displayFlags & ARDUBOY_PRESENT_STATE_DISPLAY_ON)
				m->displayDirty = 1;
			m->displayFlags &= (uint8_t)~ARDUBOY_PRESENT_STATE_DISPLAY_ON;
			m->displayExplicitOff = 1;
			return;
		case 0xaf:
			if (!(m->displayFlags & ARDUBOY_PRESENT_STATE_DISPLAY_ON))
				m->displayDirty = 1;
			m->displayFlags |= ARDUBOY_PRESENT_STATE_DISPLAY_ON;
			m->displayExplicitOff = 0;
			return;
		case 0xa0:
			if (!(m->displayFlags & 0x04u))
				m->displayDirty = 1;
			m->displayFlags |= 0x04u;
			return;
		case 0xa1:
			if (m->displayFlags & 0x04u)
				m->displayDirty = 1;
			m->displayFlags &= (uint8_t)~0x04u;
			return;
		case 0xc0:
			if (!(m->displayFlags & 0x08u))
				m->displayDirty = 1;
			m->displayFlags |= 0x08u;
			return;
		case 0xc8:
			if (m->displayFlags & 0x08u)
				m->displayDirty = 1;
			m->displayFlags &= (uint8_t)~0x08u;
			return;
		default:
			return;
	}
}

static uint8_t arduboyPrvDisplayPinsForPortD(uint8_t portD)
{
	uint8_t pins = 0;

	if (portD & 0x40u)
		pins |= ARDUBOY_DISPLAY_PIN_CS;
	if (portD & 0x10u)
		pins |= ARDUBOY_DISPLAY_PIN_DC;
	if (portD & 0x80u)
		pins |= ARDUBOY_DISPLAY_PIN_RST;
	return pins;
}

static void arduboyPrvDisplayPinsFromPortD(ArduboyMachine *m, uint8_t portD)
{
	uint8_t oldReset = (uint8_t)(m->displayFlags & 0x80u);
	uint8_t newReset = (portD & 0x80u) ? 0x80u : 0u;
	uint8_t pins = arduboyPrvDisplayPinsForPortD(portD);

	m->lastPortD = portD;
	if (oldReset && !newReset)
		arduboyPrvDisplayReset(m);
	m->displayCs = (portD & 0x40u) ? 1u : 0u;
	m->displayDi = (portD & 0x10u) ? 1u : 0u;
	if (pins != m->lastDisplayPins) {
		m->lastDisplayPins = pins;
		m->displayPinTransitions++;
	}
	m->displayFlags = (uint8_t)((m->displayFlags & 0x7fu) | newReset);
}

static bool arduboyPrvDisplaySpiEx(ArduboyMachine *m, uint8_t value, bool hle)
{
	arduboyPrvDisplayPinsFromPortD(m, m->data[AVR_PORTD]);
	m->spiWrites++;
	if (hle)
		m->hleSpiWrites++;
	if (m->displayCs) {
		m->spiIgnoredWrites++;
		if (hle)
			m->hleSpiIgnoredWrites++;
		return false;
	}
	m->spiSelectedWrites++;
	if (m->displayDi)
		arduboyPrvDisplayWriteData(m, value);
	else
		arduboyPrvDisplayWriteCommand(m, value);
	return true;
}

static void arduboyPrvDisplaySpi(ArduboyMachine *m, uint8_t value)
{
	(void)arduboyPrvDisplaySpiEx(m, value, false);
}

static void arduboyPrvCompleteAdc(ArduboyMachine *m)
{
	uint8_t mux = (uint8_t)(m->data[AVR_ADMUX] ^ (m->data[AVR_ADCSRB] << 3));
	uint16_t value = (uint16_t)((m->cycles * 37u + mux * 211u + 0x123u) & 0x03ffu);

	m->data[AVR_ADCL] = (uint8_t)value;
	m->data[AVR_ADCH] = (uint8_t)(value >> 8);
	m->data[AVR_ADCSRA] = (uint8_t)((m->data[AVR_ADCSRA] & ~AVR_ADCSRA_ADSC) | AVR_ADCSRA_ADIF);
}

static uint8_t arduboyPrvReadData(ArduboyMachine *m, uint16_t addr)
{
	if (addr >= ARDUBOY_DATA_SIZE)
		return 0xffu;

	switch (addr) {
		case AVR_SPSR:
			return (uint8_t)(m->data[AVR_SPSR] | AVR_SPSR_SPIF);
		case AVR_SPDR:
			return m->data[addr];
		case AVR_PLLCSR:
			return (uint8_t)(m->data[AVR_PLLCSR] | AVR_PLLCSR_PLOCK);
		case AVR_ADCSRA:
			return (uint8_t)(m->data[AVR_ADCSRA] & ~AVR_ADCSRA_ADSC);
		case AVR_USBSTA:
			return (uint8_t)(m->data[AVR_USBSTA] | AVR_USBSTA_VBUS);
		case AVR_USBINT:
		case AVR_UDINT:
		case AVR_UEINT:
			return 0xffu;
		case AVR_UEINTX:
			return (uint8_t)(m->data[AVR_UEINTX] | AVR_UEINTX_TXINI | AVR_UEINTX_RWAL);
		case AVR_UESTA0X:
			return (uint8_t)(m->data[AVR_UESTA0X] | AVR_UESTA0X_CFGOK);
		default:
			break;
	}
	return m->data[addr];
}

static void arduboyPrvWriteEepromControl(ArduboyMachine *m, uint8_t value)
{
	uint16_t addr = (uint16_t)m->data[AVR_EEARL] | ((uint16_t)m->data[AVR_EEARH] << 8);

	m->data[AVR_EECR] = value;
	if ((value & 0x01u) && addr < ARDUBOY_SAVE_RAM_SIZE) {
		m->data[AVR_EEDR] = m->eeprom[addr];
		m->data[AVR_EECR] &= (uint8_t)~0x01u;
	}
	if ((value & 0x02u) && addr < ARDUBOY_SAVE_RAM_SIZE) {
		m->eeprom[addr] = m->data[AVR_EEDR];
		m->eepromDirty = 1;
		m->data[AVR_EECR] &= (uint8_t)~0x06u;
	}
}

static void arduboyPrvWriteData(ArduboyMachine *m, uint16_t addr, uint8_t value)
{
	if (addr >= ARDUBOY_DATA_SIZE)
		return;
	switch (addr) {
		case AVR_PINB:
			m->data[AVR_PORTB] ^= value;
			return;
		case AVR_PIND:
			m->data[AVR_PORTD] ^= value;
			arduboyPrvDisplayPinsFromPortD(m, m->data[AVR_PORTD]);
			return;
		case AVR_PINE:
			m->data[AVR_PORTE] ^= value;
			return;
		case AVR_PINF:
			m->data[AVR_PORTF] ^= value;
			return;
		case AVR_TIFR0:
			m->data[AVR_TIFR0] &= (uint8_t)~value;
			return;
		default:
			break;
	}
	m->data[addr] = value;
	switch (addr) {
		case AVR_PORTD:
			arduboyPrvDisplayPinsFromPortD(m, value);
			break;
		case AVR_PLLCSR:
			m->data[AVR_PLLCSR] = (uint8_t)(value | AVR_PLLCSR_PLOCK);
			break;
		case AVR_WDTCSR:
			m->data[AVR_WDTCSR] = 0;
			break;
		case AVR_ADCSRA:
			m->data[AVR_ADCSRA] = value;
			if (value & AVR_ADCSRA_ADSC)
				arduboyPrvCompleteAdc(m);
			break;
		case AVR_SPSR:
			m->data[AVR_SPSR] = (uint8_t)(value | AVR_SPSR_SPIF);
			break;
		case AVR_SPDR:
			arduboyPrvDisplaySpi(m, value);
			m->data[AVR_SPSR] |= AVR_SPSR_SPIF;
			break;
		case AVR_EECR:
			arduboyPrvWriteEepromControl(m, value);
			break;
		case AVR_UHWCON:
		case AVR_USBCON:
		case AVR_USBINT:
		case AVR_UDCON:
		case AVR_UDINT:
		case AVR_UDIEN:
		case AVR_UENUM:
		case AVR_UERST:
		case AVR_UECONX:
		case AVR_UECFG0X:
		case AVR_UECFG1X:
		case AVR_UEINT:
		case AVR_UEINTX:
			break;
		case AVR_UESTA0X:
			m->data[AVR_UESTA0X] = (uint8_t)(value | AVR_UESTA0X_CFGOK);
			break;
		default:
			break;
	}
}

ARDUBOY_INLINE bool arduboyPrvDataAddrNeedsIo(uint16_t addr)
{
	return addr >= ARDUBOY_IO_START && addr <= ARDUBOY_IO_END;
}

ARDUBOY_INLINE uint8_t arduboyPrvReadDataFast(ArduboyMachine *m, uint16_t addr, uint32_t *cyclesP)
{
	if (addr >= ARDUBOY_DATA_SIZE)
		return 0xffu;
	if (arduboyPrvDataAddrNeedsIo(addr)) {
		arduboyPrvFlushCycles(m, cyclesP);
		return arduboyPrvReadData(m, addr);
	}
	return m->data[addr];
}

ARDUBOY_INLINE void arduboyPrvWriteDataFast(ArduboyMachine *m, uint16_t addr, uint8_t value, uint32_t *cyclesP)
{
	if (addr >= ARDUBOY_DATA_SIZE)
		return;
	if (arduboyPrvDataAddrNeedsIo(addr)) {
		arduboyPrvFlushCycles(m, cyclesP);
		arduboyPrvWriteData(m, addr, value);
	}
	else
		m->data[addr] = value;
}

static void arduboyPrvFlagsLogic(ArduboyMachine *m, uint8_t result)
{
	uint8_t flags = 0;

	if (!result)
		flags |= AVR_SREG_Z;
	if (result & 0x80u)
		flags |= AVR_SREG_N | AVR_SREG_S;
	arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S, flags);
}

static uint8_t arduboyPrvAdd8(ArduboyMachine *m, uint8_t a, uint8_t b, uint8_t carry)
{
	uint16_t sum = (uint16_t)a + b + carry;
	uint8_t r = (uint8_t)sum;
	uint8_t flags = 0;
	bool v = ((~(a ^ b) & (a ^ r)) & 0x80u) != 0;
	bool n = (r & 0x80u) != 0;

	if (((a & 0x0fu) + (b & 0x0fu) + carry) > 0x0fu)
		flags |= AVR_SREG_H;
	if (!r)
		flags |= AVR_SREG_Z;
	if (n)
		flags |= AVR_SREG_N;
	if (v)
		flags |= AVR_SREG_V;
	if (n ^ v)
		flags |= AVR_SREG_S;
	if (sum > 0xffu)
		flags |= AVR_SREG_C;
	arduboyPrvSetSreg(m, AVR_SREG_H | AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S | AVR_SREG_C, flags);
	return r;
}

static uint8_t arduboyPrvSub8(ArduboyMachine *m, uint8_t a, uint8_t b, uint8_t carry, bool zAnd)
{
	uint16_t diff = (uint16_t)a - b - carry;
	uint8_t r = (uint8_t)diff;
	uint8_t oldZ = m->data[AVR_SREG] & AVR_SREG_Z;
	uint8_t flags = 0;
	bool h = ((~a & b) | (b & r) | (r & ~a)) & 0x08u;
	bool v = ((a & ~b & ~r) | (~a & b & r)) & 0x80u;
	bool n = (r & 0x80u) != 0;
	bool c = ((~a & b) | (b & r) | (r & ~a)) & 0x80u;

	if (h)
		flags |= AVR_SREG_H;
	if ((!zAnd && !r) || (zAnd && oldZ && !r))
		flags |= AVR_SREG_Z;
	if (n)
		flags |= AVR_SREG_N;
	if (v)
		flags |= AVR_SREG_V;
	if (n ^ v)
		flags |= AVR_SREG_S;
	if (c)
		flags |= AVR_SREG_C;
	arduboyPrvSetSreg(m, AVR_SREG_H | AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S | AVR_SREG_C, flags);
	return r;
}

static uint16_t ARDUBOY_FAST arduboyPrvTimer0Prescale(const ArduboyMachine *m)
{
	switch (m->data[AVR_TCCR0B] & 0x07u) {
		case 1: return 1u;
		case 2: return 8u;
		case 3: return 64u;
		case 4: return 256u;
		case 5: return 1024u;
		default: return 0u;
	}
}

static bool ARDUBOY_FAST arduboyPrvTimer0RangeHits(uint8_t oldValue, uint32_t ticks, uint8_t target)
{
	uint32_t diff;

	if (!ticks)
		return false;
	if (ticks >= 256u)
		return true;
	diff = (uint8_t)(target - oldValue);
	if (!diff)
		diff = 256u;
	return diff <= ticks;
}

static void ARDUBOY_FAST arduboyPrvTimer0Advance(ArduboyMachine *m, uint32_t cycles)
{
	uint16_t prescale = arduboyPrvTimer0Prescale(m);
	uint32_t ticks, total;
	uint8_t oldTcnt;

	m->cycles += cycles;
	if (!prescale || !cycles)
		return;
	m->timer0Cycles += cycles;
	ticks = m->timer0Cycles / prescale;
	if (!ticks)
		return;
	m->timer0Cycles -= ticks * prescale;
	oldTcnt = m->data[AVR_TCNT0];
	if (arduboyPrvTimer0RangeHits(oldTcnt, ticks, m->data[AVR_OCR0A]))
		m->data[AVR_TIFR0] |= AVR_TIFR0_OCF0A;
	if (arduboyPrvTimer0RangeHits(oldTcnt, ticks, m->data[AVR_OCR0B]))
		m->data[AVR_TIFR0] |= AVR_TIFR0_OCF0B;
	total = oldTcnt + ticks;
	m->data[AVR_TCNT0] = (uint8_t)total;
	if (total >= 256u) {
		m->data[AVR_TIFR0] |= AVR_TIFR0_TOV0;
		m->timer0Pending = 1;
	}
}

static void ARDUBOY_FAST arduboyPrvTick(ArduboyMachine *m, uint8_t cycles)
{
	arduboyPrvTimer0Advance(m, cycles);
}

static void ARDUBOY_FAST arduboyPrvFlushCycles(ArduboyMachine *m, uint32_t *cyclesP)
{
	if (*cyclesP) {
		arduboyPrvTimer0Advance(m, *cyclesP);
		*cyclesP = 0;
	}
}

static void ARDUBOY_FAST arduboyPrvServiceInterrupts(ArduboyMachine *m)
{
	if (!(m->data[AVR_SREG] & AVR_SREG_I))
		return;
	if ((m->timer0Pending || (m->data[AVR_TIFR0] & AVR_TIFR0_TOV0)) &&
		(m->data[AVR_TIMSK0] & AVR_TIMSK0_TOIE0)) {
		m->timer0Pending = 0;
		m->data[AVR_TIFR0] &= (uint8_t)~AVR_TIFR0_TOV0;
		arduboyPrvPush16(m, m->pc, ARDUBOY_STACK_OP_CALL);
		m->data[AVR_SREG] &= (uint8_t)~AVR_SREG_I;
		m->pc = ARDUBOY_VECTOR_TIMER0_OVF;
		arduboyPrvTick(m, 5);
	}
}

static uint8_t arduboyPrvIoBit(ArduboyMachine *m, uint8_t ioAddr, uint8_t bit)
{
	return (uint8_t)((arduboyPrvReadData(m, (uint16_t)(0x20u + ioAddr)) >> bit) & 1u);
}

static uint8_t arduboyPrvOpcodeWords(uint16_t opcode)
{
	if ((opcode & 0xfe0fu) == 0x9000u || (opcode & 0xfe0fu) == 0x9200u)
		return 2;
	if ((opcode & 0xfe0eu) == 0x940cu || (opcode & 0xfe0eu) == 0x940eu)
		return 2;
	return 1;
}

static uint8_t arduboyPrvSkipWords(ArduboyMachine *m, uint16_t pc)
{
	return arduboyPrvOpcodeWords(arduboyPrvFlashWord(m, pc));
}

static uint8_t arduboyPrvRegD(uint16_t opcode)
{
	return (uint8_t)((opcode >> 4) & 0x1fu);
}

static uint8_t arduboyPrvRegR(uint16_t opcode)
{
	return (uint8_t)((opcode & 0x0fu) | ((opcode >> 5) & 0x10u));
}

static uint8_t arduboyPrvImm8(uint16_t opcode)
{
	return (uint8_t)((opcode & 0x0fu) | ((opcode >> 4) & 0xf0u));
}

static uint8_t arduboyPrvLdq(uint16_t opcode)
{
	return (uint8_t)((opcode & 0x07u) | ((opcode >> 7) & 0x18u) | ((opcode >> 8) & 0x20u));
}

static void arduboyPrvUnsupported(ArduboyMachine *m, uint16_t pc, uint16_t opcode)
{
	m->unsupported = 1;
	m->failureReason = ARDUBOY_FAIL_UNSUPPORTED_OPCODE;
	m->unsupportedPc = pc;
	m->unsupportedOpcode = opcode;
}

static uint16_t ARDUBOY_FAST arduboyPrvArg16(const ArduboyMachine *m, uint8_t reg)
{
	return (uint16_t)m->data[reg] | ((uint16_t)m->data[reg + 1u] << 8);
}

static uint32_t ARDUBOY_FAST arduboyPrvArg32(const ArduboyMachine *m, uint8_t reg)
{
	return (uint32_t)m->data[reg] | ((uint32_t)m->data[reg + 1u] << 8) |
		((uint32_t)m->data[reg + 2u] << 16) | ((uint32_t)m->data[reg + 3u] << 24);
}

static void ARDUBOY_FAST arduboyPrvReturn8(ArduboyMachine *m, uint8_t value, uint8_t cycles)
{
	m->data[24] = value;
	m->pc = arduboyPrvPop16(m, ARDUBOY_STACK_OP_HLE_RET);
	m->lastHleExitSp = arduboyPrvGetSp(m);
	arduboyPrvTick(m, cycles);
}

static void ARDUBOY_FAST arduboyPrvReturn16(ArduboyMachine *m, uint16_t value, uint8_t cycles)
{
	m->data[24] = (uint8_t)value;
	m->data[25] = (uint8_t)(value >> 8);
	m->pc = arduboyPrvPop16(m, ARDUBOY_STACK_OP_HLE_RET);
	m->lastHleExitSp = arduboyPrvGetSp(m);
	arduboyPrvTick(m, cycles);
}

static void ARDUBOY_FAST arduboyPrvReturnVoid(ArduboyMachine *m, uint8_t cycles)
{
	m->pc = arduboyPrvPop16(m, ARDUBOY_STACK_OP_HLE_RET);
	m->lastHleExitSp = arduboyPrvGetSp(m);
	arduboyPrvTick(m, cycles);
}

static void ARDUBOY_FAST arduboyPrvReturn32(ArduboyMachine *m, uint32_t value, uint8_t cycles)
{
	m->data[22] = (uint8_t)value;
	m->data[23] = (uint8_t)(value >> 8);
	m->data[24] = (uint8_t)(value >> 16);
	m->data[25] = (uint8_t)(value >> 24);
	m->pc = arduboyPrvPop16(m, ARDUBOY_STACK_OP_HLE_RET);
	m->lastHleExitSp = arduboyPrvGetSp(m);
	arduboyPrvTick(m, cycles);
}

static void ARDUBOY_FAST arduboyPrvAdvanceMilliseconds(ArduboyMachine *m, uint32_t ms)
{
	while (ms) {
		uint32_t now = ms > 60000u ? 60000u : ms;

		arduboyPrvTimer0Advance(m, now * (ARDUBOY_CPU_FREQ / 1000u));
		ms -= now;
	}
}

static void ARDUBOY_FAST arduboyPrvWriteGuest8(ArduboyMachine *m, uint16_t addr, uint8_t value)
{
	if (addr != ARDUBOY_ADDR_UNSET && addr < ARDUBOY_DATA_SIZE)
		m->data[addr] = value;
}

static void ARDUBOY_FAST arduboyPrvWriteGuest32(ArduboyMachine *m, uint16_t addr, uint32_t value)
{
	if (addr == ARDUBOY_ADDR_UNSET || addr + 3u >= ARDUBOY_DATA_SIZE)
		return;
	m->data[addr] = (uint8_t)value;
	m->data[addr + 1u] = (uint8_t)(value >> 8);
	m->data[addr + 2u] = (uint8_t)(value >> 16);
	m->data[addr + 3u] = (uint8_t)(value >> 24);
}

static uint32_t ARDUBOY_FAST arduboyPrvGuestMillis(const ArduboyMachine *m)
{
	return (uint32_t)(m->cycles / (ARDUBOY_CPU_FREQ / 1000u));
}

static uint8_t ARDUBOY_FAST arduboyPrvButtonsState(ArduboyMachine *m)
{
	uint8_t buttons;

	arduboyPrvApplyButtons(m);
	buttons = (uint8_t)(~m->data[AVR_PINF] & 0xf0u);
	if (!(m->data[AVR_PINE] & 0x40u))
		buttons |= 0x08u;
	if (!(m->data[AVR_PINB] & 0x10u))
		buttons |= 0x04u;
	return buttons;
}

static bool arduboyPrvRangeInData(uint16_t addr, uint16_t len)
{
	return addr < ARDUBOY_DATA_SIZE && len <= ARDUBOY_DATA_SIZE - addr;
}

static bool arduboyPrvRangeInFlash(uint16_t addr, uint16_t len)
{
	return addr < ARDUBOY_FLASH_SIZE && len <= ARDUBOY_FLASH_SIZE - addr;
}

static bool arduboyPrvHasThisArg(const ArduboyMachine *m)
{
	uint16_t ptr = arduboyPrvArg16(m, 24);

	return ptr >= ARDUBOY_RAM_START && ptr < ARDUBOY_DATA_SIZE;
}

static uint8_t arduboyPrvArgReg(bool memberCall, uint8_t index)
{
	uint8_t reg = (uint8_t)((memberCall ? 22u : 24u) - (uint8_t)(index * 2u));

	return reg < 8u ? 8u : reg;
}

static uint8_t arduboyPrvHleArg8(const ArduboyMachine *m, uint8_t index)
{
	return m->data[arduboyPrvArgReg(arduboyPrvHasThisArg(m), index)];
}

static uint16_t arduboyPrvHleArg16(const ArduboyMachine *m, uint8_t index)
{
	return arduboyPrvArg16(m, arduboyPrvArgReg(arduboyPrvHasThisArg(m), index));
}

static int16_t arduboyPrvHleArgS16(const ArduboyMachine *m, uint8_t index)
{
	return (int16_t)arduboyPrvHleArg16(m, index);
}

static uint8_t *arduboyPrvScreenBuffer(ArduboyMachine *m)
{
	if (m->screenBufferAddr == ARDUBOY_ADDR_UNSET ||
		!arduboyPrvRangeInData(m->screenBufferAddr, sizeof(m->vram)))
		return NULL;
	return m->data + m->screenBufferAddr;
}

static uint8_t arduboyPrvReadProgramOrDataByte(const ArduboyMachine *m, uint16_t addr, bool progmem)
{
	if (progmem)
		return addr < ARDUBOY_FLASH_SIZE ? m->flash[addr] : 0xffu;
	return addr < ARDUBOY_DATA_SIZE ? m->data[addr] : 0xffu;
}

static void arduboyPrvWriteScreenPixel(uint8_t *buf, int16_t x, int16_t y, uint8_t color)
{
	uint16_t index;
	uint8_t mask;

	if ((uint16_t)x >= 128u || (uint16_t)y >= 64u)
		return;
	index = (uint16_t)(((uint16_t)y >> 3) * 128u + (uint16_t)x);
	mask = (uint8_t)(1u << ((uint8_t)y & 7u));
	if (color == ARDUBOY_COLOR_BLACK)
		buf[index] &= (uint8_t)~mask;
	else if (color == ARDUBOY_COLOR_INVERT)
		buf[index] ^= mask;
	else
		buf[index] |= mask;
}

static bool arduboyPrvHleClearBuffer(ArduboyMachine *m, uint8_t color)
{
	uint8_t *buf = arduboyPrvScreenBuffer(m);

	if (!buf)
		return false;
	memset(buf, color ? 0xff : 0x00, sizeof(m->vram));
	return true;
}

static bool arduboyPrvHleDrawPixel(ArduboyMachine *m, int16_t x, int16_t y, uint8_t color)
{
	uint8_t *buf = arduboyPrvScreenBuffer(m);

	if (!buf)
		return false;
	arduboyPrvWriteScreenPixel(buf, x, y, color <= ARDUBOY_COLOR_INVERT ? color : ARDUBOY_COLOR_WHITE);
	return true;
}

static bool arduboyPrvHleDrawFastHLine(ArduboyMachine *m, int16_t x, int16_t y, uint8_t w, uint8_t color)
{
	uint8_t *buf = arduboyPrvScreenBuffer(m);
	int16_t xEnd;

	if (!buf)
		return false;
	if ((uint16_t)y >= 64u || !w)
		return true;
	xEnd = (int16_t)(x + w);
	if (x < 0)
		x = 0;
	if (xEnd > 128)
		xEnd = 128;
	while (x < xEnd)
		arduboyPrvWriteScreenPixel(buf, x++, y, color);
	return true;
}

static bool arduboyPrvHleDrawFastVLine(ArduboyMachine *m, int16_t x, int16_t y, uint8_t h, uint8_t color)
{
	uint8_t *buf = arduboyPrvScreenBuffer(m);
	int16_t yEnd;

	if (!buf)
		return false;
	if ((uint16_t)x >= 128u || !h)
		return true;
	yEnd = (int16_t)(y + h);
	if (y < 0)
		y = 0;
	if (yEnd > 64)
		yEnd = 64;
	while (y < yEnd)
		arduboyPrvWriteScreenPixel(buf, x, y++, color);
	return true;
}

static bool arduboyPrvHleFillRect(ArduboyMachine *m, int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color)
{
	uint8_t *buf = arduboyPrvScreenBuffer(m);
	int16_t x0, y0, x1, y1;

	if (!buf)
		return false;
	if (!w || !h)
		return true;
	x0 = x < 0 ? 0 : x;
	y0 = y < 0 ? 0 : y;
	x1 = (int16_t)(x + w);
	y1 = (int16_t)(y + h);
	if (x1 > 128)
		x1 = 128;
	if (y1 > 64)
		y1 = 64;
	for (int16_t py = y0; py < y1; py++)
		for (int16_t px = x0; px < x1; px++)
			arduboyPrvWriteScreenPixel(buf, px, py, color);
	return true;
}

static bool arduboyPrvHleDrawLine(ArduboyMachine *m,
	int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
	uint8_t *buf = arduboyPrvScreenBuffer(m);
	int16_t dx, dy, sx, sy, err;

	if (!buf)
		return false;
	dx = x0 < x1 ? (int16_t)(x1 - x0) : (int16_t)(x0 - x1);
	dy = y0 < y1 ? (int16_t)(y0 - y1) : (int16_t)(y1 - y0);
	sx = x0 < x1 ? 1 : -1;
	sy = y0 < y1 ? 1 : -1;
	err = (int16_t)(dx + dy);
	for (;;) {
		int16_t e2;

		arduboyPrvWriteScreenPixel(buf, x0, y0, color);
		if (x0 == x1 && y0 == y1)
			break;
		e2 = (int16_t)(err * 2);
		if (e2 >= dy) {
			err = (int16_t)(err + dy);
			x0 = (int16_t)(x0 + sx);
		}
		if (e2 <= dx) {
			err = (int16_t)(err + dx);
			y0 = (int16_t)(y0 + sy);
		}
	}
	return true;
}

static bool arduboyPrvHleDrawRect(ArduboyMachine *m,
	int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color)
{
	if (!w || !h)
		return arduboyPrvScreenBuffer(m) != NULL;
	return arduboyPrvHleDrawFastHLine(m, x, y, w, color) &&
		arduboyPrvHleDrawFastHLine(m, x, (int16_t)(y + h - 1), w, color) &&
		arduboyPrvHleDrawFastVLine(m, x, y, h, color) &&
		arduboyPrvHleDrawFastVLine(m, (int16_t)(x + w - 1), y, h, color);
}

static bool arduboyPrvHleDrawBitmapImpl(ArduboyMachine *m,
	int16_t x, int16_t y, uint16_t src, uint8_t w, uint8_t h, uint8_t color, bool progmem)
{
	uint8_t *buf = arduboyPrvScreenBuffer(m);
	uint8_t pages;
	uint16_t size;

	if (!buf || !w || !h || w > 128u || h > 64u)
		return false;
	pages = (uint8_t)((h + 7u) >> 3);
	size = (uint16_t)w * pages;
	if (progmem ? !arduboyPrvRangeInFlash(src, size) : !arduboyPrvRangeInData(src, size))
		return false;
	for (uint8_t page = 0; page < pages; page++) {
		for (uint8_t bx = 0; bx < w; bx++) {
			uint8_t bits = arduboyPrvReadProgramOrDataByte(m, (uint16_t)(src + (uint16_t)page * w + bx), progmem);

			for (uint8_t bit = 0; bit < 8u; bit++) {
				uint8_t py = (uint8_t)(page * 8u + bit);

				if (py >= h)
					break;
				if (bits & (uint8_t)(1u << bit))
					arduboyPrvWriteScreenPixel(buf, (int16_t)(x + bx), (int16_t)(y + py), color);
			}
		}
	}
	return true;
}

static bool arduboyPrvHleDrawBitmap(ArduboyMachine *m)
{
	int16_t x = arduboyPrvHleArgS16(m, 0);
	int16_t y = arduboyPrvHleArgS16(m, 1);
	uint16_t src = arduboyPrvHleArg16(m, 2);
	uint8_t w = arduboyPrvHleArg8(m, 3);
	uint8_t h = arduboyPrvHleArg8(m, 4);
	uint8_t color = arduboyPrvHleArg8(m, 5);

	if (color > ARDUBOY_COLOR_INVERT)
		color = ARDUBOY_COLOR_WHITE;
	return arduboyPrvHleDrawBitmapImpl(m, x, y, src, w, h, color, true);
}

static bool arduboyPrvHleDrawSpriteMode(ArduboyMachine *m,
	int16_t x, int16_t y, uint16_t bitmap, uint16_t mask, uint8_t frame, uint8_t mode)
{
	uint8_t *buf = arduboyPrvScreenBuffer(m);
	uint8_t w, h, pages;
	uint16_t frameSize;
	uint16_t dataBase;

	if (!buf || !arduboyPrvRangeInFlash(bitmap, 2))
		return false;
	w = m->flash[bitmap];
	h = m->flash[bitmap + 1u];
	if (!w || !h || w > 128u || h > 64u)
		return false;
	pages = (uint8_t)((h + 7u) >> 3);
	frameSize = (uint16_t)w * pages;
	if (mode == 3u)
		frameSize = (uint16_t)(frameSize * 2u);
	dataBase = (uint16_t)(bitmap + 2u + (uint16_t)frame * frameSize);
	if (!arduboyPrvRangeInFlash(dataBase, frameSize))
		return false;
	if (mode == 4u)
		mode = 2u;
	if (mode > 3u)
		mode = 0u;
	for (uint8_t page = 0; page < pages; page++) {
		for (uint8_t bx = 0; bx < w; bx++) {
			uint16_t off = (uint16_t)page * w + bx;
			uint8_t img = m->flash[dataBase + off];
			uint8_t msk = 0xffu;

			if (mode == 3u)
				msk = m->flash[dataBase + off + frameSize / 2u];
			else if (mask != ARDUBOY_ADDR_UNSET && arduboyPrvRangeInFlash(mask, 2u + frameSize))
				msk = m->flash[(uint16_t)(mask + 2u + (uint16_t)frame * frameSize + off)];
			else if (mode == 1u || mode == 2u)
				msk = img;
			for (uint8_t bit = 0; bit < 8u; bit++) {
				uint8_t py = (uint8_t)(page * 8u + bit);
				int16_t dx = (int16_t)(x + bx);
				int16_t dy = (int16_t)(y + py);
				uint8_t bitMask = (uint8_t)(1u << bit);

				if (py >= h)
					break;
				if (mode == 0u) {
					arduboyPrvWriteScreenPixel(buf, dx, dy,
						(img & bitMask) ? ARDUBOY_COLOR_WHITE : ARDUBOY_COLOR_BLACK);
				}
				else if (mode == 2u) {
					if (img & bitMask)
						arduboyPrvWriteScreenPixel(buf, dx, dy, ARDUBOY_COLOR_BLACK);
				}
				else {
					if (msk & bitMask)
						arduboyPrvWriteScreenPixel(buf, dx, dy, ARDUBOY_COLOR_BLACK);
					if (img & bitMask)
						arduboyPrvWriteScreenPixel(buf, dx, dy, ARDUBOY_COLOR_WHITE);
				}
			}
		}
	}
	return true;
}

static bool arduboyPrvHleDrawSprites(ArduboyMachine *m)
{
	int16_t x = (int16_t)arduboyPrvArg16(m, 24);
	int16_t y = (int16_t)arduboyPrvArg16(m, 22);
	uint16_t bitmap = arduboyPrvArg16(m, 20);
	uint8_t frame = m->data[18];
	uint8_t mode = m->data[16] <= 4u ? m->data[16] : 0u;
	uint16_t mask = arduboyPrvArg16(m, 16);

	return arduboyPrvHleDrawSpriteMode(m, x, y, bitmap, mask, frame, mode);
}

static void arduboyPrvSyncHleButtons(ArduboyMachine *m, uint8_t buttons)
{
	m->hlePreviousButtons = m->hleCurrentButtons;
	m->hleCurrentButtons = buttons;
	arduboyPrvWriteGuest8(m, m->previousButtonStateAddr, m->hlePreviousButtons);
	arduboyPrvWriteGuest8(m, m->currentButtonStateAddr, m->hleCurrentButtons);
}

static bool arduboyPrvHlePaintScreen(ArduboyMachine *m, uint16_t src, bool progmem, bool clear)
{
	if (!progmem) {
		if (src >= ARDUBOY_DATA_SIZE || ARDUBOY_DATA_SIZE - src < sizeof(m->vram))
			return false;
		memcpy(m->vram, m->data + src, sizeof(m->vram));
		if (clear)
			memset(m->data + src, 0, sizeof(m->vram));
		if (m->screenBufferAddr == ARDUBOY_ADDR_UNSET)
			m->screenBufferAddr = src;
	}
	else if (src < ARDUBOY_FLASH_SIZE && ARDUBOY_FLASH_SIZE - src >= sizeof(m->vram))
		memcpy(m->vram, m->flash + src, sizeof(m->vram));
	else
		return false;
	m->displayDirty = 1;
	if (!m->displayExplicitOff)
		m->displayFlags |= ARDUBOY_PRESENT_STATE_DISPLAY_ON;
	m->spiWrites += sizeof(m->vram);
	m->displayDataWrites += sizeof(m->vram);
	m->pc = arduboyPrvPop16(m, ARDUBOY_STACK_OP_HLE_RET);
	arduboyPrvTick(m, 128);
	return true;
}

static bool arduboyPrvHleDisplay(ArduboyMachine *m, bool clear)
{
	if (m->screenBufferAddr == ARDUBOY_ADDR_UNSET)
		return false;
	return arduboyPrvHlePaintScreen(m, m->screenBufferAddr, false, clear);
}

static bool arduboyPrvExecuteHle(ArduboyMachine *m)
{
	for (uint8_t i = 0; i < m->hleCount; i++) {
		ArduboyHleTarget *target = &m->hleTargets[i];

		if (target->pc != m->pc || (target->flags & ARDUBOY_HLE_FLAG_DISABLED))
			continue;
#if !ARDUBOY_HLE_ALLOW_HEURISTIC
		if (target->confidence < ARDUBOY_HLE_CONF_ANCHORED) {
			arduboyPrvSkipHeuristicHleTarget(m, target);
#ifdef ARDUBOY_PERF
			m->perfHleMisses++;
#endif
			continue;
		}
#endif
		if (!arduboyPrvValidateHleEntry(m, target)) {
#ifdef ARDUBOY_PERF
			m->perfHleMisses++;
#endif
			continue;
		}
#ifdef ARDUBOY_PERF
		if (target->kind < ArduboyHleKindCount)
			m->perfHleCalls[target->kind]++;
#endif
		switch (target->kind) {
			case ArduboyHleMutedAudio:
			case ArduboyHleRgbNoop:
			case ArduboyHleIdle:
				arduboyPrvReturnVoid(m, 8);
				return true;
			case ArduboyHleMillis:
				arduboyPrvReturn32(m, arduboyPrvGuestMillis(m), 8);
				return true;
			case ArduboyHleDelay:
				arduboyPrvAdvanceMilliseconds(m, arduboyPrvArg32(m, 22));
				arduboyPrvReturnVoid(m, 8);
				return true;
			case ArduboyHleDelayShort:
				arduboyPrvAdvanceMilliseconds(m, arduboyPrvArg16(m, 24));
				arduboyPrvReturnVoid(m, 8);
				return true;
			case ArduboyHleSetFrameRate: {
				uint8_t rate = arduboyPrvHleArg8(m, 0);

				m->frameDurationMs = rate ? (uint8_t)((1000u + rate / 2u) / rate) : 16u;
				if (!m->frameDurationMs)
					m->frameDurationMs = 1u;
				arduboyPrvWriteGuest8(m, m->eachFrameMillisAddr, m->frameDurationMs);
				arduboyPrvReturnVoid(m, 8);
				return true;
			}
			case ArduboyHleSetFrameDuration:
				m->frameDurationMs = arduboyPrvHleArg8(m, 0) ? arduboyPrvHleArg8(m, 0) : 16u;
				arduboyPrvWriteGuest8(m, m->eachFrameMillisAddr, m->frameDurationMs);
				arduboyPrvReturnVoid(m, 8);
				return true;
			case ArduboyHleNextFrame: {
				uint32_t nowMs = arduboyPrvGuestMillis(m);
				uint8_t frameMs = m->frameDurationMs ? m->frameDurationMs : 16u;
				uint32_t remainder = nowMs % frameMs;

				if (remainder)
					arduboyPrvAdvanceMilliseconds(m, frameMs - remainder);
				nowMs = arduboyPrvGuestMillis(m);
				arduboyPrvWriteGuest32(m, m->thisFrameStartAddr, nowMs);
				arduboyPrvWriteGuest8(m, m->lastFrameDurationMsAddr, frameMs);
				arduboyPrvWriteGuest8(m, m->justRenderedAddr, 1);
				m->hleFrameCount++;
				arduboyPrvWriteGuest8(m, m->frameCountAddr,
					(uint8_t)(m->frameCountAddr != ARDUBOY_ADDR_UNSET ? m->data[m->frameCountAddr] + 1u : m->hleFrameCount));
				arduboyPrvReturn8(m, 1, 8);
				return true;
			}
			case ArduboyHleEveryXFrames: {
				uint8_t frames = arduboyPrvHleArg8(m, 0);
				uint8_t count = m->frameCountAddr != ARDUBOY_ADDR_UNSET ? m->data[m->frameCountAddr] : (uint8_t)m->hleFrameCount;

				arduboyPrvReturn8(m, frames ? (uint8_t)((count % frames) == 0u) : 0u, 8);
				return true;
			}
			case ArduboyHleButtonsState: {
				uint8_t buttons = arduboyPrvButtonsState(m);

				arduboyPrvSyncHleButtons(m, buttons);
				arduboyPrvReturn8(m, buttons, 8);
				return true;
			}
			case ArduboyHlePollButtons:
				arduboyPrvSyncHleButtons(m, arduboyPrvButtonsState(m));
				arduboyPrvReturnVoid(m, 8);
				return true;
			case ArduboyHlePressed: {
				uint8_t mask = arduboyPrvHleArg8(m, 0);
				uint8_t buttons = arduboyPrvButtonsState(m);

				m->hleCurrentButtons = buttons;
				arduboyPrvWriteGuest8(m, m->currentButtonStateAddr, buttons);
				arduboyPrvReturn8(m, (uint8_t)((buttons & mask) == mask), 8);
				return true;
			}
			case ArduboyHleNotPressed: {
				uint8_t mask = arduboyPrvHleArg8(m, 0);
				uint8_t buttons = arduboyPrvButtonsState(m);

				m->hleCurrentButtons = buttons;
				arduboyPrvWriteGuest8(m, m->currentButtonStateAddr, buttons);
				arduboyPrvReturn8(m, (uint8_t)((buttons & mask) == 0u), 8);
				return true;
			}
			case ArduboyHleJustPressed: {
				uint8_t mask = arduboyPrvHleArg8(m, 0);
				uint8_t buttons = m->currentButtonStateAddr != ARDUBOY_ADDR_UNSET ? m->data[m->currentButtonStateAddr] : m->hleCurrentButtons;
				uint8_t previous = m->previousButtonStateAddr != ARDUBOY_ADDR_UNSET ? m->data[m->previousButtonStateAddr] : m->hlePreviousButtons;

				arduboyPrvReturn8(m, (uint8_t)(((buttons & mask) != 0u) && ((previous & mask) == 0u)), 8);
				return true;
			}
			case ArduboyHleJustReleased: {
				uint8_t mask = arduboyPrvHleArg8(m, 0);
				uint8_t buttons = m->currentButtonStateAddr != ARDUBOY_ADDR_UNSET ? m->data[m->currentButtonStateAddr] : m->hleCurrentButtons;
				uint8_t previous = m->previousButtonStateAddr != ARDUBOY_ADDR_UNSET ? m->data[m->previousButtonStateAddr] : m->hlePreviousButtons;

				arduboyPrvReturn8(m, (uint8_t)(((buttons & mask) == 0u) && ((previous & mask) != 0u)), 8);
				return true;
			}
			case ArduboyHleDisplay:
				if (arduboyPrvHleDisplay(m, false))
					return true;
				break;
			case ArduboyHleDisplayClear:
				if (arduboyPrvHleDisplay(m, true))
					return true;
				break;
			case ArduboyHlePaintScreen:
				if (arduboyPrvHlePaintScreen(m, arduboyPrvArg16(m, 24), false, m->data[22] != 0))
					return true;
				break;
			case ArduboyHlePaintScreenProgmem:
				if (arduboyPrvHlePaintScreen(m, arduboyPrvArg16(m, 24), true, false))
					return true;
				break;
			case ArduboyHleClear:
				if (arduboyPrvHleClearBuffer(m, ARDUBOY_COLOR_BLACK)) {
					arduboyPrvReturnVoid(m, 8);
					return true;
				}
				break;
			case ArduboyHleFillScreen:
				if (arduboyPrvHleClearBuffer(m, arduboyPrvHleArg8(m, 0))) {
					arduboyPrvReturnVoid(m, 8);
					return true;
				}
				break;
			case ArduboyHleDrawPixel:
				if (arduboyPrvHleDrawPixel(m, arduboyPrvHleArgS16(m, 0), arduboyPrvHleArgS16(m, 1), arduboyPrvHleArg8(m, 2))) {
					arduboyPrvReturnVoid(m, 8);
					return true;
				}
				break;
			case ArduboyHleDrawFastHLine:
				if (arduboyPrvHleDrawFastHLine(m, arduboyPrvHleArgS16(m, 0), arduboyPrvHleArgS16(m, 1),
					arduboyPrvHleArg8(m, 2), arduboyPrvHleArg8(m, 3))) {
					arduboyPrvReturnVoid(m, 8);
					return true;
				}
				break;
			case ArduboyHleDrawFastVLine:
				if (arduboyPrvHleDrawFastVLine(m, arduboyPrvHleArgS16(m, 0), arduboyPrvHleArgS16(m, 1),
					arduboyPrvHleArg8(m, 2), arduboyPrvHleArg8(m, 3))) {
					arduboyPrvReturnVoid(m, 8);
					return true;
				}
				break;
			case ArduboyHleDrawLine:
				if (arduboyPrvHleDrawLine(m, arduboyPrvHleArgS16(m, 0), arduboyPrvHleArgS16(m, 1),
					arduboyPrvHleArgS16(m, 2), arduboyPrvHleArgS16(m, 3), arduboyPrvHleArg8(m, 4))) {
					arduboyPrvReturnVoid(m, 16);
					return true;
				}
				break;
			case ArduboyHleDrawRect:
				if (arduboyPrvHleDrawRect(m, arduboyPrvHleArgS16(m, 0), arduboyPrvHleArgS16(m, 1),
					arduboyPrvHleArg8(m, 2), arduboyPrvHleArg8(m, 3), arduboyPrvHleArg8(m, 4))) {
					arduboyPrvReturnVoid(m, 12);
					return true;
				}
				break;
			case ArduboyHleFillRect:
				if (arduboyPrvHleFillRect(m, arduboyPrvHleArgS16(m, 0), arduboyPrvHleArgS16(m, 1),
					arduboyPrvHleArg8(m, 2), arduboyPrvHleArg8(m, 3), arduboyPrvHleArg8(m, 4))) {
					arduboyPrvReturnVoid(m, 16);
					return true;
				}
				break;
			case ArduboyHleDrawBitmap:
				if (arduboyPrvHleDrawBitmap(m) || arduboyPrvHleDrawSprites(m)) {
					arduboyPrvReturnVoid(m, 24);
					return true;
				}
				break;
			case ArduboyHleSpritesDraw:
				if (arduboyPrvHleDrawSprites(m)) {
					arduboyPrvReturnVoid(m, 24);
					return true;
				}
				break;
			case ArduboyHleAudioEnabled:
			case ArduboyHleTonePlaying:
				arduboyPrvWriteGuest8(m, m->audioEnabledAddr, 0);
				m->hleAudioEnabled = 0;
				arduboyPrvReturn8(m, 0, 8);
				return true;
			case ArduboyHleAudioOnOff:
			case ArduboyHleAudioSave:
			case ArduboyHleToneNoop:
			case ArduboyHleTonesNoop:
				arduboyPrvWriteGuest8(m, m->audioEnabledAddr, 0);
				m->hleAudioEnabled = 0;
				arduboyPrvReturnVoid(m, 8);
				return true;
			case ArduboyHleMemset: {
				uint16_t dest = arduboyPrvArg16(m, 24);
				uint8_t value = m->data[22];
				uint16_t len = arduboyPrvArg16(m, 20);

				if (arduboyPrvRangeInData(dest, len)) {
					memset(m->data + dest, value, len);
					arduboyPrvReturn16(m, dest, 12);
					return true;
				}
				break;
			}
			case ArduboyHleMemcpy:
			case ArduboyHleMemmove: {
				uint16_t dest = arduboyPrvArg16(m, 24);
				uint16_t src = arduboyPrvArg16(m, 22);
				uint16_t len = arduboyPrvArg16(m, 20);

				if (arduboyPrvRangeInData(dest, len) && arduboyPrvRangeInData(src, len)) {
					if (target->kind == ArduboyHleMemmove)
						memmove(m->data + dest, m->data + src, len);
					else
						memcpy(m->data + dest, m->data + src, len);
					arduboyPrvReturn16(m, dest, 12);
					return true;
				}
				break;
			}
			case ArduboyHleSpiTransfer:
			case ArduboyHleSpiTransferRead: {
				bool captured;

				arduboyPrvDisplayPinsFromPortD(m, m->data[AVR_PORTD]);
				if (m->displayCs && !m->displayCommandWrites && !m->displayDataWrites &&
					m->hleSpiIgnoredWrites >= ARDUBOY_SPI_HLE_HIGH_CS_DISABLE) {
					target->flags |= ARDUBOY_HLE_FLAG_DISABLED;
					m->lastHleFlags = target->flags;
					m->hleQuarantines++;
#ifdef ARDUBOY_PERF
					m->perfHleQuarantines++;
#endif
					return false;
				}
				captured = arduboyPrvDisplaySpiEx(m, m->data[24], true);
				m->data[AVR_SPDR] = m->data[24];
				m->data[AVR_SPSR] |= AVR_SPSR_SPIF;
				if (!captured && !m->displayCommandWrites && !m->displayDataWrites &&
					m->hleSpiIgnoredWrites >= ARDUBOY_SPI_HLE_HIGH_CS_DISABLE) {
					target->flags |= ARDUBOY_HLE_FLAG_DISABLED;
					m->lastHleFlags = target->flags;
					m->hleQuarantines++;
#ifdef ARDUBOY_PERF
					m->perfHleQuarantines++;
#endif
				}
				arduboyPrvReturn8(m, m->data[AVR_SPDR], 8);
				return true;
			}
			case ArduboyHleEepromRead: {
				uint16_t addr = arduboyPrvArg16(m, 24);
				arduboyPrvReturn8(m, addr < ARDUBOY_SAVE_RAM_SIZE ? m->eeprom[addr] : 0xffu, 8);
				return true;
			}
			case ArduboyHleEepromWrite:
			case ArduboyHleEepromUpdate: {
				uint16_t addr = arduboyPrvArg16(m, 24);
				uint8_t value = m->data[22];

				if (addr < ARDUBOY_SAVE_RAM_SIZE &&
					(target->kind == ArduboyHleEepromWrite || m->eeprom[addr] != value)) {
					m->eeprom[addr] = value;
					m->eepromDirty = 1;
				}
				arduboyPrvReturnVoid(m, 8);
				return true;
			}
			default:
				break;
		}
		/* A target may be a false-positive before all guest state has been discovered.
		 * Fall back to translated AVR execution rather than failing the ROM.
		 */
	}
	return false;
}


static uint8_t arduboyPrvHlePriority(uint8_t kind)
{
	switch (kind) {
		case ArduboyHleSpiTransfer:
		case ArduboyHleSpiTransferRead:
		case ArduboyHlePaintScreen:
		case ArduboyHlePaintScreenProgmem:
		case ArduboyHleDisplay:
		case ArduboyHleDisplayClear:
		case ArduboyHleDelay:
		case ArduboyHleDelayShort:
		case ArduboyHleMillis:
		case ArduboyHleNextFrame:
			return 4;
		case ArduboyHleDrawBitmap:
		case ArduboyHleSpritesDraw:
		case ArduboyHleFillRect:
		case ArduboyHleDrawLine:
		case ArduboyHleDrawPixel:
			return 3;
		case ArduboyHleButtonsState:
		case ArduboyHlePollButtons:
		case ArduboyHlePressed:
		case ArduboyHleNotPressed:
		case ArduboyHleJustPressed:
		case ArduboyHleJustReleased:
		case ArduboyHleMemset:
		case ArduboyHleMemcpy:
		case ArduboyHleMemmove:
			return 2;
		case ArduboyHleMutedAudio:
		case ArduboyHleAudioEnabled:
		case ArduboyHleAudioOnOff:
		case ArduboyHleAudioSave:
		case ArduboyHleToneNoop:
		case ArduboyHleTonesNoop:
		case ArduboyHleTonePlaying:
		case ArduboyHleRgbNoop:
			return 1;
		default:
			return 0;
	}
}

static const char *arduboyPrvHleName(uint8_t kind)
{
	switch (kind) {
		case ArduboyHleMutedAudio: return "MUTED AUDIO";
		case ArduboyHleMillis: return "MILLIS";
		case ArduboyHleDelay: return "DELAY";
		case ArduboyHleDelayShort: return "DELAY SHORT";
		case ArduboyHleIdle: return "IDLE";
		case ArduboyHleNextFrame: return "NEXT FRAME";
		case ArduboyHleSetFrameRate: return "SET FPS";
		case ArduboyHleSetFrameDuration: return "SET FRAME MS";
		case ArduboyHleButtonsState: return "BUTTONS";
		case ArduboyHleEepromRead: return "EEPROM READ";
		case ArduboyHleEepromWrite: return "EEPROM WRITE";
		case ArduboyHleEepromUpdate: return "EEPROM UPDATE";
		case ArduboyHleSpiTransfer: return "SPI TX";
		case ArduboyHleSpiTransferRead: return "SPI RX";
		case ArduboyHlePaintScreen: return "PAINT";
		case ArduboyHlePaintScreenProgmem: return "PAINT PM";
		case ArduboyHleDisplay: return "DISPLAY";
		case ArduboyHleDisplayClear: return "DISPLAY CLR";
		case ArduboyHleEveryXFrames: return "EVERY X";
		case ArduboyHlePollButtons: return "POLL BTN";
		case ArduboyHlePressed: return "PRESSED";
		case ArduboyHleNotPressed: return "NOT PRESSED";
		case ArduboyHleJustPressed: return "JUST PRESS";
		case ArduboyHleJustReleased: return "JUST REL";
		case ArduboyHleClear: return "CLEAR";
		case ArduboyHleFillScreen: return "FILL SCREEN";
		case ArduboyHleDrawPixel: return "PIXEL";
		case ArduboyHleDrawFastHLine: return "HLINE";
		case ArduboyHleDrawFastVLine: return "VLINE";
		case ArduboyHleDrawLine: return "LINE";
		case ArduboyHleDrawRect: return "RECT";
		case ArduboyHleFillRect: return "FILL RECT";
		case ArduboyHleDrawBitmap: return "BITMAP";
		case ArduboyHleSpritesDraw: return "SPRITES";
		case ArduboyHleAudioEnabled: return "AUDIO EN";
		case ArduboyHleAudioOnOff: return "AUDIO ONOFF";
		case ArduboyHleAudioSave: return "AUDIO SAVE";
		case ArduboyHleToneNoop: return "TONE";
		case ArduboyHleTonesNoop: return "TONES";
		case ArduboyHleTonePlaying: return "TONE PLAY";
		case ArduboyHleMemset: return "MEMSET";
		case ArduboyHleMemcpy: return "MEMCPY";
		case ArduboyHleMemmove: return "MEMMOVE";
		case ArduboyHleRgbNoop: return "RGB";
		default: return "NONE";
	}
}

static bool arduboyPrvAddHleTargetEx(ArduboyMachine *m, uint16_t pc, uint8_t kind, uint8_t confidence)
{
	if (m->hleCount >= ARDUBOY_HLE_TARGETS)
		return false;
	for (uint8_t i = 0; i < m->hleCount; i++) {
		if (m->hleTargets[i].pc == pc) {
			if (arduboyPrvHlePriority(kind) > arduboyPrvHlePriority(m->hleTargets[i].kind) ||
				confidence > m->hleTargets[i].confidence) {
				m->hleTargets[i].kind = kind;
				m->hleTargets[i].confidence = confidence;
			}
			return true;
		}
	}
	m->hleTargets[m->hleCount].pc = pc;
	m->hleTargets[m->hleCount].kind = kind;
	m->hleTargets[m->hleCount].confidence = confidence;
	m->hleTargets[m->hleCount].flags = 0;
	m->hleCount++;
	return true;
}

static bool arduboyPrvAddHleTarget(ArduboyMachine *m, uint16_t pc, uint8_t kind)
{
#if ARDUBOY_HLE_ALLOW_HEURISTIC
	return arduboyPrvAddHleTargetEx(m, pc, kind, ARDUBOY_HLE_CONF_HEURISTIC);
#else
	(void)m;
	(void)pc;
	(void)kind;
	return false;
#endif
}

static bool arduboyPrvIsInFrom(uint16_t opcode, uint8_t absAddr, uint8_t *regP)
{
	uint8_t ioAddr;

	if ((opcode & 0xf800u) != 0xb000u)
		return false;
	ioAddr = (uint8_t)((opcode & 0x0fu) | ((opcode >> 5) & 0x30u));
	if ((uint8_t)(0x20u + ioAddr) != absAddr)
		return false;
	if (regP)
		*regP = arduboyPrvRegD(opcode);
	return true;
}

static bool arduboyPrvIsOutTo(uint16_t opcode, uint8_t absAddr, uint8_t *regP)
{
	uint8_t ioAddr;

	if ((opcode & 0xf800u) != 0xb800u)
		return false;
	ioAddr = (uint8_t)((opcode & 0x0fu) | ((opcode >> 5) & 0x30u));
	if ((uint8_t)(0x20u + ioAddr) != absAddr)
		return false;
	if (regP)
		*regP = arduboyPrvRegD(opcode);
	return true;
}

static bool arduboyPrvLooksLikeSpiTransfer(const ArduboyMachine *m, uint16_t pc)
{
	uint8_t regOut, regIn, regSpdr;

	if (!arduboyPrvIsOutTo(arduboyPrvFlashWord(m, pc), AVR_SPDR, &regOut) || regOut != 24u)
		return false;
	if (!arduboyPrvIsInFrom(arduboyPrvFlashWord(m, (uint16_t)(pc + 1u)), AVR_SPSR, &regIn))
		return false;
	if ((arduboyPrvFlashWord(m, (uint16_t)(pc + 2u)) & 0xfe08u) != 0xfe00u ||
		arduboyPrvRegD(arduboyPrvFlashWord(m, (uint16_t)(pc + 2u))) != regIn ||
		(arduboyPrvFlashWord(m, (uint16_t)(pc + 2u)) & 0x07u) != 7u)
		return false;
	if (!arduboyPrvRjmpTargets((uint16_t)(pc + 3u), arduboyPrvFlashWord(m, (uint16_t)(pc + 3u)), (uint16_t)(pc + 1u)))
		return false;
	if (!arduboyPrvIsInFrom(arduboyPrvFlashWord(m, (uint16_t)(pc + 4u)), AVR_SPDR, &regSpdr) || regSpdr != 24u)
		return false;
	return arduboyPrvFlashWord(m, (uint16_t)(pc + 5u)) == 0x9508u;
}

#if 0
static bool arduboyPrvRemovedRawOpcodeInterpreter(ArduboyMachine *m, uint16_t opcode, uint16_t next)
{
	uint16_t pc = m->pc;
	uint8_t d = arduboyPrvRegD(opcode);
	uint8_t r = arduboyPrvRegR(opcode);
	uint8_t opHi = (uint8_t)(opcode >> 10);

	m->lastPc = pc;
	m->lastOpcode = opcode;

	if (opcode == 0x0000u) {
		m->pc = next;
		arduboyPrvTick(m, 1);
		return true;
	}

	if ((opcode & 0xff00u) == 0x0100u) {
		d = (uint8_t)(((opcode >> 4) & 0x0fu) << 1);
		r = (uint8_t)((opcode & 0x0fu) << 1);
		m->data[d] = m->data[r];
		m->data[d + 1u] = m->data[r + 1u];
		m->pc = next;
		arduboyPrvTick(m, 1);
		return true;
	}

	if ((opcode & 0xfc00u) == 0x9c00u) {
		uint16_t result = (uint16_t)m->data[d] * m->data[r];
		uint8_t flags = 0;

		m->data[0] = (uint8_t)result;
		m->data[1] = (uint8_t)(result >> 8);
		if (!result)
			flags |= AVR_SREG_Z;
		if (result & 0x8000u)
			flags |= AVR_SREG_C;
		arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_C, flags);
		m->pc = next;
		arduboyPrvTick(m, 2);
		return true;
	}

	if ((opcode & 0xff00u) == 0x0200u) {
		int16_t result = (int16_t)((int8_t)m->data[16u + ((opcode >> 4) & 0x0fu)] *
			(int8_t)m->data[16u + (opcode & 0x0fu)]);
		uint8_t flags = 0;

		m->data[0] = (uint8_t)result;
		m->data[1] = (uint8_t)((uint16_t)result >> 8);
		if (!result)
			flags |= AVR_SREG_Z;
		if ((uint16_t)result & 0x8000u)
			flags |= AVR_SREG_C;
		arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_C, flags);
		m->pc = next;
		arduboyPrvTick(m, 2);
		return true;
	}

	if ((opcode & 0xff00u) == 0x0300u) {
		uint8_t rd = (uint8_t)(16u + ((opcode >> 4) & 0x07u));
		uint8_t rr = (uint8_t)(16u + (opcode & 0x07u));
		uint8_t kind = (uint8_t)((opcode >> 3) & 0x03u);
		int32_t product;
		uint16_t result;
		uint8_t flags = 0;

		switch (kind) {
			case 0:
				product = (int32_t)((int8_t)m->data[rd]) * m->data[rr];
				result = (uint16_t)product;
				break;
			case 1:
				product = (uint16_t)m->data[rd] * (uint16_t)m->data[rr];
				result = (uint16_t)(product * 2);
				break;
			case 2:
				product = (int32_t)((int8_t)m->data[rd]) * (int8_t)m->data[rr];
				result = (uint16_t)(product * 2);
				break;
			default:
				product = (int32_t)((int8_t)m->data[rd]) * m->data[rr];
				result = (uint16_t)(product * 2);
				break;
		}
		m->data[0] = (uint8_t)result;
		m->data[1] = (uint8_t)(result >> 8);
		if (!result)
			flags |= AVR_SREG_Z;
		if (result & 0x8000u)
			flags |= AVR_SREG_C;
		arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_C, flags);
		m->pc = next;
		arduboyPrvTick(m, 2);
		return true;
	}

	if (opHi <= 0x0bu) {
		uint8_t a = m->data[d], b = m->data[r], result = 0;

		switch (opcode & 0xfc00u) {
			case 0x0400u:
				(void)arduboyPrvSub8(m, a, b, m->data[AVR_SREG] & AVR_SREG_C, true);
				break;
			case 0x0800u:
				result = arduboyPrvSub8(m, a, b, m->data[AVR_SREG] & AVR_SREG_C, true);
				m->data[d] = result;
				break;
			case 0x0c00u:
				m->data[d] = arduboyPrvAdd8(m, a, b, 0);
				break;
			case 0x1000u:
				if (a == b)
					next = (uint16_t)(next + arduboyPrvSkipWords(m, next));
				break;
			case 0x1400u:
				(void)arduboyPrvSub8(m, a, b, 0, false);
				break;
			case 0x1800u:
				m->data[d] = arduboyPrvSub8(m, a, b, 0, false);
				break;
			case 0x1c00u:
				m->data[d] = arduboyPrvAdd8(m, a, b, m->data[AVR_SREG] & AVR_SREG_C);
				break;
			case 0x2000u:
				m->data[d] = (uint8_t)(a & b);
				arduboyPrvFlagsLogic(m, m->data[d]);
				break;
			case 0x2400u:
				m->data[d] = (uint8_t)(a ^ b);
				arduboyPrvFlagsLogic(m, m->data[d]);
				break;
			case 0x2800u:
				m->data[d] = (uint8_t)(a | b);
				arduboyPrvFlagsLogic(m, m->data[d]);
				break;
			case 0x2c00u:
				m->data[d] = b;
				break;
			default:
				arduboyPrvUnsupported(m, pc, opcode);
				return false;
		}
		m->pc = next;
		arduboyPrvTick(m, 1);
		return true;
	}

	if ((opcode & 0xf000u) >= 0x3000u && (opcode & 0xf000u) <= 0x7000u) {
		d = (uint8_t)(16u + ((opcode >> 4) & 0x0fu));
		uint8_t k = arduboyPrvImm8(opcode);

		switch (opcode & 0xf000u) {
			case 0x3000u:
				(void)arduboyPrvSub8(m, m->data[d], k, 0, false);
				break;
			case 0x4000u:
				m->data[d] = arduboyPrvSub8(m, m->data[d], k, m->data[AVR_SREG] & AVR_SREG_C, true);
				break;
			case 0x5000u:
				m->data[d] = arduboyPrvSub8(m, m->data[d], k, 0, false);
				break;
			case 0x6000u:
				m->data[d] = (uint8_t)(m->data[d] | k);
				arduboyPrvFlagsLogic(m, m->data[d]);
				break;
			case 0x7000u:
				m->data[d] = (uint8_t)(m->data[d] & k);
				arduboyPrvFlagsLogic(m, m->data[d]);
				break;
			default:
				break;
		}
		m->pc = next;
		arduboyPrvTick(m, 1);
		return true;
	}

	if ((opcode & 0xd208u) == 0x8000u || (opcode & 0xd208u) == 0x8008u ||
		(opcode & 0xd208u) == 0x8200u || (opcode & 0xd208u) == 0x8208u) {
		uint8_t q = arduboyPrvLdq(opcode);
		uint8_t baseReg = (opcode & 0x0008u) ? 28u : 30u;
		uint16_t addr = (uint16_t)(arduboyPrvGetPair(m, baseReg) + q);

		d = arduboyPrvRegD(opcode);
		if (opcode & 0x0200u)
			arduboyPrvWriteData(m, addr, m->data[d]);
		else
			m->data[d] = arduboyPrvReadData(m, addr);
		m->pc = next;
		arduboyPrvTick(m, 2);
		return true;
	}

	if ((opcode & 0xf000u) == 0xe000u) {
		d = (uint8_t)(16u + ((opcode >> 4) & 0x0fu));
		m->data[d] = arduboyPrvImm8(opcode);
		m->pc = next;
		arduboyPrvTick(m, 1);
		return true;
	}

	if ((opcode & 0xf000u) == 0xb000u || (opcode & 0xf000u) == 0xb800u) {
		uint8_t ioAddr = (uint8_t)((opcode & 0x0fu) | ((opcode >> 5) & 0x30u));

		d = arduboyPrvRegD(opcode);
		if (opcode & 0x0800u)
			arduboyPrvWriteData(m, (uint16_t)(0x20u + ioAddr), m->data[d]);
		else
			m->data[d] = arduboyPrvReadData(m, (uint16_t)(0x20u + ioAddr));
		m->pc = next;
		arduboyPrvTick(m, 1);
		return true;
	}

	if ((opcode & 0xf000u) == 0xc000u) {
		m->pc = (uint16_t)(next + arduboyPrvSign(opcode, 12));
		arduboyPrvTick(m, 2);
		return true;
	}

	if ((opcode & 0xf000u) == 0xd000u) {
		arduboyPrvPush16(m, next, ARDUBOY_STACK_OP_CALL);
		m->pc = (uint16_t)(next + arduboyPrvSign(opcode, 12));
		arduboyPrvTick(m, 3);
		return true;
	}

	if ((opcode & 0xfe0fu) == 0x9000u) {
		uint16_t addr = arduboyPrvFlashWord(m, (uint16_t)(pc + 1u));

		m->data[d] = arduboyPrvReadData(m, addr);
		m->pc = next;
		arduboyPrvTick(m, 2);
		return true;
	}

	if ((opcode & 0xfe0fu) == 0x9200u) {
		uint16_t addr = arduboyPrvFlashWord(m, (uint16_t)(pc + 1u));

		arduboyPrvWriteData(m, addr, m->data[d]);
		m->pc = next;
		arduboyPrvTick(m, 2);
		return true;
	}

	if ((opcode & 0xfe0fu) == 0x900fu) {
		m->data[d] = arduboyPrvPop8(m, ARDUBOY_STACK_OP_POP);
		m->pc = next;
		arduboyPrvTick(m, 2);
		return true;
	}

	if ((opcode & 0xfe0fu) == 0x920fu) {
		arduboyPrvPush8(m, m->data[d], ARDUBOY_STACK_OP_PUSH);
		m->pc = next;
		arduboyPrvTick(m, 2);
		return true;
	}

	if ((opcode & 0xfe0eu) == 0x940cu) {
		m->pc = arduboyPrvFlashWord(m, (uint16_t)(pc + 1u));
		arduboyPrvTick(m, 3);
		return true;
	}

	if ((opcode & 0xfe0eu) == 0x940eu) {
		arduboyPrvPush16(m, next, ARDUBOY_STACK_OP_CALL);
		m->pc = arduboyPrvFlashWord(m, (uint16_t)(pc + 1u));
		arduboyPrvTick(m, 4);
		return true;
	}

	if ((opcode & 0xff00u) == 0x9600u || (opcode & 0xff00u) == 0x9700u) {
		uint8_t pair = (uint8_t)(24u + 2u * ((opcode >> 4) & 0x03u));
		uint8_t k = (uint8_t)((opcode & 0x0fu) | ((opcode >> 2) & 0x30u));
		uint16_t old = arduboyPrvGetPair(m, pair);
		uint16_t result = (opcode & 0x0100u) ? (uint16_t)(old - k) : (uint16_t)(old + k);
		uint8_t flags = 0;
		bool n = (result & 0x8000u) != 0;
		bool v;

		arduboyPrvSetPair(m, pair, result);
		if (opcode & 0x0100u) {
			v = ((old & ~result) & 0x8000u) != 0;
			if ((result & ~old) & 0x8000u)
				flags |= AVR_SREG_C;
		}
		else {
			v = ((~old & result) & 0x8000u) != 0;
			if ((~result & old) & 0x8000u)
				flags |= AVR_SREG_C;
		}
		if (!result)
			flags |= AVR_SREG_Z;
		if (n)
			flags |= AVR_SREG_N;
		if (v)
			flags |= AVR_SREG_V;
		if (n ^ v)
			flags |= AVR_SREG_S;
		arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S | AVR_SREG_C, flags);
		m->pc = next;
		arduboyPrvTick(m, 2);
		return true;
	}

	if ((opcode & 0xff00u) == 0x9800u || (opcode & 0xff00u) == 0x9a00u ||
		(opcode & 0xff00u) == 0x9900u || (opcode & 0xff00u) == 0x9b00u) {
		uint8_t ioAddr = (uint8_t)((opcode >> 3) & 0x1fu);
		uint8_t bit = (uint8_t)(opcode & 0x07u);
		uint16_t addr = (uint16_t)(0x20u + ioAddr);
		uint8_t value = arduboyPrvReadData(m, addr);

		switch (opcode & 0xff00u) {
			case 0x9800u:
				arduboyPrvWriteData(m, addr, (uint8_t)(value & ~(1u << bit)));
				break;
			case 0x9a00u:
				arduboyPrvWriteData(m, addr, (uint8_t)(value | (1u << bit)));
				break;
			case 0x9900u:
				if (!arduboyPrvIoBit(m, ioAddr, bit))
					next = (uint16_t)(next + arduboyPrvSkipWords(m, next));
				break;
			case 0x9b00u:
				if (arduboyPrvIoBit(m, ioAddr, bit))
					next = (uint16_t)(next + arduboyPrvSkipWords(m, next));
				break;
		}
		m->pc = next;
		arduboyPrvTick(m, 2);
		return true;
	}

	if ((opcode & 0xfc00u) == 0xf000u || (opcode & 0xfc00u) == 0xf400u) {
		uint8_t bit = (uint8_t)(opcode & 0x07u);
		int16_t rel = arduboyPrvSign((uint16_t)((opcode >> 3) & 0x7fu), 7);
		bool set = (m->data[AVR_SREG] & (1u << bit)) != 0;
		bool branch = ((opcode & 0x0400u) == 0) ? set : !set;

		m->pc = branch ? (uint16_t)(next + rel) : next;
		arduboyPrvTick(m, branch ? 2u : 1u);
		return true;
	}

	if ((opcode & 0xfe08u) == 0xfc00u || (opcode & 0xfe08u) == 0xfe00u) {
		r = arduboyPrvRegD(opcode);
		uint8_t bit = (uint8_t)(opcode & 0x07u);
		bool set = (m->data[r] & (1u << bit)) != 0;
		bool skip = ((opcode & 0x0200u) == 0) ? !set : set;

		if (skip)
			next = (uint16_t)(next + arduboyPrvSkipWords(m, next));
		m->pc = next;
		arduboyPrvTick(m, skip ? 2u : 1u);
		return true;
	}

	if ((opcode & 0xfe08u) == 0xf800u) {
		d = arduboyPrvRegD(opcode);
		uint8_t bit = (uint8_t)(opcode & 0x07u);
		uint8_t t = (m->data[AVR_SREG] & AVR_SREG_T) ? 1u : 0u;

		m->data[d] = (uint8_t)((m->data[d] & ~(1u << bit)) | (t << bit));
		m->pc = next;
		arduboyPrvTick(m, 1);
		return true;
	}

	if ((opcode & 0xfe08u) == 0xfa00u) {
		d = arduboyPrvRegD(opcode);
		uint8_t bit = (uint8_t)(opcode & 0x07u);

		arduboyPrvSetSreg(m, AVR_SREG_T, (m->data[d] & (1u << bit)) ? AVR_SREG_T : 0);
		m->pc = next;
		arduboyPrvTick(m, 1);
		return true;
	}

	if ((opcode & 0xfe0fu) == 0x9001u || (opcode & 0xfe0fu) == 0x9002u ||
		(opcode & 0xfe0fu) == 0x9009u || (opcode & 0xfe0fu) == 0x900au ||
		(opcode & 0xfe0fu) == 0x900cu || (opcode & 0xfe0fu) == 0x900du ||
		(opcode & 0xfe0fu) == 0x900eu || (opcode & 0xfe0fu) == 0x9004u ||
		(opcode & 0xfe0fu) == 0x9005u || (opcode & 0xfe0fu) == 0x9006u ||
		(opcode & 0xfe0fu) == 0x9007u) {
		uint8_t ptrReg = 26u;
		uint16_t addr;

		if ((opcode & 0x000fu) == 0x01u || (opcode & 0x000fu) == 0x02u ||
			(opcode & 0x000fu) == 0x04u || (opcode & 0x000fu) == 0x05u ||
			(opcode & 0x000fu) == 0x06u || (opcode & 0x000fu) == 0x07u)
			ptrReg = 30u;
		else if ((opcode & 0x000fu) == 0x09u || (opcode & 0x000fu) == 0x0au)
			ptrReg = 28u;
		addr = arduboyPrvGetPair(m, ptrReg);
		if ((opcode & 0x000fu) == 0x02u || (opcode & 0x000fu) == 0x0au || (opcode & 0x000fu) == 0x0eu) {
			addr--;
			arduboyPrvSetPair(m, ptrReg, addr);
		}
		if ((opcode & 0x000fu) == 0x04u || (opcode & 0x000fu) == 0x05u ||
			(opcode & 0x000fu) == 0x06u || (opcode & 0x000fu) == 0x07u) {
			uint32_t flashAddr = addr;
			m->data[d] = flashAddr < ARDUBOY_FLASH_SIZE ? m->flash[flashAddr] : 0xffu;
		}
		else
			m->data[d] = arduboyPrvReadData(m, addr);
		if ((opcode & 0x000fu) == 0x01u || (opcode & 0x000fu) == 0x05u ||
			(opcode & 0x000fu) == 0x07u || (opcode & 0x000fu) == 0x09u ||
			(opcode & 0x000fu) == 0x0du)
			arduboyPrvSetPair(m, ptrReg, (uint16_t)(addr + 1u));
		m->pc = next;
		arduboyPrvTick(m, 2);
		return true;
	}

	if ((opcode & 0xfe0fu) == 0x9201u || (opcode & 0xfe0fu) == 0x9202u ||
		(opcode & 0xfe0fu) == 0x9209u || (opcode & 0xfe0fu) == 0x920au ||
		(opcode & 0xfe0fu) == 0x920cu || (opcode & 0xfe0fu) == 0x920du ||
		(opcode & 0xfe0fu) == 0x920eu) {
		uint8_t ptrReg = 26u;
		uint16_t addr;

		if ((opcode & 0x000fu) == 0x01u || (opcode & 0x000fu) == 0x02u)
			ptrReg = 30u;
		else if ((opcode & 0x000fu) == 0x09u || (opcode & 0x000fu) == 0x0au)
			ptrReg = 28u;
		addr = arduboyPrvGetPair(m, ptrReg);
		if ((opcode & 0x000fu) == 0x02u || (opcode & 0x000fu) == 0x0au || (opcode & 0x000fu) == 0x0eu) {
			addr--;
			arduboyPrvSetPair(m, ptrReg, addr);
		}
		arduboyPrvWriteData(m, addr, m->data[d]);
		if ((opcode & 0x000fu) == 0x01u || (opcode & 0x000fu) == 0x09u || (opcode & 0x000fu) == 0x0du)
			arduboyPrvSetPair(m, ptrReg, (uint16_t)(addr + 1u));
		m->pc = next;
		arduboyPrvTick(m, 2);
		return true;
	}

	if ((opcode & 0xfe0fu) == 0x9204u || (opcode & 0xfe0fu) == 0x9205u ||
		(opcode & 0xfe0fu) == 0x9206u || (opcode & 0xfe0fu) == 0x9207u) {
		uint16_t addr = arduboyPrvGetPair(m, 30);
		uint8_t oldMem = arduboyPrvReadData(m, addr);
		uint8_t oldReg = m->data[d];
		uint8_t newMem = oldReg;

		switch (opcode & 0x000fu) {
			case 0x05u:
				newMem = (uint8_t)(oldMem | oldReg);
				break;
			case 0x06u:
				newMem = (uint8_t)(oldMem & ~oldReg);
				break;
			case 0x07u:
				newMem = (uint8_t)(oldMem ^ oldReg);
				break;
			default:
				break;
		}
		arduboyPrvWriteData(m, addr, newMem);
		m->data[d] = oldMem;
		m->pc = next;
		arduboyPrvTick(m, 2);
		return true;
	}

	if ((opcode & 0xfe0fu) == 0x9400u || (opcode & 0xfe0fu) == 0x9401u ||
		(opcode & 0xfe0fu) == 0x9402u || (opcode & 0xfe0fu) == 0x9403u ||
		(opcode & 0xfe0fu) == 0x9405u || (opcode & 0xfe0fu) == 0x9406u ||
		(opcode & 0xfe0fu) == 0x9407u || (opcode & 0xfe0fu) == 0x940au) {
		uint8_t old = m->data[d];
		uint8_t result = old;
		uint8_t flags = 0;

		switch (opcode & 0x000fu) {
			case 0x00u:
				result = (uint8_t)~old;
				if (!result)
					flags |= AVR_SREG_Z;
				if (result & 0x80u)
					flags |= AVR_SREG_N | AVR_SREG_S;
				flags |= AVR_SREG_C;
				arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S | AVR_SREG_C, flags);
				break;
			case 0x01u:
				result = (uint8_t)(0u - old);
				if ((result | old) & 0x08u)
					flags |= AVR_SREG_H;
				if (!result)
					flags |= AVR_SREG_Z;
				if (result & 0x80u)
					flags |= AVR_SREG_N;
				if (result == 0x80u)
					flags |= AVR_SREG_V;
				if (((flags & AVR_SREG_N) != 0) ^ ((flags & AVR_SREG_V) != 0))
					flags |= AVR_SREG_S;
				if (result)
					flags |= AVR_SREG_C;
				arduboyPrvSetSreg(m, AVR_SREG_H | AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S | AVR_SREG_C, flags);
				break;
			case 0x02u:
				result = (uint8_t)((old << 4) | (old >> 4));
				break;
			case 0x03u:
				result = (uint8_t)(old + 1u);
				if (!result)
					flags |= AVR_SREG_Z;
				if (result & 0x80u)
					flags |= AVR_SREG_N;
				if (result == 0x80u)
					flags |= AVR_SREG_V;
				if (((flags & AVR_SREG_N) != 0) ^ ((flags & AVR_SREG_V) != 0))
					flags |= AVR_SREG_S;
				arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S, flags);
				break;
			case 0x05u:
				result = (uint8_t)((old & 0x80u) | (old >> 1));
				if (!result)
					flags |= AVR_SREG_Z;
				if (result & 0x80u)
					flags |= AVR_SREG_N;
				if (old & 1u)
					flags |= AVR_SREG_C;
				if (((flags & AVR_SREG_N) != 0) ^ ((flags & AVR_SREG_C) != 0))
					flags |= AVR_SREG_V;
				if (((flags & AVR_SREG_N) != 0) ^ ((flags & AVR_SREG_V) != 0))
					flags |= AVR_SREG_S;
				arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S | AVR_SREG_C, flags);
				break;
			case 0x06u:
				result = (uint8_t)(old >> 1);
				if (!result)
					flags |= AVR_SREG_Z;
				if (old & 1u)
					flags |= AVR_SREG_C | AVR_SREG_V | AVR_SREG_S;
				arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S | AVR_SREG_C, flags);
				break;
			case 0x07u:
				result = (uint8_t)((old >> 1) | ((m->data[AVR_SREG] & AVR_SREG_C) ? 0x80u : 0u));
				if (!result)
					flags |= AVR_SREG_Z;
				if (result & 0x80u)
					flags |= AVR_SREG_N;
				if (old & 1u)
					flags |= AVR_SREG_C;
				if (((flags & AVR_SREG_N) != 0) ^ ((flags & AVR_SREG_C) != 0))
					flags |= AVR_SREG_V;
				if (((flags & AVR_SREG_N) != 0) ^ ((flags & AVR_SREG_V) != 0))
					flags |= AVR_SREG_S;
				arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S | AVR_SREG_C, flags);
				break;
			case 0x0au:
				result = (uint8_t)(old - 1u);
				if (!result)
					flags |= AVR_SREG_Z;
				if (result & 0x80u)
					flags |= AVR_SREG_N;
				if (result == 0x7fu)
					flags |= AVR_SREG_V;
				if (((flags & AVR_SREG_N) != 0) ^ ((flags & AVR_SREG_V) != 0))
					flags |= AVR_SREG_S;
				arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S, flags);
				break;
			default:
				arduboyPrvUnsupported(m, pc, opcode);
				return false;
		}
		m->data[d] = result;
		m->pc = next;
		arduboyPrvTick(m, 1);
		return true;
	}

	switch (opcode) {
		case 0x9409u:
		case 0x9419u:
			m->pc = arduboyPrvGetPair(m, 30);
			arduboyPrvTick(m, 2);
			return true;
		case 0x9509u:
		case 0x9519u:
			arduboyPrvPush16(m, next, ARDUBOY_STACK_OP_CALL);
			m->pc = arduboyPrvGetPair(m, 30);
			arduboyPrvTick(m, 3);
			return true;
		case 0x9508u:
			m->pc = arduboyPrvPop16(m, ARDUBOY_STACK_OP_RET);
			arduboyPrvTick(m, 4);
			return true;
		case 0x9518u:
			m->pc = arduboyPrvPop16(m, ARDUBOY_STACK_OP_RET);
			m->data[AVR_SREG] |= AVR_SREG_I;
			arduboyPrvTick(m, 4);
			return true;
		case 0x95c8u: {
			uint16_t z = arduboyPrvGetPair(m, 30);
			m->data[0] = z < ARDUBOY_FLASH_SIZE ? m->flash[z] : 0xffu;
			m->pc = next;
			arduboyPrvTick(m, 3);
			return true;
		}
		case 0x95d8u: {
			uint16_t z = arduboyPrvGetPair(m, 30);
			m->data[0] = z < ARDUBOY_FLASH_SIZE ? m->flash[z] : 0xffu;
			m->pc = next;
			arduboyPrvTick(m, 3);
			return true;
		}
		case 0x9588u:
		case 0x95a8u:
		case 0x95e8u:
		case 0x95f8u:
			m->pc = next;
			arduboyPrvTick(m, 1);
			return true;
		default:
			break;
	}

	if ((opcode & 0xff8fu) == 0x9408u) {
		uint8_t bit = (uint8_t)((opcode >> 4) & 0x07u);
		m->data[AVR_SREG] |= (uint8_t)(1u << bit);
		m->pc = next;
		arduboyPrvTick(m, 1);
		return true;
	}
	if ((opcode & 0xff8fu) == 0x9488u) {
		uint8_t bit = (uint8_t)((opcode >> 4) & 0x07u);
		m->data[AVR_SREG] &= (uint8_t)~(1u << bit);
		m->pc = next;
		arduboyPrvTick(m, 1);
		return true;
	}

	arduboyPrvUnsupported(m, pc, opcode);
	return false;
}

static bool arduboyPrvRemovedRawIsControlOpcode(uint16_t opcode)
{
	if ((opcode & 0xf000u) == 0xc000u || (opcode & 0xf000u) == 0xd000u)
		return true;
	if ((opcode & 0xfc00u) == 0xf000u || (opcode & 0xfc00u) == 0xf400u)
		return true;
	if ((opcode & 0xfe08u) == 0xfc00u || (opcode & 0xfe08u) == 0xfe00u)
		return true;
	if ((opcode & 0xfc00u) == 0x1000u)
		return true;
	if ((opcode & 0xff00u) == 0x9900u || (opcode & 0xff00u) == 0x9b00u)
		return true;
	if ((opcode & 0xfe0eu) == 0x940cu || (opcode & 0xfe0eu) == 0x940eu)
		return true;
	switch (opcode) {
		case 0x9409u:
		case 0x9419u:
		case 0x9509u:
		case 0x9519u:
		case 0x9508u:
		case 0x9518u:
			return true;
		default:
			return false;
	}
}

#endif

static void arduboyPrvDecodeMicroOp(ArduboyMachine *m, uint16_t pc, ArduboyCachedOp *op)
{
	uint16_t opcode = arduboyPrvFlashWord(m, pc);
	uint8_t d = arduboyPrvRegD(opcode);
	uint8_t r = arduboyPrvRegR(opcode);

	memset(op, 0, sizeof(*op));
	op->pc = pc;
	op->opcode = opcode;
	op->words = arduboyPrvOpcodeWords(opcode);
	op->next = (uint16_t)(pc + op->words);
	op->kind = ArduboyOpUnsupported;
	op->d = d;
	op->r = r;
	op->cycles = 1;

	if (opcode == 0x0000u) {
		op->kind = ArduboyOpNop;
		return;
	}
	if ((opcode & 0xff00u) == 0x0100u) {
		op->kind = ArduboyOpMovw;
		op->d = (uint8_t)(((opcode >> 4) & 0x0fu) << 1);
		op->r = (uint8_t)((opcode & 0x0fu) << 1);
		return;
	}
	if ((opcode & 0xfc00u) == 0x9c00u) {
		op->kind = ArduboyOpMul;
		op->cycles = 2;
		return;
	}
	if ((opcode & 0xff00u) == 0x0200u) {
		op->kind = ArduboyOpMuls;
		op->d = (uint8_t)(16u + ((opcode >> 4) & 0x0fu));
		op->r = (uint8_t)(16u + (opcode & 0x0fu));
		op->cycles = 2;
		return;
	}
	if ((opcode & 0xff00u) == 0x0300u) {
		op->kind = ArduboyOpFmul;
		op->d = (uint8_t)(16u + ((opcode >> 4) & 0x07u));
		op->r = (uint8_t)(16u + (opcode & 0x07u));
		op->aux = (uint8_t)((opcode >> 3) & 0x03u);
		op->cycles = 2;
		return;
	}
	if ((opcode >> 10) <= 0x0bu) {
		op->kind = ArduboyOpAluReg;
		op->aux = (uint8_t)(opcode >> 10);
		op->control = (opcode & 0xfc00u) == 0x1000u;
		return;
	}
	if ((opcode & 0xf000u) >= 0x3000u && (opcode & 0xf000u) <= 0x7000u) {
		op->kind = ArduboyOpImm;
		op->d = (uint8_t)(16u + ((opcode >> 4) & 0x0fu));
		op->k = arduboyPrvImm8(opcode);
		op->aux = (uint8_t)(opcode >> 12);
		return;
	}
	if ((opcode & 0xd208u) == 0x8000u || (opcode & 0xd208u) == 0x8008u ||
		(opcode & 0xd208u) == 0x8200u || (opcode & 0xd208u) == 0x8208u) {
		op->kind = ArduboyOpLdStQ;
		op->d = arduboyPrvRegD(opcode);
		op->q = arduboyPrvLdq(opcode);
		op->r = (opcode & 0x0008u) ? 28u : 30u;
		op->aux = (opcode & 0x0200u) ? 1u : 0u;
		op->cycles = 2;
		return;
	}
	if ((opcode & 0xf000u) == 0xe000u) {
		op->kind = ArduboyOpLdi;
		op->d = (uint8_t)(16u + ((opcode >> 4) & 0x0fu));
		op->k = arduboyPrvImm8(opcode);
		return;
	}
	if ((opcode & 0xf000u) == 0xb000u || (opcode & 0xf000u) == 0xb800u) {
		op->kind = ArduboyOpIo;
		op->d = arduboyPrvRegD(opcode);
		op->ioAddr = (uint8_t)((opcode & 0x0fu) | ((opcode >> 5) & 0x30u));
		op->aux = (opcode & 0x0800u) ? 1u : 0u;
		return;
	}
	if ((opcode & 0xf000u) == 0xc000u) {
		op->kind = ArduboyOpRjmp;
		op->target = (uint16_t)(op->next + arduboyPrvSign(opcode, 12));
		op->cycles = 2;
		op->control = 1;
		return;
	}
	if ((opcode & 0xf000u) == 0xd000u) {
		op->kind = ArduboyOpRcall;
		op->target = (uint16_t)(op->next + arduboyPrvSign(opcode, 12));
		op->cycles = 3;
		op->control = 1;
		return;
	}
	if ((opcode & 0xfe0fu) == 0x9000u) {
		op->kind = ArduboyOpLds;
		op->imm16 = arduboyPrvFlashWord(m, (uint16_t)(pc + 1u));
		op->cycles = 2;
		return;
	}
	if ((opcode & 0xfe0fu) == 0x9200u) {
		op->kind = ArduboyOpSts;
		op->imm16 = arduboyPrvFlashWord(m, (uint16_t)(pc + 1u));
		op->cycles = 2;
		return;
	}
	if ((opcode & 0xfe0fu) == 0x900fu) {
		op->kind = ArduboyOpPop;
		op->cycles = 2;
		return;
	}
	if ((opcode & 0xfe0fu) == 0x920fu) {
		op->kind = ArduboyOpPush;
		op->cycles = 2;
		return;
	}
	if ((opcode & 0xfe0eu) == 0x940cu) {
		op->kind = ArduboyOpJmp;
		op->target = arduboyPrvFlashWord(m, (uint16_t)(pc + 1u));
		op->cycles = 3;
		op->control = 1;
		return;
	}
	if ((opcode & 0xfe0eu) == 0x940eu) {
		op->kind = ArduboyOpCall;
		op->target = arduboyPrvFlashWord(m, (uint16_t)(pc + 1u));
		op->cycles = 4;
		op->control = 1;
		return;
	}
	if ((opcode & 0xff00u) == 0x9600u || (opcode & 0xff00u) == 0x9700u) {
		op->kind = ArduboyOpAdiw;
		op->d = (uint8_t)(24u + 2u * ((opcode >> 4) & 0x03u));
		op->k = (uint8_t)((opcode & 0x0fu) | ((opcode >> 2) & 0x30u));
		op->aux = (opcode & 0x0100u) ? 1u : 0u;
		op->cycles = 2;
		return;
	}
	if ((opcode & 0xff00u) == 0x9800u || (opcode & 0xff00u) == 0x9a00u ||
		(opcode & 0xff00u) == 0x9900u || (opcode & 0xff00u) == 0x9b00u) {
		op->kind = ArduboyOpIoBit;
		op->ioAddr = (uint8_t)((opcode >> 3) & 0x1fu);
		op->bit = (uint8_t)(opcode & 0x07u);
		op->aux = (uint8_t)((opcode >> 8) & 0x03u);
		op->cycles = 2;
		op->control = op->aux & 1u;
		return;
	}
	if ((opcode & 0xfc00u) == 0xf000u || (opcode & 0xfc00u) == 0xf400u) {
		op->kind = ArduboyOpBranch;
		op->bit = (uint8_t)(opcode & 0x07u);
		op->aux = (opcode & 0x0400u) ? 0u : 1u;
		op->target = (uint16_t)(op->next + arduboyPrvSign((uint16_t)((opcode >> 3) & 0x7fu), 7));
		op->control = 1;
		return;
	}
	if ((opcode & 0xfe08u) == 0xfc00u || (opcode & 0xfe08u) == 0xfe00u) {
		op->kind = ArduboyOpSkipRegBit;
		op->d = arduboyPrvRegD(opcode);
		op->bit = (uint8_t)(opcode & 0x07u);
		op->aux = (opcode & 0x0200u) ? 1u : 0u;
		op->control = 1;
		return;
	}
	if ((opcode & 0xfe08u) == 0xf800u) {
		op->kind = ArduboyOpBld;
		op->d = arduboyPrvRegD(opcode);
		op->bit = (uint8_t)(opcode & 0x07u);
		return;
	}
	if ((opcode & 0xfe08u) == 0xfa00u) {
		op->kind = ArduboyOpBst;
		op->d = arduboyPrvRegD(opcode);
		op->bit = (uint8_t)(opcode & 0x07u);
		return;
	}
	if ((opcode & 0xfe0fu) == 0x9001u || (opcode & 0xfe0fu) == 0x9002u ||
		(opcode & 0xfe0fu) == 0x9009u || (opcode & 0xfe0fu) == 0x900au ||
		(opcode & 0xfe0fu) == 0x900cu || (opcode & 0xfe0fu) == 0x900du ||
		(opcode & 0xfe0fu) == 0x900eu || (opcode & 0xfe0fu) == 0x9004u ||
		(opcode & 0xfe0fu) == 0x9005u || (opcode & 0xfe0fu) == 0x9006u ||
		(opcode & 0xfe0fu) == 0x9007u) {
		op->kind = ArduboyOpLdPtr;
		op->aux = (uint8_t)(opcode & 0x000fu);
		op->r = 26u;
		if (op->aux == 0x01u || op->aux == 0x02u || op->aux == 0x04u ||
			op->aux == 0x05u || op->aux == 0x06u || op->aux == 0x07u)
			op->r = 30u;
		else if (op->aux == 0x09u || op->aux == 0x0au)
			op->r = 28u;
		op->cycles = 2;
		return;
	}
	if ((opcode & 0xfe0fu) == 0x9201u || (opcode & 0xfe0fu) == 0x9202u ||
		(opcode & 0xfe0fu) == 0x9209u || (opcode & 0xfe0fu) == 0x920au ||
		(opcode & 0xfe0fu) == 0x920cu || (opcode & 0xfe0fu) == 0x920du ||
		(opcode & 0xfe0fu) == 0x920eu) {
		op->kind = ArduboyOpStPtr;
		op->aux = (uint8_t)(opcode & 0x000fu);
		op->r = 26u;
		if (op->aux == 0x01u || op->aux == 0x02u)
			op->r = 30u;
		else if (op->aux == 0x09u || op->aux == 0x0au)
			op->r = 28u;
		op->cycles = 2;
		return;
	}
	if ((opcode & 0xfe0fu) == 0x9204u || (opcode & 0xfe0fu) == 0x9205u ||
		(opcode & 0xfe0fu) == 0x9206u || (opcode & 0xfe0fu) == 0x9207u) {
		op->kind = ArduboyOpZMem;
		op->aux = (uint8_t)(opcode & 0x000fu);
		op->cycles = 2;
		return;
	}
	if ((opcode & 0xfe0fu) == 0x9400u || (opcode & 0xfe0fu) == 0x9401u ||
		(opcode & 0xfe0fu) == 0x9402u || (opcode & 0xfe0fu) == 0x9403u ||
		(opcode & 0xfe0fu) == 0x9405u || (opcode & 0xfe0fu) == 0x9406u ||
		(opcode & 0xfe0fu) == 0x9407u || (opcode & 0xfe0fu) == 0x940au) {
		op->kind = ArduboyOpUnary;
		op->aux = (uint8_t)(opcode & 0x000fu);
		return;
	}
	switch (opcode) {
		case 0x9409u:
		case 0x9419u:
			op->kind = ArduboyOpIjmp;
			op->cycles = 2;
			op->control = 1;
			return;
		case 0x9509u:
		case 0x9519u:
			op->kind = ArduboyOpIcall;
			op->cycles = 3;
			op->control = 1;
			return;
		case 0x9508u:
			op->kind = ArduboyOpRet;
			op->cycles = 4;
			op->control = 1;
			return;
		case 0x9518u:
			op->kind = ArduboyOpReti;
			op->cycles = 4;
			op->control = 1;
			return;
		case 0x95c8u:
		case 0x95d8u:
			op->kind = ArduboyOpLpm;
			op->cycles = 3;
			return;
		case 0x9588u:
		case 0x95a8u:
		case 0x95e8u:
		case 0x95f8u:
			op->kind = ArduboyOpNoopSpecial;
			return;
		default:
			break;
	}
	if ((opcode & 0xff8fu) == 0x9408u || (opcode & 0xff8fu) == 0x9488u) {
		op->kind = ArduboyOpSregBit;
		op->bit = (uint8_t)((opcode >> 4) & 0x07u);
		op->aux = ((opcode & 0xff8fu) == 0x9408u) ? 1u : 0u;
		return;
	}
}

static bool ARDUBOY_FAST arduboyPrvExecuteDecodedOp(ArduboyMachine *m, const ArduboyCachedOp *op, uint32_t *cyclesP)
{
	uint8_t flags = 0;

	m->lastPc = op->pc;
	m->lastOpcode = op->opcode;
	switch (op->kind) {
		case ArduboyOpNop:
			m->pc = op->next;
			break;
		case ArduboyOpMovw:
			m->data[op->d] = m->data[op->r];
			m->data[op->d + 1u] = m->data[op->r + 1u];
			m->pc = op->next;
			break;
		case ArduboyOpMul: {
			uint16_t result = (uint16_t)m->data[op->d] * m->data[op->r];
			m->data[0] = (uint8_t)result;
			m->data[1] = (uint8_t)(result >> 8);
			if (!result)
				flags |= AVR_SREG_Z;
			if (result & 0x8000u)
				flags |= AVR_SREG_C;
			arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_C, flags);
			m->pc = op->next;
			break;
		}
		case ArduboyOpMuls: {
			int16_t result = (int16_t)((int8_t)m->data[op->d] * (int8_t)m->data[op->r]);
			m->data[0] = (uint8_t)result;
			m->data[1] = (uint8_t)((uint16_t)result >> 8);
			if (!result)
				flags |= AVR_SREG_Z;
			if ((uint16_t)result & 0x8000u)
				flags |= AVR_SREG_C;
			arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_C, flags);
			m->pc = op->next;
			break;
		}
		case ArduboyOpFmul: {
			int32_t product;
			uint16_t result;
			switch (op->aux) {
				case 0:
					product = (int32_t)((int8_t)m->data[op->d]) * m->data[op->r];
					result = (uint16_t)product;
					break;
				case 1:
					product = (uint16_t)m->data[op->d] * (uint16_t)m->data[op->r];
					result = (uint16_t)(product * 2);
					break;
				case 2:
					product = (int32_t)((int8_t)m->data[op->d]) * (int8_t)m->data[op->r];
					result = (uint16_t)(product * 2);
					break;
				default:
					product = (int32_t)((int8_t)m->data[op->d]) * m->data[op->r];
					result = (uint16_t)(product * 2);
					break;
			}
			m->data[0] = (uint8_t)result;
			m->data[1] = (uint8_t)(result >> 8);
			if (!result)
				flags |= AVR_SREG_Z;
			if (result & 0x8000u)
				flags |= AVR_SREG_C;
			arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_C, flags);
			m->pc = op->next;
			break;
		}
		case ArduboyOpAluReg: {
			uint8_t a = m->data[op->d], b = m->data[op->r];
			switch (op->aux) {
				case 1:
					(void)arduboyPrvSub8(m, a, b, m->data[AVR_SREG] & AVR_SREG_C, true);
					break;
				case 2:
					m->data[op->d] = arduboyPrvSub8(m, a, b, m->data[AVR_SREG] & AVR_SREG_C, true);
					break;
				case 3:
					m->data[op->d] = arduboyPrvAdd8(m, a, b, 0);
					break;
				case 4:
					if (a == b)
						m->pc = (uint16_t)(op->next + arduboyPrvSkipWords(m, op->next));
					else
						m->pc = op->next;
					*cyclesP += op->cycles;
					return true;
				case 5:
					(void)arduboyPrvSub8(m, a, b, 0, false);
					break;
				case 6:
					m->data[op->d] = arduboyPrvSub8(m, a, b, 0, false);
					break;
				case 7:
					m->data[op->d] = arduboyPrvAdd8(m, a, b, m->data[AVR_SREG] & AVR_SREG_C);
					break;
				case 8:
					m->data[op->d] = (uint8_t)(a & b);
					arduboyPrvFlagsLogic(m, m->data[op->d]);
					break;
				case 9:
					m->data[op->d] = (uint8_t)(a ^ b);
					arduboyPrvFlagsLogic(m, m->data[op->d]);
					break;
				case 10:
					m->data[op->d] = (uint8_t)(a | b);
					arduboyPrvFlagsLogic(m, m->data[op->d]);
					break;
				case 11:
					m->data[op->d] = b;
					break;
				default:
					arduboyPrvUnsupported(m, op->pc, op->opcode);
					return false;
			}
			m->pc = op->next;
			break;
		}
		case ArduboyOpImm:
			switch (op->aux) {
				case 3:
					(void)arduboyPrvSub8(m, m->data[op->d], op->k, 0, false);
					break;
				case 4:
					m->data[op->d] = arduboyPrvSub8(m, m->data[op->d], op->k, m->data[AVR_SREG] & AVR_SREG_C, true);
					break;
				case 5:
					m->data[op->d] = arduboyPrvSub8(m, m->data[op->d], op->k, 0, false);
					break;
				case 6:
					m->data[op->d] = (uint8_t)(m->data[op->d] | op->k);
					arduboyPrvFlagsLogic(m, m->data[op->d]);
					break;
				case 7:
					m->data[op->d] = (uint8_t)(m->data[op->d] & op->k);
					arduboyPrvFlagsLogic(m, m->data[op->d]);
					break;
			}
			m->pc = op->next;
			break;
		case ArduboyOpLdStQ: {
			uint16_t addr = (uint16_t)(arduboyPrvGetPair(m, op->r) + op->q);
			if (op->aux)
				arduboyPrvWriteDataFast(m, addr, m->data[op->d], cyclesP);
			else
				m->data[op->d] = arduboyPrvReadDataFast(m, addr, cyclesP);
			m->pc = op->next;
			break;
		}
		case ArduboyOpLdi:
			m->data[op->d] = op->k;
			m->pc = op->next;
			break;
		case ArduboyOpIo:
			arduboyPrvFlushCycles(m, cyclesP);
			if (op->aux)
				arduboyPrvWriteData(m, (uint16_t)(0x20u + op->ioAddr), m->data[op->d]);
			else
				m->data[op->d] = arduboyPrvReadData(m, (uint16_t)(0x20u + op->ioAddr));
			m->pc = op->next;
			break;
		case ArduboyOpRjmp:
			m->pc = op->target;
			break;
		case ArduboyOpRcall:
			arduboyPrvPush16(m, op->next, ARDUBOY_STACK_OP_CALL);
			m->pc = op->target;
			break;
		case ArduboyOpLds:
			m->data[op->d] = arduboyPrvReadDataFast(m, op->imm16, cyclesP);
			m->pc = op->next;
			break;
		case ArduboyOpSts:
			arduboyPrvWriteDataFast(m, op->imm16, m->data[op->d], cyclesP);
			m->pc = op->next;
			break;
		case ArduboyOpPop:
			m->data[op->d] = arduboyPrvPop8(m, ARDUBOY_STACK_OP_POP);
			m->pc = op->next;
			break;
		case ArduboyOpPush:
			arduboyPrvPush8(m, m->data[op->d], ARDUBOY_STACK_OP_PUSH);
			m->pc = op->next;
			break;
		case ArduboyOpJmp:
			m->pc = op->target;
			break;
		case ArduboyOpCall:
			arduboyPrvPush16(m, op->next, ARDUBOY_STACK_OP_CALL);
			m->pc = op->target;
			break;
		case ArduboyOpAdiw: {
			uint16_t old = arduboyPrvGetPair(m, op->d);
			uint16_t result = op->aux ? (uint16_t)(old - op->k) : (uint16_t)(old + op->k);
			bool n = (result & 0x8000u) != 0;
			bool v;
			arduboyPrvSetPair(m, op->d, result);
			if (op->aux) {
				v = ((old & ~result) & 0x8000u) != 0;
				if ((result & ~old) & 0x8000u)
					flags |= AVR_SREG_C;
			}
			else {
				v = ((~old & result) & 0x8000u) != 0;
				if ((~result & old) & 0x8000u)
					flags |= AVR_SREG_C;
			}
			if (!result)
				flags |= AVR_SREG_Z;
			if (n)
				flags |= AVR_SREG_N;
			if (v)
				flags |= AVR_SREG_V;
			if (n ^ v)
				flags |= AVR_SREG_S;
			arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S | AVR_SREG_C, flags);
			m->pc = op->next;
			break;
		}
		case ArduboyOpIoBit: {
			uint16_t addr = (uint16_t)(0x20u + op->ioAddr);
			uint8_t value;
			arduboyPrvFlushCycles(m, cyclesP);
			value = arduboyPrvReadData(m, addr);
			if (op->aux == 0u)
				arduboyPrvWriteData(m, addr, (uint8_t)(value & ~(1u << op->bit)));
			else if (op->aux == 2u)
				arduboyPrvWriteData(m, addr, (uint8_t)(value | (1u << op->bit)));
			else if (op->aux == 1u) {
				if (!(value & (1u << op->bit)))
					m->pc = (uint16_t)(op->next + arduboyPrvSkipWords(m, op->next));
				else
					m->pc = op->next;
				break;
			}
			else {
				if (value & (1u << op->bit))
					m->pc = (uint16_t)(op->next + arduboyPrvSkipWords(m, op->next));
				else
					m->pc = op->next;
				break;
			}
			m->pc = op->next;
			break;
		}
		case ArduboyOpBranch: {
			bool set = (m->data[AVR_SREG] & (1u << op->bit)) != 0;
			bool branch = op->aux ? set : !set;
			m->pc = branch ? op->target : op->next;
			if (branch)
				(*cyclesP)++;
			break;
		}
		case ArduboyOpSkipRegBit: {
			bool set = (m->data[op->d] & (1u << op->bit)) != 0;
			bool skip = op->aux ? set : !set;
			m->pc = skip ? (uint16_t)(op->next + arduboyPrvSkipWords(m, op->next)) : op->next;
			if (skip)
				(*cyclesP)++;
			break;
		}
		case ArduboyOpBld: {
			uint8_t t = (m->data[AVR_SREG] & AVR_SREG_T) ? 1u : 0u;
			m->data[op->d] = (uint8_t)((m->data[op->d] & ~(1u << op->bit)) | (t << op->bit));
			m->pc = op->next;
			break;
		}
		case ArduboyOpBst:
			arduboyPrvSetSreg(m, AVR_SREG_T, (m->data[op->d] & (1u << op->bit)) ? AVR_SREG_T : 0);
			m->pc = op->next;
			break;
		case ArduboyOpLdPtr: {
			uint16_t addr = arduboyPrvGetPair(m, op->r);
			if (op->aux == 0x02u || op->aux == 0x0au || op->aux == 0x0eu) {
				addr--;
				arduboyPrvSetPair(m, op->r, addr);
			}
			if (op->aux == 0x04u || op->aux == 0x05u || op->aux == 0x06u || op->aux == 0x07u)
				m->data[op->d] = addr < ARDUBOY_FLASH_SIZE ? m->flash[addr] : 0xffu;
			else
				m->data[op->d] = arduboyPrvReadDataFast(m, addr, cyclesP);
			if (op->aux == 0x01u || op->aux == 0x05u || op->aux == 0x07u ||
				op->aux == 0x09u || op->aux == 0x0du)
				arduboyPrvSetPair(m, op->r, (uint16_t)(addr + 1u));
			m->pc = op->next;
			break;
		}
		case ArduboyOpStPtr: {
			uint16_t addr = arduboyPrvGetPair(m, op->r);
			if (op->aux == 0x02u || op->aux == 0x0au || op->aux == 0x0eu) {
				addr--;
				arduboyPrvSetPair(m, op->r, addr);
			}
			arduboyPrvWriteDataFast(m, addr, m->data[op->d], cyclesP);
			if (op->aux == 0x01u || op->aux == 0x09u || op->aux == 0x0du)
				arduboyPrvSetPair(m, op->r, (uint16_t)(addr + 1u));
			m->pc = op->next;
			break;
		}
		case ArduboyOpZMem: {
			uint16_t addr = arduboyPrvGetPair(m, 30);
			uint8_t oldMem = arduboyPrvReadDataFast(m, addr, cyclesP);
			uint8_t oldReg = m->data[op->d];
			uint8_t newMem = oldReg;
			if (op->aux == 0x05u)
				newMem = (uint8_t)(oldMem | oldReg);
			else if (op->aux == 0x06u)
				newMem = (uint8_t)(oldMem & ~oldReg);
			else if (op->aux == 0x07u)
				newMem = (uint8_t)(oldMem ^ oldReg);
			arduboyPrvWriteDataFast(m, addr, newMem, cyclesP);
			m->data[op->d] = oldMem;
			m->pc = op->next;
			break;
		}
		case ArduboyOpUnary: {
			uint8_t old = m->data[op->d];
			uint8_t result = old;
			switch (op->aux) {
				case 0x00u:
					result = (uint8_t)~old;
					if (!result)
						flags |= AVR_SREG_Z;
					if (result & 0x80u)
						flags |= AVR_SREG_N | AVR_SREG_S;
					flags |= AVR_SREG_C;
					arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S | AVR_SREG_C, flags);
					break;
				case 0x01u:
					result = (uint8_t)(0u - old);
					if ((result | old) & 0x08u)
						flags |= AVR_SREG_H;
					if (!result)
						flags |= AVR_SREG_Z;
					if (result & 0x80u)
						flags |= AVR_SREG_N;
					if (result == 0x80u)
						flags |= AVR_SREG_V;
					if (((flags & AVR_SREG_N) != 0) ^ ((flags & AVR_SREG_V) != 0))
						flags |= AVR_SREG_S;
					if (result)
						flags |= AVR_SREG_C;
					arduboyPrvSetSreg(m, AVR_SREG_H | AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S | AVR_SREG_C, flags);
					break;
				case 0x02u:
					result = (uint8_t)((old << 4) | (old >> 4));
					break;
				case 0x03u:
					result = (uint8_t)(old + 1u);
					if (!result)
						flags |= AVR_SREG_Z;
					if (result & 0x80u)
						flags |= AVR_SREG_N;
					if (result == 0x80u)
						flags |= AVR_SREG_V;
					if (((flags & AVR_SREG_N) != 0) ^ ((flags & AVR_SREG_V) != 0))
						flags |= AVR_SREG_S;
					arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S, flags);
					break;
				case 0x05u:
					result = (uint8_t)((old & 0x80u) | (old >> 1));
					if (!result)
						flags |= AVR_SREG_Z;
					if (result & 0x80u)
						flags |= AVR_SREG_N;
					if (old & 1u)
						flags |= AVR_SREG_C;
					if (((flags & AVR_SREG_N) != 0) ^ ((flags & AVR_SREG_C) != 0))
						flags |= AVR_SREG_V;
					if (((flags & AVR_SREG_N) != 0) ^ ((flags & AVR_SREG_V) != 0))
						flags |= AVR_SREG_S;
					arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S | AVR_SREG_C, flags);
					break;
				case 0x06u:
					result = (uint8_t)(old >> 1);
					if (!result)
						flags |= AVR_SREG_Z;
					if (old & 1u)
						flags |= AVR_SREG_C | AVR_SREG_V | AVR_SREG_S;
					arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S | AVR_SREG_C, flags);
					break;
				case 0x07u:
					result = (uint8_t)((old >> 1) | ((m->data[AVR_SREG] & AVR_SREG_C) ? 0x80u : 0u));
					if (!result)
						flags |= AVR_SREG_Z;
					if (result & 0x80u)
						flags |= AVR_SREG_N;
					if (old & 1u)
						flags |= AVR_SREG_C;
					if (((flags & AVR_SREG_N) != 0) ^ ((flags & AVR_SREG_C) != 0))
						flags |= AVR_SREG_V;
					if (((flags & AVR_SREG_N) != 0) ^ ((flags & AVR_SREG_V) != 0))
						flags |= AVR_SREG_S;
					arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S | AVR_SREG_C, flags);
					break;
				case 0x0au:
					result = (uint8_t)(old - 1u);
					if (!result)
						flags |= AVR_SREG_Z;
					if (result & 0x80u)
						flags |= AVR_SREG_N;
					if (result == 0x7fu)
						flags |= AVR_SREG_V;
					if (((flags & AVR_SREG_N) != 0) ^ ((flags & AVR_SREG_V) != 0))
						flags |= AVR_SREG_S;
					arduboyPrvSetSreg(m, AVR_SREG_Z | AVR_SREG_N | AVR_SREG_V | AVR_SREG_S, flags);
					break;
				default:
					arduboyPrvUnsupported(m, op->pc, op->opcode);
					return false;
			}
			m->data[op->d] = result;
			m->pc = op->next;
			break;
		}
		case ArduboyOpIjmp:
			m->pc = arduboyPrvGetPair(m, 30);
			break;
		case ArduboyOpIcall:
			arduboyPrvPush16(m, op->next, ARDUBOY_STACK_OP_CALL);
			m->pc = arduboyPrvGetPair(m, 30);
			break;
		case ArduboyOpRet:
			m->pc = arduboyPrvPop16(m, ARDUBOY_STACK_OP_RET);
			break;
		case ArduboyOpReti:
			m->pc = arduboyPrvPop16(m, ARDUBOY_STACK_OP_RET);
			m->data[AVR_SREG] |= AVR_SREG_I;
			break;
		case ArduboyOpLpm: {
			uint16_t z = arduboyPrvGetPair(m, 30);
			m->data[0] = z < ARDUBOY_FLASH_SIZE ? m->flash[z] : 0xffu;
			m->pc = op->next;
			break;
		}
		case ArduboyOpNoopSpecial:
			m->pc = op->next;
			break;
		case ArduboyOpSregBit:
			if (op->aux)
				m->data[AVR_SREG] |= (uint8_t)(1u << op->bit);
			else
				m->data[AVR_SREG] &= (uint8_t)~(1u << op->bit);
			m->pc = op->next;
			break;
		default:
			arduboyPrvUnsupported(m, op->pc, op->opcode);
			return false;
	}
	*cyclesP += op->cycles;
	return true;
}

static ArduboyBlock *arduboyPrvDecodeBlock(ArduboyMachine *m, uint16_t pc)
{
	uint16_t mixedPc = (uint16_t)(pc ^ (pc >> 4) ^ (pc >> 9));
	ArduboyBlock *block = &m->blocks[mixedPc & (ARDUBOY_BLOCK_CACHE_SIZE - 1u)];
	uint8_t opCount = 0;

	if (block->valid && block->pc == pc) {
		m->cacheHits++;
#ifdef ARDUBOY_PERF
		m->perfCacheHits++;
#endif
		return block;
	}
	m->cacheMisses++;
#ifdef ARDUBOY_PERF
	m->perfCacheMisses++;
#endif
	while (opCount < ARDUBOY_BLOCK_MAX_OPS) {
		ArduboyCachedOp *op = &block->ops[opCount];

		arduboyPrvDecodeMicroOp(m, pc, op);
		opCount++;
		pc = op->next;
		if (op->control)
			break;
	}
	block->pc = (uint16_t)(opCount ? block->ops[0].pc : pc);
	block->opCount = opCount;
	block->valid = 1;
	m->decodedBlocks++;
#ifdef ARDUBOY_PERF
	m->perfDecodedBlocks++;
#endif
	return block;
}

static bool ARDUBOY_FAST arduboyPrvRjmpTargets(uint16_t pc, uint16_t opcode, uint16_t target)
{
	if ((opcode & 0xf000u) != 0xc000u)
		return false;
	return (uint16_t)(pc + 1u + arduboyPrvSign(opcode, 12)) == target;
}

static bool ARDUBOY_FAST arduboyPrvCyclesUntilIoBit(ArduboyMachine *m, uint16_t addr, uint8_t bit,
	bool wantSet, uint32_t *cyclesP)
{
	uint8_t mask = (uint8_t)(1u << bit);
	uint8_t value = arduboyPrvReadData(m, addr);
	uint16_t prescale;
	uint32_t ticks;

	if (((value & mask) != 0) == wantSet) {
		*cyclesP = 0;
		return true;
	}
	if (addr == AVR_SPSR && bit == 7u && wantSet) {
		m->data[AVR_SPSR] |= AVR_SPSR_SPIF;
		*cyclesP = 0;
		return true;
	}
	if (addr == AVR_ADCSRA && ((bit == 4u && wantSet) || (bit == 6u && !wantSet))) {
		arduboyPrvCompleteAdc(m);
		*cyclesP = 0;
		return true;
	}
	if (addr != AVR_TIFR0 || !wantSet)
		return false;

	prescale = arduboyPrvTimer0Prescale(m);
	if (!prescale)
		return false;
	switch (mask) {
		case AVR_TIFR0_TOV0:
			ticks = 256u - m->data[AVR_TCNT0];
			break;
		case AVR_TIFR0_OCF0A:
			ticks = (uint8_t)(m->data[AVR_OCR0A] - m->data[AVR_TCNT0]);
			if (!ticks)
				ticks = 256u;
			break;
		case AVR_TIFR0_OCF0B:
			ticks = (uint8_t)(m->data[AVR_OCR0B] - m->data[AVR_TCNT0]);
			if (!ticks)
				ticks = 256u;
			break;
		default:
			return false;
	}
	*cyclesP = ticks * prescale;
	if (*cyclesP > m->timer0Cycles)
		*cyclesP -= m->timer0Cycles;
	else
		*cyclesP = 0;
	return true;
}

static bool ARDUBOY_FAST arduboyPrvTryCollapsePollingLoop(ArduboyMachine *m)
{
	uint16_t pc = m->pc;
	uint16_t op0 = arduboyPrvFlashWord(m, pc);
	uint16_t op1 = arduboyPrvFlashWord(m, (uint16_t)(pc + 1u));
	uint16_t op2 = arduboyPrvFlashWord(m, (uint16_t)(pc + 2u));
	uint32_t waitCycles;

	if ((op0 & 0xff00u) == 0x9900u || (op0 & 0xff00u) == 0x9b00u) {
		uint8_t ioAddr = (uint8_t)((op0 >> 3) & 0x1fu);
		uint8_t bit = (uint8_t)(op0 & 0x07u);
		bool wantSet = (op0 & 0xff00u) == 0x9b00u;

		if (arduboyPrvRjmpTargets((uint16_t)(pc + 1u), op1, pc) &&
			arduboyPrvCyclesUntilIoBit(m, (uint16_t)(0x20u + ioAddr), bit, wantSet, &waitCycles)) {
			arduboyPrvTimer0Advance(m, waitCycles + 3u);
			m->pc = (uint16_t)(pc + 2u);
			m->pollingCollapses++;
			return true;
		}
	}
	if ((op0 & 0xf800u) == 0xb000u &&
		((op1 & 0xfe08u) == 0xfc00u || (op1 & 0xfe08u) == 0xfe00u) &&
		arduboyPrvRjmpTargets((uint16_t)(pc + 2u), op2, pc)) {
		uint8_t ioAddr = (uint8_t)((op0 & 0x0fu) | ((op0 >> 5) & 0x30u));
		uint8_t reg = arduboyPrvRegD(op0);
		uint8_t skipReg = arduboyPrvRegD(op1);
		uint8_t bit = (uint8_t)(op1 & 0x07u);
		bool wantSet = (op1 & 0x0200u) != 0;

		if (reg == skipReg &&
			arduboyPrvCyclesUntilIoBit(m, (uint16_t)(0x20u + ioAddr), bit, wantSet, &waitCycles)) {
			arduboyPrvTimer0Advance(m, waitCycles + 4u);
			m->data[reg] = arduboyPrvReadData(m, (uint16_t)(0x20u + ioAddr));
			m->pc = (uint16_t)(pc + 3u);
			m->pollingCollapses++;
			return true;
		}
	}
	return false;
}

static void arduboyPrvNoteHotPc(ArduboyMachine *m, uint16_t pc)
{
	for (uint8_t h = 0; h < 4u; h++) {
		if (m->hotPc[h] == pc) {
			m->hotCount[h]++;
			return;
		}
		if (!m->hotCount[h]) {
			m->hotPc[h] = pc;
			m->hotCount[h] = 1;
			return;
		}
	}
}

static bool ARDUBOY_FAST arduboyPrvRunBlock(ArduboyMachine *m)
{
	ArduboyBlock *block;
	uint32_t cycleBatch = 0;

	if (!arduboyPrvPcIsExecutable(m, m->pc)) {
		m->unsupported = 1;
		m->failureReason = (m->lastHleKind != ArduboyHleNone && m->lastHleReturnPc == m->pc) ?
			ARDUBOY_FAIL_HLE_BAD_RETURN : ARDUBOY_FAIL_INVALID_PC;
		m->unsupportedPc = m->pc;
		m->unsupportedOpcode = arduboyPrvFlashWord(m, m->pc);
		return false;
	}
	if (arduboyPrvExecuteHle(m))
		return true;
	if (arduboyPrvTryCollapsePollingLoop(m))
		return true;
	arduboyPrvServiceInterrupts(m);
	block = arduboyPrvDecodeBlock(m, m->pc);
	arduboyPrvNoteHotPc(m, block->pc);
	m->blockRuns++;
#ifdef ARDUBOY_PERF
	m->perfBlocks++;
	for (uint8_t h = 0; h < 4u; h++) {
		if (m->perfHotPc[h] == block->pc) {
			m->perfHotCount[h]++;
			break;
		}
		if (!m->perfHotCount[h]) {
			m->perfHotPc[h] = block->pc;
			m->perfHotCount[h] = 1;
			break;
		}
	}
#endif
	for (uint8_t i = 0; i < block->opCount && !m->unsupported && !m->halted; i++) {
		ArduboyCachedOp *op = &block->ops[i];
		uint16_t beforePc = m->pc;

		if (m->pc != op->pc)
			break;
		if (!arduboyPrvExecuteDecodedOp(m, op, &cycleBatch))
			return false;
		m->opRuns++;
#ifdef ARDUBOY_PERF
		m->perfOps++;
#endif
		if (op->control || m->pc != op->next || beforePc == m->pc)
			break;
	}
	arduboyPrvFlushCycles(m, &cycleBatch);
	arduboyPrvServiceInterrupts(m);
	return !m->unsupported;
}

static bool arduboyPrvLoadHexData(void *userData, uint32_t addr, const uint8_t *data, uint32_t len)
{
	ArduboyMachine *m = (ArduboyMachine*)userData;

	memcpy(m->flash + addr, data, len);
	return true;
}

static void arduboyPrvApplyButtons(ArduboyMachine *m)
{
	uint_fast8_t keys = uiGetKeysRaw();

	m->data[AVR_PINB] = (uint8_t)((m->data[AVR_PINB] & 0xefu) | ((keys & KEY_BIT_B) ? 0 : 0x10u));
	m->data[AVR_PINE] = (uint8_t)((m->data[AVR_PINE] & 0xbfu) | ((keys & KEY_BIT_A) ? 0 : 0x40u));
	m->data[AVR_PINF] = (uint8_t)((m->data[AVR_PINF] & 0x0fu) |
		((keys & KEY_BIT_UP) ? 0 : 0x80u) |
		((keys & KEY_BIT_RIGHT) ? 0 : 0x40u) |
		((keys & KEY_BIT_LEFT) ? 0 : 0x20u) |
		((keys & KEY_BIT_DOWN) ? 0 : 0x10u));
}

static void arduboyPrvMachineReset(ArduboyMachine *m, const uint8_t *saveRam, uint32_t saveRamSize)
{
	memset(m->data, 0, sizeof(m->data));
	memset(m->eeprom, 0xff, sizeof(m->eeprom));
	memset(m->vram, 0, sizeof(m->vram));
	memset(m->hleTargets, 0, sizeof(m->hleTargets));
	memset(m->blocks, 0, sizeof(m->blocks));
	m->pc = 0;
	m->cycles = 0;
	m->timer0Cycles = 0;
	m->nextEepromSyncCycles = ARDUBOY_EEPROM_SYNC_CYCLES;
	m->frameGuardTrips = 0;
	m->spiWrites = 0;
	m->spiIgnoredWrites = 0;
	m->spiSelectedWrites = 0;
	m->hleSpiWrites = 0;
	m->hleSpiIgnoredWrites = 0;
	m->displayDataWrites = 0;
	m->displayCommandWrites = 0;
	m->displayResets = 0;
	m->displayPinTransitions = 0;
	m->cacheResets = 0;
	m->pollingCollapses = 0;
	m->hleQuarantines = 0;
	m->hleBadReturns = 0;
	m->hleSoftMisses = 0;
	m->hleSkippedHeuristics = 0;
	memset(m->hotPc, 0, sizeof(m->hotPc));
	memset(m->hotCount, 0, sizeof(m->hotCount));
	m->unsupportedOpcode = 0;
	m->unsupportedPc = 0;
	m->lastPc = 0;
	m->lastOpcode = 0;
	m->lastHlePc = ARDUBOY_ADDR_UNSET;
	m->lastHleReturnPc = ARDUBOY_ADDR_UNSET;
	m->lastHleEntrySp = ARDUBOY_ADDR_UNSET;
	m->lastHleExitSp = ARDUBOY_ADDR_UNSET;
	m->stackFaultSp = ARDUBOY_ADDR_UNSET;
	m->stackFaultPrevSp = ARDUBOY_ADDR_UNSET;
	m->stackFaultReturnPc = ARDUBOY_ADDR_UNSET;
	m->lastPortD = 0xffu;
	m->lastDisplayPins = ARDUBOY_DISPLAY_PIN_CS | ARDUBOY_DISPLAY_PIN_DC | ARDUBOY_DISPLAY_PIN_RST;
	m->timer0Pending = 0;
	m->halted = 0;
	m->unsupported = 0;
	m->abortRequested = 0;
	m->failureReason = ARDUBOY_FAIL_NONE;
	m->eepromDirty = 0;
	m->displayDirty = 1;
	m->displayExplicitOff = 0;
	m->blockRuns = 0;
	m->opRuns = 0;
	m->decodedBlocks = 0;
	m->cacheHits = 0;
	m->cacheMisses = 0;
	m->visibleFrames = 0;
	m->bootWatchdogTrips = 0;
	m->hleCount = 0;
	m->lastHleKind = ArduboyHleNone;
	m->lastHleConfidence = 0;
	m->lastHleFlags = 0;
	m->stackFaultOp = ARDUBOY_STACK_OP_NONE;
	m->frameDurationMs = 16u;
	m->screenBufferAddr = ARDUBOY_ADDR_UNSET;
	m->frameCountAddr = ARDUBOY_ADDR_UNSET;
	m->eachFrameMillisAddr = ARDUBOY_ADDR_UNSET;
	m->thisFrameStartAddr = ARDUBOY_ADDR_UNSET;
	m->lastFrameDurationMsAddr = ARDUBOY_ADDR_UNSET;
	m->justRenderedAddr = ARDUBOY_ADDR_UNSET;
	m->currentButtonStateAddr = ARDUBOY_ADDR_UNSET;
	m->previousButtonStateAddr = ARDUBOY_ADDR_UNSET;
	m->cursorXAddr = ARDUBOY_ADDR_UNSET;
	m->cursorYAddr = ARDUBOY_ADDR_UNSET;
	m->textColorAddr = ARDUBOY_ADDR_UNSET;
	m->textBackgroundAddr = ARDUBOY_ADDR_UNSET;
	m->audioEnabledAddr = ARDUBOY_ADDR_UNSET;
	m->hleCurrentButtons = 0;
	m->hlePreviousButtons = 0;
	m->hleFrameCount = 0;
	m->hleAudioEnabled = 0;
	m->bootHostTicks = getTime();
	m->lastVisibleHostTicks = m->bootHostTicks;
	m->lastBootDiagHostTicks = m->bootHostTicks;
#ifdef ARDUBOY_PERF
	m->perfLastHostTicks = getTime();
	m->perfLastGuestCycles = 0;
	m->perfFrames = 0;
	m->perfBlocks = 0;
	m->perfOps = 0;
	m->perfDecodedBlocks = 0;
	m->perfPresentTicks = 0;
	m->perfWaitTicks = 0;
	m->perfCacheHits = 0;
	m->perfCacheMisses = 0;
	m->perfHleQuarantines = 0;
	m->perfHleMisses = 0;
	memset(m->perfHleCalls, 0, sizeof(m->perfHleCalls));
	memset(m->perfHotPc, 0, sizeof(m->perfHotPc));
	memset(m->perfHotCount, 0, sizeof(m->perfHotCount));
#endif
	if (saveRam && saveRamSize >= ARDUBOY_SAVE_RAM_SIZE)
		memcpy(m->eeprom, saveRam, ARDUBOY_SAVE_RAM_SIZE);
	arduboyPrvSetSp(m, ARDUBOY_RAM_END);
	m->data[AVR_PINB] = 0xffu;
	m->data[AVR_PIND] = 0xffu;
	m->data[AVR_PINE] = 0xffu;
	m->data[AVR_PINF] = 0xffu;
	m->data[AVR_SPSR] = AVR_SPSR_SPIF;
	m->data[AVR_PORTB] = 0xffu;
	m->data[AVR_PORTD] = 0xffu;
	m->data[AVR_PORTE] = 0xffu;
	m->data[AVR_PORTF] = 0xffu;
	m->data[AVR_PLLCSR] = AVR_PLLCSR_PLOCK;
	m->data[AVR_USBSTA] = AVR_USBSTA_VBUS;
	m->data[AVR_UEINTX] = AVR_UEINTX_TXINI | AVR_UEINTX_RWAL;
	m->data[AVR_UESTA0X] = AVR_UESTA0X_CFGOK;
	arduboyPrvDisplayReset(m);
	arduboyPrvDisplayPinsFromPortD(m, m->data[AVR_PORTD]);
	arduboyPrvApplyButtons(m);
}

static bool arduboyPrvDirectCallTarget(const ArduboyMachine *m, uint16_t pc, uint16_t *targetP)
{
	uint16_t opcode = arduboyPrvFlashWord(m, pc);

	if ((opcode & 0xf000u) == 0xd000u) {
		*targetP = (uint16_t)(pc + 1u + arduboyPrvSign(opcode, 12));
		return true;
	}
	if ((opcode & 0xfe0eu) == 0x940eu) {
		*targetP = arduboyPrvFlashWord(m, (uint16_t)(pc + 1u));
		return true;
	}
	return false;
}

static bool arduboyPrvDirectJumpTarget(const ArduboyMachine *m, uint16_t pc, uint16_t *targetP)
{
	uint16_t opcode = arduboyPrvFlashWord(m, pc);

	if ((opcode & 0xf000u) == 0xc000u) {
		*targetP = (uint16_t)(pc + 1u + arduboyPrvSign(opcode, 12));
		return true;
	}
	if ((opcode & 0xfe0eu) == 0x940cu) {
		*targetP = arduboyPrvFlashWord(m, (uint16_t)(pc + 1u));
		return true;
	}
	return false;
}

static void arduboyPrvAddCallTarget(const ArduboyMachine *m,
	struct ArduboyCallTarget *targets, uint8_t *countP, uint16_t pc)
{
	if (pc >= ARDUBOY_FLASH_SIZE / 2u || arduboyPrvFlashWord(m, pc) == 0xffffu)
		return;
	for (uint8_t i = 0; i < *countP; i++) {
		if (targets[i].pc == pc) {
			if (targets[i].count != 0xffu)
				targets[i].count++;
			return;
		}
	}
	if (*countP >= ARDUBOY_HLE_SCAN_CALL_TARGETS)
		return;
	targets[*countP].pc = pc;
	targets[*countP].count = 1;
	targets[*countP].flags = 0;
	(*countP)++;
}

static bool arduboyPrvIsHleTargetKind(const ArduboyMachine *m, uint16_t pc, uint8_t kind)
{
	for (uint8_t i = 0; i < m->hleCount; i++) {
		if (m->hleTargets[i].pc == pc && m->hleTargets[i].kind == kind) {
#if !ARDUBOY_HLE_ALLOW_HEURISTIC
			if (m->hleTargets[i].confidence < ARDUBOY_HLE_CONF_ANCHORED)
				continue;
#endif
			return true;
		}
	}
	return false;
}

static uint16_t arduboyPrvFirstHleTargetKind(const ArduboyMachine *m, uint8_t kind)
{
	for (uint8_t i = 0; i < m->hleCount; i++) {
		if (m->hleTargets[i].kind == kind) {
#if !ARDUBOY_HLE_ALLOW_HEURISTIC
			if (m->hleTargets[i].confidence < ARDUBOY_HLE_CONF_ANCHORED)
				continue;
#endif
			return m->hleTargets[i].pc;
		}
	}
	return ARDUBOY_ADDR_UNSET;
}

static uint8_t arduboyPrvHleKindForPc(const ArduboyMachine *m, uint16_t pc)
{
	for (uint8_t i = 0; i < m->hleCount; i++) {
		if (m->hleTargets[i].pc == pc) {
#if !ARDUBOY_HLE_ALLOW_HEURISTIC
			if (m->hleTargets[i].confidence < ARDUBOY_HLE_CONF_ANCHORED)
				continue;
#endif
			return m->hleTargets[i].kind;
		}
	}
	return ArduboyHleNone;
}

struct ArduboyHleFunctionScan {
	uint16_t pc;
	uint16_t words;
	uint16_t callTargets[6];
	uint16_t dataPointer;
	uint16_t firstSramLoad;
	uint16_t secondSramLoad;
	uint16_t firstSramStore;
	uint16_t secondSramStore;
	uint8_t callCount;
	uint8_t tailCalls;
	uint8_t retSeen;
	uint8_t backwardBranches;
	uint8_t spiCalls;
	uint8_t paintCalls;
	uint8_t delayCalls;
	uint8_t millisCalls;
	uint8_t knownHleCalls;
	uint8_t drawCalls;
	uint8_t spriteCalls;
	uint8_t buttonCalls;
	uint8_t audioCalls;
	uint8_t lpmOps;
	uint8_t ldPtrOps;
	uint8_t stPtrOps;
	uint8_t sramLoads;
	uint8_t sramStores;
	uint8_t spiIo;
	uint8_t eepromIo;
	uint8_t buttonIo;
	uint8_t timerIo;
	uint8_t sregOps;
	uint8_t pinIo;
	uint8_t portIo;
	uint8_t displayPortIo;
	uint8_t hasConst0;
	uint8_t hasConst1;
	uint8_t hasConst2;
	uint8_t hasConst3;
	uint8_t hasConst4;
	uint8_t hasConst7;
	uint8_t hasConst8;
	uint8_t hasConst15;
	uint8_t hasConst16;
	uint8_t hasConst63;
	uint8_t hasConst64;
	uint8_t hasConst60;
	uint8_t hasConst127;
	uint8_t hasConst128;
	uint8_t hasConst255;
	uint8_t hasConst232;
	uint8_t ldiSeen[32];
	uint8_t ldiValue[32];
};

static void arduboyPrvScanNoteIo(struct ArduboyHleFunctionScan *scan, uint16_t addr)
{
	switch (addr) {
		case AVR_SPDR:
		case AVR_SPSR:
			scan->spiIo++;
			break;
		case AVR_EECR:
		case AVR_EEDR:
		case AVR_EEARL:
		case AVR_EEARH:
			scan->eepromIo++;
			break;
		case AVR_PINB:
		case AVR_PINE:
		case AVR_PINF:
			scan->buttonIo++;
			scan->pinIo++;
			break;
		case AVR_TIFR0:
		case AVR_TIMSK0:
		case AVR_TCCR0A:
		case AVR_TCCR0B:
		case AVR_TCNT0:
			scan->timerIo++;
			break;
		case AVR_SREG:
			scan->sregOps++;
			break;
		case AVR_PORTB:
		case AVR_PORTE:
		case AVR_PORTF:
			scan->portIo++;
			break;
		case AVR_PORTD:
			scan->portIo++;
			scan->displayPortIo++;
			break;
		default:
			break;
	}
}

static void arduboyPrvScanNoteConstant(struct ArduboyHleFunctionScan *scan, uint8_t value)
{
	if (value == 0)
		scan->hasConst0 = 1;
	else if (value == 1)
		scan->hasConst1 = 1;
	else if (value == 2)
		scan->hasConst2 = 1;
	else if (value == 3)
		scan->hasConst3 = 1;
	else if (value == 4)
		scan->hasConst4 = 1;
	else if (value == 7)
		scan->hasConst7 = 1;
	else if (value == 8)
		scan->hasConst8 = 1;
	else if (value == 15)
		scan->hasConst15 = 1;
	else if (value == 16)
		scan->hasConst16 = 1;
	else if (value == 63)
		scan->hasConst63 = 1;
	else if (value == 64)
		scan->hasConst64 = 1;
	else if (value == 60)
		scan->hasConst60 = 1;
	else if (value == 127)
		scan->hasConst127 = 1;
	else if (value == 128)
		scan->hasConst128 = 1;
	else if (value == 255)
		scan->hasConst255 = 1;
	else if (value == 232)
		scan->hasConst232 = 1;
}

static bool arduboyPrvScanDataPointerPair(const struct ArduboyHleFunctionScan *scan,
	uint8_t loReg, uint8_t hiReg, uint16_t *ptrP)
{
	uint16_t ptr;

	if (!scan->ldiSeen[loReg] || !scan->ldiSeen[hiReg])
		return false;
	ptr = (uint16_t)scan->ldiValue[loReg] | ((uint16_t)scan->ldiValue[hiReg] << 8);
	if (ptr < ARDUBOY_RAM_START || ptr >= ARDUBOY_DATA_SIZE ||
		ARDUBOY_DATA_SIZE - ptr < sizeof(((ArduboyMachine*)0)->vram))
		return false;
	*ptrP = ptr;
	return true;
}

static void arduboyPrvScanFunction(const ArduboyMachine *m, uint16_t pc,
	uint16_t spiPc, uint16_t paintPc, uint16_t delayPc, uint16_t millisPc,
	struct ArduboyHleFunctionScan *scan)
{
	memset(scan, 0, sizeof(*scan));
	scan->pc = pc;
	scan->dataPointer = ARDUBOY_ADDR_UNSET;
	scan->firstSramLoad = ARDUBOY_ADDR_UNSET;
	scan->secondSramLoad = ARDUBOY_ADDR_UNSET;
	scan->firstSramStore = ARDUBOY_ADDR_UNSET;
	scan->secondSramStore = ARDUBOY_ADDR_UNSET;

	for (uint16_t pos = pc; scan->words < ARDUBOY_HLE_SCAN_MAX_WORDS; ) {
		ArduboyCachedOp op;
		uint16_t opcode = arduboyPrvFlashWord(m, pos);
		uint16_t target;

		if (opcode == 0xffffu)
			break;
		arduboyPrvDecodeMicroOp((ArduboyMachine*)m, pos, &op);
		scan->words = (uint16_t)(scan->words + op.words);
		if (op.kind == ArduboyOpLdi) {
			scan->ldiSeen[op.d] = 1;
			scan->ldiValue[op.d] = op.k;
			arduboyPrvScanNoteConstant(scan, op.k);
		}
		if (op.kind == ArduboyOpImm)
			arduboyPrvScanNoteConstant(scan, op.k);
		if (op.kind == ArduboyOpAdiw)
			arduboyPrvScanNoteConstant(scan, op.k);
		if (op.kind == ArduboyOpSregBit)
			scan->sregOps++;
		if (op.kind == ArduboyOpIo)
			arduboyPrvScanNoteIo(scan, (uint16_t)(0x20u + op.ioAddr));
		if (op.kind == ArduboyOpIoBit)
			arduboyPrvScanNoteIo(scan, (uint16_t)(0x20u + op.ioAddr));
		if (op.kind == ArduboyOpLds) {
			if (op.imm16 >= ARDUBOY_RAM_START && op.imm16 < ARDUBOY_DATA_SIZE) {
				scan->sramLoads++;
				if (scan->firstSramLoad == ARDUBOY_ADDR_UNSET)
					scan->firstSramLoad = op.imm16;
				else if (scan->secondSramLoad == ARDUBOY_ADDR_UNSET && op.imm16 != scan->firstSramLoad)
					scan->secondSramLoad = op.imm16;
			}
			arduboyPrvScanNoteIo(scan, op.imm16);
		}
		if (op.kind == ArduboyOpSts) {
			if (op.imm16 >= ARDUBOY_RAM_START && op.imm16 < ARDUBOY_DATA_SIZE) {
				scan->sramStores++;
				if (scan->firstSramStore == ARDUBOY_ADDR_UNSET)
					scan->firstSramStore = op.imm16;
				else if (scan->secondSramStore == ARDUBOY_ADDR_UNSET && op.imm16 != scan->firstSramStore)
					scan->secondSramStore = op.imm16;
			}
			arduboyPrvScanNoteIo(scan, op.imm16);
		}
		if (op.kind == ArduboyOpLdPtr)
			scan->ldPtrOps++;
		if (op.kind == ArduboyOpStPtr)
			scan->stPtrOps++;
		if (op.kind == ArduboyOpLpm)
			scan->lpmOps++;
		if (arduboyPrvDirectCallTarget(m, pos, &target)) {
			uint8_t targetKind = arduboyPrvHleKindForPc(m, target);

			if (scan->callCount < sizeof(scan->callTargets) / sizeof(scan->callTargets[0]))
				scan->callTargets[scan->callCount] = target;
			scan->callCount++;
			if (target == spiPc)
				scan->spiCalls++;
			if (target == paintPc)
				scan->paintCalls++;
			if (target == delayPc)
				scan->delayCalls++;
			if (target == millisPc)
				scan->millisCalls++;
			if (targetKind != ArduboyHleNone) {
				scan->knownHleCalls++;
				if (targetKind >= ArduboyHleDrawPixel && targetKind <= ArduboyHleDrawBitmap)
					scan->drawCalls++;
				else if (targetKind == ArduboyHleSpritesDraw)
					scan->spriteCalls++;
				else if (targetKind == ArduboyHleButtonsState || targetKind == ArduboyHlePollButtons ||
					targetKind == ArduboyHlePressed || targetKind == ArduboyHleNotPressed ||
					targetKind == ArduboyHleJustPressed || targetKind == ArduboyHleJustReleased)
					scan->buttonCalls++;
				else if (targetKind >= ArduboyHleAudioEnabled && targetKind <= ArduboyHleTonePlaying)
					scan->audioCalls++;
			}
		}
		else if (arduboyPrvDirectJumpTarget(m, pos, &target)) {
			if (target < pos)
				scan->backwardBranches++;
			else {
				uint8_t targetKind = arduboyPrvHleKindForPc(m, target);

				scan->tailCalls++;
				if (target == spiPc)
					scan->spiCalls++;
				if (target == paintPc)
					scan->paintCalls++;
				if (target == delayPc)
					scan->delayCalls++;
				if (target == millisPc)
					scan->millisCalls++;
				if (targetKind != ArduboyHleNone) {
					scan->knownHleCalls++;
					if (targetKind >= ArduboyHleDrawPixel && targetKind <= ArduboyHleDrawBitmap)
						scan->drawCalls++;
					else if (targetKind == ArduboyHleSpritesDraw)
						scan->spriteCalls++;
					else if (targetKind == ArduboyHleButtonsState || targetKind == ArduboyHlePollButtons ||
						targetKind == ArduboyHlePressed || targetKind == ArduboyHleNotPressed ||
						targetKind == ArduboyHleJustPressed || targetKind == ArduboyHleJustReleased)
						scan->buttonCalls++;
					else if (targetKind >= ArduboyHleAudioEnabled && targetKind <= ArduboyHleTonePlaying)
						scan->audioCalls++;
				}
			}
		}
		if (op.kind == ArduboyOpBranch && op.target < pos)
			scan->backwardBranches++;
		if (op.kind == ArduboyOpRjmp && op.target < pos)
			scan->backwardBranches++;
		if (op.kind == ArduboyOpRet || op.kind == ArduboyOpReti) {
			scan->retSeen = 1;
			break;
		}
		if (op.kind == ArduboyOpJmp)
			break;
		pos = op.next;
	}

	if (!arduboyPrvScanDataPointerPair(scan, 24u, 25u, &scan->dataPointer) &&
		!arduboyPrvScanDataPointerPair(scan, 30u, 31u, &scan->dataPointer)) {
		if (scan->stPtrOps || scan->ldPtrOps)
			(void)arduboyPrvScanDataPointerPair(scan, 26u, 27u, &scan->dataPointer);
	}
}

static void arduboyPrvFindScreenBufferFromSramClearLoops(ArduboyMachine *m,
	const struct ArduboyCallTarget *targets, uint8_t targetCount, uint16_t spiPc,
	uint16_t paintPc, uint16_t delayPc, uint16_t millisPc)
{
	for (uint8_t i = 0; i < targetCount; i++) {
		struct ArduboyHleFunctionScan scan;

		arduboyPrvScanFunction(m, targets[i].pc, spiPc, paintPc, delayPc, millisPc, &scan);
		if (scan.dataPointer == ARDUBOY_ADDR_UNSET || !scan.stPtrOps || !scan.backwardBranches)
			continue;
		if (!(scan.hasConst0 || scan.hasConst255))
			continue;
		if (!(scan.hasConst4 || scan.hasConst232 || scan.hasConst3))
			continue;
		m->screenBufferAddr = scan.dataPointer;
		return;
	}
}

static bool arduboyPrvLooksLikePaintScreenScan(const struct ArduboyHleFunctionScan *scan)
{
	if (!scan->backwardBranches)
		return false;
	if (!scan->spiCalls && scan->spiIo < 2u)
		return false;
	if (!scan->ldPtrOps && !scan->lpmOps && !scan->sramLoads)
		return false;
	return scan->hasConst4;
}

static bool arduboyPrvLooksLikeDelayScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->backwardBranches && scan->callCount &&
		scan->hasConst232 && scan->hasConst3 && scan->words > 12u;
}

static bool arduboyPrvLooksLikeDelayShortScan(const struct ArduboyHleFunctionScan *scan)
{
	return !scan->backwardBranches && scan->delayCalls && scan->words <= 18u;
}

static bool arduboyPrvLooksLikeMillisScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && !scan->callCount && scan->sramLoads >= 4u &&
		scan->sregOps && scan->words >= 10u && scan->words <= 56u;
}

static bool arduboyPrvLooksLikeNextFrameScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->millisCalls && scan->sramLoads && scan->sramStores &&
		(scan->hasConst16 || scan->hasConst60 || scan->hasConst1);
}

static bool arduboyPrvLooksLikeDisplayScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->paintCalls && scan->dataPointer != ARDUBOY_ADDR_UNSET;
}

static bool arduboyPrvLooksLikeButtonsScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->buttonIo >= 2u || (scan->pinIo && scan->hasConst128);
}

static bool arduboyPrvLooksLikeEepromScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->eepromIo >= 2u && scan->words <= 80u;
}

static bool arduboyPrvLooksLikeRgbOrAudioNoopScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && scan->words <= 36u && scan->portIo && !scan->displayPortIo &&
		(scan->hasConst0 || scan->hasConst1 || scan->hasConst128);
}

static bool arduboyPrvLooksLikeSetFrameRateScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && scan->sramStores &&
		((scan->hasConst232 && scan->hasConst3) || scan->hasConst60 || scan->hasConst16) &&
		!scan->lpmOps && !scan->spiIo && !scan->displayPortIo && scan->words <= 72u;
}

static bool arduboyPrvLooksLikeSetFrameDurationScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && scan->sramStores && !scan->backwardBranches && !scan->lpmOps &&
		!scan->spiIo && !scan->eepromIo && scan->words <= 24u;
}

static bool arduboyPrvLooksLikeEveryXFramesScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && scan->sramLoads && !scan->sramStores && !scan->lpmOps &&
		!scan->spiIo && !scan->eepromIo && scan->words <= 56u && scan->callCount;
}

static bool arduboyPrvLooksLikePollButtonsScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && (scan->buttonCalls || scan->buttonIo) && scan->sramStores &&
		!scan->lpmOps && scan->words <= 64u;
}

static bool arduboyPrvLooksLikePressedScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && (scan->buttonCalls || scan->buttonIo) && !scan->lpmOps &&
		!scan->spiIo && scan->words <= 56u;
}

static bool arduboyPrvLooksLikeClearScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && (scan->stPtrOps || scan->sramStores || scan->knownHleCalls) &&
		!scan->lpmOps && !scan->spiIo && !scan->eepromIo && scan->words <= 72u &&
		(scan->hasConst0 || scan->hasConst255 || scan->hasConst128 || scan->hasConst4);
}

static bool arduboyPrvLooksLikeDrawPixelScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && (scan->sramStores || scan->stPtrOps) && !scan->lpmOps &&
		!scan->spiIo && !scan->eepromIo && scan->words <= 96u &&
		(scan->hasConst127 || scan->hasConst128) && (scan->hasConst63 || scan->hasConst64) &&
		(scan->hasConst7 || scan->hasConst8 || scan->hasConst2);
}

static bool arduboyPrvLooksLikeFastLineScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && (scan->drawCalls || scan->sramStores || scan->stPtrOps) &&
		!scan->lpmOps && !scan->spiIo && !scan->eepromIo && scan->words <= 120u &&
		(scan->hasConst127 || scan->hasConst128 || scan->hasConst63 || scan->hasConst64);
}

static bool arduboyPrvLooksLikeDrawLineScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && (scan->drawCalls || scan->sramStores || scan->stPtrOps) &&
		!scan->lpmOps && !scan->spiIo && !scan->eepromIo && scan->backwardBranches &&
		scan->words <= 160u;
}

static bool arduboyPrvLooksLikeDrawBitmapScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && scan->lpmOps && scan->backwardBranches && !scan->spiIo &&
		!scan->eepromIo && (scan->hasConst7 || scan->hasConst8) &&
		(scan->hasConst127 || scan->hasConst128 || scan->hasConst63 || scan->hasConst64);
}

static bool arduboyPrvLooksLikeSpritesScan(const struct ArduboyHleFunctionScan *scan)
{
	return arduboyPrvLooksLikeDrawBitmapScan(scan) &&
		(scan->hasConst2 || scan->hasConst4 || scan->hasConst8) && scan->words >= 32u;
}

static bool arduboyPrvLooksLikeAudioScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && !scan->displayPortIo && !scan->spiIo &&
		(scan->timerIo || scan->portIo || scan->audioCalls) &&
		scan->words <= 128u;
}

#if ARDUBOY_HLE_ALLOW_HEURISTIC
static bool arduboyPrvLooksLikeMemsetScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && scan->stPtrOps && scan->backwardBranches && !scan->lpmOps &&
		!scan->spiIo && !scan->eepromIo && !scan->portIo && scan->words <= 48u;
}

static bool arduboyPrvLooksLikeMemcpyScan(const struct ArduboyHleFunctionScan *scan)
{
	return scan->retSeen && scan->ldPtrOps && scan->stPtrOps && scan->backwardBranches &&
		!scan->lpmOps && !scan->spiIo && !scan->eepromIo && !scan->portIo && scan->words <= 72u;
}
#endif

static void arduboyPrvFindCallTargets(ArduboyMachine *m, struct ArduboyCallTarget *targets, uint8_t *countP)
{
	*countP = 0;
	for (uint16_t pc = 0; pc < (ARDUBOY_FLASH_SIZE / 2u) - 2u; pc++) {
		uint16_t target;

		if (arduboyPrvDirectCallTarget(m, pc, &target))
			arduboyPrvAddCallTarget(m, targets, countP, target);
	}
}

static void arduboyPrvFindScreenBufferFromPaintCalls(ArduboyMachine *m, uint16_t paintPc)
{
	for (uint16_t pc = 0; pc < (ARDUBOY_FLASH_SIZE / 2u) - 2u; pc++) {
		uint16_t target;
		uint8_t loSeen = 0, hiSeen = 0, lo = 0, hi = 0;

		if (!arduboyPrvDirectCallTarget(m, pc, &target) || target != paintPc)
			continue;
		for (uint16_t back = pc > 10u ? (uint16_t)(pc - 10u) : 0; back < pc; back++) {
			uint16_t opcode = arduboyPrvFlashWord(m, back);

			if ((opcode & 0xf000u) != 0xe000u)
				continue;
			if (arduboyPrvRegD(opcode) == 24u) {
				loSeen = 1;
				lo = arduboyPrvImm8(opcode);
			}
			else if (arduboyPrvRegD(opcode) == 25u) {
				hiSeen = 1;
				hi = arduboyPrvImm8(opcode);
			}
		}
		if (loSeen && hiSeen) {
			uint16_t ptr = (uint16_t)lo | ((uint16_t)hi << 8);

			if (ptr >= ARDUBOY_RAM_START && ptr < ARDUBOY_DATA_SIZE &&
				ARDUBOY_DATA_SIZE - ptr >= sizeof(m->vram)) {
				m->screenBufferAddr = ptr;
				return;
			}
		}
	}
}

static void arduboyPrvScanHleTargets(ArduboyMachine *m)
{
	struct ArduboyCallTarget targets[ARDUBOY_HLE_SCAN_CALL_TARGETS];
	uint8_t targetCount;
	uint16_t spiPc = ARDUBOY_ADDR_UNSET;
	uint16_t paintPc = ARDUBOY_ADDR_UNSET;
	uint16_t delayPc = ARDUBOY_ADDR_UNSET;
	uint16_t millisPc = ARDUBOY_ADDR_UNSET;
	uint8_t drawTargets = 0;
	uint8_t spriteTargets = 0;
	uint8_t inputTargets = 0;
	uint8_t audioTargets = 0;
	uint8_t libcTargets = 0;

	m->hleCount = 0;
	arduboyPrvFindCallTargets(m, targets, &targetCount);
	for (uint8_t i = 0; i < targetCount; i++) {
		if (arduboyPrvLooksLikeSpiTransfer(m, targets[i].pc)) {
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleSpiTransfer, ARDUBOY_HLE_CONF_EXACT);
			if (spiPc == ARDUBOY_ADDR_UNSET)
				spiPc = targets[i].pc;
		}
	}
	for (uint8_t i = 0; i < targetCount; i++) {
		struct ArduboyHleFunctionScan scan;

		arduboyPrvScanFunction(m, targets[i].pc, spiPc, paintPc, delayPc, millisPc, &scan);
		if (arduboyPrvLooksLikePaintScreenScan(&scan)) {
			uint8_t kind = scan.lpmOps ? ArduboyHlePaintScreenProgmem : ArduboyHlePaintScreen;

			arduboyPrvAddHleTargetEx(m, targets[i].pc, kind, ARDUBOY_HLE_CONF_ANCHORED);
			if (paintPc == ARDUBOY_ADDR_UNSET)
				paintPc = targets[i].pc;
		}
		if (arduboyPrvLooksLikeDelayScan(&scan)) {
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleDelay, ARDUBOY_HLE_CONF_ANCHORED);
			if (delayPc == ARDUBOY_ADDR_UNSET)
				delayPc = targets[i].pc;
		}
		if (arduboyPrvLooksLikeMillisScan(&scan)) {
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleMillis, ARDUBOY_HLE_CONF_ANCHORED);
			if (millisPc == ARDUBOY_ADDR_UNSET)
				millisPc = targets[i].pc;
		}
	}
	if (paintPc == ARDUBOY_ADDR_UNSET)
		paintPc = arduboyPrvFirstHleTargetKind(m, ArduboyHlePaintScreen);
	if (delayPc == ARDUBOY_ADDR_UNSET)
		delayPc = arduboyPrvFirstHleTargetKind(m, ArduboyHleDelay);
	if (millisPc == ARDUBOY_ADDR_UNSET)
		millisPc = arduboyPrvFirstHleTargetKind(m, ArduboyHleMillis);
	if (paintPc != ARDUBOY_ADDR_UNSET)
		arduboyPrvFindScreenBufferFromPaintCalls(m, paintPc);
	if (m->screenBufferAddr == ARDUBOY_ADDR_UNSET)
		arduboyPrvFindScreenBufferFromSramClearLoops(m, targets, targetCount,
			spiPc, paintPc, delayPc, millisPc);

	for (uint8_t i = 0; i < targetCount; i++) {
		struct ArduboyHleFunctionScan scan;

		if (arduboyPrvIsHleTargetKind(m, targets[i].pc, ArduboyHleSpiTransfer) ||
			arduboyPrvIsHleTargetKind(m, targets[i].pc, ArduboyHlePaintScreen) ||
			arduboyPrvIsHleTargetKind(m, targets[i].pc, ArduboyHlePaintScreenProgmem) ||
			arduboyPrvIsHleTargetKind(m, targets[i].pc, ArduboyHleDelay) ||
			arduboyPrvIsHleTargetKind(m, targets[i].pc, ArduboyHleMillis))
			continue;
		arduboyPrvScanFunction(m, targets[i].pc, spiPc, paintPc, delayPc, millisPc, &scan);
		if (arduboyPrvLooksLikeDelayShortScan(&scan))
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleDelayShort, ARDUBOY_HLE_CONF_ANCHORED);
		else if (arduboyPrvLooksLikeDisplayScan(&scan)) {
			if (m->screenBufferAddr == ARDUBOY_ADDR_UNSET)
				m->screenBufferAddr = scan.dataPointer;
			arduboyPrvAddHleTargetEx(m, targets[i].pc,
				scan.hasConst1 ? ArduboyHleDisplayClear : ArduboyHleDisplay, ARDUBOY_HLE_CONF_ANCHORED);
		}
		else if (arduboyPrvLooksLikeNextFrameScan(&scan)) {
			if (m->thisFrameStartAddr == ARDUBOY_ADDR_UNSET)
				m->thisFrameStartAddr = scan.firstSramStore;
			if (m->eachFrameMillisAddr == ARDUBOY_ADDR_UNSET && scan.firstSramLoad != ARDUBOY_ADDR_UNSET)
				m->eachFrameMillisAddr = scan.firstSramLoad;
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleNextFrame, ARDUBOY_HLE_CONF_ANCHORED);
		}
		else if (arduboyPrvLooksLikeSetFrameRateScan(&scan)) {
			if (m->eachFrameMillisAddr == ARDUBOY_ADDR_UNSET)
				m->eachFrameMillisAddr = scan.firstSramStore;
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleSetFrameRate, ARDUBOY_HLE_CONF_ANCHORED);
		}
		else if (arduboyPrvLooksLikeSetFrameDurationScan(&scan) &&
			m->eachFrameMillisAddr != ARDUBOY_ADDR_UNSET && scan.firstSramStore == m->eachFrameMillisAddr) {
			if (m->eachFrameMillisAddr == ARDUBOY_ADDR_UNSET)
				m->eachFrameMillisAddr = scan.firstSramStore;
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleSetFrameDuration, ARDUBOY_HLE_CONF_ANCHORED);
		}
		else if (arduboyPrvLooksLikeEveryXFramesScan(&scan))
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleEveryXFrames, ARDUBOY_HLE_CONF_ANCHORED);
		else if (arduboyPrvLooksLikePollButtonsScan(&scan)) {
			if (m->previousButtonStateAddr == ARDUBOY_ADDR_UNSET)
				m->previousButtonStateAddr = scan.firstSramStore;
			if (m->currentButtonStateAddr == ARDUBOY_ADDR_UNSET)
				m->currentButtonStateAddr = scan.secondSramStore != ARDUBOY_ADDR_UNSET ? scan.secondSramStore : scan.firstSramStore;
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHlePollButtons, ARDUBOY_HLE_CONF_ANCHORED);
			inputTargets++;
		}
		else if (scan.sramLoads >= 2u && !scan.sramStores && scan.words <= 56u &&
			(scan.buttonCalls || (scan.firstSramLoad != ARDUBOY_ADDR_UNSET && scan.secondSramLoad != ARDUBOY_ADDR_UNSET))) {
			if (m->currentButtonStateAddr == ARDUBOY_ADDR_UNSET)
				m->currentButtonStateAddr = scan.firstSramLoad;
			if (m->previousButtonStateAddr == ARDUBOY_ADDR_UNSET)
				m->previousButtonStateAddr = scan.secondSramLoad;
			arduboyPrvAddHleTargetEx(m, targets[i].pc, scan.hasConst0 ? ArduboyHleJustReleased : ArduboyHleJustPressed,
				ARDUBOY_HLE_CONF_ANCHORED);
			inputTargets++;
		}
		else if (arduboyPrvLooksLikePressedScan(&scan)) {
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHlePressed, ARDUBOY_HLE_CONF_ANCHORED);
			inputTargets++;
		}
		else if (arduboyPrvLooksLikeButtonsScan(&scan)) {
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleButtonsState, ARDUBOY_HLE_CONF_EXACT);
			inputTargets++;
		}
		else if (arduboyPrvLooksLikeEepromScan(&scan)) {
			if (scan.sramLoads && !scan.sramStores)
				arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleEepromRead, ARDUBOY_HLE_CONF_ANCHORED);
			else if (scan.callCount)
				arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleEepromUpdate, ARDUBOY_HLE_CONF_ANCHORED);
			else
				arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleEepromWrite, ARDUBOY_HLE_CONF_ANCHORED);
		}
#if ARDUBOY_HLE_ALLOW_HEURISTIC
		else if (arduboyPrvLooksLikeMemsetScan(&scan)) {
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleMemset, ARDUBOY_HLE_CONF_HEURISTIC);
			libcTargets++;
		}
		else if (arduboyPrvLooksLikeMemcpyScan(&scan)) {
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleMemcpy, ARDUBOY_HLE_CONF_HEURISTIC);
			libcTargets++;
		}
#endif
		else if (arduboyPrvLooksLikeClearScan(&scan)) {
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleClear, ARDUBOY_HLE_CONF_ANCHORED);
			drawTargets++;
		}
		else if (arduboyPrvLooksLikeDrawPixelScan(&scan)) {
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleDrawPixel, ARDUBOY_HLE_CONF_ANCHORED);
			drawTargets++;
		}
		else if (arduboyPrvLooksLikeSpritesScan(&scan)) {
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleSpritesDraw, ARDUBOY_HLE_CONF_ANCHORED);
			spriteTargets++;
		}
		else if (arduboyPrvLooksLikeDrawBitmapScan(&scan)) {
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleDrawBitmap, ARDUBOY_HLE_CONF_ANCHORED);
			drawTargets++;
		}
		else if (arduboyPrvLooksLikeDrawLineScan(&scan)) {
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleDrawLine, ARDUBOY_HLE_CONF_ANCHORED);
			drawTargets++;
		}
		else if (arduboyPrvLooksLikeFastLineScan(&scan)) {
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleDrawFastHLine, ARDUBOY_HLE_CONF_ANCHORED);
			drawTargets++;
		}
#if ARDUBOY_HLE_ALLOW_HEURISTIC
		else if (arduboyPrvLooksLikeAudioScan(&scan)) {
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleToneNoop, ARDUBOY_HLE_CONF_HEURISTIC);
			audioTargets++;
		}
		else if (arduboyPrvLooksLikeRgbOrAudioNoopScan(&scan))
			arduboyPrvAddHleTargetEx(m, targets[i].pc, ArduboyHleRgbNoop, ARDUBOY_HLE_CONF_HEURISTIC);
#endif
	}
	for (uint8_t pass = 0; pass < 2u; pass++) {
		for (uint8_t i = 0; i < targetCount; i++) {
			struct ArduboyHleFunctionScan scan;
			uint8_t kind = arduboyPrvHleKindForPc(m, targets[i].pc);
			uint8_t targetKind = ArduboyHleNone;

			if (kind != ArduboyHleNone)
				continue;
			arduboyPrvScanFunction(m, targets[i].pc, spiPc, paintPc, delayPc, millisPc, &scan);
			if (!scan.knownHleCalls || scan.words > 32u || scan.callCount + scan.tailCalls > 2u)
				continue;
			for (uint8_t c = 0; c < scan.callCount && c < sizeof(scan.callTargets) / sizeof(scan.callTargets[0]); c++) {
				targetKind = arduboyPrvHleKindForPc(m, scan.callTargets[c]);
				if (targetKind != ArduboyHleNone)
					break;
			}
			if (targetKind == ArduboyHleNone)
				continue;
			if (targetKind == ArduboyHleDisplay || targetKind == ArduboyHleDisplayClear ||
				targetKind == ArduboyHlePaintScreen || targetKind == ArduboyHlePaintScreenProgmem ||
				targetKind == ArduboyHleSpiTransfer || targetKind == ArduboyHleSpiTransferRead)
				continue;
			arduboyPrvAddHleTargetEx(m, targets[i].pc, targetKind, ARDUBOY_HLE_CONF_ANCHORED);
			if (targetKind >= ArduboyHleDrawPixel && targetKind <= ArduboyHleDrawBitmap)
				drawTargets++;
			else if (targetKind == ArduboyHleSpritesDraw)
				spriteTargets++;
			else if (targetKind >= ArduboyHleAudioEnabled && targetKind <= ArduboyHleTonePlaying)
				audioTargets++;
			else if (targetKind >= ArduboyHleMemset && targetKind <= ArduboyHleMemmove)
				libcTargets++;
		}
	}
	pr("Arduboy HLE: targets=%u spi=%04x paint=%04x delay=%04x millis=%04x sbuf=%04x draw=%u spr=%u input=%u audio=%u libc=%u\n",
		(unsigned)m->hleCount, (unsigned)spiPc, (unsigned)paintPc,
		(unsigned)delayPc, (unsigned)millisPc, (unsigned)m->screenBufferAddr,
		(unsigned)drawTargets, (unsigned)spriteTargets, (unsigned)inputTargets,
		(unsigned)audioTargets, (unsigned)libcTargets);
}

static void arduboyPrvSyncEepromToSave(bool force)
{
	if (!mMachine || !mSaveRam || mSaveRamSize < ARDUBOY_SAVE_RAM_SIZE)
		return;
	if (!force && (!mMachine->eepromDirty || mMachine->cycles < mMachine->nextEepromSyncCycles))
		return;
	memcpy(mSaveRam, mMachine->eeprom, ARDUBOY_SAVE_RAM_SIZE);
	mMachine->eepromDirty = 0;
	mMachine->nextEepromSyncCycles = mMachine->cycles + ARDUBOY_EEPROM_SYNC_CYCLES;
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
		mPresentSrcX[y] = srcX;
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
		mPresentSrcYPage[x] = page;
		mPresentSrcYMask[x] = mask;
		mPresentSrcYPageFlipped[x] = pageFlipped;
		mPresentSrcYMaskFlipped[x] = maskFlipped;
	}
	mPresentLastState = ARDUBOY_PRESENT_STATE_INVALID;
	memset(dispGetFb(), 0, DISP_WIDTH * DISP_HEIGHT * DISP_BPP / 8);
	if (mMachine)
		mMachine->displayDirty = 1;
}

static void arduboyPrvPresentFrame(void)
{
	if (!mMachine)
		return;

	uint16_t *fb = (uint16_t*)dispGetFb();
	const bool displayOn = (mMachine->displayFlags & ARDUBOY_PRESENT_STATE_DISPLAY_ON) != 0;
	const bool inverted = (mMachine->displayFlags & ARDUBOY_PRESENT_STATE_INVERTED) != 0;
	const bool segmentRemap = (mMachine->displayFlags & 0x04u) != 0;
	const bool comScanNormal = (mMachine->displayFlags & 0x08u) != 0;
	const uint8_t *srcYPage = comScanNormal ? mPresentSrcYPageFlipped : mPresentSrcYPage;
	const uint8_t *srcYMask = comScanNormal ? mPresentSrcYMaskFlipped : mPresentSrcYMask;
	uint16_t onColor = 0xffffu, offColor = 0x0000u;
	uint8_t presentState = (displayOn ? ARDUBOY_PRESENT_STATE_DISPLAY_ON : 0u) |
		(inverted ? ARDUBOY_PRESENT_STATE_INVERTED : 0u);
	bool clearFrame = presentState != mPresentLastState;

	if (inverted) {
		onColor = 0x0000u;
		offColor = 0xffffu;
	}

	if (!clearFrame && !mMachine->displayDirty)
		return;
	if (clearFrame || displayOn)
		dispPrvWaitForScanoutStart();
	if (clearFrame) {
		arduboyPrvFill(fb, DISP_WIDTH * DISP_HEIGHT, offColor);
		mPresentLastState = presentState;
	}
	if (!displayOn) {
		mMachine->displayDirty = 0;
		return;
	}

	for (uint32_t y = mPresentRowFirst; y < mPresentRowEnd; y++) {
		uint16_t srcX = mPresentSrcX[y];

		if (srcX == ARDUBOY_PRESENT_INVALID)
			continue;
		if (segmentRemap)
			srcX = ARDUBOY_DISPLAY_WIDTH - 1u - srcX;

		for (uint32_t x = mPresentColFirst; x < mPresentColEnd; x++) {
			uint8_t page = srcYPage[x];

			uint32_t dst = y * DISP_WIDTH + x;

			if (mPresentFlipped)
				dst = DISP_WIDTH * DISP_HEIGHT - 1u - dst;
			if (page != ARDUBOY_PRESENT_Y_INVALID && (mMachine->vram[page][srcX] & srcYMask[x]))
				fb[dst] = onColor;
			else
				fb[dst] = offColor;
		}
	}
	mMachine->displayDirty = 0;
	if (arduboyPrvDisplayHasVisibleContent(mMachine)) {
		mMachine->visibleFrames++;
		mMachine->lastVisibleHostTicks = getTime();
	}
}

void arduboyRefreshDisplayOptions(void)
{
	dispSetFramerate(ARDUBOY_FRAME_RATE);
	arduboyPrvUpdatePresentMap();
}

static bool arduboyPrvVramHasNonzero(const ArduboyMachine *m)
{
	for (uint32_t page = 0; page < ARDUBOY_DISPLAY_HEIGHT / 8u; page++)
		for (uint32_t col = 0; col < ARDUBOY_DISPLAY_WIDTH; col++)
			if (m->vram[page][col])
				return true;
	return false;
}

static bool arduboyPrvDisplayHasVisibleContent(const ArduboyMachine *m)
{
	if (!m || !(m->displayFlags & ARDUBOY_PRESENT_STATE_DISPLAY_ON))
		return false;
	if (m->displayFlags & ARDUBOY_PRESENT_STATE_INVERTED)
		return true;
	return arduboyPrvVramHasNonzero(m);
}

static bool arduboyPrvShouldReportBlackScreen(const ArduboyMachine *m)
{
	bool displayOn;

	if (m->cycles < ARDUBOY_BLACK_SCREEN_CYCLES)
		return false;
	if (!m->spiWrites && !m->displayCommandWrites && !m->displayDataWrites)
		return false;
	if (m->spiWrites && !m->displayCommandWrites && !m->displayDataWrites)
		return true;
	displayOn = (m->displayFlags & ARDUBOY_PRESENT_STATE_DISPLAY_ON) != 0;
	if (m->displayCommandWrites && !m->displayDataWrites)
		return true;
	if (m->displayDataWrites && !displayOn)
		return true;
	if (m->displayDataWrites &&
		!(m->displayFlags & ARDUBOY_PRESENT_STATE_INVERTED) &&
		!arduboyPrvVramHasNonzero(m))
		return true;
	return false;
}

static uint32_t arduboyPrvRatePerSecond(uint64_t count, uint64_t ticks)
{
	if (!ticks)
		return 0;
	return (uint32_t)((count * (uint64_t)TICKS_PER_SECOND) / ticks);
}

static uint32_t arduboyPrvTicksToMs(uint64_t ticks)
{
	return (uint32_t)((ticks * 1000ull) / TICKS_PER_SECOND);
}

static uint8_t arduboyPrvBootWatchdogFailureReason(const ArduboyMachine *m)
{
	if (!m->spiWrites && !m->displayCommandWrites && !m->displayDataWrites)
		return ARDUBOY_FAIL_NO_DISPLAY_IO;
	if (m->spiWrites && !m->displayCommandWrites && !m->displayDataWrites)
		return ARDUBOY_FAIL_SPI_NOT_CAPTURED;
	if (m->displayCommandWrites && !m->displayDataWrites)
		return ARDUBOY_FAIL_BLACK_SCREEN;
	if (m->displayDataWrites && !arduboyPrvDisplayHasVisibleContent(m))
		return ARDUBOY_FAIL_BLACK_SCREEN;
	return ARDUBOY_FAIL_BOOT_WATCHDOG;
}

static bool arduboyPrvShouldReportBootWatchdog(ArduboyMachine *m, uint64_t now)
{
	if (!m || m->visibleFrames)
		return false;
	if (!m->bootHostTicks || now - m->bootHostTicks < ARDUBOY_BOOT_WATCHDOG_TICKS)
		return false;
	m->bootWatchdogTrips++;
	return true;
}

static void arduboyPrvLogBootProgress(ArduboyMachine *m, uint64_t now)
{
	uint64_t elapsedTicks;

	if (!m || m->visibleFrames || !m->bootHostTicks ||
		now - m->lastBootDiagHostTicks < ARDUBOY_BOOT_PROGRESS_LOG_TICKS)
		return;
	elapsedTicks = now - m->bootHostTicks;
	prRaw("Arduboy boot: wall=%lums guest=%lu cyc/s blocks=%lu/s ops=%lu/s pc=%04x op=%04x spi=%lu sel=%lu ign=%lu hleSpi=%lu cmd=%lu data=%lu pins=%02x portD=%02x ddrD=%02x lcd=%02x/%02x fl=%02x off=%u hle=%u soft=%lu skip=%lu q=%lu cache=%lu/%lu hot=%04x/%lu,%04x/%lu\n",
		(unsigned long)arduboyPrvTicksToMs(elapsedTicks),
		(unsigned long)arduboyPrvRatePerSecond(m->cycles, elapsedTicks),
		(unsigned long)arduboyPrvRatePerSecond(m->blockRuns, elapsedTicks),
		(unsigned long)arduboyPrvRatePerSecond(m->opRuns, elapsedTicks),
		(unsigned)m->pc, (unsigned)arduboyPrvFlashWord(m, m->pc),
		(unsigned long)m->spiWrites, (unsigned long)m->spiSelectedWrites,
		(unsigned long)m->spiIgnoredWrites, (unsigned long)m->hleSpiWrites,
		(unsigned long)m->displayCommandWrites, (unsigned long)m->displayDataWrites,
		(unsigned)m->lastDisplayPins, (unsigned)m->data[AVR_PORTD],
		(unsigned)m->data[AVR_DDRD], (unsigned)m->lastDisplayCommand,
		(unsigned)m->lastDisplayData, (unsigned)m->displayFlags,
		(unsigned)m->displayExplicitOff, (unsigned)m->hleCount,
		(unsigned long)m->hleSoftMisses, (unsigned long)m->hleSkippedHeuristics,
		(unsigned long)m->hleQuarantines, (unsigned long)m->cacheHits,
		(unsigned long)m->cacheMisses, (unsigned)m->hotPc[0],
		(unsigned long)m->hotCount[0], (unsigned)m->hotPc[1],
		(unsigned long)m->hotCount[1]);
	m->lastBootDiagHostTicks = now;
}

static bool arduboyPrvServiceRuntimeControls(ArduboyMachine *m)
{
	uint64_t now;

	if (!m || mAbortRequested || m->unsupported || m->halted)
		return false;
	if (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER) {
		arduboyPrvSyncEepromToSave(true);
		arduboyPortInGameMenu();
		while (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER) {
		}
		if (mAbortRequested)
			return false;
	}
	now = getTime();
	arduboyPrvLogBootProgress(m, now);
	if (arduboyPrvShouldReportBootWatchdog(m, now)) {
		arduboyPrvStopWithRuntimeFailure(m, arduboyPrvBootWatchdogFailureReason(m));
		return false;
	}
	return true;
}

static void arduboyPrvStopWithRuntimeFailure(ArduboyMachine *m, uint8_t reason)
{
	m->failureReason = reason;
	m->halted = 1;
	m->unsupportedPc = m->pc;
	m->unsupportedOpcode = arduboyPrvFlashWord(m, m->pc);
}

void arduboyAbort(void)
{
	mAbortRequested = true;
	if (mMachine)
		mMachine->abortRequested = 1;
}

static void arduboyPrvDebugPutPixel(uint16_t *fb, uint32_t x, uint32_t y, uint16_t color)
{
	if (!fb || x >= ARDUBOY_DEBUG_LOGICAL_WIDTH || y >= ARDUBOY_DEBUG_LOGICAL_HEIGHT)
		return;
	fb[x * DISP_WIDTH + (DISP_WIDTH - 1u - y)] = color;
}

static void arduboyPrvDebugFillRect(uint16_t *fb, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color)
{
	uint32_t xEnd = x + w;
	uint32_t yEnd = y + h;

	if (xEnd > ARDUBOY_DEBUG_LOGICAL_WIDTH)
		xEnd = ARDUBOY_DEBUG_LOGICAL_WIDTH;
	if (yEnd > ARDUBOY_DEBUG_LOGICAL_HEIGHT)
		yEnd = ARDUBOY_DEBUG_LOGICAL_HEIGHT;
	for (uint32_t yy = y; yy < yEnd; yy++)
		for (uint32_t xx = x; xx < xEnd; xx++)
			arduboyPrvDebugPutPixel(fb, xx, yy, color);
}

static void arduboyPrvDebugBorder(uint16_t *fb, uint16_t color)
{
	arduboyPrvDebugFillRect(fb, 0, 0, ARDUBOY_DEBUG_LOGICAL_WIDTH, 2, color);
	arduboyPrvDebugFillRect(fb, 0, ARDUBOY_DEBUG_LOGICAL_HEIGHT - 2u, ARDUBOY_DEBUG_LOGICAL_WIDTH, 2, color);
	arduboyPrvDebugFillRect(fb, 0, 0, 2, ARDUBOY_DEBUG_LOGICAL_HEIGHT, color);
	arduboyPrvDebugFillRect(fb, ARDUBOY_DEBUG_LOGICAL_WIDTH - 2u, 0, 2, ARDUBOY_DEBUG_LOGICAL_HEIGHT, color);
}

static uint32_t arduboyPrvDebugDrawChar(uint16_t *fb, uint32_t x, uint32_t y, char ch, uint16_t color)
{
	struct FontGlyphInfo gi;

	if (!fontGetGlyphInfo(&gi, FontSmall, (unsigned char)ch))
		return 4;
	for (uint32_t yy = 0; yy < gi.height; yy++) {
		for (uint32_t xx = 0; xx < gi.width; xx++) {
			if (fontGetGlyphPixel(&gi, (uint_fast8_t)yy, (uint_fast8_t)xx))
				arduboyPrvDebugPutPixel(fb, x + xx, y + yy, color);
		}
	}
	return gi.width + 1u;
}

static void arduboyPrvDebugDrawText(uint16_t *fb, uint32_t x, uint32_t y, const char *text, uint16_t color)
{
	while (*text)
		x += arduboyPrvDebugDrawChar(fb, x, y, *text++, color);
}

static const char *arduboyPrvFailureName(uint8_t reason)
{
	switch (reason) {
		case ARDUBOY_FAIL_UNSUPPORTED_OPCODE:
			return "UNSUPPORTED OPCODE";
		case ARDUBOY_FAIL_FRAME_GUARD:
			return "FRAME GUARD";
		case ARDUBOY_FAIL_NO_DISPLAY_IO:
			return "NO DISPLAY IO";
		case ARDUBOY_FAIL_INVALID_PC:
			return "INVALID PC";
		case ARDUBOY_FAIL_HLE_BAD_RETURN:
			return "HLE BAD RETURN";
		case ARDUBOY_FAIL_STACK_FAULT:
			return "STACK FAULT";
		case ARDUBOY_FAIL_HLE_QUARANTINED:
			return "HLE QUARANTINED";
		case ARDUBOY_FAIL_BLACK_SCREEN:
			return "BLACK SCREEN";
		case ARDUBOY_FAIL_BOOT_WATCHDOG:
			return "BOOT WATCHDOG";
		case ARDUBOY_FAIL_SPI_NOT_CAPTURED:
			return "SPI NOT CAPTURED";
		default:
			return "UNKNOWN";
	}
}

static const char *arduboyPrvStackOpName(uint8_t op)
{
	switch (op) {
		case ARDUBOY_STACK_OP_PUSH:
			return "PUSH";
		case ARDUBOY_STACK_OP_POP:
			return "POP";
		case ARDUBOY_STACK_OP_CALL:
			return "CALL";
		case ARDUBOY_STACK_OP_RET:
			return "RET";
		case ARDUBOY_STACK_OP_HLE_RET:
			return "HLE RET";
		default:
			return "NONE";
	}
}

static void arduboyPrvBuildPresenterTestPattern(ArduboyMachine *m)
{
	memset(m->vram, 0, sizeof(m->vram));
	for (uint32_t x = 0; x < ARDUBOY_DISPLAY_WIDTH; x++) {
		m->vram[0][x] |= 0x01u;
		m->vram[ARDUBOY_DISPLAY_HEIGHT / 8u - 1u][x] |= 0x80u;
		if ((x & 7u) < 4u)
			m->vram[1][x] |= 0x18u;
	}
	for (uint32_t y = 0; y < ARDUBOY_DISPLAY_HEIGHT; y++) {
		uint8_t bit = (uint8_t)(1u << (y & 7u));
		m->vram[y >> 3][0] |= bit;
		m->vram[y >> 3][ARDUBOY_DISPLAY_WIDTH - 1u] |= bit;
		m->vram[y >> 3][y * 2u] |= bit;
	}
	for (uint32_t x = 8; x < 42; x++)
		for (uint32_t y = 8; y < 24; y++)
			if (x == 8 || x == 41 || y == 8 || y == 23 || x == 24)
				m->vram[y >> 3][x] |= (uint8_t)(1u << (y & 7u));
}

static void arduboyPrvPresentFailureBackground(ArduboyMachine *m)
{
	uint8_t savedVram[ARDUBOY_DISPLAY_HEIGHT / 8u][ARDUBOY_DISPLAY_WIDTH];
	uint8_t savedFlags = m->displayFlags;
	uint8_t savedPresentState = mPresentLastState;
	uint8_t savedDirty = m->displayDirty;
	uint64_t savedLastVisibleHostTicks = m->lastVisibleHostTicks;
	uint32_t savedVisibleFrames = m->visibleFrames;

	memcpy(savedVram, m->vram, sizeof(savedVram));
	arduboyPrvBuildPresenterTestPattern(m);
	m->displayFlags = (uint8_t)((m->displayFlags | ARDUBOY_PRESENT_STATE_DISPLAY_ON) &
		(uint8_t)~ARDUBOY_PRESENT_STATE_INVERTED);
	m->displayDirty = 1;
	mPresentLastState = ARDUBOY_PRESENT_STATE_INVALID;
	arduboyPrvPresentFrame();
	memcpy(m->vram, savedVram, sizeof(savedVram));
	m->displayFlags = savedFlags;
	m->displayDirty = savedDirty;
	m->lastVisibleHostTicks = savedLastVisibleHostTicks;
	m->visibleFrames = savedVisibleFrames;
	mPresentLastState = savedPresentState;
}

static void arduboyPrvShowFailureScreen(ArduboyMachine *m)
{
	uint16_t *fb = (uint16_t*)dispGetFb();
	uint16_t borderColor = 0xf800u;
	uint64_t elapsedTicks;
	char line[80];

	if (!m)
		return;
	if (m->failureReason == ARDUBOY_FAIL_FRAME_GUARD)
		borderColor = 0xf81fu;
	else if (m->failureReason == ARDUBOY_FAIL_NO_DISPLAY_IO)
		borderColor = 0x001fu;
	else if (m->failureReason == ARDUBOY_FAIL_HLE_BAD_RETURN ||
		m->failureReason == ARDUBOY_FAIL_HLE_QUARANTINED)
		borderColor = 0xfd20u;
	else if (m->failureReason == ARDUBOY_FAIL_STACK_FAULT)
		borderColor = 0x07ffu;
	else if (m->failureReason == ARDUBOY_FAIL_BLACK_SCREEN ||
		m->failureReason == ARDUBOY_FAIL_BOOT_WATCHDOG ||
		m->failureReason == ARDUBOY_FAIL_SPI_NOT_CAPTURED)
		borderColor = 0x001fu;

	elapsedTicks = m->bootHostTicks ? (getTime() - m->bootHostTicks) : 0;
	arduboyPrvPresentFailureBackground(m);
	arduboyPrvDebugFillRect(fb, 8, 8, 304, 224, 0x0000u);
	arduboyPrvDebugBorder(fb, borderColor);
	arduboyPrvDebugDrawText(fb, 14, 14, "ARDUBOY HYBRID STOP", 0xffe0u);

	(void)sprintf(line, "REASON: %s", arduboyPrvFailureName(m->failureReason));
	arduboyPrvDebugDrawText(fb, 14, 32, line, 0xffffu);
	(void)sprintf(line, "PC:%04x OP:%04x LAST:%04x/%04x",
		(unsigned)m->unsupportedPc, (unsigned)m->unsupportedOpcode,
		(unsigned)m->lastPc, (unsigned)m->lastOpcode);
	arduboyPrvDebugDrawText(fb, 14, 48, line, 0xffffu);
	(void)sprintf(line, "CYC:%08lx%08lx",
		(unsigned long)(m->cycles >> 32), (unsigned long)m->cycles);
	arduboyPrvDebugDrawText(fb, 14, 64, line, 0xffffu);
	(void)sprintf(line, "SPI:%lu SEL:%lu IGN:%lu H:%lu",
		(unsigned long)m->spiWrites, (unsigned long)m->spiSelectedWrites,
		(unsigned long)m->spiIgnoredWrites, (unsigned long)m->hleSpiWrites);
	arduboyPrvDebugDrawText(fb, 14, 80, line, 0xffffu);
	(void)sprintf(line, "CMD:%lu DATA:%lu LCD:%02x/%02x",
		(unsigned long)m->displayCommandWrites, (unsigned long)m->displayDataWrites,
		(unsigned)m->lastDisplayCommand, (unsigned)m->lastDisplayData);
	arduboyPrvDebugDrawText(fb, 14, 94, line, 0xffffu);
	(void)sprintf(line, "PINS:%02x PD:%02x DD:%02x TR:%lu",
		(unsigned)m->lastDisplayPins, (unsigned)m->data[AVR_PORTD],
		(unsigned)m->data[AVR_DDRD], (unsigned long)m->displayPinTransitions);
	arduboyPrvDebugDrawText(fb, 14, 108, line, 0xffffu);
	(void)sprintf(line, "LCD:%02x/%02x FL:%02x OFF:%u GUARD:%lu",
		(unsigned)m->lastDisplayCommand, (unsigned)m->lastDisplayData,
		(unsigned)m->displayFlags, (unsigned)m->displayExplicitOff,
		(unsigned long)m->frameGuardTrips);
	arduboyPrvDebugDrawText(fb, 14, 122, line, 0xffffu);
	(void)sprintf(line, "HLE:%s PC:%04x RET:%04x",
		arduboyPrvHleName(m->lastHleKind), (unsigned)m->lastHlePc,
		(unsigned)m->lastHleReturnPc);
	arduboyPrvDebugDrawText(fb, 14, 136, line, 0xffffu);
	(void)sprintf(line, "SP:%04x/%04x CONF:%u FL:%02x Q:%lu BAD:%lu",
		(unsigned)m->lastHleEntrySp, (unsigned)m->lastHleExitSp,
		(unsigned)m->lastHleConfidence, (unsigned)m->lastHleFlags,
		(unsigned long)m->hleQuarantines, (unsigned long)m->hleBadReturns);
	arduboyPrvDebugDrawText(fb, 14, 150, line, 0xffffu);
	(void)sprintf(line, "TGT:%u SBUF:%04x SOFT:%lu SKIP:%lu",
		(unsigned)m->hleCount, (unsigned)m->screenBufferAddr,
		(unsigned long)m->hleSoftMisses, (unsigned long)m->hleSkippedHeuristics);
	arduboyPrvDebugDrawText(fb, 14, 164, line, 0xffffu);
	(void)sprintf(line, "HOST:%lums G:%lu/s B:%lu/s VIS:%lu",
		(unsigned long)arduboyPrvTicksToMs(elapsedTicks),
		(unsigned long)arduboyPrvRatePerSecond(m->cycles, elapsedTicks),
		(unsigned long)arduboyPrvRatePerSecond(m->blockRuns, elapsedTicks),
		(unsigned long)m->visibleFrames);
	arduboyPrvDebugDrawText(fb, 14, 178, line, 0xffffu);
	if (m->stackFaultOp != ARDUBOY_STACK_OP_NONE)
		(void)sprintf(line, "STK:%s SP:%04x PRE:%04x RET:%04x",
			arduboyPrvStackOpName(m->stackFaultOp), (unsigned)m->stackFaultSp,
			(unsigned)m->stackFaultPrevSp, (unsigned)m->stackFaultReturnPc);
	else
		(void)sprintf(line, "HOT:%04x/%lu %04x/%lu",
			(unsigned)m->hotPc[0], (unsigned long)m->hotCount[0],
			(unsigned)m->hotPc[1], (unsigned long)m->hotCount[1]);
	arduboyPrvDebugDrawText(fb, 14, 192, line, 0xffffu);
	arduboyPrvDebugDrawText(fb, 14, 204, "CENTER: MENU  B: EXIT FROM MENU", 0x07e0u);
	arduboyPrvDebugDrawText(fb, 14, 222, "BACKGROUND: PRESENTER TEST PATTERN", 0x07ffu);
	dispPrvWaitForScanoutStart();
}

static void arduboyPrvLogFailure(const ArduboyMachine *m)
{
	uint64_t elapsedTicks;

	if (!m || m->failureReason == ARDUBOY_FAIL_NONE)
		return;
	elapsedTicks = m->bootHostTicks ? (getTime() - m->bootHostTicks) : 0;
	prRaw("Arduboy stop: reason=%u pc=%04x op=%04x last=%04x/%04x wall=%lums guest=%lu/s blocks=%lu/s ops=%lu/s cycles=%08lx%08lx visible=%lu bootWatch=%lu spi=%lu sel=%lu ign=%lu hleSpi=%lu hleIgn=%lu cmd=%lu data=%lu guard=%lu portD=%02x ddrD=%02x pins=%02x pinTr=%lu lcd=%02x/%02x fl=%02x off=%u sbuf=%04x hle=%s/%04x ret=%04x sp=%04x/%04x conf=%u flags=%02x q=%lu bad=%lu soft=%lu skip=%lu cache=%lu/%lu hot=%04x/%lu,%04x/%lu stk=%s fault=%04x prev=%04x stkret=%04x\n",
		(unsigned)m->failureReason, (unsigned)m->unsupportedPc, (unsigned)m->unsupportedOpcode,
		(unsigned)m->lastPc, (unsigned)m->lastOpcode,
		(unsigned long)arduboyPrvTicksToMs(elapsedTicks),
		(unsigned long)arduboyPrvRatePerSecond(m->cycles, elapsedTicks),
		(unsigned long)arduboyPrvRatePerSecond(m->blockRuns, elapsedTicks),
		(unsigned long)arduboyPrvRatePerSecond(m->opRuns, elapsedTicks),
		(unsigned long)(m->cycles >> 32),
		(unsigned long)m->cycles, (unsigned long)m->visibleFrames,
		(unsigned long)m->bootWatchdogTrips, (unsigned long)m->spiWrites,
		(unsigned long)m->spiSelectedWrites, (unsigned long)m->spiIgnoredWrites,
		(unsigned long)m->hleSpiWrites, (unsigned long)m->hleSpiIgnoredWrites,
		(unsigned long)m->displayCommandWrites, (unsigned long)m->displayDataWrites,
		(unsigned long)m->frameGuardTrips, (unsigned)m->lastPortD,
		(unsigned)m->data[AVR_DDRD], (unsigned)m->lastDisplayPins,
		(unsigned long)m->displayPinTransitions,
		(unsigned)m->lastDisplayCommand, (unsigned)m->lastDisplayData, (unsigned)m->displayFlags,
		(unsigned)m->displayExplicitOff, (unsigned)m->screenBufferAddr,
		arduboyPrvHleName(m->lastHleKind), (unsigned)m->lastHlePc, (unsigned)m->lastHleReturnPc,
		(unsigned)m->lastHleEntrySp, (unsigned)m->lastHleExitSp, (unsigned)m->lastHleConfidence,
		(unsigned)m->lastHleFlags, (unsigned long)m->hleQuarantines, (unsigned long)m->hleBadReturns,
		(unsigned long)m->hleSoftMisses, (unsigned long)m->hleSkippedHeuristics,
		(unsigned long)m->cacheHits, (unsigned long)m->cacheMisses,
		(unsigned)m->hotPc[0], (unsigned long)m->hotCount[0],
		(unsigned)m->hotPc[1], (unsigned long)m->hotCount[1],
		arduboyPrvStackOpName(m->stackFaultOp), (unsigned)m->stackFaultSp,
		(unsigned)m->stackFaultPrevSp, (unsigned)m->stackFaultReturnPc);
}

static void arduboyPrvFailureLoop(ArduboyMachine *m)
{
	if (!m || m->failureReason == ARDUBOY_FAIL_NONE)
		return;
	arduboyPrvLogFailure(m);
	arduboyPrvShowFailureScreen(m);
	while (!mAbortRequested && m->failureReason != ARDUBOY_FAIL_NONE) {
		if (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER) {
			arduboyPrvSyncEepromToSave(true);
			arduboyPortInGameMenu();
			while (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER) {
			}
			if (!mAbortRequested)
				arduboyPrvShowFailureScreen(m);
		}
		dispPrvFrameCtrWait();
	}
}

#ifdef ARDUBOY_PERF
static void arduboyPrvPerfLogFrame(ArduboyMachine *m)
{
	uint64_t now = getTime();
	uint64_t hostTicks = now - m->perfLastHostTicks;
	uint64_t guestCycles = m->cycles - m->perfLastGuestCycles;
	uint32_t hostMsT100;
	uint32_t guestCyclesPerSec;
	uint32_t hleCore;
	uint32_t hleDraw;
	uint32_t hleSprite;
	uint32_t hleInput;
	uint32_t hleAudio;
	uint32_t hleLibc;

	m->perfFrames++;
	if (hostTicks < TICKS_PER_SECOND && guestCycles < ARDUBOY_PERF_LOG_CYCLES)
		return;
	hostMsT100 = m->perfFrames ?
		(uint32_t)((hostTicks * 100000ull) / (TICKS_PER_SECOND * (uint64_t)m->perfFrames)) : 0;
	guestCyclesPerSec = hostTicks ? (uint32_t)((guestCycles * (uint64_t)TICKS_PER_SECOND) / hostTicks) : 0;
	hleCore = m->perfHleCalls[ArduboyHleDelay] + m->perfHleCalls[ArduboyHleDelayShort] +
		m->perfHleCalls[ArduboyHleDisplay] + m->perfHleCalls[ArduboyHleDisplayClear] +
		m->perfHleCalls[ArduboyHlePaintScreen] + m->perfHleCalls[ArduboyHlePaintScreenProgmem] +
		m->perfHleCalls[ArduboyHleSpiTransfer] + m->perfHleCalls[ArduboyHleNextFrame];
	hleDraw = m->perfHleCalls[ArduboyHleClear] + m->perfHleCalls[ArduboyHleFillScreen] +
		m->perfHleCalls[ArduboyHleDrawPixel] + m->perfHleCalls[ArduboyHleDrawFastHLine] +
		m->perfHleCalls[ArduboyHleDrawFastVLine] + m->perfHleCalls[ArduboyHleDrawLine] +
		m->perfHleCalls[ArduboyHleDrawRect] + m->perfHleCalls[ArduboyHleFillRect] +
		m->perfHleCalls[ArduboyHleDrawBitmap];
	hleSprite = m->perfHleCalls[ArduboyHleSpritesDraw];
	hleInput = m->perfHleCalls[ArduboyHleButtonsState] + m->perfHleCalls[ArduboyHlePollButtons] +
		m->perfHleCalls[ArduboyHlePressed] + m->perfHleCalls[ArduboyHleNotPressed] +
		m->perfHleCalls[ArduboyHleJustPressed] + m->perfHleCalls[ArduboyHleJustReleased];
	hleAudio = m->perfHleCalls[ArduboyHleAudioEnabled] + m->perfHleCalls[ArduboyHleAudioOnOff] +
		m->perfHleCalls[ArduboyHleAudioSave] + m->perfHleCalls[ArduboyHleToneNoop] +
		m->perfHleCalls[ArduboyHleTonesNoop] + m->perfHleCalls[ArduboyHleTonePlaying];
	hleLibc = m->perfHleCalls[ArduboyHleMemset] + m->perfHleCalls[ArduboyHleMemcpy] +
		m->perfHleCalls[ArduboyHleMemmove];
	pr("ARD perf: guest=%lu cyc/s frame=%lu.%02lums blocks=%lu ops=%lu decoded=%lu present=%lu wait=%lu cache=%lu/%lu reset=%lu poll=%lu hle=%lu miss=%lu q=%lu soft=%lu skip=%lu core=%lu draw=%lu spr=%lu input=%lu audio=%lu libc=%lu dly=%lu d16=%lu disp=%lu paint=%lu spi=%lu next=%lu hot=%04x:%lu %04x:%lu %04x:%lu %04x:%lu\n",
		(unsigned long)guestCyclesPerSec,
		(unsigned long)(hostMsT100 / 100u), (unsigned long)(hostMsT100 % 100u),
		(unsigned long)m->perfBlocks, (unsigned long)m->perfOps,
		(unsigned long)m->perfDecodedBlocks, (unsigned long)m->perfPresentTicks,
		(unsigned long)m->perfWaitTicks, (unsigned long)m->perfCacheHits,
		(unsigned long)m->perfCacheMisses, (unsigned long)m->cacheResets,
		(unsigned long)m->pollingCollapses,
		(unsigned long)(hleCore + hleDraw + hleSprite + hleInput + hleAudio + hleLibc),
		(unsigned long)m->perfHleMisses, (unsigned long)m->perfHleQuarantines,
		(unsigned long)m->hleSoftMisses, (unsigned long)m->hleSkippedHeuristics,
		(unsigned long)hleCore, (unsigned long)hleDraw, (unsigned long)hleSprite,
		(unsigned long)hleInput, (unsigned long)hleAudio, (unsigned long)hleLibc,
		(unsigned long)m->perfHleCalls[ArduboyHleDelay],
		(unsigned long)m->perfHleCalls[ArduboyHleDelayShort],
		(unsigned long)(m->perfHleCalls[ArduboyHleDisplay] + m->perfHleCalls[ArduboyHleDisplayClear]),
		(unsigned long)(m->perfHleCalls[ArduboyHlePaintScreen] + m->perfHleCalls[ArduboyHlePaintScreenProgmem]),
		(unsigned long)m->perfHleCalls[ArduboyHleSpiTransfer],
		(unsigned long)m->perfHleCalls[ArduboyHleNextFrame],
		(unsigned)m->perfHotPc[0], (unsigned long)m->perfHotCount[0],
		(unsigned)m->perfHotPc[1], (unsigned long)m->perfHotCount[1],
		(unsigned)m->perfHotPc[2], (unsigned long)m->perfHotCount[2],
		(unsigned)m->perfHotPc[3], (unsigned long)m->perfHotCount[3]);
	m->perfLastHostTicks = now;
	m->perfLastGuestCycles = m->cycles;
	m->perfFrames = 0;
	m->perfBlocks = 0;
	m->perfOps = 0;
	m->perfDecodedBlocks = 0;
	m->perfPresentTicks = 0;
	m->perfWaitTicks = 0;
	m->perfCacheHits = 0;
	m->perfCacheMisses = 0;
	m->perfHleQuarantines = 0;
	m->perfHleMisses = 0;
	memset(m->perfHleCalls, 0, sizeof(m->perfHleCalls));
	memset(m->perfHotPc, 0, sizeof(m->perfHotPc));
	memset(m->perfHotCount, 0, sizeof(m->perfHotCount));
}
#endif

void arduboyRun(const void *rom, uint32_t romSize, void *saveRam, uint32_t saveRamSize)
{
	struct ArduboyRomInfo info;

	if (!arduboyAnalyzeRom(rom, romSize, &info) || info.isPackage)
		return;

	mMachine = (ArduboyMachine*)(((uint8_t*)CART_RAM_ADDR_IN_RAM) + ARDUBOY_SAVE_RAM_SIZE);
	memset(mMachine, 0, sizeof(*mMachine));
	memset(mMachine->flash, 0xff, sizeof(mMachine->flash));
	if (!arduboyPrvHexParse(rom, romSize, arduboyPrvLoadHexData, mMachine, NULL)) {
		mMachine = NULL;
		return;
	}

	mSaveRam = (uint8_t*)saveRam;
	mSaveRamSize = saveRamSize;
	mAbortRequested = false;
	audioPwmStop();
	arduboyPrvMachineReset(mMachine, mSaveRam, mSaveRamSize);
	arduboyPrvScanHleTargets(mMachine);
	arduboyRefreshDisplayOptions();
	dispPrvFrameCtrReset();

	while (!mAbortRequested && !mMachine->unsupported && !mMachine->halted) {
		uint64_t frameEnd = mMachine->cycles + ARDUBOY_FRAME_CYCLES;
		uint32_t guard = 0;

		if (!arduboyPrvServiceRuntimeControls(mMachine))
			break;

		arduboyPrvApplyButtons(mMachine);
		while (!mAbortRequested && !mMachine->unsupported && !mMachine->halted &&
			mMachine->cycles < frameEnd && guard++ < ARDUBOY_FRAME_STEP_GUARD) {
			if ((guard & 0x1fu) == 0 && !arduboyPrvServiceRuntimeControls(mMachine))
				break;
			if (!arduboyPrvRunBlock(mMachine))
				break;
		}
		if (!mAbortRequested && !mMachine->unsupported && !mMachine->halted &&
			mMachine->cycles < frameEnd && guard >= ARDUBOY_FRAME_STEP_GUARD) {
			mMachine->frameGuardTrips++;
			arduboyPrvStopWithRuntimeFailure(mMachine, ARDUBOY_FAIL_FRAME_GUARD);
		}
		if (!mAbortRequested && !mMachine->unsupported && !mMachine->halted &&
			mMachine->cycles >= ARDUBOY_NO_DISPLAY_CYCLES && !mMachine->spiWrites &&
			!mMachine->displayCommandWrites && !mMachine->displayDataWrites) {
			arduboyPrvStopWithRuntimeFailure(mMachine, ARDUBOY_FAIL_NO_DISPLAY_IO);
		}
		if (!mAbortRequested && !mMachine->unsupported && !mMachine->halted &&
			arduboyPrvShouldReportBlackScreen(mMachine)) {
			arduboyPrvStopWithRuntimeFailure(mMachine, arduboyPrvBootWatchdogFailureReason(mMachine));
		}
		arduboyPrvSyncEepromToSave(false);
#ifdef ARDUBOY_PERF
		{
			uint64_t waitStart = getTime();
#endif
		dispPrvFrameCtrWait();
#ifdef ARDUBOY_PERF
			mMachine->perfWaitTicks += (uint32_t)(getTime() - waitStart);
		}
		{
			uint64_t presentStart = getTime();
#endif
		arduboyPrvPresentFrame();
#ifdef ARDUBOY_PERF
			mMachine->perfPresentTicks += (uint32_t)(getTime() - presentStart);
		}
		arduboyPrvPerfLogFrame(mMachine);
#endif
	}

	if (!mAbortRequested && mMachine->failureReason != ARDUBOY_FAIL_NONE)
		arduboyPrvFailureLoop(mMachine);
	arduboyPrvSyncEepromToSave(true);
	audioPwmStop();
	mMachine = NULL;
	dispSetFramerate(ARDUBOY_FRAME_RATE);
}
