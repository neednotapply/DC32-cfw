#ifndef FLAPPY_PORT_H
#define FLAPPY_PORT_H

#include "dcApp.h"

#ifdef __cplusplus
extern "C" {
#endif

int flappyAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void flappyAppAbort(void);

#ifdef __cplusplus
}
#endif

#endif
