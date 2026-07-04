#ifndef _IR_REMOTE_H_
#define _IR_REMOTE_H_

#include <stdbool.h>
#include <stdint.h>

struct IrRemoteOpenLasirFrame {
	uint32_t raw;
	uint8_t blockId;
	uint8_t deviceId;
	uint8_t mode;
	uint8_t data;
};

enum IrRemoteRxControl {
	IrRemoteRxControlBegin,
	IrRemoteRxControlEnd,
	IrRemoteRxControlPoll,
};

void irRemoteBegin(void);
void irRemoteEnd(void);
void irRemoteMarkUsec(uint32_t carrierHz, uint32_t usec);
void irRemoteSpaceUsec(uint32_t usec);

/* Resident-firmware entry point retained by dcAppAbortActive(). */
bool irRemoteRxControl(enum IrRemoteRxControl control, uint32_t *spaceUsecP);

bool irRemoteOpenLasirBeginReceive(void);
void irRemoteOpenLasirEndReceive(void);
bool irRemoteOpenLasirPoll(struct IrRemoteOpenLasirFrame *frame);
bool irRemoteOpenLasirFeedSpaceUsec(uint32_t spaceUsec, struct IrRemoteOpenLasirFrame *frame);
void irRemoteOpenLasirSend(uint8_t blockId, uint8_t deviceId, uint8_t color);

#endif
