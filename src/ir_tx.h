#ifndef _IR_TX_H_
#define _IR_TX_H_

#include <stdbool.h>
#include <stdint.h>

bool irTxSendRaw(const uint16_t *timings, uint32_t numTimings, uint32_t frequency, bool (*cancelF)(void *userData), void *userData);

#endif
