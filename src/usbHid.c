#include <string.h>
#include "2350.h"
#include "timebase.h"
#include "usbHid.h"

#define USB_REQ_GET_STATUS                      0x00
#define USB_REQ_CLEAR_FEATURE           0x01
#define USB_REQ_SET_ADDRESS                     0x05
#define USB_REQ_GET_DESCRIPTOR          0x06
#define USB_REQ_SET_DESCRIPTOR          0x07
#define USB_REQ_GET_CONFIGURATION       0x08
#define USB_REQ_SET_CONFIGURATION       0x09
#define USB_REQ_GET_INTERFACE           0x0a
#define USB_REQ_SET_INTERFACE           0x0b

#define USB_DT_DEVICE                           0x01
#define USB_DT_CONFIGURATION            0x02
#define USB_DT_STRING                           0x03
#define USB_DT_INTERFACE                        0x04
#define USB_DT_ENDPOINT                         0x05
#define USB_DT_HID                                      0x21
#define USB_DT_REPORT                           0x22

#define HID_REQ_GET_REPORT                      0x01
#define HID_REQ_GET_IDLE                        0x02
#define HID_REQ_GET_PROTOCOL            0x03
#define HID_REQ_SET_IDLE                        0x0a
#define HID_REQ_SET_PROTOCOL            0x0b

#define USB_EP1_IN_BUF                          0x180
#define USB_EP1_IN_BIT                          (1u << 2)
#define USB_EP0_IN_BIT                          (1u << 0)
#define USB_EP0_OUT_BIT                         (1u << 1)

#define USB_MAIN_CTRL_CONTROLLER_EN             (1u << 0)
#define USB_SIE_CTRL_EP0_INT_1BUF               (1u << 29)
#define USB_SIE_CTRL_PULLUP_EN                  (1u << 16)
#define USB_SIE_STATUS_BUS_RESET                (1u << 19)
#define USB_SIE_STATUS_SETUP_REC                (1u << 17)
#define USB_USB_MUXING_TO_PHY                   (1u << 0)
#define USB_USB_MUXING_SOFTCON                  (1u << 3)
#define USB_USB_PWR_VBUS_DETECT                 (1u << 2)
#define USB_USB_PWR_VBUS_DETECT_OVERRIDE_EN     (1u << 3)
#define USB_HID_HW_WAIT_MS                              100

struct UsbSetup {
        uint8_t bmRequestType;
        uint8_t bRequest;
        uint16_t wValue;
        uint16_t wIndex;
        uint16_t wLength;
} __attribute__((packed));

static struct UsbHidDeviceInfo mInfo;
static uint8_t mConfigured, mPendingAddress, mIdleRate, mProtocol = 1;
static const uint8_t *mCtrlData;
static uint16_t mCtrlRemaining;
static bool mInited, mEp0DataPid, mEp1DataPid;

static const uint8_t mReportDesc[] = {
        0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x85, 0x01, 0x05, 0x07,
        0x19, 0xe0, 0x29, 0xe7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01,
        0x95, 0x08, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x81, 0x01,
        0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0x65, 0x05, 0x07,
        0x19, 0x00, 0x29, 0x65, 0x81, 0x00, 0xc0,

        0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x85, 0x02, 0x09, 0x01,
        0xa1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00,
        0x25, 0x01, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02, 0x95, 0x01,
        0x75, 0x05, 0x81, 0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31,
        0x09, 0x38, 0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x03,
        0x81, 0x06, 0xc0, 0xc0,

        0x05, 0x0c, 0x09, 0x01, 0xa1, 0x01, 0x85, 0x03, 0x15, 0x00,
        0x26, 0xff, 0x03, 0x19, 0x00, 0x2a, 0xff, 0x03, 0x75, 0x10,
        0x95, 0x01, 0x81, 0x00, 0xc0,
};

void usbHidDefaultInfo(struct UsbHidDeviceInfo *info)
{
        memset(info, 0, sizeof(*info));
        info->vid = USB_HID_DEFAULT_VID;
        info->pid = USB_HID_DEFAULT_PID;
        strcpy(info->manufacturer, "DC32");
        strcpy(info->product, "DC32 BadUSB");
}

static void usbHidPrvWrite16(uint8_t *dst, uint16_t val)
{
        dst[0] = val;
        dst[1] = val >> 8;
}

static const uint8_t* usbHidPrvDeviceDesc(uint16_t *lenP)
{
        static uint8_t desc[18] = {
                18, USB_DT_DEVICE, 0x00, 0x02, 0, 0, 0, 64,
                0, 0, 0, 0, 0x00, 0x01, 1, 2, 0, 1,
        };

        usbHidPrvWrite16(desc + 8, mInfo.vid);
        usbHidPrvWrite16(desc + 10, mInfo.pid);
        *lenP = sizeof(desc);
        return desc;
}

static const uint8_t* usbHidPrvConfigDesc(uint16_t *lenP)
{
        static uint8_t desc[34] = {
                9, USB_DT_CONFIGURATION, 34, 0, 1, 1, 0, 0x80, 50,
                9, USB_DT_INTERFACE, 0, 0, 1, 0x03, 0, 0, 0,
                9, USB_DT_HID, 0x11, 0x01, 0, 1, USB_DT_REPORT, 0, 0,
                7, USB_DT_ENDPOINT, 0x81, 0x03, 16, 0, 1,
        };

        usbHidPrvWrite16(desc + 25, sizeof(mReportDesc));
        *lenP = sizeof(desc);
        return desc;
}

static const uint8_t* usbHidPrvHidDesc(uint16_t *lenP)
{
        static uint8_t desc[9] = {9, USB_DT_HID, 0x11, 0x01, 0, 1, USB_DT_REPORT, 0, 0};

        usbHidPrvWrite16(desc + 7, sizeof(mReportDesc));
        *lenP = sizeof(desc);
        return desc;
}

static uint16_t usbHidPrvStringDesc(uint8_t index, uint8_t *dst)
{
        const char *str = NULL;
        uint16_t len = 2, i;

        if (!index) {
                dst[0] = 4;
                dst[1] = USB_DT_STRING;
                dst[2] = 0x09;
                dst[3] = 0x04;
                return 4;
        }
        if (index == 1)
                str = mInfo.manufacturer;
        else if (index == 2)
                str = mInfo.product;
        else
                str = "";

        for (i = 0; str[i] && len + 2 <= 64; i++) {
                dst[len++] = str[i];
                dst[len++] = 0;
        }
        dst[0] = len;
        dst[1] = USB_DT_STRING;
        return len;
}

static uint32_t usbHidPrvBufCtrl(uint32_t len, bool data1)
{
        return USB_BUF_CTRL_AVAIL | USB_BUF_CTRL_FULL | USB_BUF_CTRL_LAST | len | (data1 ? USB_BUF_CTRL_DATA1_PID : USB_BUF_CTRL_DATA0_PID);
}

static void usbHidPrvEp0In(const void *data, uint16_t len)
{
        if (len > 64)
                len = 64;
        if (len)
                memcpy(usb_dpram->ep0_buf_a, data, len);
        usb_dpram->ep_buf_ctrl[0].in = usbHidPrvBufCtrl(len, mEp0DataPid);
        mEp0DataPid = !mEp0DataPid;
}

static void usbHidPrvEp0OutStatus(void)
{
        usb_dpram->ep_buf_ctrl[0].out = USB_BUF_CTRL_AVAIL | USB_BUF_CTRL_DATA1_PID;
}

static void usbHidPrvCtrlSendNext(void)
{
        uint16_t len = mCtrlRemaining;

        if (len > 64)
                len = 64;
        usbHidPrvEp0In(mCtrlData, len);
        mCtrlData += len;
        mCtrlRemaining -= len;
}

