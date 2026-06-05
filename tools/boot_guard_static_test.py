#!/usr/bin/env python3
"""Static regression checks for boot guard reset classification."""

from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]
BOOT_GUARD_SRC = ROOT / "src" / "bootGuard.c"


def function_body(text: str, name: str) -> str:
    match = re.search(rf"\b{name}\s*\([^)]*\)\s*\{{", text)
    if not match:
        return ""

    pos = match.end()
    depth = 1
    while pos < len(text) and depth:
        if text[pos] == "{":
            depth += 1
        elif text[pos] == "}":
            depth -= 1
        pos += 1
    return text[match.end():pos - 1] if depth == 0 else ""


def expect(ok: bool, msg: str, failures: list[str]) -> None:
    if ok:
        print(f"ok - {msg}")
    else:
        print(f"not ok - {msg}")
        failures.append(msg)


def main() -> int:
    text = BOOT_GUARD_SRC.read_text(encoding="utf-8")
    main_text = (ROOT / "src" / "main_rp2350_defcon.c").read_text(encoding="utf-8")
    init = function_body(text, "bootGuardInit")
    mark = function_body(text, "bootGuardMarkImageBooted")
    forced = function_body(text, "bootGuardPrvIsForcedWatchdogReset")
    failures: list[str] = []

    expect("mBootGuardImageBootPage" in text and
           "aligned(QSPI_WRITE_GRANULARITY)" in text and
           "section(\".rodata.boot_guard_image_boot\")" in text,
           "image-boot marker is emitted into the UF2 image on a writable flash page", failures)
    expect("bootGuardPrvIsFreshImageBoot()" in init and
           "mRecoveredMode != BootGuardModeHardFault" in init and
           "bootGuardClear();" in init,
           "first boot of a freshly flashed image clears stale non-hardfault recovery state", failures)
    expect("page[0] = BOOT_GUARD_IMAGE_BOOTED_MAGIC;" in mark and
           "flashWrite((uint32_t)(uintptr_t)mBootGuardImageBootPage, 0, page, sizeof(page))" in mark,
           "first image boot is marked by programming the marker page without erasing app flash", failures)
    expect("flashBootInit();" in main_text and
           re.search(r"flashBootInit\(\);\s*bootGuardMarkImageBooted\(\);", main_text) is not None,
           "image boot marker is written after flash direct-mode setup", failures)
    expect("mRecoveredMode != BootGuardModeHardFault" in init and
           "bootGuardPrvIsForcedWatchdogReset(reason)" in init and
           "bootGuardClear();" in init,
           "forced watchdog handoff clears stale non-hardfault guard markers", failures)
    expect("if (mRecoveredMode)" in init and "return;" in init,
           "guarded resets keep their stamped recovery mode otherwise", failures)
    expect("bootGuardPrvIsForcedWatchdogReset(reason)" in init and
           "watchdog_hw->scratch[BOOT_GUARD_SCRATCH_REASON_OR_LR] = 0;" in init,
           "unreportable unguarded resets are cleared without surfacing an alert", failures)
    expect("WATCHDOG_REASON_FORCE_BITS" in forced,
           "forced watchdog handoffs are recognized", failures)
    expect("WATCHDOG_REASON_TIMER_BITS" in forced and "== 0" in forced,
           "timer watchdog resets remain reportable", failures)

    if failures:
        print("\nfailed checks:")
        for failure in failures:
            print(f" - {failure}")
        return 1
    print("\nboot guard static checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
