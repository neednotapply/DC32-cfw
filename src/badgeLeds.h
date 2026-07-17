#ifndef _BADGE_LEDS_H_
#define _BADGE_LEDS_H_

#include <stdbool.h>
#include <stdint.h>
#include "settings.h"

const char* badgeLedsModeName(uint_fast8_t mode);
const char* badgeLedsColorName(uint_fast8_t color);
void badgeLedsApplySettings(const struct Settings *settings, bool force);
void badgeLedsSetIdle(bool idle);
void badgeLedsOverrideRgb(uint_fast8_t red, uint_fast8_t green, uint_fast8_t blue);
void badgeLedsTick(void);
void badgeLedsWatchdogTick(void);
void badgeLedsGameWrite(void);
bool badgeLedsGameRefresh(void);

#endif
