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
        check_xsokoban_source_and_generator(tmp_path)
        check_cave_packer(tmp_path)
        check_chips_packer(tmp_path)
        check_period_asset_packer(tmp_path)
    print("Period port acceptance tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
