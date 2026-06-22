#ifndef DC32_OPENJAZZ_CACHE_H
#define DC32_OPENJAZZ_CACHE_H

#include <stdbool.h>
#include <stdint.h>

struct DcAppHostApi;
struct DcAppRunArgs;
struct SDL_Surface;

struct Dc32OjSurfaceView {
	const uint8_t *pixels;
	uint16_t width;
	uint16_t height;
	uint16_t pitch;
	uint8_t colorKey;
	uint8_t colorKeyEnabled;
};

bool dc32OjCachePrepare(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void dc32OjCacheClose(void);
bool dc32OjCacheCommit(void);
bool dc32OjCacheSeal(void);
bool dc32OjCacheIsSealed(void);
bool dc32OjCacheCheckpoint(const char *stage, uint32_t expectedCount,
	const char *expectedLastEntry);
bool dc32OjCachePendingStatus(char name[16], uint32_t *entry,
	uint32_t *written, uint32_t *size);
const char *dc32OjCacheLastError(void);
void dc32OjCacheBuildFailure(const char *asset, const char *reason);

struct SDL_Surface *dc32OjCacheFindSurface(const char *name);
bool dc32OjCacheFindView(const char *name, struct Dc32OjSurfaceView *view);
struct SDL_Surface *dc32OjCacheCreateStagingSurface(
	uint16_t width, uint16_t height);
bool dc32OjCachePromoteSurfaceChecked(const char *name,
	struct SDL_Surface **surface);
struct SDL_Surface *dc32OjCacheStoreSurface(const char *name, const void *pixels,
	uint16_t width, uint16_t height, uint16_t pitch, uint8_t colorKey,
	bool colorKeyEnabled);
bool dc32OjCacheStoreView(const char *name, const void *pixels,
	uint16_t width, uint16_t height, uint16_t pitch, uint8_t colorKey,
	bool colorKeyEnabled, struct Dc32OjSurfaceView *view);
bool dc32OjCacheFindBlob(const char *name, const void **data, uint32_t *size);
bool dc32OjCacheStoreBlob(const char *name, const void *data, uint32_t size);

#define DC32_OJ_DECODE_PIXELS_CAPACITY 6144u
#define DC32_OJ_DECODE_MASK_CAPACITY   2048u
uint8_t *dc32OjCacheDecodePixels(void);
uint8_t *dc32OjCacheDecodeMask(void);

bool dc32OjCacheBeginSurface(const char *name, uint16_t width, uint16_t height,
	uint16_t pitch, uint8_t colorKey, bool colorKeyEnabled);
bool dc32OjCacheWriteSurface(const void *data, uint32_t size);
struct SDL_Surface *dc32OjCacheEndSurface(void);
bool dc32OjCacheEndView(struct Dc32OjSurfaceView *view);
void dc32OjCacheCancelSurface(void);

#endif
