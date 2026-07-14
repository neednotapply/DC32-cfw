#ifndef _PWNAGOTCHI_REMOTE_H_
#define _PWNAGOTCHI_REMOTE_H_

#include "dcApp.h"

int pwnagotchiRemoteRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void pwnagotchiRemoteAbort(void);

#endif
