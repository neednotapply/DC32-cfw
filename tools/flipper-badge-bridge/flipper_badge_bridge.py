#!/usr/bin/env python3
"""USB bridge from the DC32 Flipper Remote app to Flipper's QFlipper RPC."""

from __future__ import annotations

from collections import deque
import glob
import os
from pathlib import Path
import select
import struct
import time
import zlib

try:
    import fcntl
    import termios
except ImportError:  # Unit tests may run on Windows.
    fcntl = termios = None

MAGIC = b"FRC1"
VERSION = 1
HEADER = struct.Struct("<4sBBHIII")
MAX_PAYLOAD = 1100
FRAME_BYTES = 1024

PACKET_HELLO = 1
PACKET_READY = 2
PACKET_FRAME = 3
PACKET_INPUT = 4
PACKET_UNLOCK = 5
PACKET_STATUS = 6
PACKET_STREAM = 7

RPC_OK = 0
RPC_VERSION_REQUEST = 39
RPC_VERSION_RESPONSE = 40
RPC_START_SCREEN = 20
RPC_STOP_SCREEN = 21
RPC_SCREEN_FRAME = 22
RPC_SEND_INPUT = 23
RPC_STOP_SESSION = 19
RPC_UNLOCK = 67
RPC_DESKTOP_SUBSCRIBE = 68
RPC_DESKTOP_STATUS = 70
MIN_PROTO_MAJOR = 0
MIN_PROTO_MINOR = 16
CLI_PROMPT = b"\r\n\r\n>: "
RPC_START_COMMAND = b"start_rpc_session\r"


def crc(payload: bytes) -> int:
    return zlib.crc32(payload) & 0xFFFFFFFF if payload else 0


def make_packet(packet_type: int, sequence: int, payload: bytes = b"") -> bytes:
    if len(payload) > MAX_PAYLOAD:
        raise ValueError("FRC1 payload exceeds bridge limit")
    return HEADER.pack(MAGIC, VERSION, packet_type, 0, sequence, len(payload), crc(payload)) + payload


class PacketParser:
    """Incremental FRC1 parser that recovers after serial noise."""

    def __init__(self) -> None:
        self.buffer = bytearray()

    def feed(self, data: bytes) -> list[tuple[int, int, bytes]]:
        self.buffer.extend(data)
        packets: list[tuple[int, int, bytes]] = []
        while True:
            start = self.buffer.find(MAGIC)
            if start < 0:
                if len(self.buffer) > len(MAGIC) - 1:
                    del self.buffer[: -(len(MAGIC) - 1)]
                return packets
            if start:
                del self.buffer[:start]
            if len(self.buffer) < HEADER.size:
                return packets
            magic, version, packet_type, _flags, sequence, length, packet_crc = HEADER.unpack_from(self.buffer)
            if magic != MAGIC or version != VERSION or length > MAX_PAYLOAD:
                del self.buffer[0]
                continue
            total = HEADER.size + length
            if len(self.buffer) < total:
                return packets
            payload = bytes(self.buffer[HEADER.size:total])
            del self.buffer[:total]
            if crc(payload) == packet_crc:
                packets.append((packet_type, sequence, payload))


def encode_varint(value: int) -> bytes:
    out = bytearray()
    while value > 0x7F:
        out.append((value & 0x7F) | 0x80)
        value >>= 7
    out.append(value)
    return bytes(out)


def decode_varint(data: bytes, offset: int = 0) -> tuple[int, int]:
    value = shift = 0
    while offset < len(data) and shift < 64:
        byte = data[offset]
        offset += 1
        value |= (byte & 0x7F) << shift
        if not byte & 0x80:
            return value, offset
        shift += 7
    raise ValueError("incomplete or oversized protobuf varint")


def protobuf_field(number: int, wire_type: int, value: int | bytes) -> bytes:
    out = bytearray(encode_varint((number << 3) | wire_type))
    if wire_type == 0:
        out.extend(encode_varint(int(value)))
    elif wire_type == 2:
        assert isinstance(value, bytes)
        out.extend(encode_varint(len(value)))
        out.extend(value)
    else:
        raise ValueError(f"unsupported protobuf wire type {wire_type}")
    return bytes(out)


def protobuf_fields(data: bytes):
    offset = 0
    while offset < len(data):
        key, offset = decode_varint(data, offset)
        number, wire_type = key >> 3, key & 7
        if wire_type == 0:
            value, offset = decode_varint(data, offset)
        elif wire_type == 2:
            size, offset = decode_varint(data, offset)
            end = offset + size
            if end > len(data):
                raise ValueError("truncated protobuf length field")
            value = data[offset:end]
            offset = end
        else:
            raise ValueError(f"unsupported protobuf wire type {wire_type}")
        yield number, wire_type, value


