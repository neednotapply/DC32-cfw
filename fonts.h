#ifndef _FONTS_H_
#define _FONTS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


struct FontGlyphInfo {
	const void *src;
	uint8_t width, height;
};

enum Font {
	FontSmall,
	FontMedium,	//Century 10pt
	FontLarge,	//Comic Sans 13pt
};

bool fontGetGlyphInfo(struct FontGlyphInfo *glyph, enum Font which, wchar_t ch);
bool fontGetGlyphPixel(const struct FontGlyphInfo *glyphInfo, uint_fast8_t row, uint_fast8_t col);
uint_fast8_t fontGetHeight(enum Font which);


#endif
