#include "utf.h"


void utf8write(uint8_t **dstP, uint32_t chr)
{
	if (chr < 0x80)
		*(*dstP)++ = chr;
	else if (chr < 0x800) {
		*(*dstP)++ = 0xC0 + (chr >> 6);
		*(*dstP)++ = 0x80 + (chr & 0x3F);
	}
	else if (chr < 0x10000) {
		*(*dstP)++ = 0xE0 + (chr >> 12);
		*(*dstP)++ = 0x80 + ((chr >> 6) & 0x3F);
		*(*dstP)++ = 0x80 + (chr & 0x3F);
	}
	else if (chr < 0x110000) {
		*(*dstP)++ = 0xF0 + (chr >> 18);
		*(*dstP)++ = 0x80 + ((chr >> 12) & 0x3F);
		*(*dstP)++ = 0x80 + ((chr >> 6) & 0x3F);
		*(*dstP)++ = 0x80 + (chr & 0x3F);
	}
}

void utf8stateStart(struct Utf8state *s)
{
	s->bytesExpected = 0;
}

uint32_t utf8inputByte(struct Utf8state *s, uint8_t byte)
{
	uint32_t t;
	
	if (!(byte & 0x80)) {			//normal low byte
		if (s->bytesExpected)		//we're expecting a continuation
			return UTF_ERROR;
		return byte;
	}
	else if ((byte & 0xC0) == 0x80) {	//continuation
		if (!s->bytesExpected)		//we're not expecting a continuation
			return UTF_ERROR;
		t = s->code << 6;
		t += byte & 0x3F;
		
		if (!--s->bytesExpected)
			return t;
		s->code = t;
		return UTF_NO_OUTPUT;
	}
	
	if (s->bytesExpected)			//start while we're expecting a cont
		return UTF_ERROR;

	s->code = 0;
	switch ((byte >> 3) & 7) {
		case 0:
		case 1:
		case 2:
		case 3:
			s->bytesExpected = 1;
			s->code = byte & 0x1F;
			break;
		case 4:
		case 5:
			s->bytesExpected = 2;
			s->code = byte & 0x0F;
			break;
		case 6:
			s->bytesExpected = 3;
			s->code = byte & 0x07;
			break;
		default:
			return UTF_ERROR;
	}
	return UTF_NO_OUTPUT;
}

bool utf8stateFinish(const struct Utf8state *s)
{
	return !s->bytesExpected;
}

#ifdef USE_WIN_1251_CODE_PAGE

	static const uint16_t mCodepageChars[] =	//table of unicode codepoints corresponding to 0x80 .. 0xBF in the codepage
	{
		0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021, 0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
		0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0xFFFD, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
		0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7, 0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
		0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7, 0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
	};
	
	#define CODEPAGE_LAST_QUARTER_BASE	0x0410
	
#endif


#ifdef USE_WIN_1252_CODE_PAGE
	
	static const uint16_t mCodepageChars[] =	//table of unicode codepoints corresponding to 0x80 .. 0xBF in the codepage
	{
		0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD,
		0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0x0178,
		0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
		0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
	};
	
	#define CODEPAGE_LAST_QUARTER_BASE	0x00c0
	
#endif

#ifndef CODEPAGE_LAST_QUARTER_BASE
	#error "did not pick a codepage"
#endif

		
uint32_t utfFrom8bit(uint8_t val)
{
	uint32_t ret;
	
	if (val < 0x80)
		return val;
	if (val >= 0xC0)
		return (uint32_t)val + CODEPAGE_LAST_QUARTER_BASE - 0xC0;
	return mCodepageChars[val - 0x80];
	
	return ret;
}


int16_t utfTo8bit(uint32_t code)
{
	uint_fast16_t i;
	
	if (code < 0x80)
		return code;
	if (code >= CODEPAGE_LAST_QUARTER_BASE && code < CODEPAGE_LAST_QUARTER_BASE + 0x40)
		return code + 0xc0 - CODEPAGE_LAST_QUARTER_BASE;
	
	for (i = 0; i < sizeof(mCodepageChars) / sizeof(*mCodepageChars); i++) {
		
		if (code == mCodepageChars[i])
			return code + i + 0x80;
	}
	
	return -1;
}



//this is not nearly complete, but it will probably work for english, french, german, and russian
//unicode covered: 0x0000 - 0x017F, 0x400 - 0x4FF
uint32_t utfToCaseless(uint32_t chr)
{
	//for many ranges upcase is just lowercase minus a constant, we have an array of them
	static const struct {
		uint16_t first;
		uint16_t last;
		uint16_t minus;
	} simpleSub[] = {
		{0x0061, 0x007A, 0x0020},	//latin a-z
		{0x00E0, 0x00F6, 0x0020},	//latin misc
		{0x00F8, 0x00FE, 0x0020},	//latin misc
		{0x0430, 0x044F, 0x0020},	//cyrillic a-ya
		{0x0450, 0x045F, 0x0050},	//cyrillic misc
	};
	
	//for other ranges odds are lower and evens are capitals
	static const struct {
		uint16_t first;
		uint16_t last;
		bool evensAreCaps;
	} upperLowerEvenOdd[] = {
		{0x0100, 0x012F, true},		//latin misc
		{0x0132, 0x0137, true},		//latin misc
		{0x0139, 0x0148, false},	//latin misc
		{0x014A, 0x017E, true},		//latin misc
		{0x017A, 0x017E, false},	//latin misc
		{0x0182, 0x0185, true},		//latin misc
		{0x0460, 0x0481, true},		//cyrilic misc
		{0x048A, 0x04BF, true},		//cyrilic misc
		{0x04C1, 0x04CE, false},	//cyrilic misc
		{0x04D0, 0x04FF, true},		//cyrilic misc
	};
	
	//some others just map weirdly
	static const struct {
		uint16_t from;
		uint16_t to;
	} directMap [] = {
		{0x00FF, 0x0178},		//latin misc
		{0x0188, 0x0187},		//latin misc
		{0x04CF, 0x04C0},		//cyrrilic misc
	};
	
	
	
	uint32_t i;
	
	for (i = 0; i < sizeof(simpleSub) / sizeof(*simpleSub); i++) {
		if (simpleSub[i].first <= chr && simpleSub[i].last >= chr)
			return chr - simpleSub[i].minus;
	}
	
	for (i = 0; i < sizeof(upperLowerEvenOdd) / sizeof(*upperLowerEvenOdd); i++) {
		if (upperLowerEvenOdd[i].first <= chr && upperLowerEvenOdd[i].last >= chr) {
			if ((upperLowerEvenOdd[i].evensAreCaps && (chr & 1)) || (!upperLowerEvenOdd[i].evensAreCaps && !(chr & 1)))
				chr ^= 1;
			
			return chr;
		}
	}
	
	for (i = 0; i < sizeof(directMap) / sizeof(*directMap); i++) {
		if (directMap[i].from == chr)
			return directMap[i].to;
	}
	
	return chr;
}
