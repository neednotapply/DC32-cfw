#include <strings.h>
#include <string.h>
#include <stdio.h>
#include "printf.h"
#include "fatfs.h"
#include "utf.h"



#define VERBOSE						0

#define FATFS_MAX_FILES_REAL		(FATFS_MAX_FILES + 2)		//we sometimes use up to 2
#define FATFS_ATTR_LFN				(FATFS_ATTR_RO | FATFS_ATTR_HIDDEN | FATFS_ATTR_SYSTEM | FATFS_ATTR_VOL_LBL)

#define CLUS_IS_ROOT_DIR			0xffffffff
#define CLUS_EOC					0xfffffff8
#define FATFS_CHARS_PER_LFN			13


struct FatfsFileLocator {
	uint32_t	cluster;
	
	//size is only valid for FILES
	uint32_t	size;
	
	//these fields always valid for FILES, not alawys for DIRECTORIES (due to possibilities of ".." in path)
	uint32_t	dirEntSec;			//the SFN entry's sector
	uint8_t		dirEntIdxInSec;		//the index of the SFN entry in the sector
};


#if FATFS_USE_LFN_SUPPORT
	enum FatfsLfnMode {
		FatfsLfnModeMatch,
		FatfsLfnModeList,
	};
	
	struct FatfsLfnState {
		uint8_t cksum;
		uint8_t numEntries;
		uint8_t nextEntryExpected;
		enum FatfsLfnMode mode;
		union {
			struct {
				const char *name;
				uint32_t maxLen;		//in bytes of the above, NULL terminator WILL end it earlier than this
				bool isMatch;			//only valid after ingesting an SFN entry!
			} match;
			struct {
				char *dst;				//assumed to be at least [FATFS_NAME_BUF_LEN]
			} list;	
		};
	};
#endif

struct FatfsVol {

	//disk data
	FatfsDiskReadF	diskReadF;
	FatfsDiskWriteF	diskWriteF;
	void*		diskUserData;

	//FS data
	uint32_t	fatSec;				//first sector of the first FAT
	uint32_t	fatSec2;			//first sector of the second FAT or zero
	uint32_t	dataSec;			//first sector of the data (of cluster 2)
	uint32_t	root;				//sector or cluster
	uint32_t	numClus;
	
	uint16_t	rootDirSecs;		//zero for fat32
	uint16_t	sectorsPerFat;
	uint8_t		sectorsPerClus;		//	1 << lg2SecPerClus
	uint8_t		lg2SecPerClus;
	
	//cache
	bool		cacheDirty;
	uint32_t	cachedSec;
	uint8_t		cache[FATFS_DISK_SECT_SZ + 2 /* for fat12, do not ask */];
};

struct FatfsFil {
	
	struct FatfsVol *vol;
	
	uint32_t	firstClus;			//set to CLUS_IS_ROOT_DIR for root dir in fat16 (by us). is zero for zero-length files/dirs
	uint32_t	curClus;
	
	uint32_t	fileSz;				//only valid for FILEs not DIRs 
	uint32_t	curPos;
	bool		atEnd;				//if set, curClus is LAST cluster we did have (which could be zero)
	
	//only valid for FILEs not DIRs
	bool		writeAllowed;
	uint8_t		dirEntIdxInSec;		//the index of the SFN entry in the sector
	uint32_t	dirEntSec;			//the SFN entry's sector
};

struct FatHdr {
	//FAT12, FAT16, and FAT32 all have this:
	uint8_t		jmp[3];
	char		oemName[8];
	uint16_t	bytesPerSec;
	uint8_t		secsPerCluster;
	uint16_t	reservedSectors;
	uint8_t		numFats;
	uint16_t	numRootDirEntries;
	uint16_t	numSectors16;
	uint8_t		useless1[1];
	uint16_t	sectorsPerFat16;	//unsed for FAT32
	uint8_t		useless2[8];
	
	//FAT32 only after this (16 technically has data here but we do not care)
	uint32_t	numSectors32;
	uint32_t	sectorsPerFat32;
	uint16_t	mirrorFlags32;		//if bit 7 is set, bits 0..3 give whih FAT to use
	uint8_t		useless4[2];
	uint32_t	rootDirClus32;
		
} __attribute__((packed));

struct Mbr {
	uint8_t		status;
	uint8_t		startChs[3];
	uint8_t		partType;
	uint8_t		endChs[3];
	uint32_t	firstSec;
	uint32_t	numSec;
} __attribute__((packed));

struct FatDirEntry {
	uint8_t		name[11];
	uint8_t		attrs;
	uint8_t		caseInfo;
	uint8_t		unused1[7];
	uint16_t	clusNumHi;	//fat32 only
	uint8_t		unused2[4];
	uint16_t	clusNum;	//0 if no chain attached to this entry (empty)
	uint32_t	fileSz;
} __attribute__((packed));

struct FatLfnEntry {
	uint8_t		ordinal;
	uint16_t	chars1[5];
	uint8_t		attrs;
	uint8_t		type;
	uint8_t		checksum;
	uint16_t	chars2[6];
	uint16_t	zero;
	uint16_t	chars3[2];
} __attribute__((packed));

#define FATFS_ORDINAL_MASK_DEL	0x80
#define FATFS_ORDINAL_MASK_LAST	0x40
#define FATFS_ORDINAL_MASK_IDX	0x3F




static uint8_t mVolumesUsed = ~((1 << FATFS_MAX_VOLUMES) - 1);
static uint8_t mFilesUsed = ~((1 << FATFS_MAX_FILES_REAL) - 1);
static struct FatfsVol mVolumes[FATFS_MAX_VOLUMES];
static struct FatfsFil mFiles[FATFS_MAX_FILES_REAL];

static bool fatfsPrvSomethingFind(struct FatfsVol *vol, const char *name, uint32_t nameMaxLen, bool wantDir, struct FatFileLocator *locP);
static bool fatfsPrvSomethingSeek(struct FatfsFil* fil, uint32_t pos);
static bool fatfsPrvSomethingFindAt(struct FatfsDir* dir, const char *name, uint32_t nameMaxLen, bool wantDir, struct FatFileLocator *locatorP, uint32_t *firstEntryPosInDirP, uint8_t *numEntriesTotalP);
static bool fatfsPrvDirRead(struct FatfsDir *dir, char *name, uint32_t *sizeP, uint8_t *attrsP, uint32_t *clusP, uint32_t *dirEntSecP, uint8_t *dirEntIdxInSecP, bool allowDotEntries);

#if FATFS_USE_LFN_SUPPORT
	static bool fatfsPrvLfnIngestEntry(struct FatfsLfnState *state, const void *entry);	//true if we finished decoding a complete set of entries. does not imply match!
	static bool fatfsPrvLfnDidMatch(struct FatfsLfnState *state);
	static bool fatfsPrvLfnListDidWork(struct FatfsLfnState *state);		//did we process an LFN chain successfully?
	static void fatfsPrvLfnMatchInit(struct FatfsLfnState *state, const char *matchStr, uint32_t matchMaxLen);
	static void fatfsPrvLfnListInit(struct FatfsLfnState *state, char *outStr);
	static bool fatfsPrvLfnWasFirstLfn(struct FatfsLfnState *state);		//was the last ingsted entry the first in an LFN chain (last words of filename)
#endif


static uint16_t fatfsPrvGetLE16(const void *ptr)
{
	const uint8_t *v = (const uint8_t*)ptr;

	return (((uint16_t)v[1]) << 8) | ((uint16_t)v[0]);
}

static uint32_t fatfsPrvGetLE24(const void* ptr)
{
	const uint8_t *v = (const uint8_t*)ptr;

	return (((uint32_t)v[2]) << 16) | (((uint32_t)v[1]) << 8) | ((uint32_t)v[0]);
}

static uint32_t fatfsPrvGetLE32(const void* ptr)
{
	const uint8_t *v = (const uint8_t*)ptr;

	return (((uint32_t)v[3]) << 24) | (((uint32_t)v[2]) << 16) | (((uint32_t)v[1]) << 8) | ((uint32_t)v[0]);
}

static void fatfsPrvSetLE16(void *ptr, uint_fast16_t val)
{
	uint8_t *v = (uint8_t*)ptr;

	v[0] = val;
	v[1] = val >> 8;
}

static void fatfsPrvSetLE24(void* ptr, uint32_t val)
{
	uint8_t *v = (uint8_t*)ptr;

	v[0] = val;
	v[1] = val >> 8;
	v[2] = val >> 16;
}

static void fatfsPrvSetLE32(void* ptr, uint32_t val)
{
	uint8_t *v = (uint8_t*)ptr;

	v[0] = val;
	v[1] = val >> 8;
	v[2] = val >> 16;
	v[3] = val >> 24;
}

static int_fast8_t fatfsPrvFindFreeStruct(uint8_t *maskP)
{
	uint_fast8_t i, mask = 1;
	
	for (i = 0; i < 8; i++, mask <<= 1) {
		if ((*maskP) & mask)
			continue;
		
		*maskP |= mask;
		
		return i;
	}
	
	return -1;
}

static struct FatfsVol* fatfsPrvGetFreeVolume(void)
{
	int_fast8_t idx = fatfsPrvFindFreeStruct(&mVolumesUsed);
	
	return (idx < 0) ? NULL : (mVolumes + idx);
}

static struct FatfsFil* fatfsPrvGetFreeFile(void)
{
	int_fast8_t idx = fatfsPrvFindFreeStruct(&mFilesUsed);
	
	return (idx < 0) ? NULL : (mFiles + idx);
}

static void fatfsPrvFreeVolume(struct FatfsVol *vol)
{
	mVolumesUsed &=~ (1 << (vol - mVolumes));
}

static void fatfsPrvFreeFile(struct FatfsFil *fil)
{
	mFilesUsed &=~ (1 << (fil - mFiles));
}

static uint32_t fatfsPrvPartitionFind(const uint8_t *buf)
{
	const struct Mbr *entry = (const struct Mbr*)(buf + 0x1BE);
	unsigned i;
	
	for (i = 0; i < 4; i++) {
		if (entry->partType != 0x01 && entry->partType != 0x04 && entry->partType != 0x06 && entry->partType != 0x0B && entry->partType != 0x0C && entry->partType != 0x0E)
			continue;	//not FAT parition

		if (entry->status != 0x80 && entry->status != 0)
			continue;	//invalid status byte

		if (fatfsPrvGetLE32(&entry->numSec) <= 3)
			continue;	//invalid length
		
		return fatfsPrvGetLE32(&entry->firstSec);
	}
	
	return 0;
}

static bool fatfsPrvCacheClean(struct FatfsVol* vol)
{
	bool ret = true;
	
	if (vol->cacheDirty) {
		
		ret = vol->diskWriteF(vol->diskUserData, vol->cachedSec, 1, vol->cache) && ret;
		
		//write FAT2 if writing FAT1
		if (vol->fatSec2 && vol->cachedSec >= vol->fatSec && vol->cachedSec - vol->fatSec < vol->sectorsPerFat)
			ret = vol->diskWriteF(vol->diskUserData, vol->cachedSec - vol->fatSec + vol->fatSec2, 1, vol->cache) && ret;
	}
	
	vol->cacheDirty = false;
	return ret;
}

static bool fatfsPrvReadSecToCacheIfNeeded(struct FatfsVol* vol, uint32_t sec)
{
	if (vol->cachedSec == sec)
		return true;
	
	if (!fatfsPrvCacheClean(vol))
		return false;
	
	vol->cachedSec = 0;		//safe as we never read sec 0
	
	if (!vol->diskReadF(vol->diskUserData, sec, 1, vol->cache))
		return false;
	
	vol->cachedSec = sec;
	
	return true;
}

static bool fatfsPrvReadSecs(struct FatfsVol* vol, uint32_t sec, uint32_t numSec, void *buf)
{
	//we might be reading a sec that is dirty in the cache
	if (vol->cacheDirty && sec <= vol->cachedSec && vol->cachedSec - sec < numSec && !fatfsPrvCacheClean(vol))
		return false;
	
	return vol->diskReadF(vol->diskUserData, sec, numSec, buf);
}

static bool fatfsPrvWriteSecs(struct FatfsVol* vol, uint32_t sec, uint32_t numSec, const void *buf)
{
	if (!vol->diskWriteF(vol->diskUserData, sec, numSec, buf))
		return false;
	
	//if we overwrite a cached sec, it is invalid, even if it was dirty
	if (sec <= vol->cachedSec && vol->cachedSec - sec < numSec) {
		
		vol->cachedSec = 0;
		vol->cacheDirty = false;
	}
	
	return true;
}

static uint32_t fatfsPrvClusToSec(const struct FatfsVol* vol, uint32_t clus)
{
	return vol->dataSec + (clus - 2) * vol->sectorsPerClus;
}

static uint32_t fatfsPrvSecToClus(const struct FatfsVol* vol, uint32_t sec)
{
	uint32_t clus;
	
	if (sec < vol->dataSec)
		return 0;
	
	clus = (sec - vol->dataSec) / (1 << vol->lg2SecPerClus);
	
	if (clus >= vol->numClus)
		return 0;
	
	return clus + 2;
}

static bool fatfsPrvIsFat12(const struct FatfsVol* vol)
{
	return vol->numClus <= 0xff5 - 2;
}

static bool fatfsPrvVolIsFat32(const struct FatfsVol* vol)
{
	return !vol->rootDirSecs;
}

static bool fatfsPrvIsFat32rootDir(const struct FatfsFil *dirAsFile)
{
	return fatfsPrvVolIsFat32(dirAsFile->vol) && dirAsFile->firstClus == dirAsFile->vol->root;
}

static bool fatfsPrvIsFat12_16rootDir(const struct FatfsFil *dirAsFile)
{
	return !fatfsPrvVolIsFat32(dirAsFile->vol) && dirAsFile->firstClus == CLUS_IS_ROOT_DIR;
}

static bool fatfsPrvIsRootDir(const struct FatfsFil *dirAsFile)
{
	return fatfsPrvIsFat12_16rootDir(dirAsFile) || fatfsPrvIsFat32rootDir(dirAsFile);
}

