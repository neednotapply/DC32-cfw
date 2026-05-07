#include <stddef.h>
#include <stdint.h>

#define RAMUTILS_RAMCODE __attribute__((section(".fastcode"), noinline))

void *RAMUTILS_RAMCODE memcpy(void *dstP, const void *srcP, size_t n)
{
	uint8_t *dst = (uint8_t*)dstP;
	const uint8_t *src = (const uint8_t*)srcP;

	while (n--)
		*dst++ = *src++;
	return dstP;
}

void *RAMUTILS_RAMCODE memset(void *dstP, int val, size_t n)
{
	uint8_t *dst = (uint8_t*)dstP;

	while (n--)
		*dst++ = (uint8_t)val;
	return dstP;
}
