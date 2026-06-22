#include "apps/openjazz/openjazz_pack.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "apps/port/port_runtime.h"

static constexpr uint32_t OJ_HEADER_SIZE = 20u;
static constexpr uint32_t OJ_ENTRY_SIZE = 16u;
static constexpr uint32_t OJ_VERSION = 1u;
static constexpr uint32_t OJ_MAGIC_SIZE = 12u;
static constexpr uint32_t OJ_MAX_ENTRIES = 512u;
static constexpr uint32_t OJ_MAX_NAME = 12u;

enum {
	OJ_FILE_NONE = 0,
	OJ_FILE_PACK = 1,
	OJ_FILE_FATFS = 2,
};

struct OjPackEntry {
	char name[OJ_MAX_NAME + 1u];
	uint32_t offset;
	uint32_t size;
	uint32_t crc;
};

static const char *const gOjRequired[] = {
	"PANEL.000", "MENU.000", "MAINCHAR.000",
	"FONT2.0FN", "FONTBIG.0FN", "FONTINY.0FN", "FONTMN1.0FN", "FONTMN2.0FN",
	"STARTUP.0SC", "SOUNDS.000",
	"LEVEL0.000", "LEVEL1.000", "LEVEL2.000",
	"BLOCKS.000", "SPRITES.000", "PLANET.000",
};

static struct Dc32PortPak gOjPak;
static struct FatfsVol *gOjVol;
static struct OjPackEntry *gOjEntries;
static uint32_t gOjEntryCount;
static struct FatfsFil *gOjUserFiles[FATFS_MAX_FILES];

