#!/usr/bin/env python3
"""Static checks for accepted faithful period ports and pack builders."""

from __future__ import annotations

import struct
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RESERVED = {}
ACCEPTED = {
    "DcAppIdChips": (207, "chips.DC32", "Chip's Challenge"),
    "DcAppIdScorch": (208, "scorch.DC32", "Scorched Earth"),
    "DcAppIdPipe": (209, "pipe.DC32", "Pipe Dream"),
    "DcAppIdCave": (210, "cave.DC32", "Cave Story"),
    "DcAppIdSokoban": (211, "sokoban.DC32", "Sokoban"),
    "DcAppIdOpenJazz": (212, "openjazz.DC32", "Jazz Jackrabbit"),
}


def expect(name: str, condition: bool) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)


def check_sources_removed() -> None:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    ui_c = (ROOT / "src" / "ui.c").read_text(encoding="utf-8")
    dcapp_c = (ROOT / "src" / "dcApp.c").read_text(encoding="utf-8")
    dcapp_h = (ROOT / "src" / "dcApp.h").read_text(encoding="utf-8")
    sd_zip = (ROOT / "tools" / "build_sd_zip.py").read_text(encoding="utf-8")
    apps_dir = ROOT / "build" / "apps"

    expect("handmade period source is removed", not (ROOT / "src" / "apps" / "period" / "period_ports.c").exists())
    expect("period CMake source list is removed", "PERIOD_APP_SOURCES" not in cmake)
    expect("generated fake cave data target is removed", "dcapp_cave_data" not in cmake and "cave.dat" not in cmake)
    for symbol, (app_id, filename, label) in ACCEPTED.items():
        expect(f"{symbol} ID is assigned to accepted port", f"{symbol} = {app_id}" in dcapp_h)
        expect(f"{filename} accepted port is built", filename.removesuffix(".DC32") in cmake)
        expect(f"{filename} accepted port is packaged", f'"{filename}"' in sd_zip)
        expect(f"{label} accepted port is visible in Games menu", label in ui_c)
        expect(f"{label} catalog entry is visible", f'{{{symbol}, "{label}", "/APPS/{filename}", true}}' in dcapp_c)
        if apps_dir.exists():
            expect(f"{filename} accepted build artifact exists", (apps_dir / filename).exists())
            if filename == "sokoban.DC32":
                expect("XSokoban pack build artifact exists", (apps_dir / "sokoban-xsokoban.pak").exists())
            if filename == "chips.DC32":
                expect("Tile World pack build artifact exists", (apps_dir / "chips-tworld.pak").exists())
            if filename == "pipe.DC32":
                expect("PipeDreamer pack build artifact exists", (apps_dir / "pipe-pipedreamer.pak").exists())
            if filename == "scorch.DC32":
                expect("xscorch pack build artifact exists", (apps_dir / "scorch-xscorch.pak").exists())
    for symbol, (app_id, filename, label) in RESERVED.items():
        expect(f"{symbol} ID is assigned", f"{symbol} = {app_id}" in dcapp_h)
        expect(f"{filename} is not built", filename.removesuffix(".DC32") not in cmake)
        expect(f"{filename} is not packaged", f'"{filename}"' not in sd_zip)
        if apps_dir.exists():
            expect(f"{filename} stale build artifact is absent", not (apps_dir / filename).exists())
        expect(f"{label} is not visible in Games menu", label not in ui_c)
        expect(f"{label} catalog entry is hidden", f'{{{symbol}, "{label}", "/APPS/{filename}", false}}' in dcapp_c)
    if apps_dir.exists():
        expect("stale cave.dat build artifact is absent", not (apps_dir / "cave.dat").exists())
    expect("period user-data README is packaged", "README-period-ports.txt" in sd_zip)
    expect("Cave Story binary is packaged without user data", '"cave.DC32"' in sd_zip and '"APPS/cave.pak"' not in sd_zip and "CAVE_DATA_SOURCE" not in sd_zip)
    expect("Chip's user data is not redistributed", '"APPS/chips.pak"' not in sd_zip and "chips-tworld.pak" in sd_zip)
    expect("xscorch redistributable pack is packaged", '"APPS/scorch-xscorch.pak"' in sd_zip and "XSCORCH_PACK_SOURCE" in sd_zip)
    expect("PipeDreamer redistributable pack is packaged", '"APPS/pipe-pipedreamer.pak"' in sd_zip and "PIPEDREAMER_PACK_SOURCE" in sd_zip)


def check_port_runtime_scaffold() -> None:
    header = (ROOT / "src" / "apps" / "port" / "port_runtime.h").read_text(encoding="utf-8")
    source = (ROOT / "src" / "apps" / "port" / "port_runtime.c").read_text(encoding="utf-8")

    expect("port runtime declares 320x240 bridge", "DC32_PORT_SCREEN_W 320u" in header and "DC32_PORT_SCREEN_H 240u" in header)
    expect("port runtime uses cart scratch heap", "DC32_PORT_CART_HEAP_START" in header and "dc32PortHeapInitDefault" in source)
    expect("port runtime has free-list allocator", "struct Dc32PortBlock" in source and "dc32PortCoalesce" in source)
    expect("port runtime has FAT asset pack reader", "dc32PortOpenAssetPack" in source and "dc32PortReadAssetPack" in source)
    expect("port runtime has save helpers", "dc32PortSaveRead" in source and '"/SAVE/"' in source)
    expect("port runtime exits on center", "UI_KEY_BIT_CENTER" in source and "dc32PortCenterExitRequested" in source)


def check_xsokoban_source_and_generator(tmp: Path) -> None:
    source_root = ROOT / "third_party" / "xsokoban"
    out_c = tmp / "xsokoban_assets.c"
    out_h = tmp / "xsokoban_assets.h"
    readme = (source_root / "README").read_text(encoding="utf-8")

    expect("XSokoban source tree is vendored", source_root.is_dir())
    expect("XSokoban public domain redistribution note is retained", "public domain" in readme and "original 50" in readme and "40 additional" in readme)
    expect("XSokoban has 90 screen files", len(list((source_root / "screens").glob("screen.*"))) == 90)
    expect("XSokoban 20px pixmaps are vendored", (source_root / "bitmaps" / "20" / "man.xpm").is_file())
    result = run([
        sys.executable,
        "tools/build_xsokoban_assets.py",
        "--source-root",
        str(source_root),
        "--output-c",
        str(out_c),
        "--output-h",
        str(out_h),
    ])
    expect("XSokoban asset generator succeeds", result.returncode == 0 and out_c.is_file() and out_h.is_file())
    generated = out_c.read_text(encoding="ascii")
    header = out_h.read_text(encoding="ascii")
    expect("XSokoban generator emits 90 levels", generated.count("/* screen.") == 90 and "XSOKOBAN_LEVEL_COUNT 90u" in header)
    expect("XSokoban generator emits pixmap-style tiles", "man.xpm" in generated and "centerwall.xpm" in generated)


