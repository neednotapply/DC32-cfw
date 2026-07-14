#include "apps/flipper_usb_host.h"

#include "2350.h"
#include "timebase.h"
#include "tusb.h"

#define FLIPPER_USB_HOST_HW_WAIT_MS 100u

static bool mStarted;
static bool mMounted;
static uint8_t mCdcIndex;
static const char *mLastError = "none";

static bool flipperUsbHostWaitBits(const volatile uint32_t *reg, uint32_t bits, bool set)
{
	uint64_t end = getTime() + (uint64_t)FLIPPER_USB_HOST_HW_WAIT_MS * (TICKS_PER_SECOND / 1000u);

	while (getTime() < end) {
		uint32_t value = *reg & bits;

		if (set ? value == bits : value == 0u)
			return true;
	}
	return false;
}

static bool flipperUsbHostPrepareClock(void)
{
	uint32_t units = RESETS_RESET_PLL_USB_BITS;

	resets_hw->reset |= units;
	resets_hw->reset &= ~units;
	if (!flipperUsbHostWaitBits(&resets_hw->reset_done, units, true)) {
		mLastError = "USB PLL reset timeout";
		return false;
	}

	pll_usb_hw->pwr |= PLL_PWR_VCOPD_BITS | PLL_PWR_POSTDIVPD_BITS | PLL_PWR_PD_BITS;
	pll_usb_hw->cs = (pll_usb_hw->cs & ~(PLL_CS_BYPASS_BITS | PLL_CS_REFDIV_BITS)) |
		(1u << PLL_CS_REFDIV_LSB);
	pll_usb_hw->fbdiv_int = (pll_usb_hw->fbdiv_int & ~PLL_FBDIV_INT_BITS) |
		(100u << PLL_FBDIV_INT_LSB);
	pll_usb_hw->prim = (pll_usb_hw->prim & ~(PLL_PRIM_POSTDIV1_BITS | PLL_PRIM_POSTDIV2_BITS)) |
		(5u << PLL_PRIM_POSTDIV1_LSB) | (5u << PLL_PRIM_POSTDIV2_LSB);
	pll_usb_hw->pwr &= ~(PLL_PWR_VCOPD_BITS | PLL_PWR_POSTDIVPD_BITS | PLL_PWR_PD_BITS);
	if (!flipperUsbHostWaitBits(&pll_usb_hw->cs, PLL_CS_LOCK_BITS, true)) {
		mLastError = "USB PLL lock timeout";
		return false;
	}
	pll_usb_hw->cs &= ~PLL_CS_BYPASS_BITS;

	clocks_hw->clk[clk_usb].ctrl &= ~CLOCKS_CLK_USB_CTRL_ENABLE_BITS;
	clocks_hw->clk[clk_usb].div = 1u << CLOCKS_CLK_USB_DIV_INT_LSB;
	clocks_hw->clk[clk_usb].ctrl = CLOCKS_CLK_USB_CTRL_ENABLE_BITS |
		(CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB << CLOCKS_CLK_USB_CTRL_AUXSRC_LSB);
	return true;
}

uint32_t tusb_time_millis_api(void)
{
	return (uint32_t)(getTime() / (TICKS_PER_SECOND / 1000u));
}

bool flipperUsbHostBegin(void)
{
	if (mStarted)
		return true;
	mLastError = "none";
	if (!flipperUsbHostPrepareClock())
		return false;
	if (!tusb_init(0)) {
		mLastError = "TinyUSB host init failed";
		return false;
	}
	/* The upstream RP2040 HCD assumes an externally powered bus.  Ask the
	 * RP2350 USB block to drive its VBUS-enable output for DC32 direct mode. */
	usb_hw->pwr = USB_USB_PWR_VBUS_DETECT_BITS |
		USB_USB_PWR_VBUS_DETECT_OVERRIDE_EN_BITS |
		USB_USB_PWR_VBUS_EN_BITS |
		USB_USB_PWR_VBUS_EN_OVERRIDE_EN_BITS;
	mStarted = true;
	return true;
}

void flipperUsbHostEnd(void)
{
	if (mStarted)
		(void)tusb_deinit(0);
	usb_hw->pwr = 0u;
	mStarted = mMounted = false;
}

void flipperUsbHostTask(void)
{
	if (!mStarted)
		return;
	tusb_int_handler(0, false);
	tuh_task();
}

bool flipperUsbHostMounted(void)
{
	return mStarted && mMounted;
}

uint32_t flipperUsbHostRead(void *data, uint32_t size)
{
	return flipperUsbHostMounted() ? tuh_cdc_read(mCdcIndex, data, size) : 0u;
}

uint32_t flipperUsbHostAvailable(void)
{
	return flipperUsbHostMounted() ? tuh_cdc_read_available(mCdcIndex) : 0u;
}

uint32_t flipperUsbHostWrite(const void *data, uint32_t size)
{
	return flipperUsbHostMounted() ? tuh_cdc_write(mCdcIndex, data, size) : 0u;
}

void flipperUsbHostFlush(void)
{
	if (flipperUsbHostMounted())
		(void)tuh_cdc_write_flush(mCdcIndex);
}

const char *flipperUsbHostLastError(void)
{
	return mLastError;
}

void tuh_cdc_mount_cb(uint8_t index)
{
	mCdcIndex = index;
	mMounted = true;
	(void)tuh_cdc_set_baudrate(index, 230400u, NULL, 0u);
	(void)tuh_cdc_connect(index, NULL, 0u);
}

void tuh_cdc_umount_cb(uint8_t index)
{
	if (index == mCdcIndex)
		mMounted = false;
}

void tuh_cdc_rx_cb(uint8_t index)
{
	(void)index;
}

void tuh_cdc_tx_complete_cb(uint8_t index)
{
	(void)index;
}
