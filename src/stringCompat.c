#include <stdint.h>

int strcmp(const char *a, const char *b)
{
	const uint8_t *aa = (const uint8_t*)a;
	const uint8_t *bb = (const uint8_t*)b;

	while (*aa == *bb) {
		if (!*aa)
			return 0;
		aa++;
		bb++;
	}
	return (int)*aa - (int)*bb;
}
