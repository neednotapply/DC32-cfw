#include "badgePower.h"
#include "pioI2C.h"
#include "timebase.h"
#include <stddef.h>

#define BADGE_POWER_ACCEL_I2C_ADDR		0x18
#define BADGE_POWER_ADC_REG				0x88
#define BADGE_POWER_LOW_MV				3600u
#define BADGE_POWER_CLEAR_MV			3700u
#define BADGE_POWER_USB_PRESENT_MV		4800u
#define BADGE_POWER_USB_CLEAR_MV		4600u
#define BADGE_POWER_USB_POLL_TICKS		((uint64_t)TICKS_PER_SECOND)
#define BADGE_POWER_BATT_UPDATE_TICKS	((uint64_t)TICKS_PER_SECOND * 15u)
#define BADGE_POWER_BATT_SCALE			(65536u * 10u / 3u)
#define BADGE_POWER_USB_SCALE			(65536u * 5u)

static struct BadgePowerStatus mCachedStatus;
static uint64_t mNextPollTime;
static uint64_t mNextBatteryUpdateTime;
static bool mPollInFlight;
static uint8_t mPollAdcVals[4];
static const uint8_t mPollAdcReg = BADGE_POWER_ADC_REG;

struct BadgePowerCurvePoint {
	uint16_t mv;
	uint8_t percent;
};

static uint8_t badgePowerPrvPercent(uint32_t battMv)
{
	static const struct BadgePowerCurvePoint curve[] = {
		{3300, 0}, {3500, 3}, {3600, 7}, {3690, 10}, {3730, 20},
		{3770, 30}, {3790, 40}, {3830, 50}, {3870, 60}, {3920, 70},
		{3980, 80}, {4060, 90}, {4200, 100},
	};

	if (battMv <= curve[0].mv)
		return curve[0].percent;
	for (uint_fast8_t i = 1; i < sizeof(curve) / sizeof(*curve); i++) {
		if (battMv <= curve[i].mv) {
			uint32_t mvSpan = curve[i].mv - curve[i - 1u].mv;
			uint32_t percentSpan = curve[i].percent - curve[i - 1u].percent;

			return (uint8_t)(curve[i - 1u].percent +
				((battMv - curve[i - 1u].mv) * percentSpan + mvSpan / 2u) / mvSpan);
		}
	}
	return 100;
}

static uint32_t badgePowerPrvAdcVal2mv(const uint8_t *val, uint32_t scaleValue)
{
	uint32_t v = 256u * val[0] + val[1];		//unsigned 10-bit value referenced to 800mV. Range of the 10 bits is 800-1600mV

	//ignore all previous datashits. ACTUAL ADC behaviour is as follows:
	//0.92V -> 0x7f00
	//xV -> 104945.3257 - 78874.41376 * x
	//1.3V -> ??? ADC starts reporting negatives, but the forumla holds...
	//ceiling where this stops not tested

	//thus the map from ADC reported 16-bit value to input voltage is
	// V = (104945.3257 - adcVal) / 78874.41376

	v = 104898u - v;
	v = v * 831u / 65536u;
	v = v * scaleValue / 65536u;

	return v;
}

static bool badgePowerPrvLowBatt(uint32_t battMv)
{
	if (mCachedStatus.valid && mCachedStatus.lowBatt)
		return battMv < BADGE_POWER_CLEAR_MV;

	return battMv <= BADGE_POWER_LOW_MV;
}

static bool badgePowerPrvUsbPowered(uint32_t usbMv)
{
	if (mCachedStatus.valid && mCachedStatus.usbPowered)
		return usbMv >= BADGE_POWER_USB_CLEAR_MV;

	return usbMv >= BADGE_POWER_USB_PRESENT_MV;
}

static uint8_t badgePowerPrvLevel(uint8_t percent)
{
	if (percent < 10u)
		return 0;
	if (percent < 30u)
		return 1;
	if (percent < 50u)
		return 2;
	if (percent < 70u)
		return 3;
	if (percent < 90u)
		return 4;
	return 5;
}

