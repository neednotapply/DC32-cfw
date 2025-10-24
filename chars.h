#ifndef _CHARS_H_
#define _CHARS_H_

#include <stdint.h>


#define CHAR_WIDTH	6
#define CHAR_HEIGHT 8
void charsPrvDrawOne(uint8_t *framebuffer, uint32_t w, uint32_t h, int32_t r, int32_t c, char ch, uint_fast8_t foreColor, uint_fast8_t backColor);



#define UP_ARROW_CHAR		(0x80)
#define DOWN_ARROW_CHAR		(0x81)
#define RIGHT_ARROW_CHAR	(0x82)
#define LEFT_ARROW_CHAR		(0x83)

#define FIRST_VALID_CHAR	0x20
#define LAST_VALID_CHAR		0x83
#define MISSING_CHAR_CHAR	0x7F




#endif