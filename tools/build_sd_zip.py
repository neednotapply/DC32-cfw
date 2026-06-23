#!/usr/bin/env python3
"""Build the SD-card assets/apps zips for DC32-cfw releases."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
IR_REPO = "https://github.com/Next-Flip/Momentum-Firmware.git"
IR_BRANCH = "dev"
IR_ASSET_PATH = Path("applications/main/infrared/resources/infrared/assets")

BADUSB_REPO = "https://github.com/UberGuidoZ/Flipper.git"
BADUSB_BRANCH = "main"
BADUSB_PATH = Path("BadUSB")

MUSIC_REPO = "https://github.com/neverfa11ing/FlipperMusicRTTTL.git"
MUSIC_BRANCH = "main"
MUSIC_DIRS = ("ArcadeTones", "RTTTL_generics", "Software", "Theme_Songs")
MUSIC_ARCHIVE = "Unsorted 10k Song Archive.zip"
MUSIC_ARCHIVE_DIR = "Unsorted 10k Song Archive"
MUSIC_BUCKET_FILE_LIMIT = 64
MUSIC_BUCKET_ORDER = ("#", "0-9", *tuple(chr(ch) for ch in range(ord("A"), ord("Z") + 1)))

ARDUBOY_REPO = "https://github.com/eried/ArduboyCollection.git"
ARDUBOY_BRANCH = "master"
ARDUBOY_GENRE_DIRS = (
    "Action",
    "Application",
    "Arcade",
    "Demo",
    "Misc",
    "Platformer",
    "Puzzle",
    "RPG",
    "Racing",
    "Shooter",
    "Sports",
)

DOOM_REPO = "https://github.com/kilograham/rp2040-doom.git"
DOOM_RELEASE = "defcon32_v1"
DOOM_WHX_SOURCE = REPO_ROOT / "third_party" / "rp2040-doom" / "doom1.whx"
NULLPOMINO_REPO = "https://github.com/nullpomino/nullpomino"
NULLPOMINO_COMMIT = "4de098dd0b48d991247313d8dba30b9721e6f9d9"
NULLPOMINO_LICENSE = REPO_ROOT / "third_party" / "nullpomino" / "LICENSE"
NULLPOMINO_ENGINE_LICENSE = REPO_ROOT / "third_party" / "nullpomino" / "LICENSE-NULLNONAME"
FLAPPY_REPO = "https://github.com/VadimBoev/FlappyBird"
FLAPPY_COMMIT = "3b3060cdf6b38b819e5a649bc92d11776decd0b4"
FLAPPY_TAG = "v1.9.2"
FLAPPY_ARCHIVE_SHA256 = "e06e8553e747be5851edfe90486dda3afcb5b2a9ddac4827f4a7911a6fb39d51"
OPENJAZZ_SHAREWARE_SOURCE = REPO_ROOT / "third_party" / "openjazz-shareware" / "JAZZ.ZIP"
OPENJAZZ_SHAREWARE_SHA256 = "385f685d804b239e2ac070a1c267824b4a6b7898072248646c939a03469d345e"
CHIPS_TWORLD_PACK_SOURCE = REPO_ROOT / "build" / "apps" / "chips-tworld.pak"
PIPEDREAMER_PACK_SOURCE = REPO_ROOT / "build" / "apps" / "pipe-pipedreamer.pak"
XSCORCH_PACK_SOURCE = REPO_ROOT / "build" / "apps" / "scorch-xscorch.pak"
XSOKOBAN_PACK_SOURCE = REPO_ROOT / "build" / "apps" / "sokoban-xsokoban.pak"

ROM_DIRS = {
    "AB": "Arduboy",
    "GB": "Game Boy",
    "GBC": "Game Boy Color",
    "NES": "Nintendo Entertainment System",
}
ROM_PLACEHOLDER_DIRS = {
    "GB": "Game Boy",
    "GBC": "Game Boy Color",
    "NES": "Nintendo Entertainment System",
}
APP_BINARIES = (
    "gb.DC32",
    "nes.DC32",
    "arduboy.DC32",
    "ir.DC32",
    "image.DC32",
    "music.DC32",
    "badusb.DC32",
    "autoclicker.DC32",
    "gamepad.DC32",
    "pong.DC32",
    "tetris.DC32",
    "arkanoid.DC32",
    "flappy.DC32",
    "labyrinth.DC32",
    "trex.DC32",
    "doom.DC32",
    "chips.DC32",
    "scorch.DC32",
    "pipe.DC32",
    "cave.DC32",
    "sokoban.DC32",
    "openjazz.DC32",
    "starfield.DC32",
    "spiro.DC32",
    "cube.DC32",
)
APP_DATA_FILES = {
    "APPS/doom1.whx": DOOM_WHX_SOURCE,
    "APPS/JAZZ.ZIP": OPENJAZZ_SHAREWARE_SOURCE,
    "APPS/chips-tworld.pak": CHIPS_TWORLD_PACK_SOURCE,
    "APPS/pipe-pipedreamer.pak": PIPEDREAMER_PACK_SOURCE,
    "APPS/scorch-xscorch.pak": XSCORCH_PACK_SOURCE,
    "APPS/sokoban-xsokoban.pak": XSOKOBAN_PACK_SOURCE,
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


def resolve_remote_branch(url: str, branch: str) -> str:
    ref = f"refs/heads/{branch}"
    output = run_git(["ls-remote", url, ref])
    if not output:
        raise RuntimeError(f"Could not resolve {url} {ref}")
    return output.split()[0]


def builder_hash() -> str:
    return hashlib.sha256(Path(__file__).read_bytes()).hexdigest()


def source_manifest(ir_sha: str, badusb_sha: str, music_sha: str, arduboy_sha: str) -> dict[str, object]:
    return {
        "schema": 1,
        "builder_sha256": builder_hash(),
        "sources": {
            "ir": {
                "repository": IR_REPO,
                "branch": IR_BRANCH,
                "commit": ir_sha,
                "paths": [IR_ASSET_PATH.as_posix()],
                "patterns": ["*.ir"],
            },
            "badusb": {
                "repository": BADUSB_REPO,
                "branch": BADUSB_BRANCH,
                "commit": badusb_sha,
                "paths": [BADUSB_PATH.as_posix()],
            },
            "music": {
                "repository": MUSIC_REPO,
                "branch": MUSIC_BRANCH,
                "commit": music_sha,
                "paths": [*MUSIC_DIRS, MUSIC_ARCHIVE],
            },
            "arduboy": {
                "repository": ARDUBOY_REPO,
                "branch": ARDUBOY_BRANCH,
                "commit": arduboy_sha,
                "paths": list(ARDUBOY_GENRE_DIRS),
                "sd_path": "ROMS/AB",
                "notes": "Only .hex files are copied. Game folders are flattened into their genre folders.",
            },
            "roms": {
                "directories": ROM_DIRS,
            },
            "images": {
                "paths": ["IMAGES/image_converter.py"],
            },
        },
    }


def app_source_manifest(app_hashes: dict[str, str]) -> dict[str, object]:
    return {
        "schema": 1,
        "builder_sha256": builder_hash(),
        "sources": {
            "apps": {
                "sd_path": "APPS/",
                "files": app_hashes,
                "notes": "Built .DC32 binaries from this repository plus app data payloads copied to their runtime SD paths.",
            },
            "doom": {
                "repository": DOOM_REPO,
                "release": DOOM_RELEASE,
                "paths": ["doom1.whx"],
                "sd_path": "APPS/",
            },
            "tetris": {
                "repository": NULLPOMINO_REPO,
                "commit": NULLPOMINO_COMMIT,
                "licenses": [
                    "third_party/nullpomino/LICENSE",
                    "third_party/nullpomino/LICENSE-NULLNONAME",
                ],
                "sd_path": "APPS/tetris.DC32",
                "notes": "Native C adaptation of the upstream piece, rule, randomizer, timing, and scoring behavior.",
            },
            "flappy": {
                "repository": FLAPPY_REPO,
                "tag": FLAPPY_TAG,
                "commit": FLAPPY_COMMIT,
                "archive_sha256": FLAPPY_ARCHIVE_SHA256,
                "sd_path": "APPS/flappy.DC32",
                "notes": "Native silent DC32 adaptation of the upstream C/OpenGL ES game flow using generated pre-scaled RGB332 sprites from the pinned PNG assets.",
            },
            "openjazz_shareware": {
                "publisher": "Epic MegaGames",
                "archive": "JAZZ.ZIP",
                "sha256": OPENJAZZ_SHAREWARE_SHA256,
                "sd_path": "APPS/",
                "notes": "Original unmodified Jazz Jackrabbit shareware archive; converted to openjazz.pak on first launch, then fully decoded and sealed read-only in the shared 3 MiB ROM staging window before the menu.",
            },
            "period_ports": {
                "reserved_ids": {},
                "accepted_ids": {
                    "chips": 207,
                    "scorch": 208,
                    "pipe": 209,
                    "cave": 210,
                    "sokoban": 211,
                    "openjazz": 212,
                },
                "sd_path": "APPS/",
                "notes": "Chip's Challenge is a Tile World-derived port, Scorched Earth is an xscorch-derived port, Pipe Dream is a PipeDreamer-derived port, Cave Story is an NXEngine/doukutsu data-compatible loader port that requires user-provided Cave data, Sokoban is an XSokoban-derived port, and Jazz Jackrabbit is an OpenJazz-derived port bundled with Epic MegaGames' unmodified shareware archive.",
            },
        },
    }


def write_source_manifest(path: Path, ir_sha: str, badusb_sha: str, music_sha: str, arduboy_sha: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(source_manifest(ir_sha, badusb_sha, music_sha, arduboy_sha), indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
        newline="\n",
    )


def write_app_source_manifest(path: Path, app_hashes: dict[str, str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(app_source_manifest(app_hashes), indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
        newline="\n",
    )


def resolve_source_commits() -> tuple[str, str, str, str]:
    return (
        resolve_remote_branch(IR_REPO, IR_BRANCH),
        resolve_remote_branch(BADUSB_REPO, BADUSB_BRANCH),
        resolve_remote_branch(MUSIC_REPO, MUSIC_BRANCH),
        resolve_remote_branch(ARDUBOY_REPO, ARDUBOY_BRANCH),
    )


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
    ir_files = sorted(src.glob("*.ir"), key=lambda path: (path.name.casefold(), path.name))

    if not ir_files:
        raise FileNotFoundError(f"Missing required IR assets in: {src}")

    dst.mkdir(parents=True)
    for asset in ir_files:
        shutil.copyfile(asset, dst / asset.name)


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

    bucket_large_music_dirs(dst)


def arduboy_hex_sort_key(path: Path) -> tuple[str, str]:
    posix = path.as_posix()
    return (posix.casefold(), posix)


def arduboy_game_folder(rel: Path) -> str:
    return rel.parts[0] if len(rel.parts) > 1 else rel.stem


def arduboy_unique_hex_name(rel: Path, used_names: set[str]) -> str:
    if rel.name.casefold() not in used_names:
        used_names.add(rel.name.casefold())
        return rel.name

    base_name = f"{arduboy_game_folder(rel)} - {rel.name}"
    candidate = base_name
    stem = Path(base_name).stem
    suffix = Path(base_name).suffix
    idx = 2

    while candidate.casefold() in used_names:
        candidate = f"{stem} ({idx}){suffix}"
        idx += 1

    used_names.add(candidate.casefold())
    return candidate


def copy_arduboy_assets(repo: Path, stage: Path) -> None:
    dst_root = stage / "ROMS" / "AB"
    dst_root.mkdir(parents=True, exist_ok=True)

    for genre in ARDUBOY_GENRE_DIRS:
        src = repo / genre
        if not src.is_dir():
            raise FileNotFoundError(f"Missing required Arduboy genre directory: {src}")

        dst = dst_root / genre
        dst.mkdir(parents=True, exist_ok=True)
        hex_files = sorted(
            (item.relative_to(src) for item in src.rglob("*") if item.is_file() and item.suffix.lower() == ".hex"),
            key=arduboy_hex_sort_key,
        )

        used_names: set[str] = set()
        for rel in hex_files:
            target_name = arduboy_unique_hex_name(rel, used_names)
            shutil.copyfile(src / rel, dst / target_name)


def music_file_bucket_key(path: Path) -> tuple[str, str]:
    return (path.name.casefold(), path.name)


def music_file_bucket_label(path: Path) -> str:
    for char in path.stem:
        if char.isalnum():
            char = char.upper()
            if "0" <= char <= "9":
                return "0-9"
            if "A" <= char <= "Z":
                return char
            break
    return "#"


def music_bucket_range_label(first: str, last: str) -> str:
    return first if first == last else f"{first}-{last}"


def music_bucket_chunk_label(label: str, idx: int) -> str:
    return f"{label}-{idx:02d}"


def move_music_files_to_bucket(parent: Path, label: str, files: list[Path]) -> None:
    if not files:
        return
    bucket = parent / label
    bucket.mkdir(exist_ok=True)
    for src in files:
        target = bucket / src.name
        if target.exists():
            raise FileExistsError(f"Music bucket collision: {target}")
        src.replace(target)


def bucket_large_music_dir(path: Path) -> None:
    files = sorted((item for item in path.iterdir() if item.is_file()), key=music_file_bucket_key)
    if len(files) <= MUSIC_BUCKET_FILE_LIMIT:
        return

    grouped: dict[str, list[Path]] = {label: [] for label in MUSIC_BUCKET_ORDER}
    for file in files:
        grouped[music_file_bucket_label(file)].append(file)

    pending_labels: list[str] = []
    pending_files: list[Path] = []

    def flush_pending() -> None:
        nonlocal pending_labels, pending_files
        if pending_files:
            move_music_files_to_bucket(path, music_bucket_range_label(pending_labels[0], pending_labels[-1]), pending_files)
            pending_labels = []
            pending_files = []

    for label in MUSIC_BUCKET_ORDER:
        label_files = grouped[label]
        if not label_files:
            continue
        if len(label_files) > MUSIC_BUCKET_FILE_LIMIT:
            flush_pending()
            for idx, start in enumerate(range(0, len(label_files), MUSIC_BUCKET_FILE_LIMIT), 1):
                move_music_files_to_bucket(path, music_bucket_chunk_label(label, idx), label_files[start:start + MUSIC_BUCKET_FILE_LIMIT])
            continue
        if pending_files and len(pending_files) + len(label_files) > MUSIC_BUCKET_FILE_LIMIT:
            flush_pending()
        pending_labels.append(label)
        pending_files.extend(label_files)

    flush_pending()


def bucket_large_music_dirs(music_root: Path) -> None:
    dirs = [music_root, *sorted((path for path in music_root.rglob("*") if path.is_dir()), key=lambda path: path.as_posix())]
    for path in dirs:
        bucket_large_music_dir(path)


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

    for name in ROM_PLACEHOLDER_DIRS:
        rom_dir = stage / "ROMS" / name
        rom_dir.mkdir(parents=True, exist_ok=True)

        (rom_dir / "README.txt").write_text(
            rom_messages[name],
            encoding="utf-8",
            newline="\n",
        )


def create_image_dir(stage: Path) -> None:
    image_dir = stage / "IMAGES"

    image_dir.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(Path(__file__).resolve().parent / "image_converter.py", image_dir / "image_converter.py")
    (image_dir / "README.txt").write_text(
        "Place .jpg, .jpeg, uncompressed .bmp, .dci, or .dca files in this folder.\n"
        "The badge opens JPEG/BMP stills directly. Run image_converter.py on your PC for badge-native .dci stills and .dca animations.\n",
        encoding="utf-8",
        newline="\n",
    )


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def collect_app_hashes(apps_dir: Path) -> dict[str, str]:
    missing = [name for name in APP_BINARIES if not (apps_dir / name).is_file()]
    if missing:
        raise FileNotFoundError(f"Missing app binaries in {apps_dir}: {', '.join(missing)}")
    missing_data = [name for name, src in APP_DATA_FILES.items() if not src.is_file()]
    if missing_data:
        raise FileNotFoundError(f"Missing app data files: {', '.join(missing_data)}")
    shareware_source = APP_DATA_FILES.get("APPS/JAZZ.ZIP")
    if (
        shareware_source
        and shareware_source.resolve() == OPENJAZZ_SHAREWARE_SOURCE.resolve()
        and sha256_file(shareware_source) != OPENJAZZ_SHAREWARE_SHA256
    ):
        raise ValueError("Bundled JAZZ.ZIP does not match the recorded original shareware archive")
    hashes = {name: sha256_file(apps_dir / name) for name in APP_BINARIES}
    hashes.update({name: sha256_file(src) for name, src in APP_DATA_FILES.items()})
    return hashes


def copy_app_binaries(apps_dir: Path, stage: Path) -> dict[str, str]:
    hashes = collect_app_hashes(apps_dir)
    dst = stage / "APPS"

    dst.mkdir(parents=True, exist_ok=True)
    for name in APP_BINARIES:
        shutil.copyfile(apps_dir / name, dst / name)
    for sd_path, src in APP_DATA_FILES.items():
        target = stage / sd_path
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(src, target)
    write_period_ports_readme(dst)
    return hashes


def write_period_ports_readme(apps_dir: Path) -> None:
    (apps_dir / "README-period-ports.txt").write_text(
        "Faithful period ports status.\n"
        "\n"
        "Packaged:\n"
        "- 207 Chip's Challenge: /APPS/chips.DC32 plus /APPS/chips-tworld.pak; user builds /APPS/chips.pak from Win 3.1 CHIPS.DAT and CHIPS.EXE\n"
        "- 208 Scorched Earth: /APPS/scorch.DC32 plus /APPS/scorch-xscorch.pak\n"
        "- 209 Pipe Dream: /APPS/pipe.DC32 plus /APPS/pipe-pipedreamer.pak\n"
        "- 210 Cave Story: /APPS/cave.DC32; user builds /APPS/cave.pak from freeware Cave Story data\n"
        "- 211 Sokoban: /APPS/sokoban.DC32 plus /APPS/sokoban-xsokoban.pak\n"
        "- 212 Jazz Jackrabbit: /APPS/openjazz.DC32 plus the original Epic MegaGames /APPS/JAZZ.ZIP shareware archive; first launch creates /APPS/openjazz.pak and a validated XIP graphics cache\n"
        "\n"
        "Required user data:\n"
        "- Cave Story: /APPS/cave.pak\n"
        "- Chip's Challenge: /APPS/chips.pak\n"
        "\n"
        "Cave Story freeware data, CHIPS.DAT, and proprietary original Chip's graphics are not redistributed. Build optional packs locally by running:\n"
        "- python tools/build_cave_pack.py\n"
        "- python tools/build_chips_pack.py\n"
        "Jazz Jackrabbit shareware remains copyright Epic MegaGames. JAZZ.ZIP is distributed unchanged under the terms in its LICENSE.DOC; SHA-256: 385f685d804b239e2ac070a1c267824b4a6b7898072248646c939a03469d345e.\n"
        "OpenJazz uses the same 3 MiB staging window as DOOM, so switching between them can trigger a one-time cache rebuild. The Cave and Chip's scripts prompt for paths and write the badge-ready pak. The Chip's packer auto-extracts original tiles from CHIPS.EXE when it is beside CHIPS.DAT; advanced/automated use still accepts flags, including --tiles for CHIPS.EXE, Tile World-style, regular-grid, or DC32CHIPTIL Chip graphics.\n"
        "\n"
        "The firmware menu exposes only period titles whose source-derived ports have passed acceptance.\n",
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


def write_sources(stage: Path, ir_sha: str, badusb_sha: str, music_sha: str, arduboy_sha: str) -> None:
    text = f"""# SD-assets.zip Sources

