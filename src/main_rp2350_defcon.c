#include <string.h>
#include <stdarg.h>
#include "pinoutRp2350defcon.h"
#include "gbCartHeader.h"
#include "pioIrdaSIR.h"
#include "dispDefcon.h"
#include "badgeLeds.h"
#include "badgePower.h"
#include "badgeRtc.h"
#include "bootGuard.h"
#include "pioWS2812.h"
#include "frontend.h"
#include "timebase.h"
#include "settings.h"
#include "pioI2C.h"
#include "gbCore.h"
#include "memMap.h"
#include "printf.h"
#include "sleep.h"
#include "audioPwm.h"
#include "usbHid.h"
#include "2350.h"
#include "qspi.h"
#include "mbc.h"
#include "gb.h"
#include "nes/nes.h"
#include "ui.h"

static void uiPrvUpscalerInit(void);
static void uiPrvUpscalerDeinit(void);
static bool shouldUpscale(void);
static bool shouldRotateGame(void);
static bool mUpscaling;
static bool mRotateGame;
static bool mGameToolExitRequested;
static enum GameRuntime mActiveGameRuntime;

#define GB_SRC_WIDTH                   160u
#define GB_SRC_HEIGHT                  144u
#define GB_ASPECT_WIDTH                10u
#define GB_ASPECT_HEIGHT               9u
#ifdef UPSCALER_ROTATES
#define GB_UPSCALED_HEIGHT             DISP_WIDTH
#define GB_UPSCALED_WIDTH              ((GB_UPSCALED_HEIGHT * GB_ASPECT_WIDTH + GB_ASPECT_HEIGHT / 2u) / GB_ASPECT_HEIGHT)
#define GB_UPSCALED_LEFT               ((DISP_HEIGHT - GB_UPSCALED_WIDTH) / 2u)
#define GB_UPSCALED_TOP                0u
#else
#define GB_UPSCALED_WIDTH              DISP_WIDTH
#define GB_UPSCALED_HEIGHT             ((GB_UPSCALED_WIDTH * GB_ASPECT_HEIGHT + GB_ASPECT_WIDTH / 2u) / GB_ASPECT_WIDTH)
#define GB_UPSCALED_LEFT               0u
#define GB_UPSCALED_TOP                ((DISP_HEIGHT - GB_UPSCALED_HEIGHT) / 2u)
#endif

static uint8_t mGbUpscaleSrcX[GB_UPSCALED_WIDTH];
static uint16_t mGbUpscaleLineStart[GB_SRC_HEIGHT + 1];

static bool mRtcValid;
static enum GbExtRtcMode mGbRtcMode;
static uint64_t mRtcTickOffset;         //ticks to subtract from getTime() to get badge RTC time
static uint64_t mGbRtcTickOffset;       //same unit, but adjustable by emulated cartridge RTC writes
static volatile uint8_t mDefconExtraIoData[3];
static uint8_t mI2CreadBuf[2];
static int8_t mEEpos = 0;
static uint16_t mIrRxBuf[64];
static uint8_t mIrRxWritePos, mIrRxBytesUsed, mIrRxHadOverrun;
static bool mPrevIrdaModeWasTx;

extern uint32_t __data_start, __data_end, __bss_start, __bss_end, __stack_limit, __stack_top;

#define STACK_WATERMARK_WORD	0xC0DEC0DEul
#define STACK_WATERMARK_GUARD	256

#define ACCEL_I2C_ADDR                  0x18
#define TOUCH_I2C_ADDR                  0x48
#define RTC_I2C_ADDR                    0x51


static void stackWatermarkInit(void)
{
	uint32_t *p = &__stack_limit, *end;
	uintptr_t sp;

	asm volatile("mov %0, sp" : "=r"(sp));
	end = (uint32_t*)((sp - STACK_WATERMARK_GUARD) &~ 3ul);
	while (p < end)
		*p++ = STACK_WATERMARK_WORD;
}

static uint32_t stackWatermarkUnused(void)
{
	uint32_t *p = &__stack_limit;

	while (p < &__stack_top && *p == STACK_WATERMARK_WORD)
		p++;
	return (uint32_t)((uintptr_t)p - (uintptr_t)&__stack_limit);
}

static void memoryReport(void)
{
	uint32_t dataSz = (uint32_t)((uintptr_t)&__data_end - (uintptr_t)&__data_start);
	uint32_t bssSz = (uint32_t)((uintptr_t)&__bss_end - (uintptr_t)&__bss_start);
	uint32_t stackSz = (uint32_t)((uintptr_t)&__stack_top - (uintptr_t)&__stack_limit);

	pr("mem: data=%u bss=%u ramvec=512 stack=%u stack_free_now=%u\n",
		dataSz, bssSz, stackSz, stackWatermarkUnused());
}

static void doFreq(uint32_t freq)
{
        (void)audioPwmTone(freq);
}

static void note(uint32_t freq, uint32_t dur)
{
        doFreq(freq);
        delayMsec(dur);
}

#define NOTE_LENGTH                     2048    //msec
#define FULL                            (NOTE_LENGTH / 1)
#define HALF                            (NOTE_LENGTH / 2)
#define QUARTER                         (NOTE_LENGTH / 4)
#define EIGHTH                          (NOTE_LENGTH / 8)
#define SIXTEENTH                       (NOTE_LENGTH / 16)


#define BASE_A                          440

#define G                                       (unsigned)(BASE_A /  1.0595 / 1.0595)
#define Gs                                      (unsigned)(BASE_A /  1.0595)
#define A                                       (unsigned)(BASE_A)
#define As                                      (unsigned)(BASE_A * 1.0595)
#define B                                       (unsigned)(BASE_A * 1.0595 * 1.0595)
#define C                                       (unsigned)(BASE_A * 1.0595 * 1.0595 * 1.0595)
#define Cs                                      (unsigned)(BASE_A * 1.0595 * 1.0595 * 1.0595 * 1.0595)
#define D                                       (unsigned)(BASE_A * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595)
#define Ds                                      (unsigned)(BASE_A * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595)
#define E                                       (unsigned)(BASE_A * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595)
#define F                                       (unsigned)(BASE_A * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595)
#define Fs                                      (unsigned)(BASE_A * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595 * 1.0595)


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
        
        if (!(sta & (1 << PIN_BTN_U)))          ret |= KEY_BIT_UP;
        if (!(sta & (1 << PIN_BTN_D)))          ret |= KEY_BIT_DOWN;
        if (!(sta & (1 << PIN_BTN_L)))          ret |= KEY_BIT_LEFT;
        if (!(sta & (1 << PIN_BTN_R)))          ret |= KEY_BIT_RIGHT;
        
        if (!(sta & (1 << PIN_BTN_A)))          ret |= KEY_BIT_A;
        if (!(sta & (1 << PIN_BTN_B)))          ret |= KEY_BIT_B;
        if (!(sta & (1 << PIN_BTN_START)))      ret |= KEY_BIT_START;
        if (!(sta & (1 << PIN_BTN_SEL)))        ret |= KEY_BIT_SEL;
                
        return ret;
}

