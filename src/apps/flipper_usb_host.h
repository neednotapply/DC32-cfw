#ifndef _FLIPPER_USB_HOST_H_
#define _FLIPPER_USB_HOST_H_

#include <stdbool.h>
#include <stdint.h>

bool flipperUsbHostBegin(void);
void flipperUsbHostEnd(void);
void flipperUsbHostTask(void);
bool flipperUsbHostMounted(void);
uint32_t flipperUsbHostRead(void *data, uint32_t size);
uint32_t flipperUsbHostAvailable(void);
uint32_t flipperUsbHostWrite(const void *data, uint32_t size);
void flipperUsbHostFlush(void);
const char *flipperUsbHostLastError(void);

#endif
