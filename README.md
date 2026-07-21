# DC32 Custom Firmware

`DC32-cfw` is unofficial community firmware for the Raspberry Pi RP2350-based DEF CON 32 badge. It builds on Dmitry Grinberg's uGB badge firmware and makes the badge an SD-driven tool, emulator, and game platform.

## Get started

1. Download the latest [release](../../releases/latest): `DC32-cfw.uf2`, `SD-apps.zip`, and, if wanted, `SD-assets.zip`.
2. Flash `DC32-cfw.uf2` while the badge is in RP2350 USB bootloader mode.
3. Extract the SD archive or archives to the root of the badge microSD card.

For the complete, current installation process, see the [Installation guide](https://github.com/neednotapply/DC32-cfw/wiki/Installation).

## What it includes

- Game Boy, Game Boy Color, NES, and Arduboy emulation
- Native games and ports, including DOOM, Tetris, Pong, Jazz Jackrabbit, and more
- Infrared, USB, image, music, BadUSB, and remote-control tools
- SD-card app discovery, saved settings, and per-app loading icons

The [Apps guide](https://github.com/neednotapply/DC32-cfw/wiki/Apps) has the complete built-in app catalog, controls, game-data requirements, and upstream-project references.

## Documentation

Detailed documentation lives on the [DC32-cfw Wiki](https://github.com/neednotapply/DC32-cfw/wiki):

- [Installation](https://github.com/neednotapply/DC32-cfw/wiki/Installation)
- [Apps](https://github.com/neednotapply/DC32-cfw/wiki/Apps)
- [Features and controls](https://github.com/neednotapply/DC32-cfw/wiki/Features-and-Controls)
- [Media and storage](https://github.com/neednotapply/DC32-cfw/wiki/Media-and-Storage)
- [Building firmware and apps](https://github.com/neednotapply/DC32-cfw/wiki/Building-Firmware-and-Apps)
- [SDK v1 app compatibility](https://github.com/neednotapply/DC32-cfw/wiki/SDK-v1-App-Compatibility)

## Legal and safety

- This is an unofficial community project. It is not affiliated with or endorsed by DEF CON, Raspberry Pi, or the owners of referenced games and software.
- ROMs and commercial game data are not included. Only add content you own or are otherwise legally permitted to use.
- Ports, emulators, and bundled components retain their respective upstream licenses and notices. See their source material and the included release documentation for details.
- BadUSB and USB automation features can control a connected computer. Use them only on systems you own or are explicitly authorized to test.
- DEF CON and third-party product names are trademarks of their respective owners.

## Credits

- Original badge firmware: [Dmitry Grinberg](https://dmitry.gr/)
- Badge hardware collaboration: [Entropic Engineering](https://www.entropicengineering.com/)
- Community contributors and the upstream projects credited in the [Apps guide](https://github.com/neednotapply/DC32-cfw/wiki/Apps)
