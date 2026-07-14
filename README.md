# DC32 Custom Firmware

`DC32-cfw` is community firmware for the Raspberry Pi RP2350-based DEF CON 32 badge. It builds on Dmitry Grinberg's uGB badge firmware and turns the badge into an SD-driven tool, emulator, and game platform.

## Highlights

- SD file browser with app and media launchers
- Game Boy, Game Boy Color, NES, and Arduboy emulation
- Native ports: Pong, Tetris, Arkanoid, Flappy Bird, T-Rex Runner, DOOM, Chip's Challenge, Scorched Earth, Pipe Dream, Sokoban, Jazz Jackrabbit, and Sensible Soccer (YSoccer)
- USB Storage, BadUSB, Autoclicker, and USB Gamepad tools
- Infrared submenu with Universal IR, Flipper-style `.ir` support, and OpenLASIR Laser Tag
- ABC/RTTTL/MIDI music and JPEG/BMP/DCI/DCA image viewing
- Persistent saves and settings
- Universal Audio, Screen, and LED settings from emulator and port FN menus
- Battery percentage, idle dimming, paused static scanout, and bounded clock gating for longer runtime
- Deterministic warm-restart clock initialization and boot recovery

## Emulator compatibility

Arduboy emulation is playable but not yet 100% accurate. Many games run at or near full speed, while more demanding titles may run slower or exhibit timing or compatibility differences. Arduboy audio is not currently emulated.

## Release files

A release consists of:

- `DC32-cfw.uf2` — firmware for the RP2350 USB bootloader
- `SD-apps.zip` — `/APPS` binaries and required redistributable app data
- `SD-assets.zip` — optional starter IR, BadUSB, music, image, and ROM folders

Extract SD archives to the card root. Use the firmware and SD-apps bundle from the same build because SD apps are checked against the resident firmware contract.

## Controls and settings

The main menu groups File Browser, Infrared, USB, Media, Games, Settings, and Power Off. USB includes RaspyJack Remote: tether the badge to a Pi USB-host port, install the separate `tools/raspyjack-badge-bridge` bundle on the Pi, then use the D-pad, A, B, Start, and Select to control RaspyJack's existing UI; hold FN/center to leave the remote app locally. Infrared contains Universal IR and OpenLASIR Laser Tag. In Laser Tag, press A once to fire, Start to generate the paged sync QR, Select for the original-style settings menu, Down for statistics, and B to return. Settings include Sound, My Color, LED Brightness, Sync Reminder, At-Home Mode, and Tutorial Replay; Vibration is listed as unavailable because DC32 has no vibration motor. Block/Device identity remains hardware-bound and read-only. My Color changes visual hit feedback, not identity or team membership. While an emulator or native port is running, press FN to open the shared menu for Audio, Screen, and LED settings or to return to the main menu.

Audio provides universal mute and volume. Screen provides rotation and brightness. LED settings control pattern, color, speed, and brightness. Choices persist in QSPI flash.

OpenLASIR identifies a sender with two 8-bit values. The Block ID identifies a project or badge family; block 0 is the unregistered free-use block. The Device ID identifies one badge within that block. Laser Tag defaults to block 0 and derives a locked Device ID from the badge's flash UID. Because the protocol has only 256 device values per block, automatic IDs can collide and do not provide official leaderboard attribution. An official allocation can be provisioned at CMake configure time with `DC32_LASERTAG_BLOCK_ID` and `DC32_LASERTAG_DEVICE_ID`; use device `256` for UID derivation. These values are intentionally not runtime settings.

## SD card layout

| Path | Contents |
| --- | --- |
| `/APPS` | `.DC32` apps and app data from `SD-apps.zip` |
| `/ROMS/AB` | Arduboy `.hex` and `.arduboy` games |
| `/ROMS/GB` | Game Boy ROMs |
| `/ROMS/GBC` | Game Boy Color ROMs |
| `/ROMS/NES` | NES ROMs |
| `/SAVE/AB`, `/SAVE/GB`, `/SAVE/GBC`, `/SAVE/NES` | Emulator saves named from the ROM filename |
| `/SAVE/PORTS` | Native-port saves |
| `/SAVE/PORTS/LASERTAG.DAT` | Laser Tag identity, color, and persistent statistics |
| `/SAVE/PORTS/LASERTAG.SYNC` | Latest CRC-checked, unsigned, paged Laser Tag sync export; decode with `tools/lasertag_sync.py` |
| `/SAVE/PORTS/LASERTAG.URL` | Optional HTTP(S) QR URL prefix; the sync payload is appended to it |
| `/SAVE/PORTS/SOCCER` | Sensible Soccer competitions, teams, tactics, replays, highlights, and import/export files |
| `/IR` | Universal remote and Flipper-style IR files |
| `/BADUSB` | `.txt` and `.badusb` scripts |
| `/MUSIC` | ABC `.abc`, native MIDI `.mid`/`.midi`, RTTTL `.rtttl`, and RTTTL `.txt` files; Track focus selects ABC voices or MIDI format-1 tracks |
| `/IMAGES` | `.jpg`, `.jpeg`, uncompressed `.bmp`, and DC32-compatible `.gif` files |

ROMs and proprietary game data are not included. Add only files you are legally permitted to use.

## Building

Requirements:

- CMake 3.20+
- Ninja
- Python 3
- ARM embedded GCC and Newlib

```bash
git submodule update --init --recursive
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake \
  -DPICO_BOARD=defcon32_badge
cmake --build build
```

Build outputs are written to `build/`:

- `DC32-cfw.uf2`
- `DC32-cfw.bin`
- `apps/*.DC32`

Create the SD-app bundle with:

```bash
python tools/build_sd_zip.py \
  --apps-output build/SD-apps.zip \
  --apps-dir build/apps \
  --work-dir build/sd-apps-work
```

## Optional user tools

- `tools/image_converter.py` converts images and animations to browser-compatible `.gif` files with a DC32 playback payload, cropping each frame to its non-black content rectangle before scaling.
- `tools/build_chips_pack.py` creates `/APPS/chips.pak` from user-provided Win 3.1 Chip's Challenge data.
- `tools/build_openjazz_pack.py` creates `/APPS/openjazz.pak` from user-provided Jazz Jackrabbit data. The bundled shareware `JAZZ.ZIP` can also be converted by the badge on first launch.

Other scripts under `tools/` are required by the firmware build or release packaging.

## Flashing

Put the badge into RP2350 USB bootloader mode and copy `DC32-cfw.uf2` to the mounted UF2 drive. The badge disconnects and boots automatically after the copy completes.

Use USB Storage mode to update the microSD card, and eject or unmount it from the host before leaving that tool.

## Credits

- Original badge firmware: [Dmitry Grinberg](https://dmitry.gr/)
- Badge hardware collaboration: [Entropic Engineering](https://www.entropicengineering.com/)
- Emulator and port sources retain their upstream licenses and credits.
- Laser Tag follows the MIT-licensed OpenLASIR protocol by Dani Weidman, pinned in `third_party/openlasir`.
- Laser Tag QR generation uses Project Nayuki's MIT-licensed C library, pinned in `third_party/qrcodegen`.
- External SD assets remain owned and licensed by their respective projects.

This is an unofficial community firmware project. DEF CON and referenced game names belong to their respective owners.
