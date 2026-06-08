# DC32 SD App Roadmap

This document tracks candidates for the SD-loaded `/APPS` registry. The loader ABI remains the current `.DC32` contract unless a future app proves it cannot fit: 256 KiB image cache, about 80 KiB app RAM, and the existing static app gates.

## First Batch

| App | SD file | ID | Source reference | License status | Hardware blockers | RAM/flash risk | Status |
| --- | --- | ---: | --- | --- | --- | --- | --- |
| Pong | `/APPS/pong.DC32` | 200 | [jblanked/Picoware](https://github.com/jblanked/Picoware) | Picoware metadata: GPL-3.0; no Picoware source vendored in this implementation | None after DC32 framebuffer/buttons/timing layer | Low | Implemented as standalone app |
| Tetris | `/APPS/tetris.DC32` | 201 | [jblanked/Picoware](https://github.com/jblanked/Picoware) | Picoware metadata: GPL-3.0; no Picoware source vendored in this implementation | None after DC32 framebuffer/buttons/timing layer | Low | Implemented as standalone app |
| Arkanoid | `/APPS/arkanoid.DC32` | 202 | [jblanked/Picoware](https://github.com/jblanked/Picoware) | Picoware metadata: GPL-3.0; no Picoware source vendored in this implementation | None after DC32 framebuffer/buttons/timing layer | Low | Implemented as standalone app |
| Flappy Bird | `/APPS/flappy.DC32` | 203 | [jblanked/Picoware](https://github.com/jblanked/Picoware) | Picoware metadata: GPL-3.0; no Picoware source vendored in this implementation | None after DC32 framebuffer/buttons/timing layer; sprite-art pass is deferred | Low | Implemented as standalone app |
| Labyrinth | `/APPS/labyrinth.DC32` | 204 | [jblanked/Picoware](https://github.com/jblanked/Picoware) | Picoware metadata: GPL-3.0; no Picoware source vendored in this implementation | Badge accelerometer hook is not used yet; buttons drive the maze in this batch | Low | Implemented; maze reachability is statically checked |
| T-Rex Runner | `/APPS/trex.DC32` | 205 | [jblanked/Picoware](https://github.com/jblanked/Picoware) | Picoware metadata: GPL-3.0; no Picoware source vendored in this implementation | None after DC32 framebuffer/buttons/timing layer; sprite-art pass is deferred | Low | Implemented as standalone app |
| Starfield | `/APPS/starfield.DC32` | 220 | [jblanked/Picoware](https://github.com/jblanked/Picoware) | Picoware metadata: GPL-3.0; no Picoware source vendored in this implementation | None after DC32 framebuffer/buttons/timing layer | Low | Implemented as standalone app |
| Spiro | `/APPS/spiro.DC32` | 221 | [jblanked/Picoware](https://github.com/jblanked/Picoware) | Picoware metadata: GPL-3.0; no Picoware source vendored in this implementation | None after DC32 framebuffer/buttons/timing layer | Low | Implemented as standalone app |
| Cube | `/APPS/cube.DC32` | 222 | [jblanked/Picoware](https://github.com/jblanked/Picoware) | Picoware metadata: GPL-3.0; no Picoware source vendored in this implementation | None after DC32 framebuffer/buttons/timing layer | Low | Implemented as standalone app |
| Direct JPEG/BMP viewing | `/APPS/image.DC32` | 101 | [jblanked/Picoware](https://github.com/jblanked/Picoware) JPEG viewer reference; local picojpeg decoder | Local decoder source is vendored in `third_party/image_decoders/picojpeg`; converter path remains local | JPEG/BMP stills only; animated/heavier formats stay on converter path | Medium for large JPEG decode speed, low for app image size | Implemented |
| WAV playback | `/APPS/music.DC32` | 102 | [jblanked/Picoware](https://github.com/jblanked/Picoware) audio-player reference | Local PCM WAV implementation | Timer-buffered mono PWM on `PIN_SPQR`; no I2S/DAC dependency | Low | Implemented |
| MP3 playback | `/APPS/music.DC32` | 102 | [jblanked/Picoware](https://github.com/jblanked/Picoware), `minimp3` candidate | Needs license notice if vendored | Decoder CPU, image size, and scratch-memory gates must be proven | Medium/high | Deferred until WAV build size and scratch headroom are measured |

## Compatibility Layer Rules

- Standalone apps build as separate `.DC32` targets and dispatch by `DCAPP_RUNTIME_ID`.
- Shared services are limited to DC32 framebuffer/canvas drawing, badge buttons, timing, random/state, and small drawing primitives.
- Full-screen redraws are paced with the display frame counter and scanout-start wait used by emulator presenters.
- Do not add Arduino, MicroPython, WiFi, Bluetooth, keyboard, heap, PSRAM, I2S, HDMI/HSTX, PS/2, USB host, or board-specific display/input APIs to the Picoware-derived app layer.
- GPL-compatible vendoring is allowed only with proper notices. This first batch does not vendor Picoware source code.

## rh1tech / FRANK Candidates

Most FRANK ports target the same RP2350 family but appear to assume a stronger custom board profile than DC32: HDMI/HSTX video, PSRAM, PS/2 or USB host input, and I2S audio are common blockers. Treat repositories with undeclared GitHub license metadata as feasibility references until each repo license is inspected directly.

| Candidate | Source URL | License status | Likely blockers | RAM/flash risk | Status |
| --- | --- | --- | --- | --- | --- |
| frank-c64 | <https://github.com/rh1tech/frank-c64> | Inspect before vendoring | HDMI/HSTX video path, keyboard input, possible PSRAM | High | Later feasibility |
| frank-video | <https://github.com/rh1tech/frank-video> | Inspect before vendoring | Video pipeline likely tied to HSTX/HDMI | Medium/high | Later feasibility |
| frank-cabal | <https://github.com/rh1tech/frank-cabal> | Inspect before vendoring | Display/input/audio assumptions unknown; likely board-specific | Medium/high | Later feasibility |
| SpeccyP | <https://github.com/rh1tech/SpeccyP> | Inspect before vendoring | Keyboard input, display timing, possible PSRAM | High | Later feasibility |
| frank-doom | <https://github.com/rh1tech/frank-doom> | Inspect before vendoring | RAM, storage, input, audio, and display assumptions | Very high | Later feasibility |
| frank-heretic | <https://github.com/rh1tech/frank-heretic> | Inspect before vendoring | RAM, storage, input, audio, and display assumptions | Very high | Later feasibility |
| frank-idtech1 | <https://github.com/rh1tech/frank-idtech1> | Inspect before vendoring | Shared idtech base likely needs PSRAM/display/audio adaptation | Very high | Later feasibility |
| frank-msx | <https://github.com/rh1tech/frank-msx> | Inspect before vendoring | Keyboard input, display path, possible PSRAM | High | Later feasibility |
| frank-snes | <https://github.com/rh1tech/frank-snes> | Inspect before vendoring | CPU/RAM budget, audio, controller mapping, display | Very high | Later feasibility |
| frank-wolf3d | <https://github.com/rh1tech/frank-wolf3d> | Inspect before vendoring | RAM/storage/input/audio/display assumptions | Very high | Later feasibility |
| frank-quest | <https://github.com/rh1tech/frank-quest> | Inspect before vendoring | Engine/input/display assumptions unknown | Medium/high | Later feasibility |
| frank-386 | <https://github.com/rh1tech/frank-386> | Inspect before vendoring | x86 emulation needs memory and keyboard/display model | Very high | Later feasibility |
| frank-apple | <https://github.com/rh1tech/frank-apple> | Inspect before vendoring | Keyboard input, display timing, possible PSRAM | High | Later feasibility |
| frank-duke3d | <https://github.com/rh1tech/frank-duke3d> | Inspect before vendoring | RAM/storage/input/audio/display assumptions | Very high | Later feasibility |
| frank-genesis | <https://github.com/rh1tech/frank-genesis> | Inspect before vendoring | CPU/RAM budget, audio, controller mapping, display | Very high | Later feasibility |

## Acceptance Checklist

- Firmware and all `.DC32` app binaries build.
- `dcapp_static_test.py` validates each app id, filename, image size, CRC, RAM descriptors, and app scratch headroom.
- `build_sd_zip_test.py` validates that `SD-apps.zip` includes every standalone app binary.
- Static Picoware app checks confirm display pacing, Labyrinth reachability, and no heap, Arduino runtime, MicroPython, WiFi/Bluetooth, keyboard, or board-specific display/input APIs in the standalone Picoware-derived layer.
- Image tests cover `.dci`, `.dca`, `.jpg`, `.jpeg`, and `.bmp`; music tests cover RTTTL, WAV, and non-advertised MP3 deferral.
- Manual badge pass: every USB, Media, and Games category entry launches independently, exits cleanly back to its parent category, renders correctly, and existing file handlers still work.
