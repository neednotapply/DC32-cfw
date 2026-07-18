#!/usr/bin/env python3
"""Parse Flipper IR captures and describe the NEC/OpenLASIR packet surface."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


MODE_NAMES = (
    "laser_tag_fire", "user_presence_announcement", "base_station_presence_announcement",
    "user_to_user_handshake_initiation", "user_to_user_handshake_response",
    "user_to_base_station_handshake_initiation", "user_to_base_station_handshake_response",
    "base_station_to_user_handshake_initiation", "base_station_to_user_handshake_response",
    "color_set_temporary", "color_set_permanent", "general_interact",
)
COLOR_NAMES = ("Cyan", "Magenta", "Yellow", "Green", "Red", "Blue", "Orange", "White")


def classify_raw(raw: int) -> dict[str, object]:
    block = raw & 0xFF
    inverse = (raw >> 8) & 0xFF
    command_low = (raw >> 16) & 0xFF
    command_high = (raw >> 24) & 0xFF
    if block ^ inverse != 0xFF:
        kind = "invalid"
    elif command_low ^ command_high == 0xFF:
        kind = "standard_nec"
    else:
        kind = "openlasir"
    result: dict[str, object] = {
        "raw": f"{raw:08X}",
        "kind": kind,
        "block_id": block,
        "block_inverse": inverse,
        "command_low": command_low,
        "command_high": command_high,
    }
    if kind == "openlasir":
        mode = command_high & 0x1F
        data = command_high >> 5
        result.update({
            "device_id": command_low,
            "mode": mode,
            "mode_name": MODE_NAMES[mode] if mode < len(MODE_NAMES) else "reserved",
            "data": data,
            "color": COLOR_NAMES[data],
        })
    return result


def _raw_to_frame(values: list[int]) -> int:
    spaces = values[1::2]
    if len(spaces) < 33:
        raise ValueError("raw record has fewer than 32 data spaces")
    if not 3600 <= spaces[0] <= 5400:
        raise ValueError("raw record has no NEC-style leading space")
    bits = [1 if duration > 1000 else 0 for duration in spaces[1:33]]
    return sum(bit << index for index, bit in enumerate(bits))


def parse_flipper_ir(path: Path) -> list[dict[str, object]]:
    """Parse raw and NECext Flipper records into classified 32-bit frames."""
    text = path.read_text(encoding="utf-8", errors="replace")
    records: list[dict[str, object]] = []
    for body in text.split("#"):
        name_match = re.search(r"^\s*name:\s*([^\r\n]+)", body, re.M)
        type_match = re.search(r"^\s*type:\s*([^\r\n]+)", body, re.M)
        if not name_match or not type_match:
            continue
        name = name_match.group(1).strip()
        record_type = type_match.group(1).strip()
        if record_type == "raw":
            data_match = re.search(r"^\s*data:\s*([^\r\n]+)", body, re.M)
            if not data_match:
                raise ValueError(f"{name}: raw record has no data")
            raw = _raw_to_frame([int(value) for value in re.findall(r"\d+", data_match.group(1))])
        elif record_type == "parsed":
            address_match = re.search(r"^\s*address:\s*([^\r\n]+)", body, re.M)
            command_match = re.search(r"^\s*command:\s*([^\r\n]+)", body, re.M)
            if not address_match or not command_match:
                raise ValueError(f"{name}: parsed record is missing address or command")
            address = [int(value, 16) for value in address_match.group(1).split()]
            command = [int(value, 16) for value in command_match.group(1).split()]
            if len(address) < 2 or len(command) < 2:
                raise ValueError(f"{name}: parsed record is too short")
            raw = address[0] | (address[1] << 8) | (command[0] << 16) | (command[1] << 24)
        else:
            continue
        record = {"name": name, "record_type": record_type}
        record.update(classify_raw(raw))
        records.append(record)
    return records


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path, help="Flipper .ir capture")
    args = parser.parse_args()
    print(json.dumps(parse_flipper_ir(args.capture), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
