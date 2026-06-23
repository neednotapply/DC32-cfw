#ifndef DC32_ARKANOID_PORT_H
#define DC32_ARKANOID_PORT_H

#include "dcApp.h"

#ifdef __cplusplus
extern "C" {
#endif

int arkanoidAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void arkanoidAppAbort(void);

#ifdef __cplusplus
}
#endif

#endif
