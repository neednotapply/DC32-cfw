#ifndef DC32_PIPE_PORT_H
#define DC32_PIPE_PORT_H

#include "dcApp.h"

#ifdef __cplusplus
extern "C" {
#endif

int pipeAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void pipeAppAbort(void);

#ifdef __cplusplus
}
#endif

#endif