static uint32_t fatfsPrvGetNextClus(struct FatfsVol* vol, uint32_t curClus)
{
	if (curClus < 2 || curClus - 2 >= vol->numClus)
		return 0;
	
	if (fatfsPrvIsFat12(vol)) {
		
		const uint_fast16_t entryIdxDiv2 = curClus / 2;
		const uint_fast16_t entryByteOfst = entryIdxDiv2 * 3;
		const uint_fast16_t firstSecIdx = entryByteOfst / FATFS_DISK_SECT_SZ;
		const uint_fast16_t secondSecIdx = (entryByteOfst + 2) / FATFS_DISK_SECT_SZ;
		const uint_fast16_t byteOfst = entryByteOfst % FATFS_DISK_SECT_SZ;
		uint32_t twoEntries;
		
		if (secondSecIdx >= vol->sectorsPerFat)
			return 0;
		
		if (secondSecIdx != firstSecIdx) {
			
			if (!fatfsPrvReadSecToCacheIfNeeded(vol, vol->fatSec + secondSecIdx))
				return 0;
			
			//two bytes ar as much overhand as we can have
			vol->cache[FATFS_DISK_SECT_SZ + 0] = vol->cache[0];
			vol->cache[FATFS_DISK_SECT_SZ + 1] = vol->cache[1];
		}
		
		if (!fatfsPrvReadSecToCacheIfNeeded(vol, vol->fatSec + firstSecIdx))
			return 0;
	
		twoEntries = fatfsPrvGetLE24(vol->cache + entryByteOfst);
		
		curClus = (curClus % 2) ? (twoEntries >> 12) : (twoEntries & 0x0fff);
		
		if (curClus >= 0x0ff6)
			return CLUS_EOC;
	}
	else {
	
		const uint32_t fatEntrySzShift = fatfsPrvVolIsFat32(vol) ? __builtin_ctz(sizeof(uint32_t)) : __builtin_ctz(sizeof(uint16_t));
		const uint32_t fatEntriesPerSecShift = __builtin_ctz(FATFS_DISK_SECT_SZ) - fatEntrySzShift;
		const uint32_t sec = curClus / (1 << fatEntriesPerSecShift);
		const uint32_t ofst = (curClus % (1 << fatEntriesPerSecShift)) << fatEntrySzShift;
		
		if (sec >= vol->sectorsPerFat)
			return 0;
		
		if (!fatfsPrvReadSecToCacheIfNeeded(vol, vol->fatSec + sec))
			return 0;
		
		if (fatfsPrvVolIsFat32(vol)) {
			curClus = fatfsPrvGetLE32(vol->cache + ofst);
			curClus &=~ 0xf0000000UL;
			if (curClus >= 0x0ffffff6)
				return CLUS_EOC;
		}
		else {
			curClus = fatfsPrvGetLE16(vol->cache + ofst);
			if (curClus >= 0xfff6)
				return CLUS_EOC;
		}
	}
	
	return curClus;
}

static bool fatfsPrvSetNextClus(struct FatfsVol* vol, uint32_t curClus, uint32_t nextClus)
{
	if (curClus < 2 || curClus - 2 >= vol->numClus)
		return false;
	
	if (nextClus != CLUS_EOC && nextClus != 0 && (nextClus < 2 || nextClus - 2 >= vol->numClus))
			return false;
		
	if (fatfsPrvIsFat12(vol)) {
		
		//this is somewhat inefficient for some corner cases, but that is the price we pay for not using an extra whole sector of RAM
		
		const uint_fast16_t entryIdxDiv2 = curClus / 2;
		const uint_fast16_t entryByteOfst = entryIdxDiv2 * 3;
		const uint_fast16_t firstSecIdx = entryByteOfst / FATFS_DISK_SECT_SZ;
		const uint_fast16_t secondSecIdx = (entryByteOfst + 2) / FATFS_DISK_SECT_SZ;
		const uint_fast16_t byteOfst = entryByteOfst % FATFS_DISK_SECT_SZ;
		uint32_t twoEntries;
		
		if (secondSecIdx >= vol->sectorsPerFat)
			return false;
		
		if (secondSecIdx != firstSecIdx) {
			
			if (!fatfsPrvReadSecToCacheIfNeeded(vol, vol->fatSec + secondSecIdx))
				return false;
			
			//two bytes ar as much overhand as we can have
			vol->cache[FATFS_DISK_SECT_SZ + 0] = vol->cache[0];
			vol->cache[FATFS_DISK_SECT_SZ + 1] = vol->cache[1];
		}
		
		if (!fatfsPrvReadSecToCacheIfNeeded(vol, vol->fatSec + firstSecIdx))
			return false;
	
		twoEntries = fatfsPrvGetLE24(vol->cache + entryByteOfst);
		nextClus &= 0x0fff;
		
		if (curClus % 2)
			twoEntries = (twoEntries & 0x000fff) + (nextClus << 12);
		else
			twoEntries = (twoEntries & 0xfff000) + nextClus;
		
		fatfsPrvSetLE24(vol->cache + entryByteOfst, twoEntries);
		vol->cacheDirty = 1;
		
		if (secondSecIdx != firstSecIdx) {		//hard case - two sectors touched
			
			if (!fatfsPrvReadSecToCacheIfNeeded(vol, vol->fatSec + secondSecIdx))
				return false;
			
			
			//two bytes ar as much overhand as we can have
			vol->cache[0] = vol->cache[FATFS_DISK_SECT_SZ + 0];
			vol->cache[1] = vol->cache[FATFS_DISK_SECT_SZ + 1];
			vol->cacheDirty = 1;
		}
	}
	else {
	
		const uint32_t fatEntrySzShift = fatfsPrvVolIsFat32(vol) ? __builtin_ctz(sizeof(uint32_t)) : __builtin_ctz(sizeof(uint16_t));
		const uint32_t fatEntriesPerSecShift = __builtin_ctz(FATFS_DISK_SECT_SZ) - fatEntrySzShift;
		const uint32_t sec = curClus / (1 << fatEntriesPerSecShift);
		const uint32_t ofst = (curClus % (1 << fatEntriesPerSecShift)) << fatEntrySzShift;
		
		if (sec >= vol->sectorsPerFat)
			return false;
		
		if (!fatfsPrvReadSecToCacheIfNeeded(vol, vol->fatSec + sec))
			return false;
		
		if (fatfsPrvVolIsFat32(vol)) 
			fatfsPrvSetLE32(vol->cache + ofst, nextClus);
		else
			fatfsPrvSetLE16(vol->cache + ofst, nextClus);
	
		vol->cacheDirty = 1;
	}
	return true;
}

static bool fatfsPrvDirEntrySpecificRead(struct FatDirEntry *de, struct FatfsVol* vol, uint32_t sec, uint_fast8_t idxInSec)
{
	if (!fatfsPrvReadSecToCacheIfNeeded(vol, sec))
		return false;
	
	*de = ((const struct FatDirEntry*)vol->cache)[idxInSec];
	
	return true;
}

static bool fatfsPrvDirEntrySpecificWrite(const struct FatDirEntry *de, struct FatfsVol* vol, uint32_t sec, uint_fast8_t idxInSec)
{
	if (!fatfsPrvReadSecToCacheIfNeeded(vol, sec))
		return false;
	
	if (VERBOSE)
		pr("fatfsPrvDirEntrySpecificWrite [%u]\n", idxInSec);
	
	((struct FatDirEntry*)vol->cache)[idxInSec] = *de;
	vol->cacheDirty = 1;

	return true;
}

static bool fatfsPrvIsEOC(const struct FatfsFil *fil)		//are we just past the end of the last cluster/sector? will not be true for files if their size % clusterSz != 0
{
	if (fatfsPrvIsFat12_16rootDir(fil))
		return fil->curPos / FATFS_DISK_SECT_SZ >= fil->vol->rootDirSecs;
	else
		return fil->atEnd;
}

static bool fatfsPrvCurPosToSecAndOfst(const struct FatfsFil *fil, uint32_t *secP, uint16_t *ofstP)
{
	if (ofstP)
		*ofstP = fil->curPos % FATFS_DISK_SECT_SZ;
	
	//careful, do not give out offsets of the end of things as that might cause a write-past-the-end issue
	if (fatfsPrvIsEOC(fil))
		return false;
	
	if (fatfsPrvIsFat12_16rootDir(fil)) {
		
		uint32_t secInRoot = fil->curPos / FATFS_DISK_SECT_SZ;
		
		if (secP)
			*secP = secInRoot + fil->vol->root;
		
		return true;
	}
	else {
		
		uint32_t curSecInClus = (fil->curPos / FATFS_DISK_SECT_SZ) % (1 << fil->vol->lg2SecPerClus);
		uint32_t curClusStartSec = fatfsPrvClusToSec(fil->vol, fil->curClus);
		
		//normal cluster that is not at end
		if (secP)
			*secP = curClusStartSec + curSecInClus;
		
		return true;
	}
}

static uint32_t fatfsPrvFreeClusFind(struct FatfsVol* vol, uint32_t guess, uint32_t notAllowedClus)
{
	uint32_t i;
	
	if (guess >= 2)
		guess -= 2;	//constant offset
	
	for (i = 0; i < vol->numClus; i++) {
		
		uint32_t searchedClus = i + guess;
		
		if (searchedClus >= vol->numClus)
			searchedClus -= vol->numClus;
		searchedClus += 2;	//constant offset
		
		if (searchedClus != notAllowedClus && !fatfsPrvGetNextClus(vol, searchedClus))
			return searchedClus;
	}
	
	return 0;
}

#if FATFS_USE_LFN_SUPPORT
	//return num dir entries that will be needed, fill in SFN
	static uint32_t fatfsPrvNameCalculate(struct FatDirEntry *sfnEntry, const char *name)
	{
		bool haveUpcaseInName = false, haveLowCaseInName = false, haveUpcaseInExt = false, haveLowCaseInExt = false;
		uint32_t i = 0, j = 0, k = 0, L = 0, numUcs2Chars = 0;
		struct Utf8state u8s = UTF8_STATE_STATIC_INITIALIZER;
		char sfnSoFar[FATFS_SFN_MAX_LEN + 1], sfnExtSoFar[4];
		bool sfnPossible = true;
		int_fast8_t dotAt = -1;
		
		
		sfnEntry->caseInfo = 0;
		
		//basics (no nulls, empty strings, no dot and dotdot entries)
		if (!name || name[0] == 0 ||  (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0))))
			return 0;
		
		//verify utf validity, do some name validity checks, and some preliminary SFN checks
		while (1) {
			
			int16_t byteVal;
			uint32_t ret;
		
			ret = utf8inputByte(&u8s, name[i++]);
			if (ret == UTF_ERROR)				//invalid UTF ends the party
				return 0;
			if (ret == UTF_NO_OUTPUT)
				continue;
			if (ret == 0) {
				if (numUcs2Chars == 0)			//if the resulting name had no length, it is invalid
					return 0;
				break;
			}
	
			//some chars are not allowed in names at all
			if (ret < 32 || ret == '"' || ret == '*' || ret == '/' || ret == ':' || ret == '<' || ret == '>' || ret == '?' || ret == '\\' || ret == '|' || ret == 0x7f)
				return 0;
			
			//keep a tally of the number of 16-bit codepoints we'll need
			numUcs2Chars++;
			
			//see if we can encode it in win1251
			byteVal = utfTo8bit(ret);
			if (byteVal < 0)					//if we cannot encode the value as an 8-bit win1251 char, SFN is impossible
				sfnPossible = false;
			
			//some chars that are allowed in LFN are not allowed in SFN - check for that
			if (byteVal == 0x2b || byteVal == 0x2c || byteVal == 0x3b || byteVal == 0x3d || byteVal == 0x5b || byteVal == 0x5d) {
				
				sfnPossible = false;
				continue;	//do not add to SFN or EXT
			}
			
			//technically space is allowed, but never used due to lack of escaping in dos. disallow it
			if (byteVal == ' ') {
				
				sfnPossible = false;
				continue;	//do not add to SFN or EXT
			}
			
			//save SFN-valid chars as we need SOME short name anyways
			if (byteVal == '.') {
				if (dotAt >= 0 || !j)			//a name starting with a dot cannot be encoded as an SFN, nor can a name with multiple dots
					sfnPossible = false;
				dotAt = j;
				k = 0;
			}
			else if (byteVal != ' ') {
				
				if (j < sizeof(sfnSoFar))
					sfnSoFar[j++] = byteVal;
				if (byteVal != '.' && dotAt >= 0 && k < sizeof(sfnExtSoFar))
					sfnExtSoFar[k++] = byteVal;
			}
		}
			
		if (k > 3)								//the extension cannot be too long
			sfnPossible = false;
		if (dotAt >= 0 && k == 0)				//SFN cannot end in a dot
			sfnPossible = false;
		
		i = dotAt >= 0 ? (unsigned)dotAt : j;				//length of name minus extension
		if (i == 0 || i > 8)
			sfnPossible = false;
		
		for (L = 0; L < i && L < 8; L++)
			sfnEntry->name[L] = sfnSoFar[L];
		for (;L < 8; L++)
			sfnEntry->name[L] = ' ';
		for (L = 0; L < k && L < 3; L++)
			sfnEntry->name[8 + L] = sfnExtSoFar[L];
		for (;L < 3; L++)
			sfnEntry->name[8 + L] = ' ';
			
		//check cases and convert to upcase
		for (j = 0; j < 8; j++) {
			if (sfnEntry->name[j] >= 'A' && sfnEntry->name[j] <= 'Z')
				haveUpcaseInName = true;
			else if (sfnEntry->name[j] >= 'a' && sfnEntry->name[j] <= 'z') {
				sfnEntry->name[j] += 'A' - 'a';
				haveLowCaseInName = true;
			}
		}
		for (j = 8; j < 11; j++) {
			if (sfnEntry->name[j] >= 'A' && sfnEntry->name[j] <= 'Z')
				haveUpcaseInExt = true;
			else if (sfnEntry->name[j] >= 'a' && sfnEntry->name[j] <= 'z') {
				sfnEntry->name[j] += 'A' - 'a';
				haveLowCaseInExt = true;
			}
		}
		
		if ((haveUpcaseInName && haveLowCaseInName) || (haveUpcaseInExt && haveLowCaseInExt))
			sfnPossible = false;
			
		if (sfnEntry->name[0] == 0xe5)
			sfnEntry->name[0] = 0x05;
			
		//more SFN checks
		if (sfnPossible) {
			
			//record case info
			if (haveLowCaseInName)
				sfnEntry->caseInfo |= 0x08;
			
			if (haveLowCaseInExt)
				sfnEntry->caseInfo |= 0x10;
			
			return 1;
		}
	
		//SFN won't fly. we need a placeholder SFN - create it
		if (i > 6)
			i = 6;
		sfnEntry->name[i + 0] = '~';
		sfnEntry->name[i + 1] = '1';
		
		//verify against the LFN length limit
		if (numUcs2Chars > FATFS_LFN_MAX_LEN)
			return 0;
		
		return 1 + (numUcs2Chars + FATFS_CHARS_PER_LFN - 1) / FATFS_CHARS_PER_LFN;
	}
