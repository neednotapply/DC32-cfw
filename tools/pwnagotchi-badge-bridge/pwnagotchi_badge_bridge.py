#!/usr/bin/env python3
"""USB-only DC32 remote adapter for Jayofelony Pwnagotchi's mobile web UI."""

from __future__ import annotations

import base64
import glob
import os
import select
import struct
import sys
import time
import urllib.parse
import urllib.request
import zlib
from collections import deque
from dataclasses import dataclass, field
from html.parser import HTMLParser
from pathlib import Path
from typing import Any

try:
    import termios
except ImportError:  # Allows protocol/parser tests on non-POSIX development hosts.
    termios = None

try:
    from PIL import Image
    from io import BytesIO
except ImportError:  # Face streaming is optional; navigation still works.
    Image = None
    BytesIO = None

MAGIC = b"PWN1"
VERSION = 1
HEADER = struct.Struct("<4sBBHIII")
MAX_PAYLOAD = 1200
FACE_SIZE = (128, 64)
FACE_BYTES = FACE_SIZE[0] * FACE_SIZE[1] // 8
MAX_PAGE_ITEMS = 12
SECRET_WORDS = ("pass", "password", "secret", "token", "api_key", "apikey", "private", "key")

PACKET_HELLO = 1
PACKET_READY = 2
PACKET_PAGE = 3
PACKET_INPUT = 4
PACKET_STATUS = 5
PACKET_FACE = 6

INPUT_UP = 1
INPUT_DOWN = 2
INPUT_LEFT = 3
INPUT_RIGHT = 4
INPUT_ACCEPT = 5
INPUT_BACK = 6
INPUT_HOME = 7
INPUT_PLUGINS = 8


def make_packet(packet_type: int, sequence: int, payload: bytes = b"") -> bytes:
    return HEADER.pack(MAGIC, VERSION, packet_type, 0, sequence, len(payload), zlib.crc32(payload) & 0xFFFFFFFF) + payload


class PacketParser:
    def __init__(self) -> None:
        self.buffer = bytearray()

    def feed(self, data: bytes) -> list[tuple[int, int, bytes, int]]:
        self.buffer.extend(data)
        packets: list[tuple[int, int, bytes, int]] = []
        while True:
            start = self.buffer.find(MAGIC)
            if start < 0:
                del self.buffer[:-3]
                return packets
            if start:
                del self.buffer[:start]
            if len(self.buffer) < HEADER.size:
                return packets
            magic, version, packet_type, _reserved, sequence, size, crc = HEADER.unpack(self.buffer[:HEADER.size])
            if magic != MAGIC or version != VERSION or size > MAX_PAYLOAD:
                del self.buffer[0]
                continue
            end = HEADER.size + size
            if len(self.buffer) < end:
                return packets
            packets.append((packet_type, sequence, bytes(self.buffer[HEADER.size:end]), crc))
            del self.buffer[:end]


def configure_serial(fd: int) -> None:
    if termios is None:
        raise RuntimeError("serial CDC bridge requires POSIX termios")
    attrs = termios.tcgetattr(fd)
    attrs[0] = attrs[1] = attrs[3] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[4] = attrs[5] = termios.B115200
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)


def is_secret(name: str) -> bool:
    lowered = name.lower().replace("-", "_")
    return any(word in lowered for word in SECRET_WORDS)


@dataclass
class Action:
    label: str
    kind: str
    route: str = ""
    form: int = -1
    name: str = ""
    value: str = ""
    choices: list[str] = field(default_factory=list)
    minimum: int | None = None
    maximum: int | None = None
    step: int = 1


@dataclass
class WebForm:
    action: str
    method: str
    fields: dict[str, str] = field(default_factory=dict)
    auto_submit: bool = False


@dataclass
class PageModel:
    route: str
    title: str
    detail: str
    actions: list[Action]
    forms: list[WebForm]
    offset: int = 0
    focus: int = 0


