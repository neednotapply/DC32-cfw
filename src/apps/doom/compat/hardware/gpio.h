#ifndef DC32_DOOM_COMPAT_HARDWARE_GPIO_H
#define DC32_DOOM_COMPAT_HARDWARE_GPIO_H

#include <stdbool.h>
#include <stdint.h>
#include "pico.h"

#define GPIO_OUT 1

static inline void gpio_init(uint pin) { (void)pin; }
static inline void gpio_set_dir(uint pin, bool out) { (void)pin; (void)out; }
static inline void gpio_put(uint pin, bool value) { (void)pin; (void)value; }
static inline bool gpio_get(uint pin) { (void)pin; return true; }

#endif
