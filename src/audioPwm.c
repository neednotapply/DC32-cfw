#include "pinoutRp2350defcon.h"
#include "timebase.h"
#include "2350.h"
#include "audioPwm.h"

#define AUDIO_PWM_IDX			((PIN_SPQR >> 1) & 7)
#define AUDIO_PWM_IS_B			(PIN_SPQR & 1)
#define AUDIO_PWM_TOP			255
#define AUDIO_TONE_CLK_DIV		32
#define AUDIO_PCM_TIMER			timer0_hw
#define AUDIO_PCM_ALARM			3
#define AUDIO_PCM_ALARM_BIT		(1u << AUDIO_PCM_ALARM)
#define AUDIO_PCM_IRQ			TIMER0_IRQ_3_IRQn
#define AUDIO_PCM_US_PER_SEC	1000000U
#define AUDIO_PCM_BUF_LEN		1024

enum AudioPwmMode {
	AudioPwmModeStopped,
	AudioPwmModePcm,
	AudioPwmModeTone,
};

static volatile uint16_t mPcmReadIdx;
static volatile uint16_t mPcmWriteIdx;
static volatile uint16_t mPcmQueued;
static int16_t mPcmBuf[AUDIO_PCM_BUF_LEN];
static uint32_t mPcmSampleRate;
static uint32_t mPcmAlarmPhase;
static uint32_t mPcmNextAlarm;
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

static void audioPwmPrvWriteToneDuty(void)
{
	static const uint16_t toneGain[AUDIO_PWM_VOLUME_MAX + 1] = {
		0, 16, 24, 34, 46, 60, 76, 94, 114, 136, 160, 188, 220, 256, 294, 336
	};

	audioPwmPrvWriteDuty((mToneDuty * toneGain[mVolume]) / (256 * 2));
}

static int16_t audioPwmPrvScaleSample(int16_t sample)
{
	static const uint16_t pcmGain[AUDIO_PWM_VOLUME_MAX + 1] = {
		0, 8, 14, 22, 32, 45, 62, 84, 112, 148, 190, 238, 294, 360, 436, 512
	};
	int32_t scaled = ((int32_t)sample * pcmGain[mVolume]) / 256;

	if (scaled > 32767)
		return 32767;
	if (scaled < -32768)
		return -32768;
	return scaled;
}

static void audioPwmPrvWriteSampleNow(int16_t sample)
{
	uint32_t duty;

	sample = audioPwmPrvScaleSample(sample);
	duty = (((int32_t)sample + 32768) * AUDIO_PWM_TOP) / 65535;
	audioPwmPrvWriteDuty(duty);
}

static void audioPwmPrvArmNextPcmAlarm(void)
{
	uint32_t delta;

	mPcmAlarmPhase += AUDIO_PCM_US_PER_SEC;
	delta = mPcmAlarmPhase / mPcmSampleRate;
	mPcmAlarmPhase -= delta * mPcmSampleRate;
	if (!delta)
		delta = 1;
	mPcmNextAlarm += delta;
	while ((int32_t)(mPcmNextAlarm - AUDIO_PCM_TIMER->timerawl) <= 0)
		mPcmNextAlarm += delta;
	AUDIO_PCM_TIMER->alarm[AUDIO_PCM_ALARM] = mPcmNextAlarm;
}

void TIMER0_IRQ_3_IRQHandler(void)
{
	int16_t sample = 0;

	AUDIO_PCM_TIMER->intr = AUDIO_PCM_ALARM_BIT;
	if (mMode != AudioPwmModePcm)
		return;

	if (mPcmQueued) {
		sample = mPcmBuf[mPcmReadIdx++];
		if (mPcmReadIdx == AUDIO_PCM_BUF_LEN)
			mPcmReadIdx = 0;
		mPcmQueued--;
	}
	audioPwmPrvWriteSampleNow(sample);
	audioPwmPrvArmNextPcmAlarm();
}

void audioPwmSetVolume(uint_fast8_t volume)
{
	if (volume > AUDIO_PWM_VOLUME_MAX)
		volume = AUDIO_PWM_VOLUME_MAX;
	mVolume = volume;
	if (mMode == AudioPwmModeTone)
		audioPwmPrvWriteToneDuty();
}

uint_fast8_t audioPwmGetVolume(void)
{
	return mVolume;
}

