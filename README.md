# DC32 Custom Firmware

`DC32-cfw` is a custom DEF CON 32 badge firmware based on Dmitry Grinberg's uGB badge firmware. The current firmware boots into a badge-native tool shell named `DC32-cfw` and targets the Raspberry Pi RP2350-based DEF CON 32 badge. It combines the uGB Game Boy emulator with NES support, an SD-card file browser, USB microSD storage mode, Universal IR tools, BadUSB scripting, an RTTTL music player, LED controls, settings persistence, and badge hardware bring-up code.

The original firmware was created by Dmitry Grinberg (DmitryGR), whose broader work is highlighted at [dmitry.gr](https://dmitry.gr/), with hardware collaboration and support from [Entropic Engineering](https://www.entropicengineering.com/).

## Current functionality

- **Tool shell and recovery** - Boots to `DC32-cfw`, shows a Main Menu, and uses boot guard state to recover to the menu after a reset or hard fault inside a tool. Crash recovery can show the failed mode plus fault registers.
- **Main Menu** - Provides File Browser, Universal IR, USB Storage, BadUSB, HID Test, Music, Game, Settings, and Power Off entries. HID Test exposes a direct keyboard report check while mouse/composite HID work remains out of scope.
- **SD-card file browser** - Browses folders, hides dot/hidden entries, sorts directories before files, and opens supported files with the matching tool. Registered file types are `.gb`, `.gbc`, `.nes`, `.ir`, `.badusb`, `.rtttl`, and `.txt`.
- **Game Boy and Game Boy Color emulation** - Runs uGB on badge hardware, including cartridge metadata parsing, mapper support, optional GBC behavior, flash-backed save state, SD save import/export, display speed selection, rotation, and optional upscaling.
- **NES emulation** - Loads `.nes` files from SD, validates iNES headers, supports mappers 0, 1, 2, 3, 4, and 7, detects NTSC/PAL/Dendy timing, supports 8 KiB save RAM, and uses the same game menu, save, speed, rotation, and upscaling settings path as the Game Boy runtime.
- **Game loading and saves** - Loads games from `/ROMS` or the browser, confirms ROM metadata before flashing/loading, stores the selected game path/runtime in flash, imports saves from `/SAVE`, and exports the current save before switching games when save RAM is present.
- **Universal IR** - Supports Flipper-style `.ir` signal/library files and the older `DC32IR1` format. Built-in universal remote categories look for `/IR/ac.ir`, `/IR/audio.ir`, `/IR/projector.ir`, and `/IR/tv.ir`; browser actions can send a selected button, Power, or Mute. The IR sender supports raw records plus parsed NEC/NECext/NEC42/Samsung32/RC5/RC6/SIRC variants with bounded repeat, carrier, and raw duration validation.
- **BadUSB** - Runs scripts from `/BADUSB` or from the browser using the badge as an on-demand boot-compatible USB HID keyboard. Supported commands include `REM`, `DELAY`, `DEFAULT_DELAY`, `STRING`, `STRINGLN`, `STRING_DELAY`, `DEFAULT_STRING_DELAY`, `HOLD`, `RELEASE`, `ALTCHAR`, `ALTSTRING`/`ALTCODE`, `SYSRQ`, `GLOBE`, `WAIT_FOR_BUTTON_PRESS`, `REPEAT`, key chords, and optional first-line USB VID/PID/product overrides with `ID`.
- **USB Storage** - Exposes the full microSD card to a host computer as a standalone USB Mass Storage device. Eject or unmount from the host before leaving the tool; other SD-card tools are unavailable while storage mode is active.
- **RTTTL music player** - Plays `.rtttl` and RTTTL `.txt` files from `/MUSIC` or from the browser, with folder navigation, progress display, play/pause, previous/next, per-track loop, and persistent volume.
- **LED controls** - Drives the badge WS2812 LEDs with off, all-on, rainbow, pulse, traveling dot, random, rear-on, and front-on patterns. Color modes include custom RGB, rainbow, flame, and random, with speed and brightness controls.
- **Settings** - Persists Game, LED, Screen, and Music settings in QSPI flash. Game settings include color mode, upscaling, and display speed (`50%`, `100%`, `150%`, `200%`). Screen settings include rotation and brightness. LED settings include pattern, color, custom RGB, speed, and brightness.
- **Hardware self-test hooks** - Contains a badge self-test path for display, buttons, touch, RTC, IMU/ADC readings, LEDs, and power sequencing when the self-test trigger condition is present.

## Comparison with stock firmware

The comparison below uses the public [DEFCON-32-BadgeFirmware archive](https://github.com/jaku/DEFCON-32-BadgeFirmware), whose [firmware note](https://raw.githubusercontent.com/jaku/DEFCON-32-BadgeFirmware/main/firmware/readme.md) identifies the images as official firmware posted with DmitryGR's permission. The baseline used here is the stock image in [firmware/1.6](https://github.com/jaku/DEFCON-32-BadgeFirmware/tree/main/firmware/1.6).

| Capability | Stock official firmware | `DC32-cfw` |
| ---------- | ----------------------- | ---------- |
| Original uGB badge game runtime | Yes | Yes |
| Game settings and power off | Yes | Yes |
| SD file browser | No | Yes |
| Game Boy ROM loading from SD | Limited/original flow | Yes |
| NES ROM loading from SD | No | Yes |
| USB microSD storage mode | No | Yes |
| BadUSB scripting | No | Yes |
| RTTTL music player | No | Yes |
| Universal IR tools | No | Yes |
| Expanded LED pattern/color settings | No | Yes |
| Safe tool-shell recovery after crashes/resets | No | Yes |

Stock firmware is still the right choice if you want the original badge experience with the official game image. `DC32-cfw` is aimed at using the badge as a post-con tool platform: loading ROMs, browsing SD-card content, replaying IR files, running HID scripts, playing RTTTL files, and experimenting with the badge hardware.

## SD card layout

The firmware can browse the full card, but the menu tools look in these conventional folders first:

| Path | Used by |
| ---- | ------- |
| `/ROMS` | Game picker root, conventionally split into `/ROMS/GB`, `/ROMS/GBC`, and `/ROMS/NES`. |
| `/SAVE` | Imported/exported save RAM files for the selected game. |
| `/IR` | Universal IR files, including `ac.ir`, `audio.ir`, `projector.ir`, `tv.ir`, and optional legacy `POWER.IR`. |
| `/BADUSB` | BadUSB script picker for `.txt` and `.badusb` files. |
| `/MUSIC` | Music picker for `.rtttl` and RTTTL `.txt` files. Large generated music folders are split into alphabetic range subfolders so the badge can list them reliably. |

Release builds include `SD.zip`, an optional starter SD-card asset bundle. Extract `SD.zip` directly to the SD card root so `IR/`, `BADUSB/`, `MUSIC/`, and `ROMS/` land alongside any existing folders. The bundle is assembled at workflow time from upstream GitHub repositories and records the exact source URLs, branches, commits, and copied paths in `SOURCES.md` inside the zip.

## SD.zip asset credits

`SD.zip` fetches external assets from these upstream projects at workflow time:

| SD path | Source |
| ------- | ------ |
| `/IR` | [flipperdevices/flipperzero-firmware](https://github.com/flipperdevices/flipperzero-firmware), `applications/main/infrared/resources/infrared/assets` |
| `/BADUSB` | [UberGuidoZ/Flipper](https://github.com/UberGuidoZ/Flipper), `BadUSB` |
| `/MUSIC` | [neverfa11ing/FlipperMusicRTTTL](https://github.com/neverfa11ing/FlipperMusicRTTTL) |
| `/ROMS/GB`, `/ROMS/GBC`, `/ROMS/NES` | Folders for user-provided ROMs, each with a `README.txt` placeholder; add only files you can lawfully use and redistribute. |

Credit and licensing for bundled external assets remain with their upstream projects.

## Repository layout

| Path | Description |
| ---- | ----------- |
| `CMakeLists.txt` | Primary firmware build used by CI; builds ELF, BIN, and UF2 outputs. |
| `.github/workflows/build.yml` | GitHub Actions firmware build and release artifact packaging. |
| `src/main_rp2350_defcon.c` | Firmware entry point, clocks, GPIO, display, IRDA, RTC, LEDs, and tool shell startup. |
| `src/ui.c` | On-device UI, tool shell, file browser, game selection, settings, IR, music, BadUSB, and boot recovery screens. |
| `src/gb.c`, `src/gbCore.h`, `src/gbC.c` | Game Boy execution loop, CPU helpers, and core emulator integration. |
| `src/mappersC.c`, `src/mbc.c` | Game Boy cartridge mapper implementations and metadata parsing. |
| `src/nes/` | InfoNES-derived NES runtime and mapper code used by the current CMake build. |
| `src/dispDefcon.c` | LCD driver, framebuffer, PIO program loading, DMA management, brightness, and framerate handling. |
| `src/sd*.c`, `src/fatfs.c` | SD-card and FAT filesystem integration. |
| `src/badUsb.c`, `src/usb*.c` | Shared TinyUSB device setup, USB Mass Storage, keyboard-only HID, and BadUSB interpreter. |
| `src/irRemote.c`, `src/pioIrdaSIR.c` | IR transmitter support and badge IRDA setup. |
| `src/rtttlPlayer.c`, `src/audioPwm.c` | RTTTL parsing/playback and PWM audio output. |
| `src/badgeLeds.c`, `src/pioWS2812.c` | WS2812 LED rendering and PIO driver. |
| `src/settings.c`, `src/bootGuard.c`, `src/toolWorkspace.c` | Persistent settings, tool crash/reset recovery, and shared workspace allocation. |
| `tools/bin_to_uf2.py` | Converts the raw binary image to the RP2350 UF2 update file. |
| `tools/build_sd_zip.py` | Fetches upstream SD-card assets and packages the release `SD.zip` bundle. |
| `tools/fatfs_regression_test.c` | Host-side FAT filesystem regression helper. |
| `src/Makefile` | Legacy/developer make flow; the root CMake build is the current supported build path. |

## Building

### Prerequisites

- CMake 3.20 or newer
- Ninja or another CMake-supported build tool
- Python 3
- ARM embedded GCC toolchain (`arm-none-eabi-gcc`, `arm-none-eabi-objcopy`, `arm-none-eabi-size`)
- Newlib for ARM embedded builds

On Ubuntu, the CI installs:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake gcc-arm-none-eabi libnewlib-arm-none-eabi ninja-build
```

### Firmware build

From the repository root:

```bash
git submodule update --init --recursive
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake \
  -DPICO_BOARD=defcon32_badge
cmake --build build
```

The build emits:

- `build/DC32-cfw` - ELF image
- `build/DC32-cfw.bin` - raw XIP firmware image
- `build/DC32-cfw.uf2` - RP2350 USB bootloader image

The post-build step also runs `arm-none-eabi-size` and checks the firmware against the configured RAM budget.

## Flashing

### UF2 bootloader

Use `build/DC32-cfw.uf2` for normal updates. Put the badge into RP2350 USB bootloader mode, then copy `DC32-cfw.uf2` onto the mounted UF2 drive. The drive disconnects after the copy completes and the badge boots the new firmware.

GitHub Actions uploads `DC32-cfw.uf2` as a small `DC32-cfw-uf2` artifact. The larger SD-card bundle is uploaded separately as `DC32-cfw-sd` only when the SD source manifest changes, and release builds attach `DC32-cfw.uf2` plus `SD.zip`.

### Direct BIN programmer

Use `build/DC32-cfw.bin` only with a direct programmer. The legacy makefile used this CortexProg command shape:

```bash
sudo CortexProg info targetsel 0 write DC32-cfw.bin 0x10000000
```

Adjust the programmer command, permissions, or path for your setup.

## Development notes

- The current CMake build defines the badge orientation/display path with `DISP_LCD_DEFCON`, `UNSCALED_IMG_ROTATED`, `UPSCALER_ROTATES`, and `UI_ROTATED`.
- Hardware-specific GPIO assignments live in `src/pinoutRp2350defcon.h`; platform constants and display dimensions live in `src/platform_defs.h` and `src/dispDefcon.h`.
- QSPI layout is defined in `src/memMap.h`: settings, selected-game metadata, save RAM copy, and the loaded ROM occupy fixed flash regions.
- Game ROM size is capped by `QSPI_ROM_SIZE_MAX` and save RAM by `QSPI_RAM_SIZE_MAX`.
- The NES runtime intentionally enables only mappers 0, 1, 2, 3, 4, and 7 in `src/nes/InfoNES_Mapper.cpp`.
- USB Storage exposes the raw microSD card as a read-write Mass Storage device when the dedicated tool is active. Eject/unmount from the host before exiting the tool or unplugging the badge.
- The USB HID implementation is keyboard-only and attaches only while BadUSB or HID Test is running. BadUSB mouse/media behavior and AutoClicker output are not available in this build.

## Support and contributions

Issues and pull requests are welcome. Please include your toolchain version, badge hardware revision, the UF2 or commit you tested, and any SD-card layout details needed to reproduce the issue.

## Acknowledgements

- Firmware lead: Dmitry Grinberg (DmitryGR) - <https://dmitry.gr/>
- Hardware partner: Entropic Engineering - <https://www.entropicengineering.com/>
