#ifndef _IMAGE_VIEWER_H_
#define _IMAGE_VIEWER_H_

#include <stdbool.h>
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
	ImageViewerResultUnsupported,
	ImageViewerResultTooLarge,
	ImageViewerResultNoMemory,
};

bool imageViewerFileName(const char *name);
enum ImageViewerResult imageViewerRun(struct Canvas *cnv, struct FatfsVol *vol, const char *rootPath, const struct FatFileLocator *initialLocator, const char *initialName);

#endif
