#!/usr/bin/env python3
"""Local USB bridge between a DC32 badge and a running RaspyJack instance."""

from __future__ import annotations

from collections import deque
from io import BytesIO
import glob
import json
import os
from pathlib import Path
import select
import socket
import struct
import sys
try:
    import termios
except ImportError:  # Allows protocol tests to run on non-POSIX development hosts.
    termios = None
try:  # RaspyJack already uses Pillow; retain a stdlib JPEG fallback when absent.
    from PIL import Image
except ImportError:
    Image = None
import time
import zlib

MAGIC = b"RJC2"
VERSION = 1
MAX_FRAME_BYTES = 64 * 1024
MAX_STATUS_BYTES = 63
SOURCE_WIDTH = 128
SOURCE_HEIGHT = 128
RGB565_BYTES = SOURCE_WIDTH * SOURCE_HEIGHT * 2
TILE_SIZE = 8
TILE_BYTES = TILE_SIZE * TILE_SIZE * 2
HEADER = struct.Struct("<4sBBHIII")

PACKET_HELLO = 1
PACKET_READY = 2
PACKET_FRAME = 3
PACKET_INPUT = 4
PACKET_STATUS = 5
PACKET_FRAME_ACK = 6
PACKET_FRAME_RGB565 = 7
PACKET_FRAME_TILES = 8

CAP_FRAME_ACK = 0x80000000
CAP_RGB565 = 0x40000000
CAP_TILES = 0x20000000

BUTTON_NAMES = {
    1: "UP", 2: "DOWN", 3: "LEFT", 4: "RIGHT", 5: "OK",
    6: "KEY1", 7: "KEY2", 8: "KEY3",
}


class PacketParser:
    """Incrementally parse and resynchronize the length-delimited CDC stream."""

    def __init__(self, max_payload: int = MAX_FRAME_BYTES) -> None:
        self.max_payload = max_payload
        self.buffer = bytearray()

    def feed(self, data: bytes) -> list[tuple[int, int, bytes, int]]:
        self.buffer.extend(data)
        packets: list[tuple[int, int, bytes, int]] = []
        while True:
            start = self.buffer.find(MAGIC)
            if start < 0:
                if len(self.buffer) > len(MAGIC) - 1:
                    del self.buffer[: -(len(MAGIC) - 1)]
                break
            if start:
                del self.buffer[:start]
            if len(self.buffer) < HEADER.size:
                break
            magic, version, packet_type, _flags, sequence, length, crc = HEADER.unpack_from(self.buffer)
            if magic != MAGIC or version != VERSION or length > self.max_payload:
                del self.buffer[0]
                continue
            total = HEADER.size + length
            if len(self.buffer) < total:
                break
            payload = bytes(self.buffer[HEADER.size:total])
            del self.buffer[:total]
            packets.append((packet_type, sequence, payload, crc))
        return packets


def packet_crc(payload: bytes) -> int:
    return zlib.crc32(payload) & 0xFFFFFFFF if payload else 0


def packet_is_valid(payload: bytes, crc: int) -> bool:
    return packet_crc(payload) == crc


def make_packet(packet_type: int, sequence: int, payload: bytes = b"") -> bytes:
    if len(payload) > MAX_FRAME_BYTES:
        raise ValueError("payload exceeds protocol frame limit")
    return HEADER.pack(MAGIC, VERSION, packet_type, 0, sequence, len(payload), packet_crc(payload)) + payload


def configure_serial(fd: int) -> None:
    if termios is None:
        raise OSError("CDC serial setup requires Linux termios")
    attrs = termios.tcgetattr(fd)
    attrs[0] = attrs[1] = attrs[3] = 0
    attrs[2] = attrs[2] | termios.CLOCAL | termios.CREAD
    attrs[4] = attrs[5] = termios.B115200
    attrs[6][termios.VMIN] = attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)


