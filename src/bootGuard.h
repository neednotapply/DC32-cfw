#ifndef _BOOT_GUARD_H_
#define _BOOT_GUARD_H_

#include <stdint.h>

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
void bootGuardClear(void);

#endif
