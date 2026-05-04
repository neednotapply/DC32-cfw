# DC32 Badge Firmware

Embedded C firmware for the DEF CON 32 badge (RP2350 / Raspberry Pi Pico 2).

## Build

```
cd src && make
```

Produces `uGB.bin` (flash to badge via UF2 or SWD). Requires `gcc-arm-embedded` and `gnumake` (provided via Nix).

## Architecture

| File | Role |
|---|---|
| `main_rp2350_defcon.c` | Entry point, `micromain` game loop |
| `ui.c` | All UI: filebrowser, IR spam/remote, BadUSB, music player (4200+ lines) |
| `usbHid.c` | USB HID device driver (keyboard/mouse/consumer reports) |
| `badUsb.c` | BadUSB script engine |
| `irRemote.c` | IR GPIO bit-bang transmit (mark/space) |
| `pioIrdaSIR.c` | IrDA SIR PIO driver (TX + RX) |
| `memMap.h` | Memory layout constants |
| `Makefile` | Build system |

### Memory layout (key addresses)

| Symbol | Value | Notes |
|---|---|---|
| `CART_RAM_ADDR_IN_RAM` | `0x20000000` | Start of 64 KB cart RAM region |
| `QSPI_RAM_SIZE_MAX` | `0x10000` (64 KB) | Total cart RAM |
| `IR_LINE_BUF_SZ` | `32768` | IR file line read buffer (first 32 KB of cart RAM) |

IR button-list option storage uses the **second 32 KB** of cart RAM (`CART_RAM_ADDR_IN_RAM + IR_LINE_BUF_SZ`) to avoid overlapping with the line buffer.

## Bug Fixes Applied

### 1 — Raw IR codes never transmitted (`uiPrvIrSendCodeLine`, `ui.c`)
Inside the `while(1)` duration-parse loop, two lines (`*malformedP = true; return false;`) fired after the first value was sent, treating every multi-duration raw code as malformed. **Fix:** removed those two lines so the loop continues until the string is exhausted.

### 2 — IR Remote freezes / crashes (`uiPrvIrListButtons`, `ui.c`)
The IR button-option list and the 32 KB file-line buffer both started at `CART_RAM_ADDR_IN_RAM`. Every line read overwrote the start of the growing option list, corrupting `next` pointers and causing a crash when the menu iterated the list. **Fix:** option list now starts at `CART_RAM_ADDR_IN_RAM + IR_LINE_BUF_SZ` (32 KB offset), giving each structure its own non-overlapping half of cart RAM.

### 3 — BadUSB freezes for ~2 s when USB not enumerated (`usbHidEnd`, `usbHid.c`)
`usbHidEnd` unconditionally called `usbHidReleaseAll`, which issues three `usbHidPrvSendReport` calls each spinning up to 500 ms waiting for USB ready. When the host never enumerated the device, this wasted ~1.5–2 s after the already-5-second `badUsbPrvWaitReady` timeout. **Fix:** `usbHidReleaseAll` is now skipped when `mConfigured == 0`.

### 4 — B-button sticky after IR blast (`uiPrvIrPowerBlast` / `uiPrvIrMuteBlast`, `ui.c`)
After blast completion, the post-blast dialog could immediately consume a still-held B-press (from the last cancel check inside the transmit loop), silently dismissing the dialog and leaving the IR Tools menu waiting for input with no indication to the user. **Fix:** `uiPrvWaitKeysReleased()` is called before each post-blast dialog in both blast functions.
