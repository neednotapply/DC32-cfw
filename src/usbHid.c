#include <string.h>
#include "timebase.h"
#include "tusb.h"
#include "usbDevice.h"
#include "usbHid.h"

#define USB_HID_KEYBOARD_REPORT_SIZE	8
#define USB_HID_CONSUMER_REPORT_SIZE	2
#define USB_HID_DS4_INPUT_REPORT_ID		1
#define USB_HID_DS4_INPUT_PAYLOAD_SIZE	63
#define USB_HID_DS4_TOUCH_REPORTS_OFFSET	32
#define USB_HID_DS4_TOUCH_REPORT_SIZE	9
#define USB_HID_DS4_TOUCH_POINT_INACTIVE	0x80u
#define USB_HID_REPORT_TIMEOUT_MS		500
#define USB_HID_CLEANUP_TIMEOUT_MS		50

enum {
	UsbHidReportIdKeyboard = 1,
	UsbHidReportIdMouse,
	UsbHidReportIdConsumer,
};

static struct UsbHidDeviceInfo mInfo;
static enum UsbHidReportSet mReportSet = UsbHidReportSetKeyboard;
static bool mReportsEnabled;
static uint8_t mDs4Counter;
static uint8_t mDs4TouchTimestamp;
static uint8_t mDs4TouchContactId;
static uint16_t mDs4Timestamp;
static bool mDs4TouchActive;
static struct UsbHidGamepadFeedback mGamepadFeedback;

static const uint8_t mDs4FeatureReport02[] = {
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87, 0x22, 0x7b, 0xdd, 0xb2, 0x22,
	0x47, 0xdd, 0xbd, 0x22, 0x43, 0xdd, 0x1c, 0x02, 0x1c, 0x02, 0x7f, 0x1e,
	0x2e, 0xdf, 0x60, 0x1f, 0x4c, 0xe0, 0x3a, 0x1d, 0xc6, 0xde, 0x08, 0x00,
};

static const uint8_t mDs4FeatureReport12[] = {
	0x8b, 0x09, 0x07, 0x6d, 0x66, 0x1c, 0x08, 0x25, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
};

static const uint8_t mDs4FeatureReportA3[] = {
	0x41, 0x75, 0x67, 0x20, 0x20, 0x33, 0x20, 0x32, 0x30, 0x31, 0x33, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x30, 0x37, 0x3a, 0x30, 0x31, 0x3a, 0x31, 0x32,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x31,
	0x03, 0x00, 0x00, 0x00, 0x49, 0x00, 0x05, 0x00, 0x00, 0x80, 0x03, 0x00,
};

_Static_assert(sizeof(mDs4FeatureReport02) == 36, "unexpected DS4 feature report 0x02 size");
_Static_assert(sizeof(mDs4FeatureReport12) == 15, "unexpected DS4 feature report 0x12 size");
_Static_assert(sizeof(mDs4FeatureReportA3) == 48, "unexpected DS4 feature report 0xa3 size");

void usbHidDefaultInfo(struct UsbHidDeviceInfo *info)
{
	memset(info, 0, sizeof(*info));
	info->vid = USB_HID_DEFAULT_VID;
	info->pid = USB_HID_DEFAULT_PID;
	strcpy(info->manufacturer, "DC32");
	strcpy(info->product, "DC32 HID");
}

static void usbHidPrvNormalizeInfo(const struct UsbHidDeviceInfo *info)
{
	struct UsbHidDeviceInfo defaultInfo;

	if (!info) {
		usbHidDefaultInfo(&defaultInfo);
		info = &defaultInfo;
	}
	mInfo = *info;
	if (!mInfo.vid)
		mInfo.vid = USB_HID_DEFAULT_VID;
	if (!mInfo.pid)
		mInfo.pid = USB_HID_DEFAULT_PID;
	if (!mInfo.manufacturer[0])
		strcpy(mInfo.manufacturer, "DC32");
	if (!mInfo.product[0])
		strcpy(mInfo.product, "DC32 HID");
}

bool usbHidPrepare(void)
{
	return usbDevicePrepare();
}

