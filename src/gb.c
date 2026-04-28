//http://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
//http://web.archive.org/web/20010303020156/http://www.fh-karlsruhe.de/fbnw/html/Gameboy/Docs/Gbspec.txt
//https://gbdev.gg8.se/wiki/articles/CGB_Registers
//http://nocash.emubase.de/pandocs.htm#videodisplay
//http://slack.net/~ant/old/gb-tests/
//CGB TODO: HBLANK DMA, horiz & vert flip for BG tiles, BG priority
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "frontend.h"
#include "printf.h"
#include "gbCore.h"
#include "gb.h"

#pragma GCC optimize ("Ofast")


CachedCgbPal cgbBgPals;
CachedCgbPal cgbFgPals;
CachedDmgPal dmgBgPal;
CachedDmgPal dmgFgPals[2];


static uint32_t divOfst;

extern bool doubleSpeed;

void gbIoInit(void)
{
	divOfst = 0;
}

uint8_t gbIoRead(uint16_t addr)
{
	uint8_t *hram = mHRAM;
	uint8_t t, r = 0;
	int16_t v16;
	
	addr &= 0xff;
	//pr("IO RD 0xFF%02X ->", addr);
	
	switch (addr){
		
		case 0x00:	//JOYP
			t = ~gbExtGetKeys();
			if((hram[0x00] & 0x30) == 0x10) r = (hram[0x00] & 0xF0) | (t >> 4);
			else if((hram[0x00] & 0x30) == 0x20) r = (hram[0x00] & 0xF0) | (t & 0x0F);
			else if((hram[0x00] & 0x30) == 0x00) r = 0xF0;
			break;
		
		case 0x04:	//DIV
			return (gbGetCy() - divOfst) >> 6;
		
		case 0x69:
			r = hram[HRAM_INDEX_BG_PAL + (hram[0x68] & 63)];
			break;
			
		case 0x6B:
			r = hram[HRAM_INDEX_OB_PAL + (hram[0x6A] & 63)];
			break;
		
		case 0x4d:
			r = (gbIsDoubleSpeed() ? 0x80 : 0x00) + (hram[addr] & 1);
			break;
		
		case 0x10:
			r = 0x80 | hram[addr];
			break;

		case 0x11:
		case 0x16:
			r = 0x3f | hram[addr];
			break;

		case 0x13:	//WO audio regs
		case 0x15:
		case 0x18:
		case 0x1b:
		case 0x1d:
		case 0x1f:
		case 0x20:
			r = 0xff;
			break;

		case 0x14:
		case 0x19:
		case 0x1e:
		case 0x23:
			r = 0xbf | hram[addr];
			break;

		case 0x1a:
			r = 0x7f | hram[addr];
			break;

		case 0x1c:
			r = 0x9f | hram[addr];
			break;

		case 0x26:
			r = 0x70 | hram[addr];
			break;

		case 0x03:
		case 0x08 ... 0x0e:
		case 0x27 ... 0x2f:
		case 0x4c:
		case 0x4e:
		case 0x57 ... 0x67:
		case 0x6d ... 0x6f:
		case 0x71:
		case 0x76 ... 0x7f:
			(void)v16;
	#ifdef GB_HAVE_EXTRA_IO_REGS
			v16 = gbExtraIoRegRead(addr);
			if (v16 >= 0) {
				r = v16;
				break;
			}
	#endif
			accessFail(addr, 0, 0);
			r = 0;
			break;

		default:

	normal_read:
			r = hram[addr];
			break;
	}
	
	//pr("%02X\n", r);
	
	return r;
}




static void gbPrvRecalcCgbPal(CachedCgbPal dstPal, const uint8_t *srcPal, uint_fast8_t writtenOffset, uint_fast16_t flagFor0thElement)
{
	uint_fast8_t idx = writtenOffset / 2;
	uint_fast16_t gbcolor = ((const uint16_t*)srcPal)[idx];
	uint16_t ourCol;
	
	ourCol = ((gbcolor & 0x001f) << 11) + ((gbcolor & 0x03e0) << 1) + ((gbcolor & 0x7c00) >> 10);
	
	if (!(idx % 4))
		ourCol |= flagFor0thElement;
	
	dstPal[0][idx] = ourCol;	//a sleight of hand
}

