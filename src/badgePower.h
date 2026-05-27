#ifndef _BADGE_POWER_H_
#define _BADGE_POWER_H_

#include <stdbool.h>
#include <stdint.h>

struct BadgePowerStatus {
	bool valid;
	uint32_t battMv;
	uint32_t usbMv;
	bool lowBatt;
};

bool badgePowerReadNow(struct BadgePowerStatus *status);
void badgePowerPoll(void);
bool badgePowerGetCached(struct BadgePowerStatus *status);

#endif
