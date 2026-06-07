#ifndef _USB_HID_H_
#define _USB_HID_H_

#include <stdbool.h>
#include <stdint.h>

#define USB_HID_DEFAULT_VID		0x1209
#define USB_HID_DEFAULT_PID		0xdc32
#define USB_HID_DS4_VID		0x054c
#define USB_HID_DS4_PID		0x05c4

#define USB_HID_MOD_LCTRL		0x01
#define USB_HID_MOD_LSHIFT		0x02
#define USB_HID_MOD_LALT		0x04
#define USB_HID_MOD_LGUI		0x08
#define USB_HID_MOD_RCTRL		0x10
#define USB_HID_MOD_RSHIFT		0x20
#define USB_HID_MOD_RALT		0x40
#define USB_HID_MOD_RGUI		0x80

#define USB_HID_MOUSE_BUTTON_LEFT	0x01
#define USB_HID_MOUSE_BUTTON_RIGHT	0x02
#define USB_HID_MOUSE_BUTTON_MIDDLE	0x04
#define USB_HID_MOUSE_BUTTON_BACK	0x08
#define USB_HID_MOUSE_BUTTON_FORWARD	0x10

#define USB_HID_GAMEPAD_BUTTON_A	0x00000001u
#define USB_HID_GAMEPAD_BUTTON_B	0x00000002u
#define USB_HID_GAMEPAD_BUTTON_L1	0x00000040u
#define USB_HID_GAMEPAD_BUTTON_R1	0x00000080u
#define USB_HID_GAMEPAD_BUTTON_L2	0x00000100u
#define USB_HID_GAMEPAD_BUTTON_R2	0x00000200u
#define USB_HID_GAMEPAD_BUTTON_SELECT	0x00000400u
#define USB_HID_GAMEPAD_BUTTON_START	0x00000800u
#define USB_HID_GAMEPAD_BUTTON_HOME	0x00001000u
#define USB_HID_GAMEPAD_BUTTON_TOUCHPAD	0x00002000u

#define USB_HID_DS4_TOUCHPAD_WIDTH	1920u
#define USB_HID_DS4_TOUCHPAD_HEIGHT	942u

enum UsbHidReportSet {
	UsbHidReportSetKeyboard,
	UsbHidReportSetKeyboardMouseConsumer,
	UsbHidReportSetMouse,
	UsbHidReportSetGamepad,
};

enum UsbHidGamepadHat {
	UsbHidGamepadHatCentered,
	UsbHidGamepadHatUp,
	UsbHidGamepadHatUpRight,
	UsbHidGamepadHatRight,
	UsbHidGamepadHatDownRight,
	UsbHidGamepadHatDown,
	UsbHidGamepadHatDownLeft,
	UsbHidGamepadHatLeft,
	UsbHidGamepadHatUpLeft,
};

struct UsbHidDeviceInfo {
	uint16_t vid;
	uint16_t pid;
	char manufacturer[32];
	char product[32];
};

struct UsbHidGamepadFeedback {
	uint8_t rumbleLeft;
	uint8_t rumbleRight;
	uint8_t lightbarRed;
	uint8_t lightbarGreen;
	uint8_t lightbarBlue;
	uint32_t pingSeq;
};

struct UsbHidGamepadTouch {
	bool active;
	bool click;
	uint16_t x;
	uint16_t y;
};

void usbHidDefaultInfo(struct UsbHidDeviceInfo *info);
bool usbHidPrepare(void);
const char *usbHidLastError(void);
bool usbHidBegin(const struct UsbHidDeviceInfo *info);
bool usbHidBeginReportSet(const struct UsbHidDeviceInfo *info, enum UsbHidReportSet reportSet);
void usbHidTask(void);
bool usbHidReady(void);
bool usbHidStarted(void);
void usbHidSetReportsEnabled(bool enabled);
bool usbHidReportsEnabled(void);
bool usbHidKeyboardReport(uint8_t modifiers, const uint8_t keys[6]);
bool usbHidMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t wheel, int8_t pan);
bool usbHidConsumerReport(uint16_t usage);
bool usbHidGamepadReport(int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry, uint8_t hat, uint32_t buttons);
bool usbHidGamepadReportTouch(int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry, uint8_t hat, uint32_t buttons, const struct UsbHidGamepadTouch *touch);
void usbHidGamepadFeedback(struct UsbHidGamepadFeedback *feedback);
void usbHidReleaseAll(void);
void usbHidEnd(void);
void usbHidDropNow(void);

#endif
