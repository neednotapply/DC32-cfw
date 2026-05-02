# DC32 Badge Firmware

This repository contains the custom firmware image that shipped with the DEF CON 32 badge. The codebase targets the Raspberry Pi RP2350 microcontroller used on the badge and bundles a complete "uGB" Game Boy emulator along with the bespoke display, audio, storage, and user-interface plumbing required to run it on the hardware. The firmware and emulator were created by Dmitry Grinberg (DmitryGR), whose broader work is highlighted at [dmitry.gr](https://dmitry.gr/), with hardware collaboration and support from [Entropic Engineering](https://www.entropicengineering.com/).

## Features

- **Game Boy emulation** - The `gb*.c` modules implement the core CPU, PPU, cartridge, and memory-mapper logic for the uGB emulator that drives badge gameplay.
- **Badge-native display pipeline** - `dispDefcon.c` programs the RP2350 PIO, DMA, and PWM peripherals to stream frames to the badge LCD while handling backlight control and frame timing.
- **Audio, input, and badge peripherals** - `main_rp2350_defcon.c` initializes the hardware, services the UI, and orchestrates audio playback, touch input, IR, accelerometer, and the WS2812 LED strip integration specific to the DEF CON badge layout.
- **Filesystem and ROM loading** - The SD card and FAT filesystem stack (`sd*.c`, `fatfs.c`) provide persistent storage so Game Boy ROMs and saves can be loaded from removable media.
- **Settings and UI support** - `ui.c`, `settings.c`, `fonts.c`, and related helpers deliver an on-device configuration experience and badge-specific overlays for the emulator.
- **Original badge game** - The adventure that shipped on the badge is preserved in the [DC32BadgeGame repository](https://github.com/CosmicBonBon/DC32BadgeGame), which contains the game assets and scripting that run on top of this firmware.

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
- **Optional direct flashing utility:** The legacy `make flash` recipe depends on `CortexProg` to write `uGB.bin` to the RP2350 over USB (`sudo CortexProg info targetsel 0 write uGB.bin 0x10000000`). Adjust `CMD` in `src/Makefile` if you use a different programmer.

### Building the firmware

1. Clone the repository and `cd` into `DC32/src`.
2. Build the binary image:
   ```bash
   make app
   ```
   This produces `uGB.bin`, which contains the firmware image built with the badge-specific drivers and emulator core.
3. Build the SD-card updater image, if needed:
   ```bash
   make sdcard
   ```
   This produces `FIRMWARE.BIN`, a byte-for-byte copy of `uGB.bin` with the exact filename the badge's on-device updater looks for at the SD card root.
4. Optional flags:
   - `SDL=1` links against SDL2 to enable the desktop simulation front-end used for development builds.
   - `HQX=1` adds HQ3x upscaling support when building the optional host tooling.

Clean intermediate objects with `make clean` when switching build options or updating toolchains.

### Flashing to a badge

The GitHub Actions workflow packages a `firmware` artifact after it builds a commit. GitHub releases also receive the two end-user update files:

- `uGB.uf2` - recommended RP2350 USB bootloader image.
- `FIRMWARE.BIN` - SD-card updater image for the badge UI.

The build still creates `uGB.bin` internally as the raw firmware image, but release packages only publish the files users should copy to a badge. Use one flashing path per update:

#### UF2 bootloader

Put the badge into the RP2350 USB bootloader mode, then copy the downloaded `uGB.uf2` onto the mounted UF2 drive. The drive will disconnect after the copy completes and the badge will boot the new firmware.

#### Direct BIN programmer

The direct binary path is available with `sudo make flash`; it wraps the `CortexProg` invocation defined in `CMD` and writes `uGB.bin` to the RP2350 XIP address space (`0x10000000`). Modify the command or provide the appropriate permissions if your setup differs.

#### SD-card updater

Copy `FIRMWARE.BIN` to the root of the badge SD card so `/FIRMWARE.BIN` exists. Insert the SD card, boot the badge, and use the on-device firmware update option. The updater looks specifically for `/FIRMWARE.BIN`, so keep the filename uppercase and unchanged.

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
