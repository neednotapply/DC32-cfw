#include <string.h>
#include "audioPwm.h"
#include "memMap.h"
#include "timebase.h"
#include "wavPlayer.h"

#define WAV_BUF_SZ	2048
#define WAV_BUF		((uint8_t*)(((uint8_t*)CART_RAM_ADDR_IN_RAM) + QSPI_RAM_SIZE_MAX / 2))
#define WAV_WINDOW_MIN_SAMPLES	128
#define WAV_WINDOW_MAX_SAMPLES	1024
#define WAV_WINDOW_HZ			50
#define WAV_SILENCE_PEAK		1200
#define WAV_ZERO_HYST			900
#define WAV_FREQ_MIN			80
#define WAV_FREQ_MAX			5000

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

static bool wavPrvWaitUntil(uint64_t end, MusicPlayerControlF controlF, void *userData, struct MusicPlayerStatus *status, enum MusicPlayerResult *retP)
{
	while ((int64_t)(end - getTime()) > 0) {
		uint64_t stepEnd = getTime() + TICKS_PER_SECOND / 100;

		if ((int64_t)(stepEnd - end) > 0)
			stepEnd = end;
		if (wavPrvHandleControl(controlF, userData, status, retP))
			return true;
		while ((int64_t)(stepEnd - getTime()) > 0);
	}
	return false;
}

static int16_t wavPrvSample(const uint8_t *p, uint16_t channels, uint16_t bitsPerSample)
{
	if (bitsPerSample == 8) {
		int32_t sample = p[0];

		if (channels == 2)
			sample = (sample + p[1]) / 2;
		return (sample - 128) << 8;
	}
	else {
		int32_t sample = (int16_t)wavPrvReadLe16(p);

		if (channels == 2)
			sample = (sample + (int16_t)wavPrvReadLe16(p + 2)) / 2;
		return sample;
	}
}

struct WavToneWindow {
	uint32_t sampleRate;
	uint32_t targetSamples;
	uint32_t samples;
	uint32_t crossings;
	uint32_t peak;
	int8_t sign;
	uint32_t lastFreq;
};

static uint32_t wavPrvAbs16(int16_t sample)
{
	return sample < 0 ? -(int32_t)sample : sample;
}

static void wavPrvToneWindowInit(struct WavToneWindow *win, uint32_t sampleRate)
{
	win->sampleRate = sampleRate;
	win->targetSamples = sampleRate / WAV_WINDOW_HZ;
	if (win->targetSamples < WAV_WINDOW_MIN_SAMPLES)
		win->targetSamples = WAV_WINDOW_MIN_SAMPLES;
	if (win->targetSamples > WAV_WINDOW_MAX_SAMPLES)
		win->targetSamples = WAV_WINDOW_MAX_SAMPLES;
	win->samples = 0;
	win->crossings = 0;
	win->peak = 0;
	win->sign = 0;
	win->lastFreq = 0;
}

static void wavPrvToneWindowReset(struct WavToneWindow *win)
{
	win->samples = 0;
	win->crossings = 0;
	win->peak = 0;
	win->sign = 0;
}

static void wavPrvToneWindowSample(struct WavToneWindow *win, int16_t sample)
{
	uint32_t amp = wavPrvAbs16(sample);
	int8_t sign = win->sign;

	if (amp > win->peak)
		win->peak = amp;
	if (sample > WAV_ZERO_HYST)
		sign = 1;
	else if (sample < -WAV_ZERO_HYST)
		sign = -1;

	if (win->sign && sign != win->sign)
		win->crossings++;
	win->sign = sign;
	win->samples++;
}

static uint32_t wavPrvToneWindowFreq(struct WavToneWindow *win)
{
	uint32_t freq;

	if (win->peak < WAV_SILENCE_PEAK || win->samples < 2 || win->crossings < 2) {
		win->lastFreq = 0;
		return 0;
	}

	freq = (win->crossings * win->sampleRate + win->samples) / (2 * win->samples);
	if (freq < WAV_FREQ_MIN || freq > WAV_FREQ_MAX) {
		win->lastFreq = 0;
		return 0;
	}

	if (win->lastFreq) {
		uint32_t low = win->lastFreq * 92 / 100, high = win->lastFreq * 108 / 100;

		if (freq >= low && freq <= high)
			freq = win->lastFreq;
		else if (freq >= win->lastFreq * 3 / 4 && freq <= win->lastFreq * 4 / 3)
			freq = (win->lastFreq + freq * 3) / 4;
	}

	win->lastFreq = freq;
	return freq;
}

enum MusicPlayerResult wavPlayerPlayFile(struct FatfsFil *fil, MusicPlayerControlF controlF, void *userData)
{
	struct MusicPlayerStatus status;
	uint32_t dataPos = 0, dataSize = 0, bytesDone = 0;
	uint16_t audioFormat = 0, channels = 0, bitsPerSample = 0, blockAlign = 0;
	struct WavToneWindow toneWin;
	uint8_t *buf = WAV_BUF;
	uint64_t nextWindowTime;
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

	status.fileSize = dataSize;
	wavPrvToneWindowInit(&toneWin, status.sampleRate);
	nextWindowTime = getTime();
	while (bytesDone < dataSize) {
		enum MusicPlayerResult ctlRet;
		uint32_t bytesLeft = dataSize - bytesDone, bytesToRead = WAV_BUF_SZ, nRead, pos;

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

		for (pos = 0; pos < nRead; pos += blockAlign) {
			uint32_t samplesThisWindow;

			status.bytesPlayed = bytesDone + pos;
			wavPrvToneWindowSample(&toneWin, wavPrvSample(buf + pos, channels, bitsPerSample));
			if (toneWin.samples < toneWin.targetSamples)
				continue;

			(void)audioPwmTone(wavPrvToneWindowFreq(&toneWin));
			samplesThisWindow = toneWin.samples;
			wavPrvToneWindowReset(&toneWin);
			nextWindowTime += ((uint64_t)samplesThisWindow * TICKS_PER_SECOND + status.sampleRate / 2) / status.sampleRate;
			if (wavPrvWaitUntil(nextWindowTime, controlF, userData, &status, &ctlRet))
				return ctlRet;
			if ((int64_t)(getTime() - nextWindowTime) > 0) {
				nextWindowTime = getTime();
			}
		}
		bytesDone += nRead;
	}

	if (toneWin.samples) {
		enum MusicPlayerResult ctlRet;
		uint32_t samplesThisWindow = toneWin.samples;

		(void)audioPwmTone(wavPrvToneWindowFreq(&toneWin));
		nextWindowTime += ((uint64_t)samplesThisWindow * TICKS_PER_SECOND + status.sampleRate / 2) / status.sampleRate;
		if (wavPrvWaitUntil(nextWindowTime, controlF, userData, &status, &ctlRet))
			return ctlRet;
	}
	audioPwmStop();
	return MusicPlayerResultDone;
}
