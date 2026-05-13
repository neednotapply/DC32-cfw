#ifndef NES_FRENS_HELPERS_COMPAT_H
#define NES_FRENS_HELPERS_COMPAT_H

#include <stddef.h>

extern "C" void *nesPortAlloc(size_t size);
extern "C" void nesPortFree(void *ptr);

namespace Frens {
static inline void *f_malloc(size_t size) { return nesPortAlloc(size); }
static inline void f_free(void *ptr) { nesPortFree(ptr); }
}

#endif
