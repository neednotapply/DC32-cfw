#include <string.h>
#include "flipper_ir.h"
#include "memMap.h"

#define FLIPPER_IR_MAX_TIMINGS	1024
#define FLIPPER_IR_LINE_LEN		4096

struct IrBlock {
	char name[32];
	char type[16];
	char protocol[16];
	uint32_t address;
	uint32_t command;
	uint32_t frequency;
	uint16_t timings[FLIPPER_IR_MAX_TIMINGS];
	uint32_t numTimings;
	bool haveName;
	bool haveType;
	bool haveProtocol;
	bool haveAddress;
	bool haveCommand;
	bool haveData;
};

#define mBlock		(*(struct IrBlock*)CART_RAM_ADDR_IN_RAM)
#define mLine		((char*)(((uint8_t*)CART_RAM_ADDR_IN_RAM) + sizeof(struct IrBlock)))

static bool strEq(const char *a, const char *b)
{
	while (*a && *b) {
		char ca = *a++, cb = *b++;
		if (ca >= 'A' && ca <= 'Z')
			ca += 'a' - 'A';
		if (cb >= 'A' && cb <= 'Z')
			cb += 'a' - 'A';
		if (ca != cb)
			return false;
	}
	return !*a && !*b;
}

static char *skipSpaces(char *s)
{
	while (*s == ' ' || *s == '\t')
		s++;
	return s;
}

static void copyTrimmed(char *dst, uint32_t dstLen, char *src)
{
	uint32_t len;

	src = skipSpaces(src);
	len = strlen(src);
	while (len && (src[len - 1] == '\r' || src[len - 1] == '\n' || src[len - 1] == ' ' || src[len - 1] == '\t'))
		len--;
	if (len >= dstLen)
		len = dstLen - 1;
	memcpy(dst, src, len);
	dst[len] = 0;
}

static bool parseDec(char **sP, uint32_t *valP)
{
	char *s = skipSpaces(*sP);
	uint32_t val = 0;
	bool any = false;

	while (*s >= '0' && *s <= '9') {
		val = val * 10 + *s++ - '0';
		any = true;
	}
	*sP = s;
	*valP = val;
	return any;
}

static uint8_t hexVal(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0xff;
}

static bool parseHexBytes(char *s, uint32_t *valP)
{
	uint32_t val = 0, shift;

	for (shift = 0; shift < 32; shift += 8) {
		uint8_t hi, lo;
		s = skipSpaces(s);
		hi = hexVal(*s++);
		lo = hexVal(*s++);
		if (hi == 0xff || lo == 0xff)
			return false;
		val |= ((uint32_t)((hi << 4) | lo)) << shift;
	}
	*valP = val;
	return true;
}

static void blockReset(void)
{
	memset(&mBlock, 0, sizeof(mBlock));
	mBlock.frequency = 38000;
}

static bool appendTiming(uint32_t val)
{
	if (!val || val > 0xffff || mBlock.numTimings == FLIPPER_IR_MAX_TIMINGS)
		return false;
	mBlock.timings[mBlock.numTimings++] = val;
	return true;
}

static bool appendPulse(uint32_t mark, uint32_t space)
{
	return appendTiming(mark) && appendTiming(space);
}

static bool appendPulseBit(bool one, uint32_t mark, uint32_t zeroSpace, uint32_t oneSpace)
{
	return appendPulse(mark, one ? oneSpace : zeroSpace);
}

static bool encodePulseDistance(uint64_t data, uint32_t bits, uint32_t leadMark, uint32_t leadSpace, uint32_t bitMark, uint32_t zeroSpace, uint32_t oneSpace, uint32_t stopMark)
{
	uint32_t i;

	mBlock.numTimings = 0;
	if (!appendPulse(leadMark, leadSpace))
		return false;
	for (i = 0; i < bits; i++) {
		if (!appendPulseBit(!!(data & (((uint64_t)1) << i)), bitMark, zeroSpace, oneSpace))
			return false;
	}
	return appendTiming(stopMark);
}

