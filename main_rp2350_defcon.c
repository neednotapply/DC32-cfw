#include <string.h>
#include <stdarg.h>
#include "pinoutRp2350defcon.h"
#include "gbCartHeader.h"
#include "pioIrdaSIR.h"
#include "dispDefcon.h"
#include "pioWS2812.h"
#include "frontend.h"
#include "timebase.h"
#include "settings.h"
#include "pioI2C.h"
#include "gbCore.h"
#include "memMap.h"
#include "printf.h"
#include "sleep.h"
#include "2350.h"
#include "qspi.h"
#include "mbc.h"
#include "gb.h"
#include "ui.h"

static void uiPrvUpscalerInit(void);
static void uiPrvUpscalerDeinit(void);
static bool shouldUpscale(void);
static bool mUpscaling;

static uint64_t mRtcTickOffset;		//ticks to add to our ticks to represent RTC
static volatile uint8_t mDefconExtraIoData[3];
static uint8_t mI2CreadBuf[2];
static int8_t mEEpos = 0;
static uint16_t mIrRxBuf[64];
static uint8_t mIrRxWritePos, mIrRxBytesUsed, mIrRxHadOverrun;
static bool mPrevIrdaModeWasTx;

#define ACCEL_I2C_ADDR			0x18
#define TOUCH_I2C_ADDR			0x48
#define RTC_I2C_ADDR			0x51




#define AUDIO_PWM_IDX		4


static void doFreq(uint32_t freq)
{
	uint32_t duty, clkDiv = 32, baseFreq = TICKS_PER_SECOND / clkDiv;

	//divider is 8 bits, counter is 16
	//with no division, min freq possible is 2.3KHz
	//widh division by 32, min freq possible is 71Hz

	pwm_hw->slice[AUDIO_PWM_IDX].cc = 0;	//wait for it to go low now
	while (sio_hw->gpio_in & (1 << PIN_SPQR));
	pwm_hw->slice[AUDIO_PWM_IDX].csr &=~ PWM_CH0_CSR_EN_BITS;

	duty = (baseFreq + freq / 2) / freq;
	if (duty < 2)
		duty = 2;

	while (pwm_hw->slice[AUDIO_PWM_IDX].csr & PWM_CH0_CSR_EN_BITS);
	
	if (freq) {
		pwm_hw->slice[AUDIO_PWM_IDX].top = (pwm_hw->slice[AUDIO_PWM_IDX].top &~ PWM_CH0_TOP_BITS) | ((duty - 1) << PWM_CH1_TOP_LSB);		//this is correct. valus valis for "cc" become 0..8191 to span the range of 0..100% duty
		pwm_hw->slice[AUDIO_PWM_IDX].ctr = 0;
		*(volatile uint16_t*)&pwm_hw->slice[AUDIO_PWM_IDX].cc = duty / 2;
		pwm_hw->slice[AUDIO_PWM_IDX].csr = (pwm_hw->slice[AUDIO_PWM_IDX].csr &~ (PWM_CH0_CSR_PH_ADV_BITS | PWM_CH0_CSR_PH_RET_BITS | PWM_CH0_CSR_DIVMODE_BITS | PWM_CH0_CSR_B_INV_BITS)) | (PWM_CH0_CSR_DIVMODE_VALUE_DIV << PWM_CH0_CSR_DIVMODE_LSB) | PWM_CH0_CSR_A_INV_BITS;
		pwm_hw->slice[AUDIO_PWM_IDX].div = (pwm_hw->slice[AUDIO_PWM_IDX].div &~ (PWM_CH0_DIV_INT_BITS | PWM_CH0_DIV_FRAC_BITS)) | (32 << PWM_CH0_DIV_INT_LSB);
		pwm_hw->slice[AUDIO_PWM_IDX].csr |= PWM_CH0_CSR_EN_BITS;
	}
}

static void note(uint32_t freq, uint32_t dur)
{
	doFreq(freq);
	delayMsec(dur);
}

#define NOTE_LENGTH			2048	//msec
#define FULL				(NOTE_LENGTH / 1)
#define HALF				(NOTE_LENGTH / 2)
#define QUARTER				(NOTE_LENGTH / 4)
#define EIGHTH				(NOTE_LENGTH / 8)
#define SIXTEENTH			(NOTE_LENGTH / 16)


#define BASE_A				440

#define G					(unsigned)(BASE_A /  1.0595 / 1.0595)
#define Gs					(unsigned)(BASE_A /  1.0595)
#define A					(unsigned)(BASE_A)
#define As					(unsigned)(BASE_A * 1.0595)
#define B					(unsigned)(BASE_A * 1.0595 * 1.0595)
#define C					(unsigned)(BASE_A * 1.0595 * 1.0595 * 1.0595)
#define Cs					(unsigned)(BASE_A * 1.0595 * 1.0595 * 1.0595 * 1.0595)
#define D					(unsigned)(BASE_A * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595)
#define Ds					(unsigned)(BASE_A * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595)
#define E					(unsigned)(BASE_A * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595)
#define F					(unsigned)(BASE_A * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595)
#define Fs					(unsigned)(BASE_A * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595)


static void rollMusic(void)
{
	static const uint16_t music[] = {
		G, SIXTEENTH,
		A, SIXTEENTH,
		C, SIXTEENTH,
		A, SIXTEENTH,
		E, EIGHTH,
		0, SIXTEENTH,
		E, SIXTEENTH,
		0, EIGHTH,
		D, QUARTER,
		0, SIXTEENTH,
		G, SIXTEENTH,
		A, SIXTEENTH,
		C, SIXTEENTH,
		A, SIXTEENTH,
		D, EIGHTH,
		0, SIXTEENTH,
		D, SIXTEENTH,
		0, EIGHTH,
		C, SIXTEENTH,
		0, SIXTEENTH,
		B, SIXTEENTH,
		A, SIXTEENTH,
		0, SIXTEENTH,
		G, SIXTEENTH,
		A, SIXTEENTH,
		C, SIXTEENTH,
		A, SIXTEENTH,
		C, EIGHTH + SIXTEENTH,
		0, SIXTEENTH,
		D, SIXTEENTH,
		0, SIXTEENTH,
		B, EIGHTH + SIXTEENTH,
		A, SIXTEENTH,
		G, EIGHTH,
		0, EIGHTH,
		G, SIXTEENTH,
		0, EIGHTH,
		D, EIGHTH,
		C, QUARTER + EIGHTH,
	};
	uint32_t i;

	for (i = 0; i < sizeof(music) / sizeof(*music); i += 2) {
		note(music[i + 0], music[i + 1]);
	}
	doFreq(0);
}



void accessFail(uint16_t addr, char wr, uint8_t val)
{
	if (wr)
		pr("W ACCESS Fail to 0x%04x with val 0x%02x\n", addr, val);
	else
		pr("R ACCESS Fail to 0x%04x\n", addr);
}

static uint_fast8_t prvKeysMap(uint32_t sta)
{
	uint_fast8_t ret = 0;
	
	if (!(sta & (1 << PIN_BTN_U)))		ret |= KEY_BIT_UP;
	if (!(sta & (1 << PIN_BTN_D)))		ret |= KEY_BIT_DOWN;
	if (!(sta & (1 << PIN_BTN_L)))		ret |= KEY_BIT_LEFT;
	if (!(sta & (1 << PIN_BTN_R)))		ret |= KEY_BIT_RIGHT;
	
	if (!(sta & (1 << PIN_BTN_A)))		ret |= KEY_BIT_A;
	if (!(sta & (1 << PIN_BTN_B)))		ret |= KEY_BIT_B;
	if (!(sta & (1 << PIN_BTN_START)))	ret |= KEY_BIT_START;
	if (!(sta & (1 << PIN_BTN_SEL)))	ret |= KEY_BIT_SEL;
		
	return ret;
}

uint_fast8_t uiGetKeys(void)
{
	uint32_t val, count = 0, countUntil = 10000, ourKeysMask = (1 << PIN_BTN_U) | (1 << PIN_BTN_D) | (1 << PIN_BTN_L) | (1 << PIN_BTN_R) | (1 << PIN_BTN_START) | (1 << PIN_BTN_SEL) | (1 << PIN_BTN_A) | (1 << PIN_BTN_B) | (1 << PIN_BTN_CENTER);

	while(1) {
		val = sio_hw->gpio_in & ourKeysMask;
		for (count = 0; count < countUntil && val == (sio_hw->gpio_in & ourKeysMask); count++);
		if (count == countUntil)
			return prvKeysMap(val);
	}
}

static void exitGame(void)
{
	if (mUpscaling)
			uiPrvUpscalerDeinit();
	uiInGame();
	memset(dispGetFb(), 0, DISP_WIDTH * DISP_HEIGHT * DISP_BPP / 8);
	mUpscaling = shouldUpscale();
	if (mUpscaling)
		uiPrvUpscalerInit();
}

