# DC32 Custom Firmware

`DC32-cfw` is a custom DEF CON 32 badge firmware based on Dmitry Grinberg's uGB badge firmware. The current firmware boots into a badge-native tool shell named `DC32-cfw` and targets the Raspberry Pi RP2350-based DEF CON 32 badge. It combines the uGB Game Boy emulator with NES and classic Arduboy support, categorized SD-card app menus, an SD-card file browser, USB microSD storage mode, Universal IR tools, BadUSB scripting, USB HID utility apps, an RTTTL/WAV music player, LED controls, settings persistence, and badge hardware bring-up code.

The original firmware was created by Dmitry Grinberg (DmitryGR), whose broader work is highlighted at [dmitry.gr](https://dmitry.gr/), with hardware collaboration and support from [Entropic Engineering](https://www.entropicengineering.com/).

## Current functionality

- **Tool shell and recovery** - Boots to `DC32-cfw`, shows a Main Menu, and uses boot guard state to recover to the menu after a reset or hard fault inside a tool. Crash recovery can show the failed mode plus fault registers.
- **Main Menu** - Provides File Browser, Universal IR, USB, Media, Games, Settings, and Power Off entries. USB groups USB Storage, USB Keyboard, BadUSB, Autoclicker, and USB Gamepad. Media groups Music, Image Viewer, Starfield, Spiro, and Cube. Games groups Emulators and Ports; Ports groups Pong, Tetris, Arkanoid, Flappy Bird, Labyrinth, T-Rex Runner, DOOM shareware, Chip's Challenge, Scorched Earth, Pipe Dream, Cave Story, Sokoban, and Jazz Jackrabbit shareware.
- **SD-card file browser** - Browses folders, hides dot/hidden entries, sorts directories before files, and opens supported files with the matching tool. Registered file types are `.gb`, `.gbc`, `.nes`, `.hex`, `.arduboy`, `.ir`, `.badusb`, `.rtttl`, `.txt`, `.wav`, `.dci`, `.dca`, `.jpg`, `.jpeg`, and `.bmp`.
- **Categorized app menus** - Launches resident tools and standalone `/APPS/*.DC32` apps from USB, Media, and Games categories. Standalone SD apps include Pong, Tetris, Arkanoid, Flappy Bird, Labyrinth, T-Rex Runner, DOOM shareware, Chip's Challenge, Scorched Earth, Pipe Dream, Cave Story, Sokoban, Jazz Jackrabbit shareware, Starfield, Spiro, and Cube.
- **Image Viewer** - Opens badge-native `.dci` stills and `.dca` animations plus direct `.jpg`, `.jpeg`, and uncompressed `.bmp` still images from `/IMAGES` or the browser.
- **Game Boy and Game Boy Color emulation** - Runs uGB on badge hardware, including cartridge metadata parsing, mapper support, Game Boy Color hardware presentation for compatible cartridges, flash-backed save state, SD save import/export, per-console display speed selection, rotation, optional upscaling, and Game Boy palette selection.
- **NES emulation** - Loads `.nes` files from SD, validates iNES headers, supports mappers 0, 1, 2, 3, 4, and 7, detects NTSC/PAL/Dendy timing, supports 8 KiB save RAM, and uses the emulator menu, save, per-console speed, rotation, and upscaling settings paths.
- **Arduboy compatibility runtime** - Loads classic Arduboy `.hex` files and `.arduboy` packages from SD, runs them through the default Ardens-derived ATmega32u4 runtime with SSD1306, button, and EEPROM handling, renders the 128x64 monochrome display, and persists the 1 KiB EEPROM through the existing save flow. Arduboy FX flash/cart packages are not supported yet. Arduboy audio is disabled for v1 and no speaker-pin callbacks, synthesis, mixers, tone queues, or PWM updates are run by the Arduboy runtime.
- **Game loading and saves** - Loads games from console-specific `/ROMS/AB`, `/ROMS/GB`, `/ROMS/GBC`, and `/ROMS/NES` picker roots or from the browser, confirms ROM metadata before flashing/loading, stores the selected game path/runtime in flash, imports saves from matching `/SAVE/AB`, `/SAVE/GB`, `/SAVE/GBC`, and `/SAVE/NES` folders, and exports the current save with detected emulator titles where available.
- **Universal IR** - Supports Flipper-style `.ir` signal/library files and the older `DC32IR1` format. Built-in universal remote categories look for Momentum universal IR files in `/IR`; browser actions can send a selected button, Power, or Mute. The IR sender supports raw records plus parsed NEC/NECext/NEC42/Samsung32/RC5/RC6/SIRC variants with bounded repeat, carrier, and raw duration validation.
- **BadUSB** - Runs scripts from `/BADUSB` or from the browser using the badge as an on-demand USB HID keyboard/mouse/consumer-control device. Supported commands include `REM`, `DELAY`, `DEFAULT_DELAY`, `STRING`, `STRINGLN`, `STRING_DELAY`, `DEFAULT_STRING_DELAY`, `HOLD`, `RELEASE`, `ALTCHAR`, `ALTSTRING`/`ALTCODE`, `SYSRQ`, `GLOBE`, `WAIT_FOR_BUTTON_PRESS`, `REPEAT`, key chords, Flipper-style `MEDIA`, mouse click/move/scroll commands, and optional first-line USB VID/PID/product overrides with `ID`. Default BadUSB VID/PID/manufacturer/product values are editable in Settings > USB.
- **USB utility apps** - Autoclicker exposes a configurable USB mouse clicker with button and Clicks /s settings. USB Gamepad offers PS4/DualShock 4 HID and Xbox 360/XUSB-style profiles so Steam can identify the badge as a known controller while mapping the badge D-pad, A, B, Select, Start, and FN/Home buttons. PS4 mode also maps the badge touchscreen to the DualShock 4 touchpad, with lower-left and lower-right screen zones acting as touchpad click zones. Hold FN/Home to exit the app; the hardware reset button is not firmware-readable as a controller input. Host rumble feedback drives the badge LEDs while the app is active, and controller ping feedback chirps the speaker.
- **USB Storage** - Exposes the full microSD card to a host computer as a standalone USB Mass Storage device. Eject or unmount from the host before leaving the tool; other SD-card tools are unavailable while storage mode is active.
- **Music player** - Plays `.rtttl`, RTTTL `.txt`, and PCM `.wav` files from `/MUSIC` or from the browser, with folder navigation, progress display, play/pause, previous/next, per-track loop, and persistent volume.
- **LED controls** - Drives the badge WS2812 LEDs with off, all-on, rainbow, pulse, traveling dot, random, rear-on, front-on, and reactive input patterns. Color modes include custom RGB, rainbow, flame, and random, with speed and brightness controls.
- **Settings** - Persists Emulator, LED, Screen, USB, and Music settings in QSPI flash. Emulator settings include per-console upscaling and display speed (`50%`, `100%`, `150%`, `200%`), plus Game Boy palette selection. Screen settings include rotation and brightness. LED settings include pattern, color, custom RGB, speed, and brightness.
- **Hardware self-test hooks** - Contains a badge self-test path for display, buttons, touch, RTC, IMU/ADC readings, LEDs, and power sequencing when the self-test trigger condition is present.

## Comparison with stock firmware

The comparison below uses the public [DEFCON-32-BadgeFirmware archive](https://github.com/jaku/DEFCON-32-BadgeFirmware), whose [firmware note](https://raw.githubusercontent.com/jaku/DEFCON-32-BadgeFirmware/main/firmware/readme.md) identifies the images as official firmware posted with DmitryGR's permission. The baseline used here is the stock image in [firmware/1.6](https://github.com/jaku/DEFCON-32-BadgeFirmware/tree/main/firmware/1.6).

| Capability | Stock official firmware | `DC32-cfw` |
| ---------- | ----------------------- | ---------- |
| Original uGB badge game runtime | Yes | Yes |
| Emulator settings and power off | Yes | Yes |
| SD file browser | No | Yes |
| Game Boy ROM loading from SD | Limited/original flow | Yes |
| NES ROM loading from SD | No | Yes |
| Classic Arduboy loading from SD | No | Yes |
| USB microSD storage mode | No | Yes |
| BadUSB scripting | No | Yes |
| Autoclicker and USB Gamepad tools | No | Yes |
| RTTTL/WAV music player | No | Yes |
| Universal IR tools | No | Yes |
| Expanded LED pattern/color settings | No | Yes |
| Safe tool-shell recovery after crashes/resets | No | Yes |

Stock firmware is still the right choice if you want the original badge experience with the official game image. `DC32-cfw` is aimed at using the badge as a post-con tool platform: loading ROMs, browsing SD-card content, launching SD app binaries, replaying IR files, running HID scripts, playing RTTTL/WAV files, and experimenting with the badge hardware.

## SD card layout

The firmware can browse the full card, but the menu tools look in these conventional folders first:

| Path | Used by |
| ---- | ------- |
| `/ROMS` | Parent folder for console-specific game picker roots. |
| `/SAVE` | Parent folder for console-specific save folders: `/SAVE/AB`, `/SAVE/GB`, `/SAVE/GBC`, and `/SAVE/NES`. Save files use detected emulator titles where available, falling back to cleaned ROM filenames. |
| `/APPS` | SD-loaded app binaries built by this repo, including emulators, tools, `doom.DC32` plus `doom1.whx`, `chips.DC32` plus `chips-tworld.pak`, `scorch.DC32` plus `scorch-xscorch.pak`, `pipe.DC32` plus `pipe-pipedreamer.pak`, `cave.DC32` plus user-built `cave.pak`, `sokoban.DC32` plus `sokoban-xsokoban.pak`, and standalone apps such as Pong, Tetris, Arkanoid, Flappy Bird, Labyrinth, T-Rex Runner, Starfield, Spiro, and Cube. |
| `/IR` | Universal IR files from Momentum Firmware's universal remote assets, plus optional legacy `POWER.IR`. |
| `/BADUSB` | BadUSB script picker for `.txt` and `.badusb` files. |
| `/MUSIC` | Music picker for `.rtttl`, RTTTL `.txt`, and PCM `.wav` files. Large generated music folders are split into alphabetic range subfolders so the badge can list them reliably. |
| `/IMAGES` | Image viewer files. `.jpg`, `.jpeg`, uncompressed `.bmp`, `.dci`, and `.dca` are supported; run `/IMAGES/image_converter.py` on a PC for badge-native stills and animations. |

In-game save confirmations update the emulator's battery-backed RAM first. When the emulator menu opens or gameplay exits, the firmware copies that RAM to the QSPI save cache. SD-card export happens from safe UI paths such as selecting another game or leaving the emulator, so gameplay is not interrupted by FAT writes. The firmware looks for the canonical `.sav` name inside the matching console save folder, with an 8.3 fallback only when long filename creation fails.

Release builds include two optional SD-card bundles. Extract `SD-apps.zip` to the SD card root so `/APPS/*.DC32` is available to the resident firmware shell, then extract `SD-assets.zip` for starter content folders such as `IR/`, `BADUSB/`, `MUSIC/`, `ROMS/`, and `IMAGES/`. `SD-assets.zip` is assembled at workflow time from upstream GitHub repositories and records the exact source URLs, branches, commits, and copied paths in `SOURCES.md`. `SD-apps.zip` records the built app hashes in its own `SOURCES.md`.

## SD asset credits

`SD-assets.zip` fetches external assets from these upstream projects at workflow time:

| SD path | Source |
| ------- | ------ |
| `/IR` | [Next-Flip/Momentum-Firmware](https://github.com/Next-Flip/Momentum-Firmware), `applications/main/infrared/resources/infrared/assets` |
| `/BADUSB` | [UberGuidoZ/Flipper](https://github.com/UberGuidoZ/Flipper), `BadUSB` |
| `/MUSIC` | [neverfa11ing/FlipperMusicRTTTL](https://github.com/neverfa11ing/FlipperMusicRTTTL) |
| `/ROMS/AB` | [eried/ArduboyCollection](https://github.com/eried/ArduboyCollection), genre folders with flattened `.hex` files only. |
| `/ROMS/GB`, `/ROMS/GBC`, `/ROMS/NES` | Folders for user-provided ROMs, each with a `README.txt` placeholder; add only files you can lawfully use and redistribute. |
| `/IMAGES` | Direct JPEG/BMP still-image support plus local `image_converter.py` helper for creating `.dci` still images and `.dca` animations from user-provided images. |

Credit and licensing for bundled external assets remain with their upstream projects.

## Repository layout

| Path | Description |
| ---- | ----------- |
| `CMakeLists.txt` | Primary firmware build used by CI; builds ELF, BIN, and UF2 outputs. |
| `.github/workflows/build.yml` | GitHub Actions firmware build and release artifact packaging. |
| `src/main_rp2350_defcon.c` | Firmware entry point, clocks, GPIO, display, IRDA, RTC, LEDs, and tool shell startup. |
| `src/ui.c` | On-device UI, tool shell, file browser, game selection, settings, SD app launchers, and boot recovery screens. |
| `src/gb.c`, `src/gbCore.h`, `src/gbC.c` | Game Boy execution loop, CPU helpers, and core emulator integration. |
| `src/mappersC.c`, `src/mbc.c` | Game Boy cartridge mapper implementations and metadata parsing. |
| `src/nes/` | InfoNES-derived NES runtime and mapper code used by the current CMake build. |
| `src/arduboy/` | Arduboy HEX/package parsing, default Ardens runtime, experimental simavr/hybrid runtimes, SSD1306/input glue, presenter, diagnostics, and EEPROM save bridge. |
| `third_party/ardens/` | Vendored Ardens CPU/peripheral core trimmed for the default embedded Arduboy runtime. |
| `third_party/simavr/` | Vendored simavr core and SSD1306 virtual display pieces kept for the experimental simavr Arduboy runtime. |
| `src/dispDefcon.c` | LCD driver, framebuffer, PIO program loading, DMA management, brightness, and framerate handling. |
| `src/sd*.c`, `src/fatfs.c` | SD-card and FAT filesystem integration. |
| `src/dcApp.c`, `src/dcAppDraw.c`, `src/apps/` | Resident SD app loader, shared SD app drawing helpers, and app entry wrappers for emulators, tools, and standalone apps. The loader supports both small app-cache `.DC32` images and flagged large-XIP app images staged at `QSPI_ROM_START`. |
| `src/apps/port/` | Shared support for faithful source-derived ports: scratch heap, RGB332/paletted renderer bridge, FAT pack reads, save helpers, and center-button exit handling. |
| `src/apps/trex/` | Wayou/Chromium T-Rex Runner port with generated original sprite assets, source-matched physics and obstacle timing, night mode, and `/SAVE/trex.sav` high-score persistence. |
| `src/apps/chips/` | Tile World-derived Chip's Challenge port locked to the Win 3.1/MS rules path, generated Tile World tile rendering, optional original tile-pack loading, `/APPS/chips.pak` user data loading, reset, saves, and clean center exit. |
| `src/apps/scorch/` | xscorch-derived Scorched Earth port with generated xscorch weapon tables, terrain deformation, AI turns, shop buys, saves, and clean center exit. |
| `src/apps/pipe/` | PipeDreamer-derived Pipe Dream port with queue, ooze source/meter, bombs, score carryover, fast-forward, saves, and clean center exit. |
| `src/apps/cave/` | Cave Story data-compatible loader/renderer using user-built `/APPS/cave.pak`, NXEngine/doukutsu PXM/Stage.dat formats, PBM tilesets, stage transitions, saves, and clean center exit. |
| `src/apps/sokoban/` | XSokoban-derived badge port with the 90 public-domain screens, move/push counts, undo/reset, saves, and pixmap-style rendering generated from vendored XSokoban assets. |
| `src/badUsb.c`, `src/usb*.c` | Shared TinyUSB device setup, USB Mass Storage, keyboard/mouse/media/gamepad HID, XUSB-style Xbox 360 gamepad, and the SD-loaded BadUSB interpreter. |
| `src/irRemote.c`, `src/pioIrdaSIR.c` | SD-loaded IR transmitter support and badge IRDA setup. |
| `src/rtttlPlayer.c`, `src/wavPlayer.c`, `src/audioPwm.c` | SD-loaded RTTTL/WAV parsing/playback and resident PWM tone/PCM audio output. |
| `src/badgeLeds.c`, `src/pioWS2812.c` | WS2812 LED rendering and PIO driver. |
| `src/settings.c`, `src/bootGuard.c`, `src/toolWorkspace.c` | Persistent settings, tool crash/reset recovery, and shared workspace allocation. |
| `tools/bin_to_uf2.py` | Converts the raw binary image to the RP2350 UF2 update file. |
| `tools/build_cave_pack.py` | Interactive packer for a user-supplied Cave Story freeware data directory; writes the `/APPS/cave.pak` payload without redistributing Cave Story data. |
| `tools/build_chips_pack.py` | Interactive packer for user-provided Win 3.1 `CHIPS.DAT` plus `CHIPS.EXE` embedded tile graphics; also accepts explicit Tile World-style, regular-grid, or `DC32CHIPTIL` graphics; writes `/APPS/chips.pak`. |
| `tools/build_period_assets.py` | Interactive/defaulted builder for redistributable period-port `.pak` files from checked-out upstream source trees when licensing permits redistribution. |
| `tools/build_trex_assets.py` | Validates the pinned Wayou sprite PNG and generates the grayscale T-Rex runtime asset. |
| `tools/build_tworld_assets.py` | Converts the vendored Tile World tile bitmap into compact badge RGB332 tiles. |
| `tools/build_xscorch_assets.py` | Converts vendored xscorch `weapons.def` into the compact badge Scorched Earth weapon table. |
| `tools/build_sd_zip.py` | Fetches upstream SD-card assets and packages `SD-assets.zip` and `SD-apps.zip`. |
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
- `build/apps/*.DC32` - SD-loaded app binaries for `/APPS`

The post-build step also runs `arm-none-eabi-size` and checks the firmware against the configured RAM budget.

## Flashing

### UF2 bootloader

Use `build/DC32-cfw.uf2` for normal updates. Put the badge into RP2350 USB bootloader mode, then copy `DC32-cfw.uf2` onto the mounted UF2 drive. The drive disconnects after the copy completes and the badge boots the new firmware.

GitHub Actions uploads `DC32-cfw.uf2` as a small `DC32-cfw-uf2` artifact. It also uploads `SD-assets.zip` as `DC32-cfw-sd-assets` and `SD-apps.zip` as `DC32-cfw-sd-apps`; release builds attach all three files.

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
- Large source-derived SD apps can opt into `DCAPP_IMAGE_FLAG_LARGE_XIP`, link at `QSPI_ROM_START`, and use up to `QSPI_ROM_SIZE_MAX`. Existing small `.DC32` apps keep the 256 KiB app-cache contract.
- OpenJazz remains a standard 256 KiB app at `0x10080000`. Before showing its menu it builds and validates a complete bundled-shareware graphics cache in the shared 3 MiB `QSPI_ROM_START` staging window, then seals that cache read-only for gameplay. DOOM and emulator ROM staging may overwrite this window, so the next OpenJazz launch automatically rebuilds it from `/APPS/openjazz.pak`.
- OpenJazz reserves fixed SRAM arenas for its 320×240 indexed canvas and normal/bonus level union, plus a 36 KiB auxiliary heap above resident firmware RAM. The firmware link fails if it grows into that auxiliary arena.
- OpenJazz uses badge-native controls: A selects in menus, B goes back, Start toggles quick pause, Select cycles weapons, and FN opens the in-game Continue/Save/Load/Setup/Quit menu. JJ1 save slots preserve the selected level and difficulty; loading a save restarts that level rather than restoring an exact mid-level position.
- T-Rex Runner, Chip's Challenge, Scorched Earth, Pipe Dream, Cave Story, and Sokoban now ship as source-derived/data-compatible ports. T-Rex uses the pinned Wayou/Chromium LDPI sprite and persists its high score in `/SAVE/trex.sav`; Cave Story freeware data and `CHIPS.DAT` remain user-provided and are not redistributed.
- The NES runtime intentionally enables only mappers 0, 1, 2, 3, 4, and 7 in `src/nes/InfoNES_Mapper.cpp`.
- Arduboy support defaults to an Ardens-derived runtime for classic ATmega32u4 `.hex` games. The runtime keeps the corrected landscape presenter, throttles EEPROM mirroring, polls the center-menu path inside long emulation batches, and disables Arduboy audio work entirely for v1.
- The ProjectABE-inspired hybrid compatibility runtime and the older Arduous/simavr-derived runtime are kept as experimental build options. Configure with `-DARDUBOY_HYBRID_EXPERIMENTAL=ON` or `-DARDUBOY_SIMAVR_EXPERIMENTAL=ON` to build one of them instead of the default Ardens path.
- Arduboy implementation references include [Ardens](https://github.com/tiberiusbrown/ardens), [Arduous](https://github.com/libretro/arduous), [simavr](https://github.com/buserror/simavr), [ProjectABE](https://github.com/felipemanga/ProjectABE), [Arduboy2](https://github.com/MLXXXp/Arduboy2), Arduboy Cloud, ESPboy Arduboy2, RP2040-GB, Pico-GB, the Pico SDK, and `arduino-pico`. The default Ardens-derived path is MIT licensed; the experimental simavr path still carries GPLv3 licensing implications if it is enabled.
- USB Storage exposes the raw microSD card as a read-write Mass Storage device when the dedicated tool is active. Eject/unmount from the host before exiting the tool or unplugging the badge.
- USB devices attach only while the matching USB tool is running. BadUSB uses keyboard, mouse, and consumer-control reports; Autoclicker uses mouse reports; USB Gamepad can start as either a DualShock 4-style HID controller or an Xbox 360/XUSB-style controller. Gamepad rumble feedback temporarily overrides the badge LEDs and is restored on app exit.

## Support and contributions

Issues and pull requests are welcome. Please include your toolchain version, badge hardware revision, the UF2 or commit you tested, and any SD-card layout details needed to reproduce the issue.

## Acknowledgements

- Firmware lead: Dmitry Grinberg (DmitryGR) - <https://dmitry.gr/>
- Hardware partner: Entropic Engineering - <https://www.entropicengineering.com/>
- Arduboy runtime references and dependencies: Ardens, Arduous, simavr, ProjectABE, Arduboy2, and the Arduboy community.
- T-Rex Runner reference: Wayou's BSD-licensed Chromium runner extraction at pinned commit `5455bfa408ec6b707c7300ff194b7390733a766d`.
