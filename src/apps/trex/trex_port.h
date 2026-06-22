#ifndef TREX_PORT_H
#define TREX_PORT_H

#include "dcApp.h"

#ifdef __cplusplus
extern "C" {
#endif

int trexAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void trexAppAbort(void);

#ifdef __cplusplus
}
#endif

#endif