def check_tworld_source_and_generator(tmp: Path) -> None:
    source_root = ROOT / "third_party" / "tworld"
    out_c = tmp / "tworld_assets.c"
    out_h = tmp / "tworld_assets.h"
    readme = (source_root / "README").read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    chips_port = (ROOT / "src" / "apps" / "chips" / "chips_port.c").read_text(encoding="utf-8")

    expect("Tile World source tree is vendored", source_root.is_dir())
    expect("Tile World GPL notice is retained", "GNU General Public License" in readme and "Brian Raiter" in readme)
    expect("Tile World core source is vendored", (source_root / "mslogic.c").is_file() and (source_root / "lxlogic.c").is_file())
    expect("Tile World tile bitmap is vendored", (source_root / "res" / "tiles.bmp").is_file())
    result = run([
        sys.executable,
        "tools/build_tworld_assets.py",
        "--source-root",
        str(source_root),
        "--output-c",
        str(out_c),
        "--output-h",
        str(out_h),
    ])
    expect("Tile World asset generator succeeds", result.returncode == 0 and out_c.is_file() and out_h.is_file())
    generated = out_c.read_text(encoding="ascii")
    header = out_h.read_text(encoding="ascii")
    expect("Tile World generator emits 24px tiles", "#define TWORLD_TILE_SIZE 24u" in header and generated.count("/* tile 0x") == 256)
    expect("Chip's app uses Tile World logic", "initgamestate" in chips_port and "doturn" in chips_port and "tworldcurrentstate" in chips_port)
    expect("Chip's app requires user chips.pak", 'CHIPS_PACK_PATH "/APPS/chips.pak"' in chips_port)
    expect("Chip's app supports optional original-style tile pack", all(token in chips_port for token in ("DC32CHIPTIL", "chipsLoadTilePack", "tiles.bin", "tilePixels", "tileAlpha")))
    expect("Chip's app is locked to Win 3.1/MS rules", "DC32_CHIPS_MS_ONLY=1" in cmake and "${TWORLD_SOURCE_DIR}/lxlogic.c" not in cmake and "Ruleset_Lynx" not in chips_port and "SEL RULES" not in chips_port)
    expect("Chip's app renders inventory icons", all(token in chips_port for token in ("chipsDrawInventory", "Key_Red", "Boots_Ice", "chipsDrawInventoryTile")))
    expect("Chip's app is large-XIP", "add_dcapp(dcapp_chips chips 207 LARGE_XIP" in cmake)


def check_pipedreamer_source() -> None:
    source_root = ROOT / "third_party" / "PipeDreamer"
    license_text = (source_root / "LICENSE").read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    pipe_app = (ROOT / "src" / "apps" / "pipe" / "pipe_app.cpp").read_text(encoding="utf-8")

    expect("PipeDreamer source tree is vendored", source_root.is_dir())
    expect("PipeDreamer MIT license is retained", "Permission is hereby granted" in license_text and "Bernardo Escalona" in license_text)
    expect("PipeDreamer gameplay core is vendored", all((source_root / "Source" / name).is_file() for name in ("Board.cpp", "Queue.cpp", "TilePiece.cpp", "Randomizer.cpp")))
    expect("PipeDreamer reference art is vendored", (source_root / "Resources" / "Images" / "PipeDreamerCanvas.png").is_file())
    expect("Pipe app uses PipeDreamer logic", all(token in pipe_app for token in ("new Board", "new Queue", "Pump(", "PopBomb()", "ReplaceTile")))
    expect("Pipe app preserves fast-forward and level carryover", "fastForward" in pipe_app and "cumulativeScore" in pipe_app and "pipeRoundTotal" in pipe_app)
    expect("Pipe app checks redistributable asset pack", 'PIPE_ASSET_PATH = "/APPS/pipe-pipedreamer.pak"' in pipe_app and "dc32PortOpenAssetPack" in pipe_app)
    expect("Pipe app is large-XIP", "add_dcapp(dcapp_pipe pipe 209 LARGE_XIP" in cmake)


def check_xscorch_source_and_generator(tmp: Path) -> None:
    source_root = ROOT / "third_party" / "xscorch"
    out_c = tmp / "xscorch_assets.c"
    out_h = tmp / "xscorch_assets.h"
    readme = (source_root / "README").read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    scorch_app = (ROOT / "src" / "apps" / "scorch" / "scorch_app.c").read_text(encoding="utf-8")

    expect("xscorch source tree is vendored", source_root.is_dir())
    expect("xscorch GPLv2-only note is retained", "GNU" in readme and "General Public License ONLY" in readme and "NO later version" in readme)
    expect("xscorch weapon data is vendored", (source_root / "data" / "weapons.def").is_file())
    expect("xscorch accessory data is vendored", (source_root / "data" / "accessories.def").is_file())
    result = run([
        sys.executable,
        "tools/build_xscorch_assets.py",
        "--source-root",
        str(source_root),
        "--output-c",
        str(out_c),
        "--output-h",
        str(out_h),
    ])
    expect("xscorch asset generator succeeds", result.returncode == 0 and out_c.is_file() and out_h.is_file())
    generated = out_c.read_text(encoding="ascii")
    header = out_h.read_text(encoding="ascii")
    expect("xscorch generator emits weapon table", "XSCORCH_WEAPON_COUNT 39u" in header and generated.count("/* ") == 39)
    expect("Scorch app uses generated xscorch weapons", "xscorchWeapons" in scorch_app and "scorchFindWeapon" in scorch_app)
    expect("Scorch app implements terrain, AI, shop, and turn flow", all(token in scorch_app for token in ("scorchGenerateTerrain", "scorchAiTurn", "scorchBuySelected", "scorchNextTurn")))
    expect("Scorch app checks redistributable asset pack", 'SCORCH_ASSET_PATH "/APPS/scorch-xscorch.pak"' in scorch_app and "dc32PortOpenAssetPack" in scorch_app)
    expect("Scorch app is large-XIP", "add_dcapp(dcapp_scorch scorch 208 LARGE_XIP" in cmake)


