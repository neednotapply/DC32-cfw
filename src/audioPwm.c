#include "pinoutRp2350defcon.h"
#include "timebase.h"
#include "2350.h"
#include "audioPwm.h"

#define AUDIO_PWM_IDX			((PIN_SPQR >> 1) & 7)
#define AUDIO_PWM_IS_B			(PIN_SPQR & 1)
#define AUDIO_TONE_CLK_DIV		32
#define AUDIO_PWM_DISABLE_WAIT_MAX	100000

enum AudioPwmMode {
	AudioPwmModeStopped,
	AudioPwmModeTone,
};

static uint32_t mToneDuty;
static uint8_t mVolume = AUDIO_PWM_VOLUME_MAX;
static volatile enum AudioPwmMode mMode;

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
	uint32_t waits = 0;

	pwm_hw->slice[AUDIO_PWM_IDX].csr &=~ PWM_CH0_CSR_EN_BITS;
	while ((pwm_hw->slice[AUDIO_PWM_IDX].csr & PWM_CH0_CSR_EN_BITS) && waits++ < AUDIO_PWM_DISABLE_WAIT_MAX);
}

static uint32_t audioPwmPrvInvertBit(void)
{
	return AUDIO_PWM_IS_B ? PWM_CH0_CSR_B_INV_BITS : PWM_CH0_CSR_A_INV_BITS;
}

static void audioPwmPrvWriteDuty(uint32_t duty)
{
	volatile uint16_t *cc = (volatile uint16_t*)&pwm_hw->slice[AUDIO_PWM_IDX].cc;

	if (AUDIO_PWM_IS_B)
		cc[1] = (uint16_t)duty;
	else
		cc[0] = (uint16_t)duty;
}

static void audioPwmPrvWriteToneDuty(void)
{
	static const uint16_t toneGain[AUDIO_PWM_VOLUME_MAX + 1] = {
		0, 16, 24, 34, 46, 60, 76, 94, 114, 136, 160, 188, 220, 256, 294, 336
	};

	audioPwmPrvWriteDuty((mToneDuty * toneGain[mVolume]) / (256 * 2));
}

void audioPwmSetVolume(uint_fast8_t volume)
{
	if (volume > AUDIO_PWM_VOLUME_MAX)
		volume = AUDIO_PWM_VOLUME_MAX;
	mVolume = volume;
	if (mMode == AudioPwmModeTone) {
		if (mVolume)
			audioPwmPrvWriteToneDuty();
		else
			audioPwmStop();
	}
}

uint_fast8_t audioPwmGetVolume(void)
{
	return mVolume;
}

bool audioPwmTone(uint32_t freq)
{
	uint32_t duty, baseFreq = TICKS_PER_SECOND / AUDIO_TONE_CLK_DIV;

	if (!freq) {
		audioPwmStop();
		return true;
	}
	if (!mVolume) {
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
	mToneDuty = duty;
	audioPwmPrvWriteToneDuty();
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