const char *usbHidLastError(void)
{
	return usbDeviceLastError();
}

static enum UsbDeviceHidReportSet usbHidPrvDeviceReportSet(enum UsbHidReportSet reportSet)
{
	switch (reportSet) {
	case UsbHidReportSetKeyboardMouseConsumer:
		return UsbDeviceHidReportSetKeyboardMouseConsumer;
	case UsbHidReportSetMouse:
		return UsbDeviceHidReportSetMouse;
	case UsbHidReportSetGamepad:
		return UsbDeviceHidReportSetGamepad;
	case UsbHidReportSetKeyboard:
	default:
		return UsbDeviceHidReportSetKeyboard;
	}
}

bool usbHidBeginReportSet(const struct UsbHidDeviceInfo *info, enum UsbHidReportSet reportSet)
{
	struct UsbDeviceInfo usbInfo;

	if (reportSet > UsbHidReportSetGamepad)
		reportSet = UsbHidReportSetKeyboard;
	usbHidPrvNormalizeInfo(info);
	memset(&usbInfo, 0, sizeof(usbInfo));
	usbInfo.vid = mInfo.vid;
	usbInfo.pid = mInfo.pid;
	strcpy(usbInfo.manufacturer, mInfo.manufacturer);
	strcpy(usbInfo.product, mInfo.product);
	usbInfo.hidReportSet = usbHidPrvDeviceReportSet(reportSet);
	if (!usbDeviceBegin(UsbDeviceModeHid, &usbInfo))
		return false;

	mReportSet = reportSet;
	mReportsEnabled = false;
	mDs4Counter = 0;
	mDs4TouchTimestamp = 0;
	mDs4TouchContactId = 0;
	mDs4Timestamp = 0;
	mDs4TouchActive = false;
	memset(&mGamepadFeedback, 0, sizeof(mGamepadFeedback));
	return true;
}

bool usbHidBegin(const struct UsbHidDeviceInfo *info)
{
	return usbHidBeginReportSet(info, UsbHidReportSetKeyboard);
}

void usbHidTask(void)
{
	usbDeviceTask();
}

bool usbHidReady(void)
{
	usbHidTask();
	return usbDeviceStarted(UsbDeviceModeHid) && tud_mounted() && tud_hid_ready();
}

bool usbHidStarted(void)
{
	usbHidTask();
	return usbDeviceStarted(UsbDeviceModeHid);
}

bool usbHidReportsEnabled(void)
{
	return mReportsEnabled;
}

static bool usbHidPrvWaitReady(bool force)
{
	uint32_t timeoutMs = force ? USB_HID_CLEANUP_TIMEOUT_MS : USB_HID_REPORT_TIMEOUT_MS;
	uint64_t end = getTime() + ((uint64_t)timeoutMs * TICKS_PER_SECOND) / 1000;

	if (!force && !mReportsEnabled)
		return false;
	while (getTime() < end) {
		usbHidTask();
		if (tud_mounted() && tud_hid_ready())
			return true;
	}
	return false;
}

static bool usbHidPrvSendKeyboardReport(uint8_t modifiers, const uint8_t keys[6], bool force)
{
	uint8_t reportId;

	if (mReportSet != UsbHidReportSetKeyboard && mReportSet != UsbHidReportSetKeyboardMouseConsumer)
		return false;
	if (!usbHidPrvWaitReady(force))
		return false;
	reportId = mReportSet == UsbHidReportSetKeyboardMouseConsumer ? UsbHidReportIdKeyboard : 0;
	return tud_hid_keyboard_report(reportId, modifiers, keys);
}

bool usbHidKeyboardReport(uint8_t modifiers, const uint8_t keys[6])
{
	return usbHidPrvSendKeyboardReport(modifiers, keys, false);
}

static bool usbHidPrvSendMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t wheel, int8_t pan, bool force)
{
	uint8_t reportId;

	if (mReportSet != UsbHidReportSetMouse && mReportSet != UsbHidReportSetKeyboardMouseConsumer)
		return false;
	if (!usbHidPrvWaitReady(force))
		return false;
	reportId = mReportSet == UsbHidReportSetKeyboardMouseConsumer ? UsbHidReportIdMouse : 0;
	return tud_hid_mouse_report(reportId, buttons, x, y, wheel, pan);
}

