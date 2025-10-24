#ifndef _PIO_WS2812_H_
#define _PIO_WS2812_H_

#include <stdbool.h>
#include <stdint.h>


#define NUM_WS2812s		9



void ws2812init(void);

void ws2812Set(uint32_t ledIdx, uint_fast8_t colorIdx, uint_fast8_t val);

void ws2812refresh(void);	//will block if called too soon after last time


#endif
