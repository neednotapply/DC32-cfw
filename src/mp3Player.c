#include <string.h>
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#include "minimp3.h"
#include "audioPwm.h"
#include "mp3Player.h"
#include "memMap.h"

#define MP3_INPUT_BUF_SZ	16384

#define MP3_DECODER_PTR	((mp3dec_t*)(((uint8_t*)CART_RAM_ADDR_IN_RAM) + 0x8000))
#define MP3_INPUT_PTR	((uint8_t*)(((uintptr_t)(MP3_DECODER_PTR + 1) + 3) &~ 3u))
#define MP3_PCM_PTR		((mp3d_sample_t*)(((uintptr_t)(MP3_INPUT_PTR + MP3_INPUT_BUF_SZ) + 3) &~ 3u))

static enum Mp3PlayerControl mp3PlayerPrvPoll(Mp3PlayerControlF controlF, void *userData, struct Mp3PlayerStatus *status)
{
	return controlF ? controlF(userData, status) : Mp3PlayerControlNone;
}

static enum Mp3PlayerResult mp3PlayerPrvPause(Mp3PlayerControlF controlF, void *userData, struct Mp3PlayerStatus *status)
{
	enum Mp3PlayerControl ctl;

	status->paused = true;
	audioPwmStop();
	while (1) {
		ctl = mp3PlayerPrvPoll(controlF, userData, status);
		if (ctl == Mp3PlayerControlPause) {
			status->paused = false;
			if (!audioPwmStart(status->sampleRate))
				return Mp3PlayerResultDecodeError;
			return Mp3PlayerResultDone;
		}
		if (ctl == Mp3PlayerControlStop)
			return Mp3PlayerResultStopped;
		if (ctl == Mp3PlayerControlPrev)
			return Mp3PlayerResultPrev;
		if (ctl == Mp3PlayerControlNext)
			return Mp3PlayerResultNext;
	}
}

enum Mp3PlayerResult mp3PlayerPlayFile(struct FatfsFil *fil, Mp3PlayerControlF controlF, void *userData)
{
	struct Mp3PlayerStatus status;
	uint8_t *mp3Input = MP3_INPUT_PTR;
	mp3d_sample_t *pcm = MP3_PCM_PTR;
	mp3dec_t *decoder = MP3_DECODER_PTR;
	uint32_t have = 0, nRead = 0;
	bool eof = false, audioStarted = false;

	memset(&status, 0, sizeof(status));
	status.fileSize = fatfsFileGetSize(fil);
	mp3dec_init(decoder);

	while (1) {
		mp3dec_frame_info_t info;
		enum Mp3PlayerControl ctl;
		int samples;
		uint32_t consumed;

		while (!eof && have < MP3_INPUT_BUF_SZ / 2) {
			if (!fatfsFileRead(fil, mp3Input + have, MP3_INPUT_BUF_SZ - have, &nRead)) {
				audioPwmStop();
				return Mp3PlayerResultFileError;
			}
			if (!nRead)
				eof = true;
			have += nRead;
		}

		if (!have) {
			audioPwmStop();
			return Mp3PlayerResultDone;
		}

		memset(&info, 0, sizeof(info));
		samples = mp3dec_decode_frame(decoder, mp3Input, have, pcm, &info);
		consumed = info.frame_bytes + info.frame_offset;
		if (!consumed) {
			if (eof) {
				audioPwmStop();
				return Mp3PlayerResultDecodeError;
			}
			consumed = 1;
		}
		if (consumed > have)
			consumed = have;

		if (samples > 0 && info.hz > 0 && info.channels > 0) {
			int i;

			if (!audioStarted || status.sampleRate != (uint32_t)info.hz) {
				audioPwmStop();
				status.sampleRate = info.hz;
				if (!audioPwmStart(status.sampleRate))
					return Mp3PlayerResultDecodeError;
				audioStarted = true;
			}

			for (i = 0; i < samples; i++) {
				int32_t sample;

				if (info.channels == 1)
					sample = pcm[i];
				else
					sample = ((int32_t)pcm[i * 2 + 0] + pcm[i * 2 + 1]) / 2;

				audioPwmWriteSample(sample);
				audioPwmWaitNext();

				if (!(i & 0x7f)) {
					ctl = mp3PlayerPrvPoll(controlF, userData, &status);
					if (ctl == Mp3PlayerControlPause) {
						enum Mp3PlayerResult paused = mp3PlayerPrvPause(controlF, userData, &status);
						if (paused != Mp3PlayerResultDone) {
							audioPwmStop();
							return paused;
						}
					}
					else if (ctl == Mp3PlayerControlStop) {
						audioPwmStop();
						return Mp3PlayerResultStopped;
					}
					else if (ctl == Mp3PlayerControlPrev) {
						audioPwmStop();
						return Mp3PlayerResultPrev;
					}
					else if (ctl == Mp3PlayerControlNext) {
						audioPwmStop();
						return Mp3PlayerResultNext;
					}
				}
			}
		}

		status.bytesPlayed = fatfsFileTell(fil);
		if (consumed < have)
			memmove(mp3Input, mp3Input + consumed, have - consumed);
		have -= consumed;
	}
}
