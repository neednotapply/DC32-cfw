#!/usr/bin/env python3
"""Build the SD-card asset zip for DC32-cfw releases."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path


IR_REPO = "https://github.com/flipperdevices/flipperzero-firmware.git"
IR_BRANCH = "dev"
IR_ASSET_PATH = Path("applications/main/infrared/resources/infrared/assets")
IR_FILES = ("ac.ir", "audio.ir", "projector.ir", "tv.ir")

BADUSB_REPO = "https://github.com/UberGuidoZ/Flipper.git"
BADUSB_BRANCH = "main"
BADUSB_PATH = Path("BadUSB")

MUSIC_REPO = "https://github.com/neverfa11ing/FlipperMusicRTTTL.git"
MUSIC_BRANCH = "main"
MUSIC_DIRS = ("ArcadeTones", "RTTTL_generics", "Software", "Theme_Songs")
MUSIC_ARCHIVE = "Unsorted 10k Song Archive.zip"
MUSIC_ARCHIVE_DIR = "Unsorted 10k Song Archive"

ROM_DIRS = {
    "GB": "Game Boy",
    "GBC": "Game Boy Color",
    "NES": "Nintendo Entertainment System",
}

SKIP_DIRS = {".git", ".github", "__pycache__"}
SKIP_SUFFIXES = {".pyc", ".tmp"}


def run_git(args: list[str], cwd: Path | None = None) -> str:
    cmd = ["git", *args]
    proc = subprocess.run(
        cmd,
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if proc.returncode:
        where = f" in {cwd}" if cwd else ""
        raise RuntimeError(f"{' '.join(cmd)} failed{where}:\n{proc.stdout}")
    return proc.stdout.strip()


def clone_repo(url: str, branch: str, dest: Path, sparse_paths: list[str] | None = None) -> str:
    if dest.exists():
        remove_tree(dest)

    clone_args = [
        "clone",
        "--depth",
        "1",
        "--branch",
        branch,
        "--filter=blob:none",
    ]
    if sparse_paths:
        clone_args.append("--sparse")
    clone_args.extend([url, str(dest)])
    run_git(clone_args)

    if sparse_paths:
        run_git(["sparse-checkout", "set", *sparse_paths], cwd=dest)

    return run_git(["rev-parse", "HEAD"], cwd=dest)


def ensure_clean_dir(path: Path) -> None:
    if path.exists():
        remove_tree(path)
    path.mkdir(parents=True)


def remove_tree(path: Path) -> None:
    def make_writable(func, target, _exc_info):
        os.chmod(target, 0o700)
        func(target)

    shutil.rmtree(path, onerror=make_writable)


def should_skip(path: Path) -> bool:
    if any(part in SKIP_DIRS for part in path.parts):
        return True
    if path.name == ".gitmodules":
        return True
    if path.name == MUSIC_ARCHIVE:
        return True
    if path.suffix.lower() in SKIP_SUFFIXES:
        return True
    return False


def copy_tree(src: Path, dst: Path) -> None:
    if not src.exists():
        raise FileNotFoundError(f"Missing required source path: {src}")

    for item in src.rglob("*"):
        rel = item.relative_to(src)
        if should_skip(rel):
            continue
        target = dst / rel
        if item.is_dir():
            target.mkdir(parents=True, exist_ok=True)
        elif item.is_file():
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(item, target)


def copy_ir_assets(repo: Path, stage: Path) -> None:
    src = repo / IR_ASSET_PATH
    dst = stage / "IR"
    dst.mkdir(parents=True)
    for name in IR_FILES:
        asset = src / name
        if not asset.is_file():
            raise FileNotFoundError(f"Missing required IR asset: {asset}")
        shutil.copyfile(asset, dst / name)


def copy_badusb_assets(repo: Path, stage: Path) -> None:
    run_git(["submodule", "update", "--init", "--recursive", "--", BADUSB_PATH.as_posix()], cwd=repo)
    copy_tree(repo / BADUSB_PATH, stage / "BADUSB")


def copy_music_assets(repo: Path, stage: Path) -> None:
    dst = stage / "MUSIC"
    dst.mkdir(parents=True)
    for name in MUSIC_DIRS:
        copy_tree(repo / name, dst / name)

    archive = repo / MUSIC_ARCHIVE
    if not archive.is_file():
        raise FileNotFoundError(f"Missing required music archive: {archive}")

    extract_dir = dst / MUSIC_ARCHIVE_DIR
    extract_dir.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive) as zf:
        extract_zip_safely(zf, extract_dir)

    nested_archive = dst / MUSIC_ARCHIVE
    if nested_archive.exists():
        nested_archive.unlink()


def create_rom_dirs(stage: Path) -> None:
    rom_messages = {
        "GB": "Place your Game Boy roms in this folder.\n"
              "Files should be .gb format.\n"
              "Download GB roms here here: https://tinyurl.com/NoIntro-GB \n",

        "GBC": "Place your Game Boy Color roms in this folder.\n"
               "Files should be .gbc format.\n"
               "Download GBC roms here: https://tinyurl.com/NoIntro-GBC \n",

        "NES": "Place your Nintendo Entertainment System ROM files in this folder.\n"
               "Files should be .nes format.\n"
               "Download NES roms here: https://tinyurl.com/NoIntro-NES \n",
    }

    for name in ROM_DIRS:
        rom_dir = stage / "ROMS" / name
        rom_dir.mkdir(parents=True, exist_ok=True)

        (rom_dir / "README.txt").write_text(
            rom_messages[name],
            encoding="utf-8",
            newline="\n",
        )


def extract_zip_safely(zf: zipfile.ZipFile, dest: Path) -> None:
    dest = dest.resolve()
    for member in zf.infolist():
        target = (dest / member.filename).resolve()
        if dest != target and dest not in target.parents:
            raise RuntimeError(f"Refusing to extract zip entry outside target directory: {member.filename}")
        zf.extract(member, dest)


def write_sources(stage: Path, ir_sha: str, badusb_sha: str, music_sha: str) -> None:
    text = f"""# SD.zip Sources

