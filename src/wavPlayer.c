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
				if (!audioPwmStart(status->sampleRate)) {
					*retP = MusicPlayerResultDecodeError;
					return true;
				}
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
	if (!audioPwmStart(status.sampleRate))
		return MusicPlayerResultDecodeError;

	while (bytesDone < dataSize) {
		enum MusicPlayerResult ctlRet;
		uint32_t bytesLeft = dataSize - bytesDone, bytesToRead = WAV_BUF_SZ, nRead, pos;

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
			status.bytesPlayed = bytesDone + pos;
			audioPwmWriteSample(wavPrvSample(buf + pos, channels, bitsPerSample));
			audioPwmWaitNext();
			if (!((pos / blockAlign) & 0x7f) && wavPrvHandleControl(controlF, userData, &status, &ctlRet))
				return ctlRet;
		}
		bytesDone += nRead;
	}

	audioPwmStop();
	return MusicPlayerResultDone;
}
