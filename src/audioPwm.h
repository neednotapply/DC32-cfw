#ifndef _AUDIO_PWM_H_
#define _AUDIO_PWM_H_

#include <stdbool.h>
#include <stdint.h>

bool audioPwmStart(uint32_t sampleRate);
void audioPwmWriteSample(int16_t sample);
void audioPwmWaitNext(void);
bool audioPwmTone(uint32_t freq);
void audioPwmStop(void);

#endif
