#ifndef DC32_LASERTAG_H
#define DC32_LASERTAG_H

#include "dcApp.h"

int laserTagAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
void laserTagAppAbort(void);

#endif
