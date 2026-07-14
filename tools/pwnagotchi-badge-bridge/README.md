# Pwnagotchi USB badge bridge

This bundle makes the DC32 **Pwnagotchi Remote** app a controller-only reinterpretation of Jayofelony Pwnagotchi's local mobile web UI. It is intended for a Raspberry Pi 4 or 5 running Pwnagotchi.

The Pi is the USB host and powers the badge. Connect a **data-capable Pi USB host port** to the badge USB-C port, open **USB → Pwnagotchi Remote**, then press Start to open the adapted mobile UI. A Pi power-only USB-C connector cannot be used for this link.

## Controls

- D-pad moves between controls and adjusts toggles, choices, and numeric fields.
- A opens a page, toggles a checkbox, or submits an explicit action.
- B returns to the previous page.
- Start returns to the face/home screen.
- Select opens the Pwnagotchi Plugins page.
- Hold FN/center for one second to leave the remote on the badge.

The home page shows the current Pwnagotchi face without stretching its native aspect ratio. All mobile-web links and supported forms become badge pages, so installed plugins expand the navigation automatically; the Plugins page reparses its catalog every five seconds while open. Text values remain read-only and fields whose names imply passwords, tokens, API keys, or private keys are omitted entirely. The bridge preserves hidden values when it submits a form, but never sends them to the badge.

## Install

1. Copy this folder to the Pi and run `sudo ./install.sh`.
2. If the local Pwnagotchi web UI uses HTTP basic authentication, set `PWN_USER` and `PWN_PASSWORD` in `/etc/default/pwnagotchi-badge-bridge`; its permissions are `0600`.
3. Copy `pwnagotchi.DC32` to the badge `/APPS` folder.
4. Use a data cable from a Pi USB host port to the badge USB-C port.

The service contacts only `http://127.0.0.1:8080` by default and opens no TCP/UDP listener. Its systemd policy blocks non-loopback IP traffic. It needs `python3-pil` to stream the face; navigation and admin pages work without it.

`sudo ./uninstall.sh` removes the service and bundle. Add `--purge` to remove the retained credentials/configuration file too.
