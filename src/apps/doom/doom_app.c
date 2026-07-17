#include "doom_dc32.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include "dcAppDraw.h"
#include "fatfs.h"
#include "gb.h"
#include "memMap.h"
#include "qspi.h"
#include "sd.h"
#include "whddata.h"

void D_DoomMain(void);

const struct DcAppImageHeader dcAppImageHeader __attribute__((section(".dcapp_header"), used, aligned(256))) = {
	.magic = DCAPP_MAGIC,
	.headerSize = DCAPP_HEADER_SIZE,
	.abiVersion = DCAPP_ABI_VERSION,
	.runtime = DcAppIdDoom,
	.loadAddr = 0x10080000u,
	.appRamStart = 0x2005F000u,
	.appRamSize = 0x00014000u,
};

const struct DcAppHostApi *doomDc32Host;
struct Canvas doomDc32Canvas;
bool doomDc32CanvasValid;
static jmp_buf mDoomExit;
static volatile int mDoomRunning;
static volatile int mDoomExitCode;
static char mDoomError[160];

static void doomDc32AppendChar(char **dstP, size_t *remainP, char ch)
{
	if (*remainP > 1u) {
		**dstP = ch;
		(*dstP)++;
		(*remainP)--;
	}
}

static void doomDc32AppendStr(char **dstP, size_t *remainP, const char *str)
{
	if (!str)
		str = "(null)";
	while (*str)
		doomDc32AppendChar(dstP, remainP, *str++);
}

static void doomDc32AppendU32(char **dstP, size_t *remainP, uint32_t value, uint32_t base, bool neg)
{
	char tmp[11];
	uint32_t n = 0;

	if (neg)
		doomDc32AppendChar(dstP, remainP, '-');
	if (!value)
		tmp[n++] = '0';
	while (value && n < sizeof(tmp)) {
		uint32_t digit = value % base;

		tmp[n++] = (char)(digit < 10u ? '0' + digit : 'a' + digit - 10u);
		value /= base;
	}
	while (n)
		doomDc32AppendChar(dstP, remainP, tmp[--n]);
}

static void doomDc32Format(char *dst, size_t dstLen, const char *fmt, va_list ap)
{
	char *out = dst;
	size_t remain = dstLen;

	if (!dstLen)
		return;
	if (!fmt)
		fmt = "";
	while (*fmt) {
		char ch = *fmt++;

		if (ch != '%') {
			doomDc32AppendChar(&out, &remain, ch);
			continue;
		}
		ch = *fmt++;
		if (ch == 'l')
			ch = *fmt++;
		switch (ch) {
			case '%':
				doomDc32AppendChar(&out, &remain, '%');
				break;
			case 's':
				doomDc32AppendStr(&out, &remain, va_arg(ap, const char*));
				break;
			case 'd':
			case 'i': {
				int value = va_arg(ap, int);
				uint32_t mag = (uint32_t)value;

				if (value < 0)
					mag = (uint32_t)(-value);
				doomDc32AppendU32(&out, &remain, mag, 10u, value < 0);
				break;
			}
			case 'u':
				doomDc32AppendU32(&out, &remain, va_arg(ap, uint32_t), 10u, false);
				break;
			case 'p':
				doomDc32AppendStr(&out, &remain, "0x");
				doomDc32AppendU32(&out, &remain, (uint32_t)(uintptr_t)va_arg(ap, void*), 16u, false);
				break;
			case 'x':
			case 'X':
				doomDc32AppendU32(&out, &remain, va_arg(ap, uint32_t), 16u, false);
				break;
			default:
				doomDc32AppendChar(&out, &remain, ch ? ch : '?');
				break;
		}
	}
	*out = 0;
}

void doomDc32SetErrorV(const char *fmt, va_list ap)
{
	doomDc32Format(mDoomError, sizeof(mDoomError), fmt, ap);
	if (doomDc32Host && doomDc32Host->log)
		doomDc32Host->log("DOOM: %s\n", mDoomError);
}

void doomDc32SetError(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	doomDc32SetErrorV(fmt, ap);
	va_end(ap);
}

void doomDc32Status(const char *title, const char *detail, uint32_t done, uint32_t total)
{
	struct Canvas *cnv = doomDc32CanvasValid ? &doomDc32Canvas : NULL;
	const struct DcAppLoadingState loading = {
		.appName = "DOOM",
		.iconId = "1f479",
		.title = title,
		.detail = detail,
		.done = done,
		.total = total,
		.animationStep = done,
	};

	if (!cnv || cnv->bpp != 16u)
		return;
	dispPrvWaitForScanoutStart();
	dcAppDrawLoadingCanvas(cnv, &loading);
}

