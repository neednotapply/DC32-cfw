#!/usr/bin/env python3
"""Focused protocol checks for the standalone RaspyJack badge bridge."""

import importlib.util
from io import BytesIO
from pathlib import Path
import struct
import tempfile
import unittest


ROOT = Path(__file__).resolve().parent
BRIDGE_PATH = ROOT / "raspyjack-badge-bridge" / "raspyjack_badge_bridge.py"
SPEC = importlib.util.spec_from_file_location("raspyjack_badge_bridge", BRIDGE_PATH)
bridge = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
SPEC.loader.exec_module(bridge)


def complete_tx(instance: object) -> bytes:
    packet = instance.tx_chunk()
    instance.advance_tx(len(packet))
    return packet


class RaspyJackBridgeProtocolTests(unittest.TestCase):
    def make_ready_bridge(self, ack: bool, raw: bool = False, tiles: bool = False) -> object:
        instance = bridge.RaspyJackBadgeBridge({})
        instance.badge_ready = True
        instance.badge_supports_ack = ack
        instance.badge_supports_rgb565 = raw
        instance.badge_supports_tiles = tiles
        return instance

    def set_latest_frame(self, instance: object, body: bytes, when: float = 1.0) -> None:
        frame = b"\xff\xd8" + body + b"\xff\xd9"
        instance.latest_frame = {"type": bridge.PACKET_FRAME, "payload": frame,
                                 "source_seen": when, "rgb565": None}

    def test_defaults_preserve_native_raspyjack_display(self):
        instance = bridge.RaspyJackBadgeBridge({})
        self.assertEqual(instance.frame_path, Path("/dev/shm/raspyjack_last.jpg"))
        self.assertEqual(instance.poll_seconds, 0.1)
        self.assertEqual(instance.tick_seconds, 0.005)

    def test_fragmented_frame_round_trip(self):
        payload = b"\xff\xd8small-frame\xff\xd9"
        wire = bridge.make_packet(bridge.PACKET_FRAME, 42, payload)
        parser = bridge.PacketParser()
        self.assertEqual(parser.feed(wire[:7]), [])
        self.assertEqual(parser.feed(wire[7:19]), [])
        self.assertEqual(parser.feed(wire[19:]), [(bridge.PACKET_FRAME, 42, payload, bridge.packet_crc(payload))])

    def test_resynchronizes_after_garbage_and_rejects_oversize_header(self):
        parser = bridge.PacketParser()
        bad = bridge.HEADER.pack(bridge.MAGIC, bridge.VERSION, bridge.PACKET_FRAME, 0, 1,
                                 bridge.MAX_FRAME_BYTES + 1, 0)
        good = bridge.make_packet(bridge.PACKET_READY, 2)
        self.assertEqual(parser.feed(b"noise" + bad + good), [(bridge.PACKET_READY, 2, b"", 0)])

    def test_crc_and_button_contract(self):
        payload = bytes((1, 1))
        self.assertTrue(bridge.packet_is_valid(payload, bridge.packet_crc(payload)))
        self.assertFalse(bridge.packet_is_valid(payload, bridge.packet_crc(payload) ^ 1))
        self.assertEqual(bridge.BUTTON_NAMES[8], "KEY3")

    def test_frame_limit_is_64_kib(self):
        with self.assertRaises(ValueError):
            bridge.make_packet(bridge.PACKET_FRAME, 1, b"x" * (bridge.MAX_FRAME_BYTES + 1))
        with tempfile.TemporaryDirectory() as temp:
            frame = Path(temp) / "frame.jpg"
            frame.write_bytes(b"\xff\xd8" + b"x" * bridge.MAX_FRAME_BYTES + b"\xff\xd9")
            self.assertIsNone(bridge.RaspyJackBadgeBridge({"FRAME_PATH": str(frame)}).read_stable_frame())

    def test_ack_gates_next_frame_and_keeps_latest_replacement(self):
        instance = self.make_ready_bridge(ack=True)
        self.set_latest_frame(instance, b"first")
        self.assertTrue(instance.start_next_tx())
        first = bridge.PacketParser().feed(complete_tx(instance))[0]
        self.assertEqual(instance.inflight_sequence, first[1])
        self.set_latest_frame(instance, b"newest", when=3.0)
        self.assertFalse(instance.start_next_tx())
        ack = struct.pack("<IB", first[1], 1)
        instance.handle_packet(bridge.PACKET_FRAME_ACK, 0, ack, bridge.packet_crc(ack))
        self.assertTrue(instance.start_next_tx())
        second = bridge.PacketParser().feed(complete_tx(instance))[0]
        self.assertIn(b"newest", second[2])

    def test_crc_rejection_does_not_release_ack_gate(self):
        instance = self.make_ready_bridge(ack=True)
        self.set_latest_frame(instance, b"frame")
        instance.start_next_tx()
        sequence = bridge.PacketParser().feed(complete_tx(instance))[0][1]
        ack = struct.pack("<IB", sequence, 1)
        instance.handle_packet(bridge.PACKET_FRAME_ACK, 0, ack, bridge.packet_crc(ack) ^ 1)
        self.assertEqual(instance.inflight_sequence, sequence)

    def test_legacy_badge_streams_without_ack(self):
        instance = self.make_ready_bridge(ack=False)
        self.set_latest_frame(instance, b"one")
        instance.start_next_tx()
        complete_tx(instance)
        self.assertIsNone(instance.inflight_sequence)
        self.set_latest_frame(instance, b"two", when=2.0)
        self.assertTrue(instance.start_next_tx())

    def test_packet_writes_do_not_interleave(self):
        instance = self.make_ready_bridge(ack=False)
        self.set_latest_frame(instance, b"frame")
        instance.start_next_tx()
        first_part = instance.tx_chunk(7)
        instance.advance_tx(len(first_part))
        instance.queue_packet(bridge.PACKET_STATUS, b"status")
        remaining = complete_tx(instance)
        self.assertEqual(bridge.PacketParser().feed(first_part + remaining)[0][0], bridge.PACKET_FRAME)
        self.assertTrue(instance.start_next_tx())
        self.assertEqual(bridge.PacketParser().feed(complete_tx(instance))[0][0], bridge.PACKET_STATUS)

    def test_rgb565_and_tile_payloads_round_trip(self):
        old = bytes(bridge.RGB565_BYTES)
        new = bytearray(old)
        pixel = ((9 * bridge.SOURCE_WIDTH) + 10) * 2
        new[pixel:pixel + 2] = b"\x1f\x00"
        tiles = bridge.RaspyJackBadgeBridge.make_tile_payload(old, bytes(new))
        self.assertIsNotNone(tiles)
        width, height, tile_size, count = struct.unpack_from("<HHBH", tiles)
        self.assertEqual((width, height, tile_size, count), (128, 128, 8, 1))
        self.assertEqual(len(tiles), 7 + 2 + bridge.TILE_BYTES)

    @unittest.skipUnless(bridge.Image is not None, "Pillow is optional")
    def test_jpeg_conversion_and_auto_rgb565_selection(self):
        image = bridge.Image.new("RGB", (128, 128), (255, 0, 0))
        encoded = BytesIO()
        image.save(encoded, "JPEG", quality=90)
        raw = bridge.RaspyJackBadgeBridge.jpeg_to_rgb565(encoded.getvalue())
        self.assertEqual(len(raw), bridge.RGB565_BYTES)
        self.assertEqual(raw[:2], b"\x00\xf8")
        instance = self.make_ready_bridge(ack=True, raw=True, tiles=True)
        video = instance.make_video_frame(encoded.getvalue(), 1.0)
        self.assertEqual(video["type"], bridge.PACKET_FRAME_RGB565)

    def test_raw_ack_establishes_tile_base(self):
        instance = self.make_ready_bridge(ack=True, raw=True, tiles=True)
        raw = bytes(bridge.RGB565_BYTES)
        instance.latest_frame = {"type": bridge.PACKET_FRAME_RGB565, "payload": struct.pack("<HH", 128, 128) + raw,
                                 "source_seen": 1.0, "rgb565": raw}
        instance.start_next_tx()
        sequence = bridge.PacketParser().feed(complete_tx(instance))[0][1]
        ack = struct.pack("<IB", sequence, 1)
        instance.handle_packet(bridge.PACKET_FRAME_ACK, 0, ack, bridge.packet_crc(ack))
        self.assertEqual(instance.last_presented_rgb565, raw)

    def test_rejected_video_resets_tile_base_for_recovery(self):
        instance = self.make_ready_bridge(ack=True, raw=True, tiles=True)
        instance.last_presented_rgb565 = bytes(bridge.RGB565_BYTES)
        instance.last_frame_signature = (1, 2)
        instance.inflight_sequence = 7
        rejected = struct.pack("<IB", 7, 0)
        instance.handle_packet(bridge.PACKET_FRAME_ACK, 0, rejected, bridge.packet_crc(rejected))
        self.assertIsNone(instance.last_presented_rgb565)
        self.assertIsNone(instance.last_frame_signature)



if __name__ == "__main__":
    unittest.main()
