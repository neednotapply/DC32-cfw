#include <string.h>
#include <stdint.h>
#include "audioPwm.h"
#include "memMap.h"
#include "timebase.h"
#include "wavPlayer.h"

#define WAV_DMA_RING_BITS	13
#define WAV_DMA_BUF_BYTES	(1u << WAV_DMA_RING_BITS)
#define WAV_DMA_SAMPLES		(WAV_DMA_BUF_BYTES / sizeof(uint16_t))
#define WAV_DMA_HALF_SAMPLES	(WAV_DMA_SAMPLES / 2)
#define WAV_RAW_BUF_SZ		2048
#define WAV_DMA_STALL_TIMEOUT	(TICKS_PER_SECOND / 4)
#define WAV_BUF			((uint8_t*)(((uint8_t*)CART_RAM_ADDR_IN_RAM) + QSPI_RAM_SIZE_MAX / 2))

struct WavPlayback {
	struct FatfsFil *fil;
	uint16_t *dmaBuf;
	uint8_t *rawBuf;
	uint16_t channels;
	uint16_t bitsPerSample;
	uint16_t blockAlign;
	uint32_t totalFrames;
	uint32_t framesPrepared;
	uint8_t dutyLut[256];
	uint8_t lutVolume;
};

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

static int32_t wavPrvAbs32(int32_t value)
{
	return value < 0 ? -value : value;
}

static void wavPrvBuildDutyLut(struct WavPlayback *wav)
{
	uint_fast8_t volume = audioPwmGetVolume();
	uint32_t i;

	wav->lutVolume = volume;
	for (i = 0; i < sizeof(wav->dutyLut); i++) {
		int32_t sample = (int32_t)i - 128;
		int32_t gainSample, limited, duty;

		if (!volume) {
			wav->dutyLut[i] = 0;
			continue;
		}

		gainSample = (sample * (int32_t)volume * 10) / AUDIO_PWM_VOLUME_MAX;
		limited = (gainSample * 128) / (96 + wavPrvAbs32(gainSample));
		duty = limited + 128;
		if (duty < 0)
			duty = 0;
		else if (duty > 255)
			duty = 255;
		wav->dutyLut[i] = duty;
	}
}

static void wavPrvMaybeUpdateDutyLut(struct WavPlayback *wav)
{
	if (wav->lutVolume != audioPwmGetVolume())
		wavPrvBuildDutyLut(wav);
}

static uint16_t wavPrvCenterDuty(const struct WavPlayback *wav)
{
	return wav->dutyLut[128];
}

static uint16_t wavPrvConvertFrame(const struct WavPlayback *wav, const uint8_t *p)
{
	uint8_t idx;

	if (wav->bitsPerSample == 8) {
		if (wav->channels == 2)
			idx = (uint8_t)(((uint16_t)p[0] + p[1]) / 2);
		else
			idx = p[0];
	}
	else {
		if (wav->channels == 2) {
			int32_t sample = ((int32_t)(int8_t)p[1] + (int32_t)(int8_t)p[3]) / 2;
			idx = (uint8_t)(sample + 128);
		}
		else
			idx = (uint8_t)(p[1] + 128);
	}
	return wav->dutyLut[idx];
}

static bool wavPrvFillDmaFrames(struct WavPlayback *wav, uint32_t startFrame, uint32_t numFrames)
{
	uint32_t done = 0;

	wavPrvMaybeUpdateDutyLut(wav);
	while (done < numFrames && wav->framesPrepared < wav->totalFrames) {
		uint32_t framesLeft = wav->totalFrames - wav->framesPrepared;
		uint32_t framesToRead = numFrames - done;
		uint32_t bytesToRead, nRead, framesRead, i;

		if (framesToRead > framesLeft)
			framesToRead = framesLeft;
		if (framesToRead > WAV_RAW_BUF_SZ / wav->blockAlign)
			framesToRead = WAV_RAW_BUF_SZ / wav->blockAlign;
		bytesToRead = framesToRead * wav->blockAlign;
		if (!fatfsFileRead(wav->fil, wav->rawBuf, bytesToRead, &nRead))
			return false;
		nRead -= nRead % wav->blockAlign;
		framesRead = nRead / wav->blockAlign;
		if (!framesRead)
			return false;

		for (i = 0; i < framesRead; i++)
			wav->dmaBuf[startFrame + done + i] = wavPrvConvertFrame(wav, wav->rawBuf + i * wav->blockAlign);
		wav->framesPrepared += framesRead;
		done += framesRead;
		if (framesRead != framesToRead)
			return false;
	}

	while (done < numFrames)
		wav->dmaBuf[startFrame + done++] = wavPrvCenterDuty(wav);
	return true;
}

