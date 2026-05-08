#ifndef _USB_HID_H_
#define _USB_HID_H_

#include <stdbool.h>
#include <stdint.h>

#define USB_HID_DEFAULT_VID		0x1209
#define USB_HID_DEFAULT_PID		0xdc32

#define USB_HID_MOD_LCTRL		0x01
#define USB_HID_MOD_LSHIFT		0x02
#define USB_HID_MOD_LALT		0x04
#define USB_HID_MOD_LGUI		0x08
#define USB_HID_MOD_RCTRL		0x10
#define USB_HID_MOD_RSHIFT		0x20
#define USB_HID_MOD_RALT		0x40
#define USB_HID_MOD_RGUI		0x80

struct UsbHidDeviceInfo {
	uint16_t vid;
	uint16_t pid;
	char manufacturer[32];
	char product[32];
};

void usbHidDefaultInfo(struct UsbHidDeviceInfo *info);
bool usbHidBegin(const struct UsbHidDeviceInfo *info);
void usbHidTask(void);
bool usbHidReady(void);
bool usbHidStarted(void);
void usbHidSetReportsEnabled(bool enabled);
bool usbHidReportsEnabled(void);
bool usbHidKeyboardReport(uint8_t modifiers, const uint8_t keys[6]);
bool usbHidMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t wheel);
bool usbHidConsumerReport(uint16_t usage);
void usbHidReleaseAll(void);
void usbHidEnd(void);

#endif