bool usbHidMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t wheel, int8_t pan)
{
	return usbHidPrvSendMouseReport(buttons, x, y, wheel, pan, false);
}

static bool usbHidPrvSendConsumerReport(uint16_t usage, bool force)
{
	if (mReportSet != UsbHidReportSetKeyboardMouseConsumer)
		return false;
	if (!usbHidPrvWaitReady(force))
		return false;
	return tud_hid_report(UsbHidReportIdConsumer, &usage, sizeof(usage));
}

bool usbHidConsumerReport(uint16_t usage)
{
	return usbHidPrvSendConsumerReport(usage, false);
}

static uint8_t usbHidPrvDs4Axis(int8_t axis)
{
	int16_t val = (int16_t)axis + 128;

	if (val < 0)
		return 0;
	if (val > 255)
		return 255;
	return (uint8_t)val;
}

static uint8_t usbHidPrvDs4Trigger(int8_t trigger)
{
	if (trigger <= 0)
		return 0;
	return usbHidPrvDs4Axis(trigger);
}

static uint8_t usbHidPrvDs4Hat(uint8_t hat)
{
	static const uint8_t ds4Hat[] = {
		8,	// centered
		0,	// up
		1,	// up-right
		2,	// right
		3,	// down-right
		4,	// down
		5,	// down-left
		6,	// left
		7,	// up-left
	};

	if (hat < sizeof(ds4Hat) / sizeof(*ds4Hat))
		return ds4Hat[hat];
	return 8;
}

static void usbHidPrvPackDs4TouchPoint(uint8_t *dst, bool active, uint8_t contactId, uint16_t x, uint16_t y)
{
	if (!active) {
		dst[0] = USB_HID_DS4_TOUCH_POINT_INACTIVE | (contactId & 0x7f);
		dst[1] = 0;
		dst[2] = 0;
		dst[3] = 0;
		return;
	}

	if (x >= USB_HID_DS4_TOUCHPAD_WIDTH)
		x = USB_HID_DS4_TOUCHPAD_WIDTH - 1;
	if (y >= USB_HID_DS4_TOUCHPAD_HEIGHT)
		y = USB_HID_DS4_TOUCHPAD_HEIGHT - 1;
	dst[0] = contactId & 0x7f;
	dst[1] = (uint8_t)x;
	dst[2] = (uint8_t)(((x >> 8) & 0x0f) | ((y & 0x0f) << 4));
	dst[3] = (uint8_t)(y >> 4);
}

static void usbHidPrvPackDs4Touch(uint8_t report[static USB_HID_DS4_INPUT_PAYLOAD_SIZE], const struct UsbHidGamepadTouch *touch)
{
	uint8_t *touchReport = report + USB_HID_DS4_TOUCH_REPORTS_OFFSET + 1;
	bool active = touch && touch->active;

	if (active && !mDs4TouchActive)
		mDs4TouchContactId = (mDs4TouchContactId + 1) & 0x7f;
	mDs4TouchActive = active;
	report[USB_HID_DS4_TOUCH_REPORTS_OFFSET] = 1;
	touchReport[0] = mDs4TouchTimestamp++;
	usbHidPrvPackDs4TouchPoint(touchReport + 1, active, mDs4TouchContactId, active ? touch->x : 0, active ? touch->y : 0);
	usbHidPrvPackDs4TouchPoint(touchReport + 5, false, 1, 0, 0);
}

static void usbHidPrvPackDs4NeutralTouch(uint8_t report[static USB_HID_DS4_INPUT_PAYLOAD_SIZE])
{
	uint8_t *touchReport = report + USB_HID_DS4_TOUCH_REPORTS_OFFSET + 1;

	report[USB_HID_DS4_TOUCH_REPORTS_OFFSET] = 1;
	touchReport[0] = 0;
	usbHidPrvPackDs4TouchPoint(touchReport + 1, false, 0, 0, 0);
	usbHidPrvPackDs4TouchPoint(touchReport + 5, false, 1, 0, 0);
}

