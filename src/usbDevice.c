#include <string.h>
#include "2350.h"
#include "timebase.h"
#include "tusb.h"
#include "device/dcd.h"
#include "usbDevice.h"

#define USB_DEVICE_HW_WAIT_MS		100
#define USB_DEVICE_DETACH_WAIT_MS	100
#define USB_DEVICE_HID_REPORT_SIZE	8

#define USB_DEVICE_DEFAULT_VID		0x1209
#define USB_DEVICE_DEFAULT_PID		0xdc32

enum {
	UsbDeviceItfHidKeyboard,
	UsbDeviceItfHidTotal,
};

enum {
	UsbDeviceItfMsc,
	UsbDeviceItfMscTotal,
};

#define USB_DEVICE_HID_CONFIG_TOTAL_LEN	(TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define USB_DEVICE_MSC_CONFIG_TOTAL_LEN	(TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)
#define USB_DEVICE_EP_HID_KEYBOARD	0x81
#define USB_DEVICE_EP_MSC_OUT		0x02
#define USB_DEVICE_EP_MSC_IN		0x82

static bool mHwReady, mInited;
static enum UsbDeviceMode mMode = UsbDeviceModeNone;
static struct UsbDeviceInfo mInfo;
static const char *mLastError = "none";
static uint16_t mStringDesc[33];

void panic(const char *fmt, ...)
{
	(void)fmt;
	while (1) {}
}

uint8_t rp2040_chip_version(void)
{
	return 2;
}

static tusb_desc_device_t mDeviceDesc = {
	.bLength = sizeof(tusb_desc_device_t),
	.bDescriptorType = TUSB_DESC_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
	.idVendor = USB_DEVICE_DEFAULT_VID,
	.idProduct = USB_DEVICE_DEFAULT_PID,
	.bcdDevice = 0x0100,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 0,
	.bNumConfigurations = 1,
};

static const uint8_t mHidReportDesc[] = {
	TUD_HID_REPORT_DESC_KEYBOARD()
};

static const uint8_t mHidConfigDesc[] = {
	TUD_CONFIG_DESCRIPTOR(1, UsbDeviceItfHidTotal, 0, USB_DEVICE_HID_CONFIG_TOTAL_LEN, 0, 50),
	TUD_HID_DESCRIPTOR(UsbDeviceItfHidKeyboard, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(mHidReportDesc),
		USB_DEVICE_EP_HID_KEYBOARD, USB_DEVICE_HID_REPORT_SIZE, 1),
};

static const uint8_t mMscConfigDesc[] = {
	TUD_CONFIG_DESCRIPTOR(1, UsbDeviceItfMscTotal, 0, USB_DEVICE_MSC_CONFIG_TOTAL_LEN, 0, 50),
	TUD_MSC_DESCRIPTOR(UsbDeviceItfMsc, 0, USB_DEVICE_EP_MSC_OUT, USB_DEVICE_EP_MSC_IN, 64),
};

_Static_assert(sizeof(mHidReportDesc) >= 63, "unexpected keyboard report descriptor size");
_Static_assert(sizeof(mHidConfigDesc) == USB_DEVICE_HID_CONFIG_TOTAL_LEN, "unexpected HID configuration descriptor size");
_Static_assert(sizeof(mMscConfigDesc) == USB_DEVICE_MSC_CONFIG_TOTAL_LEN, "unexpected MSC configuration descriptor size");

static bool usbDevicePrvWaitBits(const volatile uint32_t *reg, uint32_t bits, bool set)
{
	uint64_t end = getTime() + (uint64_t)USB_DEVICE_HW_WAIT_MS * (TICKS_PER_SECOND / 1000);

	while (getTime() < end) {
		uint32_t val = *reg & bits;

		if (set ? val == bits : val == 0)
			return true;
	}
	return false;
}

static void usbDevicePrvWaitMs(uint32_t msec)
{
	uint64_t end = getTime() + (uint64_t)msec * (TICKS_PER_SECOND / 1000);

	while (getTime() < end)
		usbDeviceTask();
}

static void usbDevicePrvNormalizeInfo(const struct UsbDeviceInfo *info)
{
	if (info)
		mInfo = *info;
	else
		memset(&mInfo, 0, sizeof(mInfo));
	if (!mInfo.vid)
		mInfo.vid = USB_DEVICE_DEFAULT_VID;
	if (!mInfo.pid)
		mInfo.pid = USB_DEVICE_DEFAULT_PID;
	if (!mInfo.manufacturer[0])
		strcpy(mInfo.manufacturer, "DC32");
	if (!mInfo.product[0])
		strcpy(mInfo.product, "DC32 USB");
	mDeviceDesc.idVendor = mInfo.vid;
	mDeviceDesc.idProduct = mInfo.pid;
}

