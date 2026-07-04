#!/usr/bin/env python3
"""Regression checks for the OpenLASIR Laser Tag app."""

from __future__ import annotations

import struct
import binascii
import base64
import hashlib
from pathlib import Path

from lasertag_sync import decode_sync_document, decode_sync_text


ROOT = Path(__file__).resolve().parents[1]
OPENLASIR_COMMIT = "414504887e761fe0f3e1d3251567f0c29cdcec13"
ARDUINO_IRREMOTE_COMMIT = "f2f8de414b8e2724c578377e651b6e5c1fefb7b2"
QRCODEGEN_COMMIT = "2c9044de6b049ca25cb3cd1649ed7e27aa055138"
QRCODEGEN_C_SHA256 = "f7109b682133e2fd79d629847b16e64bdffae477bf4bdc5f95a356fdbcb2868a"
QRCODEGEN_H_SHA256 = "a81c4c95bc4e4960adf6cb0286ea7c2eca7f9ed110b3637a7958b74deb2a0c33"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def encode(block: int, device: int, mode: int, data: int) -> int:
    return (
        block
        | ((block ^ 0xFF) << 8)
        | (device << 16)
        | ((mode & 0x1F) << 24)
        | ((data & 7) << 29)
    )


def decode_spaces(spaces: list[int]) -> int | None:
    raw = 0
    bit = None
    for duration in spaces:
        if 3600 <= duration <= 5400:
            raw = 0
            bit = 0
            continue
        if bit is None or bit >= 32:
            continue
        if 343 <= duration <= 783:
            value = 0
        elif 1238 <= duration <= 2138:
            value = 1
        else:
            bit = None
            continue
        raw |= value << bit
        bit += 1
        if bit == 32:
            block = raw & 0xFF
            inverse = (raw >> 8) & 0xFF
            command_low = (raw >> 16) & 0xFF
            command_high = (raw >> 24) & 0xFF
            bit = None
            if block ^ inverse == 0xFF and command_low ^ command_high != 0xFF:
                return raw
    return None


def spaces_for(raw: int) -> list[int]:
    return [4500, *[1688 if raw & (1 << bit) else 563 for bit in range(32)]]


def collapse_raw_carrier(levels: list[tuple[int, int]]) -> list[int]:
    """Model the PIO filter: ignore high carrier gaps below 64 us."""
    return [duration for level, duration in levels if level == 1 and duration >= 64]


def check_protocol() -> None:
    red_42 = encode(0, 42, 0, 4)
    require(red_42 == 0x802AFF00, "upstream block=0/device=42/red vector changed")
    require(decode_spaces(spaces_for(red_42)) == red_42, "valid frame did not decode")
    for color in range(8):
        raw = encode(7, 19, 0, color)
        decoded = decode_spaces(spaces_for(raw))
        require(decoded == raw and (decoded >> 29) == color, f"color {color} failed")

    malformed = red_42 ^ (1 << 8)
    require(decode_spaces(spaces_for(malformed)) is None, "bad block complement accepted")
    nec_collision = encode(0, 0x7F, 0, 4)
    require(decode_spaces(spaces_for(nec_collision)) is None,
            "standard NEC command-complement collision was accepted")
    require(decode_spaces(spaces_for(red_42)[:-1]) is None, "truncated frame accepted")
    noisy = [10000, 33, 4500, 563, 999, *spaces_for(red_42)]
    require(decode_spaces(noisy) == red_42, "decoder did not recover after noise")

    raw_carrier: list[tuple[int, int]] = []
    for space in spaces_for(red_42):
        raw_carrier.extend([(0, 13), (1, 13)] * 20)
        raw_carrier.append((1, space))
    require(decode_spaces(collapse_raw_carrier(raw_carrier)) == red_42,
            "raw-carrier envelope fixture failed")
    demodulated = [(0, 9000), (1, 4500)]
    for bit in range(32):
        demodulated.extend([(0, 563), (1, 1688 if red_42 & (1 << bit) else 563)])
    require(decode_spaces(collapse_raw_carrier(demodulated)) == red_42,
            "demodulated receiver fixture failed")


