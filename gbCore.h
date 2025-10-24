#ifndef _GB_CORE_H_
#define _GB_CORE_H_

#define HRAM_INDEX_WINLINE	256
#define HRAM_INDEX_BG_PAL	258	//and the 64 bytes after it too
#define HRAM_INDEX_OB_PAL	322	//and the 64 bytes after it too
#define HRAM_SIZE			386

#define BG_FLAG_UNDER_OBJS	0x20	//lower bit of green in rgb565

#if __thumb__

	#if defined(__ARM_ARCH_4T__)
		#define THUMB_VER	1
	#elif defined(__ARM_ARCH_5T__) || defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5TEJ__)
		#define THUMB_VER	2
	#elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_8M_BASE__)
		#define THUMB_VER	3
	#elif defined(__ARM_ARCH_6T2__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
		#define THUMB_VER	4
	#elif defined(__TARGET_ARCH_THUMB)
		#define THUMB_VER	__TARGET_ARCH_THUMB
	#else
		#define THUMB_VER	0
	#endif

#endif

#ifdef ASSEMBLY

	//FDPIC will be defined if we have to keep r10 free and operate in FDPIC mode. this is in theory orthogonal to the rest of this
	//but it will never be needed in ARMv7M mode for example
	


	#if __thumb__

		
		#if THUMB_VER < 3
			.macro uxth dst, src
				lsls	\dst, \src, #16
				lsrs	\dst, \dst, #16
			.endm
	
			.macro sxth dst, src
				lsls	\dst, \src, #16
				asrs	\dst, \dst, #16
			.endm
	
			.macro uxtb dst, src
				lsls	\dst, \src, #24
				lsrs	\dst, \dst, #24
			.endm
	
			.macro sxtb dst, src
				lsls	\dst, \src, #24
				asrs	\dst, \dst, #24
			.endm
		#endif


		#if THUMB_VER < 4
			.macro cbz	reg, lbl
				cmp		\reg, #0
				beq		\lbl
			.endm

			.macro cbnz	reg, lbl
				cmp		\reg, #0
				bne		\lbl
			.endm

			.macro jtab	jp_name, reg, rTmp
				lsls	\rTmp, \reg, #2
				add		pc, \rTmp
				nop
			.endm

			.macro jpEntry jp_name, target
				bl		\target
			.endm

			.macro jtSP1c	jp_name, reg		//jtSmallPreshifted1c (cases are evens, low bit guaranteed clear)
				add		pc, \reg
				nop
			.endm

			.macro jtsP1case	jp_name, target		//jtSmallPreshifted1c
				b		\target
			.endm

			.macro jtsP1LFC	jp_name	, target	//jtSmallPreshifted1cFallthrough	//if code for last case follows jumptable
				//fallthrough
			.endm

			.macro jts	jp_name, reg, tmpReg		//jtSmall
				lsls	\tmpReg, \reg, #1
				add		pc, \reg
				nop
			.endm

			.macro jtsCase	jp_name, target			//jtSmallCase
				b		\target
			.endm

			.macro jtsLFC	jp_name	, target		//jtSmallFallthrough	//if code for last case follows jumptable
				//fallthrough
			.endm


		#else

			.macro jtab	jp_name, reg, rTmp
				tbh		[PC, \reg, lsl #1]
				_lbl_\jp_name:
			.endm

			.macro jpEntry jp_name, target
				.hword (\target - _lbl_\jp_name) / 2
			.endm

			.macro jtSP1c	jp_name, reg		//jtSmallPreshifted1c (cases are evens, low bit guaranteed clear)
				tbb		[PC, \reg]
				_lbl_\jp_name:
			.endm

			.macro jtsP1case	jp_name, target		//jtSmallPreshifted1c
				.byte (\target - _lbl_\jp_name) / 2
				.byte (\target - _lbl_\jp_name) / 2
			.endm

			.macro jtsP1LFC	jp_name	, target		//jtSmallPreshifted1cFallthrough	//if code for last case follows jumptable
				.byte (_lbl_\jp_name\()_last - _lbl_\jp_name) / 2
				.byte (_lbl_\jp_name\()_last - _lbl_\jp_name) / 2
			_lbl_\jp_name\()_last:
			.endm

			.macro jts	jp_name, reg, tmpReg		//jtSmall
				tbb		[PC, \reg]
				_lbl_\jp_name:
			.endm

			.macro jtsCase	jp_name, target			//jtSmallCase
				.byte (\target - _lbl_\jp_name) / 2
			.endm

			.macro jtsLFC	jp_name	, target		//jtSmallFallthrough	//if code for last case follows jumptable
				.byte (_lbl_\jp_name\()_last - _lbl_\jp_name) / 2
			_lbl_\jp_name\()_last:
			.endm

			#define JT_CANNOT_BRANCH_BACKWARDS

		#endif

	#else			//big arm
	
		.macro cbz	reg, lbl
			cmp		\reg, #0
			beq		\lbl
		.endm

		.macro cbnz	reg, lbl
			cmp		\reg, #0
			bne		\lbl
		.endm
		
		.macro uxth dst, src
			lsls	\dst, \src, #16
			lsrs	\dst, \dst, #16
		.endm
	
		.macro sxth dst, src
			lsls	\dst, \src, #16
			asrs	\dst, \dst, #16
		.endm
	
		.macro uxtb dst, src
			and		\dst, \src, #0xff
		.endm
	
		.macro sxtb dst, src
			lsls	\dst, \src, #24
			asrs	\dst, \dst, #24
		.endm

		.macro jtab	jp_name, reg, rTmp
			add 	pc, pc, \reg, lsl #2
			nop
		.endm

		.macro jpEntry jp_name, target
			b		\target
		.endm


		.macro jtSP1c	jp_name, reg		//jtSmallPreshifted1
			add 	pc, pc, \reg, lsl #1
			nop
		.endm

		.macro jtsCase	jp_name, target		//jtSmallCase
			b		\target
		.endm

		.macro jtsLFC	jp_name, target		//jtSmallCaseLastFallthrough	//if code for last case follows jumptable
			//fallthrough
		.endm


	#endif



		
	#ifdef FDPIC
	

		.macro ldadrD	reg, tmp, ramThing			//needs a temp reg and r10
			ldr	\reg, =\ramThing
			ldr	\tmp, =__got_start
			subs \reg, \tmp
			add \reg, r10
		.endm
	
		.macro ldadrT	reg, tmp, ramThing			//needs a temp reg and r10
			ldr		\reg, = \ramThing
			ldr		\tmp, =77f
			add		\reg, pc
			subs	\reg, \tmp
		77:
		.endm

		#define r10	UNUSABLEREG

	#else
	
		.macro ldadrD	reg, tmp, ramThing			//.data/.bss
			ldr	\reg, =\ramThing
		.endm

		.macro ldadrT	reg, tmp, ramThing			//.text
			ldr	\reg, =\ramThing
		.endm
	
	#endif



#else
	//now that we share palettes with the PPU, this needs to be here
		
	#include <stdbool.h>
	#include <stdint.h>
	#include "gb.h"

	void gbLcdCySetToCy(void);
	void gbTmrCySetToCy(void);
	void gbAudioCySetToCy(void);
	uint32_t gbGetCy(void);
	bool gbIsDoubleSpeed(void);
	
	//external
	void gbPoll(void);
	void gbDrawDisp(uint8_t* hram, uint8_t lineNum, bool presentsAsCgb);
	
	
	uint8_t gbIoRead(uint16_t addr);						//only lower 8 bits of addr matter
	bool gbIoWrite(uint16_t addr, uint8_t val);			//only lower 8 bits of addr matter

	typedef uint16_t CachedCgbPal[8][4];
	typedef uint16_t CachedDmgPal[4];

	extern CachedCgbPal cgbBgPals;	//in our RGXB5515 format	[8][4] for CGB, [1][4] for DMG
	extern CachedCgbPal cgbFgPals;	//in our RGXB5515 format	[8][4] for CGB, [2][4] for DMG

	extern CachedDmgPal dmgBgPal;
	extern CachedDmgPal dmgFgPals[2];

	extern uint8_t mOAM[], mVRAM[], mHRAM[];
	
#endif

#endif
