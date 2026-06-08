#!/usr/bin/env python3
"""Static checks for SD-loaded music format support."""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def expect(name: str, condition: bool) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {name}")


def main() -> int:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    ui = (ROOT / "src" / "ui.c").read_text(encoding="utf-8")
    audio_h = (ROOT / "src" / "audioPwm.h").read_text(encoding="utf-8")
    audio_c = (ROOT / "src" / "audioPwm.c").read_text(encoding="utf-8")
    wav_c = (ROOT / "src" / "wavPlayer.c").read_text(encoding="utf-8")
    rtttl_c = (ROOT / "src" / "rtttlPlayer.c").read_text(encoding="utf-8")

    expect("Music app still builds RTTTL player", "${SRC_DIR}/rtttlPlayer.c" in cmake and "rtttlPlayerPlayFile" in ui)
    expect("Music app builds WAV player", "${SRC_DIR}/wavPlayer.c" in cmake and "wavPlayerPlayFile" in ui)
    expect("Music picker accepts RTTTL", '".rtttl"' in ui and '".txt"' in ui)
    expect("Music picker accepts WAV", '".wav"' in ui)
    expect("WAV decoder validates RIFF/WAVE", '"RIFF"' in wav_c and '"WAVE"' in wav_c)
    expect("WAV decoder supports PCM chunks", '"fmt "' in wav_c and '"data"' in wav_c and "audioFormat != 1u" in wav_c)
    expect("WAV decoder supports 8 and 16 bit PCM", "bitsPerSample != 8u" in wav_c and "bitsPerSample != 16u" in wav_c)
    expect("WAV decoder downmixes stereo", "channels == 2u" in wav_c)
    expect("Audio PWM preserves tone API", "audioPwmTone" in audio_h and "AudioPwmModeTone" in audio_c and "rtttlPrvFreq" in rtttl_c)
    expect("Audio PWM exposes buffered PCM path", "audioPwmPcmStart(uint32_t sampleRate)" in audio_h and "audioPwmPcmCanWrite" in audio_h and "audioPwmPcmDrain" in audio_h and "AudioPwmModePcm" in audio_c)
    expect("Audio PWM feeds PCM from timer IRQ", "TIMER0_IRQ_0_IRQHandler" in audio_c and "mPcmBuf" in audio_c and "AUDIO_PCM_TIMER_HZ" in audio_c)
    expect("WAV queues fixed-rate PCM", "WAV_OUTPUT_RATE" in wav_c and "audioPwmPcmStart(WAV_OUTPUT_RATE)" in wav_c and "audioPwmPcmCanWrite" in wav_c)
    expect("WAV playback avoids busy-wait sample clock", "sampleTicks" not in wav_c and "nextSample" not in wav_c)
    expect("MP3 is not advertised without decoder", '".mp3"' not in ui and "minimp3" not in cmake)

    print("music player static tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
