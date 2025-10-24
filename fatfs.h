#ifndef _FATFS_H_
#define _FATFS_H_


#include <stdint.h>
#include <stdbool.h>

#define FATFS_MAX_VOLUMES		1
#define FATFS_MAX_FILES			4

struct FatFileLocator {	//for faster opening
	uint32_t opaque[4];
};


//makefile defines FATFS_USE_LFN_SUPPORT set to 1 to enable LFN support
//	enabling LFN can eat up up to 1275 bytes of heap inside fileopen/diropen
//	LFN is not perfectly caseless (it is for russian and english, it is not for all others because i do not have time to code caseless compare for all utf langs)

//when LFN is off, all strings are 8-bit selected encoding, when it is on, all strings are utf8 (even short names you provide, meaning the buffers should be big enough)

#define FATFS_LFN_MAX_LEN	255	//in symbols not bytes. in bytes it is about 3x this value in worst case
#define FATFS_SFN_MAX_LEN	12	//in bytes - 8 + '.' + 3 

#if FATFS_USE_LFN_SUPPORT
	#define FATFS_NAME_BUF_LEN	(FATFS_LFN_MAX_LEN * 3 + 1)
#else
	#define FATFS_NAME_BUF_LEN	(FATFS_SFN_MAX_LEN + 1)
#endif

//structs
struct FatfsVol;
struct FatfsFil;
struct FatfsDir;

//defines
#define FATFS_ATTR_RO		0x01
#define FATFS_ATTR_HIDDEN	0x02
#define FATFS_ATTR_SYSTEM	0x04
#define FATFS_ATTR_VOL_LBL	0x08
#define FATFS_ATTR_DIR		0x10
#define FATFS_ATTR_ARCHIVE	0x20

//external funcs
#define FATFS_DISK_SECT_SZ	512U

typedef bool (*FatfsDiskReadF)(void *diskUserData, uint32_t sec, uint32_t numSec, void *dst);
typedef bool (*FatfsDiskWriteF)(void *diskUserData, uint32_t sec, uint32_t numSec, const void *dst);

#define OPEN_MODE_READ		0
#define OPEN_MODE_WRITE		1
#define OPEN_MODE_CREATE	2
#define OPEN_MODE_TRUNCATE	4


//our funcs (vol)
struct FatfsVol* fatfsMount(FatfsDiskReadF readF, FatfsDiskWriteF writeF, void *diskUserData);
bool fatfsUnmount(struct FatfsVol *vol);

//our funcs (files)
struct FatfsFil* fatfsFileOpen(struct FatfsVol *vol, const char *path, uint_fast8_t mode);
struct FatfsFil* fatfsFileOpenAt(struct FatfsDir* dir, const char *name, uint_fast8_t mode);
bool fatfsFileCreateAt(struct FatfsDir* dir, const char *name, struct FatFileLocator *locatorP);
struct FatfsFil* fatfsFileOpenWithLocator(struct FatfsVol *vol, const struct FatFileLocator *locatorP, uint_fast8_t mode);
bool fatfsFileClose(struct FatfsFil* fil);
bool fatfsFileRead(struct FatfsFil* fil, void *buf, uint32_t num, uint32_t *numReadP);	//returns false only on actual errors. eof is not an error, and reads 0 bytes
bool fatfsFileWrite(struct FatfsFil* fil, const void *buf, uint32_t num, uint32_t *numWrittenP);
bool fatfsFileSeek(struct FatfsFil* fil, uint32_t pos);
uint32_t fatfsFileTell(struct FatfsFil* fil);
uint32_t fatfsFileGetSize(struct FatfsFil* fil);
bool fatfsFileDelete(struct FatfsVol *vol, const char *path);
bool fatfsFileDeleteAt(struct FatfsDir* dir, const char *name);
bool fatfsFileTruncate(struct FatfsFil* fil, uint32_t sz);


//our funcs(dirs)
struct FatfsDir* fatfsRootDirOpen(struct FatfsVol *vol);
struct FatfsDir* fatfsDirOpen(struct FatfsVol *vol, const char *path);
struct FatfsDir* fatfsDirOpenAt(struct FatfsDir* dir, const char *name);
bool fatfsDirCreate(struct FatfsVol *vol, const char *path, struct FatFileLocator *locatorP);
bool fatfsDirCreateAt(struct FatfsDir* dir, const char *name, struct FatFileLocator *locatorP);
struct FatfsDir* fatfsDirOpenWithLocator(struct FatfsVol *vol, const struct FatFileLocator *locatorP);
bool fatfsDirClose(struct FatfsDir* dir);
bool fatfsDirRead(struct FatfsDir *dir, char *name, uint32_t *sizeP, uint8_t *attrsP, struct FatFileLocator *locatorP);
bool fatfsDirRewind(struct FatfsDir *dir);
bool fatfsDirDelete(struct FatfsVol *vol, const char *path);
bool fatfsDirDeleteAt(struct FatfsDir* dir, const char *name);

//targetted dir search
bool fatfsFindFileAt(struct FatfsDir* dir, const char *name, struct FatFileLocator *locatorP);
bool fatfsFindDirAt(struct FatfsDir* dir, const char *name, struct FatFileLocator *locatorP);




#endif
