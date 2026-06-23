# DC32 SD App Roadmap

This document tracks SD-loaded `/APPS` registry candidates. Existing small apps keep the original `.DC32` contract: 256 KiB app cache at `QSPI_APP_CACHE_START` plus the app RAM window. Source-derived faithful ports that cannot fit may set `DCAPP_IMAGE_FLAG_LARGE_XIP`, link at `QSPI_ROM_START`, and use up to `QSPI_ROM_SIZE_MAX`.

## Current Shipped Apps

| App | SD file | ID | Source reference | Status |
| --- | --- | ---: | --- | --- |
| Game Boy | `/APPS/gb.DC32` | 1 | uGB-derived runtime | Implemented |
| NES | `/APPS/nes.DC32` | 2 | InfoNES-derived runtime | Implemented |
| Arduboy | `/APPS/arduboy.DC32` | 3 | Ardens-derived runtime | Implemented |
| Universal IR | `/APPS/ir.DC32` | 100 | Local Flipper-style IR parser | Implemented |
| Image Viewer | `/APPS/image.DC32` | 101 | Local JPEG/BMP/DCI/DCA support | Implemented |
| Music | `/APPS/music.DC32` | 102 | Local RTTTL/WAV player | Implemented |
| BadUSB | `/APPS/badusb.DC32` | 103 | Local DuckyScript-style runner | Implemented |
| Autoclicker | `/APPS/autoclicker.DC32` | 104 | Local USB HID app | Implemented |
| USB Gamepad | `/APPS/gamepad.DC32` | 105 | Local USB HID/XInput-style app | Implemented |
| Pong | `/APPS/pong.DC32` | 200 | Picoware-inspired local app | Implemented |
| Tetris | `/APPS/tetris.DC32` | 201 | NullpoMino-derived Marathon/Line Race/Ultra port with Standard, Fast-B, and Nintendo-R rules plus persistent records | Implemented |
| Arkanoid | `/APPS/arkanoid.DC32` | 202 | Source-derived wkeeling/arkanoid port with five rounds, enemies, power-ups, original scaled sprites, and persistent high score | Implemented |
| Flappy Bird | `/APPS/flappy.DC32` | 203 | Picoware-inspired local app | Implemented |
| Labyrinth | `/APPS/labyrinth.DC32` | 204 | Picoware-inspired local app | Implemented |
| T-Rex Runner | `/APPS/trex.DC32` | 205 | Source-derived Wayou/Chromium runner port with original sprites and persistent high score | Implemented |
| DOOM | `/APPS/doom.DC32` plus `/APPS/doom1.whx` | 206 | `kilograham/rp2040-doom` DEF CON 32 release | Implemented prototype |
| Chip's Challenge | `/APPS/chips.DC32` plus `/APPS/chips-tworld.pak`; user-built `/APPS/chips.pak` for original levels | 207 | Tile World rules/rendering locked to the Win 3.1/MS rules path | Implemented |
| Scorched Earth | `/APPS/scorch.DC32` plus `/APPS/scorch-xscorch.pak` | 208 | xscorch weapon tables/assets with badge terrain/AI/shop loop | Implemented |
| Pipe Dream | `/APPS/pipe.DC32` plus `/APPS/pipe-pipedreamer.pak` | 209 | PipeDreamer logic/assets with JUCE replaced | Implemented |
| Cave Story | `/APPS/cave.DC32`; user-built `/APPS/cave.pak` | 210 | NXEngine/doukutsu PXM/Stage.dat/PBM-compatible loader plus NXEngine sprite metadata renderer | In progress; full object/AI/TSC port pending |
| Sokoban | `/APPS/sokoban.DC32` plus `/APPS/sokoban-xsokoban.pak` | 211 | XSokoban public-domain source/screens/pixmaps | Implemented |
| Starfield | `/APPS/starfield.DC32` | 220 | Picoware-inspired local app | Implemented |
| Spiro | `/APPS/spiro.DC32` | 221 | Picoware-inspired local app | Implemented |
| Cube | `/APPS/cube.DC32` | 222 | Picoware-inspired local app | Implemented |

## Faithful Period Ports

