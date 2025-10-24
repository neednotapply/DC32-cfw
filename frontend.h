#ifndef _FRONTEND_H_
#define _FRONTEND_H_

#include <stdint.h>


#define PIXFMT	uint16_t	//RGXB 5:5:1:5

void gbDrawLine(uint8_t lineNum, PIXFMT* pixels);



#endif

