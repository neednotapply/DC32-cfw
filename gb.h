#ifndef _GB_H_
#define _GB_H_

#include <stdbool.h>
#include <stdint.h>

void gbRun(bool presentAsCgb);	//will not return by itself, but can be aborted by gbAbort()
void gbAbort(void);


#define LOG(...)		//pr(__VA_ARGS)

//external dependencies

#define KEY_BIT_START	0x80
#define KEY_BIT_SEL		0x40
#define KEY_BIT_B		0x20
#define KEY_BIT_A		0x10
#define KEY_BIT_DOWN	0x08
#define KEY_BIT_UP		0x04
#define KEY_BIT_LEFT	0x02
#define KEY_BIT_RIGHT	0x01

void gbSetPointers(const void *rom, uint32_t romSz, void *cartRam, uint32_t cartRamSz);

uint8_t gbExtGetKeys(void);
uint8_t gbExtRead(uint16_t addr);
void gbExtWrite(uint16_t addr, uint8_t val);

void gbExtSetWramPage(uint8_t pg);
void gbExtSetVramPage(uint8_t pg);

void accessFail(uint16_t addr, char wr, uint8_t val);

void gbSetFrameDithering(uint_fast8_t n);	//draw one every N frames

void gbAudioTrigger(uint_fast8_t ch);

void gbIoInit(void);

#ifdef GB_HAVE_EXTRA_IO_REGS
	int16_t gbExtraIoRegRead(uint8_t addr);	//return negative no error, positive for value
	bool gbExtraIoRegWrite(uint8_t addr, uint8_t data);
#endif

uint32_t gbExtRtcGet(void);
void gbExtRtcSet(uint32_t time);

void gbExtAccelRead(uint16_t *xP, uint16_t *yP);		//please do not block, middle should be 0x8000, one g is 0x70 in magnitude

#endif
