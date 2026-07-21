#!/usr/bin/env python3
"""Package the public DCAPP SDK ABI artifacts independently of firmware/apps."""

from __future__ import annotations

import argparse
import shutil
import tempfile
import zipfile
from pathlib import Path

from gen_dcapp_abi_veneers import IMPORTS


ROOT = Path(__file__).resolve().parents[1]
SDK_ABI = 1
VENEER_START = 0x10070000
VENEER_STRIDE = 12


def write_import_linker(path: Path) -> None:
    lines = ["/* DC32 SDK ABI v1 fixed import veneers. */"]
    for index, name in enumerate(IMPORTS):
        lines.append(f"PROVIDE({name} = 0x{VENEER_START + index * VENEER_STRIDE:08x});")
    path.write_text("\n".join(lines) + "\n", encoding="ascii", newline="\n")


def write_readme(path: Path) -> None:
    path.write_text(
        "# DC32 App SDK v1\n\n"
        "Apps built with this SDK require firmware advertising DCAPP_SDK_ABI 1. Copy a "
        "built `.DC32` file to `/APPS`; its embedded metadata controls its display name and category. "
        "A firmware rebuild is not required.\n\n"
        "Use the supplied CMake helper from an app project:\n\n"
        "```cmake\n"
        "include(path/to/DC32App.cmake)\n"
        "dc32_add_app(my_app MyApp 0x80000001 games main.c)\n"
        "```\n\n"
        "Categories are `games`, `ports`, `demos`, `media`, `infrared`, and `usb`. Use the ARM "
        "toolchain file from the firmware repository when configuring the app project.\n\n"
        "The ABI linker map points imports at fixed firmware veneers. Do not link against a firmware "
        "ELF or generated whole-firmware symbol table. Adding an import or changing a public header is "
        "an SDK ABI change.\n",
        encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="dc32-sdk-") as temp:
        stage = Path(temp) / f"DC32-App-SDK-v{SDK_ABI}"
        (stage / "include").mkdir(parents=True)
        # Preserve header include paths so SDK clients never need the firmware checkout.
        for source in (ROOT / "src").rglob("*.h"):
            target = stage / "include" / source.relative_to(ROOT / "src")
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target)
        shutil.copy2(ROOT / "src" / "linker_dcapp.lkr", stage / "linker_dcapp.lkr")
        shutil.copy2(ROOT / "src" / "linker_dcapp_large.lkr", stage / "linker_dcapp_large.lkr")
        shutil.copy2(ROOT / "src" / "syscalls.c", stage / "syscalls.c")
        shutil.copy2(ROOT / "tools" / "make_dcapp.py", stage / "make_dcapp.py")
        shutil.copy2(ROOT / "sdk" / "DC32App.cmake", stage / "DC32App.cmake")
        write_import_linker(stage / "dcapp_sdk_v1.ld")
        write_readme(stage / "README.md")
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(args.output, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for source in sorted(stage.rglob("*")):
                if source.is_file():
                    archive.write(source, source.relative_to(stage.parent).as_posix())
    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