static bool encodeNec(void)
{
	uint32_t payload;

	if (strEq(mBlock.protocol, "NEC"))
		payload = (mBlock.address & 0xff) | (((~mBlock.address) & 0xff) << 8) | ((mBlock.command & 0xff) << 16) | (((~mBlock.command) & 0xff) << 24);
	else
		payload = (mBlock.address & 0xffff) | ((mBlock.command & 0xff) << 16) | (((~mBlock.command) & 0xff) << 24);

	return encodePulseDistance(payload, 32, 9000, 4500, 560, 560, 1690, 560);
}

static bool encodeSamsung32(void)
{
	uint32_t payload = (mBlock.address & 0xffff) | ((mBlock.command & 0xff) << 16) | (((~mBlock.command) & 0xff) << 24);

	return encodePulseDistance(payload, 32, 4500, 4500, 560, 560, 1690, 560);
}

static bool encodeSirc(uint32_t bits)
{
	uint64_t payload = (mBlock.command & 0x7f) | (((uint64_t)mBlock.address) << 7);

	return encodePulseDistance(payload, bits, 2400, 600, 600, 600, 1200, 600);
}

static bool encodeRca(void)
{
	uint32_t payload = ((mBlock.address & 0x0f) << 8) | (mBlock.command & 0xff);

	return encodePulseDistance(payload, 12, 4000, 4000, 500, 1000, 2000, 500);
}

static bool appendManchesterHalf(bool mark, uint32_t usec)
{
	if (mBlock.numTimings && ((mBlock.numTimings & 1) != (mark ? 1u : 0u))) {
		uint32_t val = mBlock.timings[mBlock.numTimings - 1] + usec;
		if (val > 0xffff)
			return false;
		mBlock.timings[mBlock.numTimings - 1] = val;
		return true;
	}
	return appendTiming(usec);
}

static bool appendManchesterBit(bool bit, uint32_t halfUsec)
{
	return appendManchesterHalf(!bit, halfUsec) && appendManchesterHalf(bit, halfUsec);
}

static bool encodeRc5(bool extended)
{
	uint32_t i, bits = extended ? 15 : 14;
	uint32_t payload = (3u << 12) | ((mBlock.address & 0x1f) << 6) | (mBlock.command & 0x3f);

	mBlock.numTimings = 0;
	for (i = 0; i < bits; i++) {
		if (!appendManchesterBit(!!(payload & (1u << (bits - 1 - i))), 889))
			return false;
	}
	return true;
}

static bool encodeRc6(void)
{
	uint64_t payload = (((uint64_t)mBlock.address & 0xff) << 8) | (mBlock.command & 0xff);
	uint32_t i;

	mBlock.numTimings = 0;
	if (!appendPulse(2666, 889))
		return false;
	for (i = 0; i < 20; i++) {
		if (!appendManchesterBit(!!(payload & (((uint64_t)1) << (19 - i))), 444))
			return false;
	}
	return true;
}

static bool encodeKaseikyo(void)
{
	uint64_t payload = (((uint64_t)mBlock.address) << 16) | (mBlock.command & 0xffff);

	return encodePulseDistance(payload, 48, 3456, 1728, 432, 432, 1296, 432);
}

static bool encodeParsed(void)
{
	if (!mBlock.haveProtocol || !mBlock.haveAddress || !mBlock.haveCommand)
		return false;
	if (strEq(mBlock.protocol, "NEC") || strEq(mBlock.protocol, "NECext") || strEq(mBlock.protocol, "NEC42") || strEq(mBlock.protocol, "NEC42ext"))
		return encodeNec();
	if (strEq(mBlock.protocol, "Samsung32"))
		return encodeSamsung32();
	if (strEq(mBlock.protocol, "SIRC"))
		return encodeSirc(12);
	if (strEq(mBlock.protocol, "SIRC15"))
		return encodeSirc(15);
	if (strEq(mBlock.protocol, "SIRC20"))
		return encodeSirc(20);
	if (strEq(mBlock.protocol, "RCA"))
		return encodeRca();
	if (strEq(mBlock.protocol, "RC5"))
		return encodeRc5(false);
	if (strEq(mBlock.protocol, "RC5X"))
		return encodeRc5(true);
	if (strEq(mBlock.protocol, "RC6"))
		return encodeRc6();
	if (strEq(mBlock.protocol, "Kaseikyo"))
		return encodeKaseikyo();
	return false;
}

