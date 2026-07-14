# Flipper Remote USB bridge (optional fallback)

## Direct connection

1. Copy `flipper.DC32` to the badge `/APPS` directory.
2. Use a data-capable USB-C cable or OTG adapter between the badge and Flipper.
3. Open **USB -> Flipper Remote** on the badge.

The badge remains at **Connect Flipper USB** when the direct cable does not deliver 5 V VBUS or is charge-only. Use externally powered USB-C OTG hardware in that case. If the Flipper is locked, enable USB RPC access while locked in its firmware settings before connecting.

`flipper.DC32` now talks to a Flipper directly in USB-host mode. This standalone Linux/Pi bundle is retained as an optional fallback for badge hardware or cables that cannot deliver 5 V VBUS to the Flipper. It changes neither the Flipper firmware nor qFlipper. The host needs one USB connection to the badge and one to the Flipper; qFlipper cannot use the same Flipper serial interface concurrently.

The badge shows the native 128×64 QFlipper screen stream as its live screenshot. D-pad, A, and B map to the six Flipper navigation keys. Start sends the official desktop unlock RPC. Select pauses or resumes screen streaming, and holding the badge FN/center button returns to the badge.

## Optional Linux/Pi bridge install

1. Copy this directory to the Linux USB host and run `sudo ./install.sh`.
2. Put `flipper.DC32` in the badge `/APPS` directory.
3. Connect both USB devices, open **USB → Flipper Remote**, and wait for the QFlipper RPC status.

The service selects stable `/dev/serial/by-id/` links first. Configure explicit paths, globs, or fallbacks in `/etc/default/flipper-badge-bridge` only if discovery does not match the local device names. `sudo ./uninstall.sh` removes only this bridge and service; pass `--purge` to also remove its retained configuration.

The bridge uses the official CLI transition (`start_rpc_session`) and the official QFlipper protobuf fields for screen streaming, input, desktop status, and unlock. It requires RPC protocol 0.16 or newer because that is when the desktop API became available.

See [PROTOCOL.md](PROTOCOL.md) for the complete badge wire contract and QFlipper RPC subset.
