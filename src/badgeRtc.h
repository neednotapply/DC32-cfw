#ifndef _BADGE_RTC_H_
#define _BADGE_RTC_H_

#include <stdbool.h>
#include <stdint.h>

#define BADGE_RTC_MIN_YEAR 2000u
#define BADGE_RTC_MAX_YEAR 2099u

struct BadgeRtcDateTime {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

bool badgeRtcIsValid(void);
uint32_t badgeRtcGet(void);
bool badgeRtcGetTimeOfDay(uint_fast8_t *hourP, uint_fast8_t *minuteP, uint_fast8_t *secondP);
bool badgeRtcGetDateTime(struct BadgeRtcDateTime *timeP);
bool badgeRtcReadHardware(struct BadgeRtcDateTime *timeP);
bool badgeRtcSetDateTime(const struct BadgeRtcDateTime *timeP);
uint_fast8_t badgeRtcDaysInMonth(uint_fast16_t year, uint_fast8_t month);

#endif