uint8_t gbExtGetKeys(void)	//arrow keys, f1=a, f2=b, f3=start, f4=select
{
	uint32_t sta = sio_hw->gpio_in;
	
	if (!(sta & (1 << PIN_BTN_CENTER))) {
		
		exitGame();
	}

	if (mEEpos < 0) {

		rollMusic();
		mEEpos = 0;
	}

	return prvKeysMap(sta);
}


void prPutchar(char chr)
{
	#if (ZWT_ADDR & 3)		//byte
	
		*(volatile uint8_t*)ZWT_ADDR = chr;
				
	#else
	
	volatile uint32_t *addr = (volatile uint32_t*)ZWT_ADDR;
	uint32_t counter = 0;
	
	while (addr[0] & 0x80000000ul) {
		if (++counter > 1000000)
			break;
	}
	addr[0] = 0x80000000ul | (uint8_t)chr;
	#endif
}

static uint16_t colorToRgb565(uint32_t r, uint32_t g, uint32_t b)
{
	#if defined(THUMB_VER) && defined(__ARM_FEATURE_SIMD32) && THUMB_VER >= 4

		asm("smulwb %0, %1, %2":"=r"(r):"r"(r),"r"(7967));
		asm("smulwb %0, %1, %2":"=r"(g):"r"(g),"r"(16191));
		asm("smulwb %0, %1, %2":"=r"(b):"r"(b),"r"(7967));

	#else

		r = (r * 7967 + 32768) >> 16;
		g = (g * 16191 + 32768) >> 16;
		b = (b * 7967 + 32768) >> 16;

	#endif

	return (r << 11) + (g << 5) + b;
}

static void uiPrvFifoDump(void)
{
	while (sio_hw->fifo_st & SIO_FIFO_ST_VLD_BITS)
		(void)sio_hw->fifo_rd;
}

static void uiPrvFifoTx(uint32_t val)
{
	while(!(sio_hw->fifo_st & SIO_FIFO_ST_RDY_BITS));
	sio_hw->fifo_wr = val;
	asm volatile("sev");
}

static uint32_t uiPrvFifoRx(void)
{
	while (!(sio_hw->fifo_st & SIO_FIFO_ST_VLD_BITS));
	return sio_hw->fifo_rd;
}

static void __attribute__((naked)) uiPrvHorizStretch(uint32_t *dst, uint16_t *src)
{
	asm volatile(
		"	push	{r4-r8, lr}			\n\t"
		"	mov		r8, 0x00f800f8		\n\t"
		".rept 80						\n\t"
		"	ldmia	r1!, {r4}			\n\t"
		"	and		r2, r8, r4, lsr	#8	\n\t"	//r
		"	and		r3, r8, r4, lsr #3	\n\t"	//g
		"	and		r4, r8, r4, lsl #3	\n\t"	//b

		"	add		r2, r2, r2, lsr #5	\n\t"	//extend R to 8 bits
		"	add		r3, r3, r3, lsr #5	\n\t"	//extend G to 8 bits
		"	add		r4, r4, r4, lsr #5	\n\t"	//extend B to 8 bits
	
		"	uhsax	r5, r2, r2			\n\t"	//middle.r
		"	uhsax	r6, r3, r3			\n\t"	//middle.r
		"	uhsax	r7, r4, r4			\n\t"	//middle.r
		
		"	strb	r5,	[r0, #4]		\n\t"
		"	strb	r6,	[r0, #5]		\n\t"
		"	strb	r7,	[r0, #6]		\n\t"

		"	lsrs	r6, r2, #16			\n\t"
		"	lsrs	r7, r3,	#16			\n\t"

		"	str		r4, [r0, #8]		\n\t"
		"	strb	r6, [r0, #8]		\n\t"
		"	strb	r7, [r0, #9]		\n\t"

		"	strb	r4,	[r0, #2]		\n\t"
		"	strb	r3,	[r0, #1]		\n\t"
		"	strb	r2,	[r0], #12		\n\t"

		".endr							\n\t"
		"	pop		{r4-r8, pc}			\n\t"
		:
		:
		:"memory","cc"
	);
}

static void __attribute__((naked)) uiPrvMix(uint32_t *dst, uint32_t *src)
{
	asm volatile(
		"	push	{r4-r8, lr}			\n\t"
		"	mov		r12, 240 / 4		\n\t"
		"1:								\n\t"
		"	ldrd	r6, r7, [r0, #0]	\n\t"
		"	ldrd	r8, lr, [r0, #8]	\n\t"
		"	ldmia	r1!, {r2-r5}		\n\t"
		"	uhadd8	r2, r2, r6			\n\t"
		"	uhadd8	r3, r3, r7			\n\t"
		"	uhadd8	r4, r4, r8			\n\t"
		"	uhadd8	r5, r5, lr			\n\t"
		"	stmia	r0!, {r2-r5}		\n\t"
		"	subs	r12, #1				\n\t"
		"	bne		1b					\n\t"
		"	pop		{r4-r8, pc}			\n\t"
		:
		:
		:"memory","cc"
	);
}

static uint16_t* uiPrvOuputStretchedOnly(uint16_t *fb, const uint32_t *data)
{
	uint32_t i;

	for (i = 0; i < 240; i++) {

		uint32_t dummy1, dummy2, ret;

		asm(
			"	ldmia	%3!, {%2}			\n\t"
			"	uxtb	%0, %2				\n\t"
			"	smulwb	%0, %0, %4			\n\t"	//r
			"	ubfx	%1, %2, #8, #8		\n\t"
			"	smulwb	%1, %1, %5			\n\t"	//g
			"	ubfx	%2, %2, #16, #8		\n\t"
			"	smulwb	%2, %2, %4			\n\t"	//g
			"	add		%2, %2, %1, lsl #5	\n\t"
			"	add		%2, %2, %0, lsl #11	\n\t"
			:"=&l"(dummy1), "=&r"(dummy2), "=l"(ret), "+l"(data)
			:"r"(7967), "r"(16191)
		);

		*fb = ret;

		#ifdef UPSCALER_ROTATES
			fb += DISP_WIDTH;
		#else
			fb++;
		#endif
	}

	#ifdef UPSCALER_ROTATES
		fb -= DISP_WIDTH * 240;
		fb--;
	#endif

	return fb;
}

static uint16_t* uiPrvOuputStretchedWithSource(uint16_t *fb, const uint32_t *data, const uint16_t *src)
{
	uint32_t i;

	for (i = 0; i < 240; i += 3, data += 3, src += 2) {

		uint32_t dummy1, dummy2, ret;

		asm(
			"	uxtb	%0, %3				\n\t"
			"	smulwb	%0, %0, %4			\n\t"	//r
			"	ubfx	%1, %3, #8, #8		\n\t"
			"	smulwb	%1, %1, %5			\n\t"	//g
			"	ubfx	%2, %3, #16, #8		\n\t"
			"	smulwb	%2, %2, %4			\n\t"	//g
			"	add		%2, %2, %1, lsl #5	\n\t"
			"	add		%2, %2, %0, lsl #11	\n\t"
			:"=&l"(dummy1), "=&r"(dummy2), "=&l"(ret)
			:"l"(data[1]), "r"(7967), "r"(16191)
		);

		*fb = src[0] &~ BG_FLAG_UNDER_OBJS;

		#ifdef UPSCALER_ROTATES
			fb += DISP_WIDTH;
		#else
			fb++;
		#endif

		*fb = ret;

		#ifdef UPSCALER_ROTATES
			fb += DISP_WIDTH;
		#else
			fb++;
		#endif

		*fb = src[1] &~ BG_FLAG_UNDER_OBJS;

		#ifdef UPSCALER_ROTATES
			fb += DISP_WIDTH;
		#else
			fb++;
		#endif
	}

	#ifdef UPSCALER_ROTATES
		fb -= DISP_WIDTH * 240;
		fb--;
	#endif

	return fb;
}

static void uiPrvUpscalerMain(void)
{
	asm volatile("cpsid i");
	
	while (1) {
		
		static uint32_t lines[2][240], prevLineIdx = 0;
		static PIXFMT __attribute__((aligned(4))) pixelsIn[160];
		uint32_t *pixTwice = (uint32_t*)pixelsIn;
		uint16_t *fb = dispGetFb();
		uint32_t *dataIn, lineNum;
		uint_fast8_t i, j;
		PIXFMT* pixels;
		
		dataIn = (uint32_t*)uiPrvFifoRx();
		
		memcpy(pixelsIn, (PIXFMT*)dataIn[0], sizeof(pixelsIn));
		lineNum = dataIn[1];
		
		uiPrvFifoTx(0);

	#ifdef UPSCALER_ROTATES
		fb += DISP_WIDTH - 1 - (lineNum * 3 / 2);
	#else
		fb += DISP_WIDTH * (lineNum * 3 / 2);
	#endif

		if (lineNum & 1) {		//mix and output mix

			uiPrvHorizStretch(lines[1], pixelsIn);
			uiPrvMix(lines[0], lines[1]);
			fb = uiPrvOuputStretchedOnly(fb, lines[0]);
			fb = uiPrvOuputStretchedWithSource(fb, lines[1], pixelsIn);
		}
		else {
			uiPrvHorizStretch(lines[0], pixelsIn);
			fb = uiPrvOuputStretchedWithSource(fb, lines[0], pixelsIn);
		}
	}
}

