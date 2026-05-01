#include "pinoutRp2350defcon.h"
#include "timebase.h"
#include "2350.h"
#include "hardware/regs/dreq.h"
#include "audioPwm.h"

#define AUDIO_PWM_IDX			((PIN_SPQR >> 1) & 7)
#define AUDIO_PWM_IS_B			(PIN_SPQR & 1)
#define AUDIO_PWM_TOP			255
#define AUDIO_DMA_CH			7
#define AUDIO_DMA_CH_BIT		(1u << AUDIO_DMA_CH)
#define AUDIO_DMA_CLK_PWM_IDX	7
#define AUDIO_DMA_MAX_COUNT		0x0fffffff
#define AUDIO_TONE_CLK_DIV		32
#define AUDIO_PCM_TIMER			timer0_hw
#define AUDIO_PCM_ALARM			3
#define AUDIO_PCM_ALARM_BIT		(1u << AUDIO_PCM_ALARM)
#define AUDIO_PCM_IRQ			TIMER0_IRQ_3_IRQn
#define AUDIO_PCM_BUF_LEN		1024
#define AUDIO_PCM_FULL_TIMEOUT	(TICKS_PER_SECOND / 10)

enum AudioPwmMode {
	AudioPwmModeStopped,
	AudioPwmModePcm,
	AudioPwmModeDma,
	AudioPwmModeTone,
};

static volatile uint16_t mPcmReadIdx;
static volatile uint16_t mPcmWriteIdx;
static volatile uint16_t mPcmQueued;
static int16_t mPcmBuf[AUDIO_PCM_BUF_LEN];
static uint32_t mPcmSampleRate;
static uint32_t mPcmAlarmPhase;
static uint32_t mPcmNextAlarm;
static uintptr_t mDmaBufAddr;
static uint32_t mDmaBufSamples;
static uint32_t mDmaBufMask;
static uint32_t mToneDuty;
static uint8_t mVolume = AUDIO_PWM_VOLUME_MAX;
static bool mPcmDutySamples;
static bool mPcmTimerInited;
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

static volatile uint16_t *audioPwmPrvDutyCcHalfword(void)
{
	return ((volatile uint16_t*)&pwm_hw->slice[AUDIO_PWM_IDX].cc) + (AUDIO_PWM_IS_B ? 1 : 0);
}

static void audioPwmPrvStartDutyCarrier(void)
{
	audioPwmPrvDisable();
	audioPwmPrvPinToPwm();

	pwm_hw->slice[AUDIO_PWM_IDX].top = (pwm_hw->slice[AUDIO_PWM_IDX].top &~ PWM_CH0_TOP_BITS) | (AUDIO_PWM_TOP << PWM_CH0_TOP_LSB);
	pwm_hw->slice[AUDIO_PWM_IDX].ctr = 0;
	audioPwmPrvWriteDuty(AUDIO_PWM_TOP / 2);
	pwm_hw->slice[AUDIO_PWM_IDX].div = (pwm_hw->slice[AUDIO_PWM_IDX].div &~ (PWM_CH0_DIV_INT_BITS | PWM_CH0_DIV_FRAC_BITS)) | (1 << PWM_CH0_DIV_INT_LSB);
	pwm_hw->slice[AUDIO_PWM_IDX].csr = (pwm_hw->slice[AUDIO_PWM_IDX].csr &~ (PWM_CH0_CSR_PH_ADV_BITS | PWM_CH0_CSR_PH_RET_BITS |
		PWM_CH0_CSR_DIVMODE_BITS | PWM_CH0_CSR_B_INV_BITS | PWM_CH0_CSR_A_INV_BITS | PWM_CH0_CSR_PH_CORRECT_BITS)) |
		(PWM_CH0_CSR_DIVMODE_VALUE_DIV << PWM_CH0_CSR_DIVMODE_LSB) | audioPwmPrvInvertBit() | PWM_CH0_CSR_EN_BITS;
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

	mPcmAlarmPhase += TICKS_PER_SECOND;
	delta = mPcmAlarmPhase / mPcmSampleRate;
	mPcmAlarmPhase -= delta * mPcmSampleRate;
	if (!delta)
		delta = 1;
	mPcmNextAlarm += delta;
	while ((int32_t)(mPcmNextAlarm - AUDIO_PCM_TIMER->timerawl) <= 0)
		mPcmNextAlarm += delta;
	AUDIO_PCM_TIMER->alarm[AUDIO_PCM_ALARM] = mPcmNextAlarm;
}

