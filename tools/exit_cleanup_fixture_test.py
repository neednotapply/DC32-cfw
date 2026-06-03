from pathlib import Path
import argparse
import sys


DEFAULT_BADUSB = Path.home() / "Desktop" / "DC32-SD" / "BADUSB" / "RickRoll_YT_Win.txt"
DEFAULT_IR = Path.home() / "Desktop" / "DC32-SD" / "IR" / "tv.ir"


def report(ok: bool, message: str) -> bool:
    if ok:
        print(f"PASS: {message}")
        return True
    print(f"FAIL: {message}", file=sys.stderr)
    return False


def check_badusb(path: Path) -> bool:
    if not path.exists():
        print(f"SKIP: BadUSB fixture not found: {path}")
        return True

    data = path.read_bytes()
    lines = data.decode("utf-8", errors="replace").splitlines()
    ok = True

    ok &= report(len(lines) == 10, "RickRoll BadUSB fixture has 10 parsed lines")
    ok &= report(bool(lines) and lines[-1].strip().upper() == "ENTER", "RickRoll BadUSB fixture ends on ENTER")
    ok &= report(data.endswith(b"\n") or data.endswith(b"\r"), "RickRoll BadUSB fixture has trailing newline EOF shape")
    return ok


def parse_ir_records(path: Path) -> list[dict[str, object]]:
    records: list[dict[str, object]] = []
    current: dict[str, object] | None = None
    power_idx = 0

    for line_no, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        if line.startswith("name:"):
            name = line.split(":", 1)[1].strip()
            current = {
                "record": len(records) + 1,
                "line": line_no,
                "name": name,
                "type": "",
                "protocol": "",
                "address": "",
                "command": "",
            }
            if name.lower() == "power":
                power_idx += 1
                current["power_index"] = power_idx
            records.append(current)
            continue

        if current and ":" in line:
            key, value = line.split(":", 1)
            key = key.strip()
            if key in current:
                current[key] = value.strip()

    return records


def check_ir(path: Path) -> bool:
    if not path.exists():
        print(f"SKIP: IR fixture not found: {path}")
        return True

    data = path.read_bytes()
    records = parse_ir_records(path)
    powers = [record for record in records if str(record.get("name", "")).lower() == "power"]
    rec810 = records[809] if len(records) >= 810 else {}
    ok = True

    ok &= report(len(records) == 815, "tv.ir fixture has 815 records")
    ok &= report(len(powers) == 324, "tv.ir fixture has 324 Power records")
    ok &= report(
        rec810.get("record") == 810
        and rec810.get("line") == 4935
        and rec810.get("name") == "Power"
        and rec810.get("type") == "parsed"
        and rec810.get("protocol") == "NECext"
        and rec810.get("address") == "04 B9 00 00"
        and rec810.get("command") == "00 FF 00 00",
        "tv.ir record 810 is the final parsed NECext Power code under test",
    )
    ok &= report(not (data.endswith(b"\n") or data.endswith(b"\r")), "tv.ir fixture has no trailing newline EOF shape")
    return ok


def main() -> int:
    parser = argparse.ArgumentParser(description="Check local exit-cleanup fixture files, if present.")
    parser.add_argument("--badusb", type=Path, default=DEFAULT_BADUSB)
    parser.add_argument("--ir", type=Path, default=DEFAULT_IR)
    args = parser.parse_args()

    ok = check_badusb(args.badusb)
    ok = check_ir(args.ir) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