static uint8_t badgePowerPrvStableLevel(uint8_t percent)
{
	uint8_t level = badgePowerPrvLevel(percent);

	if (!mCachedStatus.valid)
		return level;
	if (level > mCachedStatus.battLevel) {
		uint8_t boundary = mCachedStatus.battLevel * 20u + 10u;

		return percent >= boundary + 3u ? level : mCachedStatus.battLevel;
	}
	if (level < mCachedStatus.battLevel) {
		uint8_t boundary = mCachedStatus.battLevel * 20u - 10u;

		return percent <= boundary - 3u ? level : mCachedStatus.battLevel;
	}

	return level;
}

static void badgePowerPrvUpdate(const uint8_t adcVals[4], bool forceBatteryUpdate)
{
	struct BadgePowerStatus status = mCachedStatus;
	uint32_t rawBattMv = badgePowerPrvAdcVal2mv(adcVals + 0, BADGE_POWER_BATT_SCALE);
	bool updateBattery = forceBatteryUpdate || !mCachedStatus.valid || getTime() >= mNextBatteryUpdateTime;

	if (updateBattery) {
		uint32_t battMv = rawBattMv;

		if (mCachedStatus.valid)
			battMv = (mCachedStatus.battMv * 7u + battMv + 4u) / 8u;
		status.battMv = battMv;
		status.battPercent = badgePowerPrvPercent(battMv);
		status.battLevel = badgePowerPrvStableLevel(status.battPercent);
		mNextBatteryUpdateTime = getTime() + BADGE_POWER_BATT_UPDATE_TICKS;
	}

	status.valid = true;
	status.usbMv = badgePowerPrvAdcVal2mv(adcVals + 2, BADGE_POWER_USB_SCALE);
	status.usbPowered = badgePowerPrvUsbPowered(status.usbMv);
	status.lowBatt = badgePowerPrvLowBatt(rawBattMv);
	status.mode = status.usbPowered ? BadgePowerModeCharging :
		(status.lowBatt ? BadgePowerModeLow : BadgePowerModeNormal);

	mCachedStatus = status;
}

bool badgePowerReadNow(struct BadgePowerStatus *status)
{
	uint8_t adcVals[4];

	if (!status)
		return false;

	if (!i2cRegRead(BADGE_POWER_ACCEL_I2C_ADDR, BADGE_POWER_ADC_REG, adcVals, sizeof(adcVals))) {
		*status = (struct BadgePowerStatus){0};
		return false;
	}

	badgePowerPrvUpdate(adcVals, true);
	*status = mCachedStatus;

	return true;
}

static void badgePowerPrvPollCbk(void *userData, const struct I2Creq *req, bool likelySuccess)
{
	(void)userData;
	(void)req;

	if (likelySuccess)
		badgePowerPrvUpdate(mPollAdcVals, false);
	mPollInFlight = false;
}

void badgePowerPoll(void)
{
	static const struct I2Creq req = {
		.haveNext = false,
		.addr = BADGE_POWER_ACCEL_I2C_ADDR,
		.txData = &mPollAdcReg,
		.txAcks = NULL,
		.txLen = sizeof(mPollAdcReg),
		.rxAddrAck = NULL,
		.rxData = mPollAdcVals,
		.rxAcks = NULL,
		.rxLen = sizeof(mPollAdcVals),
	};
	uint64_t now = getTime();

	if (mPollInFlight || now < mNextPollTime || i2cPrvIsBuysy())
		return;

	mPollInFlight = true;
	mNextPollTime = now + BADGE_POWER_USB_POLL_TICKS;
	if (!i2cTransact(&req, badgePowerPrvPollCbk, NULL))
		mPollInFlight = false;
}

bool badgePowerGetCached(struct BadgePowerStatus *status)
{
	if (status)
		*status = mCachedStatus;

	return mCachedStatus.valid;
}