static uint16_t ojRd16(const uint8_t *src)
{
	return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

static uint32_t ojRd32(const uint8_t *src)
{
	return (uint32_t)src[0] | ((uint32_t)src[1] << 8) |
		((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

static uint32_t ojCrc32Update(uint32_t crc, const void *data, uint32_t size)
{
	const uint8_t *src = (const uint8_t*)data;

	while (size--) {
		crc ^= *src++;
		for (uint32_t bit = 0; bit < 8u; bit++)
			crc = (crc >> 1) ^ (0xedb88320u & (uint32_t)-(int32_t)(crc & 1u));
	}
	return crc;
}

static char ojUpper(char ch)
{
	if (ch >= 'a' && ch <= 'z')
		return (char)(ch - ('a' - 'A'));
	return ch;
}

static int ojNameCompare(const char *a, const char *b)
{
	a = a ? a : "";
	b = b ? b : "";
	while (*a && *b) {
		char ca = ojUpper(*a++);
		char cb = ojUpper(*b++);

		if (ca != cb)
			return (unsigned char)ca < (unsigned char)cb ? -1 : 1;
	}
	if (*a)
		return 1;
	if (*b)
		return -1;
	return 0;
}

static const char *ojBasename(const char *name)
{
	const char *base = name;

	if (!name)
		return "";
	while (*name) {
		if (*name == '/' || *name == '\\')
			base = name + 1;
		name++;
	}
	return base;
}

static const struct OjPackEntry *ojFindEntry(const char *name)
{
	int32_t lo = 0;
	int32_t hi = (int32_t)gOjEntryCount - 1;

	name = ojBasename(name);
	while (lo <= hi) {
		int32_t mid = lo + (hi - lo) / 2;
		int cmp = ojNameCompare(name, gOjEntries[mid].name);

		if (!cmp)
			return &gOjEntries[mid];
		if (cmp < 0)
			hi = mid - 1;
		else
			lo = mid + 1;
	}
	return NULL;
}

static bool ojLoadIndex(void)
{
	uint32_t tableSize;
	uint32_t pos = 0;
	uint32_t previousEnd = 0;
	char previousName[OJ_MAX_NAME + 1u] = {};
	uint8_t *table;
	bool ok = false;

	if (gOjPak.size <= OJ_HEADER_SIZE)
		return false;
	tableSize = gOjEntryCount * (OJ_ENTRY_SIZE + OJ_MAX_NAME);
	if (tableSize > gOjPak.size - OJ_HEADER_SIZE)
		tableSize = gOjPak.size - OJ_HEADER_SIZE;
	gOjEntries = (struct OjPackEntry*)dc32PortCalloc(gOjEntryCount, sizeof(*gOjEntries));
	table = (uint8_t*)dc32PortMalloc(tableSize);
	if (!gOjEntries || !table)
		return false;
	if (!dc32PortReadAssetPack(&gOjPak, OJ_HEADER_SIZE, table, tableSize))
		goto cleanup;
	for (uint32_t i = 0; i < gOjEntryCount; i++) {
		struct OjPackEntry *entry = &gOjEntries[i];
		uint16_t nameLen;
		uint16_t reserved;

		if (pos > tableSize || OJ_ENTRY_SIZE > tableSize - pos)
			goto cleanup;
		nameLen = ojRd16(table + pos);
		reserved = ojRd16(table + pos + 2);
		entry->offset = ojRd32(table + pos + 4);
		entry->size = ojRd32(table + pos + 8);
		entry->crc = ojRd32(table + pos + 12);
		pos += OJ_ENTRY_SIZE;
		if (reserved || !nameLen || nameLen > OJ_MAX_NAME ||
				pos > tableSize || nameLen > tableSize - pos ||
				(entry->offset & 3u) || entry->offset < previousEnd ||
				entry->offset > gOjPak.size || entry->size > gOjPak.size - entry->offset)
			goto cleanup;
		memcpy(entry->name, table + pos, nameLen);
		entry->name[nameLen] = 0;
		for (uint32_t n = 0; n < nameLen; n++) {
			char ch = entry->name[n];

			if (ojUpper(ch) != ch || ch < 0x20 || ch > 0x7e || ch == '/' || ch == '\\')
				goto cleanup;
		}
		if (i && ojNameCompare(previousName, entry->name) >= 0)
			goto cleanup;
		memcpy(previousName, entry->name, nameLen + 1u);
		pos += nameLen;
		previousEnd = entry->offset + entry->size;
	}
	if ((((OJ_HEADER_SIZE + pos) + 3u) & ~3u) > gOjEntries[0].offset)
		goto cleanup;
	for (uint32_t i = 0; i < sizeof(gOjRequired) / sizeof(gOjRequired[0]); i++)
		if (!ojFindEntry(gOjRequired[i]))
			goto cleanup;
	ok = true;

cleanup:
	dc32PortFree(table);
	return ok;
}

bool dc32OjPackOpen(struct FatfsVol *vol, const char *path)
{
	uint8_t header[OJ_HEADER_SIZE];

	dc32OjPackClose();
	gOjVol = vol;
	if (!dc32PortOpenAssetPack(vol, path, &gOjPak))
		return false;
	if (!dc32PortReadAssetPack(&gOjPak, 0, header, sizeof(header)) ||
			memcmp(header, DC32_OPENJAZZ_PACK_MAGIC, OJ_MAGIC_SIZE) != 0 ||
			ojRd32(header + 12) != OJ_VERSION) {
		dc32OjPackClose();
		return false;
	}
	gOjEntryCount = ojRd32(header + 16);
	if (!gOjEntryCount || gOjEntryCount > OJ_MAX_ENTRIES || !ojLoadIndex()) {
		dc32OjPackClose();
		return false;
	}
	return true;
}

void dc32OjPackClose(void)
{
	dc32OjCloseUserFiles();
	dc32PortCloseAssetPack(&gOjPak);
	dc32PortFree(gOjEntries);
	gOjEntries = NULL;
	gOjEntryCount = 0;
	gOjVol = NULL;
}

void dc32OjCloseUserFiles(void)
{
	for (uint32_t i = 0; i < FATFS_MAX_FILES; i++) {
		if (gOjUserFiles[i])
			(void)fatfsFileClose(gOjUserFiles[i]);
		gOjUserFiles[i] = NULL;
	}
}

bool dc32OjPackIsOpen(void)
{
	return gOjPak.file != NULL && gOjEntries != NULL;
}

bool dc32OjPackFingerprint(uint32_t *fingerprint, uint32_t *fileSize, uint32_t *entryCount)
{
	uint32_t crc = 0xffffffffu;

	if (!dc32OjPackIsOpen())
		return false;
	for (uint32_t i = 0; i < gOjEntryCount; i++) {
		const struct OjPackEntry *entry = &gOjEntries[i];
		uint8_t values[8];
		uint32_t nameLen = (uint32_t)strlen(entry->name) + 1u;

		values[0] = (uint8_t)entry->size;
		values[1] = (uint8_t)(entry->size >> 8);
		values[2] = (uint8_t)(entry->size >> 16);
		values[3] = (uint8_t)(entry->size >> 24);
		values[4] = (uint8_t)entry->crc;
		values[5] = (uint8_t)(entry->crc >> 8);
		values[6] = (uint8_t)(entry->crc >> 16);
		values[7] = (uint8_t)(entry->crc >> 24);
		crc = ojCrc32Update(crc, entry->name, nameLen);
		crc = ojCrc32Update(crc, values, sizeof(values));
	}
	if (fingerprint)
		*fingerprint = crc ^ 0xffffffffu;
	if (fileSize)
		*fileSize = gOjPak.size;
	if (entryCount)
		*entryCount = gOjEntryCount;
	return true;
}

bool dc32OjFileExists(const char *name)
{
	return ojFindEntry(name) != NULL;
}

bool dc32OjFileOpen(const char *name, struct Dc32OjFile *file)
{
	const struct OjPackEntry *entry;

	if (!file)
		return false;
	memset(file, 0, sizeof(*file));
	entry = ojFindEntry(name);
	if (!entry)
		return false;
	file->offset = entry->offset;
	file->size = entry->size;
	file->kind = OJ_FILE_PACK;
	file->open = true;
	return true;
}

static const char *ojUserPath(const char *name)
{
	static const char *const source[] = {
		"OPENJAZZ.CFG", "SAVE.0", "SAVE.1", "SAVE.2", "SAVE.3",
	};
	static const char *const target[] = {
		"/SAVE/OJCFG.DAT", "/SAVE/OJSAVE0.DAT", "/SAVE/OJSAVE1.DAT",
		"/SAVE/OJSAVE2.DAT", "/SAVE/OJSAVE3.DAT",
	};

	name = ojBasename(name);
	for (uint32_t i = 0; i < sizeof(source) / sizeof(source[0]); i++)
		if (!ojNameCompare(name, source[i]))
			return target[i];
	return NULL;
}

static const char *ojUserFileName(const char *path)
{
	const char *slash = strrchr(path ? path : "", '/');

	return slash ? slash + 1 : path;
}

bool dc32OjUserFileExists(const char *name)
{
	const char *path = ojUserPath(name);
	struct FatfsDir *saveDir;
	struct FatFileLocator locator;
	bool exists;

	if (!gOjVol || !path)
		return false;
	saveDir = fatfsDirOpen(gOjVol, "/SAVE");
	if (!saveDir)
		return false;
	exists = fatfsFindFileAt(saveDir, ojUserFileName(path), &locator);
	(void)fatfsDirClose(saveDir);
	return exists;
}

bool dc32OjUserFileOpen(const char *name, bool write, struct Dc32OjFile *file)
{
	const char *path = ojUserPath(name);
	struct FatfsDir *saveDir;
	struct FatfsFil *fatFile;
	int32_t slot = -1;
	uint_fast8_t mode = write ? OPEN_MODE_CREATE | OPEN_MODE_WRITE | OPEN_MODE_TRUNCATE : OPEN_MODE_READ;

	if (!file)
		return false;
	memset(file, 0, sizeof(*file));
	if (!gOjVol || !path || (write && !dc32PortEnsureSaveDir(gOjVol)))
		return false;
	saveDir = fatfsDirOpen(gOjVol, "/SAVE");
	if (!saveDir)
		return false;
	fatFile = fatfsFileOpenAt(saveDir, ojUserFileName(path), mode);
	(void)fatfsDirClose(saveDir);
	if (!fatFile)
		return false;
	for (uint32_t i = 0; i < FATFS_MAX_FILES; i++)
		if (!gOjUserFiles[i]) {
			slot = (int32_t)i;
			break;
		}
	if (slot < 0) {
		(void)fatfsFileClose(fatFile);
		return false;
	}
	gOjUserFiles[slot] = fatFile;
	file->fatFile = fatFile;
	file->size = fatfsFileGetSize(fatFile);
	file->kind = OJ_FILE_FATFS;
	file->write = write;
	file->open = true;
	return true;
}

void dc32OjFileClose(struct Dc32OjFile *file)
{
	if (!file)
		return;
	if (file->kind == OJ_FILE_FATFS && file->fatFile) {
		if (file->write)
			(void)fatfsFileTruncate(file->fatFile, file->pos);
		for (uint32_t i = 0; i < FATFS_MAX_FILES; i++)
			if (gOjUserFiles[i] == file->fatFile)
				gOjUserFiles[i] = NULL;
		(void)fatfsFileClose(file->fatFile);
	}
	memset(file, 0, sizeof(*file));
}

uint32_t dc32OjFileSize(const struct Dc32OjFile *file)
{
	return file && file->open ? file->size : 0u;
}

uint32_t dc32OjFileTell(const struct Dc32OjFile *file)
{
	return file && file->open ? file->pos : 0u;
}

bool dc32OjFileSeek(struct Dc32OjFile *file, int32_t offset, int origin)
{
	int64_t base;
	int64_t pos;

	if (!file || !file->open)
		return false;
	if (origin == 0)
		base = 0;
	else if (origin == 1)
		base = file->pos;
	else if (origin == 2)
		base = file->size;
	else
		return false;
	pos = base + offset;
	if (pos < 0)
		pos = 0;
	if (pos > (int64_t)file->size)
		pos = file->size;
	if (file->kind == OJ_FILE_FATFS && !fatfsFileSeek(file->fatFile, (uint32_t)pos))
		return false;
	file->pos = (uint32_t)pos;
	return true;
}

uint32_t dc32OjFileRead(struct Dc32OjFile *file, void *dst, uint32_t size)
{
	uint32_t got = 0;

	if (!file || !file->open || !dst)
		return 0;
	if (size > file->size - file->pos)
		size = file->size - file->pos;
	if (!size)
		return 0;
	if (file->kind == OJ_FILE_PACK) {
		if (!gOjPak.file || !dc32PortReadAssetPack(&gOjPak, file->offset + file->pos, dst, size))
			return 0;
		got = size;
	} else if (file->kind == OJ_FILE_FATFS) {
		if (!fatfsFileRead(file->fatFile, dst, size, &got))
			return 0;
	} else {
		return 0;
	}
	file->pos += got;
	return got;
}

uint32_t dc32OjFileWrite(struct Dc32OjFile *file, const void *src, uint32_t size)
{
	uint32_t wrote = 0;

	if (!file || !file->open || file->kind != OJ_FILE_FATFS || !file->write || !src)
		return 0;
	if (!fatfsFileWrite(file->fatFile, src, size, &wrote))
		return 0;
	file->pos += wrote;
	if (file->pos > file->size)
		file->size = file->pos;
	return wrote;
}
