#ifndef _DISP_DEFCON_H_
#define _DISP_DEFCON_H_

#include <stdbool.h>
#include <stdint.h>

//these dimensions are unrotateable - they are hardware dimensions

#define HARDWARE_WIDTH		240	//hardware
#define HARDWARE_HEIGHT		320

#define DISP_WIDTH 			216	//window
#define DISP_HEIGHT			240

#define DISP_INDEXED_LE		1		//does not matter

#define DISP_BPP			16

#define DISP_SPEED_SETTINGS	{30, 60, 90, 120}
#define DISP_SPEED_NAMES	{"50%     ", "100%    ", "150%    ", "200%    "}

#define	DISP_BRIGHTNESS_ADJUSTABLE
//#define	DISP_CONTRAST_ADJUSTABLE


bool dispInit(uint32_t desiredFramerate);
bool dispOn(void);
bool dispOff(void);

void* dispGetFb(void);

void dispPrvFrameCtrReset(void);
void dispPrvFrameCtrWait(void);

void dispSetBrightness(uint_fast8_t val);
void dispSetContrast(uint_fast8_t val);

//frame
extern const uint8_t lcdFrame[];


#endif
