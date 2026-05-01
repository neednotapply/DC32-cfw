#ifndef _AUDIO_PWM_H_
#define _AUDIO_PWM_H_

#include <stdbool.h>
#include <stdint.h>

#define AUDIO_PWM_VOLUME_MIN	0
#define AUDIO_PWM_VOLUME_MAX	15

void audioPwmSetVolume(uint_fast8_t volume);
uint_fast8_t audioPwmGetVolume(void);
bool audioPwmStart(uint32_t sampleRate);
bool audioPwmStartDuty(uint32_t sampleRate);
bool audioPwmStartDutyDma(uint32_t sampleRate, const uint16_t *samples, uint32_t numSamples, uint_fast8_t ringBits);
void audioPwmDmaPause(bool paused);
uint32_t audioPwmDmaReadIndex(void);
uint32_t audioPwmDmaSamplesRemaining(void);
bool audioPwmDmaDone(void);
void audioPwmWriteSample(int16_t sample);
void audioPwmWriteDutySample(uint8_t duty);
void audioPwmWaitNext(void);
bool audioPwmPcmDrained(void);
uint32_t audioPwmPcmQueued(void);
bool audioPwmTone(uint32_t freq);
void audioPwmStop(void);

#endif
