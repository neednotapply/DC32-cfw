#ifndef _BAD_USB_H_
#define _BAD_USB_H_

#include <stdbool.h>
#include <stdint.h>
#include "fatfs.h"
#include "usbHid.h"

enum BadUsbResult {
	BadUsbResultDone,
	BadUsbResultCancelled,
	BadUsbResultFileError,
	BadUsbResultDecodeError,
	BadUsbResultUsbError,
};

enum BadUsbWorkerState {
	BadUsbStateInit,
	BadUsbStateNotConnected,
	BadUsbStateIdle,
	BadUsbStateWillRun,
	BadUsbStateRunning,
	BadUsbStateDelay,
	BadUsbStatePaused,
	BadUsbStateWaitForButton,
	BadUsbStateDone,
	BadUsbStateScriptError,
	BadUsbStateFileError,
	BadUsbStateUsbError,
};

struct BadUsbStatus {
	enum BadUsbWorkerState state;
	uint32_t lineNo;
	uint32_t lineTotal;
	uint32_t fileSize;
	uint32_t bytesRead;
	uint32_t delayRemainSec;
	uint32_t errorLine;
	uint32_t unsupportedCommands;
	uint32_t lastUnsupportedLine;
	char error[64];
	char lastUnsupportedCommand[32];
	const char *message;
};

struct BadUsbPreload {
	struct UsbHidDeviceInfo info;
	uint32_t fileSize;
	uint32_t lineTotal;
};

typedef bool (*BadUsbStatusF)(void *userData, const struct BadUsbStatus *status);
typedef bool (*BadUsbWaitButtonF)(void *userData, const struct BadUsbStatus *status);

bool badUsbReadDeviceInfo(struct FatfsFil *fil, struct UsbHidDeviceInfo *info);
enum BadUsbResult badUsbPreloadFile(struct FatfsFil *fil, BadUsbStatusF statusF, void *userData, struct BadUsbPreload *preload);
enum BadUsbResult badUsbRunPreparedFile(struct FatfsFil *fil, const struct BadUsbPreload *preload, BadUsbStatusF statusF, BadUsbWaitButtonF waitButtonF, void *userData);

#endif
