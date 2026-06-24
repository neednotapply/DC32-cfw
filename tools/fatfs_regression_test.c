#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef FATFS_USE_LFN_SUPPORT
#define FATFS_USE_LFN_SUPPORT 1
#endif

#include "../src/fatfs.h"

#define DISK_SECS			8192U
#define RESERVED_SECS		1U
#define NUM_FATS			1U
#define SECTORS_PER_FAT		64U
#define ROOT_DIR_ENTRIES	64U
#define ROOT_DIR_SECS		((ROOT_DIR_ENTRIES * 32U + FATFS_DISK_SECT_SZ - 1U) / FATFS_DISK_SECT_SZ)
#define DATA_SEC			(RESERVED_SECS + NUM_FATS * SECTORS_PER_FAT + ROOT_DIR_SECS)
#define TETRIS_ROM_NAME		"Tetris DX (World) (SGB Enhanced).gb"
#define TETRIS_SAVE_NAME	"Tetris DX.sav"
#define TETRIS_FULL_SAVE_NAME	"Tetris DX (World) (SGB Enhanced).sav"
#define CRYSTAL_ROM_NAME	"Pokemon - Crystal Version (USA, Europe).gbc"
#define CRYSTAL_SAVE_NAME	"Pokemon - Crystal Version.sav"
#define CRYSTAL_FULL_SAVE_NAME	"Pokemon - Crystal Version (USA, Europe).sav"
#define CRYSTAL_FALLBACK_SAVE_NAME	"POKEM987.SAV"
#define BUBBLE_ROM_NAME		"Bubble Bobble (USA, Europe).gb"
#define BUBBLE_SAVE_NAME	"Bubble Bobble.sav"
#define BUBBLE_FULL_SAVE_NAME	"Bubble Bobble (USA, Europe).sav"
#define BUBBLE_SAVE_SIZE	8192U
#define WRONG_SIZE_ROM_NAME	"Pokemon - Crystal Version Wrong Size (USA, Europe).gbc"
#define TEST_SAVE_MAX_SIZE	32768U
#define TEST_SAVE_DIR		"/SAVE/GB"

enum SaveNameKind {
	SaveNameKindAuto = 0,
	SaveNameKindClean = 1,
	SaveNameKindFull = 2,
	SaveNameKindFallback = 3,
};

static uint8_t mDisk[DISK_SECS * FATFS_DISK_SECT_SZ];

static void put16(uint8_t *dst, uint16_t val)
{
	dst[0] = (uint8_t)val;
	dst[1] = (uint8_t)(val >> 8);
}

static bool diskRead(void *diskUserData, uint32_t sec, uint32_t numSec, void *dst)
{
	(void)diskUserData;
	if (sec > DISK_SECS || numSec > DISK_SECS - sec)
		return false;
	memcpy(dst, mDisk + sec * FATFS_DISK_SECT_SZ, numSec * FATFS_DISK_SECT_SZ);
	return true;
}

static bool diskWrite(void *diskUserData, uint32_t sec, uint32_t numSec, const void *src)
{
	(void)diskUserData;
	if (sec > DISK_SECS || numSec > DISK_SECS - sec)
		return false;
	memcpy(mDisk + sec * FATFS_DISK_SECT_SZ, src, numSec * FATFS_DISK_SECT_SZ);
	return true;
}

static void initFat16Disk(void)
{
	uint8_t *bpb = mDisk;
	uint8_t *fat = mDisk + RESERVED_SECS * FATFS_DISK_SECT_SZ;

	memset(mDisk, 0, sizeof(mDisk));
	bpb[0] = 0xeb;
	bpb[1] = 0x3c;
	bpb[2] = 0x90;
	memcpy(bpb + 3, "MSDOS5.0", 8);
	put16(bpb + 11, FATFS_DISK_SECT_SZ);
	bpb[13] = 1;
	put16(bpb + 14, RESERVED_SECS);
	bpb[16] = NUM_FATS;
	put16(bpb + 17, ROOT_DIR_ENTRIES);
	put16(bpb + 19, DISK_SECS);
	bpb[21] = 0xf8;
	put16(bpb + 22, SECTORS_PER_FAT);
	bpb[510] = 0x55;
	bpb[511] = 0xaa;

	put16(fat + 0, 0xfff8);
	put16(fat + 2, 0xffff);
}

static uint32_t countFreeClusters(void)
{
	const uint8_t *fat = mDisk + RESERVED_SECS * FATFS_DISK_SECT_SZ;
	uint32_t numClus = DISK_SECS - DATA_SEC;
	uint32_t count = 0, clus;

	for (clus = 2; clus < numClus + 2; clus++) {
		const uint8_t *entry = fat + clus * 2;
		if (!entry[0] && !entry[1])
			count++;
	}
	return count;
}

static bool expect(bool condition, const char *msg)
{
	if (!condition)
		fprintf(stderr, "FAIL: %s\n", msg);
	return condition;
}

static void copyStr(char *dst, uint32_t dstLen, const char *src)
{
	if (!dstLen)
		return;

	while (--dstLen && *src)
		*dst++ = *src++;
	*dst = 0;
}

static const char *baseName(const char *path)
{
	const char *base = path;

	while (*path) {
		if (*path == '/' || *path == '\\')
			base = path + 1;
		path++;
	}
	return base;
}

static void romStemBounds(const char *path, const char **baseP, const char **endP)
{
	const char *base = baseName(path);
	const char *end = base + strlen(base);
	const char *dot;

	for (dot = end; dot != base; dot--) {
		if (dot[-1] == '.') {
			end = dot - 1;
			break;
		}
	}
	*baseP = base;
	*endP = end;
}

static void copyRomStem(char *dst, uint32_t dstLen, const char *path)
{
	const char *base, *end, *src;
	uint32_t pos = 0;

	if (!dstLen)
		return;
	romStemBounds(path, &base, &end);
	for (src = base; src != end && pos + 1 < dstLen; src++)
		dst[pos++] = *src;
	dst[pos] = 0;
}