#else
	//return num dir entries that will be needed, fill in SFN
	static uint32_t fatfsPrvNameCalculate(struct FatDirEntry *sfnEntry, const char *name)
	{
		bool haveUpcaseInName = false, haveLowCaseInName = false, haveUpcaseInExt = false, haveLowCaseInExt = false;
		int_fast8_t dotAt = -1;
		uint32_t i, j, L;
		
		sfnEntry->caseInfo = 0;
		
		//basics (no nulls, empty strings, no dot and dotdot entries)
		if (!name || name[0] == 0 ||  (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0))))
			return 0;
		
		L = strlen(name);
		
		if (L > 12)
			return 0;
		
		for (i = 0; i < L; i++) {
			
			//some chars are not allowed in names at all
			if ((unsigned char)name[i] < 32 || name[i] == '"' || name[i] == '*' || name[i] == '/' || name[i] == ':' || name[i] == '<' || name[i] == '>' || name[i] == '?' || name[i] == '\\' || name[i] == '|')
				return 0;
			
			//some chars that are allowed in LFN are not allowed in SFN - check for that
			if (name[i] == 0x2b || name[i] == 0x2c || name[i] == 0x3b || name[i] == 0x3d || name[i] == 0x5b || name[i] == 0x5d)
				return 0;
			
			//spaces are allowed in theory but not in practice
			if (name[i] == ' ')
				return 0;
			
			if (name[i] == '.') {
				
				if (dotAt >= 0)
					return 0;
				
				dotAt = i;
			}
		}
		
		//name len
		i = dotAt >= 0 ? (uint32_t)dotAt : L;
		
		//length limit on name
		if (i == 0 || i > 8)
			return 0;
		
		//length limit on ext
		if (dotAt >= 0 && L - dotAt - 1 > 3)
			return 0;
		
		for (j = 0; j < i; j++)
			sfnEntry->name[j] = name[j];
		for (;j < 8; j++)
			sfnEntry->name[j] = ' ';
		for (j = 0; dotAt >= 0 && j < L - dotAt - 1; j++)
			sfnEntry->name[8 + j] = name[dotAt + 1 + j];
		for (;j < 3; j++)
			sfnEntry->name[8 + j] = ' ';
			
		//check cases and convert to upcase
		for (j = 0; j < 8; j++) {
			if (sfnEntry->name[j] >= 'A' && sfnEntry->name[j] <= 'Z')
				haveUpcaseInName = true;
			else if (sfnEntry->name[j] >= 'a' && sfnEntry->name[j] <= 'z') {
				sfnEntry->name[j] += 'A' - 'a';
				haveLowCaseInName = true;
			}
		}
		for (j = 8; j < 11; j++) {
			if (sfnEntry->name[j] >= 'A' && sfnEntry->name[j] <= 'Z')
				haveUpcaseInExt = true;
			else if (sfnEntry->name[j] >= 'a' && sfnEntry->name[j] <= 'z') {
				sfnEntry->name[j] += 'A' - 'a';
				haveLowCaseInExt = true;
			}
		}
	
		if (sfnEntry->name[0] == 0xe5)
			sfnEntry->name[0] = 0x05;
			
		//record case info (only if unambiguous, else we lose the info bt allow the name)
		if (haveLowCaseInName && !haveUpcaseInName)
			sfnEntry->caseInfo |= 0x08;
		
		if (haveLowCaseInExt && !haveUpcaseInExt)
			sfnEntry->caseInfo |= 0x10;
			
		return 1;
	}
#endif

struct FatfsVol* fatfsMount(FatfsDiskReadF readF, FatfsDiskWriteF writeF, void *diskUserData)
{
	uint32_t firstDiskSec = 0, fatsEnd, numSectors, secsPerFat;
	struct FatfsVol *vol;
	struct FatHdr *hdr;
	bool fat32 = false;
	
	if (sizeof(struct FatfsFileLocator) > sizeof(struct FatFileLocator)) {
		if (VERBOSE)
			pr("external locator too small\n");
		return NULL;
	}
	
	vol = fatfsPrvGetFreeVolume();
	if (!vol)
		return false;
	
	hdr = (struct FatHdr*)vol->cache;
	
	while (1) {
		
		if (!readF(diskUserData, firstDiskSec, 1, vol->cache))
			goto fail_free_vol;
		
		//a simple criteria that we can use to determine if the header is valid FS
		if (fatfsPrvGetLE16(&hdr->bytesPerSec) == FATFS_DISK_SECT_SZ && !(hdr->secsPerCluster & (hdr->secsPerCluster - 1)))
			break;
		
		if (firstDiskSec)
			goto fail_free_vol;
		
		firstDiskSec = fatfsPrvPartitionFind(vol->cache);
		if (!firstDiskSec)
			goto fail_free_vol;
	}
	
	//at this point we think we found the FS's first sector		
	if (!fatfsPrvGetLE16(&hdr->sectorsPerFat16)) {
		if (fatfsPrvGetLE32(&hdr->sectorsPerFat32) && !fatfsPrvGetLE16(&hdr->numRootDirEntries))
			fat32 = true;
		else
			goto fail_free_vol;
	}
	
	numSectors = hdr->numSectors16;
	if (!numSectors)
		numSectors = hdr->numSectors32;
	
	if (!numSectors)
		goto fail_free_vol;
	
	//and now we're ready to allocate the structures
	vol->diskReadF = readF;
	vol->diskWriteF = writeF;
	vol->diskUserData = diskUserData;
	
	secsPerFat = fat32 ? fatfsPrvGetLE32(&hdr->sectorsPerFat32) : fatfsPrvGetLE16(&hdr->sectorsPerFat16);
	
	vol->sectorsPerFat = secsPerFat;
	vol->fatSec = firstDiskSec + fatfsPrvGetLE16(&hdr->reservedSectors);	//first sec of first fat (of possibly multiple)
	vol->rootDirSecs = ((uint32_t)fatfsPrvGetLE16(&hdr->numRootDirEntries) * sizeof(struct FatDirEntry) + FATFS_DISK_SECT_SZ - 1) / FATFS_DISK_SECT_SZ;
	fatsEnd = vol->fatSec + vol->sectorsPerFat * hdr->numFats;
	vol->root = fat32 ? fatfsPrvGetLE32(&hdr->rootDirClus32) : fatsEnd;
	vol->dataSec = fatsEnd + vol->rootDirSecs;
	vol->sectorsPerClus = hdr->secsPerCluster;
	vol->lg2SecPerClus = __builtin_ctz(vol->sectorsPerClus);
	vol->cachedSec = 0;	//this is safe because we'll never need to read the 0th sector again
	vol->numClus = (numSectors - vol->dataSec) / (1 << vol->lg2SecPerClus);

	//must be a power of 2
	if (hdr->secsPerCluster & (hdr->secsPerCluster - 1))
		goto fail_free_vol;

	if (VERBOSE)
		pr(" %u clusters @ %u sectors each (%u shift)\n", vol->numClus, vol->sectorsPerClus, vol->lg2SecPerClus);
	
	if (vol->numClus <= 0xff5 - 2) {		//ff5 is the last valid value, but clusters begin with 2

		if (VERBOSE)
			pr("FAT size: 12\n");
		if (fat32)
			goto fail_free_vol;
	}
	else if (vol->numClus <= 0xfff5 - 2) {

		if (VERBOSE)
			pr("FAT size: 16\n");
		if (fat32)
			goto fail_free_vol;
	}
	else if (vol->numClus <= 0x0ffffff5 - 2) {

		if (VERBOSE)
			pr("FAT size: 32\n");
		if (!fat32)
			goto fail_free_vol;
	}
	else {
		
		goto fail_free_vol;
	}
	
	if (!hdr->numFats || hdr->numFats > 2)
		goto fail_free_vol;
	
	vol->fatSec2 = (hdr->numFats == 2) ? vol->fatSec + secsPerFat : 0;

	if (fat32 && (fatfsPrvGetLE16(&hdr->mirrorFlags32) & 0x80)) {	//seek to proper fat in case only one is active
	
		if ((fatfsPrvGetLE16(&hdr->mirrorFlags32) & 7) >= hdr->numFats)
			goto fail_free_vol;
		
		//now sure we only are dealing with 1
		if (fatfsPrvGetLE16(&hdr->mirrorFlags32) & 7) {	//fat #1 (second) is active
			
			uint32_t t = vol->fatSec;
			vol->fatSec = vol->fatSec2;
			vol->fatSec2 = t;
			vol->fatSec += secsPerFat * (fatfsPrvGetLE16(&hdr->mirrorFlags32) & 7);
		}
	}
	
	return vol;

fail_free_vol:
	fatfsPrvFreeVolume(vol);
	return NULL;
}

bool fatfsUnmount(struct FatfsVol *vol)
{
	bool ret = fatfsPrvCacheClean(vol);
	fatfsPrvFreeVolume(vol);
	
	return ret;
}

static struct FatfsFil* fatfsPrvSomethingOpenWithClusNo(struct FatfsVol *vol, uint32_t clus, uint32_t fileSz, uint32_t dirEntSec, uint8_t dirEntIdxInSec)
{
	struct FatfsFil *fil;
	
	if (VERBOSE)
		pr("open at cluster %u (0x%08x)\n", clus, clus);
	
	//for cluster-based things we get cluster, for root dir in fat16 we'll get CLUS_IS_ROOT_DIR
	fil = fatfsPrvGetFreeFile();
	if (!fil)
		return NULL;
	
	memset(fil, 0, sizeof(*fil));
	
	fil->vol = vol;
	fil->firstClus = clus;
	fil->curClus = clus;
	fil->fileSz = fileSz;
	fil->dirEntSec = dirEntSec;
	fil->dirEntIdxInSec = dirEntIdxInSec;
	fil->atEnd = !clus;

	return fil;
}

static bool fatfsPrvOpenFlagsValidate(uint_fast8_t mode)
{
	//check for reserved bits
	if (mode &~ (OPEN_MODE_READ | OPEN_MODE_WRITE | OPEN_MODE_CREATE | OPEN_MODE_TRUNCATE))
		return false;
	
	//create and truncate not allowed if not opening for write
	if ((mode & (OPEN_MODE_CREATE | OPEN_MODE_TRUNCATE)) && !(mode & OPEN_MODE_WRITE))
		return false;
	
	return true;
}

static bool fatfsPrvNewlyOpenedFileApplyMode(struct FatfsFil *fil, uint_fast8_t mode)	//assumes valid mode
{
	fil->writeAllowed = !!(mode & OPEN_MODE_WRITE);
	if (mode & OPEN_MODE_TRUNCATE) {												//any failures here may corrupt the FA (lose clusters)
		
		uint32_t clus, nextClus;
		struct FatDirEntry de;
		
		if (!fatfsPrvDirEntrySpecificRead(&de, fil->vol, fil->dirEntSec, fil->dirEntIdxInSec))
			return false;
		de.clusNumHi = 0;
		de.clusNumHi = 0;
		de.fileSz = 0;
		if (!fatfsPrvDirEntrySpecificWrite(&de, fil->vol, fil->dirEntSec, fil->dirEntIdxInSec))
			return false;
		
		for (clus = fil->curClus; clus != 0 && clus != CLUS_EOC; clus = nextClus) {
			
			nextClus = fatfsPrvGetNextClus(fil->vol, clus);
			if (!fatfsPrvSetNextClus(fil->vol, clus, CLUS_EOC))
				return false;
		}
		
		fil->curClus = 0;
		fil->fileSz = 0;
	}
	
	return true;
}

struct FatfsFil* fatfsFileOpenAt(struct FatfsDir* dir, const char *name, uint_fast8_t mode)
{
	struct FatfsFil *dirAsFile = (struct FatfsFil*)dir;
	struct FatFileLocator loc;
	
	if (fatfsPrvSomethingFindAt(dir, name, 0x10000000, false, &loc, NULL, NULL) || ((mode & OPEN_MODE_CREATE) && fatfsFileCreateAt(dir, name, &loc)))
		return fatfsFileOpenWithLocator(dirAsFile->vol, &loc, mode);
	
	return NULL;
}

struct FatfsDir* fatfsDirOpenAt(struct FatfsDir* dir, const char *name)
{
	struct FatfsFil *dirAsFile = (struct FatfsFil*)dir;
	struct FatFileLocator loc;
	
	if (!fatfsPrvSomethingFindAt(dir, name, 0x10000000, true, &loc, NULL, NULL))
		return false;
	
	return fatfsDirOpenWithLocator(dirAsFile->vol, &loc);
}

struct FatfsFil* fatfsFileOpenWithLocator(struct FatfsVol *vol, const struct FatFileLocator *locatorP, uint_fast8_t mode)
{
	const struct FatfsFileLocator *loc = (const struct FatfsFileLocator*)locatorP;
	struct FatfsFil *fil;
	
	if (!fatfsPrvOpenFlagsValidate(mode))
		return NULL;
	
	fil = fatfsPrvSomethingOpenWithClusNo(vol, loc->cluster, loc->size, loc->dirEntSec, loc->dirEntIdxInSec);
	if (fil) {
		if (!fatfsPrvNewlyOpenedFileApplyMode(fil, mode)) {
			
			fatfsFileClose(fil);
			fil = NULL;
		}
	}
	return fil;
}

static bool fatfsPrvChainFree(struct FatfsVol* vol, uint32_t firstClus)
{
	uint32_t thisClus, nextClus;
	bool ret = true;
	
	for (thisClus = firstClus; thisClus != CLUS_EOC; thisClus = nextClus) {
		
		nextClus = fatfsPrvGetNextClus(vol, thisClus);
		ret = fatfsPrvSetNextClus(vol, thisClus, 0) && ret;
	}
	
	return ret;
}

static uint32_t fatfsPrvChainNew(struct FatfsVol* vol, uint32_t guess, uint32_t *numClustersP, bool allowLess)		//numClustersP is in/out, return first cluster allocated
{
	uint32_t i, j, firstClus = 0, prevClus = 0;
	
	for (i = 0; i < *numClustersP; i++) {
		
		uint32_t thisClus = fatfsPrvFreeClusFind(vol, guess, prevClus);
		
		if (!thisClus)
			break;
		
		if (!i)
			firstClus = thisClus;
		else
			fatfsPrvSetNextClus(vol, prevClus, thisClus);
		
		prevClus = thisClus;
	}
	
	if (i != *numClustersP) {
		
		if (!allowLess) {	//free them all
			
			uint32_t thisClus, nextClus;
			
			for (thisClus = firstClus, j = 0; j < i; j++, thisClus = nextClus) {
				
				nextClus = fatfsPrvGetNextClus(vol, thisClus);
				if (!fatfsPrvSetNextClus(vol, thisClus, CLUS_EOC))
					return 0;
			}
			return 0;
		}
		else if (!i)
			return 0;
	}
	*numClustersP = i;
	if (!fatfsPrvSetNextClus(vol, prevClus, CLUS_EOC)) {
		
		//not likely, but better to leak a chain than to continue after such a failure
		return 0;
	}
	
	return firstClus;
}