static uint_fast16_t prvUiKeysMap(uint32_t sta)
{
        uint_fast16_t ret = prvKeysMap(sta);

        if (!(sta & (1 << PIN_BTN_CENTER)))     ret |= UI_KEY_BIT_CENTER;

        return ret;
}

uint_fast8_t uiGetKeys(void)
{
        uint32_t val, count = 0, countUntil = 10000, ourKeysMask = (1 << PIN_BTN_U) | (1 << PIN_BTN_D) | (1 << PIN_BTN_L) | (1 << PIN_BTN_R) | (1 << PIN_BTN_START) | (1 << PIN_BTN_SEL) | (1 << PIN_BTN_A) | (1 << PIN_BTN_B) | (1 << PIN_BTN_CENTER);

	while(1) {
		usbHidTask();
		badgeLedsTick();
		val = sio_hw->gpio_in & ourKeysMask;
		for (count = 0; count < countUntil && val == (sio_hw->gpio_in & ourKeysMask); count++) {
			usbHidTask();
			badgeLedsTick();
		}
		if (count == countUntil)
			return prvKeysMap(val);
	}
}

uint_fast8_t uiGetKeysRaw(void)
{
	usbHidTask();
	return prvKeysMap(sio_hw->gpio_in);
}

uint_fast16_t uiGetUiKeys(void)
{
        uint32_t val, count = 0, countUntil = 10000, ourKeysMask = (1 << PIN_BTN_U) | (1 << PIN_BTN_D) | (1 << PIN_BTN_L) | (1 << PIN_BTN_R) | (1 << PIN_BTN_START) | (1 << PIN_BTN_SEL) | (1 << PIN_BTN_A) | (1 << PIN_BTN_B) | (1 << PIN_BTN_CENTER);

	while(1) {
		usbHidTask();
		badgeLedsTick();
		val = sio_hw->gpio_in & ourKeysMask;
		for (count = 0; count < countUntil && val == (sio_hw->gpio_in & ourKeysMask); count++) {
			usbHidTask();
			badgeLedsTick();
		}
		if (count == countUntil)
			return prvUiKeysMap(val);
	}
}

uint_fast16_t uiGetUiKeysRaw(void)
{
	usbHidTask();
	return prvUiKeysMap(sio_hw->gpio_in);
}

static void exitGame(void)
{
        enum UiGameAction action;

        if (mUpscaling)
                        uiPrvUpscalerDeinit();

        while (uiGetUiKeysRaw() & UI_KEY_BIT_CENTER) {
        }

        action = uiGameMenu();
        switch (action) {
                case UiGameActionResume:
                        break;

                case UiGameActionRestart:
                case UiGameActionSelectGame:
                        if (mActiveGameRuntime == GameRuntimeNes) {
                                if (action == UiGameActionSelectGame)
                                        mGameToolExitRequested = true;
                                nesAbort();
                        }
                        else
                                gbAbort();
                        return;

                case UiGameActionSwitchTool:
                        mGameToolExitRequested = true;
                        if (mActiveGameRuntime == GameRuntimeNes)
                                nesAbort();
                        else
                                gbAbort();
                        return;
        }

        memset(dispGetFb(), 0, DISP_WIDTH * DISP_HEIGHT * DISP_BPP / 8);
        if (mActiveGameRuntime == GameRuntimeNes) {
                mUpscaling = false;
                mRotateGame = false;
                nesRefreshDisplayOptions();
                return;
        }
        mUpscaling = shouldUpscale();
        mRotateGame = shouldRotateGame();
        if (mUpscaling)
                uiPrvUpscalerInit();
}

void nesPortInGameMenu(void)
{
        exitGame();
}

