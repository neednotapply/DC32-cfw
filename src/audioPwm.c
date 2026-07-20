#include "pinoutRp2350defcon.h"
#include "timebase.h"
#include "2350.h"
#include "audioPwm.h"

#define AUDIO_PWM_IDX			((PIN_SPQR >> 1) & 7)
#define AUDIO_PWM_IS_B			(PIN_SPQR & 1)
#define AUDIO_TONE_DIV_FRAC_BITS	4u
#define AUDIO_TONE_DIV_MIN		(1u << AUDIO_TONE_DIV_FRAC_BITS)
#define AUDIO_TONE_DIV_MAX		((PWM_CH0_DIV_INT_BITS | PWM_CH0_DIV_FRAC_BITS) >> PWM_CH0_DIV_FRAC_LSB)
#define AUDIO_TONE_TOP_MAX		(PWM_CH0_TOP_BITS >> PWM_CH0_TOP_LSB)
#define AUDIO_PCM_CLK_DIV		1u
#define AUDIO_PCM_MIN_SAMPLE_RATE	8000u
#define AUDIO_PCM_MAX_SAMPLE_RATE	22050u
#define AUDIO_PCM_TOP			255u
#define AUDIO_PCM_TIMER_HZ		1000000u
#define AUDIO_PCM_TIMER_ALARM		0u
#define AUDIO_PCM_TIMER_MASK		(1u << AUDIO_PCM_TIMER_ALARM)
#define AUDIO_PCM_BUF_SIZE		1024u
#define AUDIO_PCM_BUF_MASK		(AUDIO_PCM_BUF_SIZE - 1u)
#define AUDIO_PWM_DISABLE_WAIT_MAX	100000

enum AudioPwmMode {
	AudioPwmModeStopped,
	AudioPwmModeTone,
	AudioPwmModePcm,
};

static uint32_t mToneDuty;
static uint32_t mPcmTop = AUDIO_PCM_TOP;
static uint32_t mPcmTimerInterval;
static uint32_t mPcmTimerNext;
static volatile uint16_t mPcmRead;
static volatile uint16_t mPcmWrite;
static uint8_t mPcmBuf[AUDIO_PCM_BUF_SIZE];
static volatile uint8_t mPcmSample = 128;
static uint8_t mVolume = AUDIO_PWM_VOLUME_MAX;
static uint8_t mMasterVolume = AUDIO_PWM_VOLUME_MAX;
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

static void audioPwmPrvPcmIrqDisable(void)
{
	timer0_hw->inte &=~ AUDIO_PCM_TIMER_MASK;
	timer0_hw->armed = AUDIO_PCM_TIMER_MASK;
	timer0_hw->intr = AUDIO_PCM_TIMER_MASK;
	NVIC_ClearPendingIRQ(TIMER0_IRQ_0_IRQn);
}

static uint32_t audioPwmPrvInvertBit(void)
{
	return AUDIO_PWM_IS_B ? PWM_CH0_CSR_B_INV_BITS : PWM_CH0_CSR_A_INV_BITS;
}

static void audioPwmPrvWriteDuty(uint32_t duty)
{
	uint32_t cc = pwm_hw->slice[AUDIO_PWM_IDX].cc;

	if (AUDIO_PWM_IS_B)
		cc = (cc & 0x0000ffffu) | ((duty & 0xffffu) << 16);
	else
		cc = (cc & 0xffff0000u) | (duty & 0xffffu);
	pwm_hw->slice[AUDIO_PWM_IDX].cc = cc;
}

static void audioPwmPrvClearDuty(void)
{
	pwm_hw->slice[AUDIO_PWM_IDX].cc = 0;
}

static uint8_t audioPwmPrvVolumeGain(uint_fast8_t volume)
{
	/*
	 * Keep the full-scale endpoint, but make the lower user-visible levels
	 * genuinely quiet.  Settings still exposes all 16 levels so the curve can
	 * be reviewed before choosing a smaller range.
	 */
	static const uint8_t volumeGain[AUDIO_PWM_VOLUME_MAX + 1] = {
		0, 1, 2, 3, 4, 6, 8, 11, 15, 21, 30, 43, 62, 91, 137, 255
	};

	return volumeGain[volume];
}

static void audioPwmPrvWriteToneDuty(void)
{
	uint32_t gain = ((uint32_t)audioPwmPrvVolumeGain(mVolume) *
		audioPwmPrvVolumeGain(mMasterVolume) + 127u) / 255u;

	audioPwmPrvWriteDuty((uint32_t)(((uint64_t)mToneDuty * gain * 336u) / (255u * 256u * 2u)));
}