static bool usbHidPrvSendDs4Report(int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry, uint8_t hat, uint32_t buttons, const struct UsbHidGamepadTouch *touch)
{
	uint8_t report[USB_HID_DS4_INPUT_PAYLOAD_SIZE] = {0};

	report[0] = usbHidPrvDs4Axis(x);
	report[1] = usbHidPrvDs4Axis(y);
	report[2] = usbHidPrvDs4Axis(z);
	report[3] = usbHidPrvDs4Axis(rz);
	report[4] = usbHidPrvDs4Hat(hat) & 0x0f;
	if (buttons & USB_HID_GAMEPAD_BUTTON_A)
		report[4] |= 1u << 5;	// Cross
	if (buttons & USB_HID_GAMEPAD_BUTTON_B)
		report[4] |= 1u << 6;	// Circle
	if (buttons & USB_HID_GAMEPAD_BUTTON_L1)
		report[5] |= 1u << 0;
	if (buttons & USB_HID_GAMEPAD_BUTTON_R1)
		report[5] |= 1u << 1;
	if (buttons & USB_HID_GAMEPAD_BUTTON_L2)
		report[5] |= 1u << 2;
	if (buttons & USB_HID_GAMEPAD_BUTTON_R2)
		report[5] |= 1u << 3;
	if (buttons & USB_HID_GAMEPAD_BUTTON_SELECT)
		report[5] |= 1u << 4;	// Share
	if (buttons & USB_HID_GAMEPAD_BUTTON_START)
		report[5] |= 1u << 5;	// Options
	report[6] = (uint8_t)((mDs4Counter++ & 0x3f) << 2);
	if (buttons & USB_HID_GAMEPAD_BUTTON_HOME)
		report[6] |= 1u << 0;	// PS
	if ((buttons & USB_HID_GAMEPAD_BUTTON_TOUCHPAD) || (touch && touch->click))
		report[6] |= 1u << 1;	// Touchpad click
	report[7] = usbHidPrvDs4Trigger(rx);
	report[8] = usbHidPrvDs4Trigger(ry);
	mDs4Timestamp += 188;
	report[9] = (uint8_t)mDs4Timestamp;
	report[10] = (uint8_t)(mDs4Timestamp >> 8);
	report[11] = 0x1b;	// neutral temperature-ish value used by DS4 captures
	report[29] = 0x1b;	// cable connected and full battery
	usbHidPrvPackDs4Touch(report, touch);

	return tud_hid_report(USB_HID_DS4_INPUT_REPORT_ID, report, sizeof(report));
}

static bool usbHidPrvSendGamepadReport(int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry, uint8_t hat, uint32_t buttons, const struct UsbHidGamepadTouch *touch, bool force)
{
	if (mReportSet != UsbHidReportSetGamepad)
		return false;
	if (!usbHidPrvWaitReady(force))
		return false;
	return usbHidPrvSendDs4Report(x, y, z, rz, rx, ry, hat, buttons, touch);
}

bool usbHidGamepadReport(int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry, uint8_t hat, uint32_t buttons)
{
	return usbHidPrvSendGamepadReport(x, y, z, rz, rx, ry, hat, buttons, NULL, false);
}

bool usbHidGamepadReportTouch(int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry, uint8_t hat, uint32_t buttons, const struct UsbHidGamepadTouch *touch)
{
	return usbHidPrvSendGamepadReport(x, y, z, rz, rx, ry, hat, buttons, touch, false);
}

void usbHidGamepadFeedback(struct UsbHidGamepadFeedback *feedback)
{
	if (feedback)
		*feedback = mGamepadFeedback;
}