static void cleanGameTitleFromPath(char *dst, uint32_t dstLen, const char *path, const char *fallbackName)
{
	const char *base, *end, *src;
	uint32_t outLen = 0, parenDepth = 0;
	bool pendingSpace = false;

	if (!dstLen)
		return;
	if (!fallbackName)
		fallbackName = "";

	romStemBounds(path, &base, &end);
	while (base != end && (*base == ' ' || *base == '\t'))
		base++;
	while (end != base && (end[-1] == ' ' || end[-1] == '\t'))
		end--;

	for (src = base; src != end; src++) {
		char ch = *src;

		if (ch == '(') {
			parenDepth++;
			continue;
		}
		if (ch == ')') {
			if (parenDepth)
				parenDepth--;
			continue;
		}
		if (parenDepth)
			continue;

		if (ch == ' ' || ch == '\t') {
			if (outLen)
				pendingSpace = true;
			continue;
		}

		if (pendingSpace) {
			if (outLen + 2 >= dstLen)
				break;
			dst[outLen++] = ' ';
		}
		else if (outLen + 1 >= dstLen)
			break;

		pendingSpace = false;
		dst[outLen++] = ch;
	}

	dst[outLen] = 0;
	if (!outLen)
		copyStr(dst, dstLen, fallbackName);
}

static void makeSaveNameFromStem(const char *stem, char *dst, uint32_t dstLen)
{
	uint32_t pos = 0;

	if (!dstLen)
		return;

	while (*stem && pos + 5 < dstLen)
		dst[pos++] = *stem++;
	if (pos + 4 < dstLen) {
		dst[pos++] = '.';
		dst[pos++] = 's';
		dst[pos++] = 'a';
		dst[pos++] = 'v';
	}
	dst[pos] = 0;
}

static void makeCleanSaveName(const char *romName, char *dst, uint32_t dstLen)
{
	char stem[96];
	char fallbackStem[96];

	copyRomStem(fallbackStem, sizeof(fallbackStem), romName);
	cleanGameTitleFromPath(stem, sizeof(stem), romName, fallbackStem);
	makeSaveNameFromStem(stem, dst, dstLen);
}

static void makeRawSaveName(const char *romName, char *dst, uint32_t dstLen)
{
	char stem[96];

	copyRomStem(stem, sizeof(stem), romName);
	makeSaveNameFromStem(stem, dst, dstLen);
}

static uint8_t saveFallbackChar(char c)
{
	if (c >= 'a' && c <= 'z')
		c += 'A' - 'a';
	if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
		return (uint8_t)c;
	return 0;
}

static void makeFallbackSaveName(const char *romName, char dst[static 13])
{
	static const char hex[] = "0123456789ABCDEF";
	const char *base = romName, *dot = NULL, *src;
	uint32_t hash = 2166136261u, pos = 0;
	uint16_t hash12;

	for (src = romName; *src; src++) {
		if (*src == '/' || *src == '\\')
			base = src + 1;
	}
	for (src = base; *src; src++) {
		if (*src == '.')
			dot = src;
	}
	if (!dot)
		dot = src;

	for (src = base; src != dot; src++) {
		hash ^= (uint8_t)*src;
		hash *= 16777619u;
	}
	hash12 = (uint16_t)((hash ^ (hash >> 12) ^ (hash >> 24)) & 0x0fffu);

	for (src = base; src != dot && pos < 5; src++) {
		uint8_t c = saveFallbackChar(*src);

		if (c)
			dst[pos++] = (char)c;
	}
	while (pos < 5)
		dst[pos++] = 'X';
	dst[pos++] = hex[(hash12 >> 8) & 0x0f];
	dst[pos++] = hex[(hash12 >> 4) & 0x0f];
	dst[pos++] = hex[hash12 & 0x0f];
	dst[pos++] = '.';
	dst[pos++] = 'S';
	dst[pos++] = 'A';
	dst[pos++] = 'V';
	dst[pos] = 0;
}

static bool writePattern(struct FatfsFil *fil, uint32_t size, uint8_t seed)
{
	uint8_t buf[1536];
	uint32_t i, written;

	for (i = 0; i < size; i++)
		buf[i] = (uint8_t)(seed + i * 17);
	return fatfsFileWrite(fil, buf, size, &written) && written == size;
}

static bool verifyPattern(struct FatfsFil *fil, uint32_t size, uint8_t seed)
{
	uint8_t buf[1536];
	uint32_t i, numRead;

	if (!fatfsFileRead(fil, buf, size, &numRead) || numRead != size)
		return false;
	for (i = 0; i < size; i++)
		if (buf[i] != (uint8_t)(seed + i * 17))
			return false;
	return true;
}

static void fillGeneratedSaveChunk(uint8_t *buf, uint32_t pos, uint32_t size, uint8_t seed)
{
	uint32_t i;

	for (i = 0; i < size; i++)
		buf[i] = (uint8_t)(seed + (pos + i) * 13 + ((pos + i) >> 8));
}

static bool writeGeneratedSave(struct FatfsFil *fil, uint32_t size, uint8_t seed)
{
	uint8_t buf[512];
	uint32_t pos = 0;

	while (pos < size) {
		uint32_t now = size - pos, written = 0;

		if (now > sizeof(buf))
			now = sizeof(buf);
		fillGeneratedSaveChunk(buf, pos, now, seed);
		if (!fatfsFileWrite(fil, buf, now, &written) || written != now)
			return false;
		pos += now;
	}
	return true;
}

static bool verifyGeneratedSaveFile(struct FatfsVol *vol, const char *path, uint32_t size, uint8_t seed)
{
	struct FatfsFil *fil;
	uint8_t expected[512], actual[512];
	uint32_t pos = 0;
	bool ok = true;

	fil = fatfsFileOpen(vol, path, OPEN_MODE_READ);
	if (!expect(fil != NULL, "open generated save file"))
		return false;
	ok = expect(fatfsFileGetSize(fil) == size, "generated save file size matches") && ok;

	while (ok && pos < size) {
		uint32_t now = size - pos, numRead = 0;

		if (now > sizeof(actual))
			now = sizeof(actual);
		fillGeneratedSaveChunk(expected, pos, now, seed);
		ok = expect(fatfsFileRead(fil, actual, now, &numRead) && numRead == now, "read generated save chunk") && ok;
		ok = expect(!memcmp(actual, expected, now), "generated save chunk matches") && ok;
		pos += now;
	}

	ok = expect(fatfsFileClose(fil), "close generated save file") && ok;
	return ok;
}