static void audioPwmPrvWritePcmDuty(uint8_t sample)
{
	int32_t centered = (int32_t)sample - 128;
	int32_t top = (int32_t)mPcmTop;
	uint32_t gain = ((uint32_t)audioPwmPrvVolumeGain(mVolume) *
		audioPwmPrvVolumeGain(mMasterVolume) + 127u) / 255u;
	int32_t duty = top / 2 + (int32_t)(((int64_t)centered * (top / 2) * gain) / (128 * 255));

	if (duty < 0)
		duty = 0;
	if (duty > top)
		duty = top;
	audioPwmPrvWriteDuty((uint32_t)duty);
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
	else if (mMode == AudioPwmModePcm)
		audioPwmPrvWritePcmDuty(mPcmSample);
}

uint_fast8_t audioPwmGetVolume(void)
{
	return mVolume;
}

void audioPwmSetMasterVolume(uint_fast8_t volume)
{
	if (volume > AUDIO_PWM_VOLUME_MAX)
		volume = AUDIO_PWM_VOLUME_MAX;
	mMasterVolume = volume;
	if (mMode == AudioPwmModeTone) {
		if (mVolume && mMasterVolume)
			audioPwmPrvWriteToneDuty();
		else
			audioPwmStop();
	}
	else if (mMode == AudioPwmModePcm)
		audioPwmPrvWritePcmDuty(mPcmSample);
}

uint_fast8_t audioPwmGetMasterVolume(void)
{
	return mMasterVolume;
}

bool audioPwmTone(uint32_t frequency)
{
	uint64_t numerator, denominator;
	uint32_t divider, period;

	if (!frequency) {
		audioPwmStop();
		return true;
	}
	if (!mVolume || !mMasterVolume) {
		audioPwmStop();
		return true;
	}

	audioPwmPrvPcmIrqDisable();
	audioPwmPrvDisable();
	audioPwmPrvPinToPwm();

	/* The RP2350 PWM divider is 8.4 fixed point. Pick the smallest divider
	 * that keeps the period in 16 bits, maximizing pitch resolution. */
	numerator = (uint64_t)TICKS_PER_SECOND * (1u << AUDIO_TONE_DIV_FRAC_BITS);
	denominator = (uint64_t)frequency * AUDIO_TONE_TOP_MAX;
	divider = (uint32_t)((numerator + denominator - 1u) / denominator);
	if (divider < AUDIO_TONE_DIV_MIN)
		divider = AUDIO_TONE_DIV_MIN;
	if (divider > AUDIO_TONE_DIV_MAX)
		divider = AUDIO_TONE_DIV_MAX;
	denominator = (uint64_t)divider * frequency;
	period = (uint32_t)((numerator + denominator / 2u) / denominator);
	if (period < 2u)
		period = 2u;
	if (period > AUDIO_TONE_TOP_MAX)
		period = AUDIO_TONE_TOP_MAX;

	pwm_hw->slice[AUDIO_PWM_IDX].top = (pwm_hw->slice[AUDIO_PWM_IDX].top &~ PWM_CH0_TOP_BITS) |
		((period - 1u) << PWM_CH0_TOP_LSB);
	pwm_hw->slice[AUDIO_PWM_IDX].ctr = 0;
	mToneDuty = period;
	audioPwmPrvWriteToneDuty();
	pwm_hw->slice[AUDIO_PWM_IDX].div = (pwm_hw->slice[AUDIO_PWM_IDX].div &~ (PWM_CH0_DIV_INT_BITS | PWM_CH0_DIV_FRAC_BITS)) |
		(divider << PWM_CH0_DIV_FRAC_LSB);
	pwm_hw->slice[AUDIO_PWM_IDX].csr = (pwm_hw->slice[AUDIO_PWM_IDX].csr &~ (PWM_CH0_CSR_PH_ADV_BITS | PWM_CH0_CSR_PH_RET_BITS |
		PWM_CH0_CSR_DIVMODE_BITS | PWM_CH0_CSR_B_INV_BITS | PWM_CH0_CSR_A_INV_BITS | PWM_CH0_CSR_PH_CORRECT_BITS)) |
		(PWM_CH0_CSR_DIVMODE_VALUE_DIV << PWM_CH0_CSR_DIVMODE_LSB) | audioPwmPrvInvertBit() | PWM_CH0_CSR_EN_BITS;
	mMode = AudioPwmModeTone;
	return true;
}

static uint16_t audioPwmPrvPcmNext(uint16_t idx)
{
	return (uint16_t)((idx + 1u) & AUDIO_PCM_BUF_MASK);
}

