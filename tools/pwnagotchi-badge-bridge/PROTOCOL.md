# DC32 Pwnagotchi CDC protocol v1

The Pi is the USB host and the DC32 badge is a CDC device. The bridge opens no listener and talks to Pwnagotchi only through its loopback mobile web UI.

Every packet has a 20-byte little-endian header:

```
magic[4] = "PWN1" | version:u8 = 1 | type:u8 | reserved:u16 |
sequence:u32 | payload_length:u32 | crc32:u32
```

The CRC is standard IEEE CRC-32 of the payload. Payloads are capped at 1200 bytes.

| Type | Direction | Payload |
| --- | --- | --- |
| `HELLO` (1) | badge → Pi | display width/height as `<HH` |
| `READY` (2) | Pi → badge | empty |
| `PAGE` (3) | Pi → badge | `<BBBB`, then NUL-terminated title, detail, and visible item labels |
| `INPUT` (4) | badge → Pi | one discrete D-pad/button event |
| `STATUS` (5) | Pi → badge | short UTF-8 diagnostic |
| `FACE` (6) | Pi → badge | 128×64 1-bit packed face/UI frame, row-major MSB first |

`PAGE` contains `page_id`, flags, focused visible item, and item count. Page ID zero is the Face/Home page. The Pi owns focus, pagination, form state, history, and mobile-web authentication; the badge only renders the current page and emits controller events.
