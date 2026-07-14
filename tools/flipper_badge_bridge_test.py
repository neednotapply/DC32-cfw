#!/usr/bin/env python3
"""Focused protocol tests for the DC32 Flipper/QFlipper bridge."""

import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parent
BRIDGE_PATH = ROOT / "flipper-badge-bridge" / "flipper_badge_bridge.py"
SPEC = importlib.util.spec_from_file_location("flipper_badge_bridge", BRIDGE_PATH)
bridge = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
SPEC.loader.exec_module(bridge)


class FlipperBadgeBridgeTests(unittest.TestCase):
    def test_frc1_fragmentation_and_crc_recovery(self):
        packet = bridge.make_packet(bridge.PACKET_READY, 4)
        parser = bridge.PacketParser()
        self.assertEqual(parser.feed(packet[:5]), [])
        self.assertEqual(parser.feed(packet[5:]), [(bridge.PACKET_READY, 4, b"")])
        corrupt = bytearray(packet)
        corrupt[-1:] = b"x"
        self.assertEqual(bridge.PacketParser().feed(bytes(corrupt)), [])

    def test_qflipper_varint_delimited_round_trip(self):
        wire = bridge.encode_main(7, bridge.RPC_UNLOCK)
        parser = bridge.DelimitedParser()
        messages = parser.feed(wire[:2])
        self.assertEqual(messages, [])
        messages = parser.feed(wire[2:])
        self.assertEqual(len(messages), 1)
        decoded = bridge.decode_main(messages[0])
        self.assertEqual((decoded["command_id"], decoded["content_tag"]), (7, bridge.RPC_UNLOCK))

    def test_screen_frame_payload_uses_native_xbm_size(self):
        frame = bytes(bridge.FRAME_BYTES)
        content = bridge.protobuf_field(1, 2, frame) + bridge.protobuf_field(2, 0, 3)
        decoded = bridge.decode_main(bridge.DelimitedParser().feed(
            bridge.encode_main(0, bridge.RPC_SCREEN_FRAME, content))[0])
        fields = {number: value for number, _wire, value in bridge.protobuf_fields(decoded["content"])}
        self.assertEqual((len(fields[1]), fields[2]), (1024, 3))

    def test_input_preserves_every_qflipper_key_and_type(self):
        instance = bridge.FlipperBadgeBridge({})
        instance.rpc_up = True
        for key in range(6):
            for event_type in range(5):
                instance.handle_badge_packet(bridge.PACKET_INPUT, 0, bytes((key, event_type)))
        messages = bridge.DelimitedParser().feed(bytes(instance.flipper_tx))
        self.assertEqual(len(messages), 30)
        for index, message in enumerate(messages):
            decoded = bridge.decode_main(message)
            self.assertEqual(decoded["content_tag"], bridge.RPC_SEND_INPUT)
            fields = {number: value for number, _wire, value in bridge.protobuf_fields(decoded["content"])}
            self.assertEqual((fields[1], fields[2]), (index // 5, index % 5))

    def test_hello_queues_ready_and_latest_status(self):
        instance = bridge.FlipperBadgeBridge({})
        instance.last_status = "Opening Flipper RPC"
        instance.handle_badge_packet(bridge.PACKET_HELLO, 0, bytes(8))
        packets = bridge.PacketParser().feed(bytes(instance.badge_tx))
        self.assertEqual([packet[0] for packet in packets], [bridge.PACKET_READY, bridge.PACKET_STATUS])
        self.assertEqual(packets[1][2], b"Opening Flipper RPC")

    def test_protocol_version_gate_and_stream_start(self):
        instance = bridge.FlipperBadgeBridge({})
        instance.rpc_up = True
        version = bridge.protobuf_field(1, 0, 0) + bridge.protobuf_field(2, 0, 16)
        response = bridge.protobuf_field(1, 0, 1) + bridge.protobuf_field(2, 0, 0) + bridge.protobuf_field(40, 2, version)
        instance.pending[1] = "protobuf version"
        instance.handle_rpc_message(response)
        sent = bridge.DelimitedParser().feed(bytes(instance.flipper_tx))
        tags = [bridge.decode_main(message)["content_tag"] for message in sent]
        self.assertEqual(tags, [bridge.RPC_START_SCREEN, bridge.RPC_DESKTOP_SUBSCRIBE])

    def test_screen_frame_forwards_all_orientations_to_ready_badge(self):
        frame = bytes([0x80]) * bridge.FRAME_BYTES
        for orientation in range(4):
            instance = bridge.FlipperBadgeBridge({})
            instance.badge_ready = instance.stream_enabled = True
            content = bridge.protobuf_field(1, 2, frame) + bridge.protobuf_field(2, 0, orientation)
            instance.handle_rpc_message(bridge.protobuf_field(22, 2, content))
            packet = bridge.PacketParser().feed(bytes(instance.badge_tx))[0]
            self.assertEqual((packet[0], packet[2][0], packet[2][1:]),
                             (bridge.PACKET_FRAME, orientation, frame))

    def test_badge_source_uses_qflipper_key_timing_and_orientation_paths(self):
        source = (ROOT.parent / "src" / "apps" / "flipper_remote.c").read_text(encoding="utf-8")
        self.assertIn("FRC_LONG_TICKS (TICKS_PER_SECOND * 350u / 1000u)", source)
        self.assertIn("FRC_REPEAT_TICKS (TICKS_PER_SECOND * 150u / 1000u)", source)
        self.assertIn("case 2: sx = 127u - y; sy = x", source)
        self.assertIn("case 3: sx = y; sy = 63u - x", source)

    def test_direct_badge_host_uses_cdc_and_qflipper_rpc(self):
        host_config = (ROOT.parent / "src" / "apps" / "flipper_tusb_config.h").read_text(encoding="utf-8")
        host_source = (ROOT.parent / "src" / "apps" / "flipper_usb_host.c").read_text(encoding="utf-8")
        app_source = (ROOT.parent / "src" / "apps" / "flipper_remote.c").read_text(encoding="utf-8")
        self.assertIn("OPT_MODE_HOST", host_config)
        self.assertIn("CFG_TUH_CDC 1", host_config)
        self.assertIn("tuh_cdc_set_baudrate(index, 230400u", host_source)
        self.assertIn("USB_USB_PWR_VBUS_EN_BITS", host_source)
        self.assertIn('"start_rpc_session\\r"', app_source)
        self.assertIn("directSendMain(app, 20u", app_source)
        self.assertIn("directSendMain(app,67u,NULL,0u)", app_source)
        self.assertIn("Host mode; requires 5V VBUS", app_source)

    def test_rapid_stream_resume_waits_for_stop_before_restart(self):
        instance = bridge.FlipperBadgeBridge({})
        instance.rpc_up = instance.stream_active = True
        instance.stream_enabled = False
        instance.stop_stream()
        stop_id = next(iter(instance.pending))
        instance.stream_enabled = True
        instance.start_stream()
        self.assertEqual(len(bridge.DelimitedParser().feed(bytes(instance.flipper_tx))), 1)
        stopped = bridge.protobuf_field(1, 0, stop_id) + bridge.protobuf_field(2, 0, 0) + bridge.protobuf_field(4, 2, b"")
        instance.handle_rpc_message(stopped)
        tags = [bridge.decode_main(message)["content_tag"] for message in bridge.DelimitedParser().feed(bytes(instance.flipper_tx))]
        self.assertEqual(tags, [bridge.RPC_STOP_SCREEN, bridge.RPC_START_SCREEN])


if __name__ == "__main__":
    unittest.main()
