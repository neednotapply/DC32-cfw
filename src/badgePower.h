#ifndef _BADGE_POWER_H_
#define _BADGE_POWER_H_

#include <stdbool.h>
#include <stdint.h>

enum BadgePowerMode {
	BadgePowerModeNormal,
	BadgePowerModeCharging,
	BadgePowerModeLow,
};

struct BadgePowerStatus {
	bool valid;
	uint32_t battMv;
	uint8_t battPercent;
	uint8_t battLevel;
	uint32_t usbMv;
	bool usbPowered;
	bool lowBatt;
	enum BadgePowerMode mode;
};

bool badgePowerReadNow(struct BadgePowerStatus *status);
void badgePowerPoll(void);
bool badgePowerGetCached(struct BadgePowerStatus *status);

#endif