static void fillGeneratedSaveBuffer(uint8_t *buf, uint32_t size, uint8_t seed)
{
	uint32_t pos = 0;

	while (pos < size) {
		uint32_t now = size - pos;

		if (now > 512)
			now = 512;
		fillGeneratedSaveChunk(buf + pos, pos, now, seed);
		pos += now;
	}
}

static bool verifyGeneratedSaveBuffer(const uint8_t *buf, uint32_t size, uint8_t seed, const char *msg)
{
	uint8_t expected[512];
	uint32_t pos = 0;

	while (pos < size) {
		uint32_t now = size - pos;

		if (now > sizeof(expected))
			now = sizeof(expected);
		fillGeneratedSaveChunk(expected, pos, now, seed);
		if (memcmp(buf + pos, expected, now))
			return expect(false, msg);
		pos += now;
	}
	return expect(true, msg);
}

static struct FatfsDir *openGeneratedSaveDir(struct FatfsVol *vol, bool create)
{
	struct FatfsDir *dir;
	struct FatFileLocator loc;

	dir = fatfsDirOpen(vol, TEST_SAVE_DIR);
	if (dir || !create)
		return dir;

	dir = fatfsDirOpen(vol, "/SAVE");
	if (!dir) {
		if (!fatfsDirCreate(vol, "/SAVE", &loc))
			return NULL;
		dir = fatfsDirOpenWithLocator(vol, &loc);
	}
	if (!dir)
		return NULL;
	(void)fatfsDirClose(dir);

	if (fatfsDirCreate(vol, TEST_SAVE_DIR, &loc))
		return fatfsDirOpenWithLocator(vol, &loc);
	return fatfsDirOpen(vol, TEST_SAVE_DIR);
}

static struct FatfsFil* openGeneratedSaveForExport(struct FatfsVol *vol, struct FatfsDir *saveDir, const char *name,
	const char *fallbackName, bool simulatePrimaryCreateFailure, char *chosenName, uint32_t chosenNameLen)
{
	struct FatFileLocator loc;
	struct FatfsFil *fil;

	if (fatfsFindFileAt(saveDir, name, &loc)) {
		copyStr(chosenName, chosenNameLen, name);
		return fatfsFileOpenWithLocator(vol, &loc, OPEN_MODE_WRITE);
	}

	if (!simulatePrimaryCreateFailure) {
		if (fatfsFileCreateAt(saveDir, name, &loc)) {
			copyStr(chosenName, chosenNameLen, name);
			return fatfsFileOpenWithLocator(vol, &loc, OPEN_MODE_WRITE);
		}
	}

	if (!fallbackName || !fallbackName[0])
		return NULL;

	if (fatfsFindFileAt(saveDir, fallbackName, &loc)) {
		copyStr(chosenName, chosenNameLen, fallbackName);
		return fatfsFileOpenWithLocator(vol, &loc, OPEN_MODE_WRITE);
	}

	if (!fatfsFileCreateAt(saveDir, fallbackName, &loc))
		return NULL;

	copyStr(chosenName, chosenNameLen, fallbackName);
	fil = fatfsFileOpenWithLocator(vol, &loc, OPEN_MODE_WRITE);
	return fil;
}

static bool remountAndVerifyGeneratedSave(struct FatfsVol **volP, const char *path, uint32_t size, uint8_t seed)
{
	struct FatfsVol *vol;
	bool ok = true;

	ok = expect(fatfsUnmount(*volP), "unmount after generated save export") && ok;
	*volP = NULL;
	vol = fatfsMount(diskRead, diskWrite, NULL);
	if (!expect(vol != NULL, "remount after generated save export"))
		return false;
	*volP = vol;
	ok = verifyGeneratedSaveFile(vol, path, size, seed) && ok;
	return ok;
}

static struct FatfsFil *openGeneratedSaveIfSizeMatches(struct FatfsDir *saveDir, const char *name, uint32_t size,
	char *chosenName, uint32_t chosenNameLen, bool *badSizeP)
{
	struct FatfsFil *fil = fatfsFileOpenAt(saveDir, name, OPEN_MODE_READ);

	if (!fil)
		return NULL;
	if (fatfsFileGetSize(fil) == size) {
		copyStr(chosenName, chosenNameLen, name);
		return fil;
	}

	if (badSizeP)
		*badSizeP = true;
	fatfsFileClose(fil);
	return NULL;
}

static bool verifyGeneratedSaveLoadLookup(struct FatfsVol *vol, const char *cleanName, const char *fullName,
	const char *fallbackName, const char *legacyName, const char *expectedName, uint32_t size, bool expectBadSize)
{
	struct FatfsDir *saveDir;
	struct FatfsFil *fil = NULL;
	char chosenName[96] = {};
	bool badSize = false;
	bool ok = true;

	(void)legacyName;
	saveDir = openGeneratedSaveDir(vol, false);
	if (!expect(saveDir != NULL, "open console save folder for load lookup"))
		return false;

	fil = openGeneratedSaveIfSizeMatches(saveDir, fullName, size, chosenName, sizeof(chosenName), &badSize);
	if (!fil && cleanName && cleanName[0] && strcmp(cleanName, fullName))
		fil = openGeneratedSaveIfSizeMatches(saveDir, cleanName, size, chosenName, sizeof(chosenName), &badSize);
	if (!fil && fallbackName && fallbackName[0] && strcmp(fullName, fallbackName) && (!cleanName || strcmp(cleanName, fallbackName)))
		fil = openGeneratedSaveIfSizeMatches(saveDir, fallbackName, size, chosenName, sizeof(chosenName), &badSize);

	if (expectedName) {
		ok = expect(fil != NULL, "load lookup found save") && ok;
		ok = expect(!strcmp(chosenName, expectedName), "load lookup chose expected save name") && ok;
	}
	else {
		ok = expect(fil == NULL, "load lookup rejected save") && ok;
		ok = expect(badSize == expectBadSize, "load lookup reported expected bad-size state") && ok;
	}
	if (fil)
		ok = expect(fatfsFileClose(fil), "close load lookup save file") && ok;
	ok = expect(fatfsDirClose(saveDir), "close console save folder after load lookup") && ok;
	return ok;
}

