#ifndef CHIPS_PORT_H
#define CHIPS_PORT_H

#include "dcApp.h"

#ifdef __cplusplus
extern "C" {
#endif

int chipsAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void chipsAppAbort(void);

#ifdef __cplusplus
}
#endif

#endif