class MobilePageParser(HTMLParser):
    """Extracts the control-bearing subset of Pwnagotchi's jQuery-mobile HTML."""

    def __init__(self, route: str) -> None:
        super().__init__(convert_charrefs=True)
        self.route = route
        self.title = "Pwnagotchi"
        self.text: list[str] = []
        self.actions: list[Action] = []
        self.forms: list[WebForm] = []
        self.current_form = -1
        self.current_link: tuple[str, list[str]] | None = None
        self.current_button: tuple[int, str, str, list[str]] | None = None
        self.current_option: dict[str, Any] | None = None
        self.current_select: Action | None = None

    @staticmethod
    def attrs_dict(attrs: list[tuple[str, str | None]]) -> dict[str, str]:
        return {key.lower(): value or "" for key, value in attrs}

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        data = self.attrs_dict(attrs)
        if tag == "title":
            self.current_link = ("__title__", [])
        elif tag == "form":
            classes = set(data.get("class", "").split())
            self.forms.append(WebForm(
                data.get("action") or self.route,
                (data.get("method") or "GET").upper(),
                auto_submit="plugin-toggle" in classes,
            ))
            self.current_form = len(self.forms) - 1
        elif tag == "a" and data.get("href"):
            self.current_link = (data["href"], [])
        elif tag == "input" and self.current_form >= 0:
            form = self.forms[self.current_form]
            name = data.get("name", "")
            input_type = data.get("type", "text").lower()
            value = data.get("value", "")
            if not name:
                return
            if input_type == "hidden":
                form.fields[name] = value
                return
            if is_secret(name) or input_type in ("password", "file"):
                return
            label = data.get("aria-label") or data.get("placeholder") or name.replace("_", " ")
            if input_type in ("checkbox", "radio"):
                form.fields[name] = "1" if "checked" in data else "0"
                self.actions.append(Action(label, "toggle", form=self.current_form, name=name, value=form.fields[name]))
            elif input_type in ("number", "range"):
                form.fields[name] = value
                self.actions.append(Action(label, "number", form=self.current_form, name=name, value=value,
                                           minimum=int(data["min"]) if data.get("min", "").lstrip("-").isdigit() else None,
                                           maximum=int(data["max"]) if data.get("max", "").lstrip("-").isdigit() else None,
                                           step=max(1, int(data.get("step", "1"))) if data.get("step", "1").isdigit() else 1))
            elif input_type in ("submit", "button"):
                self.actions.append(Action(value or label, "submit", form=self.current_form, name=name, value=value))
            else:
                form.fields[name] = value
                self.actions.append(Action(f"{label}: {value}", "readonly", form=self.current_form, name=name, value=value))
        elif tag == "select" and self.current_form >= 0:
            name = data.get("name", "")
            if name and not is_secret(name):
                self.current_select = Action(data.get("aria-label") or name.replace("_", " "), "choice", form=self.current_form, name=name)
        elif tag == "option" and self.current_select is not None:
            self.current_option = {"value": data.get("value", ""), "selected": "selected" in data, "text": []}
        elif tag == "button" and self.current_form >= 0:
            self.current_button = (self.current_form, data.get("name", ""), data.get("value", ""), [])

    def handle_endtag(self, tag: str) -> None:
        if tag == "title" and self.current_link and self.current_link[0] == "__title__":
            self.title = " ".join(self.current_link[1]).strip() or self.title
            self.current_link = None
        elif tag == "a" and self.current_link:
            route, words = self.current_link
            label = " ".join(words).strip()
            if label and not route.startswith(("http:", "https:", "mailto:", "#")):
                self.actions.append(Action(label, "link", route=route))
            self.current_link = None
        elif tag == "option" and self.current_option is not None and self.current_select is not None:
            option = self.current_option
            value = option["value"] or " ".join(option["text"]).strip()
            if value:
                self.current_select.choices.append(value)
                if option["selected"] or not self.current_select.value:
                    self.current_select.value = value
            self.current_option = None
        elif tag == "select" and self.current_select is not None:
            if self.current_select.choices:
                self.forms[self.current_select.form].fields[self.current_select.name] = self.current_select.value
                self.actions.append(self.current_select)
            self.current_select = None
        elif tag == "form":
            self.current_form = -1
        elif tag == "button" and self.current_button is not None:
            form, name, value, words = self.current_button
            self.actions.append(Action(" ".join(words).strip() or value or "Submit", "submit", form=form, name=name, value=value))
            self.current_button = None

    def handle_data(self, data: str) -> None:
        text = " ".join(data.split())
        if not text:
            return
        if self.current_link is not None:
            self.current_link[1].append(text)
        elif self.current_button is not None:
            self.current_button[3].append(text)
        elif self.current_option is not None:
            self.current_option["text"].append(text)
        else:
            self.text.append(text)

    def model(self) -> PageModel:
        # Jayofelony's stock plugins page uses a JavaScript change handler rather
        # than a submit button.  The bridge supplies that same POST on A, and gives
        # each otherwise-unlabelled switch the name of the plugin it controls.
        for action in self.actions:
            if action.kind != "toggle" or action.form < 0:
                continue
            form = self.forms[action.form]
            if form.action.rstrip("/") == "/plugins/toggle":
                plugin = form.fields.get("plugin", "")
                if plugin:
                    action.label = f"Plugin: {plugin}"
                form.auto_submit = True
        detail = " ".join(self.text)[:220]
        if not self.actions:
            self.actions.append(Action("No supported controls on this page", "readonly"))
        return PageModel(self.route, self.title[:31], detail, self.actions, self.forms)