This release asset was assembled from upstream repositories at build time.
The external assets are distributed with credit and licensing retained by
their upstream projects.

## IR

- Repository: {IR_REPO}
- Branch: {IR_BRANCH}
- Commit: {ir_sha}
- Source path: {IR_ASSET_PATH.as_posix()}
- SD path: IR/
- Files: all .ir files in the source path

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
- Notes: {MUSIC_ARCHIVE} was extracted into MUSIC/{MUSIC_ARCHIVE_DIR}/ and the nested zip was omitted. Music folders with more than {MUSIC_BUCKET_FILE_LIMIT} direct files were split into alphabetic subfolders.

## ARDUBOY

- Repository: {ARDUBOY_REPO}
- Branch: {ARDUBOY_BRANCH}
- Commit: {arduboy_sha}
- Source paths: {', '.join(ARDUBOY_GENRE_DIRS)}
- SD path: ROMS/AB/
- Notes: Only .hex files were copied. Game folders were flattened into their genre folders, and duplicate flattened filenames were prefixed with the game folder name.

## ROMS

- SD paths: {', '.join(f'ROMS/{name}/' for name in ROM_DIRS)}
- Notes: ROMS/AB is populated from ArduboyCollection. Other ROM folders contain README.txt placeholders. Add only ROM files you can lawfully use and redistribute.

