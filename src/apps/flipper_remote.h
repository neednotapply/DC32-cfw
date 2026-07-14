#ifndef _FLIPPER_REMOTE_H_
#define _FLIPPER_REMOTE_H_

#include "dcApp.h"

int flipperRemoteRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void flipperRemoteAbort(void);

#endif