void doomDc32WaitForCenter(const char *title, const char *detail)
{
	if (title || detail)
		doomDc32Status(title, detail, 100u, 100u);
	if (!doomDc32Host || !doomDc32Host->uiKeysRaw || !doomDc32Host->delayMsec)
		return;
	while (doomDc32Host->uiKeysRaw() & UI_KEY_BIT_CENTER)
		doomDc32Host->delayMsec(10);
	while (!(doomDc32Host->uiKeysRaw() & UI_KEY_BIT_CENTER))
		doomDc32Host->delayMsec(10);
	while (doomDc32Host->uiKeysRaw() & UI_KEY_BIT_CENTER)
		doomDc32Host->delayMsec(10);
}

static bool doomDc32DiskRead(void *diskUserData, uint32_t sec, uint32_t numSec, void *dstP)
{
	uint8_t *dst = (uint8_t*)dstP;

	(void)diskUserData;
	while (numSec--) {
		if (!sdSecRead(sec++, dst))
			return false;
		dst += SD_BLOCK_SIZE;
	}
	return true;
}

static bool doomDc32DiskWrite(void *diskUserData, uint32_t sec, uint32_t numSec, const void *srcP)
{
	const uint8_t *src = (const uint8_t*)srcP;

	(void)diskUserData;
	while (numSec--) {
		if (!sdSecWrite(sec++, src))
			return false;
		src += SD_BLOCK_SIZE;
	}
	return true;
}

static uint32_t doomDc32AlignUp(uint32_t value, uint32_t align)
{
	return (value + align - 1u) & ~(align - 1u);
}

static bool doomDc32ReadExact(struct FatfsFil *fil, void *dst, uint32_t len)
{
	uint32_t got = 0;

	return fatfsFileRead(fil, dst, len, &got) && got == len;
}

static bool doomDc32ReadAt(struct FatfsFil *fil, uint32_t pos, void *dst, uint32_t len)
{
	return fatfsFileSeek(fil, pos) && doomDc32ReadExact(fil, dst, len);
}

