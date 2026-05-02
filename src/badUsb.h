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

struct BadUsbStatus {
	uint32_t lineNo;
	uint32_t fileSize;
	uint32_t bytesRead;
	const char *message;
};

typedef bool (*BadUsbStatusF)(void *userData, const struct BadUsbStatus *status);
typedef bool (*BadUsbWaitButtonF)(void *userData, const struct BadUsbStatus *status);

bool badUsbReadDeviceInfo(struct FatfsFil *fil, struct UsbHidDeviceInfo *info);
enum BadUsbResult badUsbRunFile(struct FatfsFil *fil, BadUsbStatusF statusF, BadUsbWaitButtonF waitButtonF, void *userData);

#endif
