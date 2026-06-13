#ifndef DC32_DOOM_COMPAT_HARDWARE_INTERP_H
#define DC32_DOOM_COMPAT_HARDWARE_INTERP_H

#include <stdbool.h>
#include <stdint.h>

typedef struct interp_hw {
	uint32_t accum[2];
	uint32_t base[3];
	uint32_t pop[3];
	uint32_t add_raw[2];
} interp_hw_t;

typedef struct interp_config {
	uint32_t value;
} interp_config;

static interp_hw_t doomDc32Interp0;
static interp_hw_t doomDc32Interp1;

#define interp0 (&doomDc32Interp0)
#define interp1 (&doomDc32Interp1)
#define interp0_hw (&doomDc32Interp0)
#define interp1_hw (&doomDc32Interp1)

static inline interp_config interp_default_config(void)
{
	interp_config config = {0};

	return config;
}

static inline void interp_config_set_add_raw(interp_config *config, bool add_raw)
{
	(void)config;
	(void)add_raw;
}

static inline void interp_config_set_shift(interp_config *config, uint32_t shift)
{
	(void)config;
	(void)shift;
}

static inline void interp_config_set_mask(interp_config *config, uint32_t lsb, uint32_t msb)
{
	(void)config;
	(void)lsb;
	(void)msb;
}

static inline void interp_config_set_cross_input(interp_config *config, bool cross)
{
	(void)config;
	(void)cross;
}

static inline void interp_set_config(interp_hw_t *interp, uint32_t lane, const interp_config *config)
{
	(void)interp;
	(void)lane;
	(void)config;
}

#endif
