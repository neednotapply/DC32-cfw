#include "bootGuard.h"
#include "2350.h"

#define BOOT_GUARD_MAGIC	0x44433247ul
#define BOOT_GUARD_SCRATCH_MAGIC	0
#define BOOT_GUARD_SCRATCH_MODE		1
#define BOOT_GUARD_SCRATCH_REASON	2
#define BOOT_GUARD_SCRATCH_CFSR		3
#define BOOT_GUARD_SCRATCH_HFSR		4
#define BOOT_GUARD_SCRATCH_PC		5
#define BOOT_GUARD_SCRATCH_SP		6
#define BOOT_GUARD_SCRATCH_BFAR		7

static enum BootGuardMode mRecoveredMode;

static enum BootGuardMode bootGuardPrvScratchMode(void)
{
	uint32_t mode = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MODE];

	if (watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MAGIC] != BOOT_GUARD_MAGIC)
		return BootGuardModeNone;
	if (mode > BootGuardModeHardFault)
		return BootGuardModeTool;
	return (enum BootGuardMode)mode;
}

void bootGuardInit(void)
{
	uint32_t reason = watchdog_hw->reason;

	mRecoveredMode = bootGuardPrvScratchMode();
	if (!reason)
		watchdog_hw->scratch[BOOT_GUARD_SCRATCH_REASON] = 0;
	else if (!mRecoveredMode && watchdog_hw->scratch[BOOT_GUARD_SCRATCH_REASON] != (BOOT_GUARD_MAGIC ^ reason)) {
		mRecoveredMode = BootGuardModeTool;
		watchdog_hw->scratch[BOOT_GUARD_SCRATCH_REASON] = BOOT_GUARD_MAGIC ^ reason;
	}
}

void bootGuardEnter(enum BootGuardMode mode)
{
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MAGIC] = BOOT_GUARD_MAGIC;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MODE] = mode;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_CFSR] = 0;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_HFSR] = 0;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_PC] = 0;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_SP] = 0;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_BFAR] = 0;
}

void bootGuardExit(enum BootGuardMode mode)
{
	if (bootGuardPrvScratchMode() == mode)
		bootGuardClear();
}

enum BootGuardMode bootGuardRecoveredMode(void)
{
	return mRecoveredMode;
}

void bootGuardRecoveredCrashInfo(struct BootGuardCrashInfo *info)
{
	info->mode = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MODE];
	info->reason = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_REASON];
	info->cfsr = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_CFSR];
	info->hfsr = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_HFSR];
	info->pc = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_PC];
	info->sp = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_SP];
	info->bfar = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_BFAR];
}

void bootGuardCaptureHardFault(uint32_t *frame, uint32_t retLr)
{
	uint32_t *sp = frame + 8;

	(void)retLr;

	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MAGIC] = BOOT_GUARD_MAGIC;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MODE] = BootGuardModeHardFault;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_REASON] = BOOT_GUARD_MAGIC ^ watchdog_hw->reason;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_CFSR] = SCB->CFSR;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_HFSR] = SCB->HFSR;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_PC] = frame[6];
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_SP] = (uint32_t)sp;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_BFAR] = SCB->BFAR;

	NVIC_SystemReset();
	while (1);
}

void bootGuardClear(void)
{
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MODE] = BootGuardModeNone;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MAGIC] = 0;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_CFSR] = 0;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_HFSR] = 0;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_PC] = 0;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_SP] = 0;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_BFAR] = 0;
	mRecoveredMode = BootGuardModeNone;
}
