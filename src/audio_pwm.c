#include "pinoutRp2350defcon.h"
#include "timebase.h"
#include "2350.h"
#include "audio_pwm.h"

#define AUDIO_PWM_IDX	4
#define AUDIO_PWM_TOP	255

static uint64_t mNextSampleTime;
static uint32_t mTicksPerSample;
static uint32_t mSavedFunc;
static bool mRunning;

bool audioPwmStart(uint32_t sampleRate)
{
	if (!sampleRate)
		return false;

	mSavedFunc = iobank0_hw->io[PIN_SPQR].ctrl & IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS;
	iobank0_hw->io[PIN_SPQR].ctrl = (iobank0_hw->io[PIN_SPQR].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) |
		(IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PWM_A_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);

	pwm_hw->slice[AUDIO_PWM_IDX].csr &=~ PWM_CH0_CSR_EN_BITS;
	while (pwm_hw->slice[AUDIO_PWM_IDX].csr & PWM_CH0_CSR_EN_BITS);

	pwm_hw->slice[AUDIO_PWM_IDX].top = (pwm_hw->slice[AUDIO_PWM_IDX].top &~ PWM_CH0_TOP_BITS) | (AUDIO_PWM_TOP << PWM_CH0_TOP_LSB);
	pwm_hw->slice[AUDIO_PWM_IDX].ctr = 0;
	*(volatile uint16_t*)&pwm_hw->slice[AUDIO_PWM_IDX].cc = AUDIO_PWM_TOP / 2;
	pwm_hw->slice[AUDIO_PWM_IDX].div = (pwm_hw->slice[AUDIO_PWM_IDX].div &~ (PWM_CH0_DIV_INT_BITS | PWM_CH0_DIV_FRAC_BITS)) | (1 << PWM_CH0_DIV_INT_LSB);
	pwm_hw->slice[AUDIO_PWM_IDX].csr = (pwm_hw->slice[AUDIO_PWM_IDX].csr &~ (PWM_CH0_CSR_PH_ADV_BITS | PWM_CH0_CSR_PH_RET_BITS | PWM_CH0_CSR_DIVMODE_BITS | PWM_CH0_CSR_B_INV_BITS)) |
		(PWM_CH0_CSR_DIVMODE_VALUE_DIV << PWM_CH0_CSR_DIVMODE_LSB) | PWM_CH0_CSR_A_INV_BITS | PWM_CH0_CSR_EN_BITS;

	mTicksPerSample = (TICKS_PER_SECOND + sampleRate / 2) / sampleRate;
	mNextSampleTime = getTime() + mTicksPerSample;
	mRunning = true;
	return true;
}

void audioPwmWriteSample(int16_t sample)
{
	uint32_t duty;

	if (!mRunning)
		return;

	duty = (((int32_t)sample + 32768) * AUDIO_PWM_TOP) / 65535;
	*(volatile uint16_t*)&pwm_hw->slice[AUDIO_PWM_IDX].cc = duty;
}

void audioPwmWaitNext(void)
{
	uint64_t now;

	if (!mRunning)
		return;

	while ((now = getTime()) < mNextSampleTime);
	mNextSampleTime += mTicksPerSample;
	if (now > mNextSampleTime + mTicksPerSample * 8)
		mNextSampleTime = now + mTicksPerSample;
}

void audioPwmStop(void)
{
	if (!mRunning)
		return;

	*(volatile uint16_t*)&pwm_hw->slice[AUDIO_PWM_IDX].cc = 0;
	pwm_hw->slice[AUDIO_PWM_IDX].csr &=~ PWM_CH0_CSR_EN_BITS;
	iobank0_hw->io[PIN_SPQR].ctrl = (iobank0_hw->io[PIN_SPQR].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | mSavedFunc;
	mRunning = false;
}
