#include <string.h>
#include "2350.h"
#include "timebase.h"
#include "tusb.h"
#include "device/dcd.h"
#include "usbDevice.h"

#define USB_DEVICE_HW_WAIT_MS		100
#define USB_DEVICE_DETACH_WAIT_MS	100
#define USB_DEVICE_HID_EP_SIZE		64

#define USB_DEVICE_DEFAULT_VID		0x1209
#define USB_DEVICE_DEFAULT_PID		0xdc32
#define USB_DEVICE_XINPUT_VID		0x045e
#define USB_DEVICE_XINPUT_PID		0x028e

enum {
	UsbDeviceItfHidKeyboard,
	UsbDeviceItfHidTotal,
};

enum {
	UsbDeviceHidReportIdKeyboard = 1,
	UsbDeviceHidReportIdMouse,
	UsbDeviceHidReportIdConsumer,
};

enum {
	UsbDeviceItfMsc,
	UsbDeviceItfMscTotal,
};

#define USB_DEVICE_HID_CONFIG_TOTAL_LEN	(TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define USB_DEVICE_HID_INOUT_CONFIG_TOTAL_LEN	(TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)
#define USB_DEVICE_MSC_CONFIG_TOTAL_LEN	(TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)
#define USB_DEVICE_XINPUT_CONFIG_TOTAL_LEN	153
#define USB_DEVICE_EP_HID_KEYBOARD	0x81
#define USB_DEVICE_EP_HID_GAMEPAD_OUT	0x03
#define USB_DEVICE_EP_HID_GAMEPAD_IN	0x84
#define USB_DEVICE_EP_MSC_OUT		0x02
#define USB_DEVICE_EP_MSC_IN		0x82

static bool mHwReady, mInited;
static enum UsbDeviceMode mMode = UsbDeviceModeNone;
static struct UsbDeviceInfo mInfo;
static const char *mLastError = "none";
static uint16_t mStringDesc[96];

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

static const uint8_t mHidKeyboardReportDesc[] = {
	TUD_HID_REPORT_DESC_KEYBOARD()
};

static const uint8_t mHidKeyboardMouseConsumerReportDesc[] = {
	TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(UsbDeviceHidReportIdKeyboard)),
	TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(UsbDeviceHidReportIdMouse)),
	TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(UsbDeviceHidReportIdConsumer)),
};

static const uint8_t mHidMouseReportDesc[] = {
	TUD_HID_REPORT_DESC_MOUSE()
};