bool usbDevicePrepare(void)
{
	uint32_t units;

	mLastError = "none";
	if (mHwReady)
		return true;

	units = RESETS_RESET_PLL_USB_BITS;
	resets_hw->reset |= units;
	resets_hw->reset &=~ units;
	if (!usbDevicePrvWaitBits(&resets_hw->reset_done, units, true)) {
		mLastError = "PLL reset timeout";
		return false;
	}

	pll_usb_hw->pwr |= PLL_PWR_VCOPD_BITS | PLL_PWR_POSTDIVPD_BITS | PLL_PWR_PD_BITS;
	pll_usb_hw->cs = (pll_usb_hw->cs &~ (PLL_CS_BYPASS_BITS | PLL_CS_REFDIV_BITS)) | (1 << PLL_CS_REFDIV_LSB);
	pll_usb_hw->fbdiv_int = (pll_usb_hw->fbdiv_int &~ PLL_FBDIV_INT_BITS) | (100 << PLL_FBDIV_INT_LSB);
	pll_usb_hw->prim = (pll_usb_hw->prim &~ (PLL_PRIM_POSTDIV1_BITS | PLL_PRIM_POSTDIV2_BITS)) |
		(5 << PLL_PRIM_POSTDIV1_LSB) | (5 << PLL_PRIM_POSTDIV2_LSB);
	pll_usb_hw->pwr &=~ (PLL_PWR_VCOPD_BITS | PLL_PWR_POSTDIVPD_BITS | PLL_PWR_PD_BITS);
	if (!usbDevicePrvWaitBits(&pll_usb_hw->cs, PLL_CS_LOCK_BITS, true)) {
		mLastError = "PLL lock timeout";
		return false;
	}
	pll_usb_hw->cs &=~ PLL_CS_BYPASS_BITS;

	clocks_hw->clk[clk_usb].ctrl &=~ CLOCKS_CLK_USB_CTRL_ENABLE_BITS;
	clocks_hw->clk[clk_usb].div = 1 << CLOCKS_CLK_USB_DIV_INT_LSB;
	clocks_hw->clk[clk_usb].ctrl = CLOCKS_CLK_USB_CTRL_ENABLE_BITS |
		(CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB << CLOCKS_CLK_USB_CTRL_AUXSRC_LSB);

	mHwReady = true;
	return true;
}

const char *usbDeviceLastError(void)
{
	return mLastError;
}

bool usbDeviceBegin(enum UsbDeviceMode mode, const struct UsbDeviceInfo *info)
{
	if (mInited) {
		usbDeviceEnd();
		usbDevicePrvWaitMs(USB_DEVICE_DETACH_WAIT_MS);
	}

	usbDevicePrvNormalizeInfo(info);
	if (!usbDevicePrepare())
		return false;

	mMode = mode;
	if (!tud_init(0)) {
		mMode = UsbDeviceModeNone;
		mLastError = "TinyUSB init failed";
		return false;
	}

	mInited = true;
	tud_connect();
	usbDeviceTask();
	return true;
}

void usbDeviceTask(void)
{
	if (!mInited)
		return;
	dcd_int_handler(0);
	tud_task();
}

bool usbDeviceMounted(void)
{
	usbDeviceTask();
	return mInited && tud_mounted();
}

bool usbDeviceStarted(enum UsbDeviceMode mode)
{
	usbDeviceTask();
	return mInited && mMode == mode;
}

enum UsbDeviceMode usbDeviceMode(void)
{
	return mInited ? mMode : UsbDeviceModeNone;
}

void usbDeviceEnd(void)
{
	if (!mInited)
		return;
	tud_disconnect();
	dcd_disconnect(0);
	dcd_int_disable(0);
	(void)tud_deinit(0);
	mInited = false;
	mMode = UsbDeviceModeNone;
}

void usbDeviceDropNow(void)
{
	if (!mInited)
		return;
	dcd_disconnect(0);
	dcd_int_disable(0);
	(void)tud_deinit(0);
	mInited = false;
	mMode = UsbDeviceModeNone;
}

uint8_t const *tud_descriptor_device_cb(void)
{
	return (const uint8_t*)&mDeviceDesc;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
	(void)index;
	if (mMode == UsbDeviceModeMsc)
		return mMscConfigDesc;
	return mHidConfigDesc;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
	(void)instance;
	return mHidReportDesc;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
	const char *str = NULL;
	uint32_t i, len;

	(void)langid;
	if (!index) {
		mStringDesc[0] = (TUSB_DESC_STRING << 8) | (2 * 1 + 2);
		mStringDesc[1] = 0x0409;
		return mStringDesc;
	}
	if (index == 1)
		str = mInfo.manufacturer;
	else if (index == 2)
		str = mInfo.product;
	else
		return NULL;

	len = strlen(str);
	if (len > sizeof(mStringDesc) / sizeof(mStringDesc[0]) - 1)
		len = sizeof(mStringDesc) / sizeof(mStringDesc[0]) - 1;
	for (i = 0; i < len; i++)
		mStringDesc[i + 1] = (uint8_t)str[i];
	mStringDesc[0] = (TUSB_DESC_STRING << 8) | (2 * len + 2);
	return mStringDesc;
}