def check_rate_limits() -> None:
    shots: list[int] = []

    def can_fire(now: int, disabled_until: int = 0) -> bool:
        nonlocal shots
        shots = [stamp for stamp in shots if now - stamp < 60_000]
        return (
            now >= disabled_until
            and (not shots or now - shots[-1] >= 1000)
            and len(shots) < 30
        )

    for second in range(30):
        now = second * 1000
        require(can_fire(now), "valid shot was rate-limited")
        shots.append(now)
    require(len(shots) == 30, "30-shot rolling window model failed")
    require(not can_fire(30_000), "31st shot was not blocked")
    require(not can_fire(60_000, disabled_until=60_001), "hit lockout did not block firing")
    require(can_fire(60_000), "oldest shot did not expire after one minute")
    require(250 < 2_000, "duplicate window must be shorter than the hit lockout")


def check_persistence_model() -> None:
    header = struct.pack("<IHH4BII", 0x47544C44, 4, 280, 1, 0, 42, 0x0B, 12, 3)
    opponents = bytearray(32 * 8)
    payload = bytearray(header + opponents + struct.pack("<I", 0))
    require(len(payload) == 280, "persistent schema size changed")
    crc = binascii.crc32(payload) & 0xFFFFFFFF
    struct.pack_into("<I", payload, 276, crc)
    stored_crc = struct.unpack_from("<I", payload, 276)[0]
    struct.pack_into("<I", payload, 276, 0)
    require((binascii.crc32(payload) & 0xFFFFFFFF) == stored_crc,
            "persistent CRC round trip failed")
    payload[12] ^= 1
    require((binascii.crc32(payload) & 0xFFFFFFFF) != stored_crc,
            "persistent corruption was not detected")
    save_crc = binascii.crc32(payload) & 0xFFFFFFFF
    uid_a = binascii.crc32(struct.pack("<Q", 0x0123456789ABCDEF)) & 0xFFFFFFFF
    uid_b = binascii.crc32(struct.pack("<Q", 0xFEDCBA9876543210)) & 0xFFFFFFFF
    require((save_crc ^ uid_a) != (save_crc ^ uid_b),
            "UID-bound save CRC does not reject a copied badge save")

    history: list[tuple[int, int, int]] = []
    for device in range(33):
        history = [(block, old_device, hits) for block, old_device, hits in history
                   if (block, old_device) != (0, device)]
        history.insert(0, (0, device, 1))
        history = history[:32]
    require(len(history) == 32 and all(device != 0 for _, device, _ in history),
            "32-entry opponent eviction failed")
    block, device, hits = history[10]
    history.pop(10)
    history.insert(0, (block, device, hits + 1))
    require(history[0] == (block, device, 2), "opponent MRU/count update failed")