bool audioPwmPcmStart(uint32_t sampleRate)
{
	if (sampleRate < AUDIO_PCM_MIN_SAMPLE_RATE || sampleRate > AUDIO_PCM_MAX_SAMPLE_RATE)
		return false;
	mPcmTimerInterval = (AUDIO_PCM_TIMER_HZ + sampleRate / 2u) / sampleRate;
	if (!mPcmTimerInterval)
		return false;

	audioPwmPrvPcmIrqDisable();
	audioPwmPrvDisable();
	audioPwmPrvPinToPwm();

	mPcmTop = AUDIO_PCM_TOP;
	mPcmRead = 0;
	mPcmWrite = 0;
	pwm_hw->slice[AUDIO_PWM_IDX].top = (pwm_hw->slice[AUDIO_PWM_IDX].top &~ PWM_CH0_TOP_BITS) |
		(mPcmTop << PWM_CH0_TOP_LSB);
	pwm_hw->slice[AUDIO_PWM_IDX].ctr = 0;
	mPcmSample = 128;
	audioPwmPrvWritePcmDuty(mPcmSample);
	pwm_hw->slice[AUDIO_PWM_IDX].div = (pwm_hw->slice[AUDIO_PWM_IDX].div &~ (PWM_CH0_DIV_INT_BITS | PWM_CH0_DIV_FRAC_BITS)) |
		(AUDIO_PCM_CLK_DIV << PWM_CH0_DIV_INT_LSB);
	mMode = AudioPwmModePcm;
	timer0_hw->intr = AUDIO_PCM_TIMER_MASK;
	timer0_hw->inte |= AUDIO_PCM_TIMER_MASK;
	NVIC_SetPriority(TIMER0_IRQ_0_IRQn, 2);
	NVIC_ClearPendingIRQ(TIMER0_IRQ_0_IRQn);
	NVIC_EnableIRQ(TIMER0_IRQ_0_IRQn);
	mPcmTimerNext = timer0_hw->timerawl + mPcmTimerInterval;
	timer0_hw->alarm[AUDIO_PCM_TIMER_ALARM] = mPcmTimerNext;
	pwm_hw->slice[AUDIO_PWM_IDX].csr = (pwm_hw->slice[AUDIO_PWM_IDX].csr &~ (PWM_CH0_CSR_PH_ADV_BITS | PWM_CH0_CSR_PH_RET_BITS |
		PWM_CH0_CSR_DIVMODE_BITS | PWM_CH0_CSR_B_INV_BITS | PWM_CH0_CSR_A_INV_BITS | PWM_CH0_CSR_PH_CORRECT_BITS)) |
		(PWM_CH0_CSR_DIVMODE_VALUE_DIV << PWM_CH0_CSR_DIVMODE_LSB) | audioPwmPrvInvertBit() | PWM_CH0_CSR_EN_BITS;
	return true;
}

bool audioPwmPcmCanWrite(void)
{
	if (mMode != AudioPwmModePcm)
		return false;
	return audioPwmPrvPcmNext(mPcmWrite) != mPcmRead;
}

bool audioPwmPcmWriteU8(uint8_t sample)
{
	uint16_t write, next;

	if (mMode != AudioPwmModePcm)
		return false;
	write = mPcmWrite;
	next = audioPwmPrvPcmNext(write);
	if (next == mPcmRead)
		return false;
	mPcmBuf[write] = sample;
	mPcmWrite = next;
	return true;
}

void audioPwmPcmDrain(void)
{
	uint64_t start = getTime();

	while (mMode == AudioPwmModePcm && mPcmRead != mPcmWrite && getTime() - start < TICKS_PER_SECOND / 2u);
}

void audioPwmPcmStop(void)
{
	if (mMode == AudioPwmModePcm)
		audioPwmStop();
}

void TIMER0_IRQ_0_IRQHandler(void)
{
	uint8_t sample = 128;
	uint16_t read;
	uint32_t now;

	if (!(timer0_hw->ints & AUDIO_PCM_TIMER_MASK))
		return;
	timer0_hw->intr = AUDIO_PCM_TIMER_MASK;
	if (mMode != AudioPwmModePcm)
		return;

	now = timer0_hw->timerawl;
	mPcmTimerNext += mPcmTimerInterval;
	if ((int32_t)(mPcmTimerNext - now) <= 0)
		mPcmTimerNext = now + mPcmTimerInterval;
	timer0_hw->alarm[AUDIO_PCM_TIMER_ALARM] = mPcmTimerNext;

	read = mPcmRead;
	if (read != mPcmWrite) {
		sample = mPcmBuf[read];
		mPcmRead = audioPwmPrvPcmNext(read);
	}
	mPcmSample = sample;
	audioPwmPrvWritePcmDuty(sample);
}

void audioPwmStop(void)
{
	audioPwmPrvPcmIrqDisable();
	if (mMode == AudioPwmModeStopped)
		return;

	audioPwmPrvDisable();
	audioPwmPrvClearDuty();
	audioPwmPrvPinLow();
	mMode = AudioPwmModeStopped;
}