static void audioPwmPrvPcmTimerInit(void)
{
	if (mPcmTimerInited)
		return;

	resets_hw->reset &=~ RESETS_RESET_TIMER0_BITS;
	while ((resets_hw->reset_done & RESETS_RESET_DONE_TIMER0_BITS) != RESETS_RESET_DONE_TIMER0_BITS);
	AUDIO_PCM_TIMER->source = TIMER_SOURCE_CLK_SYS_VALUE_CLK_SYS << TIMER_SOURCE_CLK_SYS_LSB;
	mPcmTimerInited = true;
}

static void audioPwmPrvPcmTimerStop(void)
{
	if (!mPcmTimerInited)
		return;

	NVIC_DisableIRQ(AUDIO_PCM_IRQ);
	AUDIO_PCM_TIMER->inte &=~ AUDIO_PCM_ALARM_BIT;
	AUDIO_PCM_TIMER->armed = AUDIO_PCM_ALARM_BIT;
	AUDIO_PCM_TIMER->intr = AUDIO_PCM_ALARM_BIT;
}

static void audioPwmPrvDmaStop(void)
{
	dma_hw->ch[AUDIO_DMA_CH].al1_ctrl &=~ DMA_CH0_CTRL_TRIG_EN_BITS;
	if (dma_hw->ch[AUDIO_DMA_CH].al1_ctrl & DMA_CH0_CTRL_TRIG_BUSY_BITS) {
		dma_hw->abort = AUDIO_DMA_CH_BIT;
		while (dma_hw->abort & AUDIO_DMA_CH_BIT);
	}
	dma_hw->ch[AUDIO_DMA_CH].al1_ctrl = 0;
	dma_hw->inte0 &=~ AUDIO_DMA_CH_BIT;
	dma_hw->inte1 &=~ AUDIO_DMA_CH_BIT;
	dma_hw->inte2 &=~ AUDIO_DMA_CH_BIT;
	dma_hw->inte3 &=~ AUDIO_DMA_CH_BIT;
	dma_hw->ints0 = AUDIO_DMA_CH_BIT;
	dma_hw->ints1 = AUDIO_DMA_CH_BIT;
	dma_hw->ints2 = AUDIO_DMA_CH_BIT;
	dma_hw->ints3 = AUDIO_DMA_CH_BIT;
	pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].csr &=~ PWM_CH0_CSR_EN_BITS;
	pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].ctr = 0;
	mDmaBufAddr = 0;
	mDmaBufSamples = 0;
	mDmaBufMask = 0;
	if (mMode == AudioPwmModeDma)
		mMode = AudioPwmModeStopped;
}