bool audioPwmStart(uint32_t sampleRate)
{
	if (!sampleRate)
		return false;

	NVIC_DisableIRQ(AUDIO_PCM_IRQ);
	AUDIO_PCM_TIMER->inte &=~ AUDIO_PCM_ALARM_BIT;
	AUDIO_PCM_TIMER->armed = AUDIO_PCM_ALARM_BIT;
	AUDIO_PCM_TIMER->intr = AUDIO_PCM_ALARM_BIT;
	audioPwmPrvDisable();
	audioPwmPrvPinToPwm();

	pwm_hw->slice[AUDIO_PWM_IDX].top = (pwm_hw->slice[AUDIO_PWM_IDX].top &~ PWM_CH0_TOP_BITS) | (AUDIO_PWM_TOP << PWM_CH0_TOP_LSB);
	pwm_hw->slice[AUDIO_PWM_IDX].ctr = 0;
	audioPwmPrvWriteDuty(AUDIO_PWM_TOP / 2);
	pwm_hw->slice[AUDIO_PWM_IDX].div = (pwm_hw->slice[AUDIO_PWM_IDX].div &~ (PWM_CH0_DIV_INT_BITS | PWM_CH0_DIV_FRAC_BITS)) | (1 << PWM_CH0_DIV_INT_LSB);
	pwm_hw->slice[AUDIO_PWM_IDX].csr = (pwm_hw->slice[AUDIO_PWM_IDX].csr &~ (PWM_CH0_CSR_PH_ADV_BITS | PWM_CH0_CSR_PH_RET_BITS |
		PWM_CH0_CSR_DIVMODE_BITS | PWM_CH0_CSR_B_INV_BITS | PWM_CH0_CSR_A_INV_BITS | PWM_CH0_CSR_PH_CORRECT_BITS)) |
		(PWM_CH0_CSR_DIVMODE_VALUE_DIV << PWM_CH0_CSR_DIVMODE_LSB) | audioPwmPrvInvertBit() | PWM_CH0_CSR_EN_BITS;

	mPcmReadIdx = 0;
	mPcmWriteIdx = 0;
	mPcmQueued = 0;
	mPcmSampleRate = sampleRate;
	mPcmAlarmPhase = 0;
	mPcmNextAlarm = AUDIO_PCM_TIMER->timerawl + 2;
	mMode = AudioPwmModePcm;
	NVIC_SetPriority(AUDIO_PCM_IRQ, 0);
	NVIC_ClearPendingIRQ(AUDIO_PCM_IRQ);
	AUDIO_PCM_TIMER->inte |= AUDIO_PCM_ALARM_BIT;
	audioPwmPrvArmNextPcmAlarm();
	NVIC_EnableIRQ(AUDIO_PCM_IRQ);
	return true;
}

void audioPwmWriteSample(int16_t sample)
{
	if (mMode != AudioPwmModePcm)
		return;

	while (mMode == AudioPwmModePcm && mPcmQueued >= AUDIO_PCM_BUF_LEN);
	if (mMode != AudioPwmModePcm)
		return;

	__disable_irq();
	if (mPcmQueued < AUDIO_PCM_BUF_LEN) {
		mPcmBuf[mPcmWriteIdx++] = sample;
		if (mPcmWriteIdx == AUDIO_PCM_BUF_LEN)
			mPcmWriteIdx = 0;
		mPcmQueued++;
	}
	__enable_irq();
}

void audioPwmWaitNext(void)
{
	while (mMode == AudioPwmModePcm && mPcmQueued >= AUDIO_PCM_BUF_LEN / 2);
}

bool audioPwmPcmDrained(void)
{
	return mMode != AudioPwmModePcm || !mPcmQueued;
}

uint32_t audioPwmPcmQueued(void)
{
	return mPcmQueued;
}

bool audioPwmTone(uint32_t freq)
{
	uint32_t duty, baseFreq = TICKS_PER_SECOND / AUDIO_TONE_CLK_DIV;

	if (!freq) {
		audioPwmStop();
		return true;
	}

	NVIC_DisableIRQ(AUDIO_PCM_IRQ);
	AUDIO_PCM_TIMER->inte &=~ AUDIO_PCM_ALARM_BIT;
	AUDIO_PCM_TIMER->armed = AUDIO_PCM_ALARM_BIT;
	AUDIO_PCM_TIMER->intr = AUDIO_PCM_ALARM_BIT;
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

	NVIC_DisableIRQ(AUDIO_PCM_IRQ);
	AUDIO_PCM_TIMER->inte &=~ AUDIO_PCM_ALARM_BIT;
	AUDIO_PCM_TIMER->armed = AUDIO_PCM_ALARM_BIT;
	AUDIO_PCM_TIMER->intr = AUDIO_PCM_ALARM_BIT;
	audioPwmPrvDisable();
	audioPwmPrvWriteDuty(0);
	audioPwmPrvPinLow();
	mPcmQueued = 0;
	mMode = AudioPwmModeStopped;
}
