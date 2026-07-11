# DC32 RaspyJack CDC protocol v1

Packets are a byte stream framed by a 20-byte little-endian header:

```
magic[4] = "RJC2" | version:u8 | type:u8 | flags:u16 |
sequence:u32 | payload_length:u32 | payload_crc32:u32 | payload
```

The CRC-32 is standard IEEE CRC-32 of the payload (zero for an empty payload). Receivers resynchronize by scanning for the magic and reject payloads over 64 KiB.

| Type | Direction | Payload |
| --- | --- | --- |
| `HELLO` (1) | badge to Pi | `<IHHI`: accepted frame limit, display width/height, capability mask |
| `READY` (2) | Pi to badge | Empty |
| `FRAME` (3) | Pi to badge | Complete baseline JPEG; compatibility fallback |
| `INPUT` (4) | badge to Pi | `button:u8`, `state:u8`; press is 1, release is 0 |
| `STATUS` (5) | Pi to badge | Short UTF-8 diagnostic text |
| `FRAME_ACK` (6) | badge to Pi | `<IB`: presented frame sequence and success (1) or rejected (0) |
| `FRAME_RGB565` (7) | Pi to badge | `<HH` width/height, then 128x128 RGB565 little-endian pixels |
| `FRAME_TILES` (8) | Pi to badge | `<HHBH` width/height, tile size (8), count; then `count` records of `tile_x:u8`, `tile_y:u8`, and 8x8 RGB565 pixels |

Buttons 1-8 are `UP`, `DOWN`, `LEFT`, `RIGHT`, `OK`, `KEY1`, `KEY2`, and `KEY3`.

`HELLO` capability bits are opt-in: bit 31 (`0x80000000`) enables `FRAME_ACK`, bit 30 (`0x40000000`) enables direct RGB565 frames, and bit 29 (`0x20000000`) enables tile deltas. A current bridge selects RGB565 only when both sides negotiate it and Pillow is available on the Pi; otherwise it retains the JPEG protocol. Tile frames are sent only after a full RGB565 frame was acknowledged.