class PwnagotchiWebClient:
    def __init__(self, base_url: str, username: str, password: str, timeout: float) -> None:
        parsed = urllib.parse.urlparse(base_url)
        if parsed.scheme != "http" or parsed.hostname not in ("127.0.0.1", "localhost", "::1"):
            raise ValueError("PWN_URL must be an http:// loopback URL")
        self.base_url = base_url.rstrip("/") + "/"
        self.timeout = timeout
        manager = urllib.request.HTTPPasswordMgrWithDefaultRealm()
        if username:
            manager.add_password(None, self.base_url, username, password)
        # Build an HTTP-only opener explicitly. build_opener() installs HTTPS support
        # even though the bridge rejects it, which is both unnecessary and can touch
        # host certificate stores during otherwise offline protocol tests.
        self.opener = urllib.request.OpenerDirector()
        self.opener.add_handler(urllib.request.ProxyHandler({}))
        self.opener.add_handler(urllib.request.UnknownHandler())
        self.opener.add_handler(urllib.request.HTTPHandler())
        self.opener.add_handler(urllib.request.HTTPDefaultErrorHandler())
        self.opener.add_handler(urllib.request.HTTPRedirectHandler())
        self.opener.add_handler(urllib.request.HTTPCookieProcessor())
        self.opener.add_handler(urllib.request.HTTPBasicAuthHandler(manager))

    def url(self, route: str) -> str:
        target = urllib.parse.urljoin(self.base_url, route.lstrip("/"))
        parsed = urllib.parse.urlparse(target)
        if parsed.hostname not in ("127.0.0.1", "localhost", "::1"):
            raise ValueError("refusing non-loopback mobile UI route")
        return target

    def request(self, route: str, method: str = "GET", fields: dict[str, str] | None = None) -> bytes:
        data = urllib.parse.urlencode(fields or {}).encode() if method == "POST" else None
        target = self.url(route)
        if method == "GET" and fields:
            target += ("&" if "?" in target else "?") + urllib.parse.urlencode(fields)
        req = urllib.request.Request(target, data=data, method=method)
        with self.opener.open(req, timeout=self.timeout) as response:
            return response.read()

    def page(self, route: str) -> PageModel:
        parser = MobilePageParser(route)
        parser.feed(self.request(route).decode("utf-8", "replace"))
        return parser.model()

    def face(self) -> bytes | None:
        if Image is None or BytesIO is None:
            return None
        try:
            source = Image.open(BytesIO(self.request("/ui"))).convert("L")
            # The source display varies by Pwnagotchi hardware.  Letterbox it
            # instead of stretching it to the badge frame, preserving its native
            # aspect ratio while keeping the wire format fixed and compact.
            source.thumbnail(FACE_SIZE)
            image = Image.new("L", FACE_SIZE, 255)
            image.paste(source, ((FACE_SIZE[0] - source.width) // 2, (FACE_SIZE[1] - source.height) // 2))
        except Exception:
            return None
        pixels = image.load()
        output = bytearray(FACE_BYTES)
        for y in range(FACE_SIZE[1]):
            for x in range(FACE_SIZE[0]):
                if pixels[x, y] < 128:
                    offset = y * FACE_SIZE[0] + x
                    output[offset >> 3] |= 0x80 >> (offset & 7)
        return bytes(output)


class PwnagotchiBadgeBridge:
    def __init__(self, environ: dict[str, str] | None = None) -> None:
        env = os.environ if environ is None else environ
        self.device = env.get("DEVICE", "").strip()
        self.device_glob = env.get("DEVICE_GLOB", "/dev/serial/by-id/*DC32*Pwnagotchi*")
        self.device_fallback = env.get("DEVICE_FALLBACK", "/dev/ttyACM0").strip()
        self.tick_seconds = max(0.002, float(env.get("TICK_MS", "10")) / 1000.0)
        self.face_seconds = max(0.25, float(env.get("FACE_SECONDS", "1")))
        self.plugin_refresh_seconds = max(1.0, float(env.get("PLUGIN_REFRESH_SECONDS", "5")))
        self.web = PwnagotchiWebClient(env.get("PWN_URL", "http://127.0.0.1:8080"), env.get("PWN_USER", ""),
                                       env.get("PWN_PASSWORD", ""), max(1.0, float(env.get("HTTP_TIMEOUT", "5"))))
        self.fd: int | None = None
        self.parser = PacketParser()
        self.sequence = 0
        self.ready = False
        self.last_badge_sequence: int | None = None
        self.tx: deque[bytearray] = deque()
        self.history: list[str] = []
        self.page: PageModel | None = None
        self.face_page = True
        self.next_face = 0.0
        self.next_plugin_refresh = 0.0

    def log(self, message: str) -> None:
        print(f"[pwnagotchi-badge-bridge] {message}", flush=True)

    def find_device(self) -> str | None:
        if self.device:
            return self.device if Path(self.device).exists() else None
        matches = sorted(glob.glob(self.device_glob))
        if matches:
            return matches[0]
        return self.device_fallback if self.device_fallback and Path(self.device_fallback).exists() else None

    def reset(self) -> None:
        self.parser = PacketParser()
        self.ready = False
        self.last_badge_sequence = None
        self.tx.clear()
        self.history.clear()
        self.page = None
        self.face_page = True
        self.next_face = 0.0
        self.next_plugin_refresh = 0.0

    def connect(self) -> bool:
        path = self.find_device()
        if not path:
            return False
        try:
            self.fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
            configure_serial(self.fd)
        except OSError as exc:
            self.fd = None
            self.log(f"cannot open {path}: {exc}")
            return False
        self.reset()
        self.log(f"connected to {path}")
        return True

    def disconnect(self, reason: str = "") -> None:
        if self.fd is not None:
            try:
                os.close(self.fd)
            except OSError:
                pass
        self.fd = None
        self.reset()
        if reason:
            self.log(f"disconnected: {reason}")

    def queue(self, packet_type: int, payload: bytes = b"") -> None:
        self.sequence = (self.sequence + 1) & 0xFFFFFFFF
        self.tx.append(bytearray(make_packet(packet_type, self.sequence, payload)))

    def status(self, text: str) -> None:
        self.queue(PACKET_STATUS, text[:79].encode("utf-8", "replace"))

    @staticmethod
    def label(action: Action) -> str:
        if action.kind == "toggle":
            return f"[{'x' if action.value == '1' else ' '}] {action.label}"
        if action.kind == "choice":
            return f"{action.label}: {action.value}"
        if action.kind == "number":
            return f"{action.label}: {action.value}"
        if action.kind == "link":
            return f"> {action.label}"
        if action.kind == "submit":
            return f"* {action.label}"
        return action.label

    def page_payload(self) -> bytes:
        assert self.page is not None
        actions = self.page.actions
        start = self.page.offset
        end = min(len(actions), start + MAX_PAGE_ITEMS)
        visible = actions[start:end]
        labels = [self.label(action)[:39] for action in visible]
        header = bytes((1, 0, min(self.page.focus - start, max(0, len(labels) - 1)), len(labels)))
        fields = [self.page.title[:31], self.page.detail[:79], *labels]
        payload = header + b"".join(value.encode("utf-8", "replace") + b"\0" for value in fields)
        return payload[:MAX_PAYLOAD]

    def send_page(self) -> None:
        if self.page is not None:
            self.queue(PACKET_PAGE, self.page_payload())

    def show_face(self) -> None:
        self.face_page = True
        self.page = PageModel("", "PWNAGOTCHI", "Face / Start opens mobile UI", [], [])
        self.queue(PACKET_PAGE, bytes((0, 0, 0, 0)) + b"PWNAGOTCHI\0Face / Start opens mobile UI\0")
        self.refresh_face(force=True)

    def open_page(self, route: str, push: bool = True) -> None:
        try:
            page = self.web.page(route)
        except Exception as exc:
            self.status(f"Mobile UI error: {exc}")
            return
        if push and self.page is not None and not self.face_page:
            self.history.append(self.page.route)
        self.page = page
        self.face_page = False
        if route.rstrip("/") == "/plugins":
            self.next_plugin_refresh = time.monotonic() + self.plugin_refresh_seconds
        self.send_page()

    def current_action(self) -> Action | None:
        if self.page is None or self.face_page or not self.page.actions:
            return None
        self.page.focus = max(0, min(self.page.focus, len(self.page.actions) - 1))
        return self.page.actions[self.page.focus]

    def move_focus(self, amount: int) -> None:
        if self.page is None or self.face_page or not self.page.actions:
            return
        self.page.focus = (self.page.focus + amount) % len(self.page.actions)
        self.page.offset = (self.page.focus // MAX_PAGE_ITEMS) * MAX_PAGE_ITEMS
        self.send_page()

    def adjust(self, amount: int) -> None:
        action = self.current_action()
        if action is None:
            return
        if action.kind == "toggle":
            action.value = "0" if action.value == "1" else "1"
        elif action.kind == "choice" and action.choices:
            index = action.choices.index(action.value) if action.value in action.choices else 0
            action.value = action.choices[(index + amount) % len(action.choices)]
        elif action.kind == "number":
            try:
                value = int(float(action.value or "0")) + amount * action.step
            except ValueError:
                value = 0
            if action.minimum is not None:
                value = max(action.minimum, value)
            if action.maximum is not None:
                value = min(action.maximum, value)
            action.value = str(value)
        else:
            return
        assert self.page is not None
        if action.form >= 0:
            self.page.forms[action.form].fields[action.name] = action.value
        self.send_page()

    def submit(self, action: Action) -> None:
        assert self.page is not None
        form = self.page.forms[action.form]
        fields = dict(form.fields)
        # HTML does not submit unchecked checkboxes.  In particular, Pwnagotchi's
        # /plugins/toggle endpoint treats the *presence* of enabled as true.
        for candidate in self.page.actions:
            if candidate.form == action.form and candidate.kind == "toggle" and candidate.value != "1":
                fields.pop(candidate.name, None)
        if action.name:
            if action.kind != "toggle" or action.value == "1":
                fields[action.name] = action.value
        try:
            self.web.request(form.action, form.method, fields)
        except Exception as exc:
            self.status(f"Save failed: {exc}")
            return
        self.open_page(self.page.route, push=False)
        self.status("Saved")

    def accept(self) -> None:
        if self.face_page:
            self.open_page("/")
            return
        action = self.current_action()
        if action is None:
            return
        if action.kind == "link":
            self.open_page(action.route)
        elif action.kind == "toggle":
            self.adjust(1)
            if self.page is not None and self.page.forms[action.form].auto_submit:
                self.submit(action)
        elif action.kind == "submit":
            self.submit(action)
        elif action.kind == "readonly":
            self.status("Text values are read-only on badge")

    def handle_input(self, value: int) -> None:
        if value == INPUT_HOME:
            self.show_face()
        elif value == INPUT_PLUGINS:
            self.open_page("/plugins", push=False)
        elif value == INPUT_BACK:
            if self.history:
                self.open_page(self.history.pop(), push=False)
            else:
                self.show_face()
        elif value == INPUT_UP:
            self.move_focus(-1)
        elif value == INPUT_DOWN:
            self.move_focus(1)
        elif value == INPUT_LEFT:
            self.adjust(-1)
        elif value == INPUT_RIGHT:
            self.adjust(1)
        elif value == INPUT_ACCEPT:
            self.accept()

    def handle_packet(self, packet_type: int, sequence: int, payload: bytes, crc: int) -> None:
        if zlib.crc32(payload) & 0xFFFFFFFF != crc:
            self.status("Bad badge packet CRC")
            return
        if self.last_badge_sequence is not None:
            delta = (sequence - self.last_badge_sequence) & 0xFFFFFFFF
            if delta == 0 or delta >= 0x80000000:
                return  # Delayed/replayed controller event from before a refresh.
        self.last_badge_sequence = sequence
        if packet_type == PACKET_HELLO:
            if len(payload) != 4:
                self.status("Bad HELLO")
                return
            self.ready = True
            self.queue(PACKET_READY)
            self.show_face()
        elif packet_type == PACKET_INPUT and len(payload) == 1:
            self.handle_input(payload[0])

    def read_badge(self) -> None:
        assert self.fd is not None
        try:
            while select.select([self.fd], [], [], 0)[0]:
                data = os.read(self.fd, 4096)
                if not data:
                    self.disconnect("USB peer closed")
                    return
                for packet in self.parser.feed(data):
                    self.handle_packet(*packet)
        except OSError as exc:
            self.disconnect(str(exc))

    def write_badge(self) -> None:
        if self.fd is None or not self.tx:
            return
        try:
            wrote = os.write(self.fd, self.tx[0])
            if wrote <= 0:
                self.disconnect("USB write returned zero")
            else:
                del self.tx[0][:wrote]
                if not self.tx[0]:
                    self.tx.popleft()
        except OSError as exc:
            self.disconnect(str(exc))

    def refresh_face(self, force: bool = False) -> None:
        if not self.ready or (not force and time.monotonic() < self.next_face):
            return
        self.next_face = time.monotonic() + self.face_seconds
        face = self.web.face()
        if face is None:
            if Image is None:
                self.status("Install python3-pil for face")
            return
        self.queue(PACKET_FACE, face)

    def run(self) -> None:
        self.log("starting; loopback mobile UI only")
        while True:
            if self.fd is None:
                self.connect()
                time.sleep(min(self.tick_seconds, 0.1))
                continue
            self.refresh_face()
            if (self.ready and self.page is not None and not self.face_page and
                    self.page.route.rstrip("/") == "/plugins" and
                    time.monotonic() >= self.next_plugin_refresh):
                # The plugins index is the dynamic page tree.  Reparse it while
                # visible so plugins changed outside the badge appear on their
                # own without a service restart or a LAN connection.
                self.open_page("/plugins", push=False)
            try:
                readable, writable, _ = select.select([self.fd], [self.fd] if self.tx else [], [], self.tick_seconds)
            except OSError as exc:
                self.disconnect(str(exc))
                continue
            if readable:
                self.read_badge()
            if self.fd is not None and writable:
                self.write_badge()


def main() -> int:
    bridge = PwnagotchiBadgeBridge()
    try:
        bridge.run()
    except KeyboardInterrupt:
        bridge.disconnect("stopped")
    return 0


if __name__ == "__main__":
    sys.exit(main())