static bool audioPwmPrvSetDmaSampleClock(uint32_t sampleRate)
{
	uint32_t div, top;
	uint64_t denom;

	if (!sampleRate)
		return false;

	denom = (uint64_t)sampleRate * 65536;
	div = (uint32_t)((TICKS_PER_SECOND + denom - 1) / denom);
	if (!div)
		div = 1;
	if (div > 255)
		return false;

	top = (uint32_t)(((uint64_t)TICKS_PER_SECOND + ((uint64_t)sampleRate * div) / 2) / ((uint64_t)sampleRate * div));
	if (!top)
		top = 1;
	if (top > 65536)
		top = 65536;
	top--;

	pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].csr &=~ PWM_CH0_CSR_EN_BITS;
	while (pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].csr & PWM_CH0_CSR_EN_BITS);
	pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].top = (pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].top &~ PWM_CH0_TOP_BITS) | (top << PWM_CH0_TOP_LSB);
	pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].ctr = 0;
	pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].cc = 0;
	pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].div = (pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].div &~ (PWM_CH0_DIV_INT_BITS | PWM_CH0_DIV_FRAC_BITS)) |
		(div << PWM_CH0_DIV_INT_LSB);
	pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].csr = (pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].csr &~ (PWM_CH0_CSR_PH_ADV_BITS |
		PWM_CH0_CSR_PH_RET_BITS | PWM_CH0_CSR_DIVMODE_BITS | PWM_CH0_CSR_B_INV_BITS | PWM_CH0_CSR_A_INV_BITS |
		PWM_CH0_CSR_PH_CORRECT_BITS)) | (PWM_CH0_CSR_DIVMODE_VALUE_DIV << PWM_CH0_CSR_DIVMODE_LSB);
	return true;
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
	if (mPcmDutySamples)
		audioPwmPrvWriteDuty((uint8_t)sample);
	else
		audioPwmPrvWriteSampleNow(sample);
	audioPwmPrvArmNextPcmAlarm();
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
	else if (mMode == AudioPwmModePcm && mPcmDutySamples) {
		if (mVolume) {
			audioPwmPrvPinToPwm();
			pwm_hw->slice[AUDIO_PWM_IDX].csr |= PWM_CH0_CSR_EN_BITS;
		}
		else {
			audioPwmPrvDisable();
			audioPwmPrvWriteDuty(0);
			audioPwmPrvPinLow();
		}
	}
}

uint_fast8_t audioPwmGetVolume(void)
{
	return mVolume;
}

static bool audioPwmPrvStartPcm(uint32_t sampleRate, bool dutySamples)
{
	if (!sampleRate)
		return false;

	audioPwmPrvDmaStop();
	audioPwmPrvPcmTimerInit();
	audioPwmPrvPcmTimerStop();
	audioPwmPrvStartDutyCarrier();
	if (dutySamples && !mVolume) {
		audioPwmPrvDisable();
		audioPwmPrvWriteDuty(0);
		audioPwmPrvPinLow();
	}

	mPcmReadIdx = 0;
	mPcmWriteIdx = 0;
	mPcmQueued = 0;
	mPcmSampleRate = sampleRate;
	mPcmAlarmPhase = 0;
	mPcmNextAlarm = AUDIO_PCM_TIMER->timerawl + 2;
	mPcmDutySamples = dutySamples;
	mMode = AudioPwmModePcm;
	NVIC_SetPriority(AUDIO_PCM_IRQ, 0);
	NVIC_ClearPendingIRQ(AUDIO_PCM_IRQ);
	AUDIO_PCM_TIMER->inte |= AUDIO_PCM_ALARM_BIT;
	audioPwmPrvArmNextPcmAlarm();
	NVIC_EnableIRQ(AUDIO_PCM_IRQ);
	return true;
}

bool audioPwmStart(uint32_t sampleRate)
{
	return audioPwmPrvStartPcm(sampleRate, false);
}

bool audioPwmStartDuty(uint32_t sampleRate)
{
	return audioPwmPrvStartPcm(sampleRate, true);
}

bool audioPwmStartDutyDma(uint32_t sampleRate, const uint16_t *samples, uint32_t numSamples, uint_fast8_t ringBits)
{
	uintptr_t sampleAddr = (uintptr_t)samples;
	uint32_t ringBytes;

	if (!sampleRate || !samples || !numSamples || numSamples > AUDIO_DMA_MAX_COUNT || ringBits < 1 || ringBits > 15)
		return false;
	ringBytes = 1u << ringBits;
	if (sampleAddr & (ringBytes - 1))
		return false;

	audioPwmPrvPcmTimerStop();
	audioPwmPrvDmaStop();
	mMode = AudioPwmModeStopped;
	if (!audioPwmPrvSetDmaSampleClock(sampleRate))
		return false;
	audioPwmPrvStartDutyCarrier();

	dma_hw->ch[AUDIO_DMA_CH].read_addr = sampleAddr;
	dma_hw->ch[AUDIO_DMA_CH].write_addr = (uintptr_t)audioPwmPrvDutyCcHalfword();
	dma_hw->ch[AUDIO_DMA_CH].transfer_count = numSamples;
	dma_hw->ch[AUDIO_DMA_CH].al1_ctrl =
		(DREQ_PWM_WRAP7 << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB) |
		(AUDIO_DMA_CH << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) |
		(ringBits << DMA_CH0_CTRL_TRIG_RING_SIZE_LSB) |
		DMA_CH0_CTRL_TRIG_INCR_READ_BITS |
		DMA_CH0_CTRL_TRIG_HIGH_PRIORITY_BITS |
		(DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_HALFWORD << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) |
		DMA_CH0_CTRL_TRIG_EN_BITS;

	mDmaBufAddr = sampleAddr;
	mDmaBufSamples = ringBytes / sizeof(uint16_t);
	mDmaBufMask = mDmaBufSamples - 1;
	mMode = AudioPwmModeDma;
	pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].csr |= PWM_CH0_CSR_EN_BITS;
	return true;
}

