/*
	(c) 2021 Dmitry Grinberg   https://dmitry.gr
	Non-commercial use only OR licensing@dmitry.gr
*/

#ifndef _PRINTF_H_
#define _PRINTF_H_

#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#ifdef SUPPORT_DEBUG_PRINTF

	#define pr			prRaw

#else

	#define pr(...) do {} while(0)

#endif

//use %lc anf %ls for wide strings/chars

//easy
void prRaw(const char* fmtStr, ...);
//...required for that
void prPutchar(char chr);

//display
void screenVprintf(uint8_t *framebuffer, uint32_t w, uint32_t h, int32_t r, int32_t c, uint_fast8_t foreColor, uint_fast8_t backColor, const char* fmtStr, va_list vl);	//no wraparound
void screenPrintf(uint8_t *framebuffer, uint32_t w, uint32_t h, int32_t r, int32_t c, uint_fast8_t foreColor, uint_fast8_t backColor, const char* fmtStr, ...);	//no wraparound

//fancy
void vxprintf(void *userData, void(*callback)(void *userData, char ch), const char *str, va_list vl);

//common
int sprintf(char *str, const char *format, ...);


#endif

