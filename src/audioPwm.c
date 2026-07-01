#include "pinoutRp2350defcon.h"
#include "timebase.h"
#include "2350.h"
#include "audioPwm.h"

#define AUDIO_PWM_IDX			((PIN_SPQR >> 1) & 7)
#define AUDIO_PWM_IS_B			(PIN_SPQR & 1)
#define AUDIO_TONE_CLK_DIV		32
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

static void audioPwmPrvWriteToneDuty(void)
{
	static const uint16_t toneGain[AUDIO_PWM_VOLUME_MAX + 1] = {
		0, 16, 24, 34, 46, 60, 76, 94, 114, 136, 160, 188, 220, 256, 294, 336
	};

	uint_fast8_t effective = (mVolume * mMasterVolume + AUDIO_PWM_VOLUME_MAX / 2u) /
		AUDIO_PWM_VOLUME_MAX;

	audioPwmPrvWriteDuty((mToneDuty * toneGain[effective]) / (256 * 2));
}

static void audioPwmPrvWritePcmDuty(uint8_t sample)
{
	int32_t centered = (int32_t)sample - 128;
	int32_t top = (int32_t)mPcmTop;
	int32_t duty = top / 2 + (centered * (top / 2) * (int32_t)mVolume *
		(int32_t)mMasterVolume) /
		(128 * AUDIO_PWM_VOLUME_MAX * AUDIO_PWM_VOLUME_MAX);

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

bool audioPwmTone(uint32_t freq)
{
	uint32_t duty, baseFreq = TICKS_PER_SECOND / AUDIO_TONE_CLK_DIV;

	if (!freq) {
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