## IMAGES

- SD path: IMAGES/
- Files: image_converter.py, README.txt
- Notes: JPEG and uncompressed BMP stills can be opened directly. Run image_converter.py on a PC to convert still PNG/JPEG images into badge-native .dci files and animated GIF/APNG/WebP files into .dca animations.
"""
    (stage / "SOURCES.md").write_text(text, encoding="utf-8", newline="\n")


def write_app_sources(stage: Path, app_hashes: dict[str, str]) -> None:
    app_lines = "\n".join(f"- {name}: `{app_hashes[name]}`" for name in (*APP_BINARIES, *APP_DATA_FILES))
    nullpomino_license = NULLPOMINO_LICENSE.read_text(encoding="utf-8").rstrip()
    nullpomino_engine_license = NULLPOMINO_ENGINE_LICENSE.read_text(encoding="utf-8").rstrip()
    text = f"""# SD-apps.zip Sources

These app binaries were built from this repository and are loaded by the
resident firmware shell from /APPS. DOOM also includes the WHX data payload
from the rp2040-doom DEF CON 32 release under /APPS. Tetris is derived from
NullpoMino commit {NULLPOMINO_COMMIT}. Arkanoid is derived from
wkeeling/arkanoid commit 7e0e876cd034ebd62890e65352c7ef0b12b45df5 and
contains a generated scaled copy of that source's graphics. Flappy Bird is
derived from VadimBoev/FlappyBird {FLAPPY_TAG} commit {FLAPPY_COMMIT} and
contains generated pre-scaled RGB332 copies of that source's visible PNG sprites. Chip's Challenge,
Scorched Earth, Pipe Dream, Cave Story, Sokoban, and Jazz Jackrabbit are included as
source-derived/data-compatible period ports. Jazz Jackrabbit includes Epic
MegaGames' original unmodified shareware ZIP. This build accepts that
shareware data only.

