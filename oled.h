#ifndef _DISP_H_
#define _DISP_H_

#include <stdint.h>

void dispInit(uint32_t desiredFramerate);
void dispOff(void);

void* dispGetFb(void);

void dispSetContrast(uint_fast8_t val);
void dispSetBrightness(uint_fast8_t val);

void dispPrvFrameCtrReset(void);
void dispPrvFrameCtrWait(void);

#define DISP_WIDTH			160
#define DISP_HEIGHT			160
#define DISP_BPP			4
#define DISP_INDEXED_LE		1

#define DISP_SPEED_SETTINGS	{30, 45, 60, 75}

//#define	DISP_BRIGHTNESS_ADJUSTABLE
#define	DISP_CONTRAST_ADJUSTABLE

#endif