static bool wavPrvHandleControl(MusicPlayerControlF controlF, void *userData, struct MusicPlayerStatus *status, enum MusicPlayerResult *retP)
{
	enum MusicPlayerControl ctl = controlF ? controlF(userData, status) : MusicPlayerControlNone;

	if (ctl == MusicPlayerControlPause) {
		status->paused = true;
		audioPwmDmaPause(true);
		while (1) {
			ctl = controlF ? controlF(userData, status) : MusicPlayerControlNone;
			if (ctl == MusicPlayerControlPause) {
				status->paused = false;
				audioPwmDmaPause(false);
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

enum MusicPlayerResult wavPlayerPlayFile(struct FatfsFil *fil, MusicPlayerControlF controlF, void *userData)
{
	struct MusicPlayerStatus status;
	struct WavPlayback wav;
	uint32_t dataPos = 0, dataSize = 0, totalFrames, nextRefillSerial = 1;
	uint32_t lastRemaining;
	uint64_t lastProgressTime;
	uint16_t audioFormat = 0, channels = 0, bitsPerSample = 0, blockAlign = 0;
	uint8_t hdr[12];
	bool haveFmt = false, haveData = false;

	memset(&status, 0, sizeof(status));
	memset(&wav, 0, sizeof(wav));
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
	dataSize -= dataSize % blockAlign;
	totalFrames = dataSize / blockAlign;
	if (!totalFrames || totalFrames > 0x0fffffff)
		return MusicPlayerResultDecodeError;
	if (!fatfsFileSeek(fil, dataPos))
		return MusicPlayerResultFileError;

	wav.fil = fil;
	wav.dmaBuf = (uint16_t*)WAV_BUF;
	wav.rawBuf = WAV_BUF + WAV_DMA_BUF_BYTES;
	wav.channels = channels;
	wav.bitsPerSample = bitsPerSample;
	wav.blockAlign = blockAlign;
	wav.totalFrames = totalFrames;
	wav.lutVolume = 0xff;
	if (((uintptr_t)wav.dmaBuf & (WAV_DMA_BUF_BYTES - 1)) != 0)
		return MusicPlayerResultDecodeError;
	wavPrvBuildDutyLut(&wav);
	if (!wavPrvFillDmaFrames(&wav, 0, WAV_DMA_HALF_SAMPLES))
		return MusicPlayerResultFileError;
	if (!wavPrvFillDmaFrames(&wav, WAV_DMA_HALF_SAMPLES, WAV_DMA_HALF_SAMPLES))
		return MusicPlayerResultFileError;
	if (!audioPwmStartDutyDma(status.sampleRate, wav.dmaBuf, totalFrames, WAV_DMA_RING_BITS))
		return MusicPlayerResultDecodeError;

	status.fileSize = dataSize;
	lastRemaining = audioPwmDmaSamplesRemaining();
	lastProgressTime = getTime();
	while (!audioPwmDmaDone()) {
		enum MusicPlayerResult ctlRet;
		uint64_t beforeControl;
		uint32_t remaining = audioPwmDmaSamplesRemaining();
		uint32_t framesPlayed = totalFrames - remaining;
		uint32_t halfSerial = framesPlayed / WAV_DMA_HALF_SAMPLES;

		status.bytesPlayed = framesPlayed * blockAlign;
		if (status.bytesPlayed > dataSize)
			status.bytesPlayed = dataSize;
		beforeControl = getTime();
		if (wavPrvHandleControl(controlF, userData, &status, &ctlRet))
		{
			audioPwmStop();
			return ctlRet;
		}
		if (getTime() - beforeControl > WAV_DMA_STALL_TIMEOUT / 2)
			lastProgressTime = getTime();

		if (remaining != lastRemaining) {
			lastRemaining = remaining;
			lastProgressTime = getTime();
		}
		else if (getTime() - lastProgressTime > WAV_DMA_STALL_TIMEOUT) {
			audioPwmStop();
			return MusicPlayerResultFileError;
		}

		while (nextRefillSerial <= halfSerial) {
			uint32_t half = (nextRefillSerial - 1) & 1;
			if (!wavPrvFillDmaFrames(&wav, half ? WAV_DMA_HALF_SAMPLES : 0, WAV_DMA_HALF_SAMPLES)) {
				audioPwmStop();
				return MusicPlayerResultFileError;
			}
			nextRefillSerial++;
		}
	}
	audioPwmStop();
	status.bytesPlayed = dataSize;
	return MusicPlayerResultDone;
}
