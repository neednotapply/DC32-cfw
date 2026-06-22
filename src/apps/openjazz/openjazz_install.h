#ifndef DC32_OPENJAZZ_INSTALL_H
#define DC32_OPENJAZZ_INSTALL_H

#include <stdbool.h>
#include <stdint.h>

struct DcAppHostApi;
struct DcAppRunArgs;

bool dc32OjPrepareData(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void dc32OjShowMessage(const struct DcAppHostApi *host, const struct DcAppRunArgs *args,
	const char *title, const char *line1, const char *line2, bool waitForCenter);
void dc32OjLoadingStart(const struct DcAppHostApi *host, const struct DcAppRunArgs *args,
	const char *stage);
void dc32OjLoadingStage(const char *stage);
void dc32OjLoadingAsset(const char *name);
void dc32OjLoadingCacheProgress(const char *name, uint32_t entry,
	uint32_t entryTotal, uint32_t bytes, uint32_t byteTotal);
void dc32OjLoadingFlashWrite(const char *name, uint32_t address, uint32_t size);
void dc32OjLoadingFailure(const char *stage, const char *detail);
void dc32OjLoadingPause(void);
void dc32OjLoadingFinish(void);
const char *dc32OjLoadingContext(void);

#endif