static bool importGeneratedSaveByLookup(struct FatfsVol *vol, const char *cleanName, const char *fullName,
	const char *fallbackName, const char *legacyName, const char *expectedName, uint32_t size, uint8_t seed,
	uint8_t *qspi, uint8_t *cart, enum SaveNameKind *kindP)
{
	struct FatfsDir *saveDir;
	struct FatfsFil *fil = NULL;
	char chosenName[96] = {};
	uint32_t numRead = 0;
	bool badSize = false;
	bool ok = true;

	(void)legacyName;
	saveDir = openGeneratedSaveDir(vol, false);
	if (!expect(saveDir != NULL, "open console save folder for lifecycle import"))
		return false;

	fil = openGeneratedSaveIfSizeMatches(saveDir, fullName, size, chosenName, sizeof(chosenName), &badSize);
	if (fil) {
		if (kindP)
			*kindP = SaveNameKindFull;
	}
	if (!fil && cleanName && cleanName[0] && strcmp(cleanName, fullName)) {
		fil = openGeneratedSaveIfSizeMatches(saveDir, cleanName, size, chosenName, sizeof(chosenName), &badSize);
		if (fil && kindP)
			*kindP = SaveNameKindClean;
	}
	if (!fil && fallbackName && fallbackName[0] && strcmp(fullName, fallbackName) && (!cleanName || strcmp(cleanName, fallbackName))) {
		fil = openGeneratedSaveIfSizeMatches(saveDir, fallbackName, size, chosenName, sizeof(chosenName), &badSize);
		if (fil && kindP)
			*kindP = SaveNameKindFallback;
	}

	ok = expect(fil != NULL, "lifecycle import found save") && ok;
	ok = expect(!badSize, "lifecycle import did not see bad-size save") && ok;
	ok = expect(!strcmp(chosenName, expectedName), "lifecycle import chose expected save") && ok;
	if (fil) {
		memset(qspi, 0xff, TEST_SAVE_MAX_SIZE);
		memset(cart, 0xff, TEST_SAVE_MAX_SIZE);
		ok = expect(fatfsFileRead(fil, qspi, size, &numRead) && numRead == size, "lifecycle import read save bytes") && ok;
		memcpy(cart, qspi, size);
		ok = verifyGeneratedSaveBuffer(qspi, size, seed, "lifecycle QSPI cache matches imported save") && ok;
		ok = expect(!memcmp(qspi, cart, size), "lifecycle cart RAM hydrated from QSPI") && ok;
		ok = expect(fatfsFileClose(fil), "close lifecycle import save") && ok;
	}
	ok = expect(fatfsDirClose(saveDir), "close console save folder after lifecycle import") && ok;
	return ok;
}

static bool exportGeneratedSave(struct FatfsVol **volP, const char *name, const char *fallbackName,
	bool simulatePrimaryCreateFailure, uint32_t size, uint8_t seed, char *chosenNameOut, uint32_t chosenNameOutLen)
{
	struct FatfsVol *vol = *volP;
	struct FatfsDir *saveDir;
	struct FatfsFil *fil;
	char chosenName[96];
	char path[128];
	bool ok = true;

	saveDir = openGeneratedSaveDir(vol, true);
	if (!expect(saveDir != NULL, "open console save directory"))
		return false;

	fil = openGeneratedSaveForExport(vol, saveDir, name, fallbackName, simulatePrimaryCreateFailure, chosenName, sizeof(chosenName));
	if (!expect(fil != NULL, "open generated save for export")) {
		fatfsDirClose(saveDir);
		return false;
	}
	if (chosenNameOut)
		copyStr(chosenNameOut, chosenNameOutLen, chosenName);
	(void)sprintf(path, "%s/%s", TEST_SAVE_DIR, chosenName);
	ok = expect(writeGeneratedSave(fil, size, seed), "write generated save") && ok;
	ok = expect(fatfsFileTruncate(fil, size), "truncate generated save to exact size") && ok;
	ok = expect(fatfsFileClose(fil), "close generated save export") && ok;
	ok = expect(fatfsDirClose(saveDir), "close console save directory") && ok;
	ok = remountAndVerifyGeneratedSave(volP, path, size, seed) && ok;
	return ok;
}

static bool generatedSaveNameExists(struct FatfsDir *saveDir, const char *name)
{
	struct FatFileLocator loc;

	return name && name[0] && fatfsFindFileAt(saveDir, name, &loc);
}

static void chooseGeneratedSaveExportName(struct FatfsDir *saveDir, const char *cleanName, const char *fullName,
	const char *fallbackName, const char *legacyName, enum SaveNameKind preferredKind, char *chosenName, uint32_t chosenNameLen)
{
	(void)legacyName;
	if (generatedSaveNameExists(saveDir, fullName)) {
		copyStr(chosenName, chosenNameLen, fullName);
		return;
	}

	switch (preferredKind) {
		case SaveNameKindClean:
			if (generatedSaveNameExists(saveDir, cleanName)) {
				copyStr(chosenName, chosenNameLen, cleanName);
				return;
			}
			break;

		case SaveNameKindFallback:
			if (generatedSaveNameExists(saveDir, fallbackName)) {
				copyStr(chosenName, chosenNameLen, fallbackName);
				return;
			}
			break;
		case SaveNameKindAuto:
			if (generatedSaveNameExists(saveDir, cleanName)) {
				copyStr(chosenName, chosenNameLen, cleanName);
				return;
			}
			if (generatedSaveNameExists(saveDir, fallbackName)) {
				copyStr(chosenName, chosenNameLen, fallbackName);
				return;
			}
			break;
		case SaveNameKindFull:
		default:
			break;
	}

	if (preferredKind != SaveNameKindFull) {
		if (generatedSaveNameExists(saveDir, cleanName)) {
			copyStr(chosenName, chosenNameLen, cleanName);
			return;
		}
		if (generatedSaveNameExists(saveDir, fallbackName)) {
			copyStr(chosenName, chosenNameLen, fallbackName);
			return;
		}
	}

	copyStr(chosenName, chosenNameLen, fullName);
}

