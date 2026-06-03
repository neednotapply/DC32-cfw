#ifndef _USB_DEVICE_H_
#define _USB_DEVICE_H_

#include <stdbool.h>
#include <stdint.h>

enum UsbDeviceMode {
	UsbDeviceModeNone,
	UsbDeviceModeHid,
	UsbDeviceModeMsc,
};

struct UsbDeviceInfo {
	uint16_t vid;
	uint16_t pid;
	char manufacturer[32];
	char product[32];
};

bool usbDevicePrepare(void);
const char *usbDeviceLastError(void);
bool usbDeviceBegin(enum UsbDeviceMode mode, const struct UsbDeviceInfo *info);
void usbDeviceTask(void);
bool usbDeviceMounted(void);
bool usbDeviceStarted(enum UsbDeviceMode mode);
enum UsbDeviceMode usbDeviceMode(void);
void usbDeviceEnd(void);
void usbDeviceDropNow(void);

#endif
