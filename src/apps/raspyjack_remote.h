#ifndef _RASPYJACK_REMOTE_H_
#define _RASPYJACK_REMOTE_H_

#include "dcApp.h"

int raspyJackRemoteRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void raspyJackRemoteAbort(void);

#endif
