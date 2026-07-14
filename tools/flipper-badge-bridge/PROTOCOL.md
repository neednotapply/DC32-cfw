# Flipper Remote protocols

## Badge CDC (`FRC1`)

The DC32 and Linux bridge exchange a 20-byte little-endian envelope:

```
magic[4] = "FRC1" | version:u8 | type:u8 | flags:u16 |
sequence:u32 | payload_length:u32 | payload_crc32:u32 | payload
```

The CRC is IEEE CRC-32. Receivers resynchronize on the magic and reject payloads over 1,100 bytes.

| Type | Direction | Payload |
| --- | --- | --- |
| `HELLO` (1) | badge → host | `<HHHH>` Flipper frame size and badge canvas size |
| `READY` (2) | host → badge | Empty |
| `FRAME` (3) | host → badge | `orientation:u8`, then the native 128×64, 1-bit QFlipper frame (1,024 bytes) |
| `INPUT` (4) | badge → host | `key:u8`, `type:u8`, using QFlipper `InputKey` and `InputType` values |
| `UNLOCK` (5) | badge → host | Empty; bridge sends `DesktopUnlockRequest` |
| `STATUS` (6) | host → badge | UTF-8 diagnostic text, at most 63 bytes |
| `STREAM` (7) | badge → host | `enabled:u8` (0 or 1), stopping or restarting the native screen stream |

The badge uses QFlipper's framebuffer bit order: pixel `(x, y)` is bit `y & 7` in byte `(y / 8) * 128 + x`.

## Flipper-side RPC

The bridge opens the Flipper's USB serial interface at 230400 baud with DTR asserted, waits for the CLI prompt, sends `start_rpc_session\r`, then uses protobuf length-delimited `PB_Main` messages. It implements the exact QFlipper schema fields needed for this remote:

- `System.ProtobufVersionRequest` / response (tags 39/40), minimum protocol 0.16;
- `Gui.StartScreenStream`, `StopScreenStream`, `ScreenFrame`, and `SendInputEvent` (20–23);
- `Desktop.UnlockRequest`, `StatusSubscribeRequest`, and status (67, 68, 70).

The compact stdlib codec deliberately covers only these upstream fields, so the installed bridge has no Python package dependency while remaining wire-compatible with the official [Flipper RPC protobuf schema](https://github.com/flipperdevices/flipperzero-protobuf).
