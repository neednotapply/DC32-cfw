#ifndef _TINYUSB_COMPAT_HARDWARE_IRQ_H_
#define _TINYUSB_COMPAT_HARDWARE_IRQ_H_

#include "2350.h"

#define USBCTRL_IRQ USBCTRL_IRQ_IRQn
#define PICO_SHARED_IRQ_HANDLER_HIGHEST_ORDER_PRIORITY 0xff

typedef void (*irq_handler_t)(void);

static inline void irq_add_shared_handler(int irq, irq_handler_t handler, uint8_t order_priority)
{
	(void)irq;
	(void)handler;
	(void)order_priority;
}

static inline void irq_remove_handler(int irq, irq_handler_t handler)
{
	(void)irq;
	(void)handler;
}

static inline void irq_set_enabled(int irq, bool enabled)
{
	(void)irq;
	(void)enabled;
}

#endif
