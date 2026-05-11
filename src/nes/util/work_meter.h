#ifndef NES_WORK_METER_COMPAT_H
#define NES_WORK_METER_COMPAT_H

#include <stdint.h>

namespace util {
static inline void WorkMeterMark(uint32_t tag) { (void)tag; }
static inline void WorkMeterReset(void) {}
}

#endif