static void gbPrvRecalcDmgPal(CachedDmgPal dst, uint_fast8_t regVal, uint_fast16_t flagFor0thElement)
{
	static const uint16_t grayToColor[] = {0xFFDF, 0xAD55, 0x528A, 0x0000};
	
	dst[0] = grayToColor[(regVal >> 0) & 3] | flagFor0thElement;
	dst[1] = grayToColor[(regVal >> 2) & 3];
	dst[2] = grayToColor[(regVal >> 4) & 3];
	dst[3] = grayToColor[(regVal >> 6) & 3];
}

bool gbIoWrite(uint16_t addr, uint8_t val)
{
	uint8_t *hram = mHRAM;
	uint8_t i = 0, t;
	
	addr &= 0xff;
	
	//pr("IO WR 0xFF%02X <- %02X\n", addr, val);
	
	if(addr == 0x40){
		//fprintf(stderr, "LCD cfg set %02X from 0x%02X while row=%d (mode %d)\n", val, hram[0x40], hram[0x44], gbIoRead(hram, 0x41) & 3);
		
		if (!(val & 0x80)) {
			
			//fprintf(stderr, "off\n");
			//val |= 0x80;
		}
	}
	
	switch(addr){
		
		case 0x47:	//BG color clut
			gbPrvRecalcDmgPal(dmgBgPal, val, BG_FLAG_UNDER_OBJS);
			goto write_complete;
		
		case 0x48:	//OBJ[0] color clut
			gbPrvRecalcDmgPal(dmgFgPals[0], val, 0);
			goto write_complete;
		
		case 0x49:	//OBJ[1] color clut
			gbPrvRecalcDmgPal(dmgFgPals[1], val, 0);
			goto write_complete;
		
		case 0x02:	//SERIAL control reg
			if(val & 0x80) accessFail(addr, 1, val);
			else goto write_complete;
			break;
		
		case 0x04:	//DIV
			divOfst = gbGetCy();
			break;
		
		case 0x07:	//timer config
			val &= 7;	//asm code assumes no weird bits are set
			if (!(hram[0x07] & 0x4) && (val & 0x04))
				gbTmrCySetToCy();
			goto write_complete;
		
		case 0x46:	//DMA
			for (i = 0; i < 0xA0; i++)
				gbExtWrite(0xFE00 | i, gbExtRead((((uint16_t)val) << 8) | i));
			break;
		
		case 0x40:	//LCD config
			if ((hram[0x40] & 0x80) && !(val & 0x80)) {	//turning off
				
				hram[0x41] &= (uint8_t)~0x07;	//LY != LYC
				hram[0x44] = 0;
			}
			else if (!(hram[0x40] & 0x80) && (val & 0x80)) {	//turning on
				
				hram[0x41] = (hram[0x41] & (uint8_t)~0x03) | 2;	//reset state to mode 2
				hram[0x44] = 0;
				gbLcdCySetToCy();
			}
			
			//adjust cgb cluts
			if ((hram[0x40] ^ val) & 0x01) {
				
				uint_fast8_t i, j;
				
				if (val & 1) {
					for (i = 0; i < 8; i++) {
						for (j = 1; j < 4; j++)
							cgbBgPals[i][j] &=~ BG_FLAG_UNDER_OBJS;
					}
				}
				else {
					
					for (i = 0; i < 8; i++) {
						for (j = 0; j < 4; j++)	//not a bug, [0] already has the bit, but this loop will triviall yunfold with the outer loop better this way
							cgbBgPals[i][j] |= BG_FLAG_UNDER_OBJS;
					}
				}
			}
			goto write_complete;
		
		case 0x41:
			
			val = (val & 0xF8) | (hram[0x41] & 7);
			goto write_complete;
		
		case 0x4F:
			val |= 0xfe;
			gbExtSetVramPage(val & 1);
			goto write_complete;
		
		case 0x70:
			val &= 7;
			gbExtSetWramPage(val);
			goto write_complete;
		
		case 0x55:	//DMA start
			{
				uint16_t d = (((uint16_t)hram[0x53]) << 8) | hram[0x54];
				uint16_t s = (((uint16_t)hram[0x51]) << 8) | hram[0x52];
			
				d = (d & 0x1FF0) | 0x8000;
				s = s & 0xFFF0;
			
				if (val & 0x80){
					
				//	fprintf(stderr,"HBLANK DMA %x0 %02X%02X->%02X%02X\n", (val & 0x7F) + 1, hram[0x51], hram[0x52], 0x80|hram[0x53], hram[0x54]);
					
					
					val &= 0x7F;	//record that we need to do the DMA
					if (hram[0x40] & 0x80)	//hblank dma requested while screen is on - normal op, while off - treat it liek immediate. not sure if right...
						goto write_complete;
				}
				
				if (hram[0x55] != 0xFF) {	//cancel ongoing HBLANK DMA
					
				//	fprintf(stderr,"HBLANK DMA CANCEL\n");
					val = 0x80 + (hram[0x55] & 0x7f);
					goto write_complete;
				}
				
				//do the DMA instantly
				
				//	fprintf(stderr,"NEW DMA %x0 %02X%02X->%02X%02X\n", (val & 0x7F) + 1, hram[0x51], hram[0x52], 0x80|hram[0x53], hram[0x54]);
				
				do{
					
					i = 0x10;
					do{
					
						gbExtWrite(d++,gbExtRead(s++));
					
					}while(--i);
				}while(val--);
				//val will end up 0xFF as needed automatically due to the above subtract
				goto write_complete;
			}
		
		case 0x56:	//IR (we never recv anything)
			val = ((val >> 6) == 3 ? 0x02 : 0x00) | (val & 0xc1);
			goto write_complete;
		
		case 0x69:		//BG pal write
			t = hram[0x68];				//t = hramPtrIdx
			i = t & 63;					//desired index
			hram[HRAM_INDEX_BG_PAL + i] = val;
			gbPrvRecalcCgbPal(cgbBgPals, hram + HRAM_INDEX_BG_PAL, i, BG_FLAG_UNDER_OBJS);
			if (t & 0x80)
				hram[0x68] = (t + 1) & 0xBF;
			break;
			
		case 0x6B:		//OBJ pal write
			t = hram[0x6A];				//t = hramPtrIdx
			i = t & 63;					//desired index
			hram[HRAM_INDEX_OB_PAL + i] = val;
			gbPrvRecalcCgbPal(cgbFgPals, hram + HRAM_INDEX_OB_PAL, i, 0);
			if (t & 0x80)
				hram[0x6A] = (t + 1) & 0xBF;
			break;
		
		case 0x4d:
			val &= 1;
			goto write_complete;
		
		case 0x27 ... 0x2F:	//nonexistent audio regs accept writes
			val = 0xff;
			goto write_complete;

		case 0x12:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if (!(val & 0xf8))			//if dac was turned off, the channel is now off
				hram[0x26] &=~ 0x01;
			goto write_complete;

		case 0x17:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if (!(val & 0xf8))			//if dac was turned off, the channel is now off
				hram[0x26] &=~ 0x02;
			goto write_complete;

		case 0x21:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if (!(val & 0xf8))			//if dac was turned off, the channel is now off
				hram[0x26] &=~ 0x08;
			goto write_complete;

		case 0x1a:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if (!(val & 0x80))			//if dac was turned off, the channel is now off
				hram[0x26] &=~ 0x04;
			goto write_complete;

		case 0x14:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if ((val & 0x80) && !(hram[0x26] & 0x01) && (hram[0x12] & 0xf8)) {	//trigger a channel that was off and whose dac allows it to come on
				hram[0x26] |= 0x01;	//it is on
				//pr("ch 1 on. NR52=0x%02x\n", hram[0x26]);
			}
			goto write_complete;

		case 0x19:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if ((val & 0x80) && !(hram[0x26] & 0x02) && (hram[0x17] & 0xf8)) {	//trigger a channel that was off and whose dac allows it to come on
				hram[0x26] |= 0x02;	//it is on
				//pr("ch 2 on. NR52=0x%02x\n", hram[0x26]);
			}
			goto write_complete;

		case 0x1e:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if ((val & 0x80) && !(hram[0x26] & 0x04) && (hram[0x1a] & 0x80))	//trigger a channel that was off and whose dac allows it to come on
				hram[0x26] |= 0x04;	//it is on
			goto write_complete;

		case 0x23:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if ((val & 0x80) && !(hram[0x26] & 0x08) && (hram[0x21] & 0xf8))	//trigger a channel that was off and whose dac allows it to come on
				hram[0x26] |= 0x08;	//it is on
			goto write_complete;

		case 0x10:
		case 0x11:						//audio regs only take writes when audio is on
		case 0x13:
		case 0x15:
		case 0x16:
		case 0x18:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x22:
		case 0x24:
		case 0x25:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			goto write_complete;

		case 0x26:
			if (val & 0x80) {
				hram[0x26] |= 0x80;	//turning on
				gbAudioCySetToCy();
			}
			else if (hram[0x26] & 0x80) {	//turning off
				for (i = 0x10; i <= 0x26; i++)
					hram[i] = 0x00;
			}
			break;
		
		case 0x75:
			val &= 0x70;
			goto write_complete;

		case 0x03:
		case 0x08 ... 0x0e:
		case 0x4c:
		case 0x4e:
		case 0x57 ... 0x67:
		case 0x6d ... 0x6f:
		case 0x77 ... 0x7f:
	#ifdef GB_HAVE_EXTRA_IO_REGS
			if (gbExtraIoRegWrite(addr, val))
				break;
	#endif
			accessFail(addr, 1, val);
			return false;

		default:
	write_complete:
			hram[addr] = val;
			break;
	}
	return true;
}

