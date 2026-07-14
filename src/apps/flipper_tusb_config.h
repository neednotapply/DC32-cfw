#ifndef _FLIPPER_TUSB_CONFIG_H_
#define _FLIPPER_TUSB_CONFIG_H_

#define CFG_TUSB_MCU OPT_MCU_RP2040
#define CFG_TUSB_OS OPT_OS_NONE
#define CFG_TUSB_DEBUG 0
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_HOST | OPT_MODE_FULL_SPEED)
#define CFG_TUH_ENABLED 1
#define CFG_TUH_MAX_SPEED OPT_MODE_FULL_SPEED
#define CFG_TUH_DEVICE_MAX 1
#define CFG_TUH_HUB 0
#define CFG_TUH_CDC 1
#define CFG_TUH_CDC_RX_BUFSIZE 2048
#define CFG_TUH_CDC_TX_BUFSIZE 2048
#define CFG_TUH_ENUMERATION_BUFSIZE 256
#define CFG_TUH_RPI_PIO_USB 0

#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))

#ifndef __not_in_flash
#define __not_in_flash(group) __attribute__((section(".time_critical." group)))
#endif

#endif
