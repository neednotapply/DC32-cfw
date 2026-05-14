#include <string.h>
#include "2350.h"
#include "timebase.h"
#include "tusb.h"
#include "device/dcd.h"
#include "usbHid.h"

#define USB_HID_HW_WAIT_MS		100
#define USB_HID_DETACH_WAIT_MS		100
#define USB_HID_KEYBOARD_REPORT_SIZE	8

enum {
	UsbHidItfKeyboard,
	UsbHidItfTotal,
};

#define USB_HID_CONFIG_TOTAL_LEN	(TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define USB_HID_EP_KEYBOARD		0x81

static struct UsbHidDeviceInfo mInfo;
static bool mHwReady, mInited, mReportsEnabled;
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
	.idVendor = USB_HID_DEFAULT_VID,
	.idProduct = USB_HID_DEFAULT_PID,
	.bcdDevice = 0x0100,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 0,
	.bNumConfigurations = 1,
};

static const uint8_t mReportDesc[] = {
	TUD_HID_REPORT_DESC_KEYBOARD()
};

static const uint8_t mConfigDesc[] = {
	TUD_CONFIG_DESCRIPTOR(1, UsbHidItfTotal, 0, USB_HID_CONFIG_TOTAL_LEN, 0, 50),
	TUD_HID_DESCRIPTOR(UsbHidItfKeyboard, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(mReportDesc),
		USB_HID_EP_KEYBOARD, USB_HID_KEYBOARD_REPORT_SIZE, 1),
};

_Static_assert(sizeof(mReportDesc) >= 63, "unexpected keyboard report descriptor size");
_Static_assert(sizeof(mConfigDesc) == USB_HID_CONFIG_TOTAL_LEN, "unexpected HID configuration descriptor size");

void usbHidDefaultInfo(struct UsbHidDeviceInfo *info)
{
	memset(info, 0, sizeof(*info));
	info->vid = USB_HID_DEFAULT_VID;
	info->pid = USB_HID_DEFAULT_PID;
	strcpy(info->manufacturer, "DC32");
	strcpy(info->product, "DC32 HID");
}

static bool usbHidPrvWaitBits(const volatile uint32_t *reg, uint32_t bits, bool set)
{
	uint64_t end = getTime() + (uint64_t)USB_HID_HW_WAIT_MS * (TICKS_PER_SECOND / 1000);

	while (getTime() < end) {
		uint32_t val = *reg & bits;

		if (set ? val == bits : val == 0)
			return true;
	}
	return false;
}

static void usbHidPrvWaitMs(uint32_t msec)
{
	uint64_t end = getTime() + (uint64_t)msec * (TICKS_PER_SECOND / 1000);

	while (getTime() < end)
		usbHidTask();
}

static void usbHidPrvPollController(void)
{
	if (!mInited)
		return;
	dcd_int_handler(0);
	tud_task();
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
	mDeviceDesc.idVendor = mInfo.vid;
	mDeviceDesc.idProduct = mInfo.pid;
}

bool usbHidPrepare(void)
{
	uint32_t units;

	mLastError = "none";
	if (mHwReady)
		return true;

	units = RESETS_RESET_PLL_USB_BITS;
	resets_hw->reset |= units;
	resets_hw->reset &=~ units;
	if (!usbHidPrvWaitBits(&resets_hw->reset_done, units, true)) {
		mLastError = "PLL reset timeout";
		return false;
	}

	pll_usb_hw->pwr |= PLL_PWR_VCOPD_BITS | PLL_PWR_POSTDIVPD_BITS | PLL_PWR_PD_BITS;
	pll_usb_hw->cs = (pll_usb_hw->cs &~ (PLL_CS_BYPASS_BITS | PLL_CS_REFDIV_BITS)) | (1 << PLL_CS_REFDIV_LSB);
	pll_usb_hw->fbdiv_int = (pll_usb_hw->fbdiv_int &~ PLL_FBDIV_INT_BITS) | (100 << PLL_FBDIV_INT_LSB);
	pll_usb_hw->prim = (pll_usb_hw->prim &~ (PLL_PRIM_POSTDIV1_BITS | PLL_PRIM_POSTDIV2_BITS)) |
		(5 << PLL_PRIM_POSTDIV1_LSB) | (5 << PLL_PRIM_POSTDIV2_LSB);
	pll_usb_hw->pwr &=~ (PLL_PWR_VCOPD_BITS | PLL_PWR_POSTDIVPD_BITS | PLL_PWR_PD_BITS);
	if (!usbHidPrvWaitBits(&pll_usb_hw->cs, PLL_CS_LOCK_BITS, true)) {
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

const char *usbHidLastError(void)
{
	return mLastError;
}

bool usbHidBegin(const struct UsbHidDeviceInfo *info)
{
	if (mInited) {
		usbHidEnd();
		usbHidPrvWaitMs(USB_HID_DETACH_WAIT_MS);
	}

	usbHidPrvNormalizeInfo(info);
	if (!usbHidPrepare())
		return false;
	if (!tud_init(0)) {
		mLastError = "TinyUSB init failed";
		return false;
	}

	mReportsEnabled = false;
	mInited = true;
	tud_connect();
	usbHidPrvPollController();
	return true;
}

void usbHidTask(void)
{
	usbHidPrvPollController();
}

bool usbHidReady(void)
{
	usbHidTask();
	return mInited && tud_mounted() && tud_hid_ready();
}

bool usbHidStarted(void)
{
	usbHidTask();
	return mInited;
}

bool usbHidReportsEnabled(void)
{
	return mReportsEnabled;
}

static bool usbHidPrvSendReport(uint8_t modifiers, const uint8_t keys[6], bool force)
{
	uint64_t end = getTime() + TICKS_PER_SECOND / 2;

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
	if (!mInited)
		return;
	if (tud_mounted())
		usbHidPrvReleaseAll(true);
	mReportsEnabled = false;
	tud_disconnect();
	usbHidPrvPollController();
	(void)tud_deinit(0);
	mInited = false;
}

uint8_t const *tud_descriptor_device_cb(void)
{
	return (const uint8_t*)&mDeviceDesc;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
	(void)index;
	return mConfigDesc;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
	(void)instance;
	return mReportDesc;
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