class RaspyJackBadgeBridge:
    def __init__(self, environ: dict[str, str] | None = None) -> None:
        env = os.environ if environ is None else environ
        self.frame_path = Path(env.get("FRAME_PATH", "/dev/shm/raspyjack_last.jpg"))
        self.input_sock = env.get("INPUT_SOCK", "/dev/shm/rj_input.sock")
        self.device = env.get("DEVICE", "").strip()
        self.device_glob = env.get("DEVICE_GLOB", "/dev/serial/by-id/*DC32*RaspyJack*")
        self.device_fallback = env.get("DEVICE_FALLBACK", "/dev/ttyACM0").strip()
        self.poll_seconds = 1.0 / max(1.0, float(env.get("POLL_HZ", "10")))
        self.tick_seconds = max(0.001, float(env.get("TICK_MS", "5")) / 1000.0)
        self.diagnostics_seconds = max(1.0, float(env.get("DIAGNOSTICS_SECONDS", "5")))
        self.video_mode = env.get("VIDEO_MODE", "auto").strip().lower()
        self.fd: int | None = None
        self.parser = PacketParser()
        self.badge_ready = False
        self.badge_frame_limit = MAX_FRAME_BYTES
        self.badge_supports_ack = False
        self.badge_supports_rgb565 = False
        self.badge_supports_tiles = False
        self.tx_sequence = self.frame_sequence = 0
        self.last_frame_signature: tuple[int, int] | None = None
        self.latest_frame: dict[str, object] | None = None
        self.last_presented_rgb565: bytes | None = None
        self.inflight_sequence: int | None = None
        self.inflight_started = self.inflight_source_seen = 0.0
        self.inflight_rgb565: bytes | None = None
        self.tx: dict[str, object] | None = None
        self.control_tx: deque[tuple[int, bytes]] = deque()
        self.last_status = ""
        self.next_source_poll = self.next_diagnostics = 0.0
        self.stats = {
            "source_frames": 0, "frames_sent": 0, "frames_acked": 0,
            "frames_rejected": 0, "frames_replaced": 0, "frame_bytes": 0,
            "jpeg_frames": 0, "rgb565_frames": 0, "tile_frames": 0,
            "transport_total": 0.0, "transport_count": 0,
            "presentation_total": 0.0, "presentation_count": 0,
            "end_to_end_total": 0.0, "end_to_end_count": 0,
        }

    def log(self, message: str) -> None:
        print(f"[raspyjack-badge-bridge] {message}", flush=True)

    def find_device(self) -> str | None:
        if self.device:
            return self.device if Path(self.device).exists() else None
        matches = sorted(glob.glob(self.device_glob))
        return matches[0] if matches else (self.device_fallback if self.device_fallback and Path(self.device_fallback).exists() else None)

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
        self.reset_transport()
        self.log(f"connected to {path}")
        return True

    def reset_transport(self) -> None:
        self.parser = PacketParser()
        self.badge_ready = self.badge_supports_ack = False
        self.badge_supports_rgb565 = self.badge_supports_tiles = False
        self.badge_frame_limit = MAX_FRAME_BYTES
        self.last_frame_signature = None
        self.latest_frame = None
        self.last_presented_rgb565 = self.inflight_rgb565 = None
        self.inflight_sequence = None
        self.inflight_started = self.inflight_source_seen = 0.0
        self.tx = None
        self.control_tx.clear()
        self.last_status = ""
        self.next_source_poll = 0.0

    def disconnect(self, reason: str = "") -> None:
        if self.fd is not None:
            try:
                os.close(self.fd)
            except OSError:
                pass
        self.fd = None
        self.reset_transport()
        if reason:
            self.log(f"disconnected: {reason}")

    def queue_packet(self, packet_type: int, payload: bytes = b"") -> None:
        self.control_tx.append((packet_type, payload))

    def send_status(self, status: str) -> None:
        status = status[:MAX_STATUS_BYTES]
        if self.badge_ready and status != self.last_status:
            self.last_status = status
            self.queue_packet(PACKET_STATUS, status.encode("utf-8", "replace"))

    def forward_input(self, payload: bytes) -> None:
        if len(payload) != 2 or payload[0] not in BUTTON_NAMES or payload[1] not in (0, 1):
            return
        message = {"type": "input", "button": BUTTON_NAMES[payload[0]], "state": "press" if payload[1] else "release"}
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as client:
                client.sendto(json.dumps(message, separators=(",", ":")).encode(), self.input_sock)
        except OSError as exc:
            self.log(f"input socket unavailable: {exc}")
            self.send_status("RaspyJack input unavailable")

    def handle_frame_ack(self, payload: bytes) -> None:
        if len(payload) != 5 or self.inflight_sequence is None:
            return
        sequence, presented = struct.unpack("<IB", payload)
        if sequence != self.inflight_sequence:
            return
        now = time.monotonic()
        presentation = max(0.0, now - self.inflight_started)
        end_to_end = max(0.0, now - self.inflight_source_seen)
        self.inflight_sequence = None
        self.inflight_started = self.inflight_source_seen = 0.0
        if presented:
            self.stats["frames_acked"] += 1
            self.stats["presentation_total"] += presentation
            self.stats["presentation_count"] += 1
            self.stats["end_to_end_total"] += end_to_end
            self.stats["end_to_end_count"] += 1
            self.last_presented_rgb565 = self.inflight_rgb565
        else:
            self.stats["frames_rejected"] += 1
            # A rejected tile means the badge may no longer share our base.
            # Re-read the current source next tick and resend a complete raw frame.
            self.last_presented_rgb565 = None
            self.last_frame_signature = None
            self.latest_frame = None
        self.inflight_rgb565 = None

    def handle_packet(self, packet_type: int, _sequence: int, payload: bytes, crc: int) -> None:
        if not packet_is_valid(payload, crc):
            self.send_status("Bad badge packet CRC")
            return
        if packet_type == PACKET_HELLO:
            if len(payload) != 12:
                self.send_status("Bad HELLO packet")
                return
            limit, _width, _height, capabilities = struct.unpack("<IHHI", payload)
            self.badge_frame_limit = min(MAX_FRAME_BYTES, max(16 * 1024, limit))
            self.badge_supports_ack = bool(capabilities & CAP_FRAME_ACK)
            self.badge_supports_rgb565 = bool(capabilities & CAP_RGB565)
            self.badge_supports_tiles = self.badge_supports_rgb565 and bool(capabilities & CAP_TILES)
            self.badge_ready = True
            self.last_frame_signature = None
            self.latest_frame = None
            self.last_presented_rgb565 = self.inflight_rgb565 = None
            self.inflight_sequence = None
            self.tx = None
            self.control_tx.clear()
            self.last_status = ""
            self.queue_packet(PACKET_READY)
            self.send_status("Waiting for RaspyJack")
        elif packet_type == PACKET_INPUT:
            self.forward_input(payload)
        elif packet_type == PACKET_FRAME_ACK:
            self.handle_frame_ack(payload)

    def read_badge(self) -> None:
        if self.fd is None:
            return
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

    def read_stable_frame(self) -> tuple[bytes, tuple[int, int]] | None:
        try:
            before = self.frame_path.stat()
            if before.st_size < 4 or before.st_size > self.badge_frame_limit:
                return None
            data = self.frame_path.read_bytes()
            after = self.frame_path.stat()
        except OSError:
            return None
        signature = (after.st_mtime_ns, after.st_size)
        if (before.st_mtime_ns, before.st_size) != signature:
            return None
        if len(data) != after.st_size or not data.startswith(b"\xff\xd8") or not data.endswith(b"\xff\xd9"):
            return None
        return data, signature

    @staticmethod
    def jpeg_to_rgb565(frame: bytes) -> bytes | None:
        if Image is None:
            return None
        try:
            with Image.open(BytesIO(frame)) as image:
                if image.size != (SOURCE_WIDTH, SOURCE_HEIGHT):
                    return None
                rgb = image.convert("RGB").tobytes()
        except Exception:
            return None
        out = bytearray(RGB565_BYTES)
        for src, dst in zip(range(0, len(rgb), 3), range(0, len(out), 2)):
            value = ((rgb[src] & 0xF8) << 8) | ((rgb[src + 1] & 0xFC) << 3) | (rgb[src + 2] >> 3)
            out[dst] = value & 0xFF
            out[dst + 1] = value >> 8
        return bytes(out)

    @staticmethod
    def make_tile_payload(previous: bytes, current: bytes) -> bytes | None:
        if len(previous) != RGB565_BYTES or len(current) != RGB565_BYTES:
            return None
        records: list[bytes] = []
        for tile_y in range(SOURCE_HEIGHT // TILE_SIZE):
            for tile_x in range(SOURCE_WIDTH // TILE_SIZE):
                block = bytearray()
                changed = False
                for row in range(TILE_SIZE):
                    offset = ((tile_y * TILE_SIZE + row) * SOURCE_WIDTH + tile_x * TILE_SIZE) * 2
                    line = current[offset:offset + TILE_SIZE * 2]
                    if line != previous[offset:offset + TILE_SIZE * 2]:
                        changed = True
                    block.extend(line)
                if changed:
                    records.append(bytes((tile_x, tile_y)) + bytes(block))
        if not records:
            return None
        payload = struct.pack("<HHBH", SOURCE_WIDTH, SOURCE_HEIGHT, TILE_SIZE, len(records)) + b"".join(records)
        return payload if len(payload) < 4 + RGB565_BYTES else None

    def raw_enabled(self) -> bool:
        return self.video_mode != "jpeg" and self.badge_supports_rgb565 and Image is not None

    def make_video_frame(self, jpeg: bytes, source_seen: float) -> dict[str, object]:
        rgb565 = self.jpeg_to_rgb565(jpeg) if self.raw_enabled() else None
        if rgb565 is None:
            return {"type": PACKET_FRAME, "payload": jpeg, "source_seen": source_seen, "rgb565": None}
        payload = struct.pack("<HH", SOURCE_WIDTH, SOURCE_HEIGHT) + rgb565
        packet_type = PACKET_FRAME_RGB565
        if self.badge_supports_tiles and self.last_presented_rgb565 is not None:
            tiles = self.make_tile_payload(self.last_presented_rgb565, rgb565)
            if tiles is not None:
                packet_type, payload = PACKET_FRAME_TILES, tiles
        return {"type": packet_type, "payload": payload, "source_seen": source_seen, "rgb565": rgb565}

    def poll_source(self, now: float) -> None:
        if now < self.next_source_poll:
            return
        self.next_source_poll = now + self.poll_seconds
        if not self.badge_ready:
            return
        result = self.read_stable_frame()
        if result is None:
            self.send_status("Waiting for RaspyJack")
            return
        jpeg, signature = result
        if signature == self.last_frame_signature:
            return
        self.last_frame_signature = signature
        self.stats["source_frames"] += 1
        if self.latest_frame is not None:
            self.stats["frames_replaced"] += 1
        self.latest_frame = self.make_video_frame(jpeg, now)
        self.send_status("Streaming RaspyJack")

    def start_next_tx(self) -> bool:
        if self.tx is not None:
            return False
        if self.control_tx:
            packet_type, payload = self.control_tx.popleft()
            self.tx_sequence = (self.tx_sequence + 1) & 0xFFFFFFFF
            self.tx = {"data": make_packet(packet_type, self.tx_sequence, payload), "offset": 0,
                       "kind": "control", "sequence": self.tx_sequence}
            return True
        if not self.badge_ready or self.latest_frame is None or (self.badge_supports_ack and self.inflight_sequence is not None):
            return False
        video = self.latest_frame
        self.latest_frame = None
        self.frame_sequence = (self.frame_sequence + 1) & 0xFFFFFFFF
        self.tx = {"data": make_packet(video["type"], self.frame_sequence, video["payload"]), "offset": 0,
                   "kind": "frame", "sequence": self.frame_sequence, "source_seen": video["source_seen"],
                   "video_type": video["type"], "rgb565": video["rgb565"]}
        return True

    def tx_chunk(self, limit: int | None = None) -> bytes:
        if self.tx is None:
            return b""
        data, offset = self.tx["data"], self.tx["offset"]
        assert isinstance(data, bytes) and isinstance(offset, int)
        return data[offset:] if limit is None else data[offset:offset + limit]

    def advance_tx(self, written: int) -> None:
        if self.tx is None or written <= 0:
            return
        data, offset = self.tx["data"], self.tx["offset"]
        assert isinstance(data, bytes) and isinstance(offset, int)
        offset = min(len(data), offset + written)
        self.tx["offset"] = offset
        if offset != len(data):
            return
        completed = self.tx
        self.tx = None
        if completed["kind"] != "frame":
            return
        source_seen, video_type, rgb565 = completed["source_seen"], completed["video_type"], completed["rgb565"]
        sequence = completed["sequence"]
        assert isinstance(source_seen, float) and isinstance(video_type, int) and isinstance(sequence, int)
        self.stats["frames_sent"] += 1
        self.stats["frame_bytes"] += len(data) - HEADER.size
        self.stats[{PACKET_FRAME: "jpeg_frames", PACKET_FRAME_RGB565: "rgb565_frames", PACKET_FRAME_TILES: "tile_frames"}[video_type]] += 1
        self.stats["transport_total"] += max(0.0, time.monotonic() - source_seen)
        self.stats["transport_count"] += 1
        if self.badge_supports_ack:
            self.inflight_sequence = sequence
            self.inflight_started = time.monotonic()
            self.inflight_source_seen = source_seen
            self.inflight_rgb565 = rgb565 if isinstance(rgb565, bytes) else None
        elif isinstance(rgb565, bytes):
            self.last_presented_rgb565 = rgb565

    def pump_tx(self) -> None:
        if self.fd is None:
            return
        self.start_next_tx()
        chunk = self.tx_chunk()
        if not chunk:
            return
        try:
            wrote = os.write(self.fd, chunk)
            if wrote <= 0:
                self.disconnect("USB write returned zero")
            else:
                self.advance_tx(wrote)
        except OSError as exc:
            self.disconnect(str(exc))

    def log_diagnostics(self, now: float) -> None:
        if now < self.next_diagnostics:
            return
        self.next_diagnostics = now + self.diagnostics_seconds
        def mean(total: str, count: str) -> float:
            return 0.0 if not self.stats[count] else 1000.0 * self.stats[total] / self.stats[count]
        self.log(
            "frames source={source_frames} sent={frames_sent} acked={frames_acked} "
            "jpeg={jpeg_frames} rgb565={rgb565_frames} tiles={tile_frames} replaced={frames_replaced} "
            "bytes={frame_bytes} transport_mean_ms={transport:.1f} present_mean_ms={present:.1f} "
            "end_to_end_mean_ms={end_to_end:.1f} ack_mode={ack}".format(
                **self.stats, transport=mean("transport_total", "transport_count"),
                present=mean("presentation_total", "presentation_count"),
                end_to_end=mean("end_to_end_total", "end_to_end_count"),
                ack="on" if self.badge_supports_ack else "legacy"))

    def run(self) -> None:
        self.log("starting")
        while True:
            if self.fd is None:
                self.connect()
                time.sleep(min(self.tick_seconds, 0.1))
                continue
            now = time.monotonic()
            self.poll_source(now)
            self.start_next_tx()
            try:
                readable, writable, _ = select.select([self.fd], [self.fd] if self.tx is not None else [], [], self.tick_seconds)
            except OSError as exc:
                self.disconnect(str(exc))
                continue
            if readable:
                self.read_badge()
            if self.fd is None:
                continue
            now = time.monotonic()
            self.poll_source(now)
            if writable:
                self.pump_tx()
            self.log_diagnostics(now)


def main() -> int:
    bridge = RaspyJackBadgeBridge()
    try:
        bridge.run()
    except KeyboardInterrupt:
        bridge.disconnect("stopped")
    return 0


if __name__ == "__main__":
    sys.exit(main())