## APPS

- SD path: APPS/
- Files:
{app_lines}

## DOOM

- Repository: {DOOM_REPO}
- Release: {DOOM_RELEASE}
- Source paths: doom1.whx
- SD path: APPS/
- Notes: Shareware DOOM1.WAD data is staged from APPS/doom1.whx at launch; raw WAD loading is not part of this build.

## Arkanoid

- Repository: https://github.com/wkeeling/arkanoid
- Commit: 7e0e876cd034ebd62890e65352c7ef0b12b45df5
- SD path: APPS/arkanoid.DC32
- Notes: The five-round gameplay is ported to native C. The pinned source archive is SHA-256 verified and its PNG
  graphics are scaled into an RGB332 atlas linked directly into the app.

## Flappy Bird

- Repository: {FLAPPY_REPO}
- Tag: {FLAPPY_TAG}
- Commit: {FLAPPY_COMMIT}
- Archive SHA-256: {FLAPPY_ARCHIVE_SHA256}
- SD path: APPS/flappy.DC32
- Notes: The Android Native Activity, OpenGL ES, OpenSLES, APK build scripts, and MP3 playback are not included. The badge app ports the source game states, physics, scoring, result panel, and visible PNG sprites into a silent native DC32 app.

## Tetris

- Repository: {NULLPOMINO_REPO}
- Commit: {NULLPOMINO_COMMIT}
- License: BSD 3-Clause
- SD path: APPS/tetris.DC32
- Notes: The native C port adapts the upstream piece, rule, randomizer, timing, and scoring behavior for three single-player modes. No upstream graphics, music, or sound assets are included.

