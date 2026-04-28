/*
	(c) 2021 Dmitry Grinberg   https://dmitry.gr
	Non-commercial use only OR licensing@dmitry.gr
*/


#pragma GCC optimize ("Os")


#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include "printf.h"
#include "utf.h"



static void doubleDabble(uint8_t *dst, uint64_t val)	//dst must be 10 bytes
{
	uint_fast8_t i, j;
	
	for (i = 0; i < 10; i++)
		dst[i] = 0;
	
	for (i = 0; i < 64; i++) {
		
		uint_fast16_t shiftIn;
		
		for (j = 0; j < 10; j++) {
			
			if (dst[j] >= 0x50)
				dst[j] += 0x30;
			
			if ((dst[j] & 0x0f) >= 5)
				dst[j] += 3;
		}
		shiftIn = val >> 63;
		val <<= 1;
		
		for (j = 0; j < 10; j++) {
			
			shiftIn += 2 * dst[j];
			dst[j] = shiftIn;
			shiftIn >>= 8;
		}
	}
}

static void copyOver(uint8_t *dst, uint64_t val)	//dst must be 10 bytes
{
	uint_fast8_t i;
	
	for (i = 0; i < 10; i++, val >>= 8)
		dst[i] = val & 0xff;
}

static uint8_t getBcdDigit(const uint8_t *buf, uint_fast8_t digit)
{
	uint_fast8_t t = buf[digit / 2];
	
	if (digit & 1)
		return t >> 4;
	else
		return t & 0x0f;
}

static void StrPrvPrintfEx_number(void *userData, void(*callback)(void *userData, char ch), uint64_t number, bool baseTen, bool zeroExtend, bool isSigned, uint32_t padToLength)
{
	uint_fast8_t numLen, printedLen, padChars, i;
	bool wasNegative = false;
	uint8_t buf[10];
	
	if (isSigned && ((int64_t)number) < 0) {
		
		wasNegative = true;
		number = -number;
	}
	
	if (baseTen)
		doubleDabble(buf, number);
	else
		copyOver(buf, number);
	
	//find the length of the number (1 minimum)
	for (numLen = sizeof(buf) * 2; numLen >= 2 && !getBcdDigit(buf, numLen - 1); numLen--);
	
	printedLen = numLen + (wasNegative ? 1 : 0);
	
	if (printedLen > padToLength)
		padChars = 0;
	else
		padChars = padToLength - printedLen;
	
	//padding works differently for zeroes and for spaced w.r.t. the minus sign
	
	if (zeroExtend && wasNegative)
		callback(userData, '-');
	while (padChars--)
		callback(userData, zeroExtend ? '0' : ' ');
	if (!zeroExtend && wasNegative)
		callback(userData, '-');
	for (i = 0; i < numLen; i++) {
		char ch = getBcdDigit(buf, numLen - 1 - i);
		
		ch = (ch < 10) ? ch + '0' : ch + 'A' - 10;
		callback(userData, ch);
	}
}

static uint32_t StrVPrintf_StrLen_withMax(const char* s, uint32_t max)
{
	uint32_t len = 0;
	
	while ((*s++) && (len < max))
		len++;
	
	return len;
}

static void vxprintfPrvString(void *userData, void(*callback)(void *userData, char ch), uint32_t padToLength, const void *src, bool wide)
{
	const wchar_t *srcW = (const wchar_t*)src;
	const char *srcN = (const char*)src;
	uint32_t i, len;
	wchar_t ch;
	
	//get length
	for (len = 0; ((padToLength && len < padToLength) || !padToLength) && (wide ? !!srcW[len] : !!srcN[len]); len++);

	//pad
	for (i = len; i < padToLength; i++)
		callback(userData, ' ');
	
	//emit
	if (wide) {
		
		uint8_t buf[6], *end = buf, *start = buf;
		
		utf8write(&end, srcW[i]);
		while(start != end)
			callback(userData, *start++);
	}
	else {
		for (i = 0; i < len; i++)
			callback(userData, srcN[i]);
	}
}

void vxprintf(void *userData, void(*callback)(void *userData, char ch), const char *fmtStr, va_list vl)
{
	uint64_t val64;
	char c, t;
	
	while ((c = *fmtStr++) != 0) {
		
		if (c == '%') {
			
			bool zeroExtend = false, useLong = false, useVeryLong = false, isSigned = false, baseTen = false;
			uint32_t padToLength = 0, len, i;
			const char* str;
			
more_fmt:
			switch (c = *fmtStr++) {
				
				case '%':
				default:	
					callback(userData, c);
					break;
				
				case 'c':
					
					if (useLong) {
						
						wchar_t t = va_arg(vl, wchar_t);
						vxprintfPrvString(userData, callback, 1, &t, true);
					}	
					else
						callback(userData, (char)va_arg(vl, int));
					break;
				
				case 's':
					
					vxprintfPrvString(userData, callback, padToLength, va_arg(vl, void*), useLong);
					break;
				
				case '0':
					
					if (!zeroExtend && !padToLength) {
						
						zeroExtend = true;
						goto more_fmt;
					}
					//fallthrough
				
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					
					padToLength = (padToLength * 10) + c - '0';
					goto more_fmt;
				
				case 'd':
					isSigned = true;
					//fallthrough
					
				case 'u':
					baseTen = true;
					//fallthrough
					
				case 'x':
				case 'X':
					val64 = useVeryLong ? va_arg(vl, uint64_t) : va_arg(vl, uint32_t);
					if (isSigned && !useVeryLong)
						val64 = (int64_t)(int32_t)val64;
					StrPrvPrintfEx_number(userData, callback, val64, baseTen, zeroExtend, isSigned, padToLength);
					break;
					
				case 'l':
					if(useLong)
						useVeryLong = true;
					useLong = true;
					goto more_fmt;

			}
		}
		else
			callback(userData, c);
	}
}

static void prRawPrintCbk(void *userData, char ch)
{
	prPutchar(ch);
}

void prRaw(const char *format, ...)
{
	va_list vl;
	
	va_start(vl, format);
	(void)vxprintf(NULL, prRawPrintCbk, format, vl);
	va_end(vl);
}

static void sprintfPrintCbk(void *userData, char ch)
{
	char **dstPP = (char**)userData;
	
	*(*dstPP)++ = ch;
}

int sprintf(char *str, const char *format, ...)
{
	char *dst = str;
	va_list vl;
	
	va_start(vl, format);
	(void)vxprintf(&dst, sprintfPrintCbk, format, vl);
	va_end(vl);
	*dst = 0;
	
	return dst - str;
}