def encode_main(command_id: int, content_tag: int, content: bytes = b"") -> bytes:
    body = protobuf_field(1, 0, command_id) + protobuf_field(content_tag, 2, content)
    return encode_varint(len(body)) + body


def decode_main(data: bytes) -> dict[str, object]:
    result: dict[str, object] = {"command_id": 0, "command_status": RPC_OK, "has_next": False,
                                 "content_tag": 0, "content": b""}
    for number, wire_type, value in protobuf_fields(data):
        if number == 1 and wire_type == 0:
            result["command_id"] = value
        elif number == 2 and wire_type == 0:
            result["command_status"] = value
        elif number == 3 and wire_type == 0:
            result["has_next"] = bool(value)
        elif wire_type == 2:
            result["content_tag"] = number
            result["content"] = value
    return result


class DelimitedParser:
    """Reads PB_ENCODE_DELIMITED messages used by the QFlipper RPC service."""

    def __init__(self) -> None:
        self.buffer = bytearray()

    def feed(self, data: bytes) -> list[bytes]:
        self.buffer.extend(data)
        messages: list[bytes] = []
        while self.buffer:
            try:
                size, start = decode_varint(self.buffer)
            except ValueError:
                break
            if size > 8192:
                self.buffer.clear()
                raise ValueError("oversized QFlipper RPC message")
            end = start + size
            if end > len(self.buffer):
                break
            messages.append(bytes(self.buffer[start:end]))
            del self.buffer[:end]
        return messages


def configure_serial(fd: int, baud: int, dtr: bool = False) -> None:
    if termios is None:
        raise OSError("serial bridge requires Linux termios")
    attrs = termios.tcgetattr(fd)
    attrs[0] = attrs[1] = attrs[3] = 0
    attrs[2] = attrs[2] | termios.CLOCAL | termios.CREAD
    attrs[4] = attrs[5] = termios.B230400 if baud == 230400 else termios.B115200
    attrs[6][termios.VMIN] = attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    if dtr and fcntl is not None and hasattr(termios, "TIOCMBIS"):
        fcntl.ioctl(fd, termios.TIOCMBIS, struct.pack("I", termios.TIOCM_DTR))


