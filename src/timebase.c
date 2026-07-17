/*
	(c) 2021 Dmitry Grinberg   https://dmitry.gr
	Non-commercial use only OR licensing@dmitry.gr
*/

#include "2350.h"
#include <stdbool.h>


#include "timebase.h"
#include "badgeLeds.h"
#include "printf.h"

#define SYSTICK_BITS		24
#define IDLE_TIMER_ALARM	1u
#define IDLE_TIMER_MASK		(1u << IDLE_TIMER_ALARM)

//These clock destinations are unused while the CPU is in the bounded menu
//wait. Keep clocks for scanout DMA/PIO, audio PWM, USB, SD, XIP/SRAM, GPIO,
//TIMER0 and the tick generators running so active peripherals are not paused.
#define IDLE_SLEEP_GATE0	(CLOCKS_SLEEP_EN0_CLK_SYS_SHA256_BITS | \
	CLOCKS_SLEEP_EN0_CLK_SYS_ROSC_BITS | \
	CLOCKS_SLEEP_EN0_CLK_SYS_ROM_BITS | \
	CLOCKS_SLEEP_EN0_CLK_SYS_POWMAN_BITS | \
	CLOCKS_SLEEP_EN0_CLK_REF_POWMAN_BITS | \
	CLOCKS_SLEEP_EN0_CLK_SYS_PLL_USB_BITS | \
	CLOCKS_SLEEP_EN0_CLK_SYS_PIO2_BITS | \
	CLOCKS_SLEEP_EN0_CLK_SYS_OTP_BITS | \
	CLOCKS_SLEEP_EN0_CLK_REF_OTP_BITS | \
	CLOCKS_SLEEP_EN0_CLK_SYS_JTAG_BITS | \
	CLOCKS_SLEEP_EN0_CLK_SYS_I2C1_BITS | \
	CLOCKS_SLEEP_EN0_CLK_SYS_I2C0_BITS | \
	CLOCKS_SLEEP_EN0_CLK_SYS_HSTX_BITS | \
	CLOCKS_SLEEP_EN0_CLK_HSTX_BITS | \
	CLOCKS_SLEEP_EN0_CLK_SYS_GLITCH_DETECTOR_BITS | \
	CLOCKS_SLEEP_EN0_CLK_SYS_BOOTRAM_BITS | \
	CLOCKS_SLEEP_EN0_CLK_SYS_ADC_BITS | \
	CLOCKS_SLEEP_EN0_CLK_ADC_ADC_BITS)

#define IDLE_SLEEP_GATE1	(CLOCKS_SLEEP_EN1_CLK_SYS_UART1_BITS | \
	CLOCKS_SLEEP_EN1_CLK_PERI_UART1_BITS | \
	CLOCKS_SLEEP_EN1_CLK_SYS_TRNG_BITS | \
	CLOCKS_SLEEP_EN1_CLK_SYS_TIMER1_BITS | \
	CLOCKS_SLEEP_EN1_CLK_SYS_TBMAN_BITS | \
	CLOCKS_SLEEP_EN1_CLK_SYS_SYSINFO_BITS | \
	CLOCKS_SLEEP_EN1_CLK_SYS_SYSCFG_BITS | \
	CLOCKS_SLEEP_EN1_CLK_SYS_SPI0_BITS | \
	CLOCKS_SLEEP_EN1_CLK_PERI_SPI0_BITS)

static uint32_t mTicks = 0;
static volatile bool mIdleAlarmFired;


void timebaseInit(void)
{
	//Clear any sleep masks retained across a watchdog handoff from an older image.
	//This firmware does not modify them again at runtime.
	clocks_hw->sleep_en0 = CLOCKS_SLEEP_EN0_RESET;
	clocks_hw->sleep_en1 = CLOCKS_SLEEP_EN1_RESET;
	SysTick->CTRL = 0;
	
	NVIC_SetPriority(SysTick_IRQn, 1);
	
	//setup SysTick
	SysTick->CTRL = 0;
	SysTick->LOAD = ((1 << SYSTICK_BITS) - 1);
	SysTick->VAL = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void)
{
	mTicks++;
	badgeLedsWatchdogTick();
}

void TIMER0_IRQ_1_IRQHandler(void)
{
	timer0_hw->intr = IDLE_TIMER_MASK;
	mIdleAlarmFired = true;
}

uint64_t getTime(void)
{
	uint32_t hi, lo;

	do {
		hi = mTicks;
		asm volatile("":::"memory");
		lo = SysTick->VAL;
		asm volatile("":::"memory");
	} while (hi != mTicks);

	return (((uint64_t)hi) << SYSTICK_BITS) + (((1 << SYSTICK_BITS) - 1) - lo);
}

void delayMsec(uint32_t msec)
{
	uint64_t till = getTime() + (uint64_t)msec * (TICKS_PER_SECOND / 1000);
	
	while (getTime() < till);
}

void delayUsec(uint32_t usec)
{
	uint64_t till = getTime() + (uint64_t)usec * (TICKS_PER_SECOND / 1000000);
	
	while (getTime() < till);
}

void timebaseIdleWaitMsec(uint32_t msec)
{
	uint64_t deadline;
	uint32_t savedSleepEn0, savedSleepEn1;

	if (!msec)
		return;

	deadline = getTime() + (uint64_t)msec * (TICKS_PER_SECOND / 1000u);
	mIdleAlarmFired = false;
	NVIC_DisableIRQ(TIMER0_IRQ_1_IRQn);
	timer0_hw->inte &=~ IDLE_TIMER_MASK;
	timer0_hw->armed = IDLE_TIMER_MASK;
	timer0_hw->intr = IDLE_TIMER_MASK;
	NVIC_ClearPendingIRQ(TIMER0_IRQ_1_IRQn);
	timer0_hw->alarm[IDLE_TIMER_ALARM] = timer0_hw->timerawl + msec * 1000u;
	timer0_hw->inte |= IDLE_TIMER_MASK;
	NVIC_SetPriority(TIMER0_IRQ_1_IRQn, 3);
	NVIC_EnableIRQ(TIMER0_IRQ_1_IRQn);

	savedSleepEn0 = clocks_hw->sleep_en0;
	savedSleepEn1 = clocks_hw->sleep_en1;
	clocks_hw->sleep_en0 = savedSleepEn0 &~ IDLE_SLEEP_GATE0;
	clocks_hw->sleep_en1 = savedSleepEn1 &~ IDLE_SLEEP_GATE1;
	__DSB();
	while (!mIdleAlarmFired && getTime() < deadline)
		__WFI();
	clocks_hw->sleep_en0 = savedSleepEn0;
	clocks_hw->sleep_en1 = savedSleepEn1;
	__DSB();

	NVIC_DisableIRQ(TIMER0_IRQ_1_IRQn);
	timer0_hw->inte &=~ IDLE_TIMER_MASK;
	timer0_hw->armed = IDLE_TIMER_MASK;
	timer0_hw->intr = IDLE_TIMER_MASK;
}
