#ifndef DC32_CAVE_PORT_H
#define DC32_CAVE_PORT_H

#include "dcApp.h"

#ifdef __cplusplus
extern "C" {
#endif

int caveAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void caveAppAbort(void);

#ifdef __cplusplus
}
#endif

#endif