static bool fatfsPrvRootReadFat16(struct FatfsFil* fil, void *bufP, uint32_t num)
{
	uint8_t *buf = (uint8_t*)bufP;
	
	while (num) {
		
		uint32_t secInRoot = fil->curPos / FATFS_DISK_SECT_SZ;
		uint32_t byteInSec = fil->curPos % FATFS_DISK_SECT_SZ;
		uint32_t numBytesPossible = FATFS_DISK_SECT_SZ - byteInSec;
		uint32_t now = numBytesPossible > num ? num : numBytesPossible;
		
		if (secInRoot >= fil->vol->rootDirSecs)
			return false;
		
		if (!fatfsPrvReadSecToCacheIfNeeded(fil->vol, fil->vol->root + secInRoot))
			return false;
		
		if (buf) {
			memcpy(buf, fil->vol->cache + byteInSec, now);
			buf += now;
		}
		fil->curPos += now;
		num -= now;
	}
	
	return true;
}

//ASSUMES there is space (and will fail if there is not), does not update any on-disk metadata, caller must take care of that
static bool fatfsSomethingWrite(struct FatfsFil* fil, const void *bufP, uint32_t numRequested, uint32_t *numWrittenP)
{
	uint32_t leftToWrite = numRequested;
	const uint8_t *buf = (const uint8_t*)bufP;
	
	while (leftToWrite) {
		
		uint32_t byteInSec = fil->curPos % FATFS_DISK_SECT_SZ;
		uint32_t curSecInFile = fil->curPos / FATFS_DISK_SECT_SZ;					//or in fat16 root dir
		uint32_t curSecInClus = curSecInFile % (1 << fil->vol->lg2SecPerClus);
		uint32_t curClusStartSec = fatfsPrvClusToSec(fil->vol, fil->curClus);
		uint32_t curSecOnDisk = fatfsPrvIsFat12_16rootDir(fil) ? fil->vol->root + fil->curPos / FATFS_DISK_SECT_SZ: curSecInClus + curClusStartSec;
		uint32_t nextClus;
		
		if (fatfsPrvIsEOC(fil)) {
			if (VERBOSE)
				pr(" $W: EOC\n");
			break;
		}
		
		if (leftToWrite < FATFS_DISK_SECT_SZ || byteInSec) {
			
			//we are not sector aligned or have less than a sector to write - use a buffer
			
			uint32_t bytesLeftInCurSec = FATFS_DISK_SECT_SZ - byteInSec;
			uint32_t now = (bytesLeftInCurSec > leftToWrite ? leftToWrite : bytesLeftInCurSec);
			
			if (!fatfsPrvReadSecToCacheIfNeeded(fil->vol, curSecOnDisk)) {
				if (VERBOSE)
					pr(" $W: cache %u\n", curSecOnDisk);
				return false;
			}
			
			memcpy(fil->vol->cache + byteInSec, buf, now);
			buf += now;
			fil->vol->cacheDirty = true;
			fil->curPos += now;
			leftToWrite -= now;
		}
		else {
			
			//we are sector aligned and have at least a sector to write, but maybe more?
			
			uint32_t secsNow, secsAvail, secsNeeded = leftToWrite / FATFS_DISK_SECT_SZ;
			
			if (fatfsPrvIsFat12_16rootDir(fil))							//root dir
				secsAvail = fil->vol->rootDirSecs - curSecInFile;
			else if (curSecOnDisk != curClusStartSec)					//we are sector aligned, but not cluster aligned
				secsAvail = fil->vol->sectorsPerClus - (curSecOnDisk - curClusStartSec);
			else														//we are cluster aligned
				secsAvail = fil->vol->sectorsPerClus;
			
			secsNow = secsAvail < secsNeeded ? secsAvail : secsNeeded;
			
			if (!fatfsPrvWriteSecs(fil->vol, curSecOnDisk, secsNow, buf)) {
				if (VERBOSE)
					pr(" $W: w %u+%u\n", curSecOnDisk, secsNow);
				return false;
			}
			buf += secsNow * FATFS_DISK_SECT_SZ;
			
			fil->curPos += secsNow * FATFS_DISK_SECT_SZ;
			leftToWrite -= secsNow * FATFS_DISK_SECT_SZ;
		}
		
		if (!fatfsPrvIsFat12_16rootDir(fil)) {
			
			//we might be at a cluster boundary. if so, grab the next cluster (except at end)
			if (fil->curPos % FATFS_DISK_SECT_SZ)
				continue;
			if ((fil->curPos / FATFS_DISK_SECT_SZ) % (1 << fil->vol->lg2SecPerClus))
				continue;
			
			//we ARE at a cluster boundary
			nextClus = fatfsPrvGetNextClus(fil->vol, fil->curClus);
			if (!nextClus) { 			//error getting the next cluster
				if (VERBOSE)
					pr(" $W: lno next clus\n");
				return false;
			}
			else if (nextClus == CLUS_EOC)
				fil->atEnd = true;
			else
				fil->curClus = nextClus;
		}
	}
	
	if (VERBOSE)
		pr(" $W: leftToWrite = %u\n", leftToWrite);
	
	if (numWrittenP)
		*numWrittenP = numRequested - leftToWrite;
	
	return true;
}

//bufP may be NULL to just dump data into the ether (useful for seeking)
static bool fatfsSomethingRead(struct FatfsFil* fil, void *bufP, uint32_t numRequested, uint32_t *numReadP, bool checkAgainstSizeLimit)
{
	uint32_t leftToRead = numRequested;
	uint8_t *buf = (uint8_t*)bufP;
	
	//stored size is only valid for files
	if (!checkAgainstSizeLimit) {
		if (leftToRead > fil->fileSz - fil->curPos)	//we can never read more bytes than we have
			leftToRead = fil->fileSz - fil->curPos;
	}
	
	while (leftToRead) {
		
		uint32_t byteInSec = fil->curPos % FATFS_DISK_SECT_SZ;
		uint32_t curSecInFile = fil->curPos / FATFS_DISK_SECT_SZ;					//or in fat16 root dir
		uint32_t curSecInClus = curSecInFile % (1 << fil->vol->lg2SecPerClus);
		uint32_t curClusStartSec = fatfsPrvClusToSec(fil->vol, fil->curClus);
		uint32_t curSecOnDisk = fatfsPrvIsFat12_16rootDir(fil) ? fil->vol->root + fil->curPos / FATFS_DISK_SECT_SZ: curSecInClus + curClusStartSec;
		uint32_t nextClus;
		
		if (fatfsPrvIsEOC(fil))
			break;
		
		if (leftToRead < FATFS_DISK_SECT_SZ || byteInSec) {
			
			//we are not sector aligned or have less than a sector to read - use a buffer
			
			uint32_t bytesLeftInCurSec = FATFS_DISK_SECT_SZ - byteInSec;
			uint32_t now = (bytesLeftInCurSec > leftToRead ? leftToRead : bytesLeftInCurSec);
			
			if (!fatfsPrvReadSecToCacheIfNeeded(fil->vol, curSecOnDisk))
				return false;
			
			if (buf) {
				
				memcpy(buf, fil->vol->cache + byteInSec, now);
				buf += now;
			}
			fil->curPos += now;
			leftToRead -= now;
		}
		else {
			
			//we are sector aligned and have at least a sector to read, but maybe more?
			
			uint32_t secsNow, secsAvail, secsNeeded = leftToRead / FATFS_DISK_SECT_SZ;
			
			if (fatfsPrvIsFat12_16rootDir(fil))							//root dir
				secsAvail = fil->vol->rootDirSecs - curSecInFile;
			else if (curSecOnDisk != curClusStartSec)					//we are sector aligned, but not cluster aligned
				secsAvail = fil->vol->sectorsPerClus - (curSecOnDisk - curClusStartSec);
			else														//we are cluster aligned
				secsAvail = fil->vol->sectorsPerClus;
			
			secsNow = secsAvail < secsNeeded ? secsAvail : secsNeeded;
			
			if (buf) {
				
				if (!fatfsPrvReadSecs(fil->vol, curSecOnDisk, secsNow, buf))
					return false;
				buf += secsNow * FATFS_DISK_SECT_SZ;
			}
			fil->curPos += secsNow * FATFS_DISK_SECT_SZ;
			leftToRead -= secsNow * FATFS_DISK_SECT_SZ;
		}
		
		if (!fatfsPrvIsFat12_16rootDir(fil)) {
			
			//we might be at a cluster boundary. if so, grab the next cluster (except at end)
			if (fil->curPos % FATFS_DISK_SECT_SZ)
				continue;
			if ((fil->curPos / FATFS_DISK_SECT_SZ) % (1 << fil->vol->lg2SecPerClus))
				continue;
			
			//we ARE at a cluster boundary
			nextClus = fatfsPrvGetNextClus(fil->vol, fil->curClus);
			if (!nextClus) 			//error getting the next cluster
				return false;
			else if (nextClus == CLUS_EOC)
				fil->atEnd = true;
			else
				fil->curClus = nextClus;
		}
	}
	
	if (numReadP)
		*numReadP = numRequested - leftToRead;
	
	return true;
}

static bool fatfsPrvDirEntryRawRead(struct FatfsFil *dirAsFile, struct FatDirEntry *dst, uint32_t *dirEntSecP, uint8_t *dirEntIdxInSecP)
{
	uint32_t numRead;
	uint16_t secOfst;
	
	if ((dirEntSecP || dirEntIdxInSecP) && !fatfsPrvCurPosToSecAndOfst(dirAsFile, dirEntSecP, &secOfst))
		return false;
	
	if (dirEntIdxInSecP)
		*dirEntIdxInSecP = secOfst / sizeof(struct FatDirEntry);
	
	return fatfsSomethingRead(dirAsFile, dst, sizeof(*dst), &numRead, true) && numRead == sizeof(*dst);
}

#if FATFS_USE_LFN_SUPPORT
	
	static void fatfsPrvWriteNthLfnChar(struct FatLfnEntry *lfne, uint_fast8_t n, uint_fast16_t code)
	{
		if (n < 5)
			lfne->chars1[n - 0] = code;
		else if (n < 5 + 6)
			lfne->chars2[n - 5] = code;
		else
			lfne->chars3[n - 5 - 6] = code;
	}
	
	static uint16_t fatfsPrvReadNthLfnChar(const struct FatLfnEntry *lfne, uint_fast8_t n)
	{
		if (n < 5)
			return lfne->chars1[n - 0];
		else if (n < 5 + 6)
			return lfne->chars2[n - 5];
		else
			return lfne->chars3[n - 5 - 6];
	}
	
	static bool fatfsPrvWriteLfnEntries(struct FatfsFil* dirAsFile, struct FatDirEntry *sfnEntry, const char *name, uint_fast8_t numLfnEntries)
	{
		struct FatLfnEntry lfne = {};
		uint_fast8_t i, j;
				
		//calculate the checksum
		for (i = 0; i < sizeof(sfnEntry->name); i++)
			lfne.checksum = (lfne.checksum >> 1) + ((lfne.checksum & 1) ? 0x80 : 0x00) + (uint8_t)(sfnEntry->name[i]);
		
		lfne.attrs = FATFS_ATTR_LFN;
		
		if (VERBOSE)
			pr("LFN: will write %u entries\n", numLfnEntries);
		
		for (i = numLfnEntries; i--;) {	//yes, you read that right
			
			struct Utf8state u8s = UTF8_STATE_STATIC_INITIALIZER;
			const char *curNamePtr = name;
			uint32_t code = 1 /* anything nonzero would do*/, bytesWritten;
		
			//skip to this point (validity is guaranteed - it's been checked
			for (j = 0; j < FATFS_CHARS_PER_LFN * i; j++) {
				
				while ((code = utf8inputByte(&u8s, *curNamePtr++)) == UTF_NO_OUTPUT);
			}
			
			//now emit the entry (incl null termination)
			for (j = 0; j < FATFS_CHARS_PER_LFN && code; j++) {
				
				while ((code = utf8inputByte(&u8s, *curNamePtr++)) == UTF_NO_OUTPUT);
				fatfsPrvWriteNthLfnChar(&lfne, j, code);
			}
			//fill with 0xffff as needed
			while (j < FATFS_CHARS_PER_LFN)
				fatfsPrvWriteNthLfnChar(&lfne, j++, 0xffff);
			
			//ordinal
			if ((lfne.ordinal = i + 1) == numLfnEntries)
				lfne.ordinal += FATFS_ORDINAL_MASK_LAST;
			
			//write
			if (!fatfsSomethingWrite(dirAsFile, &lfne, sizeof(struct FatLfnEntry), &bytesWritten) || bytesWritten != sizeof(struct FatLfnEntry)) { //bad things already happened
				if (VERBOSE)
					pr("lfn write error for entry idx %u with ordinal %02xh\n", i, lfne.ordinal);
				return false;
			}
		}
		
		return true;
	}
	
#endif

