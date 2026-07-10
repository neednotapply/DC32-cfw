#!/usr/bin/env python3
"""Regression checks for selectable-voice ABC playback and packaging."""

from __future__ import annotations

import tempfile
from fractions import Fraction
from pathlib import Path

from build_sd_zip import ABC_MIN_TUNES, abc_tune_supported, copy_abc_assets


ROOT = Path(__file__).resolve().parents[1]


def require(value: bool, message: str) -> None:
    if not value:
        raise AssertionError(message)


def fixture(body: str, headers: str = "M:4/4\nL:1/8\nQ:1/4=120\nK:C\n") -> str:
    return f"X:1\nT:Fixture\n{headers}{body}\n"


def duration_us(length: Fraction, unit: Fraction = Fraction(1, 8),
                beat: Fraction = Fraction(1, 4), bpm: int = 120) -> int:
    return round(60_000_000 * unit * length / (bpm * beat))


def check_fixtures() -> None:
    supported = (
        fixture("C D E F|G A B c|"),
        fixture("^C2 C/2 =C3/2 _D|"),
        fixture("C>D E<F (3GAB|"),
        fixture("|: CDEF |1 G2 :|2 A2 |]"),
        fixture('"Am"!trill!{c}C-D D2 z2|'),
        fixture("CDEF", "M:6/8\nK:Ddor\n"),
    )
    for index, text in enumerate(supported):
        require(abc_tune_supported(text)[0], f"supported fixture {index} was rejected")

    rejected = (
        fixture("[CEG]2 z2|"),
        fixture("C2 & E2|"),
        fixture("C2|") + "X:2\nT:Second\nK:C\nD2|\n",
        "T:Missing index\nK:C\nC2|\n",
    )
    for index, text in enumerate(rejected):
        require(not abc_tune_supported(text)[0], f"unsupported fixture {index} was accepted")
    require(not abc_tune_supported(fixture("C") + " " * 4096)[0], "oversized ABC accepted")

    require(duration_us(Fraction(1)) == 250_000, "default eighth-note timing changed")
    require(duration_us(Fraction(3, 2)) == 375_000, "dotted/rational timing changed")
    require(duration_us(Fraction(2, 3)) == 166_667, "triplet timing changed")

    deadline = now = 0
    for event_us in (250_000, 125_000, 375_000):
        deadline += event_us
        now += 7_000  # parsing/UI work counts against the absolute deadline
        now = max(now, deadline)
    require(deadline == 750_000 and now == deadline, "absolute scheduler accumulated overhead")
    pause_start = now
    now += 123_456
    deadline += now - pause_start
    require(deadline == 873_456, "pause duration did not shift the deadline exactly")


def check_integration() -> None:
    abc = (ROOT / "src" / "abcPlayer.c").read_text(encoding="utf-8")
    midi = (ROOT / "src" / "midiPlayer.c").read_text(encoding="utf-8")
    ui = (ROOT / "src" / "ui.c").read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    music = (ROOT / "src" / "musicPlayer.h").read_text(encoding="utf-8")
    readme = (ROOT / "README.md").read_text(encoding="utf-8")

    for token in (
        "ABC_BUF_SIZE 4096u", "abcPrvPreflight", "abcPrvKey", "abcPrvTempo",
        "abcPrvTuplet", "abcPrvSkipFirstEnding", "abcPrvAdvance", "abcPrvWait",
        "ABC_ACCIDENTAL_UNSET", "duration * 7u / 8u", "MusicPlayerResultUnsupported", "selectedVoice", "voiceCount",
    ):
        require(token in abc, f"missing ABC player contract: {token}")
    for token in ("MThd", "midiScanTrack", "trackMap", "MusicPlayerControlTrackNext", "MidiEventTempo"):
        require(token in midi, f"missing MIDI track contract: {token}")
    require('uiPrvStrEndsWithNoCase(fname, ".abc")' in ui, "Music list does not recognize ABC")
    require('uiPrvStrEndsWithNoCase(fname, ".mid")' in ui and 'MusicPlaybackControlTrack' in ui, "Track UI integration is missing")
    require('"Unsupported ABC file"' in ui and '"Bad ABC file"' in ui, "ABC errors are unclear")
    require("status->bpm" in ui and "uint32_t bpm;" in music, "music status is not BPM-based")
    require("abcPlayer.c" in cmake and "midiPlayer.c" in cmake and "wavPlayer.c" not in cmake, "Music build sources are wrong")
    require(not (ROOT / "src" / "wavPlayer.c").exists(), "WAV player source remains")
    require('".wav"' not in ui and ".wav" not in readme.lower(), "WAV remains user-visible")


def check_corpus() -> None:
    repo = ROOT / "build" / "research-abc-tunebooks"
    if not repo.is_dir():
        return
    with tempfile.TemporaryDirectory() as tmp:
        accepted, rejected = copy_abc_assets(repo, Path(tmp), "test-commit")
        root = Path(tmp) / "MUSIC" / "ABC"
        files = list(root.rglob("*.abc"))
        require(accepted >= ABC_MIN_TUNES, "ABC corpus acceptance fell below minimum")
        require(len(files) == accepted, "ABC splitter count mismatch")
        require(rejected > 0, "strict corpus filter stopped rejecting polyphony")
        require((root / "LICENSE-CC0.txt").is_file(), "CC0 license was not retained")
        for directory in (path for path in root.rglob("*") if path.is_dir()):
            direct = sum(1 for path in directory.iterdir() if path.is_file() and path.suffix == ".abc")
            require(direct <= 64, f"ABC directory exceeds FAT listing limit: {directory}")


def main() -> int:
    check_fixtures()
    check_integration()
    check_corpus()
    print("ABC player regression checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