struct state {		//only used for reporting and onlty relevant to the ASM core
	union{
		uint16_t hl;	//0x04
		struct{
			uint8_t l;	//0x04
			uint8_t h;	//0x05
		};
	};
	union{
		uint16_t bc;	//0x00
		struct{
			uint8_t c;	//0x00
			uint8_t b;	//0x01
		};
	};
	union{
		uint16_t de;	//0x02
		struct{
			uint8_t e;	//0x02
			uint8_t d;	//0x03
		};
	};
	
	uint8_t intWait;	//0x06	must be 0xFF or 0x00
	uint8_t intsOn;		//0x07	must be 0xFF or 0x00
	uint8_t intsOnNext;	//0x08	must be 0xFF or 0x00
	uint8_t zVal;		//0x09	VALUE based on WHICH the Z reg would be set, if we had one
	uint8_t fH;			//0x0a	actual "H" flag	MUST be 0 or 1 only
	uint8_t fN;			//0x0b	actual "N" flag	MUST be 0 or 1 only
	uint8_t fC;			//0x0c	actual "C" flag MUST be 0 or 1 only
	uint8_t actAsCgb;	//0x0d
	uint8_t dblSpeed;	//0x0e	only may contain 0 (slow) or 32 (fast)
	uint8_t dblSpeed2;	//0x0f	only may contain 11 (slow) or 12 (fast)
	uint8_t sndStep;	//0x10	only may contain evens 0..14