//somewhat optimally find a free SFN. Assumed we are called with a num suffixed with "~1" We'll scan for a number of them
//at a time to avoid repeated directory traversals
static bool fatfsPrvSfnIterateForNewDirEntry(struct FatfsDir *dir, struct FatDirEntry *newEntry)
{
	struct FatfsFil *dirAsFile = (struct FatfsFil*)dir;
	uint_fast8_t myPartialNameLen, myDigitLen = 1;
	uint32_t seenMask, base = 2, t, freeNumber;			//we start with "~1" thus 2 is the first one we care about
	struct FatDirEntry curEntry;
	
	//get a copy of the partial name and calc its len
	for (myPartialNameLen = 0; myPartialNameLen < 8 && newEntry->name[myPartialNameLen] != '~'; myPartialNameLen++);
	
	if (VERBOSE)
		pr("SFNseek looking for SFN for candidate '%s' of base len %u\n", newEntry->name, myPartialNameLen);
		
	while (1) {
		
		seenMask = 0;
		if (!fatfsDirRewind(dir))
			return false;
		
		while (fatfsPrvDirEntryRawRead(dirAsFile, &curEntry, NULL, NULL)) {
		
			uint_fast8_t i, curDigitLen, curPartialNameLen, expectedPartialNameLen;
			uint32_t curNumber = 0;
		
			if (curEntry.name[0] == 0xE5 || (curEntry.attrs & FATFS_ATTR_VOL_LBL))
				continue;
			
			if (!curEntry.name[0])
				break;
						
			//quickly compare extensions to reject some
			if (newEntry->name[8] != curEntry.name[8] || newEntry->name[9] != curEntry.name[9] || newEntry->name[10] != curEntry.name[10])
				continue;
						
			//find last tilde, it must exist and be, at the latest, second to last
			for (curPartialNameLen = 8; curPartialNameLen-- && curEntry.name[curPartialNameLen] != '~';);
			if (curPartialNameLen > 6)
				continue;
						
			//calc the number post-tilde, verify it IS a number, calc its length
			for (i = curPartialNameLen + 1, curDigitLen = 0; i < 8; i++) {
				
				if (curEntry.name[i] < '0' || curEntry.name[i] > '9')
					break;
				
				//no leading zeroes
				if (!curNumber && curEntry.name[i] == '0')
					break;
				
				curNumber = curNumber * 10 + curEntry.name[i] - '0';
				curDigitLen++;
			}
			for (; i < 8 &&curEntry.name[i] == ' ' ; i++);	//skip trailing space
			if (i != 8)										//verify we consumed it all
				continue;
						
			//if the number is not in our range (or already seen), continue without bothering to check if it is even our name (this is faster than that)
			if (curNumber < base || curNumber - base >= sizeof(seenMask) * 8 || (seenMask & (1 << (curNumber - base))))
				continue;
						
			expectedPartialNameLen = myPartialNameLen + curDigitLen > 7 ? 7 - curDigitLen : myPartialNameLen;
						
			if (expectedPartialNameLen != curPartialNameLen)
				continue;
			
			if (memcmp(curEntry.name, newEntry->name, expectedPartialNameLen))
				continue;
			
			if (VERBOSE)
				pr("SFNseek seen %d (base %d)\n", curNumber, base);
			
			//we have a new "seen" candidate - record it
			seenMask |= 1 << (curNumber - base);
		}
		if (seenMask != 0xffffffff)
			break;
		
		base += sizeof(seenMask) * 8;
	}
	
	//the number we SHOULD use is now calculateable
	freeNumber = base + __builtin_ctz(~seenMask);
	
	//let's see how long it is
	myDigitLen = 0;
	t = freeNumber;
	do {
		myDigitLen++;
	} while (t /= 10);
	
	//fill it it
	if (myPartialNameLen + myDigitLen > 7)
		myPartialNameLen = 7 - myDigitLen;
	newEntry->name[myPartialNameLen++] = '~';
	for (t = 0; t < myDigitLen; t++, freeNumber /= 10)
		newEntry->name[myPartialNameLen + myDigitLen - t - 1] = freeNumber % 10 + '0';
	
	if (VERBOSE)
		pr("SFNseek setttled on '%s'\n", newEntry->name);
	
	return true;
}

//create a dir entry in a directory. return success, provides location of SFN for futher changes. provided dirEntry is updated with name and used
static bool fatfsPrvDirEntryCreate(struct FatDirEntry *newEntry, struct FatfsDir *dir, const char *name, uint32_t *dirEntSecP, uint8_t *dirEntIdxInSecP)
{
#if FATFS_USE_LFN_SUPPORT
	struct FatfsLfnState lfn;
#endif

	uint32_t numEntriesNeeded, numEntriesFoundSoFar = 0, firstEntryPos = -1, endPos, bytesWritten;
	bool lookingForSpace = true, terminatorSeen = false, sfnMatchSeen = false;
	struct FatfsFil *dirAsFile = (struct FatfsFil*)dir;
	struct FatDirEntry curEntry;
	
	//process the name and bail out early if issues exist
	numEntriesNeeded = fatfsPrvNameCalculate(newEntry, name);
	if (!numEntriesNeeded)
		return false;
	
	if (VERBOSE)
		pr("fatfsPrvDirEntryCreate: %u entries needed\n", numEntriesNeeded);

#if FATFS_USE_LFN_SUPPORT
	fatfsPrvLfnMatchInit(&lfn, name, 0x10000000);
#endif

	//first pass: find space, check for long name perfect match, check for SFN conflict
	if (!fatfsDirRewind(dir))
		return false;
	while (fatfsPrvDirEntryRawRead(dirAsFile, &curEntry, NULL, NULL)) {
		
		bool curEntryIsFree;
		
#if FATFS_USE_LFN_SUPPORT
		if (!fatfsPrvLfnIngestEntry(&lfn, &curEntry))
			continue;
		
		//LFN match means we're done
		if (fatfsPrvLfnDidMatch(&lfn))
			return false;
#endif
		
		if (curEntry.attrs & FATFS_ATTR_VOL_LBL)				//skip volume labels and LFN from further code below
			continue;
		
		terminatorSeen = !curEntry.name[0];
		curEntryIsFree = (curEntry.name[0] == 0xE5) || terminatorSeen;
		
		//track space
		if (lookingForSpace) {
			
			if (curEntryIsFree) {
				
				if (!numEntriesFoundSoFar++)
					firstEntryPos = dirAsFile->curPos - sizeof(struct FatDirEntry);		//since we already read it, the position needs to be adjusted
				if (numEntriesFoundSoFar == numEntriesNeeded)
					lookingForSpace = false;
			}
			else
				numEntriesFoundSoFar = 0;
		}
		
		//track SFN match
		if (curEntry.name[0] == 0x05)
			curEntry.name[0] = 0xE5;
		if (!curEntryIsFree && !memcmp(curEntry.name, newEntry->name, sizeof(curEntry.name)))	//memcmp is ok since names are already normalized, both on disk and from fatfsPrvNameCalculate()
			sfnMatchSeen = true;
		
		//if we found space and we have an SFN match, no more need to read further (we know we'll need to iterate anyways
		if (!lookingForSpace && sfnMatchSeen)
			break;
	}
	
	endPos = dirAsFile->curPos;		//fatfsPrvSfnIterateForNewDirEntry() may move the pointer, but code below expects to find it at the end
	
	if (VERBOSE)
		pr("loop over, sfnMatchSeen = %d, lookingForSpace = %d\n", sfnMatchSeen, lookingForSpace);
	
	if (sfnMatchSeen) {
		
		if (!fatfsPrvSfnIterateForNewDirEntry(dir, newEntry))
			return false;
	}
	
	if (lookingForSpace) {
		
		uint32_t extraEntriesNeeded, extraSectorsNeeded, extraClustersNeeded, numNewClusters, firstNewCluster;
		
		if (VERBOSE)
			pr(" ** need more space\n");
		
		if (!fatfsPrvSomethingSeek(dirAsFile, endPos)) {
			if (VERBOSE)
				pr("seek 0 error\n");
			return false;
		}
		
		//we get here if we failed to find enough entries, numEntriesFoundSoFar tells us how many usable
		//entries are at the very end, so we know by how much to extend. It is possible that we'll need
		//to extend by two clusters. this happens if cluster size is one sector, we have a long name, and
		//the directory does not have many free entries in the end. We should also remember that the root
		//directory in FAT16 does not extend, so if we get there in that case, we fail.
		if (fatfsPrvIsFat12_16rootDir(dirAsFile))
			return false;
	
		//for all other chases, we MUST be at end. verify. Keep in mind that no directory may ever exist
		//without a cluster chain. At least one cluster is required. All non-root dirs have a "." and
		//".." dir entries. FAT32 root dir does not, but the volume header necessarily gives its cluster
		//number, thus mandating it have at least one. That is to say that if we're "atEnd", curClus is
		//necessarily valid. We assert that here just in case
		if (!dirAsFile->atEnd || !dirAsFile->curClus) {
			if (VERBOSE)
				pr("consistency issue\n");
			return false;
		}
	
		//extend the directory
		extraEntriesNeeded = numEntriesNeeded - numEntriesFoundSoFar;
		extraSectorsNeeded = (extraEntriesNeeded * sizeof(struct FatDirEntry) + FATFS_DISK_SECT_SZ - 1) / FATFS_DISK_SECT_SZ;
		extraClustersNeeded = (extraSectorsNeeded + dirAsFile->vol->sectorsPerClus - 1) / (1 << dirAsFile->vol->lg2SecPerClus);
		numNewClusters = extraClustersNeeded;
		firstNewCluster = fatfsPrvChainNew(dirAsFile->vol, dirAsFile->curClus, &numNewClusters, false);
		if (!firstNewCluster)
			return false;
		
		if (VERBOSE)
			pr("allocated a new chain of %u clusters starting at %u\n", numNewClusters, firstNewCluster);
		
		//if we had previous entries, we'll need to rewind to them, after we attach a new cluste rand set it as current it'll be harder, so do it now (optimization)
		if (numEntriesFoundSoFar && !fatfsPrvSomethingSeek(dirAsFile, firstEntryPos)) {
			if (VERBOSE)
				pr("seek 1 error\n");
			return false;
		}

		//link the new chain into our existing chain and update dir's state as no longer at end
		if (!fatfsPrvSetNextClus(dirAsFile->vol, dirAsFile->curClus, firstNewCluster)) {
			if (VERBOSE)
				pr("cannot set new cluster for dir\n");
			return false;
		}
		
		//if we're at end, adjust the structure of the file to point properly to the next cluster
		if (dirAsFile->atEnd) {
			
			dirAsFile->atEnd = false;
			dirAsFile->curClus = firstNewCluster;
		}
		
		//record that we DO need a new terminator, if there is space
		terminatorSeen = true;
		
		if (!numEntriesFoundSoFar)	//we had no space at all, so we start in the new space
			firstEntryPos = dirAsFile->curPos;
	}
	
	if (VERBOSE)
		pr("firstEntryPos = %u\n", firstEntryPos);
	
	//go there
	if (!fatfsPrvSomethingSeek(dirAsFile, firstEntryPos)) {
		if (VERBOSE)
			pr("seek error\n");
		return false;
	}
	
	if (VERBOSE)
		pr("1 dirAsFile->curPos = %u, clus = %u\n", dirAsFile->curPos, dirAsFile->curClus);
	
	//LFN entries come first
	#if FATFS_USE_LFN_SUPPORT
		
		if (!fatfsPrvWriteLfnEntries(dirAsFile, newEntry, name, numEntriesNeeded - 1)) {
			if (VERBOSE)
				pr("lfn write err\n");
			return false;
		}
		
	#endif
	
	if (VERBOSE)
		pr("2 dirAsFile->curPos = %u, clus = %u\n", dirAsFile->curPos, dirAsFile->curClus);
	
	//we're now ready to write the SFN entry. Also a good time to report its location, if requested
	if (dirEntSecP || dirEntIdxInSecP) {
	
		uint16_t sfnByteOfstInSec;
		
		if (!fatfsPrvCurPosToSecAndOfst(dirAsFile, dirEntSecP, &sfnByteOfstInSec)) {
			if (VERBOSE)
				pr("get loc err\n");
			return false;
		}
		
		if (dirEntIdxInSecP)
			*dirEntIdxInSecP = sfnByteOfstInSec / sizeof(struct FatDirEntry);
	}	
	
	//we can now write the SFN entry
	if (!fatfsSomethingWrite(dirAsFile, newEntry, sizeof(struct FatDirEntry), &bytesWritten) || bytesWritten != sizeof(struct FatDirEntry)) {
		if (VERBOSE)
			pr("sfn write err\n");
		return false;
	}
	
	if (VERBOSE)
		pr("3 dirAsFile->curPos = %u, clus = %u\n", dirAsFile->curPos, dirAsFile->curClus);
	
	//and now we can write the terminator if there is space and demand
	if (terminatorSeen && !fatfsPrvIsEOC(dirAsFile)) {
		
		memset(&curEntry, 0, sizeof(curEntry));
		if (!fatfsSomethingWrite(dirAsFile, &curEntry, sizeof(struct FatDirEntry), &bytesWritten) || bytesWritten != sizeof(struct FatDirEntry)) {
			if (VERBOSE)
				pr("sfn write error\n");
			return false;
		}
	}
	
	return true;
}

static void fatfsPrvFillLocator(struct FatFileLocator *locatorP, uint32_t size, uint32_t clus, uint32_t dirEntSec, uint8_t dirEntIdxInSec)
{
	if (locatorP) {
		
		struct FatfsFileLocator *loc = (struct FatfsFileLocator*)locatorP;
		
		loc->size = size;
		loc->cluster = clus;
		loc->dirEntSec = dirEntSec;
		loc->dirEntIdxInSec = dirEntIdxInSec;
	}
}

bool fatfsFileCreateAt(struct FatfsDir* dir, const char *name, struct FatFileLocator *locatorP)
{
	struct FatDirEntry de = {};
	uint32_t dirEntSec;
	uint8_t dirEntIdxInSec;
	
	if (!fatfsPrvDirEntryCreate(&de, dir, name, &dirEntSec, &dirEntIdxInSec))
		return false;
	
	fatfsPrvFillLocator(locatorP, 0, 0, dirEntSec, dirEntIdxInSec);
	return true;
}

bool fatfsDirCreateAt(struct FatfsDir* dir, const char *name, struct FatFileLocator *locatorP)
{
	uint32_t dirEntSec, dirCluster, numClustersForDir = 1, parentCluster;
	struct FatfsFil *dirAsFile = (struct FatfsFil*)dir;
	struct FatDirEntry dirEnt = {};
	uint8_t dirEntIdxInSec;
	bool ret = true;
		
	dirCluster = fatfsPrvChainNew(dirAsFile->vol, dirAsFile->firstClus, &numClustersForDir, false);
	if (!dirCluster)
		return false;
	
	if (VERBOSE) {
	
		pr("fatfsDirCreate: new chain created: %u clusters starting with %u\n", numClustersForDir, dirCluster);
		pr("fat now says: 0x%08x\n", fatfsPrvGetNextClus(dirAsFile->vol, dirCluster));
	}
	
	dirEnt.attrs = FATFS_ATTR_DIR;
	dirEnt.clusNumHi = dirCluster >> 16;
	dirEnt.clusNum = dirCluster;
	
	if (!fatfsPrvDirEntryCreate(&dirEnt, dir, name, &dirEntSec, &dirEntIdxInSec)) {
		
		//release the cluster we had allocated for the dir
		(void)fatfsPrvSetNextClus(dirAsFile->vol, dirCluster, 0);
		return false;
	}
	
	//make the dot entry
	memcpy(dirEnt.name, ".           ", 11);
	dirEnt.clusNumHi = dirCluster >> 16;
	dirEnt.clusNum = dirCluster;
	ret = fatfsPrvDirEntrySpecificWrite(&dirEnt, dirAsFile->vol, fatfsPrvClusToSec(dirAsFile->vol, dirCluster), 0) && ret;

	//make the dotdot entry
	parentCluster = fatfsPrvIsFat12_16rootDir(dirAsFile) ? 0 : dirAsFile->firstClus;
	dirEnt.name[1] = '.';
	dirEnt.clusNumHi = parentCluster >> 16;
	dirEnt.clusNum = parentCluster;
	ret = fatfsPrvDirEntrySpecificWrite(&dirEnt, dirAsFile->vol, fatfsPrvClusToSec(dirAsFile->vol, dirCluster), 1) && ret;
	
	//terminate
	dirEnt.name[0] = 0;
	dirEnt.clusNumHi = 0;
	dirEnt.clusNum = 0;
	ret = fatfsPrvDirEntrySpecificWrite(&dirEnt, dirAsFile->vol, fatfsPrvClusToSec(dirAsFile->vol, dirCluster), 2) && ret;
	
	fatfsPrvFillLocator(locatorP, 0, dirCluster, dirEntSec, dirEntIdxInSec);
	
	return ret;
}

