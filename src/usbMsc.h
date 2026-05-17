#ifndef _USB_MSC_H_
#define _USB_MSC_H_

#include <stdbool.h>
#include <stdint.h>

struct UsbMscStatus {
	const char *op;
	const char *error;
	uint32_t lba;
	uint32_t bytes;
	bool speedLimited;
};

bool usbMscBegin(bool writable);
void usbMscTask(void);
bool usbMscStarted(void);
bool usbMscMounted(void);
bool usbMscEjected(void);
bool usbMscWritable(void);
const char *usbMscLastError(void);
void usbMscGetStatus(struct UsbMscStatus *status);
void usbMscEnd(void);

#endif