void audioPwmDmaPause(bool paused)
{
	if (mMode != AudioPwmModeDma)
		return;

	if (paused) {
		pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].csr &=~ PWM_CH0_CSR_EN_BITS;
		dma_hw->ch[AUDIO_DMA_CH].al1_ctrl &=~ DMA_CH0_CTRL_TRIG_EN_BITS;
		while (dma_hw->ch[AUDIO_DMA_CH].al1_ctrl & DMA_CH0_CTRL_TRIG_BUSY_BITS);
		audioPwmPrvWriteDuty(AUDIO_PWM_TOP / 2);
	}
	else if (dma_hw->ch[AUDIO_DMA_CH].transfer_count) {
		dma_hw->ch[AUDIO_DMA_CH].al1_ctrl |= DMA_CH0_CTRL_TRIG_EN_BITS;
		pwm_hw->slice[AUDIO_DMA_CLK_PWM_IDX].csr |= PWM_CH0_CSR_EN_BITS;
	}
}

uint32_t audioPwmDmaReadIndex(void)
{
	uintptr_t readAddr;

	if (mMode != AudioPwmModeDma || !mDmaBufSamples)
		return 0;
	readAddr = dma_hw->ch[AUDIO_DMA_CH].read_addr;
	return ((readAddr - mDmaBufAddr) / sizeof(uint16_t)) & mDmaBufMask;
}

uint32_t audioPwmDmaSamplesRemaining(void)
{
	if (mMode != AudioPwmModeDma)
		return 0;
	return dma_hw->ch[AUDIO_DMA_CH].transfer_count & AUDIO_DMA_MAX_COUNT;
}

bool audioPwmDmaDone(void)
{
	return mMode != AudioPwmModeDma || !audioPwmDmaSamplesRemaining();
}

static void audioPwmPrvWritePcmValue(int16_t sample)
{
	uint64_t timeout;

	if (mMode != AudioPwmModePcm)
		return;

	timeout = getTime() + AUDIO_PCM_FULL_TIMEOUT;
	while (mMode == AudioPwmModePcm && mPcmQueued >= AUDIO_PCM_BUF_LEN) {
		if (getTime() > timeout) {
			__disable_irq();
			if (mPcmQueued)
				mPcmQueued--;
			__enable_irq();
			break;
		}
	}
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

void audioPwmWriteSample(int16_t sample)
{
	audioPwmPrvWritePcmValue(sample);
}

void audioPwmWriteDutySample(uint8_t duty)
{
	audioPwmPrvWritePcmValue(duty);
}

void audioPwmWaitNext(void)
{
	uint64_t timeout = getTime() + AUDIO_PCM_FULL_TIMEOUT;

	while (mMode == AudioPwmModePcm && mPcmQueued >= AUDIO_PCM_BUF_LEN / 2) {
		if (getTime() > timeout)
			break;
	}
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
	if (!mVolume) {
		audioPwmStop();
		return true;
	}

	audioPwmPrvPcmTimerStop();
	audioPwmPrvDmaStop();
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

	audioPwmPrvPcmTimerStop();
	audioPwmPrvDmaStop();
	audioPwmPrvDisable();
	audioPwmPrvWriteDuty(0);
	audioPwmPrvPinLow();
	mPcmQueued = 0;
	mPcmDutySamples = false;
	mMode = AudioPwmModeStopped;
}