static struct FatfsFil* fatfsPrvSomethingOpenByPath(struct FatfsVol *vol, const char *path, uint32_t pathMaxLen, bool wantDir)
{
	struct FatfsFileLocator loc;
	
	if (!fatfsPrvSomethingFind(vol, path, pathMaxLen, wantDir, (struct FatFileLocator*)&loc))
		return NULL;

	return fatfsPrvSomethingOpenWithClusNo(vol, loc.cluster, loc.size, loc.dirEntSec, loc.dirEntIdxInSec);
}

//find the length of the path component leading up to the filename. negative on error
static int32_t fatfsPrvFindPathLen(const char *path)
{
	const char *pathEnd = path, *t;
	
	
	//find the path up to the file
	for (t = path; *t; t++) {
		
		if (*t == '/' || *t == '\\')
			pathEnd = t + 1;
	}
	
	if (!*pathEnd)
		return -1;
	
	return pathEnd - path;
}

bool fatfsDirCreate(struct FatfsVol *vol, const char *path, struct FatFileLocator *locatorP)
{
	struct FatfsFil *parentDirAsFile;
	struct FatfsDir *parentDir;
	int32_t pathLen;
	bool ret;
	
	pathLen = fatfsPrvFindPathLen(path);
	if (pathLen < 0)
		return false;
	
	parentDirAsFile = fatfsPrvSomethingOpenByPath(vol, path, pathLen, true);
	parentDir = (struct FatfsDir*)parentDirAsFile;
	if (!parentDirAsFile)
		return false;
	
	ret = fatfsDirCreateAt(parentDir, path + pathLen, locatorP);
	
	return fatfsDirClose(parentDir) && ret;
}

static struct FatfsFil* fatfsFileCreateAndOpen(struct FatfsVol *vol, const char *path)
{
	struct FatfsFil *parentDirAsFile;
	struct FatfsDir *parentDir;
	struct FatDirEntry de = {};
	uint8_t dirEntIdxInSec;
	uint32_t dirEntSec;
	int32_t pathLen;
	bool ret;
	
	pathLen = fatfsPrvFindPathLen(path);
	if (pathLen < 0)
		return NULL;
	
	parentDirAsFile = fatfsPrvSomethingOpenByPath(vol, path, pathLen, true);
	parentDir = (struct FatfsDir*)parentDirAsFile;
	if (!parentDirAsFile)
		return NULL;

	ret = fatfsPrvDirEntryCreate(&de, parentDir, path + pathLen, &dirEntSec, &dirEntIdxInSec);
	if (!fatfsDirClose(parentDir) || !ret)
		return NULL;
	
	return fatfsPrvSomethingOpenWithClusNo(vol, 0, 0, dirEntSec, dirEntIdxInSec);
}

struct FatfsFil* fatfsFileOpen(struct FatfsVol *vol, const char *path, uint_fast8_t mode)
{
	struct FatfsFil *fil;

	if (!fatfsPrvOpenFlagsValidate(mode))
		return NULL;
	
	fil = fatfsPrvSomethingOpenByPath(vol, path, 0x10000000, false);
	if (fil)
		fatfsPrvNewlyOpenedFileApplyMode(fil, mode);
	else if (mode & OPEN_MODE_CREATE) {
		fil = fatfsFileCreateAndOpen(vol, path);
		if (fil)
			fil->writeAllowed = true;	//by definition
	}
	
	return fil;
}

struct FatfsDir* fatfsDirOpen(struct FatfsVol *vol, const char *path)
{
	return (struct FatfsDir*)fatfsPrvSomethingOpenByPath(vol, path, 0x10000000, true);	
}

struct FatfsDir* fatfsRootDirOpen(struct FatfsVol *vol)
{
	return (struct FatfsDir*)fatfsPrvSomethingOpenWithClusNo(vol, fatfsPrvVolIsFat32(vol) ? vol->root : CLUS_IS_ROOT_DIR, 0, 0, 0);
}

static bool fatfsPrvSomethingClose(struct FatfsFil *fil, bool isDir)
{
	fatfsPrvFreeFile(fil);
	return fatfsPrvCacheClean(fil->vol);
}

bool fatfsFileClose(struct FatfsFil* fil)
{
	return fatfsPrvSomethingClose(fil, false);
}

bool fatfsDirClose(struct FatfsDir* dir)
{
	return fatfsPrvSomethingClose((struct FatfsFil*)dir, false);
}

static bool fatfsPrvSomethingDeleteAt(struct FatfsDir* dir, const char *name, bool isDir)
{
	struct FatfsFil *dirAsFile = (struct FatfsFil*)dir;
	uint32_t firstEntryPosInDir, numWritten;
	bool ret = true, isLastEntryInDir;
	struct FatfsFileLocator loc;
	uint8_t numDirEntries;
	struct FatDirEntry de;
	uint_fast8_t i;
	
	if (!fatfsPrvSomethingFindAt(dir, name, 0x10000000, isDir, (struct FatFileLocator*)&loc, &firstEntryPosInDir, &numDirEntries))
		return false;

	if (isDir) {
		
		struct FatfsDir *dirUnderTest = fatfsDirOpenWithLocator(dirAsFile->vol, (struct FatFileLocator*)&loc);
		struct FatfsFil *dutAsFile = (struct FatfsFil*)dirUnderTest;
		bool canDelete;
		
		if (!dirUnderTest)
			return false;
		
		//cannot delete root or nonempty dirs
		canDelete = !fatfsPrvIsRootDir(dutAsFile) && !fatfsPrvDirRead(dirUnderTest, NULL, NULL, NULL, NULL, NULL, NULL, false);
		
		if (!fatfsDirClose(dirUnderTest) || !canDelete)
			return false;
	}
	
	if (loc.cluster)
		ret = fatfsPrvChainFree(dirAsFile->vol, loc.cluster) && ret;
	
	//seek just past our entrie(s)
	if (!fatfsPrvSomethingSeek(dirAsFile, firstEntryPosInDir + sizeof(struct FatDirEntry) * numDirEntries))
		return false;
	
	//check if there are any entries that follow. this allows us to cleanup for other users of this FS who did not truncate dir chains as needed
	isLastEntryInDir = !fatfsPrvDirRead(dir, NULL, NULL, NULL, NULL, NULL, NULL, true);
	
	//if this is the last entry, maybe some previous entries are also empty and can be discarded? We could go looking but i'd rather not

	
	//seek to the entries
	if (!fatfsPrvSomethingSeek(dirAsFile, firstEntryPosInDir))
		return false;
		
	//delete one entry at a time - you would think that this is not necessary if this is the last entry and we're about to mark it as such, but you'd be wrong.
	//Linux goes on reading anyways...

	for (i = 0; i < numDirEntries; i++) {
		
		struct FatLfnEntry *lfne = (struct FatLfnEntry*)&de;
		uint8_t idxInSec;
		uint32_t sec;
		
		if (!fatfsPrvDirEntryRawRead(dirAsFile, &de, &sec, &idxInSec)) {
			ret = false;
			continue;
		}
		
		if (i + 1 != numDirEntries) {
			if (de.attrs != FATFS_ATTR_LFN)
				return false;
			lfne->ordinal |= FATFS_ORDINAL_MASK_DEL;
		}
		else if (de.attrs & FATFS_ATTR_VOL_LBL)
			return false;
		else
			de.name[0] = 0xe5;
		
		ret = fatfsPrvDirEntrySpecificWrite(&de, dirAsFile->vol, sec, idxInSec) && ret;
	}
	
	if (isLastEntryInDir) {
		
		//seek to the entries
		if (!fatfsPrvSomethingSeek(dirAsFile, firstEntryPosInDir))
			return false;
		
		//our entry does NOT begin at a cluster start - we NEED to write a terminator
		if (fatfsPrvIsFat12_16rootDir(dirAsFile) || !firstEntryPosInDir || firstEntryPosInDir % FATFS_DISK_SECT_SZ || (firstEntryPosInDir / FATFS_DISK_SECT_SZ % (1 << dirAsFile->vol->lg2SecPerClus))) {
			
			memset(&de, 0, sizeof(struct FatDirEntry));
			ret = fatfsSomethingWrite(dirAsFile, &de, sizeof(struct FatDirEntry), &numWritten) && numWritten == sizeof(struct FatDirEntry) && ret;
		}
		
		if (!fatfsPrvIsFat12_16rootDir(dirAsFile) && firstEntryPosInDir) {
			//if the entry prior to ours is in a different cluster than the end of ours, we should truncate the chain
			//this cannot happen for fat16 root dir, nor can it happen for the first direntry in the dir (but this is unlikely since that is usually a dotentry)
			uint32_t lastNeededCluster, firstUselessCluster;
				
			if (!fatfsPrvSomethingSeek(dirAsFile, firstEntryPosInDir - 1))
				return false;
			
			lastNeededCluster = dirAsFile->curClus;
			firstUselessCluster = fatfsPrvGetNextClus(dirAsFile->vol, lastNeededCluster);
						
			if (firstUselessCluster && firstUselessCluster != lastNeededCluster && firstUselessCluster != CLUS_EOC) {
				
				ret = fatfsPrvSetNextClus(dirAsFile->vol, lastNeededCluster, CLUS_EOC) && ret;
				ret = fatfsPrvChainFree(dirAsFile->vol, firstUselessCluster) && ret;
			}
		}
	}
	
	return ret;
}

static bool fatfsPrvSomethingDeleteWithPath(struct FatfsVol *vol, const char *path, bool isDir)
{
	struct FatfsFil *dirAsFile;
	struct FatfsDir* dir;
	int32_t pathLen;
	bool ret;
	
	pathLen = fatfsPrvFindPathLen(path);
	if (pathLen < 0)
		return NULL;
	
	dirAsFile = fatfsPrvSomethingOpenByPath(vol, path, pathLen, true);
	dir = (struct FatfsDir*)dirAsFile;
	if (!dirAsFile)
		return NULL;
	
	ret = fatfsPrvSomethingDeleteAt(dir, path + pathLen, isDir);
	
	return fatfsDirClose(dir) && ret;
}

bool fatfsFileDelete(struct FatfsVol *vol, const char *path)
{
	return fatfsPrvSomethingDeleteWithPath(vol, path, false);
}

bool fatfsDirDelete(struct FatfsVol *vol, const char *path)
{
	return fatfsPrvSomethingDeleteWithPath(vol, path, false);
}

bool fatfsFileDeleteAt(struct FatfsDir* dir, const char *name)
{
	return fatfsPrvSomethingDeleteAt(dir, name, false);
}

bool fatfsDirDeleteAt(struct FatfsDir* dir, const char *name)
{
	return fatfsPrvSomethingDeleteAt(dir, name, true);
}

static void fatfsPrvFileOrDirRewind(struct FatfsFil *fil)
{
	fil->curClus = fil->firstClus;
	fil->curPos = 0;
	
	fil->atEnd = !fatfsPrvIsFat12_16rootDir(fil) && !fil->curClus;	//the beginning can also be the end
}

bool fatfsDirRewind(struct FatfsDir *dir)
{
	fatfsPrvFileOrDirRewind((struct FatfsFil*)dir);
	return true;
}

bool fatfsFileTruncate(struct FatfsFil* fil, uint32_t sz)
{
	uint32_t sectorsKept, clustersKept, curClus, prevClus, i;
	struct FatDirEntry de;
	bool ret = true;
	
	if (!fil->fileSz || fil->fileSz < sz)		//dirs have a size of zero
		return false;
	if (fil->fileSz == sz)
		return true;
	
	if (!sz) {				//truncate to nothing
		
		if (fil->firstClus)
			ret = fatfsPrvChainFree(fil->vol, fil->firstClus) && ret;
		fil->firstClus = 0;
		fil->curClus = 0;
		fil->atEnd = true;
	}
	else {					//truncate to a nonzero size
		
		sectorsKept = (sz + FATFS_DISK_SECT_SZ - 1) / FATFS_DISK_SECT_SZ;
		clustersKept = (sectorsKept + fil->vol->sectorsPerClus - 1) / (1 << fil->vol->lg2SecPerClus);
	
		for (i = 0, curClus = fil->firstClus, prevClus = 0; i < clustersKept && curClus && curClus != CLUS_EOC; i++, prevClus = curClus, curClus = fatfsPrvGetNextClus(fil->vol, curClus));
		if (i != clustersKept)
			return false;
		
		//now prevClus is the last cluster we keep, curClus is the first we toss, it will be EOC if we are not removing any clusters
		if (curClus != CLUS_EOC) {
			
			ret = fatfsPrvSetNextClus(fil->vol, prevClus, CLUS_EOC) && ret;
			ret = fatfsPrvChainFree(fil->vol, curClus) && ret;

			//if file position is a multiple of a cluster boundary, set EOC flag as we just removed the further clusters, set "current" cluster correctly
			if (!(fil->curPos % FATFS_DISK_SECT_SZ) && !(fil->curPos / FATFS_DISK_SECT_SZ % (1 << fil->vol->lg2SecPerClus))) {
				fil->curClus = prevClus;
				fil->atEnd = true;
			}
		}
	}
	
	fil->fileSz = sz;
	if (fil->curPos > fil->fileSz)
		fatfsPrvFileOrDirRewind(fil);
	
	//update metadata
	ret = fatfsPrvDirEntrySpecificRead(&de, fil->vol, fil->dirEntSec, fil->dirEntIdxInSec) && ret;
	fatfsPrvSetLE32(&de.fileSz, fil->fileSz);
	fatfsPrvSetLE16(&de.clusNumHi, fil->firstClus >> 16);
	fatfsPrvSetLE16(&de.clusNum, fil->firstClus);
	ret = fatfsPrvDirEntrySpecificWrite(&de, fil->vol, fil->dirEntSec, fil->dirEntIdxInSec) && ret;
	
	return ret;
}

bool fatfsFileRead(struct FatfsFil* fil, void *buf, uint32_t num, uint32_t *numReadP)
{
	return fatfsSomethingRead(fil, buf, num, numReadP, false);
}

