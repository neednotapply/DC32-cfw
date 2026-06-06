#include <stddef.h>

void *_sbrk(ptrdiff_t incr)
{
	(void)incr;
	return (void*)-1;
}