def check_cave_source_and_loader() -> None:
    nx_root = ROOT / "third_party" / "nxengine-evo"
    drs_root = ROOT / "third_party" / "doukutsu-rs"
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    cave_app = (ROOT / "src" / "apps" / "cave" / "cave_app.c").read_text(encoding="utf-8")

    expect("NXEngine-evo reference source is vendored", (nx_root / "src" / "map.cpp").is_file() and (nx_root / "src" / "graphics" / "Tileset.cpp").is_file())
    expect("doukutsu-rs reference source is vendored", (drs_root / "src" / "game" / "map.rs").is_file() and (drs_root / "src" / "game" / "stage.rs").is_file())
    expect("Cave app requires user cave.pak", 'CAVE_PACK_PATH "/APPS/cave.pak"' in cave_app and "dc32PortOpenAssetPack" in cave_app)
    expect("Cave app parses Cave pack entries", "DC32CAVEPAK" in cave_app and "cavePackReadEntries" in cave_app)
    expect("Cave app parses PXM maps and Stage.dat", all(token in cave_app for token in ("PXM", "Stage.dat", "caveLoadStageRecord", "caveLoadMap")))
    expect("Cave app parses PXE entities", all(token in cave_app for token in ("PXE", "caveLoadEntities", "eventNum", "npcType")))
    expect("Cave app loads TSC scripts", all(token in cave_app for token in ("Head.tsc", ".tsc", "caveStartScript", "caveScriptStep", "TRA", "MOV", "NOD")))
    expect("Cave app loads Cave tile attributes", all(token in cave_app for token in ("tilekey.dat", ".pxa", "caveLoadTileAttrs", "CAVE_TA_SOLID_PLAYER")))
    expect("Cave app loads NPC table and sheets", all(token in cave_app for token in ("npc.tbl", "caveLoadNpcTable", "caveLoadCommonNpcSheets", "caveLoadStageNpcSheets", "Npc/Npc%s.pbm")))
    expect("Cave app renders NPC sprites from original sheets", all(token in cave_app for token in ("caveNpcSheetForId", "caveDrawNpcSprite", "dispLeft", "sheetId")))
    expect("Cave app uses NXEngine sprite metadata", all(token in cave_app for token in ("cave_assets.h", "caveNxObjectSprites", "caveDrawNxObjectSprite")) and "build_cave_assets.py" in cmake)
    expect("Cave app avoids heap framebuffer pressure", "static uint8_t mCaveFrame" in cave_app and "dc32PortMalloc(CAVE_SCREEN_W * CAVE_SCREEN_H)" not in cave_app)
    expect("Cave app removes visible placeholder entities", "CAVE_FLAG_SCRIPT_ON_ACTIVATE) ? caveRgb" not in cave_app)
    expect("Cave app has side-view player physics", all(token in cave_app for token in ("playerVx", "playerVy", "playerOnGround", "caveApplyPlayerPhysics", "caveMovePlayerFixedY")))
    expect("Cave app fires Polar Star-style bullets", all(token in cave_app for token in ("Bullet.pbm", "CAVE_MAX_BULLETS", "caveFirePolarStar", "caveTickBullets", "CAVE_FLAG_SHOOTABLE")))
    expect("Cave app persists Cave game state", all(token in cave_app for token in ("CAVE_NUM_GAME_FLAGS", "CAVE_MAP_FLAG_COUNT", "skipFlags", "mapFlags", "itemFlags", "weaponFlags", "equipMask", "CAVE_SAVE_VERSION 3u")))
    expect("Cave app handles Cave TSC branch and inventory commands", all(token in cave_app for token in ("FLJ", "SKJ", "ITJ", "AMJ", "NCJ", "ECJ", "caveScriptJump")))
    expect("Cave app handles Cave TSC entity commands", all(token in cave_app for token in ("DNP", "DNA", "ANP", "CNP", "INP", "MNP", "caveRefreshEntityVisibility")))
    expect("Cave app handles Cave TSC world commands", all(token in cave_app for token in ("CMP", "SMP", "SNP", "MP+", "MPJ", "MYD", "MYB", "QUA", "FAI", "FAO", "SOU")))
    expect("Cave app renders Cave text box modes", all(token in cave_app for token in ("messageTop", "messageBorder", "faceId", "itemId", "caveScriptAppendNumber")))
    expect("Cave app decodes PBM/BMP tilesets", "caveLoadBitmap" in cave_app and "Stage/Prt%s.pbm" in cave_app)
    expect("Cave app saves and transitions stages", "caveSaveGame" in cave_app and "caveTransition" in cave_app)
    expect("Cave app is large-XIP", "add_dcapp(dcapp_cave cave 210 LARGE_XIP" in cmake)


