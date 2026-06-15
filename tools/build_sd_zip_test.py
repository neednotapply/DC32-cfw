#!/usr/bin/env python3
"""Regression checks for the SD assets/apps zip builder."""

from __future__ import annotations

import importlib.util
import tempfile
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILDER_SRC = ROOT / "tools" / "build_sd_zip.py"


def load_builder():
    spec = importlib.util.spec_from_file_location("build_sd_zip", BUILDER_SRC)
    if not spec or not spec.loader:
        raise RuntimeError(f"Cannot load {BUILDER_SRC}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def expect(name: str, condition: bool) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def write_hex(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(":00000001FF\n", encoding="ascii", newline="\n")


def main() -> int:
    builder = load_builder()

    expect("Arduboy ROM path is /ROMS/AB", "AB" in builder.ROM_DIRS and "ARDUBOY" not in builder.ROM_DIRS)
    expect("Arduboy placeholder README is not generated", "AB" not in builder.ROM_PLACEHOLDER_DIRS)

    manifest = builder.source_manifest("ir-sha", "badusb-sha", "music-sha", "arduboy-sha")
    sources = manifest["sources"]
    expect("IR source uses Momentum Firmware", sources["ir"]["repository"] == "https://github.com/Next-Flip/Momentum-Firmware.git")
    expect("IR source copies all .ir files", sources["ir"]["patterns"] == ["*.ir"])
    expect("Arduboy source metadata is present", "arduboy" in sources)
    expect("Arduboy commit is recorded", sources["arduboy"]["commit"] == "arduboy-sha")
    expect("Arduboy SD path is recorded", sources["arduboy"]["sd_path"] == "ROMS/AB")
    expect("Arduboy genre paths are recorded", sources["arduboy"]["paths"] == list(builder.ARDUBOY_GENRE_DIRS))
    expect("App metadata is not mixed into assets", "apps" not in sources)

    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        repo = tmp_path / "repo"
        stage = tmp_path / "stage"
        apps = tmp_path / "apps"
        ir_src = repo / builder.IR_ASSET_PATH
        ir_src.mkdir(parents=True)
        apps.mkdir()
        (ir_src / "tv.ir").write_text("Filetype: IR signals file\n", encoding="ascii")
        (ir_src / "fans.ir").write_text("Filetype: IR signals file\n", encoding="ascii")
        (ir_src / "notes.txt").write_text("not an IR asset\n", encoding="ascii")
        for name in builder.APP_BINARIES:
            (apps / name).write_bytes(f"{name}\n".encode("ascii"))
        fake_doom_whx = tmp_path / "doom1.whx"
        fake_doom_whx.write_bytes(b"IWHX-test")
        fake_chips_tworld_pak = tmp_path / "chips-tworld.pak"
        fake_chips_tworld_pak.write_bytes(b"DC32TWORLD-test")
        fake_pipedreamer_pak = tmp_path / "pipe-pipedreamer.pak"
        fake_pipedreamer_pak.write_bytes(b"DC32PIPEPK-test")
        fake_xscorch_pak = tmp_path / "scorch-xscorch.pak"
        fake_xscorch_pak.write_bytes(b"DC32SCORCH-test")
        fake_sokoban_pak = tmp_path / "sokoban-xsokoban.pak"
        fake_sokoban_pak.write_bytes(b"DC32XSOKO-test")
        builder.APP_DATA_FILES["APPS/doom1.whx"] = fake_doom_whx
        builder.APP_DATA_FILES["APPS/chips-tworld.pak"] = fake_chips_tworld_pak
        builder.APP_DATA_FILES["APPS/pipe-pipedreamer.pak"] = fake_pipedreamer_pak
        builder.APP_DATA_FILES["APPS/scorch-xscorch.pak"] = fake_xscorch_pak
        builder.APP_DATA_FILES["APPS/sokoban-xsokoban.pak"] = fake_sokoban_pak
        for genre in builder.ARDUBOY_GENRE_DIRS:
            (repo / genre).mkdir(parents=True)

        builder.copy_ir_assets(repo, stage)
        ir_files = sorted(path.name for path in (stage / "IR").iterdir() if path.is_file())
        expect("All .ir assets are copied", ir_files == ["fans.ir", "tv.ir"])
        expect("Non-.ir IR sidecars are omitted", not (stage / "IR" / "notes.txt").exists())

        write_hex(repo / "Action" / "Normal Game" / "Normal.hex")
        (repo / "Action" / "Normal Game" / "Normal.png").write_text("png sidecar", encoding="ascii")
        write_hex(repo / "Action" / "Alpha" / "game.hex")
        write_hex(repo / "Action" / "Beta" / "GAME.HEX")
        write_hex(repo / "Puzzle" / "RootPuzzle.hex")

        builder.copy_arduboy_assets(repo, stage)
        arduboy_root = stage / "ROMS" / "AB"
        files = sorted(path.relative_to(arduboy_root).as_posix() for path in arduboy_root.rglob("*") if path.is_file())

        expect("All Arduboy genre folders are created", all((arduboy_root / genre).is_dir() for genre in builder.ARDUBOY_GENRE_DIRS))
        expect("Non-conflicting hex files are flattened", "Action/Normal.hex" in files)
        expect("Nested game folders are not copied", "Action/Normal Game/Normal.hex" not in files)
        expect("Sidecar files are omitted", "Action/Normal.png" not in files)
        expect("First duplicate flattened name keeps its target", "Action/game.hex" in files)
        expect("Only duplicate conflicts are renamed with game folder prefix", "Action/Beta - GAME.HEX" in files and "Action/Alpha - game.hex" not in files)
        expect("Only hex files are copied under /ROMS/AB", all(path.lower().endswith(".hex") for path in files))
        expect("No Arduboy placeholder README is copied", "README.txt" not in files)

        app_hashes = builder.copy_app_binaries(apps, stage)
        app_files = sorted(path.name for path in (stage / "APPS").iterdir() if path.is_file())
        expected_hash_files = sorted((*builder.APP_BINARIES, *builder.APP_DATA_FILES))
        expected_app_data = tuple(Path(name).name for name in builder.APP_DATA_FILES)
        expect(
            "All app binaries, app data, and period README are copied to /APPS",
            app_files == sorted((*builder.APP_BINARIES, *expected_app_data, "README-period-ports.txt")),
        )
        expect("DOOM WHX is copied to /APPS", (stage / "APPS" / "doom1.whx").is_file())
        expect("Tile World pack is copied to /APPS", (stage / "APPS" / "chips-tworld.pak").is_file())
        expect("xscorch pack is copied to /APPS", (stage / "APPS" / "scorch-xscorch.pak").is_file())
        expect("PipeDreamer pack is copied to /APPS", (stage / "APPS" / "pipe-pipedreamer.pak").is_file())
        expect("XSokoban pack is copied to /APPS", (stage / "APPS" / "sokoban-xsokoban.pak").is_file())
        expect("Period port README is copied to /APPS", (stage / "APPS" / "README-period-ports.txt").is_file())
        expect("DOOM data is not copied to /ROMS/DOOM", not (stage / "ROMS" / "DOOM").exists())
        expect("App hashes are recorded", sorted(app_hashes) == expected_hash_files)
        manifest = builder.app_source_manifest(app_hashes)
        expect("App hashes appear in app manifest", manifest["sources"]["apps"]["files"] == app_hashes)
        expect("DOOM app source metadata is present", manifest["sources"]["doom"]["sd_path"] == "APPS/")
        expect("Period port metadata is present", manifest["sources"]["period_ports"]["sd_path"] == "APPS/")
        expect("Chip's Challenge accepted ID is recorded", manifest["sources"]["period_ports"]["accepted_ids"]["chips"] == 207)
        expect("Scorched Earth accepted ID is recorded", manifest["sources"]["period_ports"]["accepted_ids"]["scorch"] == 208)
        expect("Pipe Dream accepted ID is recorded", manifest["sources"]["period_ports"]["accepted_ids"]["pipe"] == 209)
        expect("Cave Story accepted ID is recorded", manifest["sources"]["period_ports"]["accepted_ids"]["cave"] == 210)
        expect("Sokoban accepted ID is recorded", manifest["sources"]["period_ports"]["accepted_ids"]["sokoban"] == 211)
        builder.write_app_sources(stage, app_hashes)
        apps_zip = tmp_path / "SD-apps.zip"
        builder.build_zip(stage, apps_zip)
        with zipfile.ZipFile(apps_zip) as zf:
            names = set(zf.namelist())
            sources_md = zf.read("SOURCES.md").decode("utf-8")
        expect("SD-apps.zip contains SOURCES.md", "SOURCES.md" in names)
        expect("SD-apps.zip contains /APPS binaries", all(f"APPS/{name}" in names for name in builder.APP_BINARIES))
        expect("SD-apps.zip contains DOOM WHX", "APPS/doom1.whx" in names)
        expect("SD-apps.zip contains Tile World pack", "APPS/chips-tworld.pak" in names)
        expect("SD-apps.zip contains xscorch pack", "APPS/scorch-xscorch.pak" in names)
        expect("SD-apps.zip contains PipeDreamer pack", "APPS/pipe-pipedreamer.pak" in names)
        expect("SD-apps.zip contains XSokoban pack", "APPS/sokoban-xsokoban.pak" in names)
        expect("SD-apps.zip contains period port README", "APPS/README-period-ports.txt" in names)
        expect("SD-apps.zip omits proprietary Cave data", "APPS/cave.pak" not in names and "APPS/cave.dat" not in names)
        expect("SD-apps.zip excludes /ROMS/DOOM", not any(name.startswith("ROMS/DOOM/") for name in names))
        expect("SD-apps.zip SOURCES records hashes", all(name in sources_md and app_hashes[name] in sources_md for name in expected_hash_files))

    print("SD assets/apps builder tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
