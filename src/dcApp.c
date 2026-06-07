#include "dcApp.h"

#include <stdio.h>
#include <string.h>
#include "dispDefcon.h"
#include "fatfs.h"
#include "memMap.h"
#include "printf.h"
#include "qspi.h"
#include "sd.h"
#include "timebase.h"
#include "toolWorkspace.h"

typedef char DcAppHeaderSizeCheck[(sizeof(struct DcAppImageHeader) == DCAPP_HEADER_SIZE) ? 1 : -1];

#define DCAPP_FLASH_CHUNK 4096u
static struct DcAppImageHeader mLoadedHeader;
static uint32_t mLoadedRuntime;
static DcAppVoidF mActiveAbort;
static DcAppVoidF mActiveRefresh;
static struct ToolWorkspaceSpan mActiveScratch;
static bool mActiveScratchValid;
static char mLastError[128];

static void *dcAppPrvDisplayFb(void)
{
	return dispGetFb();
}

static const struct DcAppHostApi mHostApi = {
	.abiVersion = DCAPP_ABI_VERSION,
	.log = prRaw,
	.getTime = getTime,
	.delayMsec = delayMsec,
	.displayFb = dcAppPrvDisplayFb,
	.keysRaw = uiGetKeysRaw,
	.uiKeysRaw = uiGetUiKeysRaw,
	.gameMenu = uiGameMenu,
	.saveState = uiSaveSavestate,
	.flushSave = uiFlushCurrentSaveToCard,
	.flashWrite = flashWrite,
	.abortActive = dcAppAbortActive,
};

static uint32_t dcAppPrvAlignUp(uint32_t val, uint32_t align)
{
	return (val + align - 1u) & ~(align - 1u);
}

static uint32_t dcAppPrvRangeEnd(uint32_t addr, uint32_t size)
{
	return size ? addr + size : DCAPP_RAM_START;
}

static void dcAppPrvSetError(const char *msg)
{
	if (!msg)
		msg = "App loader failed";
	snprintf(mLastError, sizeof(mLastError), "%s", msg);
}

static enum DcAppResult dcAppPrvFail(enum DcAppResult result, const char *msg)
{
	dcAppPrvSetError(msg);
	return result;
}

const char *dcAppLastError(void)
{
	return mLastError;
}

void dcAppClearError(void)
{
	mLastError[0] = 0;
}

const char *dcAppResultName(enum DcAppResult result)
{
	switch (result) {
		case DcAppResultOk:
			return "ok";
		case DcAppResultMissing:
			return "missing";
		case DcAppResultReadError:
			return "read error";
		case DcAppResultInvalid:
			return "invalid";
		case DcAppResultIncompatible:
			return "incompatible";
		case DcAppResultTooLarge:
			return "too large";
		case DcAppResultFlashError:
			return "flash error";
		case DcAppResultNoLoadedApp:
			return "no loaded app";
	}
	return "unknown";
}

static const char *dcAppPrvRuntimePath(uint32_t runtime)
{
	switch (runtime) {
	case GameRuntimeGb:
		return "/APPS/gb.DC32";
	case GameRuntimeNes:
		return "/APPS/nes.DC32";
	case GameRuntimeArduboy:
		return "/APPS/arduboy.DC32";
	case DcAppIdToolIr:
		return "/APPS/ir.DC32";
	case DcAppIdToolImage:
		return "/APPS/image.DC32";
	case DcAppIdToolMusic:
		return "/APPS/music.DC32";
	case DcAppIdToolBadUsb:
		return "/APPS/badusb.DC32";
	case DcAppIdToolAutoclicker:
		return "/APPS/autoclicker.DC32";
	case DcAppIdToolGamepad:
		return "/APPS/gamepad.DC32";
	case GameRuntimeNone:
		break;
	}
	return NULL;
}

static const char *dcAppPrvRuntimeName(uint32_t runtime)
{
	switch (runtime) {
	case GameRuntimeGb:
			return "Game Boy";
		case GameRuntimeNes:
		return "NES";
	case GameRuntimeArduboy:
		return "Arduboy";
	case DcAppIdToolIr:
		return "Universal IR";
	case DcAppIdToolImage:
		return "Image Viewer";
	case DcAppIdToolMusic:
		return "Music";
	case DcAppIdToolBadUsb:
		return "BadUSB";
	case DcAppIdToolAutoclicker:
		return "Autoclicker";
	case DcAppIdToolGamepad:
		return "USB Gamepad";
	case GameRuntimeNone:
		break;
	}
	return "app";
}