def check_openjazz_source_and_packer(tmp: Path) -> None:
    source_root = ROOT / "third_party" / "openjazz"
    shareware_root = ROOT / "third_party" / "openjazz-shareware"
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    app = (ROOT / "src" / "apps" / "openjazz" / "openjazz_app.cpp").read_text(encoding="utf-8")
    cache = (ROOT / "src" / "apps" / "openjazz" / "openjazz_cache.cpp").read_text(encoding="utf-8")
    heap = (ROOT / "src" / "apps" / "openjazz" / "openjazz_heap.c").read_text(encoding="utf-8")
    memory = (ROOT / "src" / "apps" / "openjazz" / "openjazz_memory.cpp").read_text(encoding="utf-8")
    memory_header = (ROOT / "src" / "apps" / "openjazz" / "openjazz_memory.h").read_text(encoding="utf-8")
    palette_effects = (source_root / "src" / "io" / "gfx" / "paletteeffects.cpp").read_text(encoding="utf-8")
    palette_header = (source_root / "src" / "io" / "gfx" / "paletteeffects.h").read_text(encoding="utf-8")
    port_header = (ROOT / "src" / "apps" / "port" / "port_runtime.h").read_text(encoding="utf-8")
    firmware_linker = (ROOT / "src" / "linker_rp2350_defcon.lkr").read_text(encoding="utf-8")
    installer = (ROOT / "src" / "apps" / "openjazz" / "openjazz_install.cpp").read_text(encoding="utf-8")
    pack = (ROOT / "src" / "apps" / "openjazz" / "openjazz_pack.cpp").read_text(encoding="utf-8")
    sdl = (ROOT / "src" / "apps" / "openjazz" / "openjazz_sdl.cpp").read_text(encoding="utf-8")
    file_io = (source_root / "src" / "io" / "file.cpp").read_text(encoding="utf-8")
    font_source = (source_root / "src" / "io" / "gfx" / "font.cpp").read_text(encoding="utf-8")
    setup_io = (source_root / "src" / "setup.cpp").read_text(encoding="utf-8")
    save_io = (source_root / "src" / "jj1" / "save" / "jj1save.cpp").read_text(encoding="utf-8")
    main_menu = (source_root / "src" / "menu" / "mainmenu.cpp").read_text(encoding="utf-8")
    game_menu = (source_root / "src" / "menu" / "gamemenu.cpp").read_text(encoding="utf-8")
    game_source = (source_root / "src" / "game" / "game.cpp").read_text(encoding="utf-8")
    game_header = (source_root / "src" / "game" / "game.h").read_text(encoding="utf-8")
    level_common = (source_root / "src" / "level" / "level.cpp").read_text(encoding="utf-8")
    player_source = (source_root / "src" / "player" / "player.cpp").read_text(encoding="utf-8")
    tool = (ROOT / "tools" / "build_openjazz_pack.py").read_text(encoding="utf-8")
    sd_zip = (ROOT / "tools" / "build_sd_zip.py").read_text(encoding="utf-8")
    readme = (source_root / "README.md").read_text(encoding="utf-8")
    licenses = (source_root / "doc" / "licenses.txt").read_text(encoding="utf-8")
    data = tmp / "jazz" / "nested"
    data.mkdir(parents=True)
    required = (
        "PANEL.000", "MENU.000", "MAINCHAR.000",
        "FONT2.0FN", "FONTBIG.0FN", "FONTINY.0FN", "FONTMN1.0FN", "FONTMN2.0FN",
        "STARTUP.0SC", "SOUNDS.000", "LEVEL0.000", "LEVEL1.000", "LEVEL2.000",
        "BLOCKS.000", "SPRITES.000", "PLANET.000",
    )
    included_extra = ("LEVEL0.001", "BONUS.018", "SONG.0FM", "SONG.PSM", "MACRO.1", "MACRO.4")
    for name in (*required, *included_extra):
        (data / name).write_bytes((name + "\n").encode("ascii"))
    (data / "README.TXT").write_text("ignored\n", encoding="ascii")
    out = tmp / "openjazz-fixture.pak"
    shareware_out = tmp / "openjazz-shareware.pak"

    expect("OpenJazz source tree is vendored", (source_root / "src" / "main.cpp").is_file() and (source_root / "COPYING").is_file())
    expect("OpenJazz GPL notice is retained", "GNU General Public License version 2 or later" in readme and "OpenJazz" in licenses)
    expect("OpenJazz target uses standard-cache exception-aware app", "add_dcapp(dcapp_openjazz openjazz 212 CXX_EXCEPTIONS" in cmake and "LARGE_XIP CXX_EXCEPTIONS" not in cmake[cmake.find("add_dcapp(dcapp_openjazz"):cmake.find("target_include_directories(dcapp_openjazz")])
    expect("OpenJazz uses the standard XIP load address", ".loadAddr = 0x10080000u" in app and ".flags = 0" in app)
    expect("OpenJazz has a standard exception linker", "DCAPP_EH_LINKER_SCRIPT" in cmake and (ROOT / "src" / "linker_dcapp_eh.lkr").is_file())
    expect("OpenJazz audio is intentionally silent", "openjazz_sound_stub.cpp" in cmake and "MUSIC_SETTINGS=0" in cmake)
    expect("OpenJazz includes first-run ZIP installer", "openjazz_install.cpp" in cmake and "mz_zip_reader_extract_to_callback" in installer)
    expect("OpenJazz installer preserves ZIP and removes partial packs", "DC32_OPENJAZZ_ZIP_PATH" in installer and "fatfsFileDelete" in installer)
    expect("OpenJazz pack reader builds a validated index", all(token in pack for token in ("gOjEntries", "ojLoadIndex", "OJ_MAX_ENTRIES", "ojFindEntry")))
    expect("OpenJazz validates playable core files", all(name in pack and name in installer for name in required))
    expect("OpenJazz writes config and four save slots", all(token in pack for token in ("OJCFG.DAT", "OJSAVE0.DAT", "OJSAVE3.DAT", "dc32OjFileWrite")))
    expect("OpenJazz upstream file layer uses writable badge files", "dc32OjUserFileOpen" in file_io and "dc32OjFileWrite" in file_io)
    expect("OpenJazz first boot avoids missing config/save exceptions", "File::exists(CONFIG_FILE, PATH_TYPE_CONFIG)" in setup_io and "File::exists(fileName, PATH_TYPE_CONFIG)" in save_io)
    expect("OpenJazz save lookup uses direct SAVE directory access", "fatfsFindFileAt" in pack and "fatfsFileOpenAt" in pack)
    expect("OpenJazz SDL shim maps badge-native gameplay controls", all(token in sdl for token in ("KEY_BIT_A", "KEY_BIT_B", "KEY_BIT_START", "KEY_BIT_SEL", "UI_KEY_BIT_CENTER", "return SDLK_p;", "return SDLK_RCTRL;", "return SDLK_ESCAPE;", "SDL_Flip")) and all(token not in sdl for token in ("gOjSelectHeld", "gOjChordActive", "ojChordKey")))
    expect("OpenJazz app has fatal allocation recovery", "setjmp" in app and "longjmp" in app and "HeapPeak" in app)
    expect("OpenJazz splits persistent auxiliary SRAM from transient app scratch", all(token in app for token in ("dcAppGetActiveScratch", "dc32OjHeapInit", "DC32_PORT_OPENJAZZ_AUX_START", "scratch.ptr", "scratch.size")) and "dc32PortHeapAddRegion" not in app)
    expect("OpenJazz accepts any build-contract scratch span with useful startup diagnostics", all(token in app for token in ("DC32_OJ_TRANSIENT_MIN", "Scratch missing", "Scratch too small", "Heap init failed", "Have %u; need %u")) and "80u * 1024u" not in app)
    expect("OpenJazz split heap guards metadata and bounds traversal", all(token in heap for token in ("OJ_HEAP_HEADER_MAGIC", "OJ_HEAP_FOOT_MAGIC", "OJ_HEAP_SIZE_XOR", "limit--", "DC32_OJ_HEAP_DOUBLE_FREE", "DC32_OJ_HEAP_INVALID_FREE")))
    expect("OpenJazz level memory resets independently of persistent state", all(token in memory for token in ("dc32OjBeginLevelMemory", "dc32OjEndLevelMemory", "dc32OjHeapResetTransient", "DC32_OJ_HEAP_PERSISTENT", "DC32_OJ_HEAP_TRANSIENT")))
    expect("OpenJazz reserves fixed canvas and level arenas", all(token in port_header for token in ("0x20000000u", "0x00012c00u", "0x20012c00u", "0x00018400u", "0x20056000u", "0x00009000u")))
    expect("Firmware linker stops before the OpenJazz auxiliary arena", 'LENGTH = 0x2B000' in firmware_linker and "__ramvec_end <= 0x20056000" in firmware_linker)
    expect("OpenJazz enforces gameplay heap reserves", "48u * 1024u" in memory and "32u * 1024u" in memory)
    expect("OpenJazz reserves the fixed level-arena tail for palette effects", all(token in memory_header for token in ("DC32_OJ_LEVEL_OBJECT_LIMIT   0x00016000u", "DC32_OJ_PALETTE_SLOT_SIZE    32u", "DC32_OJ_PALETTE_SLOT_COUNT   288u")) and "ojPalettePoolStart" in memory)
    expect("OpenJazz palette effects never use the fragmented general heap", "PaletteEffect::operator new" in palette_effects and "dc32OjPaletteEffectAlloc(size)" in palette_effects and "PaletteEffect::operator delete" in palette_effects and "operator new" in palette_header)
    expect("Every OpenJazz palette effect fits its fixed slot", palette_effects.count("static_assert(sizeof(") == 10 and palette_effects.count("<= DC32_OJ_PALETTE_SLOT_SIZE") == 10)
    expect("OpenJazz validates bundled shareware for XIP caching", all(token in cache for token in ("OJ_SHAREWARE_FINGERPRINT", "OJ_SHAREWARE_PACK_SIZE", "bundled shareware only")))
    expect("OpenJazz cache is transactional and table-CRC checked", all(token in cache for token in ("OJ_CACHE_VERSION = 5u", "OJ_CACHE_SLOT_COUNT", "tableCrc", "entry->crc", "ojHeaderValid", "dc32OjCacheCommit", "header.payloadCrc = 0u")))
    expect("OpenJazz validates reusable and newly built menu-font atlases at fixed positions", all(token in cache for token in ("ojFontEntriesValid", "entries + 3u + font", "OJ_CACHE_FONT_MIN_PIXELS = 32u", "OJ_CACHE_BLANK_FONT_CRC", "invalid font atlas", "entry->width != 128u", "entry->reserved < OJ_CACHE_FONT_MIN_PIXELS")) and "ojPayloadAt(entry->offset)" not in cache[cache.find("static bool ojFontEntriesValid"):cache.find("static bool ojEntriesValid")])
    manifest_validation = cache[cache.find("static bool ojEntriesValid"):cache.find("static bool ojHeaderValid")]
    expect("OpenJazz validates the complete manifest in one bounded linear pass", all(token in manifest_validation for token in ("OJ_CACHE_MANIFEST_CRC", "for (uint32_t i = 0; i < count; i++)", "ojCrc32Update(manifest, entry->name", "ojCrc32Update(manifest, type")) and manifest_validation.count("for (") == 1 and "ojFindInTable" not in manifest_validation and "snprintf" not in manifest_validation)
    expect("OpenJazz cache is complete and sealed before gameplay", all(token in cache for token in ("OJ_CACHE_REQUIRED_ENTRIES = 1010u", "OJ_CACHE_MANIFEST_CRC = 0xca091a34u", "dc32OjCacheSeal", "gOjCacheSealed")))
    expect("OpenJazz cache construction enforces manifest order before every transaction", all(token in cache for token in ("ojExpectedEntry", "unexpected cache transaction", "duplicate cache transaction")))
    expect("OpenJazz cache construction exposes exact phase checkpoints", all(token in cache for token in ("dc32OjCacheCheckpoint", "phase entry count mismatch", "checkpoint final entry mismatch")))
    expect("OpenJazz cache checkpoints cover every authoritative asset phase", all(token in (source_root / "src" / "main.cpp").read_text(encoding="utf-8") + (source_root / "src" / "jj1" / "level" / "jj1levelload.cpp").read_text(encoding="utf-8") + (source_root / "src" / "jj1" / "bonuslevel" / "jj1bonuslevel.cpp").read_text(encoding="utf-8") for token in ("10u", "19u", "22u", "25u", "28u", "260u", "492u", "724u", "956u", "1007u", "1008u", "1009u", "1010u")))
    expect("OpenJazz seal reconciles the SRAM table and recovers only a complete final BONUSTILES transaction", all(token in cache for token in ("ojCommittedTableCount", "ojRecoverCompleteFinalEntry", '"BONUSTILES"', "pending %u/%u at entry %u")))
    expect("OpenJazz exposes detailed pending-writer status", all(token in cache for token in ("dc32OjCachePendingStatus", "pending %u/%u at entry %u")))
    expect("OpenJazz cache promotions fail closed", "dc32OjCachePromoteSurfaceChecked" in cache and "cache promotion finalize failed" in cache and "return surface;" not in cache[cache.find("bool dc32OjCachePromoteSurfaceChecked"):cache.find("SDL_Surface *dc32OjCacheStoreSurface")])
    expect("OpenJazz cache streams surfaces into QSPI", all(token in cache for token in ("dc32OjCacheBeginSurface", "dc32OjCacheWriteSurface", "flashWrite", "flashUncachedPtr")))
    expect("OpenJazz cache uses the fixed level arena as its cold-build workspace", all(token in memory for token in ("dc32OjCacheWorkspaceAcquire", "tableAligned", "bufferAligned", "decodeSize", "dc32OjLevelArenaAcquire")) and "dc32OjCacheWorkspaceAcquire" in cache)
    expect("OpenJazz font atlases reuse the fixed flash staging buffer", all(token in cache for token in ("dc32OjCacheCreateStagingSurface", "OJ_CACHE_FLASH_BUFFER_SIZE", "ojFlushBuildBuffer", "dc32OjSdlWrapWritableSurface")) and "dc32OjCacheCreateStagingSurface(aW, aH)" in font_source)
    expect("OpenJazz writable staging surfaces never own their fixed pixels", all(token in sdl for token in ("dc32OjSdlWrapWritableSurface", "surface->ownsFormat = 1u")) and "surface->ownsPixels = 1" not in sdl[sdl.find("SDL_Surface *dc32OjSdlWrapWritableSurface"):sdl.find("bool dc32OjSdlInitReadOnlySurface")])
    expect("OpenJazz releases its 8 KiB decode scratch after cache construction", "static uint8_t *gOjCacheDecodeScratch" in cache and "static uint8_t gOjCacheDecodeScratch[" not in cache and "OJ_CACHE_DECODE_SIZE" in cache)
    expect("OpenJazz cache writes 32 KiB flash batches", "OJ_CACHE_FLASH_BUFFER_SIZE = 0x8000u" in cache and "ojFlushBuildBuffer" in cache)
    expect("OpenJazz cache pre-erases its exact two MiB footprint", "OJ_CACHE_FINAL_SIZE = 0x200000u" in cache and "host->flashWrite(QSPI_ROM_START, OJ_CACHE_FINAL_SIZE, NULL, 0)" in cache)
    expect("OpenJazz reuses a valid schema-v5 cache without erasing it", cache.find("if (selected >= 0)") < cache.find('dc32OjLoadingStage("Erasing XIP asset cache")'))
    seal_validation = cache[cache.find("bool dc32OjCacheSeal"):cache.find("bool dc32OjCacheIsSealed")]
    header_validation = cache[cache.find("static bool ojHeaderValid"):cache.find("static const OjCacheEntry *ojFindCommitted")]
    expect("OpenJazz cache commits its table and header after metadata validation and payload flush", cache.find('dc32OjLoadingStage("Checking cache manifest")') < cache.find('dc32OjLoadingStage("Flushing cache payload")') < cache.find('dc32OjLoadingStage("Committing cache table")') < cache.find("&header, sizeof(header)"))
    expect("OpenJazz fast transactional validation never rereads payload bytes", "Verifying cached graphics" not in cache and "ojPayloadAt(" not in seal_validation and "ojPayloadAt(" not in header_validation and "header.payloadCrc" not in header_validation and "ojHeaderValid(" not in seal_validation)
    expect("OpenJazz cache no longer carries a pending-entry BSS table", "OJ_CACHE_MAX_PENDING" not in cache and "gOjPending" not in cache)
    expect("OpenJazz loading progress owns its strings and throttles redraws", all(token in installer for token in ("gOjLoadingStage[64]", "gOjLoadingAsset[64]", "TICKS_PER_SECOND / 10u", "dc32OjLoadingCacheProgress")))
    expect("OpenJazz offscreen surfaces share a palette", "gOjSharedPalette" in sdl and "ownsPalette" in sdl)
    expect("OpenJazz badge startup skips memory-heavy cutscenes", 'dc32OjLoadingStage("Opening main menu")' in (source_root / "src" / "main.cpp").read_text(encoding="utf-8"))
    expect("OpenJazz badge menus avoid full-screen bitmap sets", 'generic("JAZZ JACKRABBIT"' in main_menu and 'generic("SELECT EPISODE"' in game_menu)
    expect("OpenJazz DC32 build removes unreachable multiplayer server code", "#ifndef DC32_OPENJAZZ" in game_menu and "game = new ServerGame" in game_menu)
    menu_source = (source_root / "src" / "menu" / "menu.cpp").read_text(encoding="utf-8")
    expect("OpenJazz text menus restore their palette and draw an explicit selection", 'video.setPalette(menuPalette);' in menu_source and 'fontmn2->showString(">"' in menu_source and "video.drawRect" in (source_root / "src" / "menu" / "filemenu.cpp").read_text(encoding="utf-8"))
    setup_menu = (source_root / "src" / "menu" / "setupmenu.cpp").read_text(encoding="utf-8")
    expect("OpenJazz menus use A to confirm and B or FN to go back", all(token in menu_source for token in ("menuConfirmReleased", "controls.release(C_JUMP)", "menuBackReleased", "controls.release(C_FIRE)")) and "A SELECT  B BACK" in (source_root / "src" / "menu" / "menu.h").read_text(encoding="utf-8"))
    expect("OpenJazz scrolls long text menus", all(token in menu_source for token in ("visibleOptions = options > 8 ? 8 : options", "firstOption", 'showString("^"', 'showString("v"')))
    expect("OpenJazz exposes all bundled levels without text entry", all(token in game_menu for token in ("diamondus level one", "diamondus level two", "diamondus secret level", "tubelectric level one", "tubelectric level two", "medivo level one", "medivo level two", "guardian level one", "diamondus bonus", "tubelectric bonus", "medivo bonus", 'generic("SELECT LEVEL"')) and "return newGameLevel(M_SINGLE);" in game_menu)
    expect("OpenJazz DC32 setup contains only badge-relevant options", 'setupOptions[3] = {"character colors", "audio", "gameplay"}' in setup_menu and "CHARACTER COLORS" in setup_menu)
    expect("OpenJazz ignores stale configurable controls on DC32", setup_io.count("#ifndef DC32_OPENJAZZ") >= 4 and all(token in setup_io for token in ("(void)key;", "(void)button;", "(void)a;", "(void)h;")))
    expect("OpenJazz in-game menu supports quick pause, save feedback and explicit selection", all(token in level_common for token in ("!menu && controls.release(C_PAUSE)", "menuBackReleased()", "menuConfirmReleased()", '"GAME SAVED"', '"SAVE FAILED"', 'fontmn2->showString(">"')))
    expect("OpenJazz compacts normal and bonus level grids", "timeIndex" in (source_root / "src" / "jj1" / "level" / "jj1level.h").read_text(encoding="utf-8") and "gridTiles" in (source_root / "src" / "jj1" / "bonuslevel" / "jj1bonuslevel.h").read_text(encoding="utf-8"))
    expect("OpenJazz initializes compact level state before loading", "JJ1Level (owner) {" in (source_root / "src" / "jj1" / "level" / "jj1level.cpp").read_text(encoding="utf-8"))
    level_load = (source_root / "src" / "jj1" / "level" / "jj1levelload.cpp").read_text(encoding="utf-8")
    expect("OpenJazz streams PANEL.000 instead of allocating its 46 KB decode", all(token in level_load for token in ('rawPanel = file->loadCachedSurface("PANELRAW", 1, 46272', "pixels = static_cast<const unsigned char*>(rawPanel->pixels)", "Stream the composite decode")))
    expect("OpenJazz promotes panel surfaces as checked cache transactions", level_load.find('dc32OjCachePromoteSurfaceChecked("PANELHUD"') < level_load.find("unsigned char* sorted") and "dc32OjCachePromoteSurfaceChecked" in level_load and "return E_FILE;" in level_load)
    bonus_load = (source_root / "src" / "jj1" / "bonuslevel" / "jj1bonuslevel.cpp").read_text(encoding="utf-8")
    expect("OpenJazz loading diagnostics distinguish decode and flash writes", "DECODE %s" in bonus_load and "WRITE %s %x+%u" in installer and "dc32OjLoadingFlashWrite" in cache)
    expect("OpenJazz shares one bounded bonus sprite parser", bonus_load.count("dc32LoadBonusSprite(file.get()") == 2 and "malformed blank sprite" in bonus_load)
    expect("OpenJazz normalizes legal blank bonus sprites", all(token in bonus_load for token in ("height != 1u", "pixelsLength != 0u", "setCachedPixels(cacheName, output, 1, 1, 0)")))
    expect("OpenJazz names cache-build failures and forces their redraw", "dc32OjCacheBuildFailure" in cache and "dc32OjLoadingFailure" in installer and 'ojDrawLoading(true)' in installer)
    expect("OpenJazz decodes normal sprites through fixed XIP workspace", all(token in level_load for token in ("dc32OjCacheDecodePixels", "loadPixelsInto", "setCachedPixels", "cache-hit path advances both source files")))
    expect("OpenJazz decodes bonus sprites through fixed XIP workspace", all(token in bonus_load for token in ("dc32OjCacheDecodePixels", "loadPixelsInto", "setCachedPixels")))
    expect("OpenJazz streams normal level blocks into final storage", all(token in level_load for token in ("loadRLEStream", "OjGridSink", "OjPathDataSink", "OjPlayerAnimSink", "OjByteSink")))
    expect("OpenJazz validates full player blocks while emitting their 76-byte prefix", "loadRLEPrefixStream(JJ1PANIMS * 2" in level_load and "prefix ? decoded >= emitLength" in file_io)
    expect("OpenJazz rejects invalid player animation references", "data[i] >= ANIMS" in level_load and "!playerAnimSink.valid" in level_load)
    expect("OpenJazz validates and closes the exact level trailer", all(token in level_load for token in ("Reading level trailer", "readExact(trailer", "file->getSize() - 25", "Closing level data", "file.reset()")))
    expect("OpenJazz resumes named loading stages after the title flip", all(token in level_load for token in ("Opening level data", "Creating level players", "Preparing palette effects", "Closing level data", "Effects ready: %u")) and "Entering gameplay" in game_source and "Resetting level memory" in memory and "dc32OjLoadingPause();" in sdl and "gOjLoadingActive = true;" in installer)
    play_level_start = game_source.find("int Game::playLevel")
    dc32_path_start = game_source.find("#ifdef DC32_OPENJAZZ", play_level_start)
    dc32_game_path = game_source[dc32_path_start:game_source.find("#else", dc32_path_start)]
    expect("OpenJazz bypasses planet approaches only on DC32", "Preparing planet approach" not in dc32_game_path and "new JJ1Planet" not in dc32_game_path and "new JJ1Planet" in game_source)
    scene_start = level_common.find("int Level::playScene")
    scene_dc32 = level_common[scene_start:level_common.find("#else", scene_start)]
    expect("OpenJazz bypasses heap-heavy in-level cinematics only on DC32", all(token in scene_dc32 for token in ("#ifdef DC32_OPENJAZZ", "delete paletteEffects", "(void)file", "return E_NONE")) and "new JJ1Scene(file)" not in scene_dc32 and "new JJ1Scene(file)" in level_common)
    expect("OpenJazz has one deterministic level cleanup boundary", all(token in game_source for token in ("goto cleanup", "releaseLevelPlayer()", "dc32OjBeginLevelMemory()", "dc32OjEndLevelMemory()")))
    expect("OpenJazz keeps next-level names outside transient storage", "levelFileStorage[16]" in game_header and "pendingBonusFile[16]" in game_header and "replaceLevelFile" in game_source)
    expect("OpenJazz defers bonus play until the normal level is released", "deferBonusLevel" in game_source and "pendingBonusFile[0]" in game_source and "releaseLevelPlayer" in player_source)
    level_source = (source_root / "src" / "jj1" / "level" / "jj1level.cpp").read_text(encoding="utf-8")
    expect("OpenJazz checks memory at the actual normal gameplay boundary", "int JJ1Level::play ()" in level_source and level_source.find("dc32OjRequireGameplayHeap();", level_source.find("int JJ1Level::play ()")) != -1 and "dc32OjRequireGameplayHeap();" not in level_load)
    expect("OpenJazz reports reserve totals on the loading overlay", all(token in memory for token in ("Free %u; block %u", "dc32OjLoadingAsset(detail)")))
    expect("OpenJazz reports level failures with file and exact stage", "Level load failed" in (source_root / "src" / "jj1" / "level" / "jj1level.cpp").read_text(encoding="utf-8") and "dc32OjLoadingContext()" in game_menu)
    expect("OpenJazz streams bonus grids into packed storage", "BLW * BLH - 1" in bonus_load and "bonusGridSink" in bonus_load)
    expect("OpenJazz validates the full bonus background while cropping its cache", all(token in bonus_load for token in ("SOURCE_WIDTH = 832u", "SOURCE_HEIGHT = 32u", "CACHE_WIDTH = 512u", "CACHE_HEIGHT = 20u", "sink.rowsWritten != CACHE_HEIGHT")))
    expect("OpenJazz bulk-reads planar and masked sprite streams", all(token in file_io for token in ("unsigned char input[256]", "encodedRemaining", "read(input, 1, now)", "input[inputPos++]", "int encodedLength")))
    expect("OpenJazz masked sprites use the original inverse planar mapping", "output[(source % quarter) * 4 + plane] = value;" in file_io)
    expect("OpenJazz supplies exact encoded sprite bounds", "DC32_OJ_DECODE_MASK_CAPACITY, pixelOffset" in level_load and "DC32_OJ_DECODE_MASK_CAPACITY, (int)encodedLength" in bonus_load)
    level_frame = (source_root / "src" / "jj1" / "level" / "jj1levelframe.cpp").read_text(encoding="utf-8")
    expect("OpenJazz composites ammo directly onto the writable canvas", "SDL_BlitSurface(panelAmmo[ammoType], &src, canvas, &dst);" in level_frame and "offsetX + 248" in level_frame)
    expect("OpenJazz never mutates the cached HUD on DC32", "#ifndef DC32_OPENJAZZ" in level_frame and "dst->readOnlyPixels" in sdl)
    expect("OpenJazz pack reads avoid redundant FAT seeks", all(token in (ROOT / "src" / "apps" / "port" / "port_runtime.c").read_text(encoding="utf-8") for token in ("pak->positionValid", "pak->position != offset", "pak->position = offset + got")))
    expect("OpenJazz flash surfaces share immutable formats", all(token in sdl for token in ("gOjReadOnlyFormats", "ojReadOnlyFormat", "ownsFormat")))
    main_source = (source_root / "src" / "main.cpp").read_text(encoding="utf-8")
    expect("OpenJazz cached level fonts skip glyph pixel allocations", all(token in font_source for token in ("cachedAtlas", "file->seek(width * height, false)", "if (!cachedAtlas)")))
    expect("OpenJazz prebuilds level fonts before gameplay fragments SRAM", all(token in main_source for token in ("cachedLevelFont = new Font(false)", "cachedBonusFont = new Font(true)", "while SRAM is still unfragmented")))
    expect("Original shareware ZIP is retained", (shareware_root / "JAZZ.ZIP").is_file() and (shareware_root / "README.md").is_file())
    expect("OpenJazz shareware ZIP is packaged", '"APPS/JAZZ.ZIP"' in sd_zip and "385f685d804b239e2ac070a1c267824b4a6b7898072248646c939a03469d345e" in sd_zip)
    expect("Generated OpenJazz pack is not packaged", '"APPS/openjazz.pak"' not in sd_zip)
    expect("OpenJazz packer includes all numbered worlds and macros", "NUMBERED_EXTENSION" in tool and "MACRO_FILES" in tool)
    expect("OpenJazz packer has interactive no-argument path", "resolve_inputs" in tool and "prompt_path" in tool and "default_output_path" in tool)
    expect("OpenJazz port shows startup phases and assets", "dc32OjLoadingStage" in installer and "dc32OjLoadingAsset" in file_io and "dc32OjLoadingPause" in sdl)
    expect("OpenJazz pack index uses one contiguous table read", "dc32PortReadAssetPack(&gOjPak, OJ_HEADER_SIZE, table, tableSize)" in pack)
    result = run([sys.executable, "tools/build_openjazz_pack.py", str(data.parent), "--output", str(out)])
    expect("OpenJazz packer finds a nested data root", result.returncode == 0 and out.is_file())
    raw = out.read_bytes()
    magic, version, count = struct.unpack_from("<12sII", raw, 0)
    expect("OpenJazz pack magic", magic == b"DC32JAZZPK\0\0")
    expect("OpenJazz pack version", version == 1)
    expect("OpenJazz pack includes complete fixture", count == len(required) + len(included_extra))
    expect("OpenJazz pack includes numbered world extensions", b"LEVEL0.001" in raw and b"BONUS.018" in raw)
    expect("OpenJazz pack excludes unrelated files", b"README.TXT" not in raw)

    shareware_result = run([
        sys.executable,
        "tools/build_openjazz_pack.py",
        str(shareware_root / "JAZZ.ZIP"),
        "--output",
        str(shareware_out),
    ])
    expect("Bundled OpenJazz shareware builds locally", shareware_result.returncode == 0 and shareware_out.is_file())
    _magic, _version, shareware_count = struct.unpack_from("<12sII", shareware_out.read_bytes(), 0)
    expect("Bundled OpenJazz shareware has 57 runtime files", shareware_count == 57)

    duplicate = tmp / "duplicate-jazz"
    for root_name in ("one", "two"):
        root = duplicate / root_name
        root.mkdir(parents=True)
        for name in required:
            (root / name).write_bytes(b"x")
    duplicate_result = run([sys.executable, "tools/build_openjazz_pack.py", str(duplicate), "--output", str(tmp / "duplicate.pak")])
    expect("OpenJazz packer rejects ambiguous data roots", duplicate_result.returncode != 0 and "multiple Jazz data roots" in duplicate_result.stdout)

    bad_zip = tmp / "bad-jazz.zip"
    bad_zip.write_bytes(b"not a zip")
    bad_result = run([sys.executable, "tools/build_openjazz_pack.py", str(bad_zip), "--output", str(tmp / "bad-jazz.pak")])
    expect("OpenJazz packer rejects malformed ZIP input", bad_result.returncode != 0 and "error:" in bad_result.stdout)


