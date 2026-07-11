# RaspyJack USB badge bridge

This standalone bundle connects the DC32 **RaspyJack Remote** app to a running RaspyJack installation. It does not modify, restart, or expose RaspyJack: the bridge reads its existing native-aspect `/dev/shm/raspyjack_last.jpg` mirror at 10 FPS and sends existing virtual-button messages to `/dev/shm/rj_input.sock`.

The intended headless RaspyJack configuration is the original **ST7735 1.44-inch, 128×128** display type. Its square mirror is centered on the badge without stretching; the unused space appears as side margins.

## Install

1. Copy this directory to the Pi, then run `sudo ./install.sh`.
2. Put `raspyjack.DC32` in the badge's `/APPS` folder.
3. Connect the badge to the Pi's USB-host/OTG port, open **USB → RaspyJack Remote**, and wait for the first frame.

The service discovers the serial device through `/dev/serial/by-id/` and falls back to `/dev/ttyACM0`. Override paths or disable that fallback in `/etc/default/raspyjack-badge-bridge` if needed. The default mirror preserves the original RaspyJack display aspect ratio; do not use `raspyjack_cardputer.jpg` unless you specifically want its stretched 16:9 cardputer layout.

Current bridge and badge versions negotiate frame acknowledgements. The bridge holds at most one transmitted frame, replaces any unsent frame with the newest source image, and continues polling USB input on a 5 ms tick. When Pillow is available (it is already used by RaspyJack), the bridge sends a direct 128x128 RGB565 frame instead of JPEG decoding on the badge; later frames use 8x8 changed-tile updates when smaller. Set `VIDEO_MODE=jpeg` in `/etc/default/raspyjack-badge-bridge` to force the original transport. An older badge app remains supported with JPEG and no-ACK streaming behavior.

The bridge prints a five-second diagnostic summary with JPEG/RGB565/tile counts, bytes, replaced frames, and mean transport/presentation latency. The default remains fully independent of RaspyJack: it does not change its process, configuration, service, or source files.

Controls: D-pad sends directions; A sends OK; B sends KEY3/back; Start sends KEY1; Select sends KEY2. Hold the badge FN/center button for one second to return to the badge without sending a remote input.

## Safety and lifecycle

The service uses no network transport and is sandboxed with `PrivateNetwork=yes`. It retries independently when RaspyJack, its frame/socket files, or the USB cable are unavailable. `sudo ./uninstall.sh` removes only this bundle and unit; add `--purge` to remove its retained configuration.

See [PROTOCOL.md](PROTOCOL.md) for the exact CDC wire format.

## Optional source-rate increase

The normal RaspyJack producer loop has a 100 ms sleep, which limits every consumer to roughly 10 new images per second. [optional-raspyjack-mirror-rate.patch](optional-raspyjack-mirror-rate.patch) makes that delay environment-configurable but is **not** installed or applied by this bundle. Apply it only if you choose to run the mirror faster, then set `RJ_FRAME_FPS=20` and `RJ_DISPLAY_LOOP_INTERVAL=0.05` in your RaspyJack service environment. Remove those environment entries or revert the one-file patch to return to normal behavior. Start at 20 FPS and use the bridge diagnostics to decide whether 30 FPS is worthwhile.