static void usbHidPrvParseDs4SetReport(uint8_t report_id, hid_report_type_t report_type, const uint8_t *buffer, uint16_t bufsize)
{
	bool wasRumbling, changedLed = false;
	uint8_t flags;

	if (mReportSet != UsbHidReportSetGamepad || report_type != HID_REPORT_TYPE_OUTPUT || !buffer || !bufsize)
		return;

	if (!report_id && buffer[0] == 0x05) {
		report_id = buffer[0];
		buffer++;
		bufsize--;
	}
	if (report_id != 0x05 || bufsize < 5)
		return;

	flags = buffer[0];
	wasRumbling = mGamepadFeedback.rumbleLeft || mGamepadFeedback.rumbleRight;
	if ((flags & 0x01) || bufsize >= 5) {
		mGamepadFeedback.rumbleRight = buffer[3];
		mGamepadFeedback.rumbleLeft = buffer[4];
		if (!wasRumbling && (mGamepadFeedback.rumbleLeft || mGamepadFeedback.rumbleRight))
			mGamepadFeedback.pingSeq++;
	}

	if (bufsize >= 8 && (flags & 0x02)) {
		changedLed = mGamepadFeedback.lightbarRed != buffer[5] ||
			mGamepadFeedback.lightbarGreen != buffer[6] ||
			mGamepadFeedback.lightbarBlue != buffer[7];
		mGamepadFeedback.lightbarRed = buffer[5];
		mGamepadFeedback.lightbarGreen = buffer[6];
		mGamepadFeedback.lightbarBlue = buffer[7];
		if (changedLed)
			mGamepadFeedback.pingSeq++;
	}
	if (bufsize >= 10 && (flags & 0x04) && (buffer[8] || buffer[9]))
		mGamepadFeedback.pingSeq++;
}

static void usbHidPrvReleaseAll(bool force)
{
	uint8_t keys[6] = {0};

	switch (mReportSet) {
	case UsbHidReportSetKeyboardMouseConsumer:
		(void)usbHidPrvSendKeyboardReport(0, keys, force);
		(void)usbHidPrvSendMouseReport(0, 0, 0, 0, 0, force);
		(void)usbHidPrvSendConsumerReport(0, force);
		break;
	case UsbHidReportSetMouse:
		(void)usbHidPrvSendMouseReport(0, 0, 0, 0, 0, force);
		break;
	case UsbHidReportSetGamepad:
		(void)usbHidPrvSendGamepadReport(0, 0, 0, 0, 0, 0, UsbHidGamepadHatCentered, 0, NULL, force);
		break;
	case UsbHidReportSetKeyboard:
	default:
		(void)usbHidPrvSendKeyboardReport(0, keys, force);
		break;
	}
}

void usbHidReleaseAll(void)
{
	usbHidPrvReleaseAll(false);
}

void usbHidSetReportsEnabled(bool enabled)
{
	if (mReportsEnabled && !enabled)
		usbHidPrvReleaseAll(true);
	mReportsEnabled = enabled;
}

void usbHidEnd(void)
{
	if (!usbDeviceStarted(UsbDeviceModeHid))
		return;
	if (mReportsEnabled && tud_mounted())
		usbHidPrvReleaseAll(true);
	mReportsEnabled = false;
	usbDeviceEnd();
}

void usbHidDropNow(void)
{
	mReportsEnabled = false;
	usbDeviceDropNow();
}

static uint16_t usbHidPrvGetReportSize(uint8_t report_id)
{
	switch (mReportSet) {
	case UsbHidReportSetKeyboardMouseConsumer:
		if (report_id == UsbHidReportIdKeyboard)
			return USB_HID_KEYBOARD_REPORT_SIZE;
		if (report_id == UsbHidReportIdMouse)
			return sizeof(hid_mouse_report_t);
		if (report_id == UsbHidReportIdConsumer)
			return USB_HID_CONSUMER_REPORT_SIZE;
		return CFG_TUD_HID_EP_BUFSIZE;
	case UsbHidReportSetMouse:
		return sizeof(hid_mouse_report_t);
	case UsbHidReportSetGamepad:
		return USB_HID_DS4_INPUT_PAYLOAD_SIZE;
	case UsbHidReportSetKeyboard:
	default:
		return USB_HID_KEYBOARD_REPORT_SIZE;
	}
}