def check_cave_packer(tmp: Path) -> None:
    data = tmp / "cave" / "data"
    (data / "Stage").mkdir(parents=True)
    (data / "Npc").mkdir()
    for name in ("MyChar.pbm", "Arms.pbm", "ArmsImage.pbm", "Bullet.pbm", "Caret.pbm", "Face.pbm", "ItemImage.pbm", "npc.tbl"):
        (data / name).write_bytes(b"sample")
    (data / "Stage.dat").write_bytes(b"\x01" + b"Start".ljust(32, b"\0") + b"Start Point".ljust(35, b"\0") + bytes([0, 0, 4, 0, 0, 1]))
    (data / "Stage" / "Start.pxm").write_bytes(b"pxm")
    (data / "Stage" / "Start.tsc").write_bytes(b"tsc")
    (data / "Stage" / "Start.pxe").write_bytes(b"pxe")
    (data / "Npc" / "Npc0.pbm").write_bytes(b"npc")
    out = tmp / "cave.pak"

    result = run([sys.executable, "tools/build_cave_pack.py", "--input", str(tmp / "cave"), "--output", str(out)])
    expect("Cave packer succeeds with local data", result.returncode == 0 and out.is_file())
    raw = out.read_bytes()
    magic, version, count = struct.unpack_from("<12sII", raw, 0)
    expect("Cave pack magic", magic == b"DC32CAVEPAK\0")
    expect("Cave pack version", version == 1)
    expect("Cave pack file count", count >= 11)

    (data / "Stage.dat").unlink()
    exe = tmp / "cave" / "Doukutsu.exe"
    exe_data = bytearray(0x937B0 + 95 * 0xC8)
    for index in range(95):
        pos = 0x937B0 + index * 0xC8
        exe_data[pos : pos + 32] = b"0".ljust(32, b"\0")
        exe_data[pos + 32 : pos + 64] = b"Start".ljust(32, b"\0")
        struct.pack_into("<i", exe_data, pos + 64, 4)
        exe_data[pos + 68 : pos + 100] = b"bk0".ljust(32, b"\0")
        exe_data[pos + 100 : pos + 132] = b"Guest".ljust(32, b"\0")
        exe_data[pos + 132 : pos + 164] = b"0".ljust(32, b"\0")
        exe_data[pos + 164] = 0
        exe_data[pos + 165 : pos + 200] = b"Start Point".ljust(35, b"\0")
    exe.write_bytes(exe_data)
    exe_out = tmp / "cave-from-exe.pak"
    exe_result = run([sys.executable, "tools/build_cave_pack.py", "--input", str(tmp / "cave"), "--output", str(exe_out)])
    expect("Cave packer extracts Stage.dat from Doukutsu.exe", exe_result.returncode == 0 and exe_out.is_file())
    expect("Cave packer reports executable stage source", "Doukutsu.exe" in exe_result.stdout)

    missing = run([sys.executable, "tools/build_cave_pack.py", "--input", str(tmp / "missing"), "--output", str(tmp / "bad.pak")])
    expect("Cave packer fails cleanly without data", missing.returncode != 0 and "error:" in missing.stdout)


