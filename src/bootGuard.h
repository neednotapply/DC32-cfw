#ifndef _BOOT_GUARD_H_
#define _BOOT_GUARD_H_

#include <stdint.h>

struct BootGuardCrashInfo {
	uint32_t mode;
	uint32_t reason;
	uint32_t cfsr;
	uint32_t hfsr;
	uint32_t pc;
	uint32_t sp;
	uint32_t bfar;
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
