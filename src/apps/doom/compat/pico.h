#ifndef DC32_DOOM_COMPAT_PICO_H
#define DC32_DOOM_COMPAT_PICO_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef unsigned int uint;

#ifndef __cplusplus
#define static_assert _Static_assert
#endif

#ifndef __aligned
#define __aligned(x) __attribute__((aligned(x)))
#endif
#ifndef __packed
#define __packed __attribute__((packed))
#endif
#ifndef __unused
#define __unused __attribute__((unused))
#endif
#define __force_inline __attribute__((always_inline)) inline
#define __not_in_flash_func(fn) fn
#define __not_in_flash(name)
#define __no_inline_not_in_flash_func(fn) __attribute__((noinline)) fn
#define __scratch_x(name)
#define __scratch_y(name)
#define __time_critical_func(fn) fn
#define __compiler_memory_barrier() __asm volatile ("" ::: "memory")
#define tight_loop_contents() ((void)0)
#define get_core_num() 0u
#define hard_assert assert

#ifndef count_of
#define count_of(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

static __force_inline int32_t __mul_instruction(int32_t a, int32_t b)
{
	return a * b;
}

typedef struct spin_lock {
	uint32_t unused;
} spin_lock_t;

static inline spin_lock_t *spin_lock_instance(uint num)
{
	static spin_lock_t locks[16];

	return &locks[num & 15u];
}

static inline uint32_t spin_lock_blocking(spin_lock_t *lock)
{
	(void)lock;
	return 0;
}

static inline void spin_unlock(spin_lock_t *lock, uint32_t save)
{
	(void)lock;
	(void)save;
}

#define PICO_SCANVIDEO_PIXEL_FROM_RGB8(r, g, b) \
	((uint16_t)((((uint32_t)(r) & 0xf8u) << 8) | (((uint32_t)(g) & 0xfcu) << 3) | ((uint32_t)(b) >> 3)))

#define CU_REGISTER_DEBUG_PINS(...)
#define CU_SELECT_DEBUG_PINS(...)
#define DEBUG_PINS_SET(name, bits) ((void)0)
#define DEBUG_PINS_CLR(name, bits) ((void)0)
#define DEBUG_PINS_XOR(name, bits) ((void)0)

#ifdef __cplusplus
extern "C" {
#endif

void __attribute__((noreturn)) panic(const char *fmt, ...);
void __attribute__((noreturn)) panic_unsupported(void);

#ifdef __cplusplus
}
#endif

#endif