def check_chips_packer(tmp: Path) -> None:
    chips = tmp / "CHIPS.DAT"
    upper = bytes([0x6C, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x03, 0x00])
    lower = bytes([0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x04, 0x00])
    meta = bytes([3, 7]) + b"Fixture"
    level = struct.pack("<HHHHH", 1, 0, 0, 0, len(upper)) + upper + struct.pack("<H", len(lower)) + lower + struct.pack("<H", len(meta)) + meta
    chips.write_bytes(struct.pack("<IH", 0x0002AAAC, 1) + struct.pack("<H", len(level)) + level)
    tiles = tmp / "tiles.bin"
    tile_count = 128
    tile_size = 24
    alpha_bytes = (tile_size * tile_size + 7) // 8
    tiles.write_bytes(
        struct.pack("<11sBHHHH", b"DC32CHIPTIL", 0, 1, tile_count, tile_size, alpha_bytes) +
        bytes(tile_count * tile_size * tile_size) +
        bytes([0xff]) * (tile_count * alpha_bytes)
    )
    out = tmp / "chips.pak"

    result = run([
        sys.executable,
        "tools/build_chips_pack.py",
        "--chips-dat",
        str(chips),
        "--tiles",
        str(tiles),
        "--output",
        str(out),
    ])
    expect("Chip packer succeeds with local data", result.returncode == 0 and out.is_file())
    raw = out.read_bytes()
    magic, _pad, version, count = struct.unpack_from("<11sBII", raw, 0)
    expect("Chip pack magic", magic == b"DC32CHIPSPK")
    expect("Chip pack version", version == 1)
    expect("Chip pack payload count", count == 2)
    expect("Chip pack embeds validated tile graphics", b"DC32CHIPTIL" in raw)

    bad = tmp / "bad.dat"
    bad.write_bytes(b"bad")
    missing = run([sys.executable, "tools/build_chips_pack.py", "--chips-dat", str(bad), "--output", str(tmp / "bad.pak")])
    expect("Chip packer fails cleanly with invalid data", missing.returncode != 0 and "error:" in missing.stdout)
    bad_tiles = tmp / "bad.tiles"
    bad_tiles.write_bytes(b"not a tile pack")
    invalid_tiles = run([
        sys.executable,
        "tools/build_chips_pack.py",
        "--chips-dat",
        str(chips),
        "--tiles",
        str(bad_tiles),
        "--output",
        str(tmp / "bad-tiles.pak"),
    ])
    expect("Chip packer fails cleanly with invalid tile graphics", invalid_tiles.returncode != 0 and "error:" in invalid_tiles.stdout)


