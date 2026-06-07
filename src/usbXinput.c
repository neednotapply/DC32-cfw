#include <string.h>
#include "tusb.h"
#include "usbDevice.h"
#include "usbXinput.h"

#define USB_XINPUT_REPORT_SIZE		20
#define USB_XINPUT_REPORT_TYPE_INPUT	0x00
#define USB_XINPUT_REPORT_TYPE_LED	0x01

static bool mReportsEnabled;
static struct UsbXinputFeedback mFeedback;

static int16_t usbXinputPrvAxis8To16(int8_t axis)
{
	if (axis >= 0)
		return (int16_t)axis * 32767 / 127;
	return (int16_t)axis * 32768 / 128;
}

static void usbXinputPrvPackAxis(uint8_t *dst, int16_t axis)
{
	dst[0] = (uint8_t)axis;
	dst[1] = (uint8_t)(axis >> 8);
}

static void usbXinputPrvPackReport(uint8_t report[static USB_XINPUT_REPORT_SIZE], int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry, uint8_t hat, uint32_t buttons)
{
	memset(report, 0, USB_XINPUT_REPORT_SIZE);
	report[0] = USB_XINPUT_REPORT_TYPE_INPUT;
	report[1] = USB_XINPUT_REPORT_SIZE;

	switch (hat) {
	case UsbHidGamepadHatUp:
		report[2] |= 0x01;
		break;
	case UsbHidGamepadHatUpRight:
		report[2] |= 0x01 | 0x08;
		break;
	case UsbHidGamepadHatRight:
		report[2] |= 0x08;
		break;
	case UsbHidGamepadHatDownRight:
		report[2] |= 0x02 | 0x08;
		break;
	case UsbHidGamepadHatDown:
		report[2] |= 0x02;
		break;
	case UsbHidGamepadHatDownLeft:
		report[2] |= 0x02 | 0x04;
		break;
	case UsbHidGamepadHatLeft:
		report[2] |= 0x04;
		break;
	case UsbHidGamepadHatUpLeft:
		report[2] |= 0x01 | 0x04;
		break;
	case UsbHidGamepadHatCentered:
	default:
		break;
	}

	if (buttons & USB_HID_GAMEPAD_BUTTON_SELECT)
		report[2] |= 0x20;	// Back
	if (buttons & USB_HID_GAMEPAD_BUTTON_START)
		report[2] |= 0x10;	// Start
	if (buttons & USB_HID_GAMEPAD_BUTTON_L1)
		report[3] |= 0x01;	// Left bumper
	if (buttons & USB_HID_GAMEPAD_BUTTON_R1)
		report[3] |= 0x02;	// Right bumper
	if (buttons & USB_HID_GAMEPAD_BUTTON_HOME)
		report[3] |= 0x04;	// Guide
	if (buttons & USB_HID_GAMEPAD_BUTTON_A)
		report[3] |= 0x10;	// A
	if (buttons & USB_HID_GAMEPAD_BUTTON_B)
		report[3] |= 0x20;	// B

	report[4] = (buttons & USB_HID_GAMEPAD_BUTTON_L2) ? 0xff : (rx > 0 ? (uint8_t)((uint16_t)(rx + 1) * 255 / 128) : 0);
	report[5] = (buttons & USB_HID_GAMEPAD_BUTTON_R2) ? 0xff : (ry > 0 ? (uint8_t)((uint16_t)(ry + 1) * 255 / 128) : 0);
	usbXinputPrvPackAxis(report + 6, usbXinputPrvAxis8To16(x));
	usbXinputPrvPackAxis(report + 8, (int16_t)-usbXinputPrvAxis8To16(y));
	usbXinputPrvPackAxis(report + 10, usbXinputPrvAxis8To16(z));
	usbXinputPrvPackAxis(report + 12, (int16_t)-usbXinputPrvAxis8To16(rz));
}

static bool usbXinputPrvSendReport(int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry, uint8_t hat, uint32_t buttons, bool force)
{
	uint8_t report[USB_XINPUT_REPORT_SIZE];

	if (!force && !mReportsEnabled)
		return false;
	if (!usbXinputReady())
		return false;
	if (tud_vendor_n_write_available(0) < USB_XINPUT_REPORT_SIZE)
		return true;
	usbXinputPrvPackReport(report, x, y, z, rz, rx, ry, hat, buttons);
	if (tud_vendor_n_write(0, report, sizeof(report)) != sizeof(report))
		return false;
	(void)tud_vendor_n_write_flush(0);
	return true;
}

bool usbXinputBegin(void)
{
	struct UsbDeviceInfo info;

	memset(&info, 0, sizeof(info));
	if (!usbDeviceBegin(UsbDeviceModeXinput, &info))
		return false;
	mReportsEnabled = false;
	memset(&mFeedback, 0, sizeof(mFeedback));
	return true;
}

const char *usbXinputLastError(void)
{
	return usbDeviceLastError();
}

void usbXinputTask(void)
{
	usbDeviceTask();
}

bool usbXinputReady(void)
{
	usbXinputTask();
	return usbDeviceStarted(UsbDeviceModeXinput) && tud_mounted() && tud_vendor_n_mounted(0);
}

bool usbXinputStarted(void)
{
	usbXinputTask();
	return usbDeviceStarted(UsbDeviceModeXinput);
}

void usbXinputSetReportsEnabled(bool enabled)
{
	if (mReportsEnabled && !enabled)
		usbXinputReleaseAll();
	mReportsEnabled = enabled;
}

bool usbXinputReport(int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry, uint8_t hat, uint32_t buttons)
{
	return usbXinputPrvSendReport(x, y, z, rz, rx, ry, hat, buttons, false);
}

void usbXinputReleaseAll(void)
{
	(void)usbXinputPrvSendReport(0, 0, 0, 0, 0, 0, UsbHidGamepadHatCentered, 0, true);
}

void usbXinputFeedback(struct UsbXinputFeedback *feedback)
{
	if (feedback)
		*feedback = mFeedback;
}

void usbXinputEnd(void)
{
	if (!usbDeviceStarted(UsbDeviceModeXinput))
		return;
	if (mReportsEnabled && tud_mounted())
		usbXinputReleaseAll();
	mReportsEnabled = false;
	usbDeviceEnd();
}

static void usbXinputPrvParseFeedback(const uint8_t *buffer, uint16_t bufsize)
{
	if (!bufsize)
		return;
	if (buffer[0] == USB_XINPUT_REPORT_TYPE_INPUT && bufsize >= 5) {
		bool wasRumbling = mFeedback.rumbleLeft || mFeedback.rumbleRight;

		mFeedback.rumbleLeft = buffer[3];
		mFeedback.rumbleRight = buffer[4];
		if (!wasRumbling && (mFeedback.rumbleLeft || mFeedback.rumbleRight))
			mFeedback.pingSeq++;
	}
	else if (buffer[0] == USB_XINPUT_REPORT_TYPE_LED && bufsize >= 3) {
		if (mFeedback.ledPattern != buffer[2])
			mFeedback.pingSeq++;
		mFeedback.ledPattern = buffer[2];
	}
}

void tud_vendor_rx_cb(uint8_t itf, uint8_t const *buffer, uint16_t bufsize)
{
	if (itf || !usbDeviceStarted(UsbDeviceModeXinput))
		return;
	usbXinputPrvParseFeedback(buffer, bufsize);
}
