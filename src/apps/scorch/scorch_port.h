#ifndef DC32_SCORCH_PORT_H
#define DC32_SCORCH_PORT_H

#include "dcApp.h"

#ifdef __cplusplus
extern "C" {
#endif

int scorchAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void scorchAppAbort(void);

#ifdef __cplusplus
}
#endif

#endif
