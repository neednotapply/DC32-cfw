#ifndef _IMAGE_VIEWER_ANIMATED_GIF_H_
#define _IMAGE_VIEWER_ANIMATED_GIF_H_

#include "imageViewer.h"

#ifdef __cplusplus
extern "C" {
#endif

enum ImageViewerResult imageViewerAnimatedGifRun(struct Canvas *cnv, struct FatfsVol *vol, const struct FatFileLocator *locator);

#ifdef __cplusplus
}
#endif

#endif
