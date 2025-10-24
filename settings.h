#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <stdbool.h>
#include <stdio.h>

struct Settings {
	uint8_t actLikeGBC	:	1;
	uint8_t speed		:	2;
	uint8_t contrast	:	5;

	uint8_t upscale		:	1;
	uint8_t brightness	:	5;
};


void settingsGet(struct Settings *settings);
bool settingsSet(const struct Settings *settings);




#endif

