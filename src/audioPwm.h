#ifndef _AUDIO_PWM_H_
#define _AUDIO_PWM_H_

#include <stdbool.h>
#include <stdint.h>

#define AUDIO_PWM_VOLUME_MIN	0
#define AUDIO_PWM_VOLUME_MAX	15

void audioPwmSetVolume(uint_fast8_t volume);
uint_fast8_t audioPwmGetVolume(void);
bool audioPwmTone(uint32_t freq);
void audioPwmStop(void);

#endif
