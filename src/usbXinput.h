#ifndef _USB_XINPUT_H_
#define _USB_XINPUT_H_

#include <stdbool.h>
#include <stdint.h>
#include "usbHid.h"

struct UsbXinputFeedback {
	uint8_t rumbleLeft;
	uint8_t rumbleRight;
	uint8_t ledPattern;
	uint32_t pingSeq;
};

bool usbXinputBegin(void);
const char *usbXinputLastError(void);
void usbXinputTask(void);
bool usbXinputReady(void);
bool usbXinputStarted(void);
void usbXinputSetReportsEnabled(bool enabled);
bool usbXinputReport(int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry, uint8_t hat, uint32_t buttons);
void usbXinputReleaseAll(void);
void usbXinputFeedback(struct UsbXinputFeedback *feedback);
void usbXinputEnd(void);

#endif
