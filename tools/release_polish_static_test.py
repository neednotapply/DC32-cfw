#!/usr/bin/env python3
"""Static checks for release-polish fixes across file launchers and tools."""

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def expect(name: str, condition: bool) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def function_body(text: str, name: str) -> str:
    match = re.search(rf"\b{name}\s*\([^)]*\)\s*\{{", text)
    expect(f"{name} exists", match is not None)
    pos = match.end()
    depth = 1
    while pos < len(text) and depth:
        if text[pos] == "{":
            depth += 1
        elif text[pos] == "}":
            depth -= 1
        pos += 1
    expect(f"{name} has balanced body", depth == 0)
    return text[match.end() : pos - 1]


def main() -> int:
    ui = read("src/ui.c")
    gb = read("src/gb.c")
    gbc = read("src/gbC.c")
    gb_h = read("src/gb.h")
    gb_app = read("src/apps/gb_app.c")
    video_c = read("src/videoC.c")
    dcapp_h = read("src/dcApp.h")
    dcapp_c = read("src/dcApp.c")
    dcapp_draw = read("src/dcAppDraw.c")
    usb_msc = read("src/usbMsc.c")
    usb_device = read("src/usbDevice.c")
    timebase_c = read("src/timebase.c")
    settings_h = read("src/settings.h")
    settings_c = read("src/settings.c")
    audio_h = read("src/audioPwm.h")
    audio_c = read("src/audioPwm.c")
    badge_power_h = read("src/badgePower.h")
    badge_power_c = read("src/badgePower.c")
    badge_leds_h = read("src/badgeLeds.h")
    badge_leds_c = read("src/badgeLeds.c")
    main_c = read("src/main_rp2350_defcon.c")
    display_c = read("src/dispDefcon.c")
    display_h = read("src/dispDefcon.h")
    make_dcapp = read("tools/make_dcapp.py")
    build_workflow = read(".github/workflows/build.yml")
    nightly_workflow = read(".github/workflows/nightly.yml")

    open_with = function_body(ui, "uiPrvBrowserOpenWith")
    launch_file = function_body(ui, "uiPrvLaunchBrowserFile")
    ir_button = function_body(ui, "uiPrvIrSendButtonSpamFile")
    ir_read_line = function_body(ui, "uiPrvReadLine")
    ir_read_line_stat = function_body(ui, "uiPrvIrReadLineStat")
    keyboard_tool = function_body(ui, "uiPrvUsbKeyboardTool")
    autoclicker_draw = function_body(ui, "uiPrvAutoclickerDraw")
    autoclicker_tool = function_body(ui, "uiPrvAutoclickerTool")
    badusb_status = function_body(ui, "uiPrvBadUsbStatus")
    badusb_wait = function_body(ui, "uiPrvBadUsbWaitReady")
    keyboard_wait = function_body(ui, "uiPrvUsbKeyboardWaitReady")
    hid_wait = function_body(ui, "uiPrvUsbHidWaitReady")
    gamepad_wait = function_body(ui, "uiPrvGamepadWaitReady")
    gamepad_tool = function_body(ui, "uiPrvGamepadTool")
    usb_storage_tool = function_body(ui, "uiPrvUsbStorageTool")
    usb_storage_update = function_body(ui, "uiPrvUsbStorageUpdate")
    music_guard = function_body(ui, "uiPrvMusicBatteryOkToLaunch")
    music_app = function_body(ui, "uiDcAppRunMusic")
    category_runner = function_body(ui, "uiPrvRunCategoryToolEntry")
    game_menu = function_body(ui, "uiGameMenu")
    port_menu = function_body(ui, "uiPortMenu")
    run_loaded_game = function_body(ui, "uiPrvRunLoadedGame")
    console_name = function_body(ui, "uiPrvCurrentGameConsole")
    save_export_success = function_body(ui, "uiPrvSaveExportShowSuccess")
    save_export_options = function_body(ui, "uiPrvExportCurrentSavestateWithOptions")
    save_export_prepare = function_body(ui, "uiPrvPrepareSaveExportResult")
    save_open = function_body(ui, "uiPrvOpenSaveFile")
    save_subdir = function_body(ui, "uiPrvSaveSubdirForConsole")
    save_name = function_body(ui, "uiPrvSaveFileName")
    clean_save_name = function_body(ui, "uiPrvCleanSaveFileName")
    game_settings = function_body(ui, "uiPrvGameSettings")
    audio_settings = function_body(ui, "uiPrvAudioSettings")
    screen_settings = function_body(ui, "uiPrvScreenSettings")
    draw_header = function_body(ui, "uiPrvDrawHeader")
    dmg_palette = function_body(gb, "gbPrvRecalcDmgPal")
    set_dmg_palette = function_body(gb, "gbSetDmgPalette")
    gb_run = function_body(gbc, "gbRun")
    usb_power_down = function_body(usb_device, "usbDevicePrvPowerDown")
    usb_begin = function_body(usb_device, "usbDeviceBegin")
    boot_main = function_body(main_c, "micromain")
    idle_wait = function_body(timebase_c, "timebaseIdleWaitMsec")
    dim_observe = function_body(main_c, "uiPrvDimObserveDebounced")

    expect(".ir Open With only offers Universal IR", '"Universal IR"' in open_with and '"Power"' not in open_with and '"Mute"' not in open_with)
    expect(".DC32 is registered in File Browser", 'uiPrvStrEndsWithNoCase(ref->name, ".DC32")' in open_with and "UiBrowserOpenDcApp" in launch_file)
    expect(".DC32 launch is catalog-only", "uiPrvBrowserFindDcApp" in ui and "That .DC32 app is not registered." in launch_file)
    expect("runtime DCAPP engines guide users to Emulators", "uiPrvBrowserDcAppIsRuntimeEngine" in ui and "Open Emulators from Games to run ROMs." in launch_file)

    expect("IR selected button becomes progress title", 'buttonName && *buttonName ? buttonName : "IR Button"' in ir_button)
    expect("IR line reader can cancel pathological reads", "uiPrvIrCancelRequested()" in ir_read_line and "cancelledP" in ir_read_line)
    expect("IR line reader propagates cancel to stats", "stats->cancelled = true" in ir_read_line_stat)

    expect("USB storage fast speed is 12.5 MHz", "#define USB_MSC_FAST_SD_SPEED\t12500000" in usb_msc)
    expect("USB storage safe fallback remains 500 kHz", "#define USB_MSC_NORMAL_SD_SPEED\t500000" in usb_msc and "usbMscPrvFallbackSpeed" in usb_msc)
    expect("USB PLL is lazy and powers down after USB tools", "usbDevicePrepare()" in usb_begin and "CLOCKS_CLK_USB_CTRL_ENABLE_BITS" in usb_power_down and "PLL_PWR_PD_BITS" in usb_power_down and usb_device.count("usbDevicePrvPowerDown();") >= 5 and "usbHidPrepare()" not in boot_main)
    expect("unused UART1 and PIO2 remain in reset", "RESETS_RESET_UART1_BITS" not in boot_main and "RESETS_RESET_PIO2_BITS" not in boot_main and "RESETS_RESET_PIO0_BITS" in boot_main and "RESETS_RESET_PIO1_BITS" in boot_main and "RESETS_RESET_SPI1_BITS" in boot_main)
    expect("menu debounce idles on bounded TIMER0 alarm 1 waits", "TIMER0_IRQ_1_IRQHandler" in timebase_c and "timer0_hw->alarm[IDLE_TIMER_ALARM]" in idle_wait and "__WFI()" in idle_wait and "getTime() < deadline" in idle_wait and main_c.count("timebaseIdleWaitMsec(1)") == 2 and "timer1_hw" not in timebase_c)
    expect("bounded idle waits gate unused RP2350 clock destinations and restore the masks", "IDLE_SLEEP_GATE0" in timebase_c and "IDLE_SLEEP_GATE1" in timebase_c and "savedSleepEn0 = clocks_hw->sleep_en0" in idle_wait and "savedSleepEn1 = clocks_hw->sleep_en1" in idle_wait and "clocks_hw->sleep_en0 = savedSleepEn0 &~ IDLE_SLEEP_GATE0" in idle_wait and "clocks_hw->sleep_en1 = savedSleepEn1 &~ IDLE_SLEEP_GATE1" in idle_wait and "clocks_hw->sleep_en0 = savedSleepEn0;" in idle_wait and "clocks_hw->sleep_en1 = savedSleepEn1;" in idle_wait and "CLOCKS_SLEEP_EN1_CLK_SYS_TIMER0_BITS" not in timebase_c and "CLOCKS_SLEEP_EN1_CLK_REF_TICKS_BITS" not in timebase_c and "CLOCKS_SLEEP_EN0_CLK_SYS_DMA_BITS" not in timebase_c and "CLOCKS_SLEEP_EN0_CLK_SYS_PIO0_BITS" not in timebase_c and "CLOCKS_SLEEP_EN1_CLK_USB_BITS" not in timebase_c and "CLOCKS_SLEEP_EN1_CLK_SYS_SPI1_BITS" not in timebase_c)
    expect("menu inactivity dims to a fixed near-minimum", "UI_IDLE_DIM_TICKS" in main_c and "#define UI_IDLE_DIM_BRIGHTNESS\t1u" in main_c and "mUiActiveBrightness < UI_IDLE_DIM_BRIGHTNESS" in dim_observe and "dispSetBrightness(dimBrightness)" in dim_observe and "uiPrvDimRestore" in dim_observe and "dispOff()" not in dim_observe and "dispOn()" not in dim_observe)
    expect("dimmed static menus pause scanout and resume it before restoring brightness", "dispPauseScanout();" in dim_observe and "dispResumeScanout();" in function_body(main_c, "uiPrvDimRestore") and "bool dispPauseScanout(void);" in display_h and "bool dispResumeScanout(void);" in display_h and "prevent it from chaining another frame" in display_c and "DMA_CH0_CTRL_TRIG_BUSY_BITS" in display_c and "while (!(MY_PIO->fdebug & txStallMask));" in display_c and "dispPrvStartScanout(true);" in display_c and "lcdSetRegion(0, 0, DISP_WIDTH, DISP_HEIGHT, 0, 0);" in display_c and "dispPrvFrameCtrReset();" in display_c and "dma_hw->multi_channel_trigger = 1u << DISP_DMA_START_CH;" in display_c)
    accel_idle = function_body(main_c, "i2cAccelSetIdle")
    expect("dimmed menus put the accelerometer in 10 Hz low-power mode and restore active sampling", "#define ACCEL_CTRL1_IDLE                0x2f" in main_c and "#define ACCEL_CTRL4_IDLE                0xd0" in main_c and "#define ACCEL_CTRL1_ACTIVE              0x77" in main_c and "#define ACCEL_CTRL4_ACTIVE              0xd8" in main_c and "i2cPrvIsBuysy()" in accel_idle and "i2cAccelSetIdle(true)" in dim_observe and "i2cAccelSetIdle(false)" in function_body(main_c, "uiPrvDimRestore"))
    led_idle = function_body(badge_leds_c, "badgeLedsSetIdle")
    expect("dimmed menus turn off WS2812 LEDs and restore the configured pattern on wake", "void badgeLedsSetIdle(bool idle);" in badge_leds_h and "ws2812SetAllRgb(0, 0, 0);" in led_idle and "badgeLedsPrvRenderCurrent();" in led_idle and "mIdle" in function_body(badge_leds_c, "badgeLedsTick") and "badgeLedsSetIdle(true)" in dim_observe and "badgeLedsSetIdle(false)" in function_body(main_c, "uiPrvDimRestore"))
    expect("Screen settings update the active dim restore brightness", screen_settings.count("uiPowerSetActiveBrightness(settings->brightness)") >= 2 and "void uiPowerSetActiveBrightness(uint_fast8_t brightness);" in read("src/ui.h"))

    expect("USB Keyboard uses category submenus", "UiUsbKeyboardCategoryWindows" in ui and "mUsbKeyboardCategories" in ui and "selectedCommand[UiUsbKeyboardCategoryNum]" in keyboard_tool)
    expect("USB Keyboard has root quit and submenu back", '"A = Open   B = Quit"' in ui and '"A = Send   B = Back"' in ui)
    expect("USB Keyboard Windows replaces OS labels", '"Windows"' in ui and '"OS:' not in ui)

    expect("Autoclicker running footer says B stops", '"A/B Stop"' in autoclicker_draw)
    expect("Autoclicker B stops active run without exiting", "if (pressed & KEY_BIT_B)" in autoclicker_tool and "if (running)" in autoclicker_tool and "else\n\t\t\t\tbreak;" in autoclicker_tool)
    expect("Autoclicker releases HID reports on stop", autoclicker_tool.count("usbHidReleaseAll();") >= 3)
    expect("Autoclicker redraws only when displayed state changes", "if (redraw)" in autoclicker_tool and "now - lastDraw" not in autoclicker_tool)
    expect("BadUSB redraws only for visible progress changes", "lastProgressPct" in badusb_status and "lastDelayRemainSec" in badusb_status and "now - data->lastDraw" not in badusb_status)
    expect("USB connection waits draw their static screen once", all(body.count("uiPrvReset(cnv, false);") == 1 for body in (keyboard_wait, hid_wait, gamepad_wait)) and badusb_wait.count("uiPrvBadUsbShowState") == 1)
    expect("USB Gamepad redraws only on visible input or feedback changes", "haveDrawn" in gamepad_tool and "uiPrvGamepadFeedbackChanged" in gamepad_tool and "now - lastDraw" not in gamepad_tool)
    expect("USB Storage redraws only when its status snapshot changes", "memcmp(&state, &drawnState" in usb_storage_tool and "USB_STORAGE_REDRAW_TICKS" not in ui)
    expect("USB Storage updates changed rows without clearing the full framebuffer", "uiPrvUsbStorageUpdate(cnv, &drawnState, &state)" in usb_storage_tool and "uiPrvUsbStorageClearRow" in usb_storage_update and "uiPrvUsbStorageClearArea" in usb_storage_update and "uiPrvReset" not in usb_storage_update and all(field in usb_storage_update for field in ("oldState->mounted", "oldState->writable", "oldState->blocks", "oldState->lba", "oldState->bytes", "oldState->ejected", "oldState->error")))

    expect("Music low battery guard reads immediate and cached power", "badgePowerReadNow" in music_guard and "badgePowerGetCached" in music_guard)
    expect("Music low battery guard ignores USB power", "status.lowBatt" in music_guard and "status.usbMv < UI_HEADER_USB_PRESENT_MV" in music_guard)
    expect("Music guard protects app and file launches", "uiPrvMusicBatteryOkToLaunch" in music_app and "uiPrvMusicBatteryOkToLaunch" in launch_file and "uiPrvMusicBatteryOkToLaunch" in category_runner)
    expect("battery status exposes a calculated percentage", "uint8_t battPercent;" in badge_power_h and "badgePowerPrvPercent" in badge_power_c and "{3300, 0}" in badge_power_c and "{4200, 100}" in badge_power_c)
    expect("battery sampling is filtered and power-conscious", "TICKS_PER_SECOND * 15u" in badge_power_c and "mCachedStatus.battMv * 3u" in badge_power_c)
    expect("header displays percentage instead of volatile millivolts", '"%u%%"' in draw_header and '"%umV"' not in draw_header and "battPercent == mUiHeaderBattPercent" in draw_header)

    expect("DCAPP ABI includes the live port menu callback", "#define DCAPP_ABI_VERSION       5u" in dcapp_h and "DCAPP_ABI_VERSION = 5" in make_dcapp)
    expect("DCAPP host API exposes LED tick", "void (*ledsTick)(void);" in dcapp_h and ".ledsTick = badgeLedsTick" in dcapp_c)
    expect("DCAPP draw frames tick LEDs", "ctx->host->ledsTick" in dcapp_draw and "dcAppDrawPresent(ctx);" in dcapp_draw)
    expect("DCAPP host API exposes the live Ports menu", "bool (*portMenu)(struct Canvas *activeCanvas);" in dcapp_h and ".portMenu = uiPortMenu" in dcapp_c)
    expect("Ports FN menu offers resume, audio, screen, LEDs, and main menu", all(token in port_menu for token in ('"Resume"', '"Audio"', '"Screen"', '"LEDs"', '"Main Menu"', "uiPrvAudioSettings", "uiPrvScreenSettings", "uiPrvLedSettings", "settingsSet")))
    expect("Ports rotation updates the active canvas", "activeCanvas->flipped = settings.rotation" in port_menu)
    expect("shared port drawing opens FN menu instead of exiting", "ctx->host->portMenu(&ctx->displayCnv)" in dcapp_draw and "ctx->pressed = 0" in dcapp_draw)
    expect("Emulator FN menu exposes live Audio, Screen, and LED settings", all(token in game_menu for token in ('"Audio"', '"Screen"', '"LEDs"', "GameMenuOptionAudio", "GameMenuOptionScreen", "GameMenuOptionLeds", "uiPrvAudioSettings", "uiPrvScreenSettings", "uiPrvLedSettings", "settingsSet")))
    expect("universal Audio settings persist mute and volume", all(token in settings_h for token in ("audioVolume", "audioMuted")) and "#define SETTINGS_CUR_VER\t\t\t18" in settings_c and all(token in audio_settings for token in ('"MUTE:"', '"VOLUME:"', "audioMuted", "audioVolume", "uiPrvApplyAudioSettings")))
    expect("PWM master gain enforces universal Audio settings", "audioPwmSetMasterVolume" in audio_h and "mMasterVolume" in audio_c and "mVolume * mMasterVolume" in audio_c)

    expect("release workflow publishes only split SD bundles", '"$ARTIFACT_DIR/SD.zip"' not in build_workflow and "${{ runner.temp }}/DC32-cfw/SD.zip" not in build_workflow)
    expect("nightly workflow publishes only split SD bundles", '"$ARTIFACT_DIR/SD.zip"' not in nightly_workflow and '"$RUNNER_TEMP/DC32-cfw/SD.zip"' not in nightly_workflow)
    expect("release workflows purge legacy combined SD.zip", 'delete-asset "$RELEASE_TAG" SD.zip' in build_workflow and "delete-asset nightly SD.zip" in nightly_workflow)

    expect("Game Boy palette setting is persisted", "GameBoyPaletteNumPalettes" in settings_h and "uint8_t gbPalette;" in settings_h and "#define SETTINGS_CUR_VER\t\t\t18" in settings_c)
    palette_ids = [
        "GameBoyPaletteBw",
        "GameBoyPaletteDmg",
        "GameBoyPaletteGbpocket",
        "GameBoyPaletteBgb",
        "GameBoyPaletteGbli",
        "GameBoyPaletteGrafixkidgray",
        "GameBoyPaletteGrafixkidgreen",
        "GameBoyPaletteBlackzero",
        "GameBoyPaletteGbcjp",
        "GameBoyPaletteGbcu",
        "GameBoyPaletteGbcua",
        "GameBoyPaletteGbcub",
        "GameBoyPaletteGbcl",
        "GameBoyPaletteGbcla",
        "GameBoyPaletteGbclb",
        "GameBoyPaletteGbcd",
        "GameBoyPaletteGbcda",
        "GameBoyPaletteGbcdb",
        "GameBoyPaletteGbcr",
        "GameBoyPaletteGbceuus",
        "GameBoyPaletteGbcrb",
        "GameBoyPaletteGbcPreferred",
    ]
    expect("Game Boy palette defaults and normalizes", "settings->gbPalette = GameBoyPaletteGbcPreferred" in settings_c and "settings->gbPalette >= GameBoyPaletteNumPalettes" in settings_c)
    expect("Gameboy Settings exposes palette selector", '"PALETTE:"' in game_settings and "paletteNames[GameBoyPaletteNumPalettes]" in game_settings and "settings->gbPalette" in game_settings and '"GBC Preferred"' in game_settings)
    expect("Gameboy Settings includes gb-palettes bw through gbcrb", all(pid in settings_h and f"[{pid}]" in gb and f"[{pid}]" in game_settings for pid in palette_ids))
    expect("Gameboy Settings uses color palette names", '"Pale Yellow"' in game_settings and '"Dark Green"' in game_settings and '"Reverse"' in game_settings and "Game Boy Color Splash" not in game_settings and '"DkGrn"' not in game_settings and '"OrigGB"' not in game_settings)
    expect("GB launch passes selected palette", ".gbPalette = desiredGbPalette()" in main_c and "gbSetDmgPaletteForRom(args->rom, args->romSize, args->gbPalette)" in gb_app and "void gbSetDmgPaletteForRom(const void *rom, uint32_t romSize, uint_fast8_t palette);" in gb_h)
    expect("DMG-only GB games stay on DMG palette path", "presentAsCgb && (gbExtRead(0x143) & 0x80)" in gb_run)
    expect("GB startup refreshes cached DMG palette registers", "gbIoWrite(0x47, hram[0x47]);" in gb_run and "gbIoWrite(0x48, hram[0x48]);" in gb_run and "gbIoWrite(0x49, hram[0x49]);" in gb_run)
    expect("DMG palette hook uses selectable BG and OBJ planes", "mGbDmgPaletteColors[GameBoyPaletteNumPalettes][GbDmgPaletteNumPlanes][4]" in gb and "GbDmgPaletteBg" in gb and "GbDmgPaletteObj0" in gb and "GbDmgPaletteObj1" in gb and "colors[(regVal >> 0) & 3]" in dmg_palette)
    expect("GBC preferred palette uses boot ROM title checksum table", "mGbcBootTitleChecksums" in gb and "mGbcBootPaletteByTitleChecksum" in gb and "mGbcBootDuplicateFourthLetters" in gb and "hdr->titleDMG[3]" in gb and "titleChecksum += hdr->titleDMG[i]" in gb)
    expect("GBC preferred palette preserves raw combination offsets", "mGbcBootPaletteCombinationOffsets" in gb and "combinationOffset + 2u" in gb and "mGbcBootPaletteByTitleChecksum[i] & 0x7f" in gb)
    expect("GBC key palettes keep distinct sprite planes", "GB_DMG_PLANES(GB_GBC_BOOT_PAL_2, GB_GBC_BOOT_PAL_4, GB_GBC_BOOT_PAL_0)" in gb and "GB_DMG_PLANES(GB_GBC_BOOT_PAL_29, GB_GBC_BOOT_PAL_4, GB_GBC_BOOT_PAL_4)" in gb)
    expect("DMG palette table includes source colorways in RGXB5515", "0x9DC1" in gb and "0xC654" in gb and "0x1ED9" in gb and "0x0410" in gb)
    expect("DMG palette selection is sanitized", "palette < GameBoyPaletteNumPalettes ? palette : GameBoyPaletteBw" in set_dmg_palette)
    expect("GB sprite priority uses side buffer marker", "bgToOamPrioPtr[i] & BG_FLAG_UNDER_OBJS" in video_c and "dst[i] & BG_FLAG_UNDER_OBJS" not in video_c)

    expect("current console detection ignores removed act-like-GBC toggle", "RomNoColor" in console_name and "RomColorRequired" in console_name and "settings.actLikeGBC" not in console_name)
    expect("GB Save to SD is visible for any valid ROM", "uiPrvHaveValidRom(name, NULL, NULL)" in game_menu and "if (validRom) {\n\t\tlabels[numOptions] = \"Save to SD\";" in game_menu)
    expect("manual Save to SD reports no battery save", "nothingToExport" in ui and "This game has no battery save to export." in ui)
    expect("non-manual empty save export stays quiet", "if (result->manualRequest)\n\t\t\tuiAlert(cnv, \"This game has no battery save to export.\"" in save_export_success and "result.manualRequest = force" in save_export_options)
    expect("return to main menu exports current save with popup", "mLastGameMenuAction == UiGameActionSwitchTool" in run_loaded_game and "uiPrvExportCurrentSavestateWithOptions(cnv, false, true)" in run_loaded_game)
    expect("emulator saves use console subfolders", all(token in save_subdir for token in ['"AB"', '"GB"', '"GBC"', '"NES"']) and "uiPrvOpenSaveDirForConsole(vol, console, false)" in save_open and "uiPrvOpenSaveDirForConsole(vol, result->saveConsole, true)" in ui)
    expect("emulator saves default to full ROM-derived names", "uiPrvCopyRomStem(stem" in save_name and "SaveNameKindFull" in ui and "saveNameKind = SaveNameKindFull" in ui)
    expect("emulator saves retain cleaned detected-name compatibility", "uiPrvDetectedSaveTitle" in clean_save_name and "cleanSaveName" in save_open and "uiPrvCleanSaveFileName((const char*)QSPI_FILENAME_START" in save_export_prepare)
    expect("obsolete no-extension legacy save lookup is removed", "SaveNameKindLegacy" not in ui and "uiPrvRawSaveFileName" not in ui)

    print("release polish static tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
