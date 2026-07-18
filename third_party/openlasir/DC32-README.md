# OpenLASIR source reference

The DC32 Laser Tag app implements OpenLASIR mode 0 from:

- Repository: https://github.com/danielweidman/OpenLASIR
- Branch: `master`
- Commit: `414504887e761fe0f3e1d3251567f0c29cdcec13`
- License: MIT; see `LICENSE` in this directory.

The implementation retains the upstream 38 kHz NEC-derived timings and packet
layout. It replaces the upstream Arduino/MicroPython hardware adapters with
DC32 PIO capture and the badge's built-in IrDA transceiver.

The decoder and automatic Device ID selection were also cross-checked against
Arduino-IRremote's OpenLASIR implementation at commit
`f2f8de414b8e2724c578377e651b6e5c1fefb7b2`. In particular, DC32 applies its
NEC/OpenLASIR command-complement disambiguation rule so ordinary NEC traffic is
not counted as a hit, and auto IDs avoid mode-0 values that would collide.

## Fidelity boundary

The 2025 Laser* Tag Badge used a separate proprietary game protocol. Its
firmware and leaderboard authentication format are not public, so DC32 does
not claim to reproduce that private implementation. This app targets the
documented OpenLASIR mode-0 behavior intended for the 2026 badge and the
announced 2025 firmware update.

DC32 exposes a diagnostic-first OpenLASIR Lab. It listens for every valid
OpenLASIR mode and standard NEC frame, logs a bounded packet history, and
provides explicit, manually confirmed test sends. Its Mj0ln1r profile matches
the public block 1/device 3 allocation; its Defcon33 vectors are retained only
as legacy NEC probes. The Custom OpenLASIR profile stores editable block,
device, mode, and data/color fields locally. A block-0 identity remains
free-use and does not receive leaderboard attribution until DC32 has an
allocation.

`LASERTAG.SYNC` and the QR view are **local unsigned DC32 diagnostic exports**.
They are not the Laser* Tag Badge QR format and are never submitted by this
firmware. A real official QR sample is required before claiming official sync
compatibility.
