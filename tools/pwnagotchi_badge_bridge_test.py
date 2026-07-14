#!/usr/bin/env python3
"""Focused tests for the Pwnagotchi USB badge bridge protocol and mobile page adapter."""

import importlib.util
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent
BRIDGE_PATH = ROOT / "pwnagotchi-badge-bridge" / "pwnagotchi_badge_bridge.py"
SPEC = importlib.util.spec_from_file_location("pwnagotchi_badge_bridge", BRIDGE_PATH)
bridge = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
sys.modules[SPEC.name] = bridge
SPEC.loader.exec_module(bridge)


class PwnagotchiBridgeTests(unittest.TestCase):
    def test_packet_round_trip_and_crc_recovery(self):
        packet = bridge.make_packet(bridge.PACKET_STATUS, 9, b"ready")
        self.assertEqual(bridge.PacketParser().feed(packet), [(bridge.PACKET_STATUS, 9, b"ready", bridge.zlib.crc32(b"ready") & 0xFFFFFFFF)])
        corrupt = bytearray(packet)
        corrupt[-1] ^= 1
        parsed = bridge.PacketParser().feed(corrupt)
        self.assertEqual(len(parsed), 1)
        self.assertNotEqual(bridge.zlib.crc32(parsed[0][2]) & 0xFFFFFFFF, parsed[0][3])
        oversized = bridge.HEADER.pack(bridge.MAGIC, bridge.VERSION, bridge.PACKET_PAGE, 0, 1, bridge.MAX_PAYLOAD + 1, 0)
        self.assertEqual(bridge.PacketParser().feed(oversized), [])

    def test_mobile_forms_hide_secrets_and_expose_supported_controls(self):
        parser = bridge.MobilePageParser("/plugins/example")
        parser.feed("""
            <title>Example Plugin</title><form action='/plugins/example' method='post'>
            <input name='api_key' value='nope'><input name='enabled' type='checkbox' checked>
            <input name='interval' type='number' value='5' min='1' max='10'>
            <select name='mode'><option value='slow'>Slow</option><option value='fast' selected>Fast</option></select>
            <input type='text' name='label' value='read only'><button name='save' value='1'>Save</button></form>
            <a href='/plugins/child'>Child page</a>
        """)
        page = parser.model()
        labels = [action.label for action in page.actions]
        self.assertNotIn("api_key", " ".join(labels).lower())
        self.assertEqual([action.kind for action in page.actions], ["toggle", "number", "choice", "readonly", "submit", "link"])
        self.assertEqual(page.forms[0].fields["enabled"], "1")
        self.assertEqual(page.forms[0].fields["mode"], "fast")

    def test_page_payload_is_bounded_and_paginates(self):
        instance = bridge.PwnagotchiBadgeBridge({"PWN_URL": "http://127.0.0.1:8080"})
        instance.page = bridge.PageModel("/", "Plugins", "dynamic pages", [bridge.Action(f"Plugin {index}", "link", route=f"/{index}") for index in range(20)], [])
        instance.page.focus = 14
        instance.page.offset = 12
        payload = instance.page_payload()
        self.assertLessEqual(len(payload), bridge.MAX_PAYLOAD)
        self.assertEqual(payload[3], 8)

    def test_stock_plugin_toggle_posts_checked_state_and_refreshes(self):
        parser = bridge.MobilePageParser("/plugins")
        parser.feed("""
            <title>Plugins</title><form method="POST" action="/plugins/toggle" class="plugin-toggle">
            <input type="checkbox" name="enabled" checked><input type="hidden" name="csrf_token" value="csrf">
            <input type="hidden" name="plugin" value="example"></form>
        """)
        page = parser.model()
        action = page.actions[0]
        self.assertEqual(action.label, "Plugin: example")
        self.assertTrue(page.forms[0].auto_submit)

        class FakeWeb:
            def __init__(self):
                self.calls = []

            def request(self, route, method="GET", fields=None):
                self.calls.append((route, method, fields))
                return b"success"

            def page(self, route):
                return page

        instance = bridge.PwnagotchiBadgeBridge({"PWN_URL": "http://127.0.0.1:8080"})
        instance.web = FakeWeb()
        instance.page = page
        instance.face_page = False
        instance.accept()
        self.assertEqual(instance.web.calls[0], ("/plugins/toggle", "POST", {"csrf_token": "csrf", "plugin": "example"}))

    def test_reconnect_reset_and_stale_controller_events_are_ignored(self):
        instance = bridge.PwnagotchiBadgeBridge({"PWN_URL": "http://127.0.0.1:8080"})
        received = []
        instance.handle_input = received.append
        payload = bytes((bridge.INPUT_ACCEPT,))
        crc = bridge.zlib.crc32(payload) & 0xFFFFFFFF
        instance.handle_packet(bridge.PACKET_INPUT, 9, payload, crc)
        instance.handle_packet(bridge.PACKET_INPUT, 8, payload, crc)
        self.assertEqual(received, [bridge.INPUT_ACCEPT])
        instance.ready = True
        instance.reset()
        self.assertFalse(instance.ready)
        self.assertIsNone(instance.last_badge_sequence)

    def test_badge_source_declares_controller_contract(self):
        source = (ROOT.parent / "src" / "apps" / "pwnagotchi_remote.c").read_text(encoding="utf-8")
        for token in ("PWN_MAGIC \"PWN1\"", "PwnInputHome", "PwnInputPlugins", "PwnPacketFace", "PwnPacketPage"):
            self.assertIn(token, source)


if __name__ == "__main__":
    unittest.main()