### NullpoMino project license

{nullpomino_license}

### Adapted engine-source license

{nullpomino_engine_license}

## Jazz Jackrabbit Shareware

- Publisher: Epic MegaGames
- Archive: JAZZ.ZIP
- SHA-256: {OPENJAZZ_SHAREWARE_SHA256}
- SD path: APPS/
- Notes: The original archive is copied unchanged. OpenJazz creates openjazz.pak locally on first launch, then builds and validates the complete shareware graphics cache before the menu. Gameplay performs no QSPI writes. The 3 MiB staging window is shared with DOOM and emulator ROM staging, so overwrites trigger an automatic rebuild.

## Faithful Period Ports

- Packaged: 207 Chip's Challenge, derived from GPL Tile World engine/assets; 208 Scorched Earth, derived from GPL xscorch tables/assets; 209 Pipe Dream, derived from MIT PipeDreamer logic/assets; 210 Cave Story, data-compatible with freeware Cave Story packs and informed by NXEngine-evo/doukutsu-rs formats; 211 Sokoban, derived from public-domain XSokoban screens and pixmaps; 212 Jazz Jackrabbit, derived from GPL OpenJazz and bundled with the original Epic MegaGames shareware archive
- Data helpers: tools/build_cave_pack.py, tools/build_chips_pack.py, tools/build_openjazz_pack.py, tools/build_period_assets.py
- Notes: Cave Story freeware data, CHIPS.DAT, and unclear proprietary original graphics are user-provided only. The current Jazz Jackrabbit port supports the bundled shareware data.
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
    parser = argparse.ArgumentParser(description="Build SD-assets.zip and/or SD-apps.zip.")
    parser.add_argument("--output", type=Path, help="Compatibility alias for --assets-output")
    parser.add_argument("--assets-output", type=Path, help="Path to write SD-assets.zip")
    parser.add_argument("--apps-output", type=Path, help="Path to write SD-apps.zip")
    parser.add_argument("--work-dir", type=Path, help="Temporary work directory")
    parser.add_argument("--apps-dir", type=Path, help="Directory containing built .DC32 app binaries")
    parser.add_argument("--sources-output", type=Path, help="Path to write an SD-assets source manifest JSON file")
    parser.add_argument("--apps-sources-output", type=Path, help="Path to write an SD apps source manifest JSON file")
    parser.add_argument("--sources-only", action="store_true", help="Only resolve sources and write --sources-output")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    default_apps_dir = REPO_ROOT / "build" / "apps"
    apps_dir = (args.apps_dir or default_apps_dir).resolve()
    assets_output = (args.assets_output or args.output).resolve() if (args.assets_output or args.output) else None
    apps_output = args.apps_output.resolve() if args.apps_output else None

    if args.sources_only:
        if not args.sources_output:
            raise ValueError("--sources-only requires --sources-output")
        ir_sha, badusb_sha, music_sha, arduboy_sha = resolve_source_commits()
        write_source_manifest(args.sources_output.resolve(), ir_sha, badusb_sha, music_sha, arduboy_sha)
        if args.apps_sources_output:
            write_app_source_manifest(args.apps_sources_output.resolve(), collect_app_hashes(apps_dir))
        print(f"Wrote {args.sources_output.resolve()}")
        return 0

    if not assets_output and not apps_output:
        raise ValueError("--assets-output/--output or --apps-output is required unless --sources-only is used")
    if not args.work_dir:
        raise ValueError("--work-dir is required unless --sources-only is used")

    work_dir = args.work_dir.resolve()

    if assets_output:
        repos = work_dir / "repos"
        stage = work_dir / "assets-stage"

        ensure_clean_dir(repos)
        ensure_clean_dir(stage)

        ir_repo = repos / "Momentum-Firmware"
        badusb_repo = repos / "UberGuidoZ-Flipper"
        music_repo = repos / "FlipperMusicRTTTL"
        arduboy_repo = repos / "ArduboyCollection"

        ir_sha = clone_repo(IR_REPO, IR_BRANCH, ir_repo, [IR_ASSET_PATH.as_posix()])
        badusb_sha = clone_repo(BADUSB_REPO, BADUSB_BRANCH, badusb_repo, [BADUSB_PATH.as_posix()])
        music_sha = clone_repo(MUSIC_REPO, MUSIC_BRANCH, music_repo)
        arduboy_sha = clone_repo(ARDUBOY_REPO, ARDUBOY_BRANCH, arduboy_repo, list(ARDUBOY_GENRE_DIRS))

        copy_ir_assets(ir_repo, stage)
        copy_badusb_assets(badusb_repo, stage)
        copy_music_assets(music_repo, stage)
        copy_arduboy_assets(arduboy_repo, stage)
        create_rom_dirs(stage)
        create_image_dir(stage)
        write_sources(stage, ir_sha, badusb_sha, music_sha, arduboy_sha)
        if args.sources_output:
            write_source_manifest(args.sources_output.resolve(), ir_sha, badusb_sha, music_sha, arduboy_sha)
        build_zip(stage, assets_output)
        print(f"Wrote {assets_output}")

    if apps_output:
        app_stage = work_dir / "apps-stage"
        ensure_clean_dir(app_stage)
        app_hashes = copy_app_binaries(apps_dir, app_stage)
        write_app_sources(app_stage, app_hashes)
        if args.apps_sources_output:
            write_app_source_manifest(args.apps_sources_output.resolve(), app_hashes)
        build_zip(app_stage, apps_output)
        print(f"Wrote {apps_output}")

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
