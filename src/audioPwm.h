#ifndef _AUDIO_PWM_H_
#define _AUDIO_PWM_H_

#include <stdbool.h>
#include <stdint.h>

#define AUDIO_PWM_VOLUME_MIN	0
#define AUDIO_PWM_VOLUME_MAX	10

void audioPwmSetVolume(uint_fast8_t volume);
uint_fast8_t audioPwmGetVolume(void);
bool audioPwmStart(uint32_t sampleRate);
void audioPwmWriteSample(int16_t sample);
void audioPwmWaitNext(void);
bool audioPwmTone(uint32_t freq);
void audioPwmStop(void);

#endif