static bool exportGeneratedSaveResolved(struct FatfsVol **volP, const char *cleanName, const char *fullName,
	const char *fallbackName, const char *legacyName, enum SaveNameKind preferredKind, bool simulatePrimaryCreateFailure,
	uint32_t size, uint8_t seed, char *chosenNameOut, uint32_t chosenNameOutLen)
{
	struct FatfsVol *vol = *volP;
	struct FatfsDir *saveDir;
	struct FatfsFil *fil;
	char targetName[96];
	char chosenName[96];
	char path[128];
	bool ok = true;

	saveDir = openGeneratedSaveDir(vol, true);
	if (!expect(saveDir != NULL, "open console save directory for resolved export"))
		return false;

	chooseGeneratedSaveExportName(saveDir, cleanName, fullName, fallbackName, legacyName, preferredKind, targetName, sizeof(targetName));
	fil = openGeneratedSaveForExport(vol, saveDir, targetName, fallbackName, simulatePrimaryCreateFailure, chosenName, sizeof(chosenName));
	if (!expect(fil != NULL, "open resolved generated save for export")) {
		fatfsDirClose(saveDir);
		return false;
	}
	if (chosenNameOut)
		copyStr(chosenNameOut, chosenNameOutLen, chosenName);
	(void)sprintf(path, "%s/%s", TEST_SAVE_DIR, chosenName);
	ok = expect(writeGeneratedSave(fil, size, seed), "write resolved generated save") && ok;
	ok = expect(fatfsFileTruncate(fil, size), "truncate resolved generated save to exact size") && ok;
	ok = expect(fatfsFileClose(fil), "close resolved generated save export") && ok;
	ok = expect(fatfsDirClose(saveDir), "close console save directory after resolved export") && ok;
	ok = remountAndVerifyGeneratedSave(volP, path, size, seed) && ok;
	return ok;
}

static bool lifecycleExportGeneratedSave(struct FatfsVol **volP, const char *cleanName, const char *fullName,
	const char *fallbackName, const char *legacyName, enum SaveNameKind preferredKind, uint32_t size,
	uint8_t qspiSeed, uint8_t cartSeed, bool cartOwnerValid, uint8_t *qspi, uint8_t *cart,
	uint8_t *exportedSeedP, char *chosenNameOut, uint32_t chosenNameOutLen)
{
	uint8_t exportedSeed = qspiSeed;
	bool ok;

	if (cartOwnerValid) {
		if (!verifyGeneratedSaveBuffer(cart, size, cartSeed, "lifecycle owned cart RAM matches expected save"))
			return false;
		memcpy(qspi, cart, size);
		exportedSeed = cartSeed;
	}
	else
		fillGeneratedSaveBuffer(qspi, size, qspiSeed);

	ok = verifyGeneratedSaveBuffer(qspi, size, exportedSeed, "lifecycle QSPI cache selected for export") &&
		exportGeneratedSaveResolved(volP, cleanName, fullName, fallbackName, legacyName, preferredKind, false,
			size, exportedSeed, chosenNameOut, chosenNameOutLen);
	if (exportedSeedP)
		*exportedSeedP = exportedSeed;
	return ok;
}

static bool testTruncateOpenRewrite(struct FatfsVol *vol)
{
	struct FatfsFil *fil;
	bool ok = true;

	fil = fatfsFileOpen(vol, "/TRUNC.BIN", OPEN_MODE_WRITE | OPEN_MODE_CREATE);
	if (!expect(fil != NULL, "open initial truncate test file"))
		return false;
	ok = expect(writePattern(fil, 1500, 3), "create initial truncate test file") && ok;
	ok = expect(fatfsFileClose(fil), "close initial truncate test file") && ok;

	fil = fatfsFileOpen(vol, "/TRUNC.BIN", OPEN_MODE_WRITE | OPEN_MODE_TRUNCATE);
	if (!expect(fil != NULL, "open with truncate"))
		return false;
	ok = expect(fatfsFileGetSize(fil) == 0, "open with truncate resets size") && ok;
	ok = expect(writePattern(fil, 333, 9), "write after open with truncate") && ok;
	ok = expect(fatfsFileClose(fil), "close rewritten truncate test file") && ok;

	fil = fatfsFileOpen(vol, "/TRUNC.BIN", OPEN_MODE_READ);
	if (!expect(fil != NULL, "open rewritten truncate test file"))
		return false;
	ok = expect(fatfsFileGetSize(fil) == 333, "rewritten size is persisted") && ok;
	ok = expect(verifyPattern(fil, 333, 9), "rewritten contents are persisted") && ok;
	ok = expect(fatfsFileClose(fil), "close rewritten read handle") && ok;
	return ok;
}

static bool testReadOnlyGuards(struct FatfsVol *vol)
{
	struct FatfsFil *fil = fatfsFileOpen(vol, "/TRUNC.BIN", OPEN_MODE_READ);
	uint8_t byte = 0xaa;
	uint32_t written = 1234;
	bool ok = true;

	if (!expect(fil != NULL, "open read-only guard test file"))
		return false;
	ok = expect(!fatfsFileWrite(fil, &byte, 1, &written) && written == 0, "read-only write is rejected") && ok;
	ok = expect(!fatfsFileTruncate(fil, 0), "read-only truncate is rejected") && ok;
	ok = expect(fatfsFileClose(fil), "close read-only guard test file") && ok;
	return ok;
}

static bool testTruncateZeroFreesClusters(struct FatfsVol *vol)
{
	struct FatfsFil *fil;
	uint32_t freeBefore = countFreeClusters(), freeAfterWrite, freeAfterTruncate, freeAfterReuse;
	bool ok = true;

	fil = fatfsFileOpen(vol, "/REUSE.BIN", OPEN_MODE_WRITE | OPEN_MODE_CREATE);
	if (!expect(fil != NULL, "open cluster reuse test file"))
		return false;
	ok = expect(writePattern(fil, 1536, 42), "create cluster reuse test file") && ok;
	ok = expect(fatfsFileClose(fil), "close cluster reuse test file") && ok;
	freeAfterWrite = countFreeClusters();
	ok = expect(freeAfterWrite + 3 == freeBefore, "write consumes expected clusters") && ok;

	fil = fatfsFileOpen(vol, "/REUSE.BIN", OPEN_MODE_WRITE);
	if (!expect(fil != NULL, "open cluster reuse test file for truncate"))
		return false;
	ok = expect(fatfsFileTruncate(fil, 0), "truncate to zero succeeds") && ok;
	ok = expect(fatfsFileClose(fil), "close zero-truncated test file") && ok;
	freeAfterTruncate = countFreeClusters();
	ok = expect(freeAfterTruncate == freeBefore, "truncate to zero frees clusters") && ok;

	fil = fatfsFileOpen(vol, "/REUSE.BIN", OPEN_MODE_WRITE);
	if (!expect(fil != NULL, "open zero-truncated test file for rewrite"))
		return false;
	ok = expect(writePattern(fil, 512, 7), "rewrite zero-truncated file") && ok;
	ok = expect(fatfsFileClose(fil), "close rewritten zero-truncated file") && ok;
	freeAfterReuse = countFreeClusters();
	ok = expect(freeAfterReuse + 1 == freeBefore, "freed clusters are reusable") && ok;
	return ok;
}

