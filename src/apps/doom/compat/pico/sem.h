#ifndef DC32_DOOM_COMPAT_PICO_SEM_H
#define DC32_DOOM_COMPAT_PICO_SEM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct semaphore {
	volatile int count;
	int max;
} semaphore_t;

static inline void sem_init(semaphore_t *sem, int initial_permits, int max_permits)
{
	sem->count = initial_permits;
	sem->max = max_permits;
}

static inline int sem_available(semaphore_t *sem)
{
	return sem->count;
}

static inline void sem_release(semaphore_t *sem)
{
	if (sem->count < sem->max)
		sem->count++;
}

static inline void sem_acquire_blocking(semaphore_t *sem)
{
	while (sem->count <= 0) {
	}
	sem->count--;
}

static inline bool sem_acquire_timeout_ms(semaphore_t *sem, uint32_t timeout_ms)
{
	(void)timeout_ms;
	if (sem->count <= 0)
		return false;
	sem->count--;
	return true;
}

#endif