static uint16_t doomDc32Le16(const uint8_t *p)
{
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t doomDc32Le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

struct DoomDc32WhxIdentity {
	uint32_t fileSize;
	uint32_t hash;
	char sourceName[15];
};

static char doomDc32AsciiLower(char ch)
{
	if (ch >= 'A' && ch <= 'Z')
		return (char)(ch + 'a' - 'A');
	return ch;
}

static bool doomDc32AsciiEquals(const char *a, const char *b)
{
	while (*a || *b) {
		if (doomDc32AsciiLower(*a++) != doomDc32AsciiLower(*b++))
			return false;
	}
	return true;
}

static bool doomDc32NameEquals(const uint8_t *name, const char *want)
{
	for (uint32_t i = 0; i < 10u; i++) {
		char a = (char)name[i];
		char b = want[i];

		a = doomDc32AsciiLower(a);
		b = doomDc32AsciiLower(b);
		if (!b)
			return !a;
		if (a != b)
			return false;
	}
	return want[10] == 0;
}

static void doomDc32CopyStr(char *dst, uint32_t dstLen, const char *src)
{
	uint32_t i = 0;

	if (!dstLen)
		return;
	if (!src)
		src = "";
	while (src[i] && i + 1u < dstLen) {
		dst[i] = src[i];
		i++;
	}
	dst[i] = 0;
}

static void doomDc32CopyFixedName(char *dst, uint32_t dstLen, const uint8_t *src, uint32_t srcLen)
{
	uint32_t i = 0;

	if (!dstLen)
		return;
	while (i + 1u < dstLen && i < srcLen && src[i]) {
		dst[i] = (char)src[i];
		i++;
	}
	dst[i] = 0;
}

static bool doomDc32WhxPStartCompatible(uint32_t pstartBytes, bool setError)
{
	uint32_t entries = pstartBytes / 2u;

	if ((pstartBytes & 1u) ||
		(entries != DOOM_DC32_WHX_LEGACY_VPATCH_COUNT && entries != (uint32_t)NUM_VPATCHES)) {
		if (setError)
			doomDc32SetError("doom WHX vpatches %u expected %u/%u", entries,
				DOOM_DC32_WHX_LEGACY_VPATCH_COUNT, (uint32_t)NUM_VPATCHES);
		return false;
	}
	return true;
}

static bool doomDc32WhxHeaderValid(const uint8_t *hdr, uint32_t fileSize, bool setError)
{
	uint32_t whxSize;

	if (!hdr || hdr[0] != 'I' || hdr[1] != 'W' || hdr[2] != 'H' || hdr[3] != 'X') {
		if (setError)
			doomDc32SetError("Invalid WHX magic");
		return false;
	}
	whxSize = doomDc32Le32(hdr + 12u);
	if (whxSize != fileSize) {
		if (setError)
			doomDc32SetError("WHX size field %u != %u", whxSize, fileSize);
		return false;
	}
	return true;
}

static uint32_t doomDc32WhxLumpSize(const uint8_t *offsets)
{
	uint32_t raw0 = doomDc32Le32(offsets);
	uint32_t raw1 = doomDc32Le32(offsets + 4u);

	return ((raw1 - raw0) & 0x00ffffffu) - (raw0 >> 30);
}

static bool doomDc32WhxFileCompatible(struct FatfsFil *fil, uint32_t fileSize,
	struct DoomDc32WhxIdentity *identity, bool setError)
{
	uint8_t hdr[36];
	uint8_t entry[12];
	uint8_t offsets[8];
	uint32_t numLumps, infoTableOfs, namedCount, namesOfs, pstartNum = 0xffffffffu;
	uint32_t pstartSize;
	bool hasE1m1 = false;
	char sourceName[15];

	if (!fileSize || fileSize > QSPI_ROM_SIZE_MAX) {
		if (setError)
			doomDc32SetError("WHX size %u exceeds %u", fileSize, QSPI_ROM_SIZE_MAX);
		return false;
	}
	if (!doomDc32ReadAt(fil, 0, hdr, sizeof(hdr)) || !doomDc32WhxHeaderValid(hdr, fileSize, setError)) {
		if (setError && !mDoomError[0])
			doomDc32SetError("Invalid WHX header");
		return false;
	}
	numLumps = doomDc32Le32(hdr + 4u);
	infoTableOfs = doomDc32Le32(hdr + 8u);
	namedCount = doomDc32Le16(hdr + 34u);
	doomDc32CopyFixedName(sourceName, sizeof(sourceName), hdr + 20u, 14u);
	if (!doomDc32AsciiEquals(sourceName, "DOOM1.WAD")) {
		if (setError)
			doomDc32SetError("WHX source %s is not DOOM1.WAD", sourceName);
		return false;
	}
	if (!numLumps || infoTableOfs > fileSize || (numLumps + 1u) > (fileSize - infoTableOfs) / 4u) {
		if (setError)
			doomDc32SetError("Invalid WHX directory");
		return false;
	}
	namesOfs = 36u + (numLumps + 1u) * 4u;
	if (namesOfs > fileSize || namedCount > (fileSize - namesOfs) / 12u) {
		if (setError)
			doomDc32SetError("Invalid WHX names");
		return false;
	}
	for (uint32_t i = 0; i < namedCount; i++) {
		if (!doomDc32ReadAt(fil, namesOfs + i * 12u, entry, sizeof(entry))) {
			if (setError)
				doomDc32SetError("Cannot read WHX names");
			return false;
		}
		if (doomDc32NameEquals(entry, "p_start"))
			pstartNum = doomDc32Le16(entry + 10u);
		else if (doomDc32NameEquals(entry, "e1m1"))
			hasE1m1 = true;
	}
	if (pstartNum >= numLumps) {
		if (setError)
			doomDc32SetError("WHX missing P_START");
		return false;
	}
	if (!hasE1m1) {
		if (setError)
			doomDc32SetError("WHX missing E1M1");
		return false;
	}
	if (!doomDc32ReadAt(fil, infoTableOfs + pstartNum * 4u, offsets, sizeof(offsets))) {
		if (setError)
			doomDc32SetError("Cannot read WHX P_START");
		return false;
	}
	pstartSize = doomDc32WhxLumpSize(offsets);
	if (!doomDc32WhxPStartCompatible(pstartSize, setError))
		return false;
	if (identity) {
		identity->fileSize = fileSize;
		identity->hash = doomDc32Le32(hdr + 16u);
		doomDc32CopyStr(identity->sourceName, sizeof(identity->sourceName), sourceName);
	}
	return true;
}

static bool doomDc32WhxMemoryCompatible(const uint8_t *whx, uint32_t fileSize,
	bool setError, struct DoomDc32WhxIdentity *identity)
{
	uint32_t numLumps, infoTableOfs, namedCount, namesOfs, pstartNum = 0xffffffffu;
	uint32_t pstartSize;
	bool hasE1m1 = false;
	char sourceName[15];

	if (!doomDc32WhxHeaderValid(whx, fileSize, setError))
		return false;
	numLumps = doomDc32Le32(whx + 4u);
	infoTableOfs = doomDc32Le32(whx + 8u);
	namedCount = doomDc32Le16(whx + 34u);
	doomDc32CopyFixedName(sourceName, sizeof(sourceName), whx + 20u, 14u);
	if (!doomDc32AsciiEquals(sourceName, "DOOM1.WAD")) {
		if (setError)
			doomDc32SetError("Staged WHX source %s is not DOOM1.WAD", sourceName);
		return false;
	}
	if (!numLumps || infoTableOfs > fileSize || (numLumps + 1u) > (fileSize - infoTableOfs) / 4u) {
		if (setError)
			doomDc32SetError("Invalid staged WHX directory");
		return false;
	}
	namesOfs = 36u + (numLumps + 1u) * 4u;
	if (namesOfs > fileSize || namedCount > (fileSize - namesOfs) / 12u) {
		if (setError)
			doomDc32SetError("Invalid staged WHX names");
		return false;
	}
	for (uint32_t i = 0; i < namedCount; i++) {
		const uint8_t *entry = whx + namesOfs + i * 12u;

		if (doomDc32NameEquals(entry, "p_start"))
			pstartNum = doomDc32Le16(entry + 10u);
		else if (doomDc32NameEquals(entry, "e1m1"))
			hasE1m1 = true;
	}
	if (pstartNum >= numLumps) {
		if (setError)
			doomDc32SetError("Staged WHX missing P_START");
		return false;
	}
	if (!hasE1m1) {
		if (setError)
			doomDc32SetError("Staged WHX missing E1M1");
		return false;
	}
	pstartSize = doomDc32WhxLumpSize(whx + infoTableOfs + pstartNum * 4u);
	if (!doomDc32WhxPStartCompatible(pstartSize, setError))
		return false;
	if (identity) {
		identity->fileSize = fileSize;
		identity->hash = doomDc32Le32(whx + 16u);
		doomDc32CopyStr(identity->sourceName, sizeof(identity->sourceName), sourceName);
	}
	return true;
}

static bool doomDc32WhxSameIdentity(const struct DoomDc32WhxIdentity *a, const struct DoomDc32WhxIdentity *b)
{
	return a->fileSize == b->fileSize &&
		a->hash == b->hash &&
		doomDc32AsciiEquals(a->sourceName, b->sourceName);
}

static bool doomDc32WhxAlreadyStaged(const struct DoomDc32WhxIdentity *identity)
{
	const uint8_t *hdr = (const uint8_t*)flashUncachedPtr(DOOM_DC32_WHX_FLASH_ADDR);
	struct DoomDc32WhxIdentity staged;

	memset(&staged, 0, sizeof(staged));
	return identity &&
		doomDc32Le32(hdr + 12u) == identity->fileSize &&
		doomDc32WhxMemoryCompatible(hdr, identity->fileSize, false, &staged) &&
		doomDc32WhxSameIdentity(identity, &staged);
}

static bool doomDc32StageWhxFromVol(struct FatfsVol *vol)
{
	struct DoomDc32WhxIdentity identity;
	struct DoomDc32WhxIdentity verifyIdentity;
	struct FatfsFil *fil;
	uint32_t eraseSize, pos;
	uint8_t buf[4096 + QSPI_WRITE_GRANULARITY];

	doomDc32Status("Checking DOOM data", DOOM_DC32_WHX_PATH, 0, 100);
	fil = fatfsFileOpen(vol, DOOM_DC32_WHX_PATH, OPEN_MODE_READ);
	if (!fil) {
		doomDc32SetError("Missing %s", DOOM_DC32_WHX_PATH);
		return false;
	}
	memset(&identity, 0, sizeof(identity));
	if (!doomDc32WhxFileCompatible(fil, fatfsFileGetSize(fil), &identity, true)) {
		(void)fatfsFileClose(fil);
		return false;
	}
	if (doomDc32WhxAlreadyStaged(&identity)) {
		doomDc32Status("DOOM data ready", "Doom shareware", 100u, 100u);
		(void)fatfsFileClose(fil);
		return true;
	}
	memset(&verifyIdentity, 0, sizeof(verifyIdentity));
	if (!doomDc32WhxFileCompatible(fil, fatfsFileGetSize(fil), &verifyIdentity, true) ||
		!doomDc32WhxSameIdentity(&identity, &verifyIdentity)) {
		(void)fatfsFileClose(fil);
		return false;
	}
	if (!fatfsFileSeek(fil, 0)) {
		doomDc32SetError("Cannot rewind %s", DOOM_DC32_WHX_PATH);
		(void)fatfsFileClose(fil);
		return false;
	}

	eraseSize = doomDc32AlignUp(identity.fileSize, QSPI_ERASE_GRANULARITY);
	doomDc32Status("Staging DOOM data", "Erasing flash", 0, 0);
	if (!doomDc32Host->flashWrite(DOOM_DC32_WHX_FLASH_ADDR, eraseSize, NULL, 0)) {
		doomDc32SetError("Failed to erase DOOM data flash");
		(void)fatfsFileClose(fil);
		return false;
	}
	for (pos = 0; pos < identity.fileSize; pos += 4096u) {
		uint32_t now = identity.fileSize - pos;
		uint32_t writeSize;

		if (now > 4096u)
			now = 4096u;
		doomDc32Status("Staging DOOM data", DOOM_DC32_WHX_PATH, pos, identity.fileSize);
		if (!doomDc32ReadExact(fil, buf, now)) {
			doomDc32SetError("Failed reading WHX at %u", pos);
			(void)fatfsFileClose(fil);
			return false;
		}
		writeSize = doomDc32AlignUp(now, QSPI_WRITE_GRANULARITY);
		if (writeSize > now)
			memset(buf + now, 0xff, writeSize - now);
		if (!doomDc32Host->flashWrite(DOOM_DC32_WHX_FLASH_ADDR + pos, 0, buf, writeSize)) {
			doomDc32SetError("Failed writing DOOM data at %u", pos);
			(void)fatfsFileClose(fil);
			return false;
		}
	}
	(void)fatfsFileClose(fil);
	doomDc32Status("Staging DOOM data", "Verifying", identity.fileSize, identity.fileSize);
	return doomDc32WhxMemoryCompatible((const uint8_t*)flashUncachedPtr(DOOM_DC32_WHX_FLASH_ADDR),
		identity.fileSize, true, NULL) && doomDc32WhxAlreadyStaged(&identity);
}

static bool doomDc32StageWhx(const struct DcAppRunArgs *args)
{
	struct FatfsVol *vol;
	bool mountedHere = false;
	bool ok;

	if (args && args->vol)
		return doomDc32StageWhxFromVol(args->vol);
	if (!sdCardInit())
		return false;
	vol = fatfsMount(doomDc32DiskRead, doomDc32DiskWrite, NULL);
	if (!vol)
		return false;
	mountedHere = true;
	ok = doomDc32StageWhxFromVol(vol);
	if (mountedHere)
		(void)fatfsUnmount(vol);
	return ok;
}

int dcAppEntry(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	doomDc32Host = host;
	mDoomError[0] = 0;
	mDoomExitCode = 0;
	if (!host || !host->flashWrite || !host->displayFb || !host->uiKeysRaw)
		return -1;
	memset(&doomDc32Canvas, 0, sizeof(doomDc32Canvas));
	if (args && args->canvas)
		doomDc32Canvas = *args->canvas;
	if (!doomDc32Canvas.framebuffer)
		doomDc32Canvas.framebuffer = host->displayFb();
	if (!doomDc32Canvas.w)
		doomDc32Canvas.w = DOOM_DC32_SCREEN_W;
	if (!doomDc32Canvas.h)
		doomDc32Canvas.h = DOOM_DC32_PRESENT_H;
	if (!doomDc32Canvas.bpp)
		doomDc32Canvas.bpp = 16u;
	if (!args || !args->canvas)
		doomDc32Canvas.rotated = 1u;
	if (args && args->rotate)
		doomDc32Canvas.flipped = 1u;
	doomDc32CanvasValid = true;
	if (!doomDc32StageWhx(args)) {
		if (!mDoomError[0])
			doomDc32SetError("Failed to stage DOOM WHX");
		doomDc32WaitForCenter("DOOM failed", mDoomError);
		return -1;
	}

	doomDc32Status("Starting DOOM", "Loading engine", 100u, 100u);
	mDoomRunning = 1;
	if (setjmp(mDoomExit) == 0) {
		D_DoomMain();
	}
	mDoomRunning = 0;
	doomDc32SoundStopAll();
	if (mDoomExitCode) {
		doomDc32WaitForCenter("DOOM failed", mDoomError[0] ? mDoomError : "Unknown error");
		return -1;
	}
	if (host->delayMsec) {
		while (host->uiKeysRaw() & UI_KEY_BIT_CENTER)
			host->delayMsec(10);
	}
	return 0;
}

void dcAppAbort(void)
{
	doomDc32Exit(0);
}

void dcAppRefreshDisplayOptions(void)
{
}

void doomDc32Exit(int code)
{
	mDoomExitCode = code;
	doomDc32SoundStopAll();
	if (mDoomRunning)
		longjmp(mDoomExit, 1);
	while (1) {
	}
}
