#ifndef SOKOBAN_PORT_H
#define SOKOBAN_PORT_H

#include "dcApp.h"

#ifdef __cplusplus
extern "C" {
#endif

int sokobanAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void sokobanAppAbort(void);

#ifdef __cplusplus
}
#endif

#endif
