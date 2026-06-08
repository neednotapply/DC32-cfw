#include <string.h>
#include "audioPwm.h"
#include "timebase.h"
#include "wavPlayer.h"

#define WAV_READ_BUF_SIZE       768u
#define WAV_MIN_SAMPLE_RATE     4000u
#define WAV_MAX_SAMPLE_RATE     48000u
#define WAV_OUTPUT_RATE         11025u
#define WAV_CONTROL_INTERVAL    128u

struct WavFmt {
	uint16_t audioFormat;
	uint16_t channels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
};

static uint8_t mWavBuf[WAV_READ_BUF_SIZE];

static uint16_t wavPrvReadLe16(const uint8_t *p)
{
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t wavPrvReadLe32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool wavPrvReadExact(struct FatfsFil *fil, void *buf, uint32_t len)
{
	uint32_t got = 0;

	return fatfsFileRead(fil, buf, len, &got) && got == len;
}

static bool wavPrvReadChunkHeader(struct FatfsFil *fil, uint32_t pos, uint8_t id[4], uint32_t *sizeP)
{
	uint8_t hdr[8];

	if (!fatfsFileSeek(fil, pos) || !wavPrvReadExact(fil, hdr, sizeof(hdr)))
		return false;
	memcpy(id, hdr, 4);
	*sizeP = wavPrvReadLe32(hdr + 4);
	return true;
}

static bool wavPrvValidFmt(const struct WavFmt *fmt)
{
	uint16_t expectedAlign;

	if (fmt->audioFormat != 1u)
		return false;
	if (fmt->channels != 1u && fmt->channels != 2u)
		return false;
	if (fmt->bitsPerSample != 8u && fmt->bitsPerSample != 16u)
		return false;
	if (fmt->sampleRate < WAV_MIN_SAMPLE_RATE || fmt->sampleRate > WAV_MAX_SAMPLE_RATE)
		return false;
	expectedAlign = (uint16_t)(fmt->channels * (fmt->bitsPerSample / 8u));
	return fmt->blockAlign == expectedAlign && fmt->byteRate == fmt->sampleRate * fmt->blockAlign;
}

static uint8_t wavPrvDecodeSample(const uint8_t *frame, const struct WavFmt *fmt)
{
	if (fmt->bitsPerSample == 8u) {
		uint32_t sample = frame[0];

		if (fmt->channels == 2u)
			sample = (sample + frame[1]) / 2u;
		return (uint8_t)sample;
	}
	else {
		int32_t sample = (int16_t)wavPrvReadLe16(frame);

		if (fmt->channels == 2u) {
			int32_t right = (int16_t)wavPrvReadLe16(frame + 2);

			sample = (sample + right) / 2;
		}
		sample += 32768;
		if (sample < 0)
			sample = 0;
		if (sample > 65535)
			sample = 65535;
		return (uint8_t)((sample + 128) >> 8);
	}
}

static bool wavPrvHandleControl(MusicPlayerControlF controlF, void *userData, struct MusicPlayerStatus *status, enum MusicPlayerResult *retP)
{
	enum MusicPlayerControl ctl = controlF ? controlF(userData, status) : MusicPlayerControlNone;

	if (ctl == MusicPlayerControlPause) {
		status->paused = true;
		audioPwmPcmDrain();
		(void)audioPwmPcmWriteU8(128);
		while (1) {
			uint64_t stepEnd = getTime() + TICKS_PER_SECOND / 100;

			ctl = controlF ? controlF(userData, status) : MusicPlayerControlNone;
			if (ctl == MusicPlayerControlPause) {
				status->paused = false;
				return false;
			}
			if (ctl == MusicPlayerControlStop) {
				*retP = MusicPlayerResultStopped;
				return true;
			}
			if (ctl == MusicPlayerControlPrev) {
				*retP = MusicPlayerResultPrev;
				return true;
			}
			if (ctl == MusicPlayerControlNext) {
				*retP = MusicPlayerResultNext;
				return true;
			}
			while (getTime() < stepEnd);
		}
	}
	if (ctl == MusicPlayerControlStop) {
		*retP = MusicPlayerResultStopped;
		return true;
	}
	if (ctl == MusicPlayerControlPrev) {
		*retP = MusicPlayerResultPrev;
		return true;
	}
	if (ctl == MusicPlayerControlNext) {
		*retP = MusicPlayerResultNext;
		return true;
	}
	return false;
}

static bool wavPrvQueueSample(uint8_t sample, MusicPlayerControlF controlF, void *userData,
	struct MusicPlayerStatus *status, enum MusicPlayerResult *retP, uint32_t *samplesSinceControlP)
{
	while (!audioPwmPcmCanWrite()) {
		if (wavPrvHandleControl(controlF, userData, status, retP))
			return false;
	}
	if (!audioPwmPcmWriteU8(sample)) {
		*retP = MusicPlayerResultDecodeError;
		return false;
	}
	if (++*samplesSinceControlP >= WAV_CONTROL_INTERVAL) {
		*samplesSinceControlP = 0;
		if (wavPrvHandleControl(controlF, userData, status, retP))
			return false;
	}
	return true;
}

static enum MusicPlayerResult wavPrvParse(struct FatfsFil *fil, struct WavFmt *fmt, uint32_t *dataOffsetP, uint32_t *dataSizeP)
{
	uint8_t hdr[16];
	uint32_t fileSize = fatfsFileGetSize(fil);
	uint32_t pos = 12;
	bool haveFmt = false;

	if (fileSize < 44u)
		return MusicPlayerResultDecodeError;
	if (!fatfsFileSeek(fil, 0) || !wavPrvReadExact(fil, hdr, 12))
		return MusicPlayerResultFileError;
	if (memcmp(hdr, "RIFF", 4) || memcmp(hdr + 8, "WAVE", 4))
		return MusicPlayerResultDecodeError;
	while (pos + 8u <= fileSize) {
		uint8_t id[4];
		uint32_t chunkSize, nextPos;

		if (!wavPrvReadChunkHeader(fil, pos, id, &chunkSize))
			return MusicPlayerResultFileError;
		pos += 8u;
		if (chunkSize > fileSize - pos)
			return MusicPlayerResultDecodeError;
		nextPos = pos + chunkSize + (chunkSize & 1u);
		if (nextPos < pos || nextPos > fileSize)
			return MusicPlayerResultDecodeError;

		if (!memcmp(id, "fmt ", 4)) {
			if (chunkSize < 16u)
				return MusicPlayerResultDecodeError;
			if (!fatfsFileSeek(fil, pos) || !wavPrvReadExact(fil, hdr, 16))
				return MusicPlayerResultFileError;
			fmt->audioFormat = wavPrvReadLe16(hdr + 0);
			fmt->channels = wavPrvReadLe16(hdr + 2);
			fmt->sampleRate = wavPrvReadLe32(hdr + 4);
			fmt->byteRate = wavPrvReadLe32(hdr + 8);
			fmt->blockAlign = wavPrvReadLe16(hdr + 12);
			fmt->bitsPerSample = wavPrvReadLe16(hdr + 14);
			haveFmt = true;
		}
		else if (!memcmp(id, "data", 4)) {
			if (!haveFmt || !wavPrvValidFmt(fmt))
				return MusicPlayerResultDecodeError;
			*dataOffsetP = pos;
			*dataSizeP = chunkSize - (chunkSize % fmt->blockAlign);
			return *dataSizeP ? MusicPlayerResultDone : MusicPlayerResultDecodeError;
		}
		pos = nextPos;
	}
	return MusicPlayerResultDecodeError;
}

enum MusicPlayerResult wavPlayerPlayFile(struct FatfsFil *fil, MusicPlayerControlF controlF, void *userData)
{
	struct MusicPlayerStatus status;
	struct WavFmt fmt;
	enum MusicPlayerResult ret;
	uint32_t dataOffset, dataSize, bytesRemaining, resampleAccum = 0, samplesSinceControl = WAV_CONTROL_INTERVAL;

	memset(&status, 0, sizeof(status));
	memset(&fmt, 0, sizeof(fmt));
	status.fileSize = fatfsFileGetSize(fil);
	ret = wavPrvParse(fil, &fmt, &dataOffset, &dataSize);
	if (ret != MusicPlayerResultDone)
		return ret;
	status.sampleRate = fmt.sampleRate;
	status.bytesPlayed = dataOffset;
	if (!fatfsFileSeek(fil, dataOffset))
		return MusicPlayerResultFileError;
	if (!audioPwmPcmStart(WAV_OUTPUT_RATE))
		return MusicPlayerResultDecodeError;

	bytesRemaining = dataSize;
	while (bytesRemaining) {
		uint32_t want = bytesRemaining < sizeof(mWavBuf) ? bytesRemaining : sizeof(mWavBuf);
		uint32_t off;

		want -= want % fmt.blockAlign;
		if (!want)
			break;
		if (!wavPrvReadExact(fil, mWavBuf, want)) {
			audioPwmPcmStop();
			return MusicPlayerResultFileError;
		}
		bytesRemaining -= want;
		for (off = 0; off + fmt.blockAlign <= want; off += fmt.blockAlign) {
			uint8_t sample = wavPrvDecodeSample(mWavBuf + off, &fmt);

			status.bytesPlayed = dataOffset + dataSize - bytesRemaining - want + off;
			resampleAccum += WAV_OUTPUT_RATE;
			while (resampleAccum >= fmt.sampleRate) {
				resampleAccum -= fmt.sampleRate;
				if (!wavPrvQueueSample(sample, controlF, userData, &status, &ret, &samplesSinceControl)) {
					audioPwmPcmStop();
					return ret;
				}
			}
		}
	}
	status.bytesPlayed = dataOffset + dataSize;
	(void)wavPrvHandleControl(controlF, userData, &status, &ret);
	audioPwmPcmDrain();
	audioPwmPcmStop();
	return MusicPlayerResultDone;
}
