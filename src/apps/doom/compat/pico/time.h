#ifndef DC32_DOOM_COMPAT_PICO_TIME_H
#define DC32_DOOM_COMPAT_PICO_TIME_H

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t absolute_time_t;

uint64_t time_us_64(void);
void sleep_ms(uint32_t ms);

static inline uint32_t time_us_32(void)
{
	return (uint32_t)time_us_64();
}

static inline absolute_time_t get_absolute_time(void)
{
	return time_us_64();
}

static inline absolute_time_t make_timeout_time_ms(uint32_t ms)
{
	return time_us_64() + (uint64_t)ms * 1000ull;
}

static inline bool time_reached(absolute_time_t target)
{
	return time_us_64() >= target;
}

static inline uint32_t to_ms_since_boot(absolute_time_t t)
{
	return (uint32_t)(t / 1000ull);
}

#endif
