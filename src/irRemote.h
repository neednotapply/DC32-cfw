#ifndef _IR_REMOTE_H_
#define _IR_REMOTE_H_

#include <stdint.h>

void irRemoteBegin(void);
void irRemoteEnd(void);
void irRemoteMarkUsec(uint32_t carrierHz, uint32_t usec);
void irRemoteSpaceUsec(uint32_t usec);

#endif