bool fatfsFileWrite(struct FatfsFil* fil, const void *bufP, uint32_t bytesToWrite, uint32_t *numWrittenP)
{
	uint32_t numOriginallyWanted, curFileSzOnDisk, spaceAvailNow, numDone, now, bytesWritten;
	const uint8_t *buf = (const uint8_t*)bufP;
	
	//we have file size maximums to respect
	if (bytesToWrite > 0xffffffff - fil->curPos)
		bytesToWrite = 0xffffffff - fil->curPos;
	numOriginallyWanted = bytesToWrite;
	
	//let's see how much space we have
	curFileSzOnDisk = ((((fil->fileSz + ((FATFS_DISK_SECT_SZ << fil->vol->lg2SecPerClus) - 1)) / FATFS_DISK_SECT_SZ) >> fil->vol->lg2SecPerClus) << fil->vol->lg2SecPerClus) * FATFS_DISK_SECT_SZ;
	spaceAvailNow = curFileSzOnDisk - fil->curPos;
	
	if (VERBOSE)
		pr("FW: space avail: %d\n", spaceAvailNow);
	
	//write now into space we DO have
	if (spaceAvailNow && bytesToWrite) {
		
		now = spaceAvailNow > bytesToWrite ? bytesToWrite : spaceAvailNow;
		
		if (!fatfsSomethingWrite(fil, buf, now, &numDone))
			return false;
		if (numDone != now) {
			
			if (numWrittenP)
				*numWrittenP = numDone;
			goto done;
		}
		bytesToWrite -= numDone;
		buf += numDone;
	}
	
	if (VERBOSE && bytesToWrite) {
		pr("FW: still need to write(1) %u\n", bytesToWrite);
		pr("pos %u, curClus %u firstClus%u, atEnd %u\n", fil->curPos, fil->curClus, fil->firstClus, fil->atEnd);
	}
	//we're here because we need more space. We should be at end. But due to file chain being too long, we might actually not be	
	while (bytesToWrite && !fatfsPrvIsEOC(fil)) {
		
		spaceAvailNow = FATFS_DISK_SECT_SZ << fil->vol->lg2SecPerClus;
		now = spaceAvailNow > bytesToWrite ? bytesToWrite : spaceAvailNow;
		
		if (!fatfsSomethingWrite(fil, buf, now, &numDone))
			return false;
		if (numDone != now)
			goto done;
		bytesToWrite -= numDone;
		buf += numDone;
	}
	
	if (VERBOSE && bytesToWrite)
		pr("FW: still need to write(2) %u\n", bytesToWrite);
	
	//we are now sure that if we have any more to write, we'll need more clusters
	if (bytesToWrite) {
		
		uint32_t clustersNeeded = ((bytesToWrite + ((FATFS_DISK_SECT_SZ << fil->vol->lg2SecPerClus) - 1)) / FATFS_DISK_SECT_SZ) >> fil->vol->lg2SecPerClus;
		uint32_t newClus, numClustersGotten = clustersNeeded;
		
		if (VERBOSE)
			pr("FW: need space for %u bytes -> need %u clusters\n", bytesToWrite, clustersNeeded);
		
		//due to the above cod we are sure we're at EOC
		newClus = fatfsPrvChainNew(fil->vol, fil->curClus, &numClustersGotten, true);
		if (!newClus)
			goto done;
		
		if (VERBOSE)
			pr("FW: alloced chain of %u clusters starting with %u\n", numClustersGotten, newClus);
		
		//attach chain and update object state
		if (fil->curClus) {
			
			if (!fatfsPrvSetNextClus(fil->vol, fil->curClus, newClus))
				return false;
		}
		else {	//first
			
			fil->firstClus = newClus;
		}
		fil->atEnd = false;
		fil->curClus = newClus;
		
		spaceAvailNow = numClustersGotten * FATFS_DISK_SECT_SZ << fil->vol->lg2SecPerClus;
		now = spaceAvailNow > bytesToWrite ? bytesToWrite : spaceAvailNow;
		
		if (!fatfsSomethingWrite(fil, buf, now, &numDone))
			return false;
		bytesToWrite -= numDone;
		buf += numDone;
	}
	
done:
	bytesWritten = numOriginallyWanted - bytesToWrite;
	if (numWrittenP)
		*numWrittenP = bytesWritten;
	
	if (fil->curPos > fil->fileSz) {	//update metadata
		
		struct FatDirEntry de;
		fil->fileSz = fil->curPos;
		
		if (!fatfsPrvDirEntrySpecificRead(&de, fil->vol, fil->dirEntSec, fil->dirEntIdxInSec))
			return false;
		fatfsPrvSetLE32(&de.fileSz, fil->fileSz);
		fatfsPrvSetLE16(&de.clusNumHi, fil->firstClus >> 16);
		fatfsPrvSetLE16(&de.clusNum, fil->firstClus);
		if (!fatfsPrvDirEntrySpecificWrite(&de, fil->vol, fil->dirEntSec, fil->dirEntIdxInSec))
			return false;
	}
	return true;
}

static bool fatfsPrvSomethingSeek(struct FatfsFil* fil, uint32_t pos)	//assumes you know what you are doing size-wise
{
	uint32_t bytesToSkip, bytesSkipped;
	
	if (fil->curPos == pos)
		return true;
	
	if (fil->curPos > pos) {				//seeking backwards is complex, but some simple cases exist
		
		uint32_t curOfstInCluster = fil->curPos % (FATFS_DISK_SECT_SZ << fil->vol->lg2SecPerClus);	//not valid for fat16 root
		uint32_t backBy = fil->curPos - pos;
		
		//if we're in same cluster, in fat16 root dir, or at end of cluster chain (where we still remember current cluster num) - it is easy
		if (backBy < curOfstInCluster || fatfsPrvIsFat12_16rootDir(fil) || (fil->atEnd && backBy < (FATFS_DISK_SECT_SZ << fil->vol->lg2SecPerClus))) {
			
			fil->atEnd = false;			//not at end anymore, even if we were
			fil->curPos -= backBy;
			return true;
		}
		
		//some other case - we'll need to rewind and go forward
		fatfsPrvFileOrDirRewind(fil);
	}
	
	bytesToSkip = (pos - fil->curPos);
	if (VERBOSE)
		pr("seek: need to skip %u fom pos %u\n", bytesToSkip, fil->curPos);
	
	return fatfsSomethingRead(fil, NULL, bytesToSkip, &bytesSkipped, true) && bytesToSkip == bytesSkipped;
}

bool fatfsFileSeek(struct FatfsFil* fil, uint32_t pos)
{
	if (fil->fileSz < pos)					//seeking to end of file is OK (pos == fileSz), but past that is not OK
		return false;
	
	return fatfsPrvSomethingSeek(fil, pos);
}

uint32_t fatfsFileTell(struct FatfsFil* fil)
{
	return fil->curPos;
}

uint32_t fatfsFileGetSize(struct FatfsFil* fil)
{
	return fil->fileSz;
}

struct FatfsDir* fatfsDirOpenWithLocator(struct FatfsVol *vol, const struct FatFileLocator *locatorP)
{
	struct FatfsFileLocator *loc = (struct FatfsFileLocator*)locatorP;
	
	return (struct FatfsDir*)fatfsPrvSomethingOpenWithClusNo(vol, loc->cluster, 0, loc->dirEntSec, loc->dirEntIdxInSec);
}

#if FATFS_USE_LFN_SUPPORT

	static void fatfsPrvFilenameCharWrite(char **dstP, char chr)
	{	
		uint32_t codePt = utfFrom8bit((uint8_t)chr);
		
		if (codePt >= 0x10000)
			codePt = UTF_REPLACEMENT_CHAR;
	
		utf8write((uint8_t**)dstP, utfFrom8bit((uint8_t)chr));
	}
	
	static void fatfsPrvShortNameExportAsUtf(char* dst, const uint8_t *sfnEntryName)
	{
		uint_fast8_t i;
			
		for (i = 0; i < 8 && sfnEntryName[i] != ' '; i++)
			fatfsPrvFilenameCharWrite(&dst, sfnEntryName[i]);
		
		if (sfnEntryName[8] != ' ') {
			*dst++ = '.';
			fatfsPrvFilenameCharWrite(&dst, sfnEntryName[8]);
			if (sfnEntryName[9] != ' ') {
				fatfsPrvFilenameCharWrite(&dst, sfnEntryName[9]);
				if (sfnEntryName[10] != ' ')
					fatfsPrvFilenameCharWrite(&dst, sfnEntryName[10]);
			}
		}
		*dst = 0;
	}
#endif

static void fatfsPrvShortNameExportAsCodepage(char* sfnNameFixed, const uint8_t *sfnEntryName)
{
	uint_fast8_t i, j;
	
	//convert name to human readable (add dot, remove spaces)
	for (i = 0, j = 0; i < 8; i++) {
		
		sfnNameFixed[i] = sfnEntryName[i];
		if (sfnEntryName[i] != ' ')
			j = i;
	}
	sfnNameFixed[++j] = '.';
	for (i = 0; i < 3 && sfnEntryName[8 + i] != ' '; i++)
		sfnNameFixed[j + i + 1] = sfnEntryName[8 + i];
	if (!i)
		sfnNameFixed[j] = 0;
	else
		sfnNameFixed[j + i + 1] = 0;
}

static void fatfsPrvShortNameExport(const struct FatDirEntry *entry, char *dst)
{
#if FATFS_USE_LFN_SUPPORT
	fatfsPrvShortNameExportAsUtf(dst, entry->name);
#else
	fatfsPrvShortNameExportAsCodepage(dst, entry->name);
#endif
}

#if FATFS_USE_LFN_SUPPORT

	static bool fatfsPrvCodepageToUtf8CaselessEqualSfn(const uint8_t *sfnEntry, const char *utfName, uint32_t utfNameMaxLen)
	{
		struct Utf8state u8s = UTF8_STATE_STATIC_INITIALIZER;
		const char *utfNameEnd = utfName + utfNameMaxLen;
		char sfnNameFixed[13];
		uint_fast8_t i;
		
		//convert name to human readable (add dot, remove spaces)
		fatfsPrvShortNameExportAsCodepage(sfnNameFixed, sfnEntry);
		
		for (i = 0; i < sizeof(sfnNameFixed); i++) {		//this will compare the final terminator too. clever, eh?
			
			uint32_t ret;
			
			//get code
			while (1) {
				ret = utfName < utfNameEnd ? utf8inputByte(&u8s, *utfName++) : 0;
				
				if (ret == UTF_ERROR)
					return false;
				
				if (ret == UTF_NO_OUTPUT)
					continue;
				
				break;
			}
			
			if (utfToCaseless(utfFrom8bit(sfnNameFixed[i])) != utfToCaseless(ret))
				return false;
			
			if (!ret)
				return true;								//a mathc of NULLs is also a success
		}
		return true;
	}

#else

	static bool fatfsPrvCaselessEqualSfn(const uint8_t *sfnEntry, const char *cpName, uint32_t cpNameMaxLen)
	{
		uint8_t sfnNameFixed[13];
		uint_fast8_t i;
	
		//convert name to human readable (add dot, remove spaces)
		fatfsPrvShortNameExportAsCodepage((char*)sfnNameFixed, sfnEntry);
		
		//caselessly compare
		for (i = 0; i < sizeof(sfnNameFixed); i++) {
			
			char valA = (i < cpNameMaxLen) ? cpName[i] : 0;
			char valB = sfnNameFixed[i];
			
			if (valA >= 'a' && valA <= 'z')
				valA += 'A' - 'a';
			if (valB >= 'a' && valB <= 'z')
				valB += 'A' - 'a';
			if (valA != valB)
				return false;
			if (!valA)
				break;
		}
		return true;
	}
#endif

static bool fatfsPrvSomethingFindAt(struct FatfsDir* dir, const char *name, uint32_t nameMaxLen, bool wantDir, struct FatFileLocator *locatorP, uint32_t *firstEntryPosInDirP, uint8_t *numEntriesTotalP)
{
#if FATFS_USE_LFN_SUPPORT
	struct FatfsLfnState lfn;
#endif

	uint32_t fileClus, dirEntSec, firstEntryPos = -1, lastEntryPos;
	struct FatfsFil *dirAsFile = (struct FatfsFil*)dir;
	struct FatDirEntry entry;
	uint8_t dirEntIdxInSec;
	
	//a name must exist
	if (!nameMaxLen || !name || !*name)
		return false;
	
#if FATFS_USE_LFN_SUPPORT
	fatfsPrvLfnMatchInit(&lfn, name, nameMaxLen);
#endif
	
	if (!fatfsDirRewind(dir))
		return false;
	
	//handle "." in all dirs and ".." in root dir. In both cases we return the "current" dir's locator
	if (name[0] == '.' && (!name[1] || (name[1] == '.' && !name[2] && fatfsPrvIsRootDir(dirAsFile)))) {
		
		if (VERBOSE)
			pr("processing request '%s' as \"cur dir\"\n", name);
		
		if (!wantDir)		//"." is always a directory
			return false;
		else {
			
			//fill locator with the current dir's info
			fatfsPrvFillLocator(locatorP, 0, dirAsFile->firstClus, 0, 0);
			return true;
		}
	}
	
	while (1) {
		
		lastEntryPos = dirAsFile->curPos;
		
		if (!fatfsPrvDirEntryRawRead(dirAsFile, &entry, &dirEntSec, &dirEntIdxInSec))
			break;
		
#if FATFS_USE_LFN_SUPPORT
		if (!fatfsPrvLfnIngestEntry(&lfn, &entry)) {		//LFN entries are not final thmselves so we expct ingest to return false
			if (fatfsPrvLfnWasFirstLfn(&lfn))
				firstEntryPos = lastEntryPos;
			continue;
		}
#endif
		
		if (entry.attrs & FATFS_ATTR_VOL_LBL)				//skip volume labels and LFN from further code below
			continue;
		
#if FATFS_USE_LFN_SUPPORT
		if (!fatfsPrvLfnListDidWork(&lfn))
			firstEntryPos = lastEntryPos;
#else
		firstEntryPos = lastEntryPos;
#endif

		if (entry.name[0] == 0xE5)							//skip deleted files. dot files are allowed because we want to allow traversals via them
			continue;
		
		if (!entry.name[0])									//means no more files
			break;
		
		if (entry.name[0] == 0x05)
			entry.name[0] = 0xE5;
		
		if (!wantDir != !(entry.attrs & FATFS_ATTR_DIR))
			continue;
		
#if FATFS_USE_LFN_SUPPORT
		
		if (fatfsPrvLfnDidMatch(&lfn))
			goto found;
		
		//in case of LFN support, the provided name will be utf and the name in the direntry is not... compare
		if (fatfsPrvCodepageToUtf8CaselessEqualSfn(entry.name, name, nameMaxLen))
			goto found;
#else
		//in case of no LFN support, provided name is same encoding as dir entry
		if (fatfsPrvCaselessEqualSfn(entry.name, name, nameMaxLen))
			goto found;
#endif
	}

	return 0;

found:
	
	if (firstEntryPosInDirP)
		*firstEntryPosInDirP = firstEntryPos;
	if (numEntriesTotalP)
		*numEntriesTotalP = (lastEntryPos - firstEntryPos) / sizeof(struct FatDirEntry) + 1;
	
	fileClus = fatfsPrvGetLE16(&entry.clusNum);
	if (fatfsPrvVolIsFat32(dirAsFile->vol))
		fileClus += (((uint32_t)fatfsPrvGetLE16(&entry.clusNumHi)) << 16);
	//fileClus is zero for an empty file
	//in fat16 it is also zero for root dir (which we do need to handle manually)
	if (!fatfsPrvVolIsFat32(dirAsFile->vol) && wantDir && !fileClus)
		fileClus = CLUS_IS_ROOT_DIR;
	
	fatfsPrvFillLocator(locatorP, fatfsPrvGetLE32(&entry.fileSz), fileClus, dirEntSec, dirEntIdxInSec);
	return true;
}

