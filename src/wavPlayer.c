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

static bool wavPrvHandleControl(Mp3PlayerControlF controlF, void *userData, struct Mp3PlayerStatus *status, enum Mp3PlayerResult *retP)
{
	enum Mp3PlayerControl ctl = controlF ? controlF(userData, status) : Mp3PlayerControlNone;

	if (ctl == Mp3PlayerControlPause) {
		status->paused = true;
		audioPwmStop();
		while (1) {
			ctl = controlF ? controlF(userData, status) : Mp3PlayerControlNone;
			if (ctl == Mp3PlayerControlPause) {
				status->paused = false;
				if (!audioPwmStart(status->sampleRate)) {
					*retP = Mp3PlayerResultDecodeError;
					return true;
				}
				return false;
			}
			if (ctl == Mp3PlayerControlStop) {
				*retP = Mp3PlayerResultStopped;
				return true;
			}
			if (ctl == Mp3PlayerControlPrev) {
				*retP = Mp3PlayerResultPrev;
				return true;
			}
			if (ctl == Mp3PlayerControlNext) {
				*retP = Mp3PlayerResultNext;
				return true;
			}
		}
	}
	if (ctl == Mp3PlayerControlStop) {
		audioPwmStop();
		*retP = Mp3PlayerResultStopped;
		return true;
	}
	if (ctl == Mp3PlayerControlPrev) {
		audioPwmStop();
		*retP = Mp3PlayerResultPrev;
		return true;
	}
	if (ctl == Mp3PlayerControlNext) {
		audioPwmStop();
		*retP = Mp3PlayerResultNext;
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

enum Mp3PlayerResult wavPlayerPlayFile(struct FatfsFil *fil, Mp3PlayerControlF controlF, void *userData)
{
	struct Mp3PlayerStatus status;
	uint32_t dataPos = 0, dataSize = 0, bytesDone = 0;
	uint16_t audioFormat = 0, channels = 0, bitsPerSample = 0, blockAlign = 0;
	uint8_t *buf = WAV_BUF;
	uint8_t hdr[12];
	bool haveFmt = false, haveData = false;

	memset(&status, 0, sizeof(status));
	status.fileSize = fatfsFileGetSize(fil);

	if (!wavPrvReadExact(fil, hdr, sizeof(hdr)))
		return Mp3PlayerResultFileError;
	if (memcmp(hdr + 0, "RIFF", 4) || memcmp(hdr + 8, "WAVE", 4))
		return Mp3PlayerResultDecodeError;

	while (fatfsFileTell(fil) + 8 <= status.fileSize) {
		uint8_t chunkHdr[8];
		uint32_t chunkSize, nextChunk;

		if (!wavPrvReadExact(fil, chunkHdr, sizeof(chunkHdr)))
			return Mp3PlayerResultFileError;
		chunkSize = wavPrvReadLe32(chunkHdr + 4);
		nextChunk = fatfsFileTell(fil) + chunkSize + (chunkSize & 1);

		if (!memcmp(chunkHdr, "fmt ", 4)) {
			uint8_t fmt[16];

			if (chunkSize < sizeof(fmt) || !wavPrvReadExact(fil, fmt, sizeof(fmt)))
				return Mp3PlayerResultDecodeError;
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
			return Mp3PlayerResultDecodeError;
		if (!fatfsFileSeek(fil, nextChunk))
			return Mp3PlayerResultFileError;
		if (haveFmt && haveData)
			break;
	}

	if (!haveFmt || !haveData || audioFormat != 1 || (channels != 1 && channels != 2) ||
		(bitsPerSample != 8 && bitsPerSample != 16) || !status.sampleRate)
		return Mp3PlayerResultDecodeError;

	if (blockAlign != channels * bitsPerSample / 8 || dataSize < blockAlign)
		return Mp3PlayerResultDecodeError;
	if (!fatfsFileSeek(fil, dataPos))
		return Mp3PlayerResultFileError;
	if (!audioPwmStart(status.sampleRate))
		return Mp3PlayerResultDecodeError;

	while (bytesDone < dataSize) {
		enum Mp3PlayerResult ctlRet;
		uint32_t bytesLeft = dataSize - bytesDone, bytesToRead = WAV_BUF_SZ, nRead, pos;

		if (bytesToRead > bytesLeft)
			bytesToRead = bytesLeft;
		bytesToRead -= bytesToRead % blockAlign;
		if (!bytesToRead)
			break;
		if (!fatfsFileRead(fil, buf, bytesToRead, &nRead)) {
			audioPwmStop();
			return Mp3PlayerResultFileError;
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
	return Mp3PlayerResultDone;
}