static const uint8_t mHidGamepadReportDesc[] = {
	// DualShock 4 USB HID report descriptor for report ID 1 input reports.
	0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x85, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35,
	0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95, 0x04, 0x81, 0x02, 0x09, 0x39, 0x15, 0x00, 0x25,
	0x07, 0x35, 0x00, 0x46, 0x3b, 0x01, 0x65, 0x14, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42, 0x65, 0x00,
	0x05, 0x09, 0x19, 0x01, 0x29, 0x0e, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0e, 0x81, 0x02,
	0x06, 0x00, 0xff, 0x09, 0x20, 0x75, 0x06, 0x95, 0x01, 0x15, 0x00, 0x25, 0x7f, 0x81, 0x02, 0x05,
	0x01, 0x09, 0x33, 0x09, 0x34, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02,
	0x06, 0x00, 0xff, 0x09, 0x21, 0x95, 0x36, 0x81, 0x02, 0x85, 0x05, 0x09, 0x22, 0x95, 0x1f, 0x91,
	0x02, 0x85, 0x04, 0x09, 0x23, 0x95, 0x24, 0xb1, 0x02, 0x85, 0x02, 0x09, 0x24, 0x95, 0x24, 0xb1,
	0x02, 0x85, 0x08, 0x09, 0x25, 0x95, 0x03, 0xb1, 0x02, 0x85, 0x10, 0x09, 0x26, 0x95, 0x04, 0xb1,
	0x02, 0x85, 0x11, 0x09, 0x27, 0x95, 0x02, 0xb1, 0x02, 0x85, 0x12, 0x06, 0x02, 0xff, 0x09, 0x21,
	0x95, 0x0f, 0xb1, 0x02, 0x85, 0x13, 0x09, 0x22, 0x95, 0x16, 0xb1, 0x02, 0x85, 0x14, 0x06, 0x05,
	0xff, 0x09, 0x20, 0x95, 0x10, 0xb1, 0x02, 0x85, 0x15, 0x09, 0x21, 0x95, 0x2c, 0xb1, 0x02, 0x06,
	0x80, 0xff, 0x85, 0x80, 0x09, 0x20, 0x95, 0x06, 0xb1, 0x02, 0x85, 0x81, 0x09, 0x21, 0x95, 0x06,
	0xb1, 0x02, 0x85, 0x82, 0x09, 0x22, 0x95, 0x05, 0xb1, 0x02, 0x85, 0x83, 0x09, 0x23, 0x95, 0x01,
	0xb1, 0x02, 0x85, 0x84, 0x09, 0x24, 0x95, 0x04, 0xb1, 0x02, 0x85, 0x85, 0x09, 0x25, 0x95, 0x06,
	0xb1, 0x02, 0x85, 0x86, 0x09, 0x26, 0x95, 0x06, 0xb1, 0x02, 0x85, 0x87, 0x09, 0x27, 0x95, 0x23,
	0xb1, 0x02, 0x85, 0x88, 0x09, 0x28, 0x95, 0x22, 0xb1, 0x02, 0x85, 0x89, 0x09, 0x29, 0x95, 0x02,
	0xb1, 0x02, 0x85, 0x90, 0x09, 0x30, 0x95, 0x05, 0xb1, 0x02, 0x85, 0x91, 0x09, 0x31, 0x95, 0x03,
	0xb1, 0x02, 0x85, 0x92, 0x09, 0x32, 0x95, 0x03, 0xb1, 0x02, 0x85, 0x93, 0x09, 0x33, 0x95, 0x0c,
	0xb1, 0x02, 0x85, 0xa0, 0x09, 0x40, 0x95, 0x06, 0xb1, 0x02, 0x85, 0xa1, 0x09, 0x41, 0x95, 0x01,
	0xb1, 0x02, 0x85, 0xa2, 0x09, 0x42, 0x95, 0x01, 0xb1, 0x02, 0x85, 0xa3, 0x09, 0x43, 0x95, 0x30,
	0xb1, 0x02, 0x85, 0xa4, 0x09, 0x44, 0x95, 0x0d, 0xb1, 0x02, 0x85, 0xa5, 0x09, 0x45, 0x95, 0x15,
	0xb1, 0x02, 0x85, 0xa6, 0x09, 0x46, 0x95, 0x15, 0xb1, 0x02, 0x85, 0xf0, 0x09, 0x47, 0x95, 0x3f,
	0xb1, 0x02, 0x85, 0xf1, 0x09, 0x48, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf2, 0x09, 0x49, 0x95, 0x0f,
	0xb1, 0x02, 0x85, 0xa7, 0x09, 0x4a, 0x95, 0x01, 0xb1, 0x02, 0x85, 0xa8, 0x09, 0x4b, 0x95, 0x01,
	0xb1, 0x02, 0x85, 0xa9, 0x09, 0x4c, 0x95, 0x08, 0xb1, 0x02, 0x85, 0xaa, 0x09, 0x4e, 0x95, 0x01,
	0xb1, 0x02, 0x85, 0xab, 0x09, 0x4f, 0x95, 0x39, 0xb1, 0x02, 0x85, 0xac, 0x09, 0x50, 0x95, 0x39,
	0xb1, 0x02, 0x85, 0xad, 0x09, 0x51, 0x95, 0x0b, 0xb1, 0x02, 0x85, 0xae, 0x09, 0x52, 0x95, 0x01,
	0xb1, 0x02, 0x85, 0xaf, 0x09, 0x53, 0x95, 0x02, 0xb1, 0x02, 0x85, 0xb0, 0x09, 0x54, 0x95, 0x3f,
	0xb1, 0x02, 0xc0,
};

static const uint8_t mHidKeyboardConfigDesc[] = {
	TUD_CONFIG_DESCRIPTOR(1, UsbDeviceItfHidTotal, 0, USB_DEVICE_HID_CONFIG_TOTAL_LEN, 0, 50),
	TUD_HID_DESCRIPTOR(UsbDeviceItfHidKeyboard, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(mHidKeyboardReportDesc),
		USB_DEVICE_EP_HID_KEYBOARD, USB_DEVICE_HID_EP_SIZE, 1),
};

static const uint8_t mHidKeyboardMouseConsumerConfigDesc[] = {
	TUD_CONFIG_DESCRIPTOR(1, UsbDeviceItfHidTotal, 0, USB_DEVICE_HID_CONFIG_TOTAL_LEN, 0, 50),
	TUD_HID_DESCRIPTOR(UsbDeviceItfHidKeyboard, 0, HID_ITF_PROTOCOL_NONE, sizeof(mHidKeyboardMouseConsumerReportDesc),
		USB_DEVICE_EP_HID_KEYBOARD, USB_DEVICE_HID_EP_SIZE, 1),
};

