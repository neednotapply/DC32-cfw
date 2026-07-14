#ifndef _IMAGE_VIEWER_H_
#define _IMAGE_VIEWER_H_

#include <stdbool.h>
#include <sys/types.h>
#include "fatfs.h"
#include "ui.h"

enum ImageViewerResult {
	ImageViewerResultBack,
	ImageViewerResultExit,
	ImageViewerResultMenu,
	ImageViewerResultPrev,
	ImageViewerResultNext,
	ImageViewerResultOpenError,
	ImageViewerResultReadError,
	ImageViewerResultDecodeError,
	ImageViewerResultIncompatibleGif,
	ImageViewerResultUnsupported,
	ImageViewerResultTooLarge,
	ImageViewerResultNoMemory,
};

bool imageViewerFileName(const char *name);
enum ImageViewerResult imageViewerRun(struct Canvas *cnv, struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *initialLocator, const char *initialName);
enum ImageViewerResult imageViewerRunStill(struct Canvas *cnv, struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *initialLocator, const char *initialName);

int imageViewerGifOpen(const char *path, int flags);
ssize_t imageViewerGifRead(int fd, void *buf, size_t size);
off_t imageViewerGifSeek(int fd, off_t offset, int whence);
int imageViewerGifClose(int fd);
void *imageViewerAlloc(size_t size);
void *imageViewerCalloc(size_t count, size_t size);
void *imageViewerRealloc(void *ptr, size_t size);
void imageViewerFree(void *ptr);

#endif
