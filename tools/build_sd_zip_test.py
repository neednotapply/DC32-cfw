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
        builder.APP_DATA_FILES["APPS/doom1.whx"] = fake_doom_whx
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
        expect("All app binaries and app data are copied to /APPS", app_files == sorted((*builder.APP_BINARIES, "doom1.whx")))
        expect("DOOM WHX is copied to /APPS", (stage / "APPS" / "doom1.whx").is_file())
        expect("DOOM data is not copied to /ROMS/DOOM", not (stage / "ROMS" / "DOOM").exists())
        expect("App hashes are recorded", sorted(app_hashes) == expected_hash_files)
        manifest = builder.app_source_manifest(app_hashes)
        expect("App hashes appear in app manifest", manifest["sources"]["apps"]["files"] == app_hashes)
        expect("DOOM app source metadata is present", manifest["sources"]["doom"]["sd_path"] == "APPS/")
        builder.write_app_sources(stage, app_hashes)
        apps_zip = tmp_path / "SD-apps.zip"
        builder.build_zip(stage, apps_zip)
        with zipfile.ZipFile(apps_zip) as zf:
            names = set(zf.namelist())
            sources_md = zf.read("SOURCES.md").decode("utf-8")
        expect("SD-apps.zip contains SOURCES.md", "SOURCES.md" in names)
        expect("SD-apps.zip contains /APPS binaries", all(f"APPS/{name}" in names for name in builder.APP_BINARIES))
        expect("SD-apps.zip contains DOOM WHX", "APPS/doom1.whx" in names)
        expect("SD-apps.zip excludes /ROMS/DOOM", not any(name.startswith("ROMS/DOOM/") for name in names))
        expect("SD-apps.zip SOURCES records hashes", all(name in sources_md and app_hashes[name] in sources_md for name in expected_hash_files))

    print("SD assets/apps builder tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