static const uint8_t mHidMouseConfigDesc[] = {
	TUD_CONFIG_DESCRIPTOR(1, UsbDeviceItfHidTotal, 0, USB_DEVICE_HID_CONFIG_TOTAL_LEN, 0, 50),
	TUD_HID_DESCRIPTOR(UsbDeviceItfHidKeyboard, 0, HID_ITF_PROTOCOL_MOUSE, sizeof(mHidMouseReportDesc),
		USB_DEVICE_EP_HID_KEYBOARD, USB_DEVICE_HID_EP_SIZE, 1),
};

static const uint8_t mHidGamepadConfigDesc[] = {
	TUD_CONFIG_DESCRIPTOR(1, UsbDeviceItfHidTotal, 0, USB_DEVICE_HID_INOUT_CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_SELF_POWERED, 500),
	9, TUSB_DESC_INTERFACE, UsbDeviceItfHidKeyboard, 0, 2, TUSB_CLASS_HID, 0, HID_ITF_PROTOCOL_NONE, 0,
	9, HID_DESC_TYPE_HID, U16_TO_U8S_LE(0x0111), 0, 1, HID_DESC_TYPE_REPORT, U16_TO_U8S_LE(sizeof(mHidGamepadReportDesc)),
	7, TUSB_DESC_ENDPOINT, USB_DEVICE_EP_HID_GAMEPAD_IN, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(USB_DEVICE_HID_EP_SIZE), 5,
	7, TUSB_DESC_ENDPOINT, USB_DEVICE_EP_HID_GAMEPAD_OUT, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(USB_DEVICE_HID_EP_SIZE), 5,
};

static const uint8_t mXinputConfigDesc[] = {
	0x09, 0x02, 0x99, 0x00, 0x04, 0x01, 0x00, 0xa0, 0xfa,
	0x09, 0x04, 0x00, 0x00, 0x02, 0xff, 0x5d, 0x01, 0x00,
	0x11, 0x21, 0x00, 0x01, 0x01, 0x25, 0x81, 0x14, 0x00,
	0x00, 0x00, 0x00, 0x13, 0x01, 0x08, 0x00, 0x00,
	0x07, 0x05, 0x81, 0x03, 0x20, 0x00, 0x04,
	0x07, 0x05, 0x01, 0x03, 0x20, 0x00, 0x08,
	0x09, 0x04, 0x01, 0x00, 0x04, 0xff, 0x5d, 0x03, 0x00,
	0x1b, 0x21, 0x00, 0x01, 0x01, 0x01, 0x82, 0x40, 0x01,
	0x02, 0x20, 0x16, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x16, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x07, 0x05, 0x82, 0x03, 0x20, 0x00, 0x02,
	0x07, 0x05, 0x02, 0x03, 0x20, 0x00, 0x04,
	0x07, 0x05, 0x83, 0x03, 0x20, 0x00, 0x40,
	0x07, 0x05, 0x03, 0x03, 0x20, 0x00, 0x10,
	0x09, 0x04, 0x02, 0x00, 0x01, 0xff, 0x5d, 0x02, 0x00,
	0x09, 0x21, 0x00, 0x01, 0x01, 0x22, 0x84, 0x07, 0x00,
	0x07, 0x05, 0x84, 0x03, 0x20, 0x00, 0x10,
	0x09, 0x04, 0x03, 0x00, 0x00, 0xff, 0xfd, 0x13, 0x04,
	0x06, 0x41, 0x00, 0x01, 0x01, 0x03,
};

static const uint8_t mMscConfigDesc[] = {
	TUD_CONFIG_DESCRIPTOR(1, UsbDeviceItfMscTotal, 0, USB_DEVICE_MSC_CONFIG_TOTAL_LEN, 0, 50),
	TUD_MSC_DESCRIPTOR(UsbDeviceItfMsc, 0, USB_DEVICE_EP_MSC_OUT, USB_DEVICE_EP_MSC_IN, 64),
};

