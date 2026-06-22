#ifndef DC32_OPENJAZZ_PACK_H
#define DC32_OPENJAZZ_PACK_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "fatfs.h"

#define DC32_OPENJAZZ_PACK_PATH "/APPS/openjazz.pak"
#define DC32_OPENJAZZ_ZIP_PATH "/APPS/JAZZ.ZIP"
#define DC32_OPENJAZZ_PACK_MAGIC "DC32JAZZPK\0"

struct Dc32OjFile {
	struct FatfsFil *fatFile;
	uint32_t offset;
	uint32_t size;
	uint32_t pos;
	uint8_t kind;
	bool write;
	bool open;
};

bool dc32OjPackOpen(struct FatfsVol *vol, const char *path);
void dc32OjPackClose(void);
void dc32OjCloseUserFiles(void);
bool dc32OjPackIsOpen(void);
bool dc32OjPackFingerprint(uint32_t *fingerprint, uint32_t *fileSize, uint32_t *entryCount);
bool dc32OjFileExists(const char *name);
bool dc32OjFileOpen(const char *name, struct Dc32OjFile *file);
bool dc32OjUserFileExists(const char *name);
bool dc32OjUserFileOpen(const char *name, bool write, struct Dc32OjFile *file);
void dc32OjFileClose(struct Dc32OjFile *file);
uint32_t dc32OjFileSize(const struct Dc32OjFile *file);
uint32_t dc32OjFileTell(const struct Dc32OjFile *file);
bool dc32OjFileSeek(struct Dc32OjFile *file, int32_t offset, int origin);
uint32_t dc32OjFileRead(struct Dc32OjFile *file, void *dst, uint32_t size);
uint32_t dc32OjFileWrite(struct Dc32OjFile *file, const void *src, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
