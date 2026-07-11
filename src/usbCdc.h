#ifndef _USB_CDC_H_
#define _USB_CDC_H_

#include <stdbool.h>
#include <stdint.h>

#define USB_CDC_DEFAULT_VID 0x1209
#define USB_CDC_DEFAULT_PID 0xdc35

struct UsbCdcDeviceInfo {
	uint16_t vid;
	uint16_t pid;
	char manufacturer[32];
	char product[32];
};

void usbCdcDefaultInfo(struct UsbCdcDeviceInfo *info);
const char *usbCdcLastError(void);
bool usbCdcBegin(const struct UsbCdcDeviceInfo *info);
void usbCdcTask(void);
bool usbCdcStarted(void);
bool usbCdcMounted(void);
bool usbCdcConnected(void);
uint32_t usbCdcRead(void *buffer, uint32_t size);
uint32_t usbCdcAvailable(void);
uint32_t usbCdcWrite(const void *buffer, uint32_t size);
uint32_t usbCdcWriteAvailable(void);
void usbCdcFlush(void);
void usbCdcEnd(void);

#endif
