#ifndef _PICO_H
#define _PICO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef  unsigned int uint;
 
#include <assert.h>
#ifndef __cplusplus
	#define static_assert _Static_assert
#endif

#ifdef FLIPPER_TINYUSB_HOST
#include "inc_RP2350/pico/platform.h"
#else
#ifndef __not_in_flash_func
	#ifdef NES_FASTCODE_ENABLED
		#define __not_in_flash_func(fn) __attribute__((section(".fastcode." #fn))) fn
	#else
		#define __not_in_flash_func(fn) fn
	#endif
#endif
#ifndef __not_in_flash
	#define __not_in_flash(name)
#endif
#ifndef __force_inline
	#define __force_inline __attribute__((always_inline)) inline
#endif
#ifndef __unused
	#define __unused __attribute__((unused))
#endif
#ifndef __no_inline_not_in_flash_func
	#define __no_inline_not_in_flash_func(fn) __attribute__((noinline)) fn
#endif
#ifndef ADDRESS_ALIAS
	#define ADDRESS_ALIAS 0
#endif
#ifndef PARAM_ASSERTIONS_ENABLED_0
	#define PARAM_ASSERTIONS_ENABLED_0 0
#endif
#ifndef valid_params_if
	#define valid_params_if(group, test) ((void)(test))
#endif
#ifndef invalid_params_if
	#define invalid_params_if(group, test) ((void)(test))
#endif
#ifndef remove_volatile_cast
	#define remove_volatile_cast(type, value) ((type)(uintptr_t)(value))
#endif
#endif

void __attribute__((noreturn)) panic(const char *fmt, ...);

#endif