static bool dcAppPrvDiskRead(void *diskUserData, uint32_t sec, uint32_t numSec, void *dstP)
{
	uint8_t *dst = (uint8_t*)dstP;

	(void)diskUserData;
	while (numSec--) {
		if (!sdSecRead(sec++, dst))
			return false;
		dst += SD_BLOCK_SIZE;
	}
	return true;
}

static bool dcAppPrvDiskWrite(void *diskUserData, uint32_t sec, uint32_t numSec, const void *srcP)
{
	const uint8_t *src = (const uint8_t*)srcP;

	(void)diskUserData;
	while (numSec--) {
		if (!sdSecWrite(sec++, src))
			return false;
		src += SD_BLOCK_SIZE;
	}
	return true;
}

static bool dcAppPrvReadExact(struct FatfsFil *fil, void *dst, uint32_t len)
{
	uint32_t got = 0;

	return fatfsFileRead(fil, dst, len, &got) && got == len;
}

static uint32_t dcAppPrvCrc32Update(uint32_t crc, const void *data, uint32_t len)
{
	const uint8_t *bytes = (const uint8_t*)data;

	while (len--) {
		crc ^= *bytes++;
		for (uint_fast8_t i = 0; i < 8; i++)
			crc = (crc >> 1) ^ (0xedb88320u & (uint32_t)-(int32_t)(crc & 1u));
	}
	return crc;
}

static bool dcAppPrvRangeInAppRam(uint32_t addr, uint32_t size)
{
	uint32_t end;

	if (!size)
		return true;
	end = addr + size;
	if (end < addr)
		return false;
	return addr >= DCAPP_RAM_START && end <= DCAPP_RAM_END;
}

static enum DcAppResult dcAppPrvValidateHeader(const struct DcAppImageHeader *hdr, uint32_t runtime)
{
	uint32_t dataEnd;

	if (!hdr || hdr->magic != DCAPP_MAGIC || hdr->headerSize != DCAPP_HEADER_SIZE)
		return DcAppResultInvalid;
	if (hdr->abiVersion != DCAPP_ABI_VERSION || hdr->runtime != (uint32_t)runtime)
		return DcAppResultIncompatible;
	if (hdr->loadAddr != QSPI_APP_CACHE_START ||
			hdr->appRamStart != DCAPP_RAM_START ||
			hdr->appRamSize != DCAPP_RAM_SIZE)
		return DcAppResultIncompatible;
	if (hdr->imageSize <= hdr->headerSize || hdr->imageSize > QSPI_APP_CACHE_SIZE)
		return DcAppResultTooLarge;
	if ((hdr->entryOffset & ~1u) < hdr->headerSize || (hdr->entryOffset & ~1u) >= hdr->imageSize)
		return DcAppResultInvalid;
	if (hdr->abortOffset && ((hdr->abortOffset & ~1u) < hdr->headerSize || (hdr->abortOffset & ~1u) >= hdr->imageSize))
		return DcAppResultInvalid;
	if (hdr->refreshOffset && ((hdr->refreshOffset & ~1u) < hdr->headerSize || (hdr->refreshOffset & ~1u) >= hdr->imageSize))
		return DcAppResultInvalid;
	if (!dcAppPrvRangeInAppRam(hdr->dataAddr, hdr->dataSize) ||
			!dcAppPrvRangeInAppRam(hdr->bssAddr, hdr->bssSize))
		return DcAppResultTooLarge;
	if (hdr->dataSize) {
		dataEnd = hdr->dataLoadOffset + hdr->dataSize;
		if (dataEnd < hdr->dataLoadOffset ||
				hdr->dataLoadOffset < hdr->headerSize ||
				dataEnd > hdr->imageSize)
			return DcAppResultInvalid;
	}
	return DcAppResultOk;
}

static bool dcAppPrvCachedBuildMatches(const struct DcAppImageHeader *wanted, uint32_t runtime)
{
	const struct DcAppImageHeader *cached = (const struct DcAppImageHeader*)QSPI_APP_CACHE_START;

	return dcAppPrvValidateHeader(cached, runtime) == DcAppResultOk &&
		!memcmp(cached->buildId, wanted->buildId, sizeof(wanted->buildId));
}

static bool dcAppPrvApplyAppRam(const struct DcAppImageHeader *hdr)
{
	if (hdr->dataSize)
		memcpy((void*)(uintptr_t)hdr->dataAddr,
			(const void*)(uintptr_t)(hdr->loadAddr + hdr->dataLoadOffset),
			hdr->dataSize);
	if (hdr->bssSize)
		memset((void*)(uintptr_t)hdr->bssAddr, 0, hdr->bssSize);
	return true;
}

