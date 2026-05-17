#ifndef _IMAGE_VIEWER_H_
#define _IMAGE_VIEWER_H_

#include <stdbool.h>
#include <stddef.h>
#include "fatfs.h"
#include "ui.h"

enum ImageViewerResult {
	ImageViewerResultBack,
	ImageViewerResultExit,
	ImageViewerResultPrev,
	ImageViewerResultNext,
	ImageViewerResultOpenError,
	ImageViewerResultReadError,
	ImageViewerResultDecodeError,
	ImageViewerResultUnsupported,
	ImageViewerResultTooLarge,
	ImageViewerResultNoMemory,
};

bool imageViewerFileName(const char *name);
enum ImageViewerResult imageViewerRun(struct Canvas *cnv, struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *initialLocator, const char *initialName);

void imageViewerAllocReset(void);
void *imageViewerAlloc(size_t size);
void *imageViewerCalloc(size_t count, size_t size);
void *imageViewerRealloc(void *ptr, size_t size);
void imageViewerFree(void *ptr);

int imageViewerGifOpen(const char *name, int flags);
int imageViewerGifRead(int fd, void *buf, unsigned int count);
int imageViewerGifClose(int fd);
long imageViewerGifSeek(int fd, long offset, int whence);

#endif
