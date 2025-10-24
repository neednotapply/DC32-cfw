#ifndef _UTF_H_
#define _UTF_H_

#include <stdbool.h>
#include <stdint.h>

//once an error is encountered, code gives up. this is ok for our purposes

#define UTF_ERROR		0xFFFFFFFEUL	//retutrned by *inputWord/*stateFinish/utf16AnalyzeBom
#define UTF_NO_OUTPUT		0xFFFFFFFFUL

#define UTF16_LE		0xFFFFFFF0UL	//returned by utf16AnalyzeBom
#define UTF16_BE		0xFFFFFFF1UL

#define UTF_REPLACEMENT_CHAR	0xFFFD




struct Utf8state {
	uint32_t code;
	uint8_t bytesExpected;
};

#define UTF8_STATE_STATIC_INITIALIZER		{.bytesExpected = 0, }

void utf8stateStart(struct Utf8state *s);
uint32_t utf8inputByte(struct Utf8state *s, uint8_t byte);
bool utf8stateFinish(const struct Utf8state *s);	//return false for error (invalid state at end)

void utf8write(uint8_t **dstP, uint32_t codepoint);



//codepage
uint32_t utfFrom8bit(uint8_t val);		//uses selected codepage, because that is the one i like. deal with it!
int16_t utfTo8bit(uint32_t code);		//negative if impossible
uint32_t utfToCaseless(uint32_t chr);	//not necessarily to upper or to lower, but to some caseless state (mostly to upper)

#endif