static uint32_t dcAppPrvAppRamUsedEnd(const struct DcAppImageHeader *hdr)
{
	uint32_t usedEnd = DCAPP_RAM_START, end;

	end = dcAppPrvRangeEnd(hdr->dataAddr, hdr->dataSize);
	if (usedEnd < end)
		usedEnd = end;
	end = dcAppPrvRangeEnd(hdr->bssAddr, hdr->bssSize);
	if (usedEnd < end)
		usedEnd = end;
	return usedEnd;
}

static void dcAppPrvClearActiveRamContext(void)
{
	mActiveScratch.ptr = NULL;
	mActiveScratch.size = 0;
	mActiveScratchValid = false;
}

static void dcAppPrvBeginActiveRamContext(const struct DcAppImageHeader *hdr)
{
	uint32_t scratchStart = dcAppPrvAlignUp(dcAppPrvAppRamUsedEnd(hdr), 8u);
	uint32_t scratchEnd = DCAPP_RAM_END;

	dcAppPrvClearActiveRamContext();
	if (scratchStart < scratchEnd) {
		mActiveScratch.ptr = (void*)(uintptr_t)scratchStart;
		mActiveScratch.size = scratchEnd - scratchStart;
	}
	mActiveScratchValid = true;
}

bool dcAppGetActiveScratch(struct ToolWorkspaceSpan *spanP)
{
	if (!mActiveScratchValid)
		return false;
	if (spanP)
		*spanP = mActiveScratch;
	return true;
}

static void *dcAppPrvImageFunc(uint32_t offset)
{
	if (!offset)
		return NULL;
	return (void*)(uintptr_t)(mLoadedHeader.loadAddr + offset);
}

static enum DcAppResult dcAppLoadByIdFromVol(uint32_t runtime, struct FatfsVol *mountedVol)
{
	const char *path = dcAppPrvRuntimePath(runtime);
	struct DcAppImageHeader hdr;
	struct FatfsVol *vol = mountedVol;
	struct FatfsFil *fil = NULL;
	uint8_t *buf = (uint8_t*)DCAPP_RAM_START;
	uint32_t fileSize, pos, crc, eraseSize;
	enum DcAppResult result;

	dcAppClearError();
	if (!path)
		return dcAppPrvFail(DcAppResultInvalid, "No app is registered for this game type");
	if (!vol) {
		if (!sdCardInit())
			return dcAppPrvFail(DcAppResultMissing, "Insert an SD card containing /APPS");
		vol = fatfsMount(dcAppPrvDiskRead, dcAppPrvDiskWrite, NULL);
		if (!vol)
			return dcAppPrvFail(DcAppResultMissing, "Cannot mount SD card to load app");
	}
	fil = fatfsFileOpen(vol, path, OPEN_MODE_READ);
	if (!fil) {
		char msg[96];

		snprintf(msg, sizeof(msg), "Missing %s", path);
		result = dcAppPrvFail(DcAppResultMissing, msg);
		goto out;
	}
	fileSize = fatfsFileGetSize(fil);
	if (!dcAppPrvReadExact(fil, &hdr, sizeof(hdr))) {
		result = dcAppPrvFail(DcAppResultReadError, "Cannot read app header");
		goto out;
	}
	result = dcAppPrvValidateHeader(&hdr, runtime);
	if (result != DcAppResultOk) {
		char msg[96];

		snprintf(msg, sizeof(msg), "%s app is %s", dcAppPrvRuntimeName(runtime), dcAppResultName(result));
		result = dcAppPrvFail(result, msg);
		goto out;
	}
	if (fileSize != hdr.imageSize) {
		result = dcAppPrvFail(DcAppResultInvalid, "App file size does not match its header");
		goto out;
	}
	if (dcAppPrvCachedBuildMatches(&hdr, runtime)) {
		mLoadedHeader = *(const struct DcAppImageHeader*)QSPI_APP_CACHE_START;
		mLoadedRuntime = runtime;
		result = DcAppResultOk;
		goto out;
	}

	if (!fatfsFileSeek(fil, 0)) {
		result = dcAppPrvFail(DcAppResultReadError, "Cannot rewind app file");
		goto out;
	}
	eraseSize = dcAppPrvAlignUp(hdr.imageSize, QSPI_ERASE_GRANULARITY);
	if (!flashWrite(QSPI_APP_CACHE_START, eraseSize, NULL, 0)) {
		result = dcAppPrvFail(DcAppResultFlashError, "Cannot erase app cache");
		goto out;
	}

