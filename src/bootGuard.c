#include "bootGuard.h"
#include "2350.h"

#define BOOT_GUARD_MAGIC	0x44433247ul
#define BOOT_GUARD_SCRATCH_MAGIC	0
#define BOOT_GUARD_SCRATCH_MODE		1
#define BOOT_GUARD_SCRATCH_REASON	2

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

void bootGuardClear(void)
{
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MODE] = BootGuardModeNone;
	watchdog_hw->scratch[BOOT_GUARD_SCRATCH_MAGIC] = 0;
	mRecoveredMode = BootGuardModeNone;
}
