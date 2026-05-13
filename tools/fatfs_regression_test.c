#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/fatfs.h"

#define DISK_SECS			8192U
#define RESERVED_SECS		1U
#define NUM_FATS			1U
#define SECTORS_PER_FAT		64U
#define ROOT_DIR_ENTRIES	64U
#define ROOT_DIR_SECS		((ROOT_DIR_ENTRIES * 32U + FATFS_DISK_SECT_SZ - 1U) / FATFS_DISK_SECT_SZ)
#define DATA_SEC			(RESERVED_SECS + NUM_FATS * SECTORS_PER_FAT + ROOT_DIR_SECS)

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
	ok = expect(fatfsUnmount(vol), "unmount in-memory FAT16 image") && ok;

	if (!ok)
		return 1;
	puts("fatfs regression tests passed");
	return 0;
}