class FlipperBadgeBridge:
    """Owns both serial links and translates only the QFlipper RPC subset we use."""

    def __init__(self, environ: dict[str, str] | None = None) -> None:
        env = os.environ if environ is None else environ
        self.badge_device = env.get("BADGE_DEVICE", "").strip()
        self.badge_glob = env.get("BADGE_DEVICE_GLOB", "/dev/serial/by-id/*DC32*Flipper*")
        self.badge_fallback = env.get("BADGE_DEVICE_FALLBACK", "/dev/ttyACM0").strip()
        self.flipper_device = env.get("FLIPPER_DEVICE", "").strip()
        self.flipper_glob = env.get("FLIPPER_DEVICE_GLOB", "/dev/serial/by-id/*Flipper*")
        self.flipper_fallback = env.get("FLIPPER_DEVICE_FALLBACK", "/dev/ttyACM1").strip()
        self.tick_seconds = max(0.001, float(env.get("TICK_MS", "5")) / 1000.0)
        self.badge_fd: int | None = None
        self.flipper_fd: int | None = None
        self.badge_parser = PacketParser()
        self.rpc_parser = DelimitedParser()
        self.badge_tx = bytearray()
        self.flipper_tx = bytearray()
        self.badge_sequence = 0
        self.command_id = 1
        self.pending: dict[int, str] = {}
        self.badge_ready = False
        self.rpc_up = False
        self.stream_enabled = True

        self.stream_active = False
        self.stream_transition = ""
        self.cli_buffer = bytearray()
        self.flipper_state = "down"
        self.last_status = ""
        self.next_connect = 0.0

    def log(self, message: str) -> None:
        print(f"[flipper-badge-bridge] {message}", flush=True)

    @staticmethod
    def find_device(explicit: str, pattern: str, fallback: str) -> str | None:
        if explicit:
            return explicit if Path(explicit).exists() else None
        matches = sorted(glob.glob(pattern))
        return matches[0] if matches else (fallback if fallback and Path(fallback).exists() else None)

    def queue_badge(self, packet_type: int, payload: bytes = b"") -> None:
        self.badge_sequence += 1
        self.badge_tx.extend(make_packet(packet_type, self.badge_sequence, payload))

    def status(self, text: str) -> None:
        text = text[:63]
        if text != self.last_status:
            self.last_status = text
            if self.badge_ready:
                self.queue_badge(PACKET_STATUS, text.encode("utf-8", "replace"))
            self.log(text)

    def connect_badge(self) -> bool:
        path = self.find_device(self.badge_device, self.badge_glob, self.badge_fallback)
        if not path:
            return False
        try:
            self.badge_fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
            configure_serial(self.badge_fd, 115200)
        except OSError as exc:
            self.badge_fd = None
            self.log(f"cannot open badge {path}: {exc}")
            return False
        self.badge_parser = PacketParser()
        self.badge_tx.clear()
        self.badge_ready = False
        self.log(f"badge connected: {path}")
        return True

    def connect_flipper(self) -> bool:
        path = self.find_device(self.flipper_device, self.flipper_glob, self.flipper_fallback)
        if not path:
            return False
        try:
            self.flipper_fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
            configure_serial(self.flipper_fd, 230400, dtr=True)
        except OSError as exc:
            self.flipper_fd = None
            self.log(f"cannot open Flipper {path}: {exc}")
            return False
        self.rpc_parser = DelimitedParser()
        self.flipper_tx.clear()
        self.pending.clear()
        self.rpc_up = False
        self.stream_active = False
        self.stream_transition = ""
        self.flipper_state = "wait_prompt"
        self.cli_buffer.clear()
        self.flipper_tx.extend(b"\r")
        self.status("Opening Flipper RPC")
        self.log(f"Flipper connected: {path}")
        return True

    def disconnect_badge(self, reason: str = "") -> None:
        if self.badge_fd is not None:
            os.close(self.badge_fd)
        self.badge_fd = None
        self.badge_ready = False
        self.badge_tx.clear()
        if reason:
            self.log(f"badge disconnected: {reason}")

    def disconnect_flipper(self, reason: str = "") -> None:
        if self.flipper_fd is not None:
            os.close(self.flipper_fd)
        self.flipper_fd = None
        self.rpc_up = False
        self.flipper_state = "down"
        self.pending.clear()
        self.flipper_tx.clear()
        self.status("Flipper disconnected")
        if reason:
            self.log(f"Flipper disconnected: {reason}")

    def send_rpc(self, content_tag: int, content: bytes = b"", label: str = "") -> int:
        if not self.rpc_up and content_tag not in (RPC_VERSION_REQUEST,):
            return 0
        command_id = self.command_id
        self.command_id += 1
        self.flipper_tx.extend(encode_main(command_id, content_tag, content))
        if label:
            self.pending[command_id] = label
        return command_id

    def begin_rpc(self) -> None:
        self.flipper_state = "rpc"
        self.rpc_up = True
        self.send_rpc(RPC_VERSION_REQUEST, label="protobuf version")

    def start_stream(self) -> None:
        if self.rpc_up and self.stream_enabled and not self.stream_active and self.stream_transition != "start":
            self.send_rpc(RPC_START_SCREEN, label="screen stream")
            self.stream_transition = "start"

    def stop_stream(self) -> None:
        if self.rpc_up and self.stream_active and self.stream_transition != "stop":
            self.send_rpc(RPC_STOP_SCREEN, label="stop screen stream")
            self.stream_transition = "stop"

    def handle_badge_packet(self, packet_type: int, _sequence: int, payload: bytes) -> None:
        if packet_type == PACKET_HELLO:
            if len(payload) != 8:
                self.status("Bad badge HELLO")
                return
            self.badge_ready = True
            self.queue_badge(PACKET_READY)
            self.queue_badge(PACKET_STATUS, (self.last_status or "Waiting for Flipper").encode("utf-8", "replace"))
        elif packet_type == PACKET_INPUT:
            if len(payload) != 2 or payload[0] > 5 or payload[1] > 4:
                self.status("Bad input event")
                return
            event = protobuf_field(1, 0, payload[0]) + protobuf_field(2, 0, payload[1])
            if self.rpc_up:
                self.send_rpc(RPC_SEND_INPUT, event, "input")
            else:
                self.status("Flipper RPC unavailable")
        elif packet_type == PACKET_UNLOCK:
            if self.rpc_up:
                self.send_rpc(RPC_UNLOCK, label="unlock")
            else:
                self.status("Flipper RPC unavailable")
        elif packet_type == PACKET_STREAM and len(payload) == 1 and payload[0] in (0, 1):
            self.stream_enabled = bool(payload[0])
            if self.stream_enabled:
                self.start_stream()
                self.status("Screen stream resumed")
            else:
                self.stop_stream()
                self.status("Screen stream paused")

    def handle_rpc_message(self, data: bytes) -> None:
        try:
            message = decode_main(data)
        except ValueError as exc:
            self.status(f"RPC decode error: {exc}")
            return
        content_tag = int(message["content_tag"])
        content = message["content"]
        assert isinstance(content, bytes)
        command_id = int(message["command_id"])
        status = int(message["command_status"])
        label = self.pending.pop(command_id, "")
        if status != RPC_OK:
            self.status(f"RPC error {label or command_id}: {status}")
            return
        if content_tag == RPC_VERSION_RESPONSE:
            try:
                fields = {number: value for number, _wire, value in protobuf_fields(content)}
                major, minor = int(fields.get(1, 0)), int(fields.get(2, 0))
            except ValueError:
                self.status("Invalid protobuf version")
                return
            if (major, minor) < (MIN_PROTO_MAJOR, MIN_PROTO_MINOR):
                self.status(f"RPC {major}.{minor} too old")
                return
            self.start_stream()
            self.send_rpc(RPC_DESKTOP_SUBSCRIBE, label="desktop status")
            self.status(f"QFlipper RPC {major}.{minor}")
        elif content_tag == RPC_SCREEN_FRAME:
            try:
                fields = {number: value for number, _wire, value in protobuf_fields(content)}
                frame = fields.get(1, b"")
                orientation = int(fields.get(2, 0))
            except ValueError:
                self.status("Invalid screen frame")
                return
            if isinstance(frame, bytes) and len(frame) == FRAME_BYTES and orientation in range(4) and self.badge_ready and self.stream_enabled:
                self.queue_badge(PACKET_FRAME, bytes((orientation,)) + frame)
        elif content_tag == RPC_DESKTOP_STATUS:
            try:
                fields = {number: value for number, _wire, value in protobuf_fields(content)}
                self.status("Flipper locked" if int(fields.get(1, 0)) else "Flipper unlocked")
            except ValueError:
                self.status("Invalid desktop status")
        elif label == "unlock":
            self.status("Unlock requested")
        elif label == "screen stream":
            self.stream_active = True
            self.stream_transition = ""
            if self.stream_enabled:
                self.status("Screen stream active")
            else:
                self.stop_stream()
        elif label == "stop screen stream":
            self.stream_active = False
            self.stream_transition = ""
            if self.stream_enabled:
                self.start_stream()

    def read_badge(self) -> None:
        assert self.badge_fd is not None
        try:
            data = os.read(self.badge_fd, 2048)
        except BlockingIOError:
            return
        if not data:
            self.disconnect_badge("EOF")
            return
        for packet_type, sequence, payload in self.badge_parser.feed(data):
            self.handle_badge_packet(packet_type, sequence, payload)

    def read_flipper(self) -> None:
        assert self.flipper_fd is not None
        try:
            data = os.read(self.flipper_fd, 4096)
        except BlockingIOError:
            return
        if not data:
            self.disconnect_flipper("EOF")
            return
        if self.flipper_state == "wait_prompt":
            self.cli_buffer.extend(data)
            if CLI_PROMPT in self.cli_buffer:
                self.flipper_state = "wait_rpc_echo"
                self.cli_buffer.clear()
                self.flipper_tx.extend(RPC_START_COMMAND)
            elif len(self.cli_buffer) > 4096:
                del self.cli_buffer[:-len(CLI_PROMPT)]
        elif self.flipper_state == "wait_rpc_echo":
            self.cli_buffer.extend(data)
            if self.cli_buffer.endswith(b"\n"):
                self.begin_rpc()
        else:
            try:
                messages = self.rpc_parser.feed(data)
            except ValueError as exc:
                self.disconnect_flipper(str(exc))
                return
            for message in messages:
                self.handle_rpc_message(message)

    @staticmethod
    def flush(fd: int, output: bytearray) -> None:
        if not output:
            return
        try:
            written = os.write(fd, output)
        except BlockingIOError:
            return
        del output[:written]

    def step(self) -> None:
        now = time.monotonic()
        if self.badge_fd is None and now >= self.next_connect:
            self.connect_badge()
            self.next_connect = now + 1.0
        if self.flipper_fd is None and now >= self.next_connect:
            self.connect_flipper()
            self.next_connect = now + 1.0
        read_fds = [fd for fd in (self.badge_fd, self.flipper_fd) if fd is not None]
        if read_fds:
            readable, _, _ = select.select(read_fds, [], [], self.tick_seconds)
            if self.badge_fd in readable:
                self.read_badge()
            if self.flipper_fd in readable:
                self.read_flipper()
        else:
            time.sleep(self.tick_seconds)
        if self.badge_fd is not None:
            self.flush(self.badge_fd, self.badge_tx)
        if self.flipper_fd is not None:
            self.flush(self.flipper_fd, self.flipper_tx)

    def run(self) -> None:
        self.status("Waiting for badge and Flipper")
        try:
            while True:
                self.step()
        except KeyboardInterrupt:
            pass
        finally:
            self.disconnect_badge()
            self.disconnect_flipper()


if __name__ == "__main__":
    FlipperBadgeBridge().run()
