#ifndef _BOOT_GUARD_H_
#define _BOOT_GUARD_H_

#include <stdint.h>

struct BootGuardCrashInfo {
	uint32_t mode;
	uint32_t originalMode;
	uint32_t reason;
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t cfsr;
	uint32_t hfsr;
	uint32_t pc;
	uint32_t lr;
	uint32_t sp;
	uint32_t bfar;
	uint32_t xpsr;
	uint32_t excReturn;
};

enum BootGuardMode {
	BootGuardModeNone = 0,
	BootGuardModeGame,
	BootGuardModeIr,
	BootGuardModeBadUsb,
	BootGuardModeMusic,
	BootGuardModeTool,
	BootGuardModeHardFault,
};

void bootGuardInit(void);
void bootGuardEnter(enum BootGuardMode mode);
void bootGuardExit(enum BootGuardMode mode);
enum BootGuardMode bootGuardRecoveredMode(void);
void bootGuardRecoveredCrashInfo(struct BootGuardCrashInfo *info);
void bootGuardCaptureHardFault(uint32_t *frame, uint32_t retLr);
void bootGuardClear(void);

#endif
