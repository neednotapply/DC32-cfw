#include <string.h>
#include "timebase.h"
#include "tusb.h"
#include "usbDevice.h"
#include "usbHid.h"

#define USB_HID_KEYBOARD_REPORT_SIZE	8
#define USB_HID_REPORT_TIMEOUT_MS		500
#define USB_HID_CLEANUP_TIMEOUT_MS		50

static struct UsbHidDeviceInfo mInfo;
static bool mReportsEnabled;

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

bool usbHidBegin(const struct UsbHidDeviceInfo *info)
{
	struct UsbDeviceInfo usbInfo;

	usbHidPrvNormalizeInfo(info);
	memset(&usbInfo, 0, sizeof(usbInfo));
	usbInfo.vid = mInfo.vid;
	usbInfo.pid = mInfo.pid;
	strcpy(usbInfo.manufacturer, mInfo.manufacturer);
	strcpy(usbInfo.product, mInfo.product);
	if (!usbDeviceBegin(UsbDeviceModeHid, &usbInfo))
		return false;

	mReportsEnabled = false;
	return true;
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

static bool usbHidPrvSendReport(uint8_t modifiers, const uint8_t keys[6], bool force)
{
	uint32_t timeoutMs = force ? USB_HID_CLEANUP_TIMEOUT_MS : USB_HID_REPORT_TIMEOUT_MS;
	uint64_t end = getTime() + ((uint64_t)timeoutMs * TICKS_PER_SECOND) / 1000;

	if (!force && !mReportsEnabled)
		return false;
	while (getTime() < end) {
		usbHidTask();
		if (tud_mounted() && tud_hid_ready())
			return tud_hid_keyboard_report(0, modifiers, keys);
	}
	return false;
}

bool usbHidKeyboardReport(uint8_t modifiers, const uint8_t keys[6])
{
	return usbHidPrvSendReport(modifiers, keys, false);
}

static void usbHidPrvReleaseAll(bool force)
{
	uint8_t keys[6] = {0};

	(void)usbHidPrvSendReport(0, keys, force);
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

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
	(void)instance;
	(void)report_id;
	(void)report_type;
	if (reqlen > USB_HID_KEYBOARD_REPORT_SIZE)
		reqlen = USB_HID_KEYBOARD_REPORT_SIZE;
	memset(buffer, 0, reqlen);
	return reqlen;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, const uint8_t *buffer, uint16_t bufsize)
{
	(void)instance;
	(void)report_id;
	(void)report_type;
	(void)buffer;
	(void)bufsize;
}