uint8_t gbExtGetKeys(void)      //arrow keys, f1=a, f2=b, f3=start, f4=select
{
        uint32_t sta = sio_hw->gpio_in;

        badgeLedsTick();
        
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
        #if (ZWT_ADDR & 3)              //byte
        
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

static void uiPrvGbUpscalerBuildMaps(void)
{
        uint32_t i;

        for (i = 0; i < GB_UPSCALED_WIDTH; i++) {
                uint32_t srcX = (i * GB_SRC_WIDTH + GB_UPSCALED_WIDTH / 2u) / GB_UPSCALED_WIDTH;

                if (srcX >= GB_SRC_WIDTH)
                        srcX = GB_SRC_WIDTH - 1u;
                mGbUpscaleSrcX[i] = (uint8_t)srcX;
        }

        for (i = 0; i <= GB_SRC_HEIGHT; i++)
                mGbUpscaleLineStart[i] = (uint16_t)((i * GB_UPSCALED_HEIGHT + GB_SRC_HEIGHT - 1u) / GB_SRC_HEIGHT);
}

static void uiPrvOutputGbAspectFitLine(uint16_t *fb, const PIXFMT *src, bool flipScaleMap)
{
        uint32_t i;

        for (i = 0; i < GB_UPSCALED_WIDTH; i++) {
                uint32_t mapIdx = flipScaleMap ? GB_UPSCALED_WIDTH - 1u - i : i;

                *fb = src[mGbUpscaleSrcX[mapIdx]] &~ BG_FLAG_UNDER_OBJS;

        #ifdef UPSCALER_ROTATES
                fb += DISP_WIDTH;
        #else
                fb++;
        #endif
        }
}

static void __attribute__((naked)) uiPrvHorizStretch(uint32_t *dst, uint16_t *src)
{
        asm volatile(
                "       push    {r4-r8, lr}                     \n\t"
                "       mov             r8, 0x00f800f8          \n\t"
                ".rept 80                                               \n\t"
                "       ldmia   r1!, {r4}                       \n\t"
                "       and             r2, r8, r4, lsr #8      \n\t"   //r
                "       and             r3, r8, r4, lsr #3      \n\t"   //g
                "       and             r4, r8, r4, lsl #3      \n\t"   //b

                "       add             r2, r2, r2, lsr #5      \n\t"   //extend R to 8 bits
                "       add             r3, r3, r3, lsr #5      \n\t"   //extend G to 8 bits
                "       add             r4, r4, r4, lsr #5      \n\t"   //extend B to 8 bits
        
                "       uhsax   r5, r2, r2                      \n\t"   //middle.r
                "       uhsax   r6, r3, r3                      \n\t"   //middle.r
                "       uhsax   r7, r4, r4                      \n\t"   //middle.r
                
                "       strb    r5,     [r0, #4]                \n\t"
                "       strb    r6,     [r0, #5]                \n\t"
                "       strb    r7,     [r0, #6]                \n\t"

                "       lsrs    r6, r2, #16                     \n\t"
                "       lsrs    r7, r3, #16                     \n\t"

                "       str             r4, [r0, #8]            \n\t"
                "       strb    r6, [r0, #8]            \n\t"
                "       strb    r7, [r0, #9]            \n\t"

                "       strb    r4,     [r0, #2]                \n\t"
                "       strb    r3,     [r0, #1]                \n\t"
                "       strb    r2,     [r0], #12               \n\t"

                ".endr                                                  \n\t"
                "       pop             {r4-r8, pc}                     \n\t"
                :
                :
                :"memory","cc"
        );
}

static void __attribute__((naked)) uiPrvMix(uint32_t *dst, uint32_t *src)
{
        asm volatile(
                "       push    {r4-r8, lr}                     \n\t"
                "       mov             r12, 240 / 4            \n\t"
                "1:                                                             \n\t"
                "       ldrd    r6, r7, [r0, #0]        \n\t"
                "       ldrd    r8, lr, [r0, #8]        \n\t"
                "       ldmia   r1!, {r2-r5}            \n\t"
                "       uhadd8  r2, r2, r6                      \n\t"
                "       uhadd8  r3, r3, r7                      \n\t"
                "       uhadd8  r4, r4, r8                      \n\t"
                "       uhadd8  r5, r5, lr                      \n\t"
                "       stmia   r0!, {r2-r5}            \n\t"
                "       subs    r12, #1                         \n\t"
                "       bne             1b                                      \n\t"
                "       pop             {r4-r8, pc}                     \n\t"
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
                        "       ldmia   %3!, {%2}                       \n\t"
                        "       uxtb    %0, %2                          \n\t"
                        "       smulwb  %0, %0, %4                      \n\t"   //r
                        "       ubfx    %1, %2, #8, #8          \n\t"
                        "       smulwb  %1, %1, %5                      \n\t"   //g
                        "       ubfx    %2, %2, #16, #8         \n\t"
                        "       smulwb  %2, %2, %4                      \n\t"   //g
                        "       add             %2, %2, %1, lsl #5      \n\t"
                        "       add             %2, %2, %0, lsl #11     \n\t"
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
                        "       uxtb    %0, %3                          \n\t"
                        "       smulwb  %0, %0, %4                      \n\t"   //r
                        "       ubfx    %1, %3, #8, #8          \n\t"
                        "       smulwb  %1, %1, %5                      \n\t"   //g
                        "       ubfx    %2, %3, #16, #8         \n\t"
                        "       smulwb  %2, %2, %4                      \n\t"   //g
                        "       add             %2, %2, %1, lsl #5      \n\t"
                        "       add             %2, %2, %0, lsl #11     \n\t"
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
                
                static PIXFMT __attribute__((aligned(4))) pixelsIn[GB_SRC_WIDTH];
                uint16_t *fb = dispGetFb();
                uint32_t *dataIn, sourceLineNum, dstLine, dstStart, dstEnd;
                
                dataIn = (uint32_t*)uiPrvFifoRx();
                
                memcpy(pixelsIn, (PIXFMT*)dataIn[0], sizeof(pixelsIn));
                sourceLineNum = dataIn[2];
                
                uiPrvFifoTx(0);

                dstStart = mGbUpscaleLineStart[sourceLineNum];
                dstEnd = mGbUpscaleLineStart[sourceLineNum + 1u];
                if (mRotateGame) {
                        uint32_t flippedStart = GB_UPSCALED_HEIGHT - dstEnd;
                        uint32_t flippedEnd = GB_UPSCALED_HEIGHT - dstStart;

                        dstStart = flippedStart;
                        dstEnd = flippedEnd;
                }

                for (dstLine = dstStart; dstLine < dstEnd; dstLine++) {
                #ifdef UPSCALER_ROTATES
                        fb = dispGetFb() + GB_UPSCALED_LEFT * DISP_WIDTH;
                        fb += DISP_WIDTH - 1u - (GB_UPSCALED_TOP + dstLine);
                #else
                        fb = dispGetFb() + DISP_WIDTH * (GB_UPSCALED_TOP + dstLine) + GB_UPSCALED_LEFT;
                #endif
                        uiPrvOutputGbAspectFitLine(fb, pixelsIn, mRotateGame);
                }
        }
}

static void uiPrvUpscalerInit(void)
{
        static uint32_t mUpscalerStack[32];
        uint32_t cmds[] = {0, 0, 1, SCB->VTOR, ((uintptr_t)mUpscalerStack) + sizeof(mUpscalerStack), (uintptr_t)&uiPrvUpscalerMain};
        uint32_t idx = 0;

        uiPrvGbUpscalerBuildMaps();
        
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
        PIXFMT flippedPixels[160];
        PIXFMT *linePixels = pixels;
        uint8_t outLineNum = lineNum;
        
        if (mRotateGame) {
                uint_fast16_t i;

                outLineNum = 143 - lineNum;
                for (i = 0; i < 160; i++)
                        flippedPixels[i] = pixels[159 - i];
                linePixels = flippedPixels;
        }

        if (!lineNum)
                dispPrvFrameCtrWait();


        if (mUpscaling) {

                uint32_t info[3] = {(uintptr_t)linePixels, outLineNum, lineNum};

                uiPrvFifoTx((uintptr_t)info);
                uiPrvFifoRx();  //wait for copied-out status
        }
        else {
        
                uint_fast8_t i;
                
                #ifdef UNSCALED_IMG_ROTATED
        
                //      fb += DISP_HEIGHT * (lineNum + (DISP_WIDTH - 144) / 2) + (DISP_HEIGHT - 160) / 2;
                        
                        fb += (DISP_WIDTH - 144) / 2;
                        fb += DISP_WIDTH * (DISP_HEIGHT - 160) / 2;
                        fb += 144 - outLineNum - 1;

                        
                        for (i = 0; i < 160; i++, fb += DISP_WIDTH)
                                *fb = *linePixels++ &~ BG_FLAG_UNDER_OBJS;

                #else

                        fb += DISP_WIDTH * (outLineNum + (DISP_HEIGHT - 144) / 2) + (DISP_WIDTH - 160) / 2;
                        
                        for (i = 0; i < 160; i++)
                                fb[i] = linePixels[i] &~ BG_FLAG_UNDER_OBJS;
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

static bool __attribute__((noinline)) shouldRotateGame(void)
{
        struct Settings settings;

        settingsGet(&settings);

        return settings.rotation;
}

static void applySavedLeds(void)
{
        struct Settings settings;

        settingsGet(&settings);
        badgeLedsApplySettings(&settings, true);
}

static void runSelectedGameTool(void *userData)
{
        struct GameSelection selection;
        uint32_t romSzExpected, ramSzExpected;

        (void)userData;
        mGameToolExitRequested = false;
        
        while(!mGameToolExitRequested) {

                if (!uiGetGameSelection(&selection)) {
                        pr("Failed to identify selected game\n");
                        mGameToolExitRequested = true;
                }
                else if (selection.runtime == GameRuntimeNes) {
                        dispPrvFrameCtrReset();
                        mUpscaling = false;
                        mRotateGame = false;
                        memset(dispGetFb(), 0, DISP_WIDTH * DISP_HEIGHT * DISP_BPP / 8);
                        mActiveGameRuntime = GameRuntimeNes;
                        nesRun((const void*)QSPI_ROM_START, selection.romSize, CART_RAM_ADDR_IN_RAM, selection.saveRamSize);
                        mActiveGameRuntime = GameRuntimeNone;
                }
                else if (!mbcInit((void*)QSPI_ROM_START, &romSzExpected, CART_RAM_ADDR_IN_RAM, &ramSzExpected)) {
                        pr("Failed to init the MBC\n");
                        mGameToolExitRequested = true;
                }
                else if (ramSzExpected > QSPI_RAM_SIZE_MAX) {
                        pr("too much ram needed\n");
                        mGameToolExitRequested = true;
                }
                else {
                        
                        dispPrvFrameCtrReset();
                        mUpscaling = shouldUpscale();
                        mRotateGame = shouldRotateGame();
                        if (mUpscaling) 
                                uiPrvUpscalerInit();
                        memset(dispGetFb(), 0, DISP_WIDTH * DISP_HEIGHT * DISP_BPP / 8);        
                        gbSetFrameDithering(1);
                        mActiveGameRuntime = GameRuntimeGb;
                        gbRun(shouldActAsCgb());
                        mActiveGameRuntime = GameRuntimeNone;
                        //if we are aborted by gbAbort, we'll return here and restart or leave the game tool
                }
        }
}

static void gpiosConfig(bool firstTime)
{
        static struct {
                uint8_t pin;
                uint8_t padCfg;
                uint8_t funcSel                 : 5;
                uint8_t oe                              : 1;
                uint8_t val                             : 1;
                uint8_t skipReconfig    : 1;    //do not reconfigure if not first call to gpiosConfig()
        } const cfgs[] = {
                {PIN_RAM_NCS, PADS_BANK0_GPIO0_SLEWFAST_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_XIP_SS_N_1, 1, 1},

                {PIN_LCD_DnC, 0, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 1, 0, 1},
                {PIN_LCD_SCK, PADS_BANK0_GPIO0_SLEWFAST_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO0_0, 1, 0, 1},
                {PIN_LCD_DO, PADS_BANK0_GPIO0_SLEWFAST_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO0_0, 1, 0, 1},
                {PIN_LCD_BL, 0, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PWM_A_0, 1, 0, 1},
                {PIN_LCD_CS, 0, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 1, 1, 1},

                {PIN_TOUCHINT, PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_IE_BITS, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0, 0, 0},

                {PIN_WS2812, 0, IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO1_0, 1, 0},

                {PIN_SD_MISO, PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS, IO_BANK0_GPIO12_CTRL_FUNCSEL_VALUE_SPI1_RX, 0, 0},
                {PIN_SD_NCS, PADS_BANK0_GPIO0_SLEWFAST_BITS, IO_BANK0_GPIO13_CTRL_FUNCSEL_VALUE_SIO_13, 1, 1},
                {PIN_SD_SCK, PADS_BANK0_GPIO0_SLEWFAST_BITS, IO_BANK0_GPIO14_CTRL_FUNCSEL_VALUE_SPI1_SCLK, 1, 0},
                {PIN_SD_MOSI, PADS_BANK0_GPIO0_SLEWFAST_BITS, IO_BANK0_GPIO15_CTRL_FUNCSEL_VALUE_SPI1_TX, 1, 0},

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

                success = i2cOneByteRegWrite(ACCEL_I2C_ADDR, 0x1f, 0xc0) && success;            //temp measurement on
                success = i2cOneByteRegWrite(ACCEL_I2C_ADDR, 0x20, 0x77) && success;            ///400Hz no-low power mode, all axes on
                success = i2cOneByteRegWrite(ACCEL_I2C_ADDR, 0x23, 0xd8) && success;            //update data when both halves read, BE, 4g scale, high res

                pr("ACCEL config success: %d\n", success);
        }
}

static void defconIoI2Cread16cbk(void *userData, const struct I2Creq *req, bool likelySuccess)
{
        uint_fast8_t shrBy = (uint_fast8_t)(uintptr_t)userData;

        if (!likelySuccess)
                mDefconExtraIoData[1] = 0xc2;   //done, error, error type 2
        else {

                uint_fast16_t val = 256 * mI2CreadBuf[0] + mI2CreadBuf[1];

                val >>= shrBy;
                val &= 0x0fff;

                mDefconExtraIoData[0] = val;
                mDefconExtraIoData[1] = 0x80 + (val >> 8);      //done, value
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

        asm volatile("":::"memory");    //sigh...
        if (!i2cTransact(&req, defconIoI2Cread16cbk, (void*)(uintptr_t)shrBy))
                mDefconExtraIoData[1] = 0xc1;   //done, error, error type 1
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

                if (val >> 8) {                                                                                                 //error causes a reset
                        mEEpos = 0;
                        continue;
                }
                
                if (val != match[mEEpos] && val != match[mEEpos = 0])                   //so does mismatch, but it might match [0]
                        continue;
                
                if (mEEpos != sizeof(match) - 1)                                                                //match advances
                        mEEpos++;
                else {

                        mEEpos = -1;
                        break;
                }
        }
}

void badgeIrdaInit(bool txMode)
{
        union UartCfg cfg = {
                .baudrate = 9600,
                .charBits = 3,  //8
                .stopBits = 1,  //1
                .parEna = 0,
                .rxEn = !txMode,
                .txEn = !!txMode,
        };

        mPrevIrdaModeWasTx = txMode;
        pr("irda config: rx=%d tx=%d\n", cfg.rxEn, cfg.txEn);

        iobank0_hw->io[PIN_IRDA_IN].ctrl = (iobank0_hw->io[PIN_IRDA_IN].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | (IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO1_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
        iobank0_hw->io[PIN_IRDA_OUT].ctrl = (iobank0_hw->io[PIN_IRDA_OUT].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | (IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO1_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
        iobank0_hw->io[PIN_IRDA_SD].ctrl = (iobank0_hw->io[PIN_IRDA_SD].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | (IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);

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


        badgeIrdaInit(true);
        for (i = 0; i < 8; i++) {
                
                irdaSIRuartTx(buf, sizeof(buf), true);
                while(!(irdaSIRuartGetSta() & UART_STA_BIT_TX_FIFO_EMPTY));
        }
        badgeIrdaInit(prevMode);
}

static void defconIoCmd(uint_fast8_t byte)
{
        static const uint8_t idStatic[16] = {0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x44, 0x6d, 0x69, 0x74, 0x72, 0x79, 0x2e, 0x47, 0x52, 0x00};
        uint_fast8_t cmd = byte >> 4, subCmd = byte & 0x0f;

        switch (cmd) {
                case 0 ... 8:                   //WRLEDx
                        if (subCmd < 3) {

                                pr("defcon led set 0x%02x -> (%u, %u)\n", mDefconExtraIoData[0], cmd, subCmd);
                                badgeLedsGameWrite();
                                ws2812Set(cmd, subCmd, mDefconExtraIoData[0]);
                        }
                        break;

                case 9:                                 //LEDSYNC
                        pr("leds refresh\n");
                        badgeLedsGameWrite();
                        ws2812refresh();
                        break;

                case 10:                                //RDPEN / RDIMU / RDADC
                        switch (subCmd) {
                                case 0 ... 3:   //RDPEN
                                        mDefconExtraIoData[1] = 0;
                                        defconIoI2Cread16(TOUCH_I2C_ADDR, 0xc0 + 0x10 * subCmd, 4);
                                        break;

                                case 4 ... 6:   //RDIMU
                                        mDefconExtraIoData[1] = 0;
                                        defconIoI2Cread16(ACCEL_I2C_ADDR, 0xa8 + 2 * (subCmd - 4), 8);
                                        break;

                                case 7 ... 9:   //RDADC
                                        mDefconExtraIoData[1] = 0;
                                        defconIoI2Cread16(ACCEL_I2C_ADDR, 0x88 + 2 * (subCmd - 7), 4);
                                        break;
                                
                                default:
                                        break;
                        }
                        break;

                case 11:                                //EMUCMD
                        switch (subCmd) {
                                case 0:                 //EMUEXIT
                                        exitGame();
                                        break;

                                case 1:                 //EMUOFF
                                        uiSaveSavestate();
                                        doSleep();
                                        break;

                                case 2:                 //SAVEFLUSH
                                        uiSaveSavestate();
                                        break;

                                default:
                                        break;
                        }
                        break;
                
                case 12:                                //IRCOMD
                        switch (subCmd) {
                                case 0:                 //power/mode control
                                        badgeIrdaInit(!!mDefconExtraIoData[0]);
                                        if (mDefconExtraIoData[0])
                                                mIrRxBytesUsed = 0;     //reset RX buffer if going to TX mode
                                        break;

                                case 1:                 //tx
                                        (void)irdaSIRuartTx((const uint8_t*)&mDefconExtraIoData[0], 1, true);
                                        break;

                                case 2:                 //rx
                                        if (!mIrRxBytesUsed)
                                                mDefconExtraIoData[1] = 0;
                                        else {
                                                uint_fast8_t rxPos = (mIrRxWritePos + sizeof(mIrRxBuf) / sizeof(*mIrRxBuf) - mIrRxBytesUsed) % (sizeof(mIrRxBuf) / sizeof(*mIrRxBuf));
                                                uint_fast16_t rxVal = mIrRxBuf[rxPos];
                                                bool haveMore, hadRxOverrun;

                                                asm volatile("cpsid i":::"memory");             //we could use ldres/stres or even disable the proper interrupt, but i am lazy and it is the 11th hour, so ... you'll live :)
                                                haveMore = !!--mIrRxBytesUsed;
                                                hadRxOverrun = mIrRxHadOverrun;
                                                mIrRxHadOverrun = 0;
                                                asm volatile("cpsie i":::"memory");
                                                
                                                mDefconExtraIoData[0] = rxVal;
                                                mDefconExtraIoData[1] = 0x01 + (haveMore ? 0x80 : 0) + ((hadRxOverrun || (rxVal & UART_BIT_MASK_RX_OVERRUN)) ? 0x40 : 0) + ((rxVal & (UART_BIT_MASK_PAR_ERR | UART_BIT_MASK_BRK_RXED | UART_BIT_MASK_FRM_ERR)) ? 0x02 : 0x00);
                                        }
                                        break;

                                case 3:                 //tx poll
                                        mDefconExtraIoData[0] = !(irdaSIRuartGetSta() & UART_STA_BIT_TX_FIFO_EMPTY);
                                        break;
                        }
                        break;

                case 15:                                //RDID
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
                case 0x7D:      //data [0]
                        ret = (uint32_t)mDefconExtraIoData[0];
                        break;
        
                case 0x7E:      //data [1]
                        ret = (uint32_t)mDefconExtraIoData[1];
                        break;

                case 0x7F:      //IDR
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
                case 0x7D:      //data [0]
                        mDefconExtraIoData[0] = data;
                        return true;

                case 0x7E:      //data [1]
                        mDefconExtraIoData[1] = data;
                        return true;
                
                case 0x7F:      //CTL
                        defconIoCmd(data);
                        return true;

                default:
                        return false;
        }
}

#define RTC_REG_CONTROL_STATUS_1        0x00
#define RTC_REG_VL_SECONDS             0x02
#define RTC_TIME_REG_COUNT             7
#define RTC_CTRL_STOP                  0x20
#define RTC_SECONDS_VL                 0x80
#define RTC_CENTURY_2000S              0x80

static bool rtcPrvIsLeapYear(uint_fast16_t year)
{
        return !(year & 3u);
}

uint_fast8_t badgeRtcDaysInMonth(uint_fast16_t year, uint_fast8_t month)
{
        static const uint8_t daysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        if (month < 1 || month > 12)
                return 31;
        if (month == 2 && rtcPrvIsLeapYear(year))
                return 29;
        return daysPerMonth[month - 1];
}

static bool rtcPrvDateTimeValid(const struct BadgeRtcDateTime *time)
{
        if (time->year < BADGE_RTC_MIN_YEAR || time->year > BADGE_RTC_MAX_YEAR)
                return false;
        if (time->month < 1 || time->month > 12)
                return false;
        if (time->day < 1 || time->day > badgeRtcDaysInMonth(time->year, time->month))
                return false;
        if (time->hour > 23 || time->minute > 59 || time->second > 59)
                return false;
        return true;
}

static bool rtcPrvFromBCD(uint8_t raw, uint8_t mask, uint_fast8_t min, uint_fast8_t max, uint8_t *valP)
{
        uint8_t val = raw & mask;
        uint8_t ret;

        if ((val & 0x0f) > 9 || ((val >> 4) & 0x0f) > 9)
                return false;
        ret = (val & 0x0f) + 10 * (val >> 4);
        if (ret < min || ret > max)
                return false;
        *valP = ret;
        return true;
}

static uint_fast8_t rtcPrvToBCD(uint_fast8_t val)
{
        return (val % 10) + 16 * (val / 10);
}

static uint32_t rtcPrvDateToDays(const struct BadgeRtcDateTime *time)
{
        uint32_t days = 0;
        uint_fast16_t year;
        uint_fast8_t month;

        for (year = BADGE_RTC_MIN_YEAR; year < time->year; year++)
                days += rtcPrvIsLeapYear(year) ? 366 : 365;
        for (month = 1; month < time->month; month++)
                days += badgeRtcDaysInMonth(time->year, month);
        return days + time->day - 1;
}

static bool rtcPrvDateTimeToSeconds(const struct BadgeRtcDateTime *time, uint32_t *secondsP)
{
        uint32_t days;

        if (!rtcPrvDateTimeValid(time))
                return false;
        days = rtcPrvDateToDays(time);
        *secondsP = days * 86400u + (uint32_t)time->hour * 3600u + (uint32_t)time->minute * 60u + time->second;
        return true;
}

static bool rtcPrvSecondsToDateTime(uint32_t seconds, struct BadgeRtcDateTime *timeP)
{
        uint32_t days = seconds / 86400u;
        uint_fast16_t year = BADGE_RTC_MIN_YEAR;
        uint_fast8_t month = 1;

        seconds %= 86400u;
        while (year <= BADGE_RTC_MAX_YEAR) {
                uint_fast16_t daysInYear = rtcPrvIsLeapYear(year) ? 366 : 365;

                if (days < daysInYear)
                        break;
                days -= daysInYear;
                year++;
        }
        if (year > BADGE_RTC_MAX_YEAR)
                return false;

        while (month <= 12) {
                uint_fast8_t daysInMonth = badgeRtcDaysInMonth(year, month);

                if (days < daysInMonth)
                        break;
                days -= daysInMonth;
                month++;
        }
        if (month > 12)
                return false;

        timeP->year = year;
        timeP->month = month;
        timeP->day = days + 1;
        timeP->hour = seconds / 3600u;
        seconds %= 3600u;
        timeP->minute = seconds / 60u;
        timeP->second = seconds % 60u;
        return true;
}

static uint_fast8_t rtcPrvWeekday(const struct BadgeRtcDateTime *time)
{
        return (6u + rtcPrvDateToDays(time)) % 7u;       //2000-01-01 was Saturday; PCF8563 uses Sunday == 0
}

static uint32_t rtcPrvGbSecondsForMode(uint32_t seconds, enum GbExtRtcMode mode)
{
        struct BadgeRtcDateTime time;

        if (mode != GbExtRtcModeDefcon32BadgeGame || !rtcPrvSecondsToDateTime(seconds, &time))
                return seconds;

        //DC32BadgeGame uses MBC3 days as DEF CON day index, with Thursday == 0.
        return (uint32_t)((rtcPrvWeekday(&time) + 3u) % 7u) * 86400u +
                (uint32_t)time.hour * 3600u + (uint32_t)time.minute * 60u + time.second;
}

static void rtcPrvResetGbRtc(uint32_t badgeSeconds, uint64_t sampleTicks)
{
        uint32_t gbSeconds = rtcPrvGbSecondsForMode(badgeSeconds, mGbRtcMode);

        mGbRtcTickOffset = sampleTicks - (uint64_t)gbSeconds * TICKS_PER_SECOND;
}

static uint32_t rtcPrvGetFromOffset(uint64_t tickOffset)
{
        return (getTime() - tickOffset + TICKS_PER_SECOND / 2) / TICKS_PER_SECOND;
}

static void rtcPrvCacheSeconds(uint32_t seconds, uint64_t sampleTicks, bool resetGbRtc)
{
        mRtcTickOffset = sampleTicks - (uint64_t)seconds * TICKS_PER_SECOND;
        if (resetGbRtc)
                rtcPrvResetGbRtc(seconds, sampleTicks);
        mRtcValid = true;
}

static bool rtcPrvReadDateTime(struct BadgeRtcDateTime *timeP, uint32_t *secondsP, bool resetGbRtc)
{
        uint8_t rtcVals[RTC_REG_VL_SECONDS + RTC_TIME_REG_COUNT];
        struct BadgeRtcDateTime time;
        uint32_t seconds;
        uint64_t sampleTicks = getTime();
        uint8_t year;

        if (!i2cRegRead(RTC_I2C_ADDR, RTC_REG_CONTROL_STATUS_1, rtcVals, sizeof(rtcVals))) {
                pr("RTC READ Fail\n");
                mRtcValid = false;
                return false;
        }

        if ((rtcVals[RTC_REG_CONTROL_STATUS_1] & RTC_CTRL_STOP) ||
                (rtcVals[RTC_REG_VL_SECONDS] & RTC_SECONDS_VL) ||
                !rtcPrvFromBCD(rtcVals[2], 0x7f, 0, 59, &time.second) ||
                !rtcPrvFromBCD(rtcVals[3], 0x7f, 0, 59, &time.minute) ||
                !rtcPrvFromBCD(rtcVals[4], 0x3f, 0, 23, &time.hour) ||
                !rtcPrvFromBCD(rtcVals[5], 0x3f, 1, 31, &time.day) ||
                (rtcVals[6] & 0x07) > 6 ||
                !rtcPrvFromBCD(rtcVals[7], 0x1f, 1, 12, &time.month) ||
                !rtcPrvFromBCD(rtcVals[8], 0xff, 0, 99, &year)) {

                pr("RTC invalid data\n");
                mRtcValid = false;
                return false;
        }

        time.year = BADGE_RTC_MIN_YEAR + year;
        if (!rtcPrvDateTimeToSeconds(&time, &seconds)) {
                pr("RTC invalid date\n");
                mRtcValid = false;
                return false;
        }

        rtcPrvCacheSeconds(seconds, sampleTicks, resetGbRtc);
        if (timeP)
                *timeP = time;
        if (secondsP)
                *secondsP = seconds;
        return true;
}

static int64_t rtcPrvReadU32(void)
{
        uint32_t seconds;

        if (!rtcPrvReadDateTime(NULL, &seconds, false))
                return -1;
        return seconds;
}

static bool rtcPrvWriteDateTime(const struct BadgeRtcDateTime *time)
{
        uint8_t rtcVals[RTC_TIME_REG_COUNT + 1];
        uint32_t seconds;

        if (!rtcPrvDateTimeToSeconds(time, &seconds))
                return false;
        if (!i2cOneByteRegWrite(RTC_I2C_ADDR, RTC_REG_CONTROL_STATUS_1, RTC_CTRL_STOP)) {
                pr("RTC stop fail\n");
                mRtcValid = false;
                return false;
        }

        rtcVals[0] = RTC_REG_VL_SECONDS;
        rtcVals[1] = rtcPrvToBCD(time->second);
        rtcVals[2] = rtcPrvToBCD(time->minute);
        rtcVals[3] = rtcPrvToBCD(time->hour);
        rtcVals[4] = rtcPrvToBCD(time->day);
        rtcVals[5] = rtcPrvWeekday(time);
        rtcVals[6] = RTC_CENTURY_2000S | rtcPrvToBCD(time->month);
        rtcVals[7] = rtcPrvToBCD(time->year - BADGE_RTC_MIN_YEAR);

        if (!i2cSimpleWrite(RTC_I2C_ADDR, rtcVals, sizeof(rtcVals))) {
                pr("RTC time write fail\n");
                mRtcValid = false;
                (void)i2cOneByteRegWrite(RTC_I2C_ADDR, RTC_REG_CONTROL_STATUS_1, 0);
                return false;
        }
        if (!i2cOneByteRegWrite(RTC_I2C_ADDR, RTC_REG_CONTROL_STATUS_1, 0)) {
                pr("RTC start fail\n");
                mRtcValid = false;
                return false;
        }

        rtcPrvCacheSeconds(seconds, getTime(), true);
        return true;
}

static void rtcInit(void)
{
        if (!rtcPrvReadDateTime(NULL, NULL, true)) {
                uint64_t currentTicks = getTime();

                pr("RTC comms/integrity error\n");
                mRtcTickOffset = currentTicks;
                mGbRtcTickOffset = mRtcTickOffset;
        }
}

bool badgeRtcIsValid(void)
{
        return mRtcValid;
}

uint32_t badgeRtcGet(void)
{
        return mRtcValid ? rtcPrvGetFromOffset(mRtcTickOffset) : 0;
}

bool badgeRtcGetTimeOfDay(uint_fast8_t *hourP, uint_fast8_t *minuteP, uint_fast8_t *secondP)
{
        struct BadgeRtcDateTime time;

        if (!badgeRtcGetDateTime(&time))
                return false;
        if (hourP)
                *hourP = time.hour;
        if (minuteP)
                *minuteP = time.minute;
        if (secondP)
                *secondP = time.second;
        return true;
}

bool badgeRtcGetDateTime(struct BadgeRtcDateTime *timeP)
{
        if (!mRtcValid || !timeP)
                return false;
        return rtcPrvSecondsToDateTime(badgeRtcGet(), timeP);
}

bool badgeRtcReadHardware(struct BadgeRtcDateTime *timeP)
{
        return rtcPrvReadDateTime(timeP, NULL, false);
}

bool badgeRtcSetDateTime(const struct BadgeRtcDateTime *timeP)
{
        return timeP && rtcPrvWriteDateTime(timeP);
}

uint32_t gbExtRtcGet(void)
{
        return rtcPrvGetFromOffset(mGbRtcTickOffset);
}

void gbExtRtcSet(uint32_t time)
{
        mGbRtcTickOffset = getTime() - (uint64_t)time * TICKS_PER_SECOND;
}

void gbExtRtcReset(enum GbExtRtcMode mode)
{
        uint64_t sampleTicks = getTime();

        mGbRtcMode = mode;
        if (mRtcValid)
                rtcPrvResetGbRtc(rtcPrvGetFromOffset(mRtcTickOffset), sampleTicks);
        else
                mGbRtcTickOffset = mRtcTickOffset;
}

static void gbExtAccelReadCbk(void *userData, const struct I2Creq *req, bool likelySuccess)
{
        uint16_t *data = (uint16_t*)userData;

        if (likelySuccess) {
                
                int16_t accelX = __builtin_bswap16(data[1]);
                int16_t accelY = -__builtin_bswap16(data[0]);
                uint16_t gameX =  0x8000 + accelX * 0x70 / 8192;        //middle 0x8000, one G = 0x70
                uint16_t gameY =  0x8000 + accelY * 0x70 / 8192;        //middle 0x8000, one G = 0x70

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

#define TEST_ID_TOUCH_COMMS             0x00000001
#define TEST_ID_RTC_COMMS               0x00000002
#define TEST_ID_RTC_ACCURACY    0x00000004
#define TEST_ID_IMU_COMMS               0x00000008
#define TEST_ID_IMU_VALS                0x00000010      //verifies magnitude aggs up to about gravity
#define TEST_ID_IRDA                    0x00000020
#define TEST_ID_POWER_SEQ               0x00000040
#define TEST_ID_PASSABLE                0x00000080      //all of the above are done and pass

#define TEST_ID_SOUNDS_CYCLED   0x00000100
#define TEST_ID_LEDS_CYCLED             0x00000200
#define TEST_ID_BUTTONS                 0x00000400

#define TESTS_FOR_STAGE_1               (TEST_ID_TOUCH_COMMS | TEST_ID_RTC_COMMS | TEST_ID_IMU_COMMS | TEST_ID_IMU_VALS | TEST_ID_IRDA)
#define TESTS_FOR_PASSABLE              (TEST_ID_TOUCH_COMMS | TEST_ID_RTC_COMMS | TEST_ID_IMU_COMMS | TEST_ID_IMU_VALS | TEST_ID_IRDA | TEST_ID_RTC_ACCURACY)

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

static uint16_t prvReadBe16(const uint8_t *val)
{
	return ((uint16_t)val[0] << 8) | val[1];
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

        uint32_t passedTests = 0, failedTests = 0;      //these are sticky, on purpose!!
        uint8_t adcVals[6], stage = 0;
        struct BadgePowerStatus powerStatus;
        struct Canvas cnv;
        bool doSelfTest;

        if (!badgePowerReadNow(&powerStatus))
                doSelfTest = 1;
        else {
                pr("BATT: %umV, USB: %umV\n", powerStatus.battMv, powerStatus.usbMv);
        
                if (powerStatus.usbMv < 4600) {
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
        doSelfTest |= uiGetKeys() == (KEY_BIT_START     | KEY_BIT_SEL);
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
                                                ledColor = 4;   //out of bounds on purpose
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

                                        if (x >= 0 && x < (int32_t)cnv.w && y >= 0 && y < (int32_t)cnv.h) {
                                                uiSelfTestSetText(&cnv, cnv.h - 1 - y, cnv.w - 1 - x, "*");
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

                                
                                if (badgePowerReadNow(&powerStatus)) {

                                        uint32_t battMv = powerStatus.battMv, busMv = powerStatus.usbMv;

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

                                                int_fast16_t x = (int16_t)prvReadBe16(adcVals + 0);
                                                int_fast16_t y = (int16_t)prvReadBe16(adcVals + 2);
                                                int_fast16_t z = (int16_t)prvReadBe16(adcVals + 4);
                                                uint_fast16_t magnitude;
                                                bool accuracyPass;

                                                magnitude = isqrt(x * x + y * y + z * z);
                                                accuracyPass = (magnitude >= 8192 * 8 / 10 && magnitude <= 8192 * 12 / 10);     //20% slop because this is a shitty accelerometer

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

                                stage = 1;              //all LEDs green to indicate end of stage 0

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
        bootGuardInit();
        timebaseInit();
        stackWatermarkInit();
        
        pr("ready, time is 0x%016llx\n", getTime());
        pr("ready, time is 0x%016llx\n", getTime());
        memoryReport();
        
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
        pll_sys_hw->pwr |= (PLL_PWR_VCOPD_BITS | PLL_PWR_POSTDIVPD_BITS | PLL_PWR_PD_BITS);             //dividers on
        pll_sys_hw->fbdiv_int = (pll_sys_hw->fbdiv_int &~ PLL_FBDIV_INT_BITS) | ((TICKS_PER_SECOND / 1000000 / 2) << PLL_FBDIV_INT_LSB);                //x100 = 1200MHz
        pll_sys_hw->prim = (pll_sys_hw->prim &~ (PLL_PRIM_POSTDIV1_BITS | PLL_PRIM_POSTDIV2_BITS)) | (6 << PLL_PRIM_POSTDIV1_LSB) | (1 << PLL_PRIM_POSTDIV2_LSB);
        pll_sys_hw->pwr &=~ (PLL_PWR_VCOPD_BITS | PLL_PWR_POSTDIVPD_BITS | PLL_PWR_PD_BITS);            //dividers on
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
        applySavedLeds();

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
        badgeIrdaInit(true);
        static const uint8_t tx[] = "hello";
        irdaSIRuartTx(tx, sizeof(tx), true);
        while (!(irdaSIRuartGetSta() & UART_STA_BIT_TX_FIFO_EMPTY));
        badgeIrdaInit(false);

        pr("USB HID prepare...\n");
        if (!usbHidPrepare())
                pr("USB HID prepare failed\n");

        uiPrvSelfTestsIfNeeded();

        pr("UI...\n");
        uiRunToolShell(runSelectedGameTool, NULL);

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
                        "tst  lr, #4                            \n\t"
                        "bne  1f                                \n\t"
                        "mrs  r0, msp                           \n\t"
                        "b    2f                                \n\t"
                        "1:                                     \n\t"
                        "mrs  r0, psp                           \n\t"
                        "2:                                     \n\t"
                        "mov  r1, lr                             \n\t"
                        "b    bootGuardCaptureHardFault          \n\t"
                        :::"memory");
}