static bool testSaveExportRewriteVerify(struct FatfsVol **volP)
{
	char fallbackName[13];
	char cleanName[96];
	char fullName[96];
	char chosenName[96];

	makeCleanSaveName(TETRIS_ROM_NAME, cleanName, sizeof(cleanName));
	makeRawSaveName(TETRIS_ROM_NAME, fullName, sizeof(fullName));
	if (!expect(!strcmp(cleanName, TETRIS_SAVE_NAME), "cleaned save name for Tetris DX"))
		return false;
	if (!expect(!strcmp(fullName, TETRIS_FULL_SAVE_NAME), "old full save name for Tetris DX"))
		return false;
	makeFallbackSaveName(TETRIS_ROM_NAME, fallbackName);
	if (!expect(!strcmp(fallbackName, "TETRI933.SAV"), "deterministic fallback name for Tetris DX"))
		return false;
	if (!expect(exportGeneratedSaveResolved(volP, cleanName, fullName, fallbackName, TETRIS_ROM_NAME, SaveNameKindFull, true, 16384, 55, chosenName, sizeof(chosenName)), "fallback generated save export when primary create fails"))
		return false;
	if (!expect(!strcmp(chosenName, fallbackName), "simulated primary create failure uses fallback name"))
		return false;
	if (!expect(verifyGeneratedSaveLoadLookup(*volP, cleanName, fullName, fallbackName, TETRIS_ROM_NAME, fallbackName, 16384, false), "load lookup falls back to 8.3 save"))
		return false;
	if (!expect(exportGeneratedSaveResolved(volP, cleanName, fullName, fallbackName, TETRIS_ROM_NAME, SaveNameKindFull, false, 16384, 66, chosenName, sizeof(chosenName)), "primary full-name generated save export"))
		return false;
	if (!expect(!strcmp(chosenName, TETRIS_FULL_SAVE_NAME), "primary full-name save is preferred when creatable"))
		return false;
	if (!expect(verifyGeneratedSaveLoadLookup(*volP, cleanName, fullName, fallbackName, TETRIS_ROM_NAME, TETRIS_FULL_SAVE_NAME, 16384, false), "load lookup prefers primary full-name save"))
		return false;
	if (!expect(exportGeneratedSave(volP, "Pokemon Blue.sav", NULL, false, 32768, 11, NULL, 0), "create missing generated save export"))
		return false;
	if (!expect(exportGeneratedSave(volP, "Pokemon Red.sav", NULL, false, 8192, 33, NULL, 0), "create missing generated save in existing console save folder"))
		return false;
	if (!expect(exportGeneratedSave(volP, "Pokemon Blue.sav", NULL, false, 8192, 77, NULL, 0), "rewrite, shrink, and verify generated save export"))
		return false;
	if (!expect(exportGeneratedSave(volP, "Pokemon Blue.sav", NULL, false, 32768, 91, NULL, 0), "rewrite, extend, and verify generated save export"))
		return false;
	return true;
}