	uint32_t tmrCy;		//0x14
	uint32_t lcdCy;		//0x18
	uint32_t sndCy;		//0x1c
};


#define OFST_ACT_CGB		0x0d
#define OFST_DBL_SPEED		0x0e		//must only contain 0 or 32
#define OFST_SND_STEP		0x0f		//stores step * 2 (only evens)

#define OFST_TMR_CY			0x10
#define OFST_LCD_CY			0x14
#define OFST_SND_CY			0x18
#define SIZEOF_STATE		0x1c



void report(struct state* state, uint32_t a, uint32_t pc, uint32_t sp, uint32_t cy, uint32_t minusLcdCy, uint32_t minusTmrCy, uint32_t minusSndCy)
{
	(void)minusLcdCy;
	(void)minusTmrCy;
	(void)minusSndCy;
		
	//if(doLog)
	{
		pr("[%04x] A=%02x BC=%04x DE=%04x HL=%04x SP=%04x F=%02x @ %lu (lcd %lu, tmr %lu, snd %lu)\n",
			pc, a, state->bc, state->de, state->hl, sp, 
				(state->zVal ? 0x00 : 0x80) + 
				(state->fN ? 0x40 : 0x00) + 
				(state->fH ? 0x20 : 0x00) +
				(state->fC ? 0x10 : 0x00), cy, -minusLcdCy, -minusTmrCy, -minusSndCy
		);
	}
	
	if (a >> 8)
		pr("invalid A\n");
	else if (sp >> 16)
		pr("invalid SP\n");
	else if (pc >> 16)
		pr("invalid PC\n");
	else if (state->fC > 1)
		pr("invalid fC\n");
	else if (state->fN > 1)
		pr("invalid fN\n");
	else if (state->fH > 1)
		pr("invalid fH\n");
	else if (state->intWait != 0x00 && state->intWait != 0xff)
		pr("invalid intWait\n");
	else if (state->intsOn != 0x00 && state->intsOn != 0xff)
		pr("invalid intsOn\n");
	else if (state->intsOnNext != 0x00 && state->intsOnNext != 0xff)
		pr("invalid intsOnNext\n");
	else
		return;
	
	while(1);
}

