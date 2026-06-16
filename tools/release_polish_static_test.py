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
    settings_h = read("src/settings.h")
    settings_c = read("src/settings.c")
    main_c = read("src/main_rp2350_defcon.c")
    make_dcapp = read("tools/make_dcapp.py")

    open_with = function_body(ui, "uiPrvBrowserOpenWith")
    launch_file = function_body(ui, "uiPrvLaunchBrowserFile")
    ir_button = function_body(ui, "uiPrvIrSendButtonSpamFile")
    ir_read_line = function_body(ui, "uiPrvReadLine")
    ir_read_line_stat = function_body(ui, "uiPrvIrReadLineStat")
    keyboard_tool = function_body(ui, "uiPrvUsbKeyboardTool")
    autoclicker_draw = function_body(ui, "uiPrvAutoclickerDraw")
    autoclicker_tool = function_body(ui, "uiPrvAutoclickerTool")
    music_guard = function_body(ui, "uiPrvMusicBatteryOkToLaunch")
    music_app = function_body(ui, "uiDcAppRunMusic")
    category_runner = function_body(ui, "uiPrvRunCategoryToolEntry")
    game_menu = function_body(ui, "uiGameMenu")
    run_loaded_game = function_body(ui, "uiPrvRunLoadedGame")
    console_name = function_body(ui, "uiPrvCurrentGameConsole")
    save_export_success = function_body(ui, "uiPrvSaveExportShowSuccess")
    save_export_options = function_body(ui, "uiPrvExportCurrentSavestateWithOptions")
    save_export_prepare = function_body(ui, "uiPrvPrepareSaveExportResult")
    save_open = function_body(ui, "uiPrvOpenSaveFile")
    save_subdir = function_body(ui, "uiPrvSaveSubdirForConsole")
    save_name = function_body(ui, "uiPrvSaveFileName")
    game_settings = function_body(ui, "uiPrvGameSettings")
    dmg_palette = function_body(gb, "gbPrvRecalcDmgPal")
    set_dmg_palette = function_body(gb, "gbSetDmgPalette")
    gb_run = function_body(gbc, "gbRun")

    expect(".ir Open With only offers Universal IR", '"Universal IR"' in open_with and '"Power"' not in open_with and '"Mute"' not in open_with)
    expect(".DC32 is registered in File Browser", 'uiPrvStrEndsWithNoCase(ref->name, ".DC32")' in open_with and "UiBrowserOpenDcApp" in launch_file)
    expect(".DC32 launch is catalog-only", "uiPrvBrowserFindDcApp" in ui and "That .DC32 app is not registered." in launch_file)
    expect("runtime DCAPP engines guide users to Emulators", "uiPrvBrowserDcAppIsRuntimeEngine" in ui and "Open Emulators from Games to run ROMs." in launch_file)

    expect("IR selected button becomes progress title", 'buttonName && *buttonName ? buttonName : "IR Button"' in ir_button)
    expect("IR line reader can cancel pathological reads", "uiPrvIrCancelRequested()" in ir_read_line and "cancelledP" in ir_read_line)
    expect("IR line reader propagates cancel to stats", "stats->cancelled = true" in ir_read_line_stat)

    expect("USB storage fast speed is 12.5 MHz", "#define USB_MSC_FAST_SD_SPEED\t12500000" in usb_msc)
    expect("USB storage safe fallback remains 500 kHz", "#define USB_MSC_NORMAL_SD_SPEED\t500000" in usb_msc and "usbMscPrvFallbackSpeed" in usb_msc)

    expect("USB Keyboard uses category submenus", "UiUsbKeyboardCategoryWindows" in ui and "mUsbKeyboardCategories" in ui and "selectedCommand[UiUsbKeyboardCategoryNum]" in keyboard_tool)
    expect("USB Keyboard has root quit and submenu back", '"A = Open   B = Quit"' in ui and '"A = Send   B = Back"' in ui)
    expect("USB Keyboard Windows replaces OS labels", '"Windows"' in ui and '"OS:' not in ui)

    expect("Autoclicker running footer says B stops", '"A/B Stop"' in autoclicker_draw)
    expect("Autoclicker B stops active run without exiting", "if (pressed & KEY_BIT_B)" in autoclicker_tool and "if (running)" in autoclicker_tool and "else\n\t\t\t\tbreak;" in autoclicker_tool)
    expect("Autoclicker releases HID reports on stop", autoclicker_tool.count("usbHidReleaseAll();") >= 3)

    expect("Music low battery guard reads immediate and cached power", "badgePowerReadNow" in music_guard and "badgePowerGetCached" in music_guard)
    expect("Music low battery guard ignores USB power", "status.lowBatt" in music_guard and "status.usbMv < UI_HEADER_USB_PRESENT_MV" in music_guard)
    expect("Music guard protects app and file launches", "uiPrvMusicBatteryOkToLaunch" in music_app and "uiPrvMusicBatteryOkToLaunch" in launch_file and "uiPrvMusicBatteryOkToLaunch" in category_runner)

    expect("DCAPP ABI bumped for host callback and GB palette args", "#define DCAPP_ABI_VERSION       4u" in dcapp_h and "DCAPP_ABI_VERSION = 4" in make_dcapp)
    expect("DCAPP host API exposes LED tick", "void (*ledsTick)(void);" in dcapp_h and ".ledsTick = badgeLedsTick" in dcapp_c)
    expect("DCAPP draw frames tick LEDs", "ctx->host->ledsTick" in dcapp_draw and "dcAppDrawPresent(ctx);" in dcapp_draw)

    expect("Game Boy palette setting is persisted", "GameBoyPaletteNumPalettes" in settings_h and "uint8_t gbPalette;" in settings_h and "#define SETTINGS_CUR_VER\t\t\t17" in settings_c)
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
    expect("emulator saves prefer detected titles", "detectedName" in save_name and "uiPrvDetectedSaveTitle" in save_name and "uiPrvDetectedSaveTitleForSelection(&selection, detectedName" in save_export_prepare)
    expect("legacy ROM-name save lookup is removed", "SaveNameKindLegacy" not in ui and "uiPrvRawSaveFileName" not in ui)

    print("release polish static tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
