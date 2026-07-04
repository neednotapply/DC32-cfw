# OpenLASIR source reference

The DC32 Laser Tag app implements OpenLASIR mode 0 from:

- Repository: https://github.com/danielweidman/OpenLASIR
- Branch: `master`
- Commit: `414504887e761fe0f3e1d3251567f0c29cdcec13`
- License: MIT; see `LICENSE` in this directory.

The implementation retains the upstream 38 kHz NEC-derived timings, packet
layout, color assignments, transmit limits, and post-tag lockout. It replaces
the upstream Arduino/MicroPython hardware adapters with DC32 PIO capture and
the badge's built-in IrDA transceiver.

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

Normal play uses a hardware-derived, read-only identity. My Color remains a
user setting, matching the original badge, but does not change identity or
score attribution. Firing is edge-triggered, statistics cannot be reset, and
the save CRC is bound to the flash UID. OpenLASIR colors are hit feedback;
the public game uses an individual leaderboard rather than teams. Block 0
remains free-use and is intentionally shown as ineligible for leaderboard
credit until DC32 receives an allocation.
