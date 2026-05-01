#include <string.h>
#include "audioPwm.h"
#include "memMap.h"
#include "wavPlayer.h"

#define WAV_BUF_SZ	2048
#define WAV_BUF		((uint8_t*)(((uint8_t*)CART_RAM_ADDR_IN_RAM) + QSPI_RAM_SIZE_MAX / 2))

static uint16_t wavPrvReadLe16(const uint8_t *p)
{
	return p[0] | (((uint16_t)p[1]) << 8);
}

static uint32_t wavPrvReadLe32(const uint8_t *p)
{
	return p[0] | (((uint32_t)p[1]) << 8) | (((uint32_t)p[2]) << 16) | (((uint32_t)p[3]) << 24);
}

static bool wavPrvReadExact(struct FatfsFil *fil, void *buf, uint32_t len)
{
	uint32_t nRead;

	return fatfsFileRead(fil, buf, len, &nRead) && nRead == len;
}

static bool wavPrvHandleControl(MusicPlayerControlF controlF, void *userData, struct MusicPlayerStatus *status, enum MusicPlayerResult *retP)
{
	enum MusicPlayerControl ctl = controlF ? controlF(userData, status) : MusicPlayerControlNone;

	if (ctl == MusicPlayerControlPause) {
		status->paused = true;
		audioPwmStop();
		while (1) {
			ctl = controlF ? controlF(userData, status) : MusicPlayerControlNone;
			if (ctl == MusicPlayerControlPause) {
				status->paused = false;
				(void)audioPwmStartDuty(status->sampleRate);
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
		}
	}
	if (ctl == MusicPlayerControlStop) {
		audioPwmStop();
		*retP = MusicPlayerResultStopped;
		return true;
	}
	if (ctl == MusicPlayerControlPrev) {
		audioPwmStop();
		*retP = MusicPlayerResultPrev;
		return true;
	}
	if (ctl == MusicPlayerControlNext) {
		audioPwmStop();
		*retP = MusicPlayerResultNext;
		return true;
	}
	return false;
}

static int32_t wavPrvSigned8Sample(const uint8_t *p, uint16_t channels, uint16_t bitsPerSample)
{
	if (bitsPerSample == 8) {
		int32_t sample = (int32_t)p[0] - 128;

		if (channels == 2)
			sample = (sample + (int32_t)p[1] - 128) / 2;
		return sample;
	}
	else {
		int32_t sample = (int16_t)wavPrvReadLe16(p);

		if (channels == 2)
			sample = (sample + (int16_t)wavPrvReadLe16(p + 2)) / 2;
		return sample / 256;
	}
}

static int32_t wavPrvAbs32(int32_t value)
{
	return value < 0 ? -value : value;
}

static uint8_t wavPrvDutyFromSigned8(int32_t sample)
{
	uint_fast8_t volume = audioPwmGetVolume();
	int32_t gainSample, limited, duty;

	if (!volume)
		return 0;

	/* Maps 0..15 to roughly Flipper's 0.0..10.0 gain range, then softly limits. */
	gainSample = (sample * (int32_t)volume * 10) / AUDIO_PWM_VOLUME_MAX;
	limited = (gainSample * 128) / (96 + wavPrvAbs32(gainSample));
	duty = limited + 128;
	if (duty < 0)
		return 0;
	if (duty > 255)
		return 255;
	return duty;
}

static uint8_t wavPrvDutySample(const uint8_t *p, uint16_t channels, uint16_t bitsPerSample)
{
	return wavPrvDutyFromSigned8(wavPrvSigned8Sample(p, channels, bitsPerSample));
}

enum MusicPlayerResult wavPlayerPlayFile(struct FatfsFil *fil, MusicPlayerControlF controlF, void *userData)
{
	struct MusicPlayerStatus status;
	uint32_t dataPos = 0, dataSize = 0, bytesDone = 0;
	uint16_t audioFormat = 0, channels = 0, bitsPerSample = 0, blockAlign = 0;
	uint8_t *buf = WAV_BUF;
	uint8_t hdr[12];
	bool haveFmt = false, haveData = false;

	memset(&status, 0, sizeof(status));
	status.fileSize = fatfsFileGetSize(fil);

	if (!wavPrvReadExact(fil, hdr, sizeof(hdr)))
		return MusicPlayerResultFileError;
	if (memcmp(hdr + 0, "RIFF", 4) || memcmp(hdr + 8, "WAVE", 4))
		return MusicPlayerResultDecodeError;

	while (fatfsFileTell(fil) + 8 <= status.fileSize) {
		uint8_t chunkHdr[8];
		uint32_t chunkSize, nextChunk;

		if (!wavPrvReadExact(fil, chunkHdr, sizeof(chunkHdr)))
			return MusicPlayerResultFileError;
		chunkSize = wavPrvReadLe32(chunkHdr + 4);
		nextChunk = fatfsFileTell(fil) + chunkSize + (chunkSize & 1);

		if (!memcmp(chunkHdr, "fmt ", 4)) {
			uint8_t fmt[16];

			if (chunkSize < sizeof(fmt) || !wavPrvReadExact(fil, fmt, sizeof(fmt)))
				return MusicPlayerResultDecodeError;
			audioFormat = wavPrvReadLe16(fmt + 0);
			channels = wavPrvReadLe16(fmt + 2);
			status.sampleRate = wavPrvReadLe32(fmt + 4);
			blockAlign = wavPrvReadLe16(fmt + 12);
			bitsPerSample = wavPrvReadLe16(fmt + 14);
			haveFmt = true;
		}
		else if (!memcmp(chunkHdr, "data", 4)) {
			dataPos = fatfsFileTell(fil);
			dataSize = chunkSize;
			haveData = true;
		}

		if (nextChunk < fatfsFileTell(fil) || nextChunk > status.fileSize + 1)
			return MusicPlayerResultDecodeError;
		if (!fatfsFileSeek(fil, nextChunk))
			return MusicPlayerResultFileError;
		if (haveFmt && haveData)
			break;
	}

	if (!haveFmt || !haveData || audioFormat != 1 || (channels != 1 && channels != 2) ||
		(bitsPerSample != 8 && bitsPerSample != 16) || !status.sampleRate)
		return MusicPlayerResultDecodeError;

	if (blockAlign != channels * bitsPerSample / 8 || dataSize < blockAlign)
		return MusicPlayerResultDecodeError;
	if (!fatfsFileSeek(fil, dataPos))
		return MusicPlayerResultFileError;
	if (!audioPwmStartDuty(status.sampleRate))
		return MusicPlayerResultDecodeError;

	status.fileSize = dataSize;
	while (bytesDone < dataSize) {
		enum MusicPlayerResult ctlRet;
		uint32_t bytesLeft = dataSize - bytesDone, bytesToRead = WAV_BUF_SZ, nRead, pos, frame = 0;

		if (wavPrvHandleControl(controlF, userData, &status, &ctlRet))
			return ctlRet;
		if (bytesToRead > bytesLeft)
			bytesToRead = bytesLeft;
		bytesToRead -= bytesToRead % blockAlign;
		if (!bytesToRead)
			break;
		if (!fatfsFileRead(fil, buf, bytesToRead, &nRead)) {
			audioPwmStop();
			return MusicPlayerResultFileError;
		}
		nRead -= nRead % blockAlign;
		if (!nRead)
			break;

		for (pos = 0; pos < nRead; pos += blockAlign, frame++) {
			status.bytesPlayed = bytesDone + pos;
			audioPwmWriteDutySample(wavPrvDutySample(buf + pos, channels, bitsPerSample));
			if ((frame & 0x3f) == 0) {
				audioPwmWaitNext();
				if (wavPrvHandleControl(controlF, userData, &status, &ctlRet))
					return ctlRet;
			}
		}
		bytesDone += nRead;
	}

	status.bytesPlayed = dataSize;
	while (!audioPwmPcmDrained()) {
		enum MusicPlayerResult ctlRet;

		if (wavPrvHandleControl(controlF, userData, &status, &ctlRet))
			return ctlRet;
	}
	audioPwmStop();
	return MusicPlayerResultDone;
}