This release asset was assembled from upstream repositories at build time.
The external assets are distributed with credit and licensing retained by
their upstream projects.

## IR

- Repository: {IR_REPO}
- Branch: {IR_BRANCH}
- Commit: {ir_sha}
- Source path: {IR_ASSET_PATH.as_posix()}
- SD path: IR/
- Files: {', '.join(IR_FILES)}

## BADUSB

- Repository: {BADUSB_REPO}
- Branch: {BADUSB_BRANCH}
- Commit: {badusb_sha}
- Source path: {BADUSB_PATH.as_posix()}/
- SD path: BADUSB/
- Notes: BadUSB submodules under the source path were initialized recursively.

## MUSIC

- Repository: {MUSIC_REPO}
- Branch: {MUSIC_BRANCH}
- Commit: {music_sha}
- Source paths: {', '.join(MUSIC_DIRS)}, {MUSIC_ARCHIVE}
- SD path: MUSIC/
- Notes: {MUSIC_ARCHIVE} was extracted into MUSIC/{MUSIC_ARCHIVE_DIR}/ and the nested zip was omitted.

## ROMS

- SD paths: {', '.join(f'ROMS/{name}/' for name in ROM_DIRS)}
- Notes: These folders contain README.txt placeholders. Add only ROM files you can lawfully use and redistribute.
"""
    (stage / "SOURCES.md").write_text(text, encoding="utf-8", newline="\n")


def build_zip(stage: Path, output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    if output.exists():
        output.unlink()

    dirs = sorted(path for path in stage.rglob("*") if path.is_dir() and not should_skip(path.relative_to(stage)))
    files = sorted(path for path in stage.rglob("*") if path.is_file() and not should_skip(path.relative_to(stage)))
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for path in dirs:
            zf.write(path, path.relative_to(stage).as_posix() + "/")
        for path in files:
            zf.write(path, path.relative_to(stage).as_posix())


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build SD.zip from upstream asset repositories.")
    parser.add_argument("--output", required=True, type=Path, help="Path to write SD.zip")
    parser.add_argument("--work-dir", required=True, type=Path, help="Temporary work directory")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    work_dir = args.work_dir.resolve()
    output = args.output.resolve()
    repos = work_dir / "repos"
    stage = work_dir / "stage"

    ensure_clean_dir(repos)
    ensure_clean_dir(stage)

    ir_repo = repos / "flipperzero-firmware"
    badusb_repo = repos / "UberGuidoZ-Flipper"
    music_repo = repos / "FlipperMusicRTTTL"

    ir_sha = clone_repo(IR_REPO, IR_BRANCH, ir_repo, [IR_ASSET_PATH.as_posix()])
    badusb_sha = clone_repo(BADUSB_REPO, BADUSB_BRANCH, badusb_repo, [BADUSB_PATH.as_posix()])
    music_sha = clone_repo(MUSIC_REPO, MUSIC_BRANCH, music_repo)

    copy_ir_assets(ir_repo, stage)
    copy_badusb_assets(badusb_repo, stage)
    copy_music_assets(music_repo, stage)
    create_rom_dirs(stage)
    write_sources(stage, ir_sha, badusb_sha, music_sha)
    build_zip(stage, output)

    print(f"Wrote {output}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
