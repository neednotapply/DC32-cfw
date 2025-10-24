#include <setjmp.h>
#include "gbCore.h"
#include "printf.h"
#include "gb.h"

/*
https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware
https://gbdev.io/pandocs/Audio.html
https://gbdev.io/pandocs/Audio_details.html#div-apu
https://gbdev.io/pandocs/Audio_Registers.html#ff11--nr11-channel-1-length-timer--duty-cycle
https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
https://gist.github.com/armornick/3447121

*/




#pragma GCC optimize ("Ofast")

#define BC	((((uint16_t)B) << 8) | C)
#define DE	((((uint16_t)D) << 8) | E)
#define HL	((((uint16_t)H) << 8) | L)


uint32_t cy = 0;
uint32_t lcdCy = 0;
static uint32_t tmrCy = 0;
static uint32_t mAudioCy = 0;
static uint8_t mAudioStep = 0;
static bool doubleSpeed = false;
static jmp_buf mAbortBuf;

//covers cases for x1 x2 x3 x4 x5 x6
#define STD_REG_OP1(ofst, hi, lo, both, autoinc)	\
	case ofst + 0x01:	/* hl = (PC) */		\
		lo = gbRead(PC++);		\
		hi = gbRead(PC++);		\
		cy += 2;				\
		break;					\
							\
	case ofst + 0x02:	/*(hl) <- A */		\
		gbWrite(both, A);			\
		if(autoinc) if(!++lo) hi++;		\
		cy++;					\
		break;					\
							\
	case ofst + 0x03:	/* hl++ */		\
		if(!++lo) hi++;				\
		cy++;					\
		break;					\
							\
	case ofst + 0x04:	/* hi++ */		\
		hi++;					\
		fN = 0;					\
		fZ = !hi;				\
		fH = !(hi & 0x0F);			\
		break;					\
							\
	case ofst + 0x05:	/* hi-- */		\
		fH = !(hi & 0x0F);			\
		hi--;					\
		fN = 1;					\
		fZ = !hi;				\
		break;					\
							\
	case ofst + 0x06:	/* hi = immed */	\
		hi = gbRead(PC++);		\
		cy++;					\
		break;

//covers cases for x9 xA xB xC xD xE
#define STD_REG_OP2(ofst, hi, lo, both, autoinc)	\
	case ofst + 0x09:	/* HL += hl */		\
		fH = ((HL & 0x0fff) + (both & 0x0fff)) >> 12;	\
		cy++;					\
		fN = 0;					\
		T = HL + both;			\
		fC = T < HL;			\
		H = T >> 8;				\
		L = T;					\
		break;					\
							\
	case ofst + 0x0A:	/* A <- (hl) */		\
		A = gbRead(both);			\
		if(autoinc) if(!++lo) hi++;		\
		cy++;					\
		break;					\
							\
	case ofst + 0x0B:	/* hl-- */		\
		if (!lo--) hi--;		\
		cy++;					\
		break;					\
							\
	case ofst + 0x0C:	/* lo++ */		\
		lo++;					\
		fZ = !lo;				\
		fN = 0;					\
		fH = !(lo & 0x0F);			\
		break;					\
							\
	case ofst + 0x0D:	/* lo-- */		\
		fH = !(lo & 0x0F);			\
		lo--;					\
		fZ = !lo;				\
		fN = 1;					\
		break;					\
							\
	case ofst + 0x0E:	/* lo = immed */	\
		lo = gbRead(PC++);		\
		cy++;					\
		break;

#define DO_POP(hi, lo)					\
		lo = gbRead(SP++);		\
		hi = gbRead(SP++);		\
		cy += 2;

#define DO_PUSH(hi, lo)					\
		gbWrite(--SP, hi);		\
		gbWrite(--SP, lo);		\
		cy += 3;

#define GET_REG_FROM_INSTR()						\
		switch(instr & 7){					\
			case 0:	t8 = B; break;				\
			case 1:	t8 = C; break;				\
			case 2: t8 = D; break;				\
			case 3: t8 = E; break;				\
			case 4: t8 = H; break;				\
			case 5: t8 = L; break;				\
			case 6: t8 = gbRead(HL); cy++; break;	\
			case 7: t8 = A; break;				\
		}

uint32_t gbGetCy(void)
{
	return cy;
}

void gbLcdCySetToCy(void)
{
	lcdCy = cy;
}

void gbTmrCySetToCy(void)
{
	tmrCy = cy;
}

void gbAudioCySetToCy(void)
{
	mAudioCy = cy;
	mAudioStep = 0;
}

static uint8_t gbRead(uint16_t addr){
	
	return (addr >= 0xFF00) ? gbIoRead(addr) : gbExtRead(addr);
}

static void gbWrite(uint16_t addr, uint8_t val){
	
	if(addr >= 0xFF00) gbIoWrite(addr, val);
	else gbExtWrite(addr, val);
}

