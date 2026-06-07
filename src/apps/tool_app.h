#ifndef TOOL_APP_H
#define TOOL_APP_H

#include "dcApp.h"

#ifdef __cplusplus
extern "C" {
#endif

int uiDcAppRunIr(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
int uiDcAppRunImage(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
int uiDcAppRunMusic(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
int uiDcAppRunBadUsb(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
int uiDcAppRunAutoclicker(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
int uiDcAppRunGamepad(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);

#ifdef __cplusplus
}
#endif

#endif