	crc = 0xffffffffu;
	for (pos = 0; pos < hdr.imageSize; pos += DCAPP_FLASH_CHUNK) {
		uint32_t now = hdr.imageSize - pos;
		uint32_t writeSize;

		if (now > DCAPP_FLASH_CHUNK)
			now = DCAPP_FLASH_CHUNK;
		if (!dcAppPrvReadExact(fil, buf, now)) {
			result = dcAppPrvFail(DcAppResultReadError, "Cannot read app payload");
			goto out;
		}
		if (pos + now > hdr.headerSize) {
			uint32_t crcStart = pos < hdr.headerSize ? hdr.headerSize - pos : 0;

			crc = dcAppPrvCrc32Update(crc, buf + crcStart, now - crcStart);
		}
		writeSize = dcAppPrvAlignUp(now, QSPI_WRITE_GRANULARITY);
		if (writeSize > now)
			memset(buf + now, 0xff, writeSize - now);
		if (!flashWrite(QSPI_APP_CACHE_START + pos, 0, buf, writeSize)) {
			result = dcAppPrvFail(DcAppResultFlashError, "Cannot write app cache");
			goto out;
		}
	}
	crc ^= 0xffffffffu;
	if (crc != hdr.crc32) {
		result = dcAppPrvFail(DcAppResultInvalid, "App CRC check failed");
		goto out;
	}

	mLoadedHeader = *(const struct DcAppImageHeader*)QSPI_APP_CACHE_START;
	mLoadedRuntime = runtime;
	result = DcAppResultOk;

out:
	if (fil)
		(void)fatfsFileClose(fil);
	if (vol && !mountedVol)
		(void)fatfsUnmount(vol);
	return result;
}

static enum DcAppResult dcAppLoadById(uint32_t runtime)
{
	return dcAppLoadByIdFromVol(runtime, NULL);
}

enum DcAppResult dcAppLoadGameRuntime(enum GameRuntime runtime)
{
	return dcAppLoadById((uint32_t)runtime);
}

static enum DcAppResult dcAppRunLoadedById(uint32_t runtime, const struct DcAppRunArgs *args)
{
	DcAppEntryF entry;
	int ret;

	if (mLoadedRuntime != runtime || dcAppPrvValidateHeader(&mLoadedHeader, runtime) != DcAppResultOk)
		return dcAppPrvFail(DcAppResultNoLoadedApp, "No compatible app is loaded");
	if (!dcAppPrvApplyAppRam(&mLoadedHeader))
		return dcAppPrvFail(DcAppResultInvalid, "Cannot initialize app RAM");

	dcAppPrvBeginActiveRamContext(&mLoadedHeader);
	entry = (DcAppEntryF)dcAppPrvImageFunc(mLoadedHeader.entryOffset);
	mActiveAbort = (DcAppVoidF)dcAppPrvImageFunc(mLoadedHeader.abortOffset);
	mActiveRefresh = (DcAppVoidF)dcAppPrvImageFunc(mLoadedHeader.refreshOffset);
	ret = entry(&mHostApi, args);
	dcAppPrvClearActiveRamContext();
	mActiveAbort = NULL;
	mActiveRefresh = NULL;
	if (ret) {
		dcAppPrvSetError("App returned an error");
		return DcAppResultInvalid;
	}
	return DcAppResultOk;
}

enum DcAppResult dcAppRunLoadedRuntime(enum GameRuntime runtime, const struct DcAppRunArgs *args)
{
	return dcAppRunLoadedById((uint32_t)runtime, args);
}

enum DcAppResult dcAppRunGameRuntime(enum GameRuntime runtime, const struct DcAppRunArgs *args)
{
	enum DcAppResult result = dcAppLoadGameRuntime(runtime);

	if (result != DcAppResultOk)
		return result;
	return dcAppRunLoadedRuntime(runtime, args);
}

enum DcAppResult dcAppRunTool(enum DcAppId appId, const struct DcAppRunArgs *args)
{
	bool workspaceWasActive = toolWorkspaceActive();
	enum DcAppResult result;

	if (!workspaceWasActive)
		toolWorkspaceBegin();
	result = dcAppLoadByIdFromVol((uint32_t)appId, args ? args->vol : NULL);

	if (result != DcAppResultOk)
		goto out;
	result = dcAppRunLoadedById((uint32_t)appId, args);

out:
	if (!workspaceWasActive)
		toolWorkspaceEnd();
	return result;
}

void dcAppAbortActive(void)
{
	if (mActiveAbort)
		mActiveAbort();
}

void dcAppRefreshActive(void)
{
	if (mActiveRefresh)
		mActiveRefresh();
}