static bool processBlock(const char *wantedName, FlipperIrSignalF signalF, void *userData, uint32_t *numSentP, uint32_t *numUnsupportedP)
{
	struct FlipperIrSignal sig;

	if (!mBlock.haveName || !strEq(mBlock.name, wantedName))
		return true;

	if (strEq(mBlock.type, "raw")) {
		if (!mBlock.haveData) {
			(*numUnsupportedP)++;
			return true;
		}
	}
	else if (strEq(mBlock.type, "parsed")) {
		if (!encodeParsed()) {
			(*numUnsupportedP)++;
			return true;
		}
	}
	else {
		(*numUnsupportedP)++;
		return true;
	}

	sig.timings = mBlock.timings;
	sig.numTimings = mBlock.numTimings;
	sig.frequency = mBlock.frequency ? mBlock.frequency : 38000;
	sig.name = mBlock.name;
	sig.protocol = mBlock.protocol;
	(*numSentP)++;
	return signalF(userData, &sig, *numSentP);
}

static void parseData(char *s)
{
	uint32_t val;

	mBlock.numTimings = 0;
	while (parseDec(&s, &val)) {
		if (!appendTiming(val))
			break;
		s = skipSpaces(s);
	}
	mBlock.haveData = mBlock.numTimings != 0;
}

static void parseLine(char *line)
{
	char *colon, *val;

	line = skipSpaces(line);
	if (!*line || *line == '#')
		return;

	colon = strchr(line, ':');
	if (!colon)
		return;
	*colon = 0;
	val = colon + 1;

	if (strEq(line, "name")) {
		copyTrimmed(mBlock.name, sizeof(mBlock.name), val);
		mBlock.haveName = true;
	}
	else if (strEq(line, "type")) {
		copyTrimmed(mBlock.type, sizeof(mBlock.type), val);
		mBlock.haveType = true;
	}
	else if (strEq(line, "protocol")) {
		copyTrimmed(mBlock.protocol, sizeof(mBlock.protocol), val);
		mBlock.haveProtocol = true;
	}
	else if (strEq(line, "address")) {
		mBlock.haveAddress = parseHexBytes(val, &mBlock.address);
	}
	else if (strEq(line, "command")) {
		mBlock.haveCommand = parseHexBytes(val, &mBlock.command);
	}
	else if (strEq(line, "frequency")) {
		mBlock.haveData = mBlock.haveData;
		(void)parseDec(&val, &mBlock.frequency);
	}
	else if (strEq(line, "data")) {
		parseData(val);
	}
}

bool flipperIrBlastNamed(struct FatfsFil *fil, const char *wantedName, FlipperIrSignalF signalF, void *userData, uint32_t *numSentP, uint32_t *numUnsupportedP)
{
	uint32_t n, linePos = 0;
	char ch;
	bool ok = true;

	*numSentP = 0;
	*numUnsupportedP = 0;
	blockReset();

	while (fatfsFileRead(fil, &ch, 1, &n) && n == 1) {
		if (ch == '#') {
			mLine[linePos] = 0;
			parseLine(mLine);
			ok = processBlock(wantedName, signalF, userData, numSentP, numUnsupportedP);
			blockReset();
			linePos = 0;
			if (!ok)
				return false;
			continue;
		}
		if (ch == '\n') {
			mLine[linePos] = 0;
			parseLine(mLine);
			linePos = 0;
		}
		else if (linePos < sizeof(mLine) - 1) {
			mLine[linePos++] = ch;
		}
	}

	if (linePos) {
		mLine[linePos] = 0;
		parseLine(mLine);
	}
	return processBlock(wantedName, signalF, userData, numSentP, numUnsupportedP);
}