static void uiPrvUpscalerInit(void)
{
	static uint32_t mUpscalerStack[32];
	uint32_t cmds[] = {0, 0, 1, SCB->VTOR, ((uintptr_t)mUpscalerStack) + sizeof(mUpscalerStack), (uintptr_t)&uiPrvUpscalerMain};
	uint32_t idx = 0;
	
	//first, reset the core
	psm_hw->frce_off |= PSM_FRCE_OFF_PROC1_BITS;
	while (!(psm_hw->frce_off & PSM_FRCE_OFF_PROC1_BITS));
	psm_hw->frce_off &=~ PSM_FRCE_OFF_PROC1_BITS;
	while (psm_hw->frce_off & PSM_FRCE_OFF_PROC1_BITS);
	
	
	//then bring it up to our code
	do {
		uint32_t tx = cmds[idx];
		if (idx < 2) {
			uiPrvFifoDump();
			asm volatile("sev");
		}
		uiPrvFifoTx(tx);
		idx = (uiPrvFifoRx() == tx) ? idx + 1 : 0;
	} while (idx < sizeof(cmds) / sizeof(*cmds));
}

static void uiPrvUpscalerDeinit(void)
{
	psm_hw->frce_off |= PSM_FRCE_OFF_PROC1_BITS;
	while (!(psm_hw->frce_off & PSM_FRCE_OFF_PROC1_BITS));
}

void gbDrawLine(uint8_t lineNum, PIXFMT* pixels)
{
	uint16_t *fb = dispGetFb();
	
	if (!lineNum)
		dispPrvFrameCtrWait();


	if (mUpscaling) {

		uint32_t info[2] = {(uintptr_t)pixels, lineNum};

		uiPrvFifoTx((uintptr_t)info);
		uiPrvFifoRx();	//wait for copied-out status
	}
	else {
	
		uint_fast8_t i;
		
		#ifdef UNSCALED_IMG_ROTATED
	
		//	fb += DISP_HEIGHT * (lineNum + (DISP_WIDTH - 144) / 2) + (DISP_HEIGHT - 160) / 2;
			
			fb += (DISP_WIDTH - 144) / 2;
			fb += DISP_WIDTH * (DISP_HEIGHT - 160) / 2;
			fb += 144 - lineNum - 1;

			
			for (i = 0; i < 160; i++, fb += DISP_WIDTH)
				*fb = *pixels++ &~ BG_FLAG_UNDER_OBJS;

		#else

			fb += DISP_WIDTH * (lineNum + (DISP_HEIGHT - 144) / 2) + (DISP_WIDTH - 160) / 2;
			
			for (i = 0; i < 160; i++)
				fb[i] = pixels[i] &~ BG_FLAG_UNDER_OBJS;
		#endif
	}
	
	if (lineNum == 143) {
		static uint32_t which = 0;
		static uint64_t prevTime = 0;
		uint64_t curTime = getTime();
		
		
		if (++which == 100) {
			
			uint64_t ticksPerFrame = curTime - prevTime;
			uint32_t fpsT100 = TICKS_PER_SECOND * 100ULL / ticksPerFrame;
			
			pr("Ticks since last: %u. FPS: %u.%02u\n", (uint32_t)ticksPerFrame, fpsT100 / 100, fpsT100 % 100);
			which = 0;
		}
		prevTime = curTime;
	}
}

static uint32_t __attribute__((noinline)) desiredFramerate(void)
{
	static const uint8_t fpsVals[] = DISP_SPEED_SETTINGS;
	struct Settings settings;
	
	settingsGet(&settings);
		
	return fpsVals[settings.speed];
}

static uint32_t __attribute__((noinline)) desiredContrast(void)
{
	struct Settings settings;
	
	settingsGet(&settings);
		
	return settings.contrast;
}

static uint32_t __attribute__((noinline)) desiredBrightness(void)
{
	struct Settings settings;
	
	settingsGet(&settings);
		
	return settings.brightness;
}

static bool __attribute__((noinline)) shouldActAsCgb(void)
{
	struct Settings settings;
	
	settingsGet(&settings);
	
	return settings.actLikeGBC;
}

static bool __attribute__((noinline)) shouldUpscale(void)
{
	struct Settings settings;
	
	settingsGet(&settings);
	
	return settings.upscale;
}

static void gb(void)
{
	uint32_t romSzExpected, ramSzExpected;
	
	while(1) {
		
		if (!mbcInit((void*)QSPI_ROM_START, &romSzExpected, CART_RAM_ADDR_IN_RAM, &ramSzExpected))
			pr("Failed to init the MBC\n");
		else if (ramSzExpected > QSPI_RAM_SIZE_MAX)
			pr("too much ram needed\n");
		else {
			
			dispPrvFrameCtrReset();
			mUpscaling = shouldUpscale();
			if (mUpscaling) 
				uiPrvUpscalerInit();
			memset(dispGetFb(), 0, DISP_WIDTH * DISP_HEIGHT * DISP_BPP / 8);	
			gbSetFrameDithering(1);
			gbRun(shouldActAsCgb());
			//if we are aborted by gbAbort, we'll return here and restart our run
		}
	}
}