static void usbHidPrvEp0Stall(void)
{
        usb_hw->ep_stall_arm = USB_EP0_IN_BIT | USB_EP0_OUT_BIT;
        usb_dpram->ep_buf_ctrl[0].in = USB_BUF_CTRL_STALL;
        usb_dpram->ep_buf_ctrl[0].out = USB_BUF_CTRL_STALL;
}

static void usbHidPrvEp1Init(void)
{
        usb_dpram->ep_ctrl[0].in = EP_CTRL_ENABLE_BITS | EP_CTRL_INTERRUPT_PER_BUFFER | (3u << EP_CTRL_BUFFER_TYPE_LSB) | USB_EP1_IN_BUF;
        usb_dpram->ep_buf_ctrl[1].in = 0;
        mEp1DataPid = false;
}

static void usbHidPrvBusReset(void)
{
        usb_hw->dev_addr_ctrl = 0;
        mConfigured = 0;
        mPendingAddress = 0;
        mCtrlData = NULL;
        mCtrlRemaining = 0;
        usb_dpram->ep_buf_ctrl[0].in = 0;
        usb_dpram->ep_buf_ctrl[0].out = 0;
        usbHidPrvEp1Init();
}

static void usbHidPrvControlRead(const void *data, uint16_t len, uint16_t wanted)
{
        if (len > wanted)
                len = wanted;
        mCtrlData = data;
        mCtrlRemaining = len;
        usbHidPrvCtrlSendNext();
}

