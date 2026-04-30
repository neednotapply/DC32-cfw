#ifndef _BADGE_LEDS_H_
#define _BADGE_LEDS_H_

#include <stdbool.h>
#include <stdint.h>
#include "settings.h"

const char* badgeLedsModeName(uint_fast8_t mode);
void badgeLedsApplySettings(const struct Settings *settings, bool force);
void badgeLedsTick(void);
void badgeLedsGameWrite(void);

#endif