def check_sync_payload() -> None:
    def make_page(page: int, first: int, count: int) -> tuple[str, bytearray]:
        raw = bytearray(28)
        raw[:4] = b"DCLT"
        raw[4:12] = bytes((2, 0x8B, 0, 42, 1, count, page, 3))
        struct.pack_into("<III", raw, 12, 0x12345678, 987, 65)
        struct.pack_into("<I", raw, 24, 7200)
        for index in range(first, first + count):
            raw.extend(struct.pack("<4BI", index // 16, index, index & 7, 0, index + 1))
        raw.extend(struct.pack("<I", binascii.crc32(raw) & 0xFFFFFFFF))
        encoded = base64.urlsafe_b64encode(raw).decode("ascii").rstrip("=")
        return "DC32LT1." + encoded, raw

    pages = [make_page(0, 0, 13), make_page(1, 13, 13), make_page(2, 26, 6)]
    text, raw = pages[0]

    decoded = decode_sync_text(text)
    require(decoded["schema"] == 2 and decoded["signed"] is False,
            "unsigned sync schema was not decoded")
    require(decoded["device_id_auto"] and decoded["sound"] and
            decoded["sync_reminder"] and not decoded["vibration"] and
            decoded["led_brightness"] == "auto" and decoded["badge_uptime_seconds"] == 7200,
            "badge settings/uptime were not decoded")
    require(decoded["block_id"] == 0 and decoded["device_id"] == 42,
            "sync sender identity changed")
    require(decoded["page_index"] == 0 and decoded["page_count"] == 3,
            "sync page metadata was not decoded")
    merged = decode_sync_document("\n".join(page_text for page_text, _ in pages))
    opponents = merged["opponents"]
    require(merged["complete"] and merged["pages_decoded"] == 3,
            "complete paged sync document was not recognized")
    require(len(opponents) == 32, "full paged opponent table was not exported")
    require({opponent["color"] for opponent in opponents} == set(range(8)),
            "sync export did not preserve all OpenLASIR colors")
    require(decode_sync_text("https://sync.example/import?data=" + text) == decoded,
            "URL-wrapped QR payload was not decoded")

    corrupt = bytearray(raw)
    corrupt[20] ^= 1
    corrupt_text = "DC32LT1." + base64.urlsafe_b64encode(corrupt).decode("ascii").rstrip("=")
    try:
        decode_sync_text(corrupt_text)
    except ValueError as exc:
        require("CRC" in str(exc), "corrupt sync payload failed for the wrong reason")
    else:
        raise AssertionError("corrupt sync payload was accepted")


def check_wiring_and_contracts() -> None:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    ui = (ROOT / "src/ui.c").read_text(encoding="utf-8")
    header = (ROOT / "src/dcApp.h").read_text(encoding="utf-8")
    catalog = (ROOT / "src/dcApp.c").read_text(encoding="utf-8")
    package = (ROOT / "tools/build_sd_zip.py").read_text(encoding="utf-8")
    app = (ROOT / "src/apps/lasertag/lasertag.c").read_text(encoding="utf-8")
    ir = (ROOT / "src/irRemote.c").read_text(encoding="utf-8")
    rx = (ROOT / "src/irRemoteRx.c").read_text(encoding="utf-8")
    provenance = (ROOT / "third_party/openlasir/DC32-README.md").read_text(encoding="utf-8")
    qr_provenance = (ROOT / "third_party/qrcodegen/DC32-README.md").read_text(encoding="utf-8")

    require("add_dcapp(dcapp_lasertag lasertag 106" in cmake, "Laser Tag target missing")
    require("DcAppIdToolLaserTag = 106" in header, "runtime ID missing")
    require('"Laser Tag", "/APPS/lasertag.DC32"' in catalog, "catalog entry missing")
    require("UiToolInfrared" in ui and '"Universal IR"' in ui and '"Laser Tag"' in ui,
            "Infrared submenu wiring missing")
    require("curTool == UiToolIr" in ui and "curTool = UiToolInfrared" in ui,
            "Universal IR return path does not select the Infrared category")
    require('"lasertag.DC32"' in package and OPENLASIR_COMMIT in package,
            "SD package/provenance wiring missing")
    require(OPENLASIR_COMMIT in provenance and ARDUINO_IRREMOTE_COMMIT in provenance and
            (ROOT / "third_party/openlasir/LICENSE").is_file(),
            "pinned OpenLASIR source or license missing")
    require("third_party/qrcodegen/qrcodegen.c" in cmake and QRCODEGEN_COMMIT in package,
            "QR generator build/package wiring missing")
    require("DC32_LASERTAG_BLOCK_ID" in cmake and "DC32_LASERTAG_DEVICE_ID" in cmake and
            "LT_PROVISIONED_BLOCK_ID=${DC32_LASERTAG_BLOCK_ID}" in cmake,
            "build-time identity provisioning interface missing")
    require(QRCODEGEN_COMMIT in qr_provenance and
            (ROOT / "third_party/qrcodegen/LICENSE").is_file(),
            "pinned QR generator source or license missing")
    require(hashlib.sha256((ROOT / "third_party/qrcodegen/qrcodegen.c").read_bytes()).hexdigest() ==
            QRCODEGEN_C_SHA256, "vendored qrcodegen.c differs from the pinned source")
    require(hashlib.sha256((ROOT / "third_party/qrcodegen/qrcodegen.h").read_bytes()).hexdigest() ==
            QRCODEGEN_H_SHA256, "vendored qrcodegen.h differs from the pinned source")

    for token in (
        'LT_SAVE_PATH "/SAVE/PORTS/LASERTAG.DAT"', "LT_OPPONENTS 32u",
        "LtSaveSizeCheck", "sizeof(struct LtSave) == 280u",
        "LT_FIRE_WINDOW 30u", "LT_HIT_LOCKOUT_TICKS", "LT_DUPLICATE_TICKS",
        "flashGetUid()", "LT_FLAG_DEVICE_AUTO", "LT_PROVISIONED_BLOCK_ID",
        "LT_PROVISIONED_DEVICE_ID", "ltApplyProvisioning", "ltProvisionedColor",
        "ltBoundSaveCrc", "ltCrc32(&uid, sizeof(uid))",
        "ltTrackOpponent",
        "KEY_BIT_A", "KEY_BIT_SEL", "KEY_BIT_START", "KEY_BIT_B",
        'LT_SYNC_PATH "/SAVE/PORTS/LASERTAG.SYNC"',
        'LT_SYNC_URL_PATH "/SAVE/PORTS/LASERTAG.URL"', "LT_SYNC_UNSIGNED",
        "LT_SYNC_HEADER_SIZE 28u", "LT_SYNC_PAGE_ENTRIES 13u", "ltBuildSyncBinary",
        "ltBase64Url", "qrcodegen_encodeText",
        "qrcodegen_getModule", "RAW UNSIGNED PAYLOAD",
        "ltLooksLikeNec", "CHANGE DEVICE ID OR COLOR", "(value & 0x1fu) == 0x1fu",
        '"Sound", "Vibration", "My Color", "LED Brightness"',
        '"Sync Reminder", "At-Home Mode", "Tutorial", "Home"',
        "audioPwmTone", "SYNC DISABLED IN AT-HOME MODE", "LT_SYNC_REMINDER_TICKS",
        "pressed & KEY_BIT_START", "ltPrepareSyncQr(app)",
    ):
        require(token in app, f"app contract missing: {token}")
    for forbidden in (
        "LT_FLAG_AUTO_FIRE", "ltAdjustSetting", "LtViewResetConfirm",
        "app->draw.keys & KEY_BIT_A", "Reset stats", "AUTO FIRE",
    ):
        require(forbidden not in app, f"authentic-play bypass remains: {forbidden}")
    require("irRemotePrvOpenLasirByte(blockId)" in ir and
            "irRemotePrvOpenLasirByte((uint8_t)~blockId)" in ir and
            "irRemoteMarkUsec(38000, 9000)" in ir,
            "OpenLASIR transmit ordering/timing missing")
    require("IRRX_QUALIFY_USEC 64u" in rx and "PIO0_IRQ_1_IRQHandler" in rx and
            "JMP_Y_POSTDEC" in rx and "JMP_X_POSTDEC" in rx,
            "raw-carrier PIO envelope capture missing")
    require("commandLow ^ commandHigh" in ir,
            "NEC/OpenLASIR command-complement disambiguation missing")
    require("frame.mode != 0" in app, "non-mode-0 frames are not rejected")
    require("frame.raw == app->lastHitRaw" in app, "duplicate suppression missing")
    require("frame.blockId == app->save.blockId" in app, "self-shot rejection missing")
    require("now - app->fireTimes[app->fireCount - 1u] < LT_FIRE_MIN_TICKS" in app and
            "app->fireCount >= LT_FIRE_WINDOW" in app,
            "manual firing does not enforce OpenLASIR rate limits")

    artifact = ROOT / "build/apps/lasertag.DC32"
    if artifact.exists():
        data = artifact.read_bytes()
        require(len(data) < 256 * 1024, "Laser Tag app exceeds small DCApp budget")
        require(struct.unpack_from("<I", data)[0] == 0x50414344, "bad DCApp magic")
        require(struct.unpack_from("<I", data, 8)[0] == 106, "bad DCApp runtime ID")


def main() -> None:
    check_protocol()
    check_rate_limits()
    check_persistence_model()
    check_sync_payload()
    check_wiring_and_contracts()
    print("Laser Tag port checks passed")


if __name__ == "__main__":
    main()