The earlier handmade period games were removed. IDs 207-211 are now occupied by source-derived or original-data-compatible ports. Cave Story freeware data and `CHIPS.DAT` are still user-provided and must not be redistributed.

| App | SD file | ID | Required basis | Data policy | Status |
| --- | --- | ---: | --- | --- | --- |
| Chip's Challenge | `/APPS/chips.DC32` plus `/APPS/chips-tworld.pak`; user-built `/APPS/chips.pak` | 207 | Tile World rules/rendering, Win 3.1/MS rules path only | User-provided `CHIPS.DAT`; redistributable Tile World assets packaged | Implemented |
| Scorched Earth | `/APPS/scorch.DC32` plus `/APPS/scorch-xscorch.pak` | 208 | xscorch weapon tables/assets, terrain deformation, AI, shop, turn flow | Redistributable xscorch assets only | Implemented |
| Pipe Dream | `/APPS/pipe.DC32` plus `/APPS/pipe-pipedreamer.pak` | 209 | PipeDreamer logic/assets with JUCE replaced | Redistributable PipeDreamer assets only | Implemented |
| Cave Story | `/APPS/cave.DC32` plus user-built `/APPS/cave.pak` | 210 | NXEngine-evo/doukutsu-rs PXM, Stage.dat, PBM, backdrop, and sprite metadata renderer | User-built pack from Cave Story data; no bundled Cave data | In progress; replace remaining hand-written gameplay with NXEngine object/AI/TSC behavior |
| Sokoban | `/APPS/sokoban.DC32` plus `/APPS/sokoban-xsokoban.pak` | 211 | XSokoban ANSI C logic, 90 levels, pixmap rendering | Redistributable XSokoban source/assets only | Implemented |

## Port Runtime Requirements

- Use `src/apps/port/` for shared scratch-heap allocation, FAT-backed pack reads, `/SAVE/<app>.sav` helpers, RGB332/paletted presentation, and center-button exit.
- Missing required user data must show an in-app diagnostic naming the exact SD path, such as `/APPS/cave.pak` or `/APPS/chips.pak`.
- Do not redistribute Cave Story freeware data, `CHIPS.DAT`, or proprietary original graphics with unclear licensing.
- Music can be deferred. Buzzer/PWM SFX are acceptable when the original game used minimal PC speaker-style sound.

## Host Tools

- `python tools/build_cave_pack.py` prompts for the Cave Story install/data directory and output path. It accepts a loose `data/Stage.dat` when present, or extracts the original stage table from `Doukutsu.exe` and writes the generated `Stage.dat` entry into `cave.pak`.
- `python tools/build_chips_pack.py` prompts for `CHIPS.DAT` or an extracted Win 3.1 folder and output path. When `CHIPS.EXE` is beside `CHIPS.DAT`, it extracts the original embedded Windows tile graphics automatically; explicit tile input still accepts Tile World-style sheets, regular-grid image sheets, or `DC32CHIPTIL` packs.
- `tools/build_tworld_assets.py --source-root third_party/tworld --output-c <out.c> --output-h <out.h>`
- `tools/build_xscorch_assets.py --source-root third_party/xscorch --output-c <out.c> --output-h <out.h>`
- `python tools/build_period_assets.py` prompts/defaults to `third_party` and `build/apps`; flags remain available for automation.

## Acceptance Checklist

- `cmake --build build` produces all existing apps and any newly completed faithful period app.
- DCAPP static tests cover header CRCs, RAM descriptors, small/large load regions, and package membership.
- `SD-apps.zip` includes only accepted `.DC32` period apps plus redistributable packs and README entries for missing user data.
- Each accepted port survives missing asset pack, level completion, reset, save write failure, and center-button exit.
- Cave: load title/first map, scripted room transition, save/reload.
- Chip's Challenge: parse `CHIPS.DAT`, complete/reset a level without crash, and render inventory with tile icons from the active tile pack.
- Sokoban: validate all 90 levels, complete/reset/undo without corrupting state.
- Pipe Dream: complete a level, spill/fail, use bombs, fast-forward.
- Scorched Earth: complete a round, buy/fire multiple weapon classes, destroy terrain, AI turn transition.
