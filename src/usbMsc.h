#ifndef _USB_MSC_H_
#define _USB_MSC_H_

#include <stdbool.h>

bool usbMscBegin(bool writable);
void usbMscTask(void);
bool usbMscStarted(void);
bool usbMscMounted(void);
bool usbMscEjected(void);
bool usbMscWritable(void);
const char *usbMscLastError(void);
void usbMscEnd(void);

#endif