static bool testSaveSwitchLifecycle(struct FatfsVol **volP)
{
	static uint8_t qspi[TEST_SAVE_MAX_SIZE];
	static uint8_t cart[TEST_SAVE_MAX_SIZE];
	char crystalClean[96];
	char crystalFull[96];
	char crystalFallback[13];
	char bubbleClean[96];
	char bubbleFull[96];
	char bubbleFallback[13];
	char chosenName[96];
	enum SaveNameKind selectedKind = SaveNameKindAuto;
	uint8_t crystalSeed = 0, bubbleSeed = 0, exportedSeed = 0;

	makeCleanSaveName(CRYSTAL_ROM_NAME, crystalClean, sizeof(crystalClean));
	makeRawSaveName(CRYSTAL_ROM_NAME, crystalFull, sizeof(crystalFull));
	makeFallbackSaveName(CRYSTAL_ROM_NAME, crystalFallback);
	makeCleanSaveName(BUBBLE_ROM_NAME, bubbleClean, sizeof(bubbleClean));
	makeRawSaveName(BUBBLE_ROM_NAME, bubbleFull, sizeof(bubbleFull));
	makeFallbackSaveName(BUBBLE_ROM_NAME, bubbleFallback);

	if (!expect(!strcmp(crystalClean, CRYSTAL_SAVE_NAME), "lifecycle Crystal cleaned save name"))
		return false;
	if (!expect(!strcmp(crystalFallback, CRYSTAL_FALLBACK_SAVE_NAME), "lifecycle Crystal fallback save name"))
		return false;
	if (!expect(!strcmp(bubbleClean, BUBBLE_SAVE_NAME), "lifecycle Bubble cleaned save name"))
		return false;
	if (!expect(!strcmp(bubbleFull, BUBBLE_FULL_SAVE_NAME), "lifecycle Bubble full save name"))
		return false;

	if (!expect(exportGeneratedSave(volP, crystalFallback, crystalFallback, false, TEST_SAVE_MAX_SIZE, 21, chosenName, sizeof(chosenName)), "lifecycle create Crystal fallback-only save"))
		return false;
	if (!expect(!strcmp(chosenName, crystalFallback), "lifecycle Crystal starts with fallback only"))
		return false;

	crystalSeed = 21;
	if (!expect(importGeneratedSaveByLookup(*volP, crystalClean, crystalFull, crystalFallback, CRYSTAL_ROM_NAME,
		crystalFallback, TEST_SAVE_MAX_SIZE, crystalSeed, qspi, cart, &selectedKind), "lifecycle import Crystal fallback save"))
		return false;
	if (!expect(selectedKind == SaveNameKindFallback, "lifecycle Crystal source is fallback"))
		return false;

	fillGeneratedSaveBuffer(cart, TEST_SAVE_MAX_SIZE, 44);
	if (!expect(lifecycleExportGeneratedSave(volP, crystalClean, crystalFull, crystalFallback, CRYSTAL_ROM_NAME,
		selectedKind, TEST_SAVE_MAX_SIZE, crystalSeed, 44, true, qspi, cart, &exportedSeed, chosenName, sizeof(chosenName)),
		"lifecycle export played Crystal save"))
		return false;
	crystalSeed = exportedSeed;
	if (!expect(crystalSeed == 44, "lifecycle Crystal export uses owned cart RAM"))
		return false;
	if (!expect(!strcmp(chosenName, crystalFallback), "lifecycle fallback-imported Crystal exports back to fallback"))
		return false;
	if (!expect(verifyGeneratedSaveFile(*volP, TEST_SAVE_DIR "/POKEM987.SAV", TEST_SAVE_MAX_SIZE, crystalSeed), "lifecycle select-game auto-export persisted Crystal before loading Bubble"))
		return false;

	fillGeneratedSaveBuffer(cart, TEST_SAVE_MAX_SIZE, 0xe1);
	if (!expect(lifecycleExportGeneratedSave(volP, crystalClean, crystalFull, crystalFallback, CRYSTAL_ROM_NAME,
		selectedKind, TEST_SAVE_MAX_SIZE, crystalSeed, 0xe1, false, qspi, cart, &exportedSeed, chosenName, sizeof(chosenName)),
		"lifecycle picker-corrupted cart RAM does not overwrite Crystal fallback"))
		return false;
	if (!expect(exportedSeed == crystalSeed, "lifecycle stale cart RAM keeps Crystal QSPI cache"))
		return false;

	if (!expect(exportGeneratedSaveResolved(volP, bubbleClean, bubbleFull, bubbleFallback, BUBBLE_ROM_NAME,
		SaveNameKindFull, false, BUBBLE_SAVE_SIZE, 7, chosenName, sizeof(chosenName)), "lifecycle create Bubble full-name save"))
		return false;
	if (!expect(!strcmp(chosenName, bubbleFull), "lifecycle Bubble uses full-name primary"))
		return false;
	bubbleSeed = 7;
	if (!expect(importGeneratedSaveByLookup(*volP, bubbleClean, bubbleFull, bubbleFallback, BUBBLE_ROM_NAME,
		bubbleFull, BUBBLE_SAVE_SIZE, bubbleSeed, qspi, cart, &selectedKind), "lifecycle import Bubble save"))
		return false;
	if (!expect(selectedKind == SaveNameKindFull, "lifecycle Bubble source is full-name primary"))
		return false;

	fillGeneratedSaveBuffer(cart, BUBBLE_SAVE_SIZE, 66);
	if (!expect(lifecycleExportGeneratedSave(volP, bubbleClean, bubbleFull, bubbleFallback, BUBBLE_ROM_NAME,
		selectedKind, BUBBLE_SAVE_SIZE, bubbleSeed, 66, true, qspi, cart, &exportedSeed, chosenName, sizeof(chosenName)),
		"lifecycle export played Bubble save"))
		return false;
	bubbleSeed = exportedSeed;
	if (!expect(!strcmp(chosenName, bubbleFull), "lifecycle Bubble exports to full-name primary"))
		return false;
	if (!expect(verifyGeneratedSaveFile(*volP, TEST_SAVE_DIR "/Bubble Bobble (USA, Europe).sav", BUBBLE_SAVE_SIZE, bubbleSeed), "lifecycle select-game auto-export persisted Bubble before switching back"))
		return false;

	fillGeneratedSaveBuffer(cart, TEST_SAVE_MAX_SIZE, 0xd2);
	if (!expect(importGeneratedSaveByLookup(*volP, crystalClean, crystalFull, crystalFallback, CRYSTAL_ROM_NAME,
		crystalFallback, TEST_SAVE_MAX_SIZE, crystalSeed, qspi, cart, &selectedKind), "lifecycle re-import Crystal after Bubble"))
		return false;
	if (!expect(selectedKind == SaveNameKindFallback, "lifecycle Crystal still imports fallback after switching back"))
		return false;
	if (!expect(verifyGeneratedSaveFile(*volP, TEST_SAVE_DIR "/POKEM987.SAV", TEST_SAVE_MAX_SIZE, crystalSeed), "lifecycle Crystal fallback file survived switch"))
		return false;

	fillGeneratedSaveBuffer(cart, TEST_SAVE_MAX_SIZE, 55);
	if (!expect(lifecycleExportGeneratedSave(volP, crystalClean, crystalFull, crystalFallback, CRYSTAL_ROM_NAME,
		selectedKind, TEST_SAVE_MAX_SIZE, crystalSeed, 55, true, qspi, cart, &exportedSeed, chosenName, sizeof(chosenName)),
		"lifecycle browser-open pre-export updates Crystal fallback"))
		return false;
	crystalSeed = exportedSeed;
	fillGeneratedSaveBuffer(cart, TEST_SAVE_MAX_SIZE, 0xc3);
	if (!expect(lifecycleExportGeneratedSave(volP, crystalClean, crystalFull, crystalFallback, CRYSTAL_ROM_NAME,
		selectedKind, TEST_SAVE_MAX_SIZE, crystalSeed, 0xc3, false, qspi, cart, &exportedSeed, chosenName, sizeof(chosenName)),
		"lifecycle browser-corrupted cart RAM keeps pre-exported Crystal fallback"))
		return false;
	if (!expect(exportedSeed == crystalSeed, "lifecycle direct browser path exports cached save when cart owner is stale"))
		return false;
	if (!expect(verifyGeneratedSaveFile(*volP, TEST_SAVE_DIR "/POKEM987.SAV", TEST_SAVE_MAX_SIZE, crystalSeed), "lifecycle direct browser path leaves Crystal fallback valid"))
		return false;

	return true;
}

