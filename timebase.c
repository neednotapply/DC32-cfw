/*
	(c) 2021 Dmitry Grinberg   https://dmitry.gr
	Non-commercial use only OR licensing@dmitry.gr
*/

#include "2350.h"


#include "timebase.h"
#include "printf.h"

#define SYSTICK_BITS		24

static uint32_t mTicks = 0;


void timebaseInit(void)
{
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