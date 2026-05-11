#ifndef _PICO_H
#define _PICO_H

#include <stdint.h>
#include <stddef.h>

typedef  unsigned int uint;
 
#include <assert.h>
#ifndef __cplusplus
	#define static_assert _Static_assert
#endif

#ifndef __not_in_flash_func
	#define __not_in_flash_func(fn) fn
#endif
#ifndef __not_in_flash
	#define __not_in_flash(name)
#endif

#endif
