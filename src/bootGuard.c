#include "bootGuard.h"
#include "2350.h"

#define BOOT_GUARD_MAGIC	0x44433247ul
#define BOOT_GUARD_SCRATCH_MAGIC	0
#define BOOT_GUARD_SCRATCH_MODE		1
#define BOOT_GUARD_SCRATCH_REASON_OR_LR	2
#define BOOT_GUARD_SCRATCH_CFSR		3
#define BOOT_GUARD_SCRATCH_HFSR		4
#define BOOT_GUARD_SCRATCH_PC		5
#define BOOT_GUARD_SCRATCH_SP		6
#define BOOT_GUARD_SCRATCH_BFAR		7

#define BOOT_GUARD_POWER_SCRATCH_ORIGINAL_MODE	0
#define BOOT_GUARD_POWER_SCRATCH_R0		1
#define BOOT_GUARD_POWER_SCRATCH_R1		2
#define BOOT_GUARD_POWER_SCRATCH_R2		3
#define BOOT_GUARD_POWER_SCRATCH_R3		4
#define BOOT_GUARD_POWER_SCRATCH_R12		5
#define BOOT_GUARD_POWER_SCRATCH_XPSR		6
#define BOOT_GUARD_POWER_SCRATCH_EXC_RETURN	7

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
		watchdog_hw->scratch[BOOT_GUARD_SCRATCH_REASON_OR_LR] = 0;
	else if (!mRecoveredMode && watchdog_hw->scratch[BOOT_GUARD_SCRATCH_REASON_OR_LR] != (BOOT_GUARD_MAGIC ^ reason)) {
		mRecoveredMode = BootGuardModeTool;
		watchdog_hw->scratch[BOOT_GUARD_SCRATCH_REASON_OR_LR] = BOOT_GUARD_MAGIC ^ reason;
	}
}

void bootGuardEnter(enum BootGuardMode mode)
{
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MAGIC] = BOOT_GUARD_MAGIC;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MODE] = mode;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_ORIGINAL_MODE] = mode;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_CFSR] = 0;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_HFSR] = 0;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_PC] = 0;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_SP] = 0;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_BFAR] = 0;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R0] = 0;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R1] = 0;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R2] = 0;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R3] = 0;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R12] = 0;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_XPSR] = 0;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_EXC_RETURN] = 0;
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
	info->originalMode = powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_ORIGINAL_MODE];
	if (info->mode == BootGuardModeHardFault) {
		info->reason = 0;
		info->lr = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_REASON_OR_LR];
	}
	else {
		info->reason = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_REASON_OR_LR];
		info->lr = 0;
	}
	info->r0 = powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R0];
	info->r1 = powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R1];
	info->r2 = powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R2];
	info->r3 = powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R3];
	info->r12 = powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R12];
	info->cfsr = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_CFSR];
	info->hfsr = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_HFSR];
	info->pc = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_PC];
	info->sp = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_SP];
	info->bfar = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_BFAR];
	info->xpsr = powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_XPSR];
	info->excReturn = powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_EXC_RETURN];
}

void bootGuardCaptureHardFault(uint32_t *frame, uint32_t retLr)
{
	uint32_t *sp = frame + 8;
	uint32_t originalMode = watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MODE];

	if (watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MAGIC] != BOOT_GUARD_MAGIC)
		originalMode = BootGuardModeNone;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MAGIC] = BOOT_GUARD_MAGIC;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MODE] = BootGuardModeHardFault;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_REASON_OR_LR] = frame[5];
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_CFSR] = SCB->CFSR;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_HFSR] = SCB->HFSR;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_PC] = frame[6];
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_SP] = (uint32_t)sp;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_BFAR] = SCB->BFAR;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_ORIGINAL_MODE] = originalMode;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R0] = frame[0];
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R1] = frame[1];
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R2] = frame[2];
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R3] = frame[3];
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R12] = frame[4];
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_XPSR] = frame[7];
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_EXC_RETURN] = retLr;

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
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_ORIGINAL_MODE] = BootGuardModeNone;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R0] = 0;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R1] = 0;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R2] = 0;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R3] = 0;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_R12] = 0;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_XPSR] = 0;
	powman_hw->scratch[BOOT_GUARD_POWER_SCRATCH_EXC_RETURN] = 0;
	mRecoveredMode = BootGuardModeNone;
}
