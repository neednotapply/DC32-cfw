# DC32 Badge Firmware

This repository contains `DC32-cfw`, a custom DEF CON 32 badge firmware based on Dmitry Grinberg's uGB badge firmware. The current on-device header is `DC32-cfw v1.6.9`. The codebase targets the Raspberry Pi RP2350 microcontroller used on the badge and bundles the uGB Game Boy emulator alongside badge-specific display, audio, storage, IR, USB HID, LED, and user-interface plumbing.

The original firmware and emulator were created by Dmitry Grinberg (DmitryGR), whose broader work is highlighted at [dmitry.gr](https://dmitry.gr/), with hardware collaboration and support from [Entropic Engineering](https://www.entropicengineering.com/).

## Current functionality

- **Game Boy and Game Boy Color emulation** - Runs uGB on the DEF CON 32 badge hardware, including cartridge parsing, mapper support, save RAM handling, optional GBC behavior, display speed selection, and optional upscaling.
- **Tool shell** - Boots into a badge-native launcher with Browser, IR, BadUSB, Music, Game, Settings, Firmware Update, and Power Off entries. A boot guard records crashes or resets inside tools and restarts into a safe shell instead of immediately re-entering the failing mode.
- **SD-card file browser** - Browses folders on the SD card and opens supported files with the right tool: `.gb`/`.gbc` games, `.ir` remotes, `.txt`/`.badusb` BadUSB scripts, `.rtttl`/`.txt` RTTTL music, and `.bin` firmware images.
- **Game loading and saves** - Loads Game Boy ROMs from SD, confirms ROM metadata before flashing/loading, exports the current save when changing games, and keeps flash-backed save state support for the active game.
- **IR tools** - Supports Flipper-style `.ir` files, DC32 IR library files, universal remote categories for TV, A/C, Audio, and Projector files on the SD card, individual button selection, and power/mute blast modes with cancellable progress screens.
- **BadUSB** - Runs scripts from `/BADUSB` or from the browser using the badge as a USB HID keyboard/mouse/consumer-control device. Supported commands include strings, delays, repeats, key chords, hold/release, mouse movement/click/scroll, media keys, Alt codes, SysRq, custom USB IDs, and wait-for-button pauses.
- **RTTTL music player** - Plays `.rtttl` and RTTTL `.txt` files from `/MUSIC` or from the browser, including folder navigation, progress display, play/pause, previous/next, per-track loop, and persistent volume.
- **LED controls** - Drives the badge WS2812 LEDs with off, all-on, front-on, rear-on, rainbow, pulse, dot, and random patterns; supports custom, rainbow, flame, and random colors; and exposes speed plus brightness controls.
- **Screen and game settings** - Provides on-device settings for screen rotation, brightness/contrast where supported, Game Boy Color mode, upscaling, emulator speed, and LED behavior.
- **Firmware updater** - Installs raw `.bin` images from the SD card through the dedicated updater or from the file browser after basic firmware-image checks. UF2 output is also generated for RP2350 bootloader flashing.

## Comparison with stock firmware

The comparison below uses the public [DEFCON-32-BadgeFirmware archive](https://github.com/jaku/DEFCON-32-BadgeFirmware), whose [firmware note](https://raw.githubusercontent.com/jaku/DEFCON-32-BadgeFirmware/main/firmware/readme.md) identifies the images as official firmware posted with DmitryGR's permission. The baseline used here is the stock image in [firmware/1.6](https://github.com/jaku/DEFCON-32-BadgeFirmware/tree/main/firmware/1.6). A string-level check against `firmware/1.6/stock-firmware.bin` shows the stock image exposes the original uGB game/settings/update flow, while this firmware adds the broader tool shell and file-driven utilities.

| Capability | Stock official firmware | `DC32-cfw v1.6.9` |
| ---------- | ----------------------- | ----------------- |
| uGB badge game runtime | Yes | Yes |
| Game settings, firmware update, power off | Yes | Yes |
| SD-card ROM loading | Limited to stock/custom-image flows | Yes, from the Game tool and browser |
| General SD file browser | No visible browser entry | Yes |
| BadUSB scripting | Not present | Yes |
| RTTTL music player | Not present | Yes |
| IR remote and IR blast tools | Not present as user tools | Yes |
| Expanded LED pattern/color settings | Basic badge LED behavior | Yes |
| Safe tool-shell recovery after crashes/resets | Not present as a visible workflow | Yes |
| Release formats | Stock `.bin` and `.uf2` images | `.bin`, `FIRMWARE.BIN`, and `.uf2` images |

Stock firmware is still the right choice if you want the original badge experience with the official game image. `DC32-cfw` is aimed at using the badge as a post-con tool platform: loading ROMs, browsing SD-card content, replaying IR files, running HID scripts, playing RTTTL files, and updating firmware without rebuilding a full custom image each time.

## Repository layout

| Path | Description |
| ---- | ----------- |
| `src/main_rp2350_defcon.c` | Firmware entry point and hardware bring-up for the badge. |
| `src/gb.c`, `src/gbCore.h` | Core Game Boy execution loop, CPU, and timing helpers. |
| `src/mappersC.c`, `src/mbc.c` | Cartridge mapper implementations and metadata parsing. |
| `src/dispDefcon.c` | LCD driver, PIO program loading, and DMA management. |
| `src/pio*.c/.h` | RP2350 PIO programs for I2C, IRDA, and WS2812 peripherals. |
| `src/sd*.c`, `src/fatfs.c` | Storage stack and filesystem integration for ROM loading. |
| `src/timebase.c`, `src/sleepDefcon.c` | Timing, delay, and power utilities for the badge runtime. |
| `src/settings.c`, `src/ui.c`, `src/fonts.c` | Badge configuration UI, persistence, and fonts. |
| `src/Makefile` | Build rules for generating the badge firmware binary, UF2 input, and SD-card updater image. |

## Getting started

### Prerequisites

- **Toolchain:** Install the ARM embedded GCC toolchain (`arm-none-eabi-gcc`, `arm-none-eabi-objcopy`) and `make`, which are used to compile and package the firmware.
- **Optional direct flashing utility:** The legacy `make flash` recipe depends on `CortexProg` to write the raw `DC32-cfw.bin` image to the RP2350 over USB (`sudo CortexProg info targetsel 0 write DC32-cfw.bin 0x10000000`). Adjust `CMD` in `src/Makefile` if you use a different programmer.

### Building the firmware

1. Clone the repository and `cd` into `DC32/src`.
2. Build the binary image:
   ```bash
   make app
   ```
   This produces `DC32-cfw.bin`, which contains the firmware image built with the badge-specific drivers and emulator core.
3. Build the SD-card updater image, if needed:
   ```bash
   make sdcard
   ```
   This produces `FIRMWARE.BIN`, a byte-for-byte copy of `DC32-cfw.bin` using the legacy updater filename.
4. Optional flags:
   - `SDL=1` links against SDL2 to enable the desktop simulation front-end used for development builds.
   - `HQX=1` adds HQ3x upscaling support when building the optional host tooling.

Clean intermediate objects with `make clean` when switching build options or updating toolchains.

### Flashing to a badge

The GitHub Actions workflow packages a `firmware` artifact after it builds a commit. GitHub releases also receive the end-user update files:

- `DC32-cfw.uf2` - recommended RP2350 USB bootloader image. Copy this only to the mounted UF2 bootloader drive.
- `DC32-cfw.bin` and `FIRMWARE.BIN` - raw binary images for the SD-card updater or direct programmer. Do not copy these to the UF2 bootloader drive.
- `uGB.uf2` - temporary compatibility alias for `DC32-cfw.uf2`.

The build still creates `DC32-cfw.bin` internally as the raw firmware image, but release packages only publish the files users should copy to a badge. Use one flashing path per update:

#### UF2 bootloader

Put the badge into the RP2350 USB bootloader mode, then copy the downloaded `DC32-cfw.uf2` onto the mounted UF2 drive. The drive will disconnect after the copy completes and the badge will boot the new firmware. The UF2 bootloader path requires a `.uf2` file, not `DC32-cfw.bin` or `FIRMWARE.BIN`.

#### Direct BIN programmer

The direct binary path is available with `sudo make flash`; it wraps the `CortexProg` invocation defined in `CMD` and writes the raw `DC32-cfw.bin` image to the RP2350 XIP address space (`0x10000000`). Modify the command or provide the appropriate permissions if your setup differs.

#### SD-card updater

Copy a raw `.bin` firmware image such as `DC32-cfw.bin` or `FIRMWARE.BIN` into `/FIRMWARE` on the badge SD card. The on-device firmware update option starts there and can browse subfolders. The general file browser also opens `.bin` files with the firmware updater.

A `make trace` target is also provided for developers who want to start the badge with hardware tracing enabled at the configured watch address (`ZWT_ADDR`).

## Development tips

- The firmware is tuned for the badge's rotated LCD orientation (`DISP_LCD_DEFCON`, `UNSCALED_IMG_ROTATED`, and `UI_ROTATED` configuration flags) and an upscaler path that can be toggled in the UI (`ui.c`).
- Hardware-specific constants such as GPIO assignments live in `pinoutRp2350defcon.h` and `platform_defs.h`. Update these headers if you adapt the firmware to custom hardware.
- The emulator core supports multiple mapper types; see `mappersC.c` and `mbc.c` for examples when adding new cartridge hardware definitions.

## Support and contributions

Issues and pull requests are welcome. Please include details about your toolchain version, the badge hardware revision, and any customizations you've made when reporting problems so maintainers can reproduce the environment.

## Acknowledgements

- Firmware lead: Dmitry Grinberg (DmitryGR) - <https://dmitry.gr/>
- Hardware partner: Entropic Engineering - <https://www.entropicengineering.com/>
