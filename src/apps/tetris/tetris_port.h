#ifndef DC32_TETRIS_PORT_H
#define DC32_TETRIS_PORT_H

#include "dcApp.h"

#ifdef __cplusplus
extern "C" {
#endif

int tetrisAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void tetrisAppAbort(void);

#ifdef __cplusplus
}
#endif

#endif