bool gbIsDoubleSpeed(void)
{
	return doubleSpeed;
}

void gbAbort(void)
{
	longjmp(mAbortBuf, 1);
}

void gbRun(bool presentAsCgb)
{
	const bool presentsAsCgb = presentAsCgb && (gbExtRead(0x143) & 0x80);
	uint8_t *hram = mHRAM;
	uint8_t A = presentsAsCgb ? 0x11 : 0x01, B = 0x00, C = 0x13, D = 0x00, E = 0xD8, H = 0x01, L = 0x4D, instr, t8, intsOn = 0, intsOnNext = 0, fZ = 1, fC = 1, intWait = 0, oldFC, fN = 0, fH = 1;
	uint16_t SP = 0xFFFE, PC = 0x100, T;
	
	if (setjmp(mAbortBuf))
		return;
	
	gbIoInit();

	//init registers & state
	{
		static const uint8_t initDataRng[] =	{
								0x10, 3, 0x80, 0xBF, 0xF3,
								0x19, 5, 0xBF, 0x7F, 0xFF, 0x9F, 0xBF,
								0x23, 4, 0xBF, 0x77, 0xF3, 0xF1,
								0x47, 3, 0xFC, 0xFF, 0xFF
							};
		static const uint8_t initDataSep[] =	{
								0x14, 0xBF,
								0x16, 0x3F,
								0x20, 0xFF,
								0x40, 0x91,
								0x41, 0x02
							};
		
		static const uint8_t initWaveDataDMG[] = {0x84, 0x40, 0x43, 0xAA, 0x2D, 0x78, 0x92, 0x3C, 0x60, 0x59, 0x59, 0xB0, 0x34, 0xB8, 0x2E, 0xDA, };
		static const uint8_t initWaveDataCGB[] = {0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, };

		for(T = 0; T < sizeof(initDataRng);){
			
			t8 = initDataRng[T++];
			instr = initDataRng[T++];
			while(instr--) hram[t8++] = initDataRng[T++];
		}
		for(T = 0; T < sizeof(initDataSep); T += 2) hram[initDataSep[T]] = initDataSep[T + 1];

		for(T = 0; T < 16; T++)
			hram[0x30 + T] = (presentAsCgb ? initWaveDataCGB : initWaveDataDMG)[T];
	}
	
	while(1){
	
		if (0) {
			
			pr("[%04x] A=%02x BC=%04x DE=%04x HL=%04x SP=%04x F=%02x @ %lu (lcd %lu, tmr %lu)\n",
				PC, A, BC, DE, HL, SP, 
					(fZ ? 0x80 : 0x00) + 
					(fN ? 0x40 : 0x00) + 
					(fH ? 0x20 : 0x00) +
					(fC ? 0x10 : 0x00),
				cy, lcdCy, tmrCy
			);
		}
		
		#ifndef EMBEDDED
			if(!(cy & 15)) gbPoll();
		#endif

#if 0
	//---	this is the perfectly correct timer handler that covers all cases, the code below is faster
		if(hram[0x07] & 0x04){	//timer on? -> do the timery things
			
			uint16_t per;
			
			switch(hram[0x07] & 3){
				case 0: per = 256; break;
				case 1: per = 4; break;
				case 2: per = 16; break;
				case 3: per = 64; break;
			}
			
			while(cy - tmrCy >= per){
				tmrCy += per;
				if(!++hram[0x05]){
				
					hram[0x05] = hram[0x06];
					hram[0x0F] |= 0x04;	//int
				}
			}
		}
#else
		if(hram[0x07] & 0x04){	//timer on? -> do the timery things
			
			uint_fast16_t t, perShift, numTicks;
			
			switch(hram[0x07] & 3){
				case 0: perShift = 8; break;
				case 1: perShift = 2; break;
				case 2: perShift = 4; break;
				case 3: perShift = 6; break;
			}
			
			numTicks = (cy - tmrCy) >> perShift;	//this many ticks happened since we last were here
			if (numTicks) {
				
				tmrCy += numTicks << perShift;			//account for having handled it

				t = (uint_fast16_t)hram[0x05] + numTicks;
				if (t >> 8) {							//timer overflowed
					hram[0x05] = t + hram[0x06];		//technically this can overflow too, if numTicks is too high. we do not expect that
					hram[0x0F] |= 0x04;	//int
				}
				else {									//timer did not overflow
					hram[0x05] = t;
				}
			}
		}
#endif

		//do the audio things
		if (hram[0x26] & 0x80) {	//Audio on? -> do the audio things
			uint16_t since = cy - mAudioCy;

			if (doubleSpeed)
				since /= 2;

			if (since >= 2048) {

				static const uint8_t mLengthTimerMasks[] = {0x3f, 0x3f, 0xff, 0x3f};
				uint_fast8_t i;


				switch (mAudioStep++) {
					case 2:
					case 6:
						//sweep (xxx for ch1 this can turn it off)
						//fallthrough
					case 0:
					case 4:
						//length (clocked even if the channel is OFF)

						for (i = 0; i < 4; i++) {

							uint8_t *regs = hram + 0x10 + 5 * i, lengthMask = mLengthTimerMasks[i];
							uint_fast8_t enableMask = 1 << i;

							
							if (regs[4] & 0x40) {						//ch_i has length timer on?
								if (!(lengthMask & ++regs[1])) {		//account for it and check for length overflow (end)
									regs[1] -= lengthMask + 1;			//fix the overflow we just caused
									hram[0x26] &=~ enableMask;			//off it goes

									//pr("ch %u off. NR52=0x%02x\n", i + 1, hram[0x26]);
								}
							}
						}
						break;
					case 7:
						//vol envelope
						mAudioStep = 0;
						break;
					default:
						//nothing;
						break;
				}

				mAudioCy += 2048 << (doubleSpeed ? 1 : 0);
			}
		}

		if(hram[0x40] & 0x80){	//LCD on? -> do the LCD things
			
			uint16_t since = cy - lcdCy;
			
			if (doubleSpeed)
				since /= 2;
			
			switch(hram[0x41] & 3){
				case 0:	//we're in H-blank
					if(since >= 51){
						lcdCy += 51 << (doubleSpeed ? 1 : 0);
						
						if(++hram[0x44] == 144){
							
							hram[0x0F] |= 0x01;				//V-blank int
		/* XXX */			if(hram[0x41] & 0x10) hram[0x0F] |= 0x02;	//raise int on entering mode 1, if requested
							hram[0x41] += 1;	//go to state 1
						}
						else{
						
							if(hram[0x41] & 0x20) hram[0x0F] |= 0x02;	//raise int on entering mode 2, if requested
							hram[0x41] += 2;	//go to state 2
						}
						
						hram[0x41] &= (uint8_t)~0x04;				//handle coincidence interrupt
						if(hram[0x44] == hram[0x45]){
							hram[0x41] |= 0x04;				//set status
							if(hram[0x41] & 0x40) hram[0x0F] |= 0x02;	//raise int
						}
					}
					break;
				
				case 1:	//we're in vblank
					if (since >= 114){
						lcdCy += 114 << (doubleSpeed ? 1 : 0);
						
						if(++hram[0x44] == 154){
							hram[0x44] = 0;
							hram[HRAM_INDEX_WINLINE] = 0;

							if(hram[0x41] & 0x20) hram[0x0F] |= 0x02;	//raise int on entering mode 2, if requested
							hram[0x41]++;		//go to state 2

						}

						hram[0x41] &= (uint8_t)~0x04;				//handle coincidence interrupt
						if(hram[0x44] == hram[0x45]){
							hram[0x41] |= 0x04;				//set status
							if(hram[0x41] & 0x40) hram[0x0F] |= 0x02;	//raise int
						}
					}
					break;
				
				case 2:	//drawing a line (reading OAM)
					if(since >= 20){
						lcdCy += 20 << (doubleSpeed ? 1 : 0);
						hram[0x41]++;
					}
					break;
				
				case 3:	//drawing a line (reading OAM & VRAM)
					if(since >= 43){
						lcdCy += 43 << (doubleSpeed ? 1 : 0);
						
						if(hram[0x44] < 144) gbDrawDisp(hram, hram[0x44], presentsAsCgb);
						
						hram[0x41] &= (uint8_t)~0x03;
						if(hram[0x41] & 0x08) hram[0x0F] |= 0x02;	//raise int on entering mode 0
						
						if(presentsAsCgb && !(hram[0x55] & 0x80)){
							
							uint16_t d = (((uint16_t)hram[0x53]) << 8) | hram[0x54];
							uint16_t s = (((uint16_t)hram[0x51]) << 8) | hram[0x52];
						
							d = (d & 0x1FF0) | 0x8000;
							s = s & 0xFFF0;
						
					//		fprintf(stderr,"HBDMA 0x10 %04X->%04X\n", s, d);
							t8 = 0x10;
							do{
							
								gbExtWrite(d++,gbExtRead(s++));
							
							}while(--t8);
							hram[0x51] = s >> 8;
							hram[0x52] = s;
							hram[0x53] = d >> 8;
							hram[0x54] = d;
							hram[0x55]--;	//automatically goes to 0xFF as needed when we're done...
						}
					}
					break;
			}
		}
		
		if(intsOn){
				
			t8 = (hram[0xFF] & hram[0x0F]);
			
			if(t8){
				
				if (intWait) {
					PC++;
					intWait = 0;
				}
				
				gbWrite(--SP, PC >> 8);
				gbWrite(--SP, PC);
				
				if(t8 & 0x01){		PC = 0x40; hram[0x0F] &= (uint8_t)~0x01;	}
				else if(t8 & 0x02){	PC = 0x48; hram[0x0F] &= (uint8_t)~0x02;	}
				else if(t8 & 0x04){	PC = 0x50; hram[0x0F] &= (uint8_t)~0x04;	}
				else if(t8 & 0x08){	PC = 0x58; hram[0x0F] &= (uint8_t)~0x08;	}
				else if(t8 & 0x10){	PC = 0x60; hram[0x0F] &= (uint8_t)~0x10;	}
				intsOnNext = 0;		//will be copied to intsOn right away
				cy += 5;
			}
		}
		intsOn = intsOnNext;
		
		instr = gbRead(PC++);
		LOG("[%04X] => %02X [a%02X b%02X c%02X d%02X e%02X h%02X l%02X SP%04X]\n", PC - 1, instr,A,B,C,D,E,H,L,SP);
		cy++;
		
		
		switch(instr){
			
			case 0x00:	//NOP
				break;
		
			STD_REG_OP1(0x00, B, C, BC, 0)
			
			case 0x07:	//RLCA
				fC = A >> 7;
				A = A + A + fC;
				fZ = 0;
				fN = 0;
				fH = 0;
				break;
			
			case 0x08:	//SP -> (immed)
				T = gbRead(PC++);
				T |= ((uint16_t)gbRead(PC++)) << 8;
				gbWrite(T++, SP);
				gbWrite(T, SP >> 8);
				cy += 4;
				break;
			
			STD_REG_OP2(0x00, B, C, BC, 0)
			
			case 0x0F:	//RRCA
				fC = A & 1;
				A >>= 1;
				A += ((uint8_t)fC) << 7;
				fZ = 0;
				fN = 0;
				fH = 0;
				break;
			
			case 0x10:	//STOP
				if (hram[0x4d]) {
					doubleSpeed = !doubleSpeed;
					hram[0x4d] = 0;
				}
				LOG("STOP\n");
				break;
			
			STD_REG_OP1(0x10, D, E, DE, 0)
			
			case 0x17:	//RLA
				T = (uint16_t)A + (uint16_t)A + fC;
				fC = T >> 8;
				A = T;
				fZ = 0;
				fN = 0;
				fH = 0;
				break;
			
			case 0x18:	//JR: PC += sign-extend(*PC++)
				cy++;
		do_short_jr:
				instr = gbRead(PC++);
				if(instr & 0x80){
					
					instr = -instr;
					PC -= instr;
				}
				else PC += instr;
				cy++;
				break;
			
			STD_REG_OP2(0x10, D, E, DE, 0)
		
			case 0x1F:	//RRA
				instr = fC;
				fC = A & 1;
				A >>= 1;
				A += instr << 7;
				fZ = 0;
				fN = 0;
				fH = 0;
				break;
			
			case 0x20:	//JR nz,
				cy++;
				if(fZ) PC++;
				else goto do_short_jr;
				break;
			
			STD_REG_OP1(0x20, H, L, HL, 1)
			
			case 0x27:	//DAA
				
				if (fN) {
					if (fC)
						A -= 0x60;
					if (fH)
						A -= 0x06;
				}
				else {
					if (fC || A > 0x99) {
						A += 0x60;
						fC = 1;
					}
					if (fH || (A & 0x0F) >= 10)
						A += 0x06;
				}
				fH = 0;
				fZ = !A;
				break;
			
			case 0x28:	//JR z,
				cy++;
				if(fZ) goto do_short_jr;
				else PC++;
				break;
			
			STD_REG_OP2(0x20, H, L, HL, 1)
			
			case 0x2F:	//CPL
				A ^= 0xFF;
				fN = 1;
				fH = 1;
				break;
			
			case 0x30:	//JR nc,
				cy++;
				if(fC) PC++;
				else goto do_short_jr;
				break;
			
			case 0x31:	//SP = immed
				cy += 2;
				SP = gbRead(PC++);
				SP |= ((uint16_t)gbRead(PC++)) << 8;
				break;
			
			case 0x32:	//(HL-) = A
				cy++;
				gbWrite(HL, A);
				if(!L--) H--;
				break;
			
			case 0x33:	//SP++;
				cy++;
				SP++;
				break;
			
			case 0x34:	//(HL)++
				cy += 2;
				t8 = gbRead(HL) + 1;
				fZ = !t8;
				fH = !(t8 & 0x0F);
				gbWrite(HL, t8);
				fN = 0;
				break;
			
			case 0x35:	//(HL)--
				cy += 2;
				t8 = gbRead(HL);
				fH = !(t8 & 0x0F);
				t8--;
				fZ = !t8;
				gbWrite(HL, t8);
				fN = 1;
				break;
			
			case 0x36:	//(HL) = immed
				cy += 2;
				gbWrite(HL, gbRead(PC++));
				break;
			
			case 0x37:	//carry = 1;
				fC = 1;
				fN = 0;
				fH = 0;
				break;
			
			case 0x38:	//JR c,
				cy++;
				if(fC) goto do_short_jr;
				else PC++;
				break;
			
			case 0x39:	//HL += SP
				fH = ((HL & 0x0fff) + (SP & 0x0fff)) >> 12;
				cy++;
				fN = 0;
				T = HL + SP;
				fC = T < SP;
				H = T >> 8;
				L = T;
				break;
			
			case 0x3A:	//A = (HL-)
				cy++;
				A = gbRead(HL);
				if (!L--) H--;
				break;
			
			case 0x3B:	//SP--
				cy++;
				SP--;
				break;
			
			case 0x3C:	//A++
				A++;
				fH = !(A & 0x0F);
				fZ = !A;
				fN = 0;
				break;
			
			case 0x3D:	//A--
				fH = !(A & 0x0F);
				A--;
				fZ = !A;
				fN = 1;
				break;
			
			case 0x3E:	//A = immed
				cy++;
				A = gbRead(PC++);
				break;
			
			case 0x3F:	//CCF
				fC = !fC;
				fN = 0;
				fH = 0;
				break;
			
			case 0x40 ... 0x75:	//moves
			case 0x77 ... 0x7F:
				
				//get data
				GET_REG_FROM_INSTR()
				
				//set data
				switch((instr >> 3) & 7){
					case 0: B = t8; break;
					case 1: C = t8; break;
					case 2: D = t8; break;
					case 3: E = t8; break;
					case 4: H = t8; break;
					case 5: L = t8; break;
					case 6: gbWrite(HL, t8); cy++; break;
					case 7: A = t8; break;
				}
				break;
			
			case 0x76:		//HALT
				
				if (hram[0x0F]) {
					intWait = 0;
					break;
				}
				intWait = 1;
				PC--; //re-execute this until int...
	//			LOG("HALT cy=0x%04X FF=0x%02X 0F=0x%02X\n", cy, hram[0xFF], hram[0x0F]);
				break;
			
			case 0x80 ... 0xBF:	//arithmetic
				
				//get data
				GET_REG_FROM_INSTR();
				
			do_alu_op:
			
				T = A;
				oldFC = fC;
				fC = 0;
				fZ = 0;
				//do op
				switch((instr >> 3) & 7){
					case 0: //ADD
						fH = ((A & 0x0F) + (t8 & 0x0F)) >> 4;
						T += t8;
						A = T;
						fC = T >> 8;
						fZ = !A;
						fN = 0;
						break;
					
					case 1:	//ADC
						fH = ((A & 0x0F) + (t8 & 0x0F) + oldFC) >> 4;
						T += t8 + oldFC;
						A = T;
						fC = T >> 8;
						fZ = !A;
						fN = 0;
						break;
					
					case 2:	//SUB
						fH = (A & 0x0F) < (t8 & 0x0F);
						T -= t8;
						A = T;
						fC = (T >> 8) & 1;
						fZ = !A;
						fN = 1;
						break;
					
					case 3:	//SBC
						fH = (A & 0x0F) < ((t8 & 0x0F) + oldFC);
						T -= t8 + oldFC;
						A = T;
						fC = (T >> 8) & 1;
						fZ = !A;
						fN = 1;
						break;
					
					case 4:	//AND
						A &= t8;
						fZ = !A;
						fN = 0;
						fH = 1;
						fC = 0;
						break;
					
					case 5:	//XOR
						A ^= t8;
						fZ = !A;
						fN = 0;
						fH = 0;
						fC = 0;
						break;
					
					case 6:	//OR
						A |= t8;
						fZ = !A;
						fN = 0;
						fH = 0;
						fC = 0;
						break;
					
					case 7:	//CP
						fH = (A & 0x0F) < (t8 & 0x0F);
						fC = A < t8;
						fZ = A == t8;
						fN = 1;
						break;
				}
				break;
			
			case 0xC0:	//RET nz
				cy++;
				if(fZ) break;
				goto do_ret;
			
			case 0xC1:	//POP BC
				DO_POP(B, C)
				break;
			
			case 0xC2:	//JP nz
				
				if(fZ){
		skip_2_by_2_cy:
					cy += 2;
					PC += 2;
					break;
				}
				//fallthrough
			
			case 0xC3:	//JP	
		do_jp:
				cy += 3;
				PC = gbRead(PC + 0) + (((uint16_t)gbRead(PC + 1)) << 8);
				break;
			
			case 0xC4:	//CALL nz
				if(fZ) goto skip_2_by_2_cy;
				goto do_call;
			
			case 0xC5:	//PUSH BC
				DO_PUSH(B, C)
				break;
			
			case 0xC6:	//ADD A, imm
			case 0xCE:	//ADC A, imm
			case 0xD6:	//SUB A, imm
			case 0xDE:	//SBC A, imm
			case 0xE6:	//AND A, imm
			case 0xEE:	//XOR A, imm
			case 0xF6:	//OR  A, imm
			case 0xFE:	//CP  A, imm
				t8 = gbRead(PC++);
				cy++;
				goto do_alu_op;
			
			case 0xC7:	//RST 0x00
			case 0xCF:	//RST 0x08
			case 0xD7:	//RST 0x10
			case 0xDF:	//RST 0x18
			case 0xE7:	//RST 0x20
			case 0xEF:	//RST 0x28
			case 0xF7:	//RST 0x30
			case 0xFF:	//RST 0x48
			
				T = instr - 0xC7;
				cy += 3;
				goto do_call_t;
			
			case 0xC8:	//RET z
				cy++;
				if(!fZ) break;
				//fallthrough
			
			case 0xC9:	//RET	PC.lo <- (SP);PC.hi <- (SP+1); SP <- (SP+2)
		do_ret:
				cy += 3;
				PC = gbRead(SP + 0) + (((uint16_t)gbRead(SP + 1)) << 8);
				SP += 2;
				//LOG("returning to 0x%04X with SP = %04X\n", PC, SP);
				break;
			
			case 0xCA:	//JP z
				
				if(fZ) goto do_jp;
				goto skip_2_by_2_cy;
			
			case 0xCB:	//CB prefix
				
				cy++;
				instr = gbRead(PC++);
				LOG(" -> CBxx => CB %02X\n", instr);
				
				//get data
				GET_REG_FROM_INSTR()
				
				//do stuff
				switch(instr >> 3){
					
					case 0:		//RLC
						fC = t8 >> 7;
						t8 = (t8 << 1) + fC;
						fZ = !t8;
						fN = 0;
						fH = 0;
						break;
					
					case 1:		//RRC
						fC = (t8 & 1);
						t8 >>= 1;
						t8 += ((uint8_t)fC) << 7;
						fZ = !t8;
						fN = 0;
						fH = 0;
						break;
					
					case 2:		//RL
						oldFC = fC;
						fC = t8 >> 7;
						t8 = (t8 << 1) + oldFC;
						fZ = !t8;
						fN = 0;
						fH = 0;
						break;
					
					case 3:		//RR
						oldFC = fC;
						fC = t8 & 1;
						t8 >>= 1;
						t8 += ((uint8_t)oldFC) << 7;
						fZ = !t8;
						fN = 0;
						fH = 0;
						break;
					
					case 4:		//SLA
						fC = t8 >> 7;
						t8 <<= 1;
						fZ = !t8;
						fN = 0;
						fH = 0;
						break;
					
					case 5:		//SRA
						fC = t8 & 1;
						t8 = ((int8_t)t8) >> 1;
						fZ = !t8;
						fN = 0;
						fH = 0;
						break;
					
					case 6:		//SWAP
						t8 = (t8 >> 4) | (t8 << 4);
						fZ = !t8;
						fC = 0;
						fN = 0;
						fH = 0;
						break;
					
					case 7:		//SRL
						fC = t8 & 1;
						t8 >>= 1;
						fZ = !t8;
						fN = 0;
						fH = 0;
						break;
					
					case 8:		//BIT 0
						fZ = !(t8 & 0x01);
						fN = 0;
						fH = 1;
						goto cb_no_wbak;
					
					
					case 9:		//BIT 1
						fZ = !(t8 & 0x02);
						fN = 0;
						fH = 1;
						goto cb_no_wbak;
					
					case 10:	//BIT 2
						fZ = !(t8 & 0x04);
						fN = 0;
						fH = 1;
						goto cb_no_wbak;
					
					case 11:	//BIT 3
						fZ = !(t8 & 0x08);
						fN = 0;
						fH = 1;
						goto cb_no_wbak;
					
					case 12:	//BIT 4
						fZ = !(t8 & 0x10);
						fN = 0;
						fH = 1;
						goto cb_no_wbak;
					
					case 13:	//BIT 5
						fZ = !(t8 & 0x20);
						fN = 0;
						fH = 1;
						goto cb_no_wbak;
					
					case 14:	//BIT 6
						fZ = !(t8 & 0x40);
						fN = 0;
						fH = 1;
						goto cb_no_wbak;
					
					case 15:	//BIT 7
						fZ = !(t8 & 0x80);
						fN = 0;
						fH = 1;
						goto cb_no_wbak;
					
					case 16:	//RES 0
						t8 &=~ (uint8_t)0x01;
						break;
					
					case 17:	//RES 1
						t8 &=~ (uint8_t)0x02;
						break;
					
					case 18:	//RES 2
						t8 &=~ (uint8_t)0x04;
						break;
					
					case 19:	//RES 3
						t8 &=~ (uint8_t)0x08;
						break;
					
					case 20:	//RES 4
						t8 &=~ (uint8_t)0x10;
						break;
					
					case 21:	//RES 5
						t8 &=~ (uint8_t)0x20;
						break;
					
					case 22:	//RES 6
						t8 &=~ (uint8_t)0x40;
						break;
					
					case 23:	//RES 7
						t8 &=~ (uint8_t)0x80;
						break;
					
					case 24:	//SET 0
						t8 |= (uint8_t)0x01;
						break;
					
					case 25:	//SET 1
						t8 |= (uint8_t)0x02;
						break;
					
					case 26:	//SET 2
						t8 |= (uint8_t)0x04;
						break;
					
					case 27:	//SET 3
						t8 |= (uint8_t)0x08;
						break;
					
					case 28:	//SET 4
						t8 |= (uint8_t)0x10;
						break;
					
					case 29:	//SET 5
						t8 |= (uint8_t)0x20;
						break;
					
					case 30:	//SET 6
						t8 |= (uint8_t)0x40;
						break;
					
					case 31:	//SET 7
						t8 |= (uint8_t)0x80;
						break;
				}
				
				//store data
				switch(instr & 7){
					case 0:	B = t8; break;
					case 1:	C = t8; break;
					case 2: D = t8; break;
					case 3: E = t8; break;
					case 4: H = t8; break;
					case 5: L = t8; break;
					case 6: gbWrite(HL, t8); cy++; break;
					case 7: A = t8; break;
				}
		cb_no_wbak:
				break;
			
			case 0xCC:	//CALL z
			
				if(!fZ) goto skip_2_by_2_cy;
				//fallthrough
			
			case 0xCD:	//CALL
		
		do_call:
				cy += 5;
				T = gbRead(PC + 0) + (((uint16_t)gbRead(PC + 1)) << 8);
				PC += 2;
			
		do_call_t:
				//LOG("calling to 0x%04X from 0x%04X with SP 0x%04X\n", T, PC, SP);
				gbWrite(--SP, PC >> 8);
				gbWrite(--SP, PC);
				PC = T;
				break;
			
			case 0xD0:	//RET nc
				cy++;
				if(fC) break;
				goto do_ret;
			
			case 0xD1:	//POP DE
				DO_POP(D, E);
				break;
			
			case 0xD2:	//JP nc
				if(fC) goto skip_2_by_2_cy;
				goto do_jp;
			
			case 0xD4:	//CALL nc
				if(fC) goto skip_2_by_2_cy;
				goto do_call;
			
			case 0xD5:	//PUSH DE
				DO_PUSH(D, E);
				break;
			
			case 0xD8:	//RET c
				cy++;
				if(fC) goto do_ret;
				break;
			
			case 0xD9:	//RETI
				intsOnNext = 1;
				goto do_ret;
			
			case 0xDA:	//JP c
				if(fC) goto do_jp;
				goto skip_2_by_2_cy;
			
			case 0xDC:	//CALL c
				
				if(fC) goto do_call;
				goto skip_2_by_2_cy;
			
			case 0xE0:	//(FF00 + imm) = A
				cy += 2;
				gbIoWrite(gbRead(PC++), A);
				break;
			
			case 0xE1:	//POP HL
				DO_POP(H, L);
				break;
			
			case 0xE2:	//(FF00 + C) = A
				cy++;
				gbIoWrite(C, A);
				break;
			
			case 0xE5:	//PUSH HL
				DO_PUSH(H, L);
				break;
			
			case 0xE8:	//SP += imm		//https://stackoverflow.com/questions/5159603/gbz80-how-does-ld-hl-spe-affect-h-and-c-flags
				T = (uint16_t)(int16_t)(int8_t)gbRead(PC++);
				fH = ((SP & 0x0f) + (T & 0x0f)) >> 4;
				fC = ((SP & 0xff) + (T & 0xff)) >> 8;
				SP += T;
				fN = 0;
				fZ = 0;
				cy += 3;
				break;

			case 0xE9:	//JP HL
				PC = HL;
				break;
			
			case 0xEA:	//(imm16) = A
				gbWrite(gbRead(PC + 0) + (((uint16_t)gbRead(PC + 1)) << 8), A);
				cy += 3;
				PC += 2;
				break;
			
			case 0xF0:	//A = (FF00 + imm)
				cy += 2;
				A = gbIoRead(gbRead(PC++));
				break;
			
			case 0xF1:	//POP AF
				DO_POP(A, t8);
				fZ = !!(t8 & 0x80);
				fN = !!(t8 & 0x40);
				fH = !!(t8 & 0x20);
				fC = !!(t8 & 0x10);
				break;
			
			case 0xF2:	//A = (FF00 + C)
				cy++;
				A = gbIoRead(C);
				break;
			
			case 0xF3:	//DI
				intsOnNext = 0;
				break;
			
			case 0xF5:	//PUSH AF
				t8 = 0;
				if(fZ) t8 |= 0x80;
				if(fN) t8 |= 0x40;
				if(fH) t8 |= 0x20;
				if(fC) t8 |= 0x10;
				DO_PUSH(A, t8);
				break;
			
			case 0xF8:	//HL = SP + imm
				T = (uint16_t)(int16_t)(int8_t)gbRead(PC++);
				fH = ((SP & 0x0f) + (T & 0x0f)) >> 4;
				fC = ((SP & 0xff) + (T & 0xff)) >> 8;
				T += SP;
				fN = 0;
				fZ = 0;
				cy += 2;
				H = T >> 8;
				L = T;
				break;
			
			case 0xF9:	//SP = HL;
				SP = HL;
				cy++;
				break;
			
			case 0xFA:	//A = (imm16)
				A = gbRead(gbRead(PC + 0) + (((uint16_t)gbRead(PC + 1)) << 8));
				cy += 3;
				PC += 2;
				break;
			
			case 0xFB:	//EI
				intsOnNext = 1;
				break;
			
			default:
				//TODO: invalid opcode
				LOG("unknown opcode %02X at 0x%04X (A=%02X fC=%d)\n", instr, PC - 1, A, fC);
				while(1);
				break;
		}
	}
}
/*
struct GbAudioChannel {
	uint32_t nextClock;
	union {
		uint16_t curDutyTick;	//for ch0..2
		uint16_t lfsr;			//fior ch3;
	};
	uint8_t curVol;			//should 1-init
	int8_t curOutput;
};

static struct GbAudioChannel mAudioChannels[4];

static uint32_t gbAudioRecalcDownCounterVal(uint_fast8_t chIdx)
{
	uint8_t *hram = mHRAM, *chRegs = hram + 0x10 + 5 * chIdx;;
	uint32_t freq;
	
	if (chIdx < 3)
		freq = 2048 - (chRegs[3] + (((uint_fast16_t)(chRegs[4] & 0x07)) << 8));	//every this many "cy" we step to the next waveform value
	else {
		static const uint8_t rs[] = {2, 4, 8, 12, 16, 20, 24, 28};
		uint_fast8_t cfg = hram[0x22];
		
		freq = ((uint32_t)(rs[cfg] & 3)) << (cfg >> 4);
	}
	
	return freq;
}

static int8_t gbAudioPrvRun(uint_fast8_t chIdx)
{
	uint8_t *hram = mHRAM, *chRegs = hram + 0x10 + 5 * chIdx;;
	struct GbAudioChannel *ch = mAudioChannels + chIdx;
	
	if (gbGetCy() >= ch->nextClock) {
		
		ch->nextClock += gbAudioRecalcDownCounterVal(chIdx);
		
		if (chIdx < 2) {	//square channel
			
			static const uint8_t waveform[] = {0xfe, 0x7e, 0x17, 0x81};
			bool high;
			
			if (!(ch->curDutyTick >>= 1))
				ch->curDutyTick = 0x80;
			
			high = !!(waveform[chRegs[1] >> 6] & ch->curDutyTick);
			
			if (high)
				ch->curOutput = ch->curVol;
			else
				ch->curOutput = -ch->curVol;
		}
		else if (chIdx == 3) {	//waveform channel
			
			int_fast8_t val = hram[0x30 + ch->curDutyTick / 2];
			
			if (ch->curDutyTick % 2)
				val &= 0x0f;
			else
				val = ((uint_fast8_t)val) >> 4;
			
			val -= 8;
			val *= ch->curVol;
			ch->curOutput = val * 2;
		}
		else {					//noise channel
			
			uint_fast16_t feedbackMask = (hram[0x43] & 0x08) ? 0x4040 : 0x4000;
			bool out;
			
			switch (ch->lfsr % 4) {
				case 0:
					out = true;
					ch->lfsr = (ch->lfsr >> 1) | feedbackMask;
					break;
				
				case 1:
					out = false;
					ch->lfsr = (ch->lfsr >> 1) &~ feedbackMask;
					break;
				
				case 2:
					out = true;
					ch->lfsr >>= 1;
					ch->lfsr = (ch->lfsr >> 1) &~ feedbackMask;
					break;
				
				case 3:
					out = false;
					ch->lfsr = (ch->lfsr >> 1) | feedbackMask;
					break;
			}
			
			if (out)
				ch->curOutput = ch->curVol;
			else
				ch->curOutput = -ch->curVol;
		}
	}
	
	return ch->curOutput;
}

void gbAudioTrigger(uint_fast8_t ch)
{
	//noise On a trigger event, all the bits of LFSR are set to 1.
}
*/
