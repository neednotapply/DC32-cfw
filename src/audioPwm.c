#include "pinoutRp2350defcon.h"
#include "timebase.h"
#include "2350.h"
#include "audioPwm.h"

#define AUDIO_PWM_IDX			((PIN_SPQR >> 1) & 7)
#define AUDIO_PWM_IS_B			(PIN_SPQR & 1)
#define AUDIO_PWM_TOP			255
#define AUDIO_TONE_CLK_DIV		32

enum AudioPwmMode {
	AudioPwmModeStopped,
	AudioPwmModePcm,
	AudioPwmModeTone,
};

static uint64_t mNextSampleTime;
static uint32_t mTicksPerSample;
static enum AudioPwmMode mMode;

static void audioPwmPrvPinToPwm(void)
{
	iobank0_hw->io[PIN_SPQR].ctrl = (iobank0_hw->io[PIN_SPQR].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) |
		(IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PWM_A_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
	sio_hw->gpio_oe_set = 1 << PIN_SPQR;
}

static void audioPwmPrvPinLow(void)
{
	iobank0_hw->io[PIN_SPQR].ctrl = (iobank0_hw->io[PIN_SPQR].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) |
		(IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
	sio_hw->gpio_clr = 1 << PIN_SPQR;
	sio_hw->gpio_oe_set = 1 << PIN_SPQR;
}

static void audioPwmPrvDisable(void)
{
	pwm_hw->slice[AUDIO_PWM_IDX].csr &=~ PWM_CH0_CSR_EN_BITS;
	while (pwm_hw->slice[AUDIO_PWM_IDX].csr & PWM_CH0_CSR_EN_BITS);
}

static uint32_t audioPwmPrvInvertBit(void)
{
	return AUDIO_PWM_IS_B ? PWM_CH0_CSR_B_INV_BITS : PWM_CH0_CSR_A_INV_BITS;
}

static void audioPwmPrvWriteDuty(uint32_t duty)
{
	if (AUDIO_PWM_IS_B)
		pwm_hw->slice[AUDIO_PWM_IDX].cc = (pwm_hw->slice[AUDIO_PWM_IDX].cc &~ PWM_CH0_CC_B_BITS) | (duty << PWM_CH0_CC_B_LSB);
	else
		pwm_hw->slice[AUDIO_PWM_IDX].cc = (pwm_hw->slice[AUDIO_PWM_IDX].cc &~ PWM_CH0_CC_A_BITS) | (duty << PWM_CH0_CC_A_LSB);
}

bool audioPwmStart(uint32_t sampleRate)
{
	if (!sampleRate)
		return false;

	audioPwmPrvDisable();
	audioPwmPrvPinToPwm();

	pwm_hw->slice[AUDIO_PWM_IDX].top = (pwm_hw->slice[AUDIO_PWM_IDX].top &~ PWM_CH0_TOP_BITS) | (AUDIO_PWM_TOP << PWM_CH0_TOP_LSB);
	pwm_hw->slice[AUDIO_PWM_IDX].ctr = 0;
	audioPwmPrvWriteDuty(AUDIO_PWM_TOP / 2);
	pwm_hw->slice[AUDIO_PWM_IDX].div = (pwm_hw->slice[AUDIO_PWM_IDX].div &~ (PWM_CH0_DIV_INT_BITS | PWM_CH0_DIV_FRAC_BITS)) | (1 << PWM_CH0_DIV_INT_LSB);
	pwm_hw->slice[AUDIO_PWM_IDX].csr = (pwm_hw->slice[AUDIO_PWM_IDX].csr &~ (PWM_CH0_CSR_PH_ADV_BITS | PWM_CH0_CSR_PH_RET_BITS |
		PWM_CH0_CSR_DIVMODE_BITS | PWM_CH0_CSR_B_INV_BITS | PWM_CH0_CSR_A_INV_BITS | PWM_CH0_CSR_PH_CORRECT_BITS)) |
		(PWM_CH0_CSR_DIVMODE_VALUE_DIV << PWM_CH0_CSR_DIVMODE_LSB) | audioPwmPrvInvertBit() | PWM_CH0_CSR_EN_BITS;

	mTicksPerSample = (TICKS_PER_SECOND + sampleRate / 2) / sampleRate;
	mNextSampleTime = getTime() + mTicksPerSample;
	mMode = AudioPwmModePcm;
	return true;
}

void audioPwmWriteSample(int16_t sample)
{
	uint32_t duty;

	if (mMode != AudioPwmModePcm)
		return;

	duty = (((int32_t)sample + 32768) * AUDIO_PWM_TOP) / 65535;
	audioPwmPrvWriteDuty(duty);
}

void audioPwmWaitNext(void)
{
	uint64_t now;

	if (mMode != AudioPwmModePcm)
		return;

	while ((now = getTime()) < mNextSampleTime);
	mNextSampleTime += mTicksPerSample;
	if (now > mNextSampleTime + mTicksPerSample * 8)
		mNextSampleTime = now + mTicksPerSample;
}

bool audioPwmTone(uint32_t freq)
{
	uint32_t duty, baseFreq = TICKS_PER_SECOND / AUDIO_TONE_CLK_DIV;

	if (!freq) {
		audioPwmStop();
		return true;
	}

	audioPwmPrvDisable();
	audioPwmPrvPinToPwm();

	duty = (baseFreq + freq / 2) / freq;
	if (duty < 2)
		duty = 2;
	if (duty > PWM_CH0_TOP_BITS)
		duty = PWM_CH0_TOP_BITS;

	pwm_hw->slice[AUDIO_PWM_IDX].top = (pwm_hw->slice[AUDIO_PWM_IDX].top &~ PWM_CH0_TOP_BITS) | ((duty - 1) << PWM_CH0_TOP_LSB);
	pwm_hw->slice[AUDIO_PWM_IDX].ctr = 0;
	audioPwmPrvWriteDuty(duty / 2);
	pwm_hw->slice[AUDIO_PWM_IDX].div = (pwm_hw->slice[AUDIO_PWM_IDX].div &~ (PWM_CH0_DIV_INT_BITS | PWM_CH0_DIV_FRAC_BITS)) |
		(AUDIO_TONE_CLK_DIV << PWM_CH0_DIV_INT_LSB);
	pwm_hw->slice[AUDIO_PWM_IDX].csr = (pwm_hw->slice[AUDIO_PWM_IDX].csr &~ (PWM_CH0_CSR_PH_ADV_BITS | PWM_CH0_CSR_PH_RET_BITS |
		PWM_CH0_CSR_DIVMODE_BITS | PWM_CH0_CSR_B_INV_BITS | PWM_CH0_CSR_A_INV_BITS | PWM_CH0_CSR_PH_CORRECT_BITS)) |
		(PWM_CH0_CSR_DIVMODE_VALUE_DIV << PWM_CH0_CSR_DIVMODE_LSB) | audioPwmPrvInvertBit() | PWM_CH0_CSR_EN_BITS;
	mMode = AudioPwmModeTone;
	return true;
}

void audioPwmStop(void)
{
	if (mMode == AudioPwmModeStopped)
		return;

	audioPwmPrvDisable();
	audioPwmPrvWriteDuty(0);
	audioPwmPrvPinLow();
	mMode = AudioPwmModeStopped;
}