bool fatfsFindFileAt(struct FatfsDir* dir, const char *name, struct FatFileLocator *locatorP)
{
	return fatfsPrvSomethingFindAt(dir, name, 0x10000000, false, locatorP, NULL, NULL);
}

bool fatfsFindDirAt(struct FatfsDir* dir, const char *name, struct FatFileLocator *locatorP)
{
	return fatfsPrvSomethingFindAt(dir, name, 0x10000000, true, locatorP, NULL, NULL);
}

static bool fatfsPrvSomethingFind(struct FatfsVol *vol, const char *path, uint32_t pathMaxLen, bool wantDir, struct FatFileLocator *locP)
{
	const char *pathEnd = path + pathMaxLen, *componentEnd;
	bool isLast, wantDirNow, foundNow;
	
	//create a locator for the root dir
	fatfsPrvFillLocator(locP, 0, fatfsPrvVolIsFat32(vol) ? vol->root : CLUS_IS_ROOT_DIR, 0, 0);
	
	do {
		
		for (componentEnd = path; componentEnd < pathEnd && *componentEnd && *componentEnd != '/' && *componentEnd != '\\'; componentEnd++);
		isLast = componentEnd >= pathEnd || !*componentEnd;
		wantDirNow = wantDir || !isLast;
		
		if (componentEnd != path) {		//zero length components are ignored
			
			struct FatfsDir *curDir = fatfsDirOpenWithLocator(vol, locP);
			if (!curDir)
				return false;
			
			foundNow = fatfsPrvSomethingFindAt(curDir, path, componentEnd - path, wantDirNow, locP, NULL, NULL);
			fatfsDirClose(curDir);
			
			if (!foundNow)
				return false;
		}
		path = componentEnd + 1;
		
	} while (!isLast);
	
	return true;
}

static bool fatfsPrvDirRead(struct FatfsDir *dir, char *name, uint32_t *sizeP, uint8_t *attrsP, uint32_t *clusP, uint32_t *dirEntSecP, uint8_t *dirEntIdxInSecP, bool allowDotEntries)
{
#if FATFS_USE_LFN_SUPPORT
	struct FatfsLfnState lfn;
#endif

	struct FatfsFil *dirAsFile = (struct FatfsFil*)dir;
	struct FatDirEntry entry;
	uint_fast8_t i;
	
#if FATFS_USE_LFN_SUPPORT
	if (name)
		fatfsPrvLfnListInit(&lfn, name);
#endif

	while(1) {
		
		uint32_t fileSz, fileClus;
		
		if (!fatfsPrvDirEntryRawRead(dirAsFile, &entry, dirEntSecP, dirEntIdxInSecP))
			break;
		
#if FATFS_USE_LFN_SUPPORT
		if (name) {
			if (!fatfsPrvLfnIngestEntry(&lfn, &entry))
				continue;
		}
#endif
		
		if (entry.attrs & FATFS_ATTR_VOL_LBL)										//skip volume labels and LFN from further code below
			continue;

		if ((!allowDotEntries && entry.name[0] == '.') || entry.name[0] == 0xE5)	//skip dot files (unless requested not to) and deleted files
			continue;
		
		if (entry.name[0] == 0)														//means no more files
			break;
		
		if (entry.name[0] == 0x05)
			entry.name[0] = 0xE5;
		
		//we now have an SFN entry - apply case info to name
		
		if (name) {
			
			if (entry.caseInfo & 0x10) {
				for (i = 0; i < 3; i++) {
					
					if (entry.name[i + 8] >= 'A' && entry.name[i + 8] <= 'Z')
						entry.name[i + 8] += 'a' - 'A';
				}
			}
			if (entry.caseInfo & 0x08) {
				for (i = 0; i < 8; i++) {
					
					if (entry.name[i] >= 'A' && entry.name[i] <= 'Z')
						entry.name[i] += 'a' - 'A';
				}
			}
		
#if FATFS_USE_LFN_SUPPORT
			if (!fatfsPrvLfnListDidWork(&lfn))		//else LFN would have been exported already
				fatfsPrvShortNameExport(&entry, name);
#else
			fatfsPrvShortNameExport(&entry, name);
#endif
		}
		
		fileSz = fatfsPrvGetLE32(&entry.fileSz);
		fileClus = fatfsPrvGetLE16(&entry.clusNum);
		if (fatfsPrvVolIsFat32(dirAsFile->vol))
			fileClus += (((uint32_t)fatfsPrvGetLE16(&entry.clusNumHi)) << 16);
		
		//fileClus is zero for an empty file
		
		if (sizeP)
			*sizeP = fileSz;
		
		if (attrsP)
			*attrsP = entry.attrs;
					
		if (clusP)
			*clusP = fileClus;
		
		return true;
	}

	return false;
}

bool fatfsDirRead(struct FatfsDir *dir, char *name, uint32_t *sizeP, uint8_t *attrsP, struct FatFileLocator *locatorP)
{
	uint32_t clus, size, entrySec;
	uint8_t entryIdxInSec;
	
	if (!sizeP)
		sizeP = &size;
	
	if (!fatfsPrvDirRead(dir, name, sizeP, attrsP, &clus, &entrySec, &entryIdxInSec, false))
		return false;
	
	fatfsPrvFillLocator(locatorP, *sizeP, clus, entrySec, entryIdxInSec);
	return true;
}

#if FATFS_USE_LFN_SUPPORT

	static void fatfsPrvLfnResetState(struct FatfsLfnState *state)
	{
		state->numEntries = 0;
		
		switch (state->mode) {
			case FatfsLfnModeMatch:
				state->match.isMatch = true;
				break;
			
			case FatfsLfnModeList:
				//nothing here
				break;
		}
	}
	
	static bool fatfsPrvLfnDidMatch(struct FatfsLfnState *state)
	{
		return state->mode == FatfsLfnModeMatch && state->numEntries && !state->nextEntryExpected && state->match.isMatch;
	}
	
	static bool fatfsPrvLfnWasFirstLfn(struct FatfsLfnState *state)
	{
		return state->numEntries && state->nextEntryExpected == state->numEntries - 1;
	}
	
	static bool fatfsPrvLfnListDidWork(struct FatfsLfnState *state)
	{
		return state->numEntries && !state->nextEntryExpected;
	}
	
	static void fatfsPrvLfnMatchInit(struct FatfsLfnState *state, const char *matchStr, uint32_t matchMaxLen)
	{
		state->mode = FatfsLfnModeMatch;
		state->match.name = matchStr;
		state->match.maxLen = matchMaxLen;
		fatfsPrvLfnResetState(state);
	}
	
	static void fatfsPrvLfnListInit(struct FatfsLfnState *state, char *outStr)
	{
		state->mode = FatfsLfnModeList;
		state->list.dst = outStr;
		fatfsPrvLfnResetState(state);
	}
	
	static void fatfsPrvLfnProcessIngestedMatchEntry(struct FatfsLfnState *state, const struct FatLfnEntry *le, uint_fast8_t curEntryIdx, uint_fast8_t numEntries)
	{
		struct Utf8state u8s = UTF8_STATE_STATIC_INITIALIZER;
		const char *ch = state->match.name;
		uint32_t ret, curCodepointPos = 0;
		uint_fast8_t i;
		
		
		//go to the proper codepoint in input
		while (curCodepointPos < curEntryIdx * FATFS_CHARS_PER_LFN) {
			
			if (ch >= state->match.name + state->match.maxLen) {		//out of input -> no match
				
				state->match.isMatch = false;
				return;
			}
			
			ret = utf8inputByte(&u8s, *ch++);
			
			if (ret == UTF_ERROR || ret == 0) {			//error or terminator in input before we expected it -> no match
				state->match.isMatch = false;
				return;
			}
			
			if (ret == UTF_NO_OUTPUT)
				continue;
			
			curCodepointPos++;
		}
		
		//we get here if we've fast-forwarded to the proper place
		for (i = 0; i < FATFS_CHARS_PER_LFN; i++) {
					
			//get the utf8 codepoint
			while(1) {
				if (ch >= state->match.name + state->match.maxLen) {		//no more input - same as a temrinator
					ret = 0;
					break;
				}
				ret = utf8inputByte(&u8s, *ch++);
				
				if (ret == UTF_ERROR) {										//error in input before we expected it -> no match
					state->match.isMatch = false;
					return;
				}
				
				if (ret == UTF_NO_OUTPUT)
					continue;
				
				break;
			}
			
			if (utfToCaseless(ret) != utfToCaseless(fatfsPrvReadNthLfnChar(le, i))) {
				
				state->match.isMatch = false;
				return;		//do not bother further if we have a mismatch
			}
			
			if (!ret)
				break;		//no checking past matched null terminator
		}
		
		//verify that the request terminated if this is our last entry
		//this will avoid matching a file named "0123456789abc"(exactly 13 chars) to request "0123456789abcdef"
		if (i == FATFS_CHARS_PER_LFN && curEntryIdx == numEntries - 1) {
			
			if (ch < state->match.name + state->match.maxLen && utf8inputByte(&u8s, *ch++)) {
				
				//request goes on, but file name is done
				state->match.isMatch = false;
			}
		}
	}
	
	static void fatfsPrvLfnProcessIngestedListEntry(struct FatfsLfnState *state, const struct FatLfnEntry *le, uint_fast8_t curEntryIdx, uint_fast8_t numEntries)
	{
		//each utf8 char up to 0xffff takes 3 chars, which is why FATFS_NAME_BUF_LEN is 3 * FATFS_LFN_MAX_LEN
		//each ucs2 codepoint is 2 bytes, which means that we can store the latter in place of the former and not run out of space
		//for extra credit we can do it backwards :)
		//the passed-in LFN has been somewhat verified (eg for max length)
		uint16_t *bufAlignedEnd = (uint16_t*)(((uintptr_t)state->list.dst + FATFS_NAME_BUF_LEN) / sizeof(uint16_t) * sizeof(uint16_t));
		uint16_t *wholeBuf16 = bufAlignedEnd - FATFS_LFN_MAX_LEN, *thisChunk = wholeBuf16 + FATFS_CHARS_PER_LFN * curEntryIdx;
		uint8_t *dst8 = (uint8_t*)state->list.dst;
		uint_fast16_t i, L;
		
		
		//if this is the last chunk, remember that it is shorter
		if ((curEntryIdx + 1) * FATFS_CHARS_PER_LFN >= FATFS_LFN_MAX_LEN)
			L = FATFS_LFN_MAX_LEN - FATFS_CHARS_PER_LFN * curEntryIdx;
		else
			L = FATFS_CHARS_PER_LFN;
		
		//copy this chunk into place
		for (i = 0; i < L; i++)
			thisChunk[i] = fatfsPrvReadNthLfnChar(le, i);
		
		//if this is the 0th chunk (last one seen), we can convert to utf8
		if (!curEntryIdx) {
			for (i = 0; i < FATFS_LFN_MAX_LEN && i < numEntries * FATFS_CHARS_PER_LFN && wholeBuf16[i]; i++)
				utf8write(&dst8, wholeBuf16[i]);
			
			//terminate
			utf8write(&dst8, 0);
		}
	}
	
	static bool fatfsPrvLfnIngestEntry(struct FatfsLfnState *state, const void *entry)	//true if we finished decoding a complete set of entries. does not imply match or a functional LFN sequence
	{
		const struct FatLfnEntry *le = (const struct FatLfnEntry*)entry;
		const struct FatDirEntry *fe = (const struct FatDirEntry*)entry;
		uint_fast8_t i;
	
		if (le->attrs != FATFS_ATTR_LFN) {								//not an LFN entry
			uint8_t cksum = 0;
			
			if (!state->numEntries || state->nextEntryExpected)	 {		//bad sequencing
				
				fatfsPrvLfnResetState(state);
				return true;
			}
			
			//correct state -> check checksum
			for (i = 0; i < sizeof(fe->name); i++)
				cksum = (cksum >> 1) + ((cksum & 1) ? 0x80 : 0x00) + (uint8_t)(fe->name[i]);
			
			if (cksum != state->cksum) {
				fatfsPrvLfnResetState(state);
				return true;
			}
					
			//successful LFN decoding completed
			return true;
		}
	
		if (le->ordinal & FATFS_ORDINAL_MASK_DEL)							//deleted LFN entry
			goto fail_and_reset;
		if (le->zero || le->type)											//skip invalid entries
			goto fail_and_reset;							
			
		if (le->ordinal & FATFS_ORDINAL_MASK_LAST) {						//last entry (seen first)
			
			fatfsPrvLfnResetState(state);
			state->numEntries = le->ordinal & FATFS_ORDINAL_MASK_IDX;
			state->nextEntryExpected = state->numEntries;
			state->cksum = le->checksum;
			
			//verify limits (storage space exists for 260 chars, 255 max are allowed)
			if (FATFS_CHARS_PER_LFN * state->numEntries > FATFS_LFN_MAX_LEN) {
				
				for (i = 0; i < FATFS_CHARS_PER_LFN && fatfsPrvReadNthLfnChar(le, i); i++);
				
				if (FATFS_CHARS_PER_LFN * (state->numEntries - 1) + i > FATFS_LFN_MAX_LEN)
					goto fail_and_reset;
			}
		}
		else if (state->nextEntryExpected != le->ordinal) {					//wrong order entry from expected
		
			goto fail_and_reset;
		}
		else if (state->cksum != le->checksum) {							//chcksummismatch between entries
		
			goto fail_and_reset;
		}
		
		//we're here because we got an entry of the index we had expected
		state->nextEntryExpected--;		//record which we expect next
		
		//these processing funcs only see things in proper order
		switch (state->mode) {
			case FatfsLfnModeMatch:
				fatfsPrvLfnProcessIngestedMatchEntry(state, le, state->nextEntryExpected, state->numEntries);
				return false;
				
			case FatfsLfnModeList:
				fatfsPrvLfnProcessIngestedListEntry(state, le, state->nextEntryExpected, state->numEntries);
				return false;
			
			default:
				//shouldnt happen
				goto fail_and_reset;
		}
	
	fail_and_reset:
		fatfsPrvLfnResetState(state);
		return false;
	}
#endif