static uint16_t usbHidPrvDs4FeatureReportSize(uint8_t report_id)
{
	switch (report_id) {
	case 0x02:
	case 0x04:
		return 36;
	case 0x08:
	case 0x91:
	case 0x92:
		return 3;
	case 0x10:
	case 0x84:
		return 4;
	case 0x11:
	case 0x89:
	case 0xaf:
		return 2;
	case 0x12:
	case 0xf2:
		return 15;
	case 0x13:
		return 22;
	case 0x14:
		return 16;
	case 0x15:
		return 44;
	case 0x80:
	case 0x81:
	case 0x85:
	case 0x86:
	case 0xa0:
		return 6;
	case 0x82:
	case 0x90:
		return 5;
	case 0x83:
	case 0xa1:
	case 0xa2:
	case 0xa7:
	case 0xa8:
	case 0xaa:
	case 0xae:
		return 1;
	case 0x87:
		return 35;
	case 0x88:
		return 34;
	case 0x93:
		return 12;
	case 0xa3:
		return 48;
	case 0xa4:
		return 13;
	case 0xa5:
	case 0xa6:
		return 21;
	case 0xa9:
		return 8;
	case 0xab:
	case 0xac:
		return 57;
	case 0xad:
		return 11;
	case 0xb0:
	case 0xf0:
	case 0xf1:
		return 63;
	default:
		return 0;
	}
}

static uint16_t usbHidPrvCopyReport(uint8_t *buffer, uint16_t reqlen, const uint8_t *src, uint16_t srcLen)
{
	uint16_t len = srcLen;

	if (len > reqlen)
		len = reqlen;
	memcpy(buffer, src, len);
	return len;
}

static uint16_t usbHidPrvGetDs4Report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
	uint16_t reportSize;

	if (report_type == HID_REPORT_TYPE_INPUT && report_id == USB_HID_DS4_INPUT_REPORT_ID) {
		uint8_t report[USB_HID_DS4_INPUT_PAYLOAD_SIZE] = {0};

		report[0] = 0x80;
		report[1] = 0x80;
		report[2] = 0x80;
		report[3] = 0x80;
		report[4] = 0x08;
		report[11] = 0x1b;
		report[29] = 0x1b;
		usbHidPrvPackDs4NeutralTouch(report);
		return usbHidPrvCopyReport(buffer, reqlen, report, sizeof(report));
	}
	if (report_type == HID_REPORT_TYPE_FEATURE) {
		if (report_id == 0x02)
			return usbHidPrvCopyReport(buffer, reqlen, mDs4FeatureReport02, sizeof(mDs4FeatureReport02));
		if (report_id == 0x12)
			return usbHidPrvCopyReport(buffer, reqlen, mDs4FeatureReport12, sizeof(mDs4FeatureReport12));
		if (report_id == 0xa3)
			return usbHidPrvCopyReport(buffer, reqlen, mDs4FeatureReportA3, sizeof(mDs4FeatureReportA3));
		reportSize = usbHidPrvDs4FeatureReportSize(report_id);
		if (reportSize) {
			if (reportSize > reqlen)
				reportSize = reqlen;
			memset(buffer, 0, reportSize);
			return reportSize;
		}
	}
	if (report_type == HID_REPORT_TYPE_OUTPUT && report_id == 0x05) {
		reportSize = 31;
		if (reportSize > reqlen)
			reportSize = reqlen;
		memset(buffer, 0, reportSize);
		return reportSize;
	}
	return 0;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
	uint16_t reportSize = usbHidPrvGetReportSize(report_id);

	(void)instance;
	if (mReportSet == UsbHidReportSetGamepad)
		return usbHidPrvGetDs4Report(report_id, report_type, buffer, reqlen);
	if (reqlen > reportSize)
		reqlen = reportSize;
	memset(buffer, 0, reqlen);
	return reqlen;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, const uint8_t *buffer, uint16_t bufsize)
{
	(void)instance;
	usbHidPrvParseDs4SetReport(report_id, report_type, buffer, bufsize);
}
