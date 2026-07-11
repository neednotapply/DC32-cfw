#include "usbCdc.h"

#include <string.h>
#include "tusb.h"
#include "usbDevice.h"

static bool mStarted;
static const char *mLastError = "none";

void usbCdcDefaultInfo(struct UsbCdcDeviceInfo *info)
{
	if (!info)
		return;
	memset(info, 0, sizeof(*info));
	info->vid = USB_CDC_DEFAULT_VID;
	info->pid = USB_CDC_DEFAULT_PID;
	strcpy(info->manufacturer, "DC32");
	strcpy(info->product, "DC32 RaspyJack Remote");
}

const char *usbCdcLastError(void)
{
	return mLastError;
}

bool usbCdcBegin(const struct UsbCdcDeviceInfo *info)
{
	struct UsbCdcDeviceInfo defaults;
	struct UsbDeviceInfo deviceInfo;

	usbCdcDefaultInfo(&defaults);
	if (info)
		deviceInfo.vid = info->vid ? info->vid : defaults.vid;
	else
		deviceInfo.vid = defaults.vid;
	if (info)
		deviceInfo.pid = info->pid ? info->pid : defaults.pid;
	else
		deviceInfo.pid = defaults.pid;
	strcpy(deviceInfo.manufacturer, info && info->manufacturer[0] ? info->manufacturer : defaults.manufacturer);
	strcpy(deviceInfo.product, info && info->product[0] ? info->product : defaults.product);
	deviceInfo.hidReportSet = UsbDeviceHidReportSetKeyboard;
	mStarted = usbDeviceBegin(UsbDeviceModeCdc, &deviceInfo);
	mLastError = mStarted ? "none" : usbDeviceLastError();
	return mStarted;
}

void usbCdcTask(void)
{
	if (mStarted)
		usbDeviceTask();
}

bool usbCdcStarted(void)
{
	return mStarted && usbDeviceStarted(UsbDeviceModeCdc);
}

bool usbCdcMounted(void)
{
	return usbCdcStarted() && usbDeviceMounted();
}

bool usbCdcConnected(void)
{
	usbCdcTask();
	return usbCdcMounted() && tud_cdc_connected();
}

uint32_t usbCdcRead(void *buffer, uint32_t size)
{
	if (!buffer || !size || !usbCdcStarted())
		return 0;
	return tud_cdc_read(buffer, size);
}

uint32_t usbCdcAvailable(void)
{
	return usbCdcStarted() ? tud_cdc_available() : 0;
}

uint32_t usbCdcWrite(const void *buffer, uint32_t size)
{
	if (!buffer || !size || !usbCdcConnected())
		return 0;
	return tud_cdc_write(buffer, size);
}

uint32_t usbCdcWriteAvailable(void)
{
	return usbCdcConnected() ? tud_cdc_write_available() : 0;
}

void usbCdcFlush(void)
{
	if (usbCdcStarted())
		(void)tud_cdc_write_flush();
}

void usbCdcEnd(void)
{
	if (mStarted)
		usbDeviceEnd();
	mStarted = false;
}