_Static_assert(sizeof(mHidKeyboardReportDesc) >= 63, "unexpected keyboard report descriptor size");
_Static_assert(sizeof(mHidKeyboardConfigDesc) == USB_DEVICE_HID_CONFIG_TOTAL_LEN, "unexpected keyboard HID configuration descriptor size");
_Static_assert(sizeof(mHidKeyboardMouseConsumerConfigDesc) == USB_DEVICE_HID_CONFIG_TOTAL_LEN, "unexpected composite HID configuration descriptor size");
_Static_assert(sizeof(mHidMouseConfigDesc) == USB_DEVICE_HID_CONFIG_TOTAL_LEN, "unexpected mouse HID configuration descriptor size");
_Static_assert(sizeof(mHidGamepadReportDesc) == 467, "unexpected DualShock 4 report descriptor size");
_Static_assert(sizeof(mHidGamepadConfigDesc) == USB_DEVICE_HID_INOUT_CONFIG_TOTAL_LEN, "unexpected gamepad HID configuration descriptor size");
_Static_assert(sizeof(mXinputConfigDesc) == USB_DEVICE_XINPUT_CONFIG_TOTAL_LEN, "unexpected XInput configuration descriptor size");
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
	if (mInfo.hidReportSet > UsbDeviceHidReportSetGamepad)
		mInfo.hidReportSet = UsbDeviceHidReportSetKeyboard;
	mDeviceDesc.idVendor = mInfo.vid;
	mDeviceDesc.idProduct = mInfo.pid;
}

static void usbDevicePrvApplyDeviceDesc(enum UsbDeviceMode mode)
{
	mDeviceDesc.bcdUSB = 0x0200;
	mDeviceDesc.bDeviceClass = 0;
	mDeviceDesc.bDeviceSubClass = 0;
	mDeviceDesc.bDeviceProtocol = 0;
	mDeviceDesc.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE;
	mDeviceDesc.idVendor = mInfo.vid;
	mDeviceDesc.idProduct = mInfo.pid;
	mDeviceDesc.bcdDevice = 0x0100;
	mDeviceDesc.iManufacturer = 1;
	mDeviceDesc.iProduct = 2;
	mDeviceDesc.iSerialNumber = 0;
	mDeviceDesc.bNumConfigurations = 1;

	if (mode == UsbDeviceModeXinput) {
		mDeviceDesc.bDeviceClass = TUSB_CLASS_VENDOR_SPECIFIC;
		mDeviceDesc.bDeviceSubClass = 0xff;
		mDeviceDesc.bDeviceProtocol = 0xff;
		mDeviceDesc.idVendor = USB_DEVICE_XINPUT_VID;
		mDeviceDesc.idProduct = USB_DEVICE_XINPUT_PID;
		mDeviceDesc.bcdDevice = 0x0114;
		mDeviceDesc.iSerialNumber = 3;
	}
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
	usbDevicePrvApplyDeviceDesc(mode);
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
	if (mMode == UsbDeviceModeXinput)
		return mXinputConfigDesc;
	if (mMode == UsbDeviceModeMsc)
		return mMscConfigDesc;
	switch (mInfo.hidReportSet) {
	case UsbDeviceHidReportSetKeyboardMouseConsumer:
		return mHidKeyboardMouseConsumerConfigDesc;
	case UsbDeviceHidReportSetMouse:
		return mHidMouseConfigDesc;
	case UsbDeviceHidReportSetGamepad:
		return mHidGamepadConfigDesc;
	case UsbDeviceHidReportSetKeyboard:
	default:
		return mHidKeyboardConfigDesc;
	}
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
	(void)instance;
	switch (mInfo.hidReportSet) {
	case UsbDeviceHidReportSetKeyboardMouseConsumer:
		return mHidKeyboardMouseConsumerReportDesc;
	case UsbDeviceHidReportSetMouse:
		return mHidMouseReportDesc;
	case UsbDeviceHidReportSetGamepad:
		return mHidGamepadReportDesc;
	case UsbDeviceHidReportSetKeyboard:
	default:
		return mHidKeyboardReportDesc;
	}
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
	if (mMode == UsbDeviceModeXinput) {
		switch (index) {
		case 1:
			str = "Microsoft Corporation";
			break;
		case 2:
			str = "Controller";
			break;
		case 3:
			str = "08FEC93";
			break;
		case 4:
			str = "Xbox Security Method 3, Version 1.00, (C) 2005 Microsoft Corporation. All rights reserved.";
			break;
		default:
			return NULL;
		}
	}
	else {
		if (index == 1)
			str = mInfo.manufacturer;
		else if (index == 2)
			str = mInfo.product;
		else
			return NULL;
	}

	len = strlen(str);
	if (len > sizeof(mStringDesc) / sizeof(mStringDesc[0]) - 1)
		len = sizeof(mStringDesc) / sizeof(mStringDesc[0]) - 1;
	for (i = 0; i < len; i++)
		mStringDesc[i + 1] = (uint8_t)str[i];
	mStringDesc[0] = (TUSB_DESC_STRING << 8) | (2 * len + 2);
	return mStringDesc;
}