static void usbHidPrvSetup(void)
{
        struct UsbSetup setup;
        uint8_t tmp[64];
        const uint8_t *data = NULL;
        uint16_t len = 0;

        memcpy(&setup, (const void*)usb_dpram->setup_packet, sizeof(setup));
        mEp0DataPid = true;
        mCtrlData = NULL;
        mCtrlRemaining = 0;

        if ((setup.bmRequestType & 0x60) == 0) {
                if (setup.bRequest == USB_REQ_GET_DESCRIPTOR) {
                        uint8_t descType = setup.wValue >> 8, descIdx = setup.wValue;

                        if (descType == USB_DT_DEVICE)
                                data = usbHidPrvDeviceDesc(&len);
                        else if (descType == USB_DT_CONFIGURATION)
                                data = usbHidPrvConfigDesc(&len);
                        else if (descType == USB_DT_STRING) {
                                len = usbHidPrvStringDesc(descIdx, tmp);
                                data = tmp;
                        }
                        else if (descType == USB_DT_REPORT) {
                                data = mReportDesc;
                                len = sizeof(mReportDesc);
                        }
                        else if (descType == USB_DT_HID)
                                data = usbHidPrvHidDesc(&len);

                        if (data)
                                usbHidPrvControlRead(data, len, setup.wLength);
                        else
                                usbHidPrvEp0Stall();
                }
                else if (setup.bRequest == USB_REQ_SET_ADDRESS) {
                        mPendingAddress = setup.wValue & 0x7f;
                        usbHidPrvEp0In(NULL, 0);
                }
                else if (setup.bRequest == USB_REQ_SET_CONFIGURATION) {
                        mConfigured = setup.wValue;
                        usbHidPrvEp1Init();
                        usbHidPrvEp0In(NULL, 0);
                }
                else if (setup.bRequest == USB_REQ_GET_CONFIGURATION) {
                        tmp[0] = mConfigured;
                        usbHidPrvControlRead(tmp, 1, setup.wLength);
                }
                else if (setup.bRequest == USB_REQ_GET_STATUS) {
                        tmp[0] = tmp[1] = 0;
                        usbHidPrvControlRead(tmp, 2, setup.wLength);
                }
                else if (setup.bRequest == USB_REQ_CLEAR_FEATURE || setup.bRequest == USB_REQ_SET_DESCRIPTOR || setup.bRequest == USB_REQ_SET_INTERFACE) {
                        usbHidPrvEp0In(NULL, 0);
                }
                else if (setup.bRequest == USB_REQ_GET_INTERFACE) {
                        tmp[0] = 0;
                        usbHidPrvControlRead(tmp, 1, setup.wLength);
                }
                else {
                        usbHidPrvEp0Stall();
                }
        }
        else if ((setup.bmRequestType & 0x60) == 0x20) {
                if (setup.bRequest == HID_REQ_SET_IDLE) {
                        mIdleRate = setup.wValue >> 8;
                        usbHidPrvEp0In(NULL, 0);
                }
                else if (setup.bRequest == HID_REQ_GET_IDLE) {
                        tmp[0] = mIdleRate;
                        usbHidPrvControlRead(tmp, 1, setup.wLength);
                }
                else if (setup.bRequest == HID_REQ_SET_PROTOCOL) {
                        mProtocol = setup.wValue;
                        usbHidPrvEp0In(NULL, 0);
                }
                else if (setup.bRequest == HID_REQ_GET_PROTOCOL) {
                        tmp[0] = mProtocol;
                        usbHidPrvControlRead(tmp, 1, setup.wLength);
                }
                else if (setup.bRequest == HID_REQ_GET_REPORT) {
                        memset(tmp, 0, sizeof(tmp));
                        usbHidPrvControlRead(tmp, setup.wLength > sizeof(tmp) ? sizeof(tmp) : setup.wLength, setup.wLength);
                }
                else {
                        usbHidPrvEp0Stall();
                }
        }
        else {
                usbHidPrvEp0Stall();
        }
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

bool usbHidBegin(const struct UsbHidDeviceInfo *info)
{
        uint32_t units;

        mInfo = *info;
        if (!mInfo.vid)
                mInfo.vid = USB_HID_DEFAULT_VID;
        if (!mInfo.pid)
                mInfo.pid = USB_HID_DEFAULT_PID;
        if (!mInfo.manufacturer[0])
                strcpy(mInfo.manufacturer, "DC32");
        if (!mInfo.product[0])
                strcpy(mInfo.product, "DC32 BadUSB");

        units = RESETS_RESET_USBCTRL_BITS | RESETS_RESET_PLL_USB_BITS;
        resets_hw->reset |= units;
        resets_hw->reset &=~ units;
        if (!usbHidPrvWaitBits(&resets_hw->reset_done, units, true))
                return false;

        pll_usb_hw->pwr |= PLL_PWR_VCOPD_BITS | PLL_PWR_POSTDIVPD_BITS | PLL_PWR_PD_BITS;
        pll_usb_hw->fbdiv_int = (pll_usb_hw->fbdiv_int &~ PLL_FBDIV_INT_BITS) | (100 << PLL_FBDIV_INT_LSB);
        pll_usb_hw->prim = (pll_usb_hw->prim &~ (PLL_PRIM_POSTDIV1_BITS | PLL_PRIM_POSTDIV2_BITS)) | (5 << PLL_PRIM_POSTDIV1_LSB) | (5 << PLL_PRIM_POSTDIV2_LSB);
        pll_usb_hw->pwr &=~ (PLL_PWR_VCOPD_BITS | PLL_PWR_POSTDIVPD_BITS | PLL_PWR_PD_BITS);
        if (!usbHidPrvWaitBits(&pll_usb_hw->cs, PLL_CS_LOCK_BITS, true))
                return false;
        pll_usb_hw->cs &=~ PLL_CS_BYPASS_BITS;

        clocks_hw->clk[clk_usb].ctrl &=~ CLOCKS_CLK_USB_CTRL_ENABLE_BITS;
        clocks_hw->clk[clk_usb].div = 1 << CLOCKS_CLK_USB_DIV_INT_LSB;
        clocks_hw->clk[clk_usb].ctrl = CLOCKS_CLK_USB_CTRL_ENABLE_BITS | (CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB << CLOCKS_CLK_USB_CTRL_AUXSRC_LSB);

        memset((void*)usb_dpram, 0, USB_DPRAM_SIZE);
        usb_hw->main_ctrl = 0;
        usb_hw->muxing = USB_USB_MUXING_TO_PHY | USB_USB_MUXING_SOFTCON;
        usb_hw->pwr = USB_USB_PWR_VBUS_DETECT | USB_USB_PWR_VBUS_DETECT_OVERRIDE_EN;
        usb_hw->sie_ctrl = USB_SIE_CTRL_EP0_INT_1BUF | USB_SIE_CTRL_PULLUP_EN;
        usb_hw->main_ctrl = USB_MAIN_CTRL_CONTROLLER_EN;
        usbHidPrvBusReset();
        mInited = true;
        return true;
}

void usbHidTask(void)
{
        uint32_t status, bufStatus;

        if (!mInited)
                return;

        status = usb_hw->sie_status;
        if (status & USB_SIE_STATUS_BUS_RESET) {
                usb_hw->sie_status = USB_SIE_STATUS_BUS_RESET;
                usbHidPrvBusReset();
        }
        if (status & USB_SIE_STATUS_SETUP_REC) {
                usb_hw->sie_status = USB_SIE_STATUS_SETUP_REC;
                usbHidPrvSetup();
        }

        bufStatus = usb_hw->buf_status;
        if (bufStatus) {
                usb_hw->buf_status = bufStatus;
                if (bufStatus & USB_EP0_IN_BIT) {
                        if (mCtrlRemaining)
                                usbHidPrvCtrlSendNext();
                        else if (mPendingAddress) {
                                usb_hw->dev_addr_ctrl = mPendingAddress;
                                mPendingAddress = 0;
                        }
                        else {
                                usbHidPrvEp0OutStatus();
                        }
                }
        }
}

bool usbHidReady(void)
{
        usbHidTask();
        return mInited && mConfigured;
}

static bool usbHidPrvSendReport(const uint8_t *report, uint32_t len)
{
        uint8_t *dst = &usb_dpram->epx_data[USB_EP1_IN_BUF - 0x180];
        uint64_t end = getTime() + TICKS_PER_SECOND / 2;

        if (len > 16)
                return false;
        while (getTime() < end) {
                usbHidTask();
                if (!usbHidReady())
                        continue;
                if (!(usb_dpram->ep_buf_ctrl[1].in & USB_BUF_CTRL_FULL)) {
                        memcpy(dst, report, len);
                        usb_dpram->ep_buf_ctrl[1].in = usbHidPrvBufCtrl(len, mEp1DataPid);
                        mEp1DataPid = !mEp1DataPid;
                        return true;
                }
        }
        return false;
}

bool usbHidKeyboardReport(uint8_t modifiers, const uint8_t keys[6])
{
        uint8_t report[9] = {1, modifiers, 0, keys[0], keys[1], keys[2], keys[3], keys[4], keys[5]};

        return usbHidPrvSendReport(report, sizeof(report));
}

bool usbHidMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t wheel)
{
        uint8_t report[5] = {2, buttons, (uint8_t)x, (uint8_t)y, (uint8_t)wheel};

        return usbHidPrvSendReport(report, sizeof(report));
}

bool usbHidConsumerReport(uint16_t usage)
{
        uint8_t report[3] = {3, usage, usage >> 8};

        if (!usbHidPrvSendReport(report, sizeof(report)))
                return false;
        report[1] = report[2] = 0;
        return usbHidPrvSendReport(report, sizeof(report));
}

void usbHidReleaseAll(void)
{
        uint8_t keys[6] = {0};

        (void)usbHidKeyboardReport(0, keys);
        (void)usbHidMouseReport(0, 0, 0, 0);
        (void)usbHidConsumerReport(0);
}

void usbHidEnd(void)
{
        if (!mInited)
                return;
        if (mConfigured)
                usbHidReleaseAll();
        usb_hw->sie_ctrl &=~ USB_SIE_CTRL_PULLUP_EN;
        usb_hw->main_ctrl = 0;
        mConfigured = 0;
        mInited = false;
}
