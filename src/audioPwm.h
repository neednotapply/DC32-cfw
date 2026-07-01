#ifndef _AUDIO_PWM_H_
#define _AUDIO_PWM_H_

#include <stdbool.h>
#include <stdint.h>

#define AUDIO_PWM_VOLUME_MIN	0
#define AUDIO_PWM_VOLUME_MAX	15

void audioPwmSetVolume(uint_fast8_t volume);
uint_fast8_t audioPwmGetVolume(void);
void audioPwmSetMasterVolume(uint_fast8_t volume);
uint_fast8_t audioPwmGetMasterVolume(void);
bool audioPwmTone(uint32_t freq);
bool audioPwmPcmStart(uint32_t sampleRate);
bool audioPwmPcmCanWrite(void);
bool audioPwmPcmWriteU8(uint8_t sample);
void audioPwmPcmDrain(void);
void audioPwmPcmStop(void);
void audioPwmStop(void);

#endif