def check_period_asset_packer(tmp: Path) -> None:
    cave_tool = (ROOT / "tools" / "build_cave_pack.py").read_text(encoding="utf-8")
    chips_tool = (ROOT / "tools" / "build_chips_pack.py").read_text(encoding="utf-8")
    period_tool = (ROOT / "tools" / "build_period_assets.py").read_text(encoding="utf-8")
    tworld_tool = (ROOT / "tools" / "build_tworld_assets.py").read_text(encoding="utf-8")
    xscorch_tool = (ROOT / "tools" / "build_xscorch_assets.py").read_text(encoding="utf-8")
    xsokoban_tool = (ROOT / "tools" / "build_xsokoban_assets.py").read_text(encoding="utf-8")
    root = tmp / "sources" / "PipeDreamer"
    (root / "Images").mkdir(parents=True)
    (root / "Source").mkdir()
    (root / "Images" / "pipe.png").write_bytes(b"png")
    (root / "Source" / "notes.txt").write_text("source note\n", encoding="utf-8")
    out_dir = tmp / "period-packs"

    result = run([
        sys.executable,
        "tools/build_period_assets.py",
        "--source-root",
        str(tmp / "sources"),
        "--output-dir",
        str(out_dir),
        "--only",
        "pipedreamer",
    ])
    expect("Period asset packer succeeds with local source tree", result.returncode == 0)
    expect("PipeDreamer pack is written", (out_dir / "pipe-pipedreamer.pak").is_file())
    expect("Period asset manifest is written", (out_dir / "period-assets-manifest.json").is_file())
    expect("User asset packers have interactive no-argument paths", all("resolve_inputs" in text and "prompt_path" in text for text in (cave_tool, chips_tool, period_tool)))
    expect("Generated asset scripts have no required terminal arguments", all("required=True" not in text and "resolve_inputs" in text for text in (tworld_tool, xscorch_tool, xsokoban_tool)))
    expect("Chip packer supports flexible tile image import", all(token in chips_tool for token in ("PIL", "guess_grid", "build_tile_pack_from_image", "DC32CHIPTIL")))
    expect("Chip packer supports Win 3.1 executable tile extraction", all(token in chips_tool for token in ("CHIPS.EXE", "read_ne_bitmap_resource", "WIN_EXE_TILE_RESOURCE_ID")))


def main() -> int:
    check_sources_removed()
    check_port_runtime_scaffold()
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        check_tworld_source_and_generator(tmp_path)
        check_xscorch_source_and_generator(tmp_path)
        check_pipedreamer_source()
        check_cave_source_and_loader()
        check_openjazz_source_and_packer(tmp_path)
        check_xsokoban_source_and_generator(tmp_path)
        check_cave_packer(tmp_path)
        check_chips_packer(tmp_path)
        check_period_asset_packer(tmp_path)
    print("Period port acceptance tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