static void gpiosConfig(bool firstTime)
{
	static struct {
		uint8_t pin;
		uint8_t padCfg;
		uint8_t funcSel			: 5;
		uint8_t oe				: 1;
		uint8_t val				: 1;
		uint8_t skipReconfig	: 1;	//do not reconfigure if not first call to gpiosConfig()
	} const cfgs[] = {
		{PIN_RAM_NCS, PADS_BANK0_GPIO0_SLEWFAST_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_XIP_SS_N_1, 1, 1},

		{PIN_LCD_DnC, 0, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 1, 0, 1},
		{PIN_LCD_SCK, PADS_BANK0_GPIO0_SLEWFAST_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO0_0, 1, 0, 1},
		{PIN_LCD_DO, PADS_BANK0_GPIO0_SLEWFAST_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO0_0, 1, 0, 1},
		{PIN_LCD_BL, 0, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PWM_A_0, 1, 0, 1},
		{PIN_LCD_CS, 0, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 1, 1, 1},

		{PIN_TOUCHINT, PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_IE_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 0, 0},

		{PIN_WS2812, 0, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO1_0, 1, 0},

		{PIN_SD_MISO, PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SPI0_RX, 0, 0},
		{PIN_SD_NCS, PADS_BANK0_GPIO0_SLEWFAST_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 1, 1},
		{PIN_SD_SCK, PADS_BANK0_GPIO0_SLEWFAST_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SPI0_RX, 1, 0},
		{PIN_SD_MOSI, PADS_BANK0_GPIO0_SLEWFAST_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SPI0_RX, 1, 0},

		{PIN_BTN_L, PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS | PADS_BANK0_GPIO0_PUE_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 0, 0},
		{PIN_BTN_R, PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS | PADS_BANK0_GPIO0_PUE_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 0, 0},
		{PIN_BTN_U, PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS | PADS_BANK0_GPIO0_PUE_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 0, 0},
		{PIN_BTN_D, PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS | PADS_BANK0_GPIO0_PUE_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 0, 0},
		{PIN_BTN_A, PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS | PADS_BANK0_GPIO0_PUE_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 0, 0},
		{PIN_BTN_B, PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS | PADS_BANK0_GPIO0_PUE_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 0, 0},
		{PIN_BTN_START, PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS | PADS_BANK0_GPIO0_PUE_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 0, 0},
		{PIN_BTN_SEL, PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS | PADS_BANK0_GPIO0_PUE_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 0, 0},
		{PIN_BTN_CENTER, PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS | PADS_BANK0_GPIO0_PUE_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 0, 0},

		{PIN_IRDA_IN, PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO1_SCHMITT_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO1_0, 0, 0},
		{PIN_IRDA_OUT, 0, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO1_0, 1, 0},
		{PIN_IRDA_SD, 0, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 1, 1},

		{PIN_SELF_PWR, 0, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 1, 1},

		{PIN_SPQR, 0, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PWM_A_0, 1, 0},
	};
	
	uint32_t gpiosConfigged = 0;
	uint_fast8_t i;
	
	for (i = 0; i < sizeof(cfgs) / sizeof(*cfgs); i++) {
		
		uint_fast8_t pinNo = cfgs[i].pin;
		
		gpiosConfigged |= 1 << pinNo;

		if (firstTime || !cfgs[i].skipReconfig) {

			padsbank0_hw->io[pinNo] = (padsbank0_hw->io[pinNo] &~ (PADS_BANK0_GPIO0_ISO_BITS | PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_DRIVE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_PDE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS | PADS_BANK0_GPIO0_SLEWFAST_BITS)) | cfgs[i].padCfg;
			iobank0_hw->io[pinNo].ctrl = (iobank0_hw->io[pinNo].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | (((uint32_t)cfgs[i].funcSel) << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);

			if (cfgs[i].oe) {

				if (cfgs[i].val)
					sio_hw->gpio_set = 1 << pinNo;
				else
					sio_hw->gpio_clr = 1 << pinNo;

				sio_hw->gpio_oe_set = 1 << pinNo;
			}
			else
				sio_hw->gpio_oe_clr = 1 << pinNo;
		}
	}

	//now the leftovers
	if (firstTime) {
		for (i = 0; i < 30; i++) {
			
			if (!(gpiosConfigged & (1 << i))) {
			
				padsbank0_hw->io[i] |= PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_ISO_BITS;
				iobank0_hw->io[i].ctrl |= IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_NULL;
			}
		}
	}
}

static void i2cAccelConfigure(void)
{
	uint8_t rv = 0xff;

	if (!i2cRegRead(ACCEL_I2C_ADDR, 0x0f, &rv, 1) || rv != 0x33)
		pr("Accelerometer not found\n");
	else {

		bool success = true;

		success = i2cOneByteRegWrite(ACCEL_I2C_ADDR, 0x1f, 0xc0) && success;		//temp measurement on
		success = i2cOneByteRegWrite(ACCEL_I2C_ADDR, 0x20, 0x77) && success;		///400Hz no-low power mode, all axes on
		success = i2cOneByteRegWrite(ACCEL_I2C_ADDR, 0x23, 0xd8) && success;		//update data when both halves read, BE, 4g scale, high res

		pr("ACCEL config success: %d\n", success);
	}
}

static void defconIoI2Cread16cbk(void *userData, const struct I2Creq *req, bool likelySuccess)
{
	uint_fast8_t shrBy = (uint_fast8_t)(uintptr_t)userData;

	if (!likelySuccess)
		mDefconExtraIoData[1] = 0xc2;	//done, error, error type 2
	else {

		uint_fast16_t val = 256 * mI2CreadBuf[0] + mI2CreadBuf[1];

		val >>= shrBy;
		val &= 0x0fff;

		mDefconExtraIoData[0] = val;
		mDefconExtraIoData[1] = 0x80 + (val >> 8);	//done, value
	}
}

static void defconIoI2Cread16(uint8_t addr, uint8_t reg, uint_fast8_t shrBy)
{
	static struct I2Creq req;

	req.addr = addr;
	req.txData = &reg;
	req.txLen = sizeof(reg);
	req.rxData = mI2CreadBuf;
	req.rxLen = sizeof(mI2CreadBuf);

	asm volatile("":::"memory");	//sigh...
	if (!i2cTransact(&req, defconIoI2Cread16cbk, (void*)(uintptr_t)shrBy))
		mDefconExtraIoData[1] = 0xc1;	//done, error, error type 1
}

static void myIrdaSIRuartRxF(void *userData, uint16_t *rawBuf, uint32_t nItems)
{
	static const uint8_t match[] = {0xFC, 0xFF, 0xFD, 0x44, 0x47};
	uint32_t i;

	for (i = 0; i < nItems; i++) {

		if (mIrRxBytesUsed == sizeof(mIrRxBuf) / sizeof(*mIrRxBuf)) {
			mIrRxHadOverrun = 1;
			break;
		}
		mIrRxBuf[mIrRxWritePos] = rawBuf[i];
		if (++mIrRxWritePos == sizeof(mIrRxBuf) / sizeof(*mIrRxBuf))
			mIrRxWritePos = 0;
		mIrRxBytesUsed++;
	}


	if (mEEpos < 0)
		return;

	while (nItems--) {

		uint16_t val = *rawBuf++;

		if (val >> 8) {													//error causes a reset
			mEEpos = 0;
			continue;
		}
		
		if (val != match[mEEpos] && val != match[mEEpos = 0])			//so does mismatch, but it might match [0]
			continue;
		
		if (mEEpos != sizeof(match) - 1) 								//match advances
			mEEpos++;
		else {

			mEEpos = -1;
			break;
		}
	}
}

static void irdaInit(bool txMode)
{
	union UartCfg cfg = {
		.baudrate = 9600,
		.charBits = 3,	//8
		.stopBits = 1,	//1
		.parEna = 0,
		.rxEn = !txMode,
		.txEn = !!txMode,
	};

	mPrevIrdaModeWasTx = txMode;
	pr("irda config: rx=%d tx=%d\n", cfg.rxEn, cfg.txEn);
	if (!irdaSIRuartConfig(&cfg, myIrdaSIRuartRxF, NULL)) {

		pr("IrDA RX cfg fail\n");
		return;
	}
	sio_hw->gpio_clr = 1 << PIN_IRDA_SD;
}

void irdaRoll(void)
{
	bool prevMode = mPrevIrdaModeWasTx;
	static const uint8_t buf[] = {0xfc, 0xff, 0xfd, 0x44, 0x47};
	uint8_t i;


	irdaInit(true);
	for (i = 0; i < 8; i++) {
		
		irdaSIRuartTx(buf, sizeof(buf), true);
		while(!(irdaSIRuartGetSta() & UART_STA_BIT_TX_FIFO_EMPTY));
	}
	irdaInit(prevMode);
}

static void defconIoCmd(uint_fast8_t byte)
{
	static const uint8_t idStatic[16] = {0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x44, 0x6d, 0x69, 0x74, 0x72, 0x79, 0x2e, 0x47, 0x52, 0x00};
	uint_fast8_t cmd = byte >> 4, subCmd = byte & 0x0f;

	switch (cmd) {
		case 0 ... 8:			//WRLEDx
			if (subCmd < 3) {

				pr("defcon led set 0x%02x -> (%u, %u)\n", mDefconExtraIoData[0], cmd, subCmd);
				ws2812Set(cmd, subCmd, mDefconExtraIoData[0]);
			}
			break;

		case 9:					//LEDSYNC
			pr("leds refresh\n");
			ws2812refresh();
			break;

		case 10:				//RDPEN / RDIMU / RDADC
			switch (subCmd) {
				case 0 ... 3:	//RDPEN
					mDefconExtraIoData[1] = 0;
					defconIoI2Cread16(TOUCH_I2C_ADDR, 0xc0 + 0x10 * subCmd, 4);
					break;

				case 4 ... 6:	//RDIMU
					mDefconExtraIoData[1] = 0;
					defconIoI2Cread16(ACCEL_I2C_ADDR, 0xa8 + 2 * (subCmd - 4), 8);
					break;

				case 7 ... 9:	//RDADC
					mDefconExtraIoData[1] = 0;
					defconIoI2Cread16(ACCEL_I2C_ADDR, 0x88 + 2 * (subCmd - 7), 4);
					break;
				
				default:
					break;
			}
			break;

		case 11:				//EMUCMD
			switch (subCmd) {
				case 0:			//EMUEXIT
					exitGame();
					break;

				case 1:			//EMUOFF
					uiSaveSavestate();
					doSleep();
					break;

				case 2:			//SAVEFLUSH
					uiSaveSavestate();
					break;

				default:
					break;
			}
			break;
		
		case 12:				//IRCOMD
			switch (subCmd) {
				case 0:			//power/mode control
					irdaInit(!!mDefconExtraIoData[0]);
					if (mDefconExtraIoData[0])
						mIrRxBytesUsed = 0;	//reset RX buffer if going to TX mode
					break;

				case 1:			//tx
					(void)irdaSIRuartTx((const uint8_t*)&mDefconExtraIoData[0], 1, true);
					break;

				case 2:			//rx
					if (!mIrRxBytesUsed)
						mDefconExtraIoData[1] = 0;
					else {
						uint_fast8_t rxPos = (mIrRxWritePos + sizeof(mIrRxBuf) / sizeof(*mIrRxBuf) - mIrRxBytesUsed) % (sizeof(mIrRxBuf) / sizeof(*mIrRxBuf));
						uint_fast16_t rxVal = mIrRxBuf[rxPos];
						bool haveMore, hadRxOverrun;

						asm volatile("cpsid i":::"memory");		//we could use ldres/stres or even disable the proper interrupt, but i am lazy and it is the 11th hour, so ... you'll live :)
						haveMore = !!--mIrRxBytesUsed;
						hadRxOverrun = mIrRxHadOverrun;
						mIrRxHadOverrun = 0;
						asm volatile("cpsie i":::"memory");
						
						mDefconExtraIoData[0] = rxVal;
						mDefconExtraIoData[1] = 0x01 + (haveMore ? 0x80 : 0) + ((hadRxOverrun || (rxVal & UART_BIT_MASK_RX_OVERRUN)) ? 0x40 : 0) + ((rxVal & (UART_BIT_MASK_PAR_ERR | UART_BIT_MASK_BRK_RXED | UART_BIT_MASK_FRM_ERR)) ? 0x02 : 0x00);
					}
					break;

				case 3:			//tx poll
					mDefconExtraIoData[0] = !(irdaSIRuartGetSta() & UART_STA_BIT_TX_FIFO_EMPTY);
					break;
			}
			break;

		case 15:				//RDID
			if (subCmd < 8) {

				mDefconExtraIoData[1] = idStatic[subCmd * 2 + 0];
				mDefconExtraIoData[0] = idStatic[subCmd * 2 + 1];
			}
			else {
				uint16_t val = flashGetUid() >> ((subCmd - 8) * 16);

				mDefconExtraIoData[1] = val >> 8;
				mDefconExtraIoData[0] = val;
			}
			break;


		default:
			break;
	}
}

int16_t gbExtraIoRegRead(uint8_t addr)
{
	int16_t ret;

	switch (addr) {
		case 0x7D:	//data [0]
			ret = (uint32_t)mDefconExtraIoData[0];
			break;
	
		case 0x7E:	//data [1]
			ret = (uint32_t)mDefconExtraIoData[1];
			break;

		case 0x7F:	//IDR
			ret = 0x21;
			break;

		default:
			ret = -1;
			break;
	}

	pr("DEFCON RD [%02x] -> %04x\n", addr, (uint16_t)ret);

	return ret;
}

bool gbExtraIoRegWrite(uint8_t addr, uint8_t data)
{
	pr("DEFCON WR %02x -> [%02x]\n", data, addr);
	switch (addr) {
		case 0x7D:	//data [0]
			mDefconExtraIoData[0] = data;
			return true;

		case 0x7E:	//data [1]
			mDefconExtraIoData[1] = data;
			return true;
		
		case 0x7F:	//CTL
			defconIoCmd(data);
			return true;

		default:
			return false;
	}
}

static uint_fast8_t rtcPrvFromBCD(uint_fast8_t val)
{
	return (val % 16) + 10 * (val / 16);
}

static uint_fast8_t rtcPrvToBCD(uint_fast8_t val)
{
	return (val % 10) + 16 * (val / 10);
}

static int64_t rtcPrvReadU32(void)
{
	const uint32_t secondsPerMinute = 60, secondsPerHour = 60 * secondsPerMinute, secondsPerDay = 24 * secondsPerHour, secondsPerFourYears = (3 * 365 + 366) * secondsPerDay;
	static const uint16_t daysBeforeMonthNormal[] = {31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
	static const uint16_t daysBeforeMonthLeap[] = {31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
	bool inLeapYear = false;
	uint_fast16_t years;
	uint_fast8_t month;
	uint8_t rtcVals[7];
	uint32_t val = 0;

	if (!i2cRegRead(RTC_I2C_ADDR, 0x02, rtcVals, sizeof(rtcVals))) {
		pr("RTC READ Fail\n");
		return -1;
	}

	//account for years
	years = rtcPrvFromBCD(rtcVals[6]) + ((rtcVals[5] & 0x80) ? 100 : 0);	//years since 1900
	val += secondsPerFourYears * (years / 4);	//combos of 4 years that include a leap year
	years %= 4;
	if (!years) {

		inLeapYear = true;
	}
	else {
		val += 366 * secondsPerDay;
		val += 365 * secondsPerDay * (years - 1);
	}

	//months
	month = rtcPrvFromBCD(rtcVals[5] & 0x1f) - 1;
	if (month)
		val += secondsPerDay * (inLeapYear ? daysBeforeMonthLeap : daysBeforeMonthNormal)[month - 1];

	val += (rtcPrvFromBCD(rtcVals[3]) - 1) * secondsPerDay;
	val += rtcPrvFromBCD(rtcVals[2]) * secondsPerHour;
	val += rtcPrvFromBCD(rtcVals[1]) * secondsPerMinute;
	val += rtcPrvFromBCD(rtcVals[0] & 0x7f);

	return (uint64_t)(uint32_t)val;
}

static bool rtcPrvWriteU32(uint32_t time)
{
	static const uint8_t daysPerMonthNormal[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	static const uint8_t daysPerMonthLeap[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	const uint32_t daysPerFourYears = 3 * 365 + 366;
	uint_fast16_t years, months = 0;
	const uint8_t *daysPerMonth;
	uint8_t rtcVals[8];

	rtcVals[1] = rtcPrvToBCD(time % 60); time /= 60;		//seconds
	rtcVals[2] = rtcPrvToBCD(time % 60); time /= 60;		//minutes
	rtcVals[3] = rtcPrvToBCD(time % 24); time /= 24;		//hours
	//time is now in days

	//subtract out units of 4 years (one leap year and four not)
	years = (time / daysPerFourYears) * 4;
	time %= daysPerFourYears;

	//now handle any leftover years
	if (time >= 366) {

		years++;
		time -= 366;
		years += time / 365;
		time %=365;
		daysPerMonth = daysPerMonthNormal;
	}
	else {

		daysPerMonth = daysPerMonthLeap;
	}

	while (time >= *daysPerMonth) {
		
		months++;
		time -= *daysPerMonth++;
	}

	rtcVals[4] = rtcPrvToBCD(time + 1);
	rtcVals[6] = (years >= 100 ? 0x80 : 0x00) | rtcPrvToBCD(1 + months);
	rtcVals[7] = rtcPrvToBCD(years % 100);
	rtcVals[0] = 0x02;	//register idx itself
	
	return i2cSimpleWrite(RTC_I2C_ADDR, rtcVals, sizeof(rtcVals));
}

static void rtcInit(void)
{
	uint64_t currentTicks = getTime();		//order here matters since rtcPrvReadU32() gives time at start of the actual read that takes half an ms
	int64_t rtcTime = rtcPrvReadU32();
	uint8_t rtcVals[7];

	if (rtcTime < 0) {
		pr("RTC comms error\n");
		return;
	}

	//record the RTC offset so that our tick counter can function as the clock (it is much faster to read)
	mRtcTickOffset = currentTicks - (uint64_t)(uint32_t)rtcTime * TICKS_PER_SECOND;
}

uint32_t gbExtRtcGet(void)
{
	return (getTime() - mRtcTickOffset + TICKS_PER_SECOND /  2) / TICKS_PER_SECOND;
}

void gbExtRtcSet(uint32_t time)
{
	mRtcTickOffset = getTime() - (uint64_t)time * TICKS_PER_SECOND;

	rtcPrvWriteU32(time);
}

static void gbExtAccelReadCbk(void *userData, const struct I2Creq *req, bool likelySuccess)
{
	uint16_t *data = (uint16_t*)userData;

	if (likelySuccess) {
		
		int16_t accelX = __builtin_bswap16(data[1]);
		int16_t accelY = -__builtin_bswap16(data[0]);
		uint16_t gameX =  0x8000 + accelX * 0x70 / 8192;	//middle 0x8000, one G = 0x70
		uint16_t gameY =  0x8000 + accelY * 0x70 / 8192;	//middle 0x8000, one G = 0x70

		data[0] = gameX;
		data[1] = gameX;
	}
}

void gbExtAccelRead(uint16_t *xP, uint16_t *yP)
{
	static uint16_t samples[2] = {0x8000, 0x8000};
	static const uint8_t regAddr = 0xa8;
	static const struct I2Creq req = {
		.haveNext = false,
		.addr = ACCEL_I2C_ADDR,
		.txData = &regAddr,
		.txAcks = NULL,
		.txLen = 1,
		.rxAddrAck = NULL,
		.rxData = (void*)samples,
		.rxAcks = NULL,
		.rxLen = sizeof(samples),
	};

	//this will work if we assume that reads are often enough that one sample late is ok

	//provide current sample
	*xP = samples[0];
	*yP = samples[1];

	//get next (if not busy)
	(void)i2cTransact(&req, gbExtAccelReadCbk, samples);
}

static uint_fast16_t prvAdcVal2mv(const uint8_t *val, uint32_t scaleValue)
{
	uint32_t v = 256 * val[0] + val[1];		//unsigned 10-bit value referenced to 800mV. Range of the 10 bits is 800-1600mV

	//ignore all previous datashits. ACTUAL ADC behaviour is as follows:
	//0.92V -> 0x7f00
	//xV -> 104945.3257 - 78874.41376 * x
	//1.3V -> ??? ADC starts reporting negatives, but the forumla holds...
	//ceiling where this stops not tested

	//thus the map from ADC reported 16-bit value to input voltage is
	// V = (104945.3257 - adcVal) / 78874.41376

	v = 104898 - v;
	v = v * 831 / 65536;
	v = v * scaleValue / 65536;

	return v;
}

static uint_fast16_t uiPrvSelfTestReadNonGbKeys(void)
{
	uint_fast16_t ret = 0;

	if (!(sio_hw->gpio_in & (1 << PIN_BTN_CENTER)))
		ret += 0x100;

	if (!(sio_hw->gpio_hi_in & (1 << 27)))
		ret += 0x200;

	return ret;
}

//lowest 8 bits will be shown on ui

#define TEST_ID_TOUCH_COMMS		0x00000001
#define TEST_ID_RTC_COMMS		0x00000002
#define TEST_ID_RTC_ACCURACY	0x00000004
#define TEST_ID_IMU_COMMS		0x00000008
#define TEST_ID_IMU_VALS		0x00000010	//verifies magnitude aggs up to about gravity
#define TEST_ID_IRDA			0x00000020
#define TEST_ID_POWER_SEQ		0x00000040
#define TEST_ID_PASSABLE		0x00000080	//all of the above are done and pass

#define TEST_ID_SOUNDS_CYCLED	0x00000100
#define TEST_ID_LEDS_CYCLED		0x00000200
#define TEST_ID_BUTTONS			0x00000400

#define TESTS_FOR_STAGE_1		(TEST_ID_TOUCH_COMMS | TEST_ID_RTC_COMMS | TEST_ID_IMU_COMMS | TEST_ID_IMU_VALS | TEST_ID_IRDA)
#define TESTS_FOR_PASSABLE		(TEST_ID_TOUCH_COMMS | TEST_ID_RTC_COMMS | TEST_ID_IMU_COMMS | TEST_ID_IMU_VALS | TEST_ID_IRDA | TEST_ID_RTC_ACCURACY)

static uint_fast16_t isqrt(uint32_t val)
{
	uint32_t min = 0, max = 0xffff;

	while (min < max) {
		uint32_t guess = (max + min) / 2, guessSqr = guess * guess;

		if (guessSqr < val)
			min = guess + 1;
		else if (guessSqr == val)
			break;
		else
			max = guess - 1;
	}

	return (max + min) / 2;
}

static void uiPrvSelfTestDrawStatic(struct Canvas *cnv, bool inverted, bool flipped)
{
	uiSelfTestInit(cnv, inverted, flipped);
	uiSelfTestSetText(cnv, 25, 0, "TOUCH:");
	uiSelfTestSetText(cnv, 40, 0, "RTC:");
	uiSelfTestSetText(cnv, 55, 0, "BATT:");
	uiSelfTestSetText(cnv, 70, 0, "BUS:");
	uiSelfTestSetText(cnv, 85, 0, "IMU:");
	uiSelfTestSetText(cnv, 100, 0, "BTNs:");
	uiSelfTestSetText(cnv, 115, 0, "SIR:");
}

static void uiPrvSelfTestsIfNeeded(void)
{
	const unsigned magicGpio = 7;

	uint32_t passedTests = 0, failedTests = 0;	//these are sticky, on purpose!!
	uint8_t adcVals[6], stage = 0;
	uint32_t battMv, usbMv;
	struct Canvas cnv;
	bool doSelfTest;

	if (!i2cRegRead(ACCEL_I2C_ADDR, 0x88, adcVals, 4))
		doSelfTest = 1;
	else {
		battMv = prvAdcVal2mv(adcVals + 0, 65536 / 0.3);
		usbMv = prvAdcVal2mv(adcVals + 2, 65536 / 0.2);
		
		pr("BATT: %umV, USB: %umV\n", battMv, usbMv);
	
		if (usbMv < 4600) {
			pr("SELF TEST: VBUS too low\n");
			return;
		}
	}

	//set up magic gpio, wait, read
	sio_hw->gpio_oe_clr = 1 << magicGpio;
	padsbank0_hw->io[magicGpio] = PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PDE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS;
	iobank0_hw->io[magicGpio].ctrl = (iobank0_hw->io[magicGpio].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | (IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
	pr("Wait for self test indication to settle\n");
	delayMsec(1000);
	doSelfTest = (sio_hw->gpio_in >> magicGpio) & 1;
	pr("SELF TEST: magic gpio read: %d\n", doSelfTest);
	doSelfTest |= uiGetKeys() == (KEY_BIT_START	| KEY_BIT_SEL);
	pr("SELF TEST: buttons too: %d\n", doSelfTest);


	if (doSelfTest) {
		uint64_t prevLedTime = 0, prevIrTime = 0, soundTime = 0, prevTouchTime = 0, prevSensorsTime = 0, prevRtcChange = 0, prevStageTime = 0;
		uint8_t soundCounter = 0, rtcSampleCounter = 0;
		uint16_t btnsSeen = 0, btnsStart;
		uint8_t ledIndex = 0;
		uint8_t ledColor = 0;
		uint32_t prevRtc = 0;
		
		//set up IrDA GPIOs for gpio use
		iobank0_hw->io[PIN_IRDA_IN].ctrl = (iobank0_hw->io[PIN_IRDA_IN].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | (IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
		iobank0_hw->io[PIN_IRDA_OUT].ctrl = (iobank0_hw->io[PIN_IRDA_OUT].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | (IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
		iobank0_hw->io[PIN_IRDA_SD].ctrl = (iobank0_hw->io[PIN_IRDA_SD].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | (IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);

		//set up bootsel pin as input...i hear you, the index here should be 5...but it is 1...do not ask
		ioqspi_hw->io[1].ctrl = (ioqspi_hw->io[1].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | (IO_QSPI_GPIO_QSPI_SCLK_CTRL_FUNCSEL_VALUE_SIO_58 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
		pads_qspi_hw->io[1] = PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_PDE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS;
		sio_hw->gpio_hi_oe_clr = 1 << 27;
		
		dispSetBrightness(31);
		uiPrvSelfTestDrawStatic(&cnv, false, true);

		btnsStart = uiGetKeys() | uiPrvSelfTestReadNonGbKeys();
		prevStageTime = getTime();

		while(1) {

			uint8_t touchX[2], touchY[2], touchZ[2];
			int64_t curRtc = rtcPrvReadU32();
			uint16_t xi, yi, zi, btnsNow;
			uint64_t time = getTime();
			int16_t x, y;
			bool penDown;

			if (!((passedTests | failedTests) & TEST_ID_LEDS_CYCLED) && time - prevLedTime >= TICKS_PER_SECOND / 8) {
				prevLedTime = time;

				ws2812Set(ledIndex, ledColor, 0);
				if (++ledColor == 3) {
					ledColor = 0;
					if (++ledIndex == NUM_WS2812s) {

						uint_fast8_t i, j;

						passedTests |= TEST_ID_LEDS_CYCLED;
						for (i = 0; i < NUM_WS2812s; i++) {
							for (j = 0; j < 3; j++)
								ws2812Set(i, j, 16);
						}
						ws2812refresh();
						ledIndex = 0;
						ledColor = 4;	//out of bounds on purpose
					}
				}
				ws2812Set(ledIndex, ledColor, 64);
				ws2812refresh();
			}

			if (!((passedTests | failedTests) & TEST_ID_SOUNDS_CYCLED) && time - soundTime >= TICKS_PER_SECOND / 20) {
				soundTime = time;

				switch (soundCounter++) {
					case 0:
						doFreq(4000);
						break;
					case 32:
						doFreq(2000);
						break;
					case 64:
						doFreq(1000);
						break;
					case 96:
						doFreq(500);
						break;
					case 128:
						passedTests |= TEST_ID_SOUNDS_CYCLED;
						soundCounter = 0;
						break;
					default:
						doFreq(0);
						break;
				}
			}

			if (!((passedTests | failedTests) & TEST_ID_IRDA) && time - prevIrTime >= TICKS_PER_SECOND) {
				
				uint_fast8_t i, in = 0;
				bool pass;

				prevIrTime = time;

				//IR shutdown pin goes outout and low
				sio_hw->gpio_oe_set = 1 << PIN_IRDA_SD;
				sio_hw->gpio_clr = 1 << PIN_IRDA_SD;
				delayMsec(1);

				for (i = 0; i < 4; i++) {

					//set output to high to send a pulse
					sio_hw->gpio_set = 1 << PIN_IRDA_OUT;
					delayUsec(180);

					//read input
					in = in * 2 + ((sio_hw->gpio_in >> PIN_IRDA_IN) & 1);

					//set output to low to kill the pulse
					sio_hw->gpio_clr = 1 << PIN_IRDA_OUT;
					delayUsec(180);

					//read input
					in = in * 2 + ((sio_hw->gpio_in >> PIN_IRDA_IN) & 1);
				}

				//IR shutdown pin goes back to being an input
				sio_hw->gpio_oe_clr = 1 << PIN_IRDA_SD;

				pass = in == 0x55;

				if (pass)
					passedTests |= TEST_ID_IRDA;
				else
					failedTests |= TEST_ID_IRDA;

				uiSelfTestSetText(&cnv, 115, 50, "%s        ", (failedTests & TEST_ID_IRDA) ? "FAIL" : ((passedTests & TEST_ID_IRDA)  ? "PASS" : "????"));
			}

			if (i2cRegRead(TOUCH_I2C_ADDR, 0xc0, touchX, sizeof(touchX)) && 
					i2cRegRead(TOUCH_I2C_ADDR, 0xd0, touchY, sizeof(touchY)) &&
					i2cRegRead(TOUCH_I2C_ADDR, 0xe0, touchZ, sizeof(touchZ))) {

				xi = touchX[0] * 16 + touchX[1] / 16;
				yi = touchY[0] * 16 + touchY[1] / 16;
				zi = touchZ[0] * 16 + touchZ[1] / 16;

				penDown = zi > 100;

				if (penDown) {

					x = 350 - xi * 94 / 1024;
					y = 260 - yi * 71 / 1024;

					uiSelfTestSetText(&cnv, 25, 50, "(%5u %5u) = (%6d %6d)            ", xi, yi, x, y);

					if (x >= (320 - 240) / 2 && x - (320 - 240) / 2 < 240 && y >= (240 - 216) / 2 && y - (240 - 216) / 2 < 216) {

						x -= (320 - 240) / 2;
						y -= (240 - 216) / 2;

						uiSelfTestSetText(&cnv, 215 - y, 239 - x, "*");
					}
				}
				else {

					uiSelfTestSetText(&cnv, 25, 50, "PEN IS UP                                                      ");
				}
				passedTests |= TEST_ID_TOUCH_COMMS;
			}
			else {

				failedTests |= TEST_ID_TOUCH_COMMS;
				uiSelfTestSetText(&cnv, 25, 50, "FAIL               ");
			}

			btnsNow = uiGetKeys() | uiPrvSelfTestReadNonGbKeys();
			btnsSeen |= btnsNow;
			uiSelfTestSetText(&cnv, 100, 50, "NOW: 0x%03x, %s%03x      ", btnsNow, (btnsSeen == 0x3ff && btnsStart == 0) ? "(ALL PASS: 0x" : "SEEN: 0x", btnsSeen);

			if (!((passedTests | failedTests) & TEST_ID_RTC_ACCURACY)) {

				if (curRtc < 0) {

					failedTests |= TEST_ID_RTC_COMMS;
					failedTests |= TEST_ID_RTC_ACCURACY;
					uiSelfTestSetText(&cnv, 40, 50, "FAIL               ");
				}
				else if (prevRtc != (uint32_t)curRtc) {

					const char *verdict;

					if (!prevRtc)
						verdict = "(..)";
					else if (rtcSampleCounter < 2){
						rtcSampleCounter++;
						verdict = "(...)";
					}
					else {

						uint64_t ticksLapse = time - prevRtcChange;
						bool withinTolerance;

						//allow 10% slop
						withinTolerance = (ticksLapse >= TICKS_PER_SECOND * 90ull / 100 && ticksLapse < TICKS_PER_SECOND * 110ull / 100 &&  prevRtc + 1 == curRtc);

						verdict = withinTolerance ? "(ACC PASS)" : "(ACC FAIL)";
						if (withinTolerance)
							passedTests |= TEST_ID_RTC_ACCURACY;
						else
							failedTests |= TEST_ID_RTC_ACCURACY;
					}

					prevRtc = curRtc;
					prevRtcChange = time;

					uiSelfTestSetText(&cnv, 40, 50, "%u %s     ", (uint32_t)curRtc, verdict);
					passedTests |= TEST_ID_RTC_COMMS;
				}
			}

			if (time - prevSensorsTime >= TICKS_PER_SECOND / 10) {
				prevSensorsTime = time;

				
				if (i2cRegRead(ACCEL_I2C_ADDR, 0x88, adcVals, 4)) {

					uint32_t battMv = prvAdcVal2mv(adcVals + 0, 65536 / 0.3), busMv = prvAdcVal2mv(adcVals + 2, 65536 / 0.2);

					passedTests |= TEST_ID_IMU_COMMS;

					uiSelfTestSetText(&cnv, 55, 50, "%u mV    ", battMv);
					uiSelfTestSetText(&cnv, 70, 50, "%u mV    ", busMv);

					if (stage == 4 && battMv >= 3900 && busMv >= 4800) {

						prevStageTime = time;
						stage = 5;
						uiSelfTestSetText(&cnv, 130, 80, "wait for no VBUS             ");
					}
					else if (stage == 5 && battMv >= 3900 && busMv < 4800) {

						prevStageTime = time;
						stage = 6;
						uiSelfTestSetText(&cnv, 130, 80, "wait for VBUS             ");
					}
					else if (stage == 6 && battMv >= 3900 && busMv >= 4800) {

						prevStageTime = time;
						stage = 7;
						uiSelfTestSetText(&cnv, 130, 80, "wait for no BATT            ");
					}
					else if (stage == 7 && battMv < 3900 && busMv >= 4800) {

						uint_fast8_t i, j;

						stage = 8;
						uiSelfTestSetText(&cnv, 130, 80, "PWR SEQ SUCCESS            ");

						passedTests |= TEST_ID_POWER_SEQ;

						//all LEDs write to indicate full success
						for (i = 0; i < NUM_WS2812s; i++) {
							for (j = 0; j < 3; j++)
								ws2812Set(i, j, 16);
						}
						ws2812refresh();
					}

				}
				else {

					failedTests |= TEST_ID_IMU_COMMS;

					uiSelfTestSetText(&cnv, 55, 50, "FAIL               ");
					uiSelfTestSetText(&cnv, 70, 50, "FAIL               ");
				}
				
				if (!((passedTests | failedTests) & TEST_ID_IMU_VALS)) {

					if (i2cRegRead(ACCEL_I2C_ADDR, 0xa8, adcVals, 6)) {

						int_fast16_t x = (int16_t)__builtin_bswap16(*(uint16_t*)(adcVals + 0));
						int_fast16_t y = (int16_t)__builtin_bswap16(*(uint16_t*)(adcVals + 2));
						int_fast16_t z = (int16_t)__builtin_bswap16(*(uint16_t*)(adcVals + 4));
						uint_fast16_t magnitude;
						bool accuracyPass;

						magnitude = isqrt(x * x + y * y + z * z);
						accuracyPass = (magnitude >= 8192 * 8 / 10 && magnitude <= 8192 * 12 / 10);	//20% slop because this is a shitty accelerometer

						passedTests |= TEST_ID_IMU_COMMS;
						uiSelfTestSetText(&cnv, 85, 50, "%6d %6d %6d %s       ", x, y, z, accuracyPass ? " (ACC PASS)" : " (ACC FAIL)");
						
						if (accuracyPass)
							passedTests |= TEST_ID_IMU_VALS;
						else
							failedTests |= TEST_ID_IMU_VALS;
					}
					else {

						failedTests |= TEST_ID_IMU_COMMS | TEST_ID_IMU_VALS;

						uiSelfTestSetText(&cnv, 85, 50, "FAIL               ");
					}
				}
			}

			if (stage == 0 && ((passedTests == TESTS_FOR_STAGE_1 && failedTests == 0) || (time - prevStageTime > TICKS_PER_SECOND * 2))) {

				uint_fast8_t i, j;

				stage = 1;		//all LEDs green to indicate end of stage 0

				//stage 1 is 100 ms of magic GPIO == 0
				sio_hw->gpio_clr = 1 << magicGpio;
				sio_hw->gpio_oe_set = 1 << magicGpio;
				prevStageTime = time;
			}
			else if (stage == 1 && time - prevStageTime >= TICKS_PER_SECOND / 10) {
				
				//stage 2 is 100ms of magic GPIO == 1
				stage = 2;
				sio_hw->gpio_set = 1 << magicGpio;
				prevStageTime = time;
			}
			else if (stage == 2 && time - prevStageTime >= TICKS_PER_SECOND / 10) {
				
				//stage 3 is 100ms of magic GPIO == 0
				stage = 3;
				sio_hw->gpio_clr = 1 << magicGpio;
				prevStageTime = time;
			}
			else if (stage == 3 && time - prevStageTime >= TICKS_PER_SECOND / 10) {
				
				//stage 4 is checking power sequences. release magic gpio
				stage = 4;
				prevStageTime = time;

				uiSelfTestSetText(&cnv, 130, 0, "PWR SEQ: ");
				uiSelfTestSetText(&cnv, 130, 80, "wait for BAT");
			}

			if (stage >= 3 && stage < 8 && time - prevStageTime > TICKS_PER_SECOND * 10) {

				uiSelfTestSetText(&cnv, 130, 80, "PWR SEQ TIMEOUT");
				failedTests |= TEST_ID_POWER_SEQ;
				stage = 8;
			}

			if ((passedTests & TESTS_FOR_PASSABLE) == TESTS_FOR_PASSABLE && (failedTests & TESTS_FOR_PASSABLE) == 0)
				passedTests |= TEST_ID_PASSABLE;
			if (failedTests & TESTS_FOR_PASSABLE)
				failedTests |= TEST_ID_PASSABLE;

			pr("P: 0x%08x F 0x%08x, s%u\n", passedTests, failedTests, stage);
			uiSelfTestSetText(&cnv, 145, 0, "stage %u, p:0x%x f 0x%x", stage, passedTests, failedTests);
			uiSelfTestSetMarks(&cnv, passedTests, failedTests &~ TEST_ID_POWER_SEQ);
		}
	}

	//restore gpio settings
	gpiosConfig(false);
}


void __attribute__((noreturn, used)) micromain(void)
{
	uint32_t *ptr = (uint32_t *)0x10010000, len = ((2 << 20) >> 2), i, j, unitsToReset;
	uint64_t t;
	pr("running!!\n");
		
	
	asm volatile("cpsie i");
	timebaseInit();
	
	pr("ready, time is 0x%016llx\n", getTime());
	pr("ready, time is 0x%016llx\n", getTime());
	
	//tell refclock to use ROSC
	clocks_hw->clk[clk_ref].ctrl = (clocks_hw->clk[clk_ref].ctrl &~ CLOCKS_CLK_REF_CTRL_SRC_BITS) | (CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH << CLOCKS_CLK_REF_CTRL_SRC_LSB);
	
	//use ref clock for cpu, use sys clock for periphs
	clocks_hw->clk[clk_peri].ctrl &=~ CLOCKS_CLK_PERI_CTRL_ENABLE_BITS;
	clocks_hw->clk[clk_sys].ctrl = (clocks_hw->clk[clk_sys].ctrl &~ CLOCKS_CLK_SYS_CTRL_SRC_BITS)| (CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF << CLOCKS_CLK_SYS_CTRL_SRC_LSB);
	clocks_hw->clk[clk_peri].ctrl = (clocks_hw->clk[clk_peri].ctrl &~ (CLOCKS_CLK_PERI_CTRL_KILL_BITS | CLOCKS_CLK_PERI_CTRL_AUXSRC_BITS)) | CLOCKS_CLK_PERI_CTRL_ENABLE_BITS | (CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS << CLOCKS_CLK_PERI_CTRL_AUXSRC_LSB);

	//release some peripherals from reset
	unitsToReset = RESETS_RESET_PWM_BITS | RESETS_RESET_UART1_BITS | RESETS_RESET_PADS_BANK0_BITS | RESETS_RESET_IO_BANK0_BITS | RESETS_RESET_PIO0_BITS | RESETS_RESET_PIO1_BITS | RESETS_RESET_PIO2_BITS | RESETS_RESET_DMA_BITS | RESETS_RESET_SPI1_BITS | RESETS_WDSEL_PLL_SYS_BITS;
	resets_hw->reset |= unitsToReset;
	resets_hw->reset |= unitsToReset;
	resets_hw->reset &=~ unitsToReset;
	resets_hw->reset &=~ unitsToReset;
	resets_hw->reset &=~ unitsToReset;
	while ((resets_hw->reset_done & unitsToReset) != unitsToReset);
	
	//start XOSC (by stopping it first...)
	xosc_hw->ctrl = (xosc_hw->ctrl &~ XOSC_CTRL_ENABLE_BITS) | (XOSC_CTRL_ENABLE_VALUE_DISABLE << XOSC_CTRL_ENABLE_LSB);
	while (xosc_hw->status & XOSC_STATUS_ENABLED_BITS);
	xosc_hw->startup = (xosc_hw->startup &~ XOSC_STARTUP_DELAY_BITS) | (8191 << XOSC_STARTUP_DELAY_LSB);
	xosc_hw->ctrl = (xosc_hw->ctrl &~ (XOSC_CTRL_FREQ_RANGE_BITS | XOSC_CTRL_ENABLE_BITS)) | (XOSC_CTRL_ENABLE_VALUE_ENABLE << XOSC_CTRL_ENABLE_LSB) | (XOSC_CTRL_FREQ_RANGE_VALUE_1_15MHZ << XOSC_CTRL_FREQ_RANGE_LSB);
	while ((xosc_hw->status & (XOSC_STATUS_STABLE_BITS | XOSC_STATUS_ENABLED_BITS)) != (XOSC_STATUS_STABLE_BITS | XOSC_STATUS_ENABLED_BITS));
	
	//tell refclock to use XOSC
	clocks_hw->clk[clk_ref].ctrl = (clocks_hw->clk[clk_ref].ctrl &~ CLOCKS_CLK_REF_CTRL_SRC_BITS) | (CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC << CLOCKS_CLK_REF_CTRL_SRC_LSB);

	
	//many MHz please
	pll_sys_hw->pwr |= (PLL_PWR_VCOPD_BITS | PLL_PWR_POSTDIVPD_BITS | PLL_PWR_PD_BITS);		//dividers on
	pll_sys_hw->fbdiv_int = (pll_sys_hw->fbdiv_int &~ PLL_FBDIV_INT_BITS) | ((TICKS_PER_SECOND / 1000000 / 2) << PLL_FBDIV_INT_LSB);		//x100 = 1200MHz
	pll_sys_hw->prim = (pll_sys_hw->prim &~ (PLL_PRIM_POSTDIV1_BITS | PLL_PRIM_POSTDIV2_BITS)) | (6 << PLL_PRIM_POSTDIV1_LSB) | (1 << PLL_PRIM_POSTDIV2_LSB);
	pll_sys_hw->pwr &=~ (PLL_PWR_VCOPD_BITS | PLL_PWR_POSTDIVPD_BITS | PLL_PWR_PD_BITS);		//dividers on
	while (!(pll_sys_hw->cs & PLL_CS_LOCK_BITS));
	pll_sys_hw->cs &=~PLL_CS_BYPASS_BITS;

	//switch sys.AUX to pll
	clocks_hw->clk[clk_sys].ctrl = (clocks_hw->clk[clk_sys].ctrl &~ CLOCKS_CLK_SYS_CTRL_AUXSRC_BITS) | (CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS << CLOCKS_CLK_SYS_CTRL_AUXSRC_LSB);
	
	//switch sys to AUX and wait for it
	clocks_hw->clk[clk_sys].ctrl = (clocks_hw->clk[clk_sys].ctrl &~ CLOCKS_CLK_SYS_CTRL_SRC_BITS) | (CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX << CLOCKS_CLK_SYS_CTRL_SRC_LSB);
	while (((clocks_hw->clk[clk_sys].selected & CLOCKS_CLK_REF_SELECTED_BITS) >> CLOCKS_CLK_REF_SELECTED_LSB) != (1 << CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX));

	
	pr("VTAB relocation...\n");
	extern uint32_t __vtab[], __VECTORS[];
	memcpy(__vtab, __VECTORS, 512);
	SCB->VTOR = (uintptr_t)__vtab;

	pr("flash...\n");
	flashBootInit();

	pr("gpios...\n");
	gpiosConfig(true);

	pr("ws2812...");
	ws2812init();

	pr("disp...\n");
	dispInit(desiredFramerate());

	pr("backlight...\n");
	dispSetContrast(desiredContrast());
	dispSetBrightness(desiredBrightness());
	
	pr("i2c...\n");
	i2cInit();
	i2cAccelConfigure();

	pr("RTC...\n");
	rtcInit();

	pr("IrDA...\n");
	irdaInit(true);
	static const uint8_t tx[] = "hello";
	irdaSIRuartTx(tx, sizeof(tx), true);
	while (!(irdaSIRuartGetSta() & UART_STA_BIT_TX_FIFO_EMPTY));
	irdaInit(false);


	uiPrvSelfTestsIfNeeded();

	pr("UI...\n");
	uiPreGame();
	gb();

	while(1);
}



void __attribute__((used)) report_hard_fault(uint32_t* regs, uint32_t ret_lr, uint32_t *user_sp)
{
	uint32_t *push = (ret_lr == 0xFFFFFFFD) ? user_sp : (regs + 8), *sp = push + 8;
	unsigned i;
	
	pr("============ HARD FAULT ============\n");
	pr("R0  = 0x%08X    R8  = 0x%08X\n", (unsigned)push[0], (unsigned)regs[0]);
	pr("R1  = 0x%08X    R9  = 0x%08X\n", (unsigned)push[1], (unsigned)regs[1]);
	pr("R2  = 0x%08X    R10 = 0x%08X\n", (unsigned)push[2], (unsigned)regs[2]);
	pr("R3  = 0x%08X    R11 = 0x%08X\n", (unsigned)push[3], (unsigned)regs[3]);
	pr("R4  = 0x%08X    R12 = 0x%08X\n", (unsigned)regs[4], (unsigned)push[4]);
	pr("R5  = 0x%08X    SP  = 0x%08X\n", (unsigned)regs[5], (unsigned)sp);
	pr("R6  = 0x%08X    LR  = 0x%08X\n", (unsigned)regs[6], (unsigned)push[5]);
	pr("R7  = 0x%08X    PC  = 0x%08X\n", (unsigned)regs[7], (unsigned)push[6]);
	pr("RA  = 0x%08X    SR  = 0x%08X\n", (unsigned)ret_lr,  (unsigned)push[7]);
	pr("SHCSR = 0x%08X\n", SCB->SHCSR);
	pr("CFSR  = 0x%08X    HFSR  = 0x%08X\n", SCB->CFSR, SCB->HFSR);
	pr("MMFAR = 0x%08X    BFAR  = 0x%08X\n", SCB->MMFAR, SCB->BFAR);
	pr("WORDS @ SP: \n");
	
	for (i = 0; i < 32; i++)
		pr("[sp, #0x%03x = 0x%08x] = 0x%08x\n", i * 4, (unsigned)&sp[i], (unsigned)sp[i]);
	
	
	pr("\n\n");
	
	while(1);
}


void __attribute__((naked, used)) HardFault_Handler(void)
{
	asm volatile(
			"push {r4-r7}				\n\t"
			"mov  r0, r8				\n\t"
			"mov  r1, r9				\n\t"
			"mov  r2, r10				\n\t"
			"mov  r3, r11				\n\t"
			"push {r0-r3}				\n\t"
			"mov  r0, sp				\n\t"
			"mov  r1, lr				\n\t"
			"mrs  r2, PSP				\n\t"
			"bl   report_hard_fault		\n\t"
			:::"memory");
}