static bool testCrystalSaveNamingAndCompatibility(struct FatfsVol **volP)
{
	char cleanName[96];
	char fullName[96];
	char fallbackName[13];
	char wrongCleanName[96];
	char wrongFullName[96];
	char wrongFallbackName[13];
	char chosenName[96];

	makeCleanSaveName(CRYSTAL_ROM_NAME, cleanName, sizeof(cleanName));
	makeRawSaveName(CRYSTAL_ROM_NAME, fullName, sizeof(fullName));
	makeFallbackSaveName(CRYSTAL_ROM_NAME, fallbackName);
	if (!expect(!strcmp(cleanName, CRYSTAL_SAVE_NAME), "Crystal cleaned save name"))
		return false;
	if (!expect(!strcmp(fullName, CRYSTAL_FULL_SAVE_NAME), "Crystal old full save name"))
		return false;
	if (!expect(!strcmp(fallbackName, CRYSTAL_FALLBACK_SAVE_NAME), "Crystal deterministic fallback save name"))
		return false;

	if (!expect(exportGeneratedSave(volP, fallbackName, fallbackName, false, 32768, 21, chosenName, sizeof(chosenName)), "create Crystal fallback-only save"))
		return false;
	if (!expect(!strcmp(chosenName, fallbackName), "fallback-only Crystal save uses fallback name"))
		return false;
	if (!expect(verifyGeneratedSaveLoadLookup(*volP, cleanName, fullName, fallbackName, CRYSTAL_ROM_NAME, fallbackName, 32768, false), "fallback-only Crystal save imports"))
		return false;
	if (!expect(exportGeneratedSaveResolved(volP, cleanName, fullName, fallbackName, CRYSTAL_ROM_NAME, SaveNameKindFallback, false, 32768, 44, chosenName, sizeof(chosenName)), "fallback-imported Crystal save exports back to fallback"))
		return false;
	if (!expect(!strcmp(chosenName, fallbackName), "fallback-imported Crystal save chooses fallback on export"))
		return false;

	if (!expect(exportGeneratedSave(volP, cleanName, fallbackName, false, 32768, 88, chosenName, sizeof(chosenName)), "create Crystal cleaned compatibility save"))
		return false;
	if (!expect(!strcmp(chosenName, cleanName), "Crystal cleaned compatibility save is created"))
		return false;
	if (!expect(verifyGeneratedSaveLoadLookup(*volP, cleanName, fullName, fallbackName, CRYSTAL_ROM_NAME, cleanName, 32768, false), "Crystal cleaned compatibility save wins over fallback when full name is absent"))
		return false;
	if (!expect(exportGeneratedSaveResolved(volP, cleanName, fullName, fallbackName, CRYSTAL_ROM_NAME, SaveNameKindClean, false, 32768, 99, chosenName, sizeof(chosenName)), "Crystal cleaned imported save exports back to cleaned name"))
		return false;
	if (!expect(!strcmp(chosenName, cleanName), "Crystal export keeps cleaned compatibility target when selected"))
		return false;

	makeCleanSaveName(WRONG_SIZE_ROM_NAME, wrongCleanName, sizeof(wrongCleanName));
	makeRawSaveName(WRONG_SIZE_ROM_NAME, wrongFullName, sizeof(wrongFullName));
	makeFallbackSaveName(WRONG_SIZE_ROM_NAME, wrongFallbackName);
	if (!expect(exportGeneratedSave(volP, wrongFullName, NULL, false, 1024, 12, NULL, 0), "create wrong-size save fixture"))
		return false;
	if (!expect(verifyGeneratedSaveLoadLookup(*volP, wrongCleanName, wrongFullName, wrongFallbackName, WRONG_SIZE_ROM_NAME, NULL, 32768, true), "wrong-size save is rejected"))
		return false;

	return true;
}

int main(void)
{
	struct FatfsVol *vol;
	bool ok = true;

	initFat16Disk();
	vol = fatfsMount(diskRead, diskWrite, NULL);
	if (!vol) {
		fprintf(stderr, "FAIL: mount in-memory FAT16 image\n");
		return 1;
	}

	ok = testTruncateOpenRewrite(vol) && ok;
	ok = testReadOnlyGuards(vol) && ok;
	ok = testTruncateZeroFreesClusters(vol) && ok;
	ok = testSaveExportRewriteVerify(&vol) && ok;
	ok = testSaveSwitchLifecycle(&vol) && ok;
	ok = testCrystalSaveNamingAndCompatibility(&vol) && ok;
	if (vol)
		ok = expect(fatfsUnmount(vol), "unmount in-memory FAT16 image") && ok;
	else
		ok = false;

	vol = fatfsMount(diskRead, diskWrite, NULL);
	if (!vol) {
		fprintf(stderr, "FAIL: remount in-memory FAT16 image\n");
		return 1;
	}
	ok = verifyGeneratedSaveFile(vol, TEST_SAVE_DIR "/Pokemon Blue.sav", 32768, 91) && ok;
	ok = verifyGeneratedSaveFile(vol, TEST_SAVE_DIR "/Pokemon Red.sav", 8192, 33) && ok;
	ok = verifyGeneratedSaveFile(vol, TEST_SAVE_DIR "/Tetris DX (World) (SGB Enhanced).sav", 16384, 66) && ok;
	ok = verifyGeneratedSaveFile(vol, TEST_SAVE_DIR "/TETRI933.SAV", 16384, 55) && ok;
	ok = verifyGeneratedSaveFile(vol, TEST_SAVE_DIR "/Bubble Bobble (USA, Europe).sav", BUBBLE_SAVE_SIZE, 66) && ok;
	ok = verifyGeneratedSaveFile(vol, TEST_SAVE_DIR "/Pokemon - Crystal Version.sav", 32768, 99) && ok;
	ok = verifyGeneratedSaveFile(vol, TEST_SAVE_DIR "/POKEM987.SAV", 32768, 44) && ok;
	ok = verifyGeneratedSaveFile(vol, TEST_SAVE_DIR "/Pokemon - Crystal Version Wrong Size (USA, Europe).sav", 1024, 12) && ok;
	ok = expect(fatfsUnmount(vol), "unmount remounted in-memory FAT16 image") && ok;

	if (!ok)
		return 1;
	puts("fatfs regression tests passed");
	return 0;
}
