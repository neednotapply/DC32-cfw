#include "dcApp.h"
#include "dcAppDraw.h"

#include <stdio.h>
#include <string.h>
#include "dcapp_build_contract.h"
#include "audioPwm.h"
#include "badgeLeds.h"
#include "dispDefcon.h"
#include "fatfs.h"
#include "memMap.h"
#include "irRemote.h"
#include "printf.h"
#include "qspi.h"
#include "sd.h"
#include "timebase.h"
#include "toolWorkspace.h"
#include "RP2350.h"
#include "hardware/regs/sio.h"
#include "hardware/regs/psm.h"
#include "hardware/structs/sio.h"
#include "hardware/structs/psm.h"

typedef char DcAppHeaderSizeCheck[(sizeof(struct DcAppImageHeader) == DCAPP_HEADER_SIZE) ? 1 : -1];

#define DCAPP_FLASH_CHUNK 4096u
static struct DcAppImageHeader mLoadedHeader;
static uint32_t mLoadedRuntime;
static DcAppVoidF mActiveAbort;
static DcAppVoidF mActiveRefresh;
static struct ToolWorkspaceSpan mActiveScratch;
static bool mActiveScratchValid;
static volatile DcAppCore1EntryF mCore1Entry;
static volatile void *mCore1Context;
static volatile bool mCore1Owned;
static volatile bool mCore1Done;
static char mLastError[128];
static const uint32_t mDcAppBuildContractWords[DCAPP_CONTRACT_HASH_WORDS] = {
	DCAPP_BUILD_CONTRACT_WORDS
};

static const struct DcAppCatalogEntry mDcAppCatalog[] = {
	{DcAppIdGameGb, "Game Boy", "/APPS/gb.DC32", false},
	{DcAppIdGameNes, "NES", "/APPS/nes.DC32", false},
	{DcAppIdGameArduboy, "Arduboy", "/APPS/arduboy.DC32", false},
	{DcAppIdToolIr, "Universal IR", "/APPS/ir.DC32", false},
	{DcAppIdToolImage, "Image Viewer", "/APPS/image.DC32", false},
	{DcAppIdToolMusic, "Music", "/APPS/music.DC32", false},
	{DcAppIdToolBadUsb, "BadUSB", "/APPS/badusb.DC32", false},
	{DcAppIdToolAutoclicker, "Autoclicker", "/APPS/autoclicker.DC32", false},
	{DcAppIdToolGamepad, "USB Gamepad", "/APPS/gamepad.DC32", false},
	{DcAppIdToolLaserTag, "Laser Tag", "/APPS/lasertag.DC32", false},
	{DcAppIdToolRaspyJack, "RaspyJack Remote", "/APPS/raspyjack.DC32", false},
	{DcAppIdToolPwnagotchi, "Pwnagotchi Remote", "/APPS/pwnagotchi.DC32", false},
	{DcAppIdPong, "Pong", "/APPS/pong.DC32", true},
	{DcAppIdTetris, "Tetris (NullpoMino)", "/APPS/tetris.DC32", true},
	{DcAppIdArkanoid, "Arkanoid (wkeeling)", "/APPS/arkanoid.DC32", true},
	{DcAppIdFlappy, "Flappy Bird (VadimBoev)", "/APPS/flappy.DC32", true},
	{DcAppIdTrex, "T-Rex Runner (wayou)", "/APPS/trex.DC32", true},
	{DcAppIdDoom, "DOOM (rp2040-doom)", "/APPS/doom.DC32", true},
	{DcAppIdChips, "Chip's Challenge (Tile World)", "/APPS/chips.DC32", true},
	{DcAppIdScorch, "Scorched Earth (xscorch)", "/APPS/scorch.DC32", true},
	{DcAppIdPipe, "Pipe Dream (PipeDreamer)", "/APPS/pipe.DC32", true},
	{DcAppIdSokoban, "Sokoban (XSokoban)", "/APPS/sokoban.DC32", true},
	{DcAppIdOpenJazz, "Jazz Jackrabbit (OpenJazz)", "/APPS/openjazz.DC32", true},
	{DcAppIdSoccer, "Sensible Soccer (YSoccer)", "/APPS/soccer.DC32", true},
	{DcAppIdStarfield, "Starfield", "/APPS/starfield.DC32", true},
	{DcAppIdSpiro, "Spiro", "/APPS/spiro.DC32", true},
	{DcAppIdCube, "Cube", "/APPS/cube.DC32", true},
};

static void *dcAppPrvDisplayFb(void)
{
	return dispGetFb();
}

static const struct DcAppHostApi mHostApi = {
	.abiVersion = DCAPP_ABI_VERSION,
	.log = prRaw,
	.getTime = getTime,
	.delayMsec = delayMsec,
	.idleWaitMsec = timebaseIdleWaitMsec,
	.displayFb = dcAppPrvDisplayFb,
	.keysRaw = uiGetKeysRaw,
	.uiKeysRaw = uiGetUiKeysRaw,
	.gameMenu = uiGameMenu,
	.saveState = uiSaveSavestate,
	.flushSave = uiFlushCurrentSaveToCard,
	.flashWrite = flashWrite,
	.abortActive = dcAppAbortActive,
	.ledsTick = badgeLedsTick,
	.portMenu = uiPortMenu,
	.setFnSettings = uiSetFnSettings,
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

const struct DcAppCatalogEntry *dcAppCatalogEntries(uint_fast8_t *countP)
{
	if (countP)
		*countP = sizeof(mDcAppCatalog) / sizeof(*mDcAppCatalog);
	return mDcAppCatalog;
}

const struct DcAppCatalogEntry *dcAppCatalogFind(uint32_t appId)
{
	uint_fast8_t i;

	for (i = 0; i < sizeof(mDcAppCatalog) / sizeof(*mDcAppCatalog); i++)
		if ((uint32_t)mDcAppCatalog[i].appId == appId)
			return &mDcAppCatalog[i];
	return NULL;
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
	const struct DcAppCatalogEntry *entry = dcAppCatalogFind(runtime);

	return entry ? entry->path : NULL;
}

static const char *dcAppPrvRuntimeName(uint32_t runtime)
{
	const struct DcAppCatalogEntry *entry = dcAppCatalogFind(runtime);

	return entry ? entry->name : "app";
}

/*
 * An app can take a noticeable amount of time to copy from SD, verify, or
 * initialize its own assets.  Put a visible status on the current canvas
 * before either blocking phase so a launch never looks like a frozen menu.
 */
static void dcAppPrvDrawLaunchLoading(const struct DcAppRunArgs *args,
	uint32_t runtime, const char *title, const char *detail)
{
	struct Canvas cnv = {0};
	const struct DcAppLoadingState loading = {
		.appName = dcAppPrvRuntimeName(runtime),
		.title = title,
		.detail = detail,
	};

	if (!args)
		return;
	if (args->canvas)
		cnv = *args->canvas;
	if (!cnv.framebuffer && mHostApi.displayFb)
		cnv.framebuffer = mHostApi.displayFb();
	if (!cnv.w || !cnv.h || !cnv.framebuffer)
		return;
	if (!cnv.bpp)
		cnv.bpp = DISP_BPP;
	dispPrvWaitForScanoutStart();
	dcAppDrawLoadingCanvas(&cnv, &loading);
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

static const struct DcAppImageHeader *dcAppPrvCachedHeaderAt(uint32_t loadAddr)
{
	return (const struct DcAppImageHeader*)flashUncachedPtr(loadAddr);
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

static bool dcAppPrvHeaderHasCurrentBuildContract(const struct DcAppImageHeader *hdr)
{
	if (!hdr || hdr->reserved[DCAPP_CONTRACT_RESERVED_INDEX] != DCAPP_CONTRACT_MAGIC)
		return false;
	for (uint_fast8_t i = 0; i < DCAPP_CONTRACT_HASH_WORDS; i++)
		if (hdr->reserved[DCAPP_CONTRACT_HASH_RESERVED_INDEX + i] != mDcAppBuildContractWords[i])
			return false;
	return true;
}

static bool dcAppPrvHeaderLooksLikeThisRuntime(const struct DcAppImageHeader *hdr, uint32_t runtime)
{
	return hdr &&
		hdr->magic == DCAPP_MAGIC &&
		hdr->headerSize == DCAPP_HEADER_SIZE &&
		hdr->abiVersion == DCAPP_ABI_VERSION &&
		hdr->runtime == runtime;
}

static bool dcAppPrvImageLimit(const struct DcAppImageHeader *hdr, uint32_t *limitP)
{
	uint32_t limit;

	if (!hdr || (hdr->flags & ~DCAPP_IMAGE_FLAG_LARGE_XIP))
		return false;
	if (hdr->flags & DCAPP_IMAGE_FLAG_LARGE_XIP) {
		if (hdr->loadAddr != QSPI_ROM_START)
			return false;
		limit = QSPI_ROM_SIZE_MAX;
	}
	else {
		if (hdr->loadAddr != QSPI_APP_CACHE_START)
			return false;
		limit = QSPI_APP_CACHE_SIZE;
	}
	if (limitP)
		*limitP = limit;
	return true;
}

static enum DcAppResult dcAppPrvValidateHeader(const struct DcAppImageHeader *hdr, uint32_t runtime)
{
	uint32_t dataEnd, imageLimit;

	if (!hdr || hdr->magic != DCAPP_MAGIC || hdr->headerSize != DCAPP_HEADER_SIZE)
		return DcAppResultInvalid;
	if (hdr->abiVersion != DCAPP_ABI_VERSION || hdr->runtime != (uint32_t)runtime)
		return DcAppResultIncompatible;
	if (!dcAppPrvHeaderHasCurrentBuildContract(hdr))
		return DcAppResultIncompatible;
	if (!dcAppPrvImageLimit(hdr, &imageLimit))
		return DcAppResultIncompatible;
	if (hdr->appRamStart != DCAPP_RAM_START ||
			hdr->appRamSize != DCAPP_RAM_SIZE)
		return DcAppResultIncompatible;
	if (hdr->imageSize <= hdr->headerSize || hdr->imageSize > imageLimit)
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

static bool dcAppPrvCachedHeaderLooksValidAt(const struct DcAppImageHeader *hdr, uint32_t loadAddr)
{
	uint32_t imageLimit;

	if (!hdr || hdr->magic != DCAPP_MAGIC || hdr->headerSize != DCAPP_HEADER_SIZE)
		return false;
	if (hdr->abiVersion != DCAPP_ABI_VERSION)
		return false;
	if (hdr->loadAddr != loadAddr ||
			hdr->appRamStart != DCAPP_RAM_START ||
			hdr->appRamSize != DCAPP_RAM_SIZE)
		return false;
	if (!dcAppPrvImageLimit(hdr, &imageLimit))
		return false;
	return hdr->imageSize > hdr->headerSize && hdr->imageSize <= imageLimit;
}

static uint32_t dcAppPrvCachedEraseSpan(const struct DcAppImageHeader *wanted)
{
	const struct DcAppImageHeader *cached = dcAppPrvCachedHeaderAt(wanted->loadAddr);
	uint32_t imageLimit, eraseSize = dcAppPrvAlignUp(wanted->imageSize, QSPI_ERASE_GRANULARITY);

	if (!dcAppPrvImageLimit(wanted, &imageLimit))
		return 0;
	if (dcAppPrvCachedHeaderLooksValidAt(cached, wanted->loadAddr)) {
		uint32_t cachedEraseSize = dcAppPrvAlignUp(cached->imageSize, QSPI_ERASE_GRANULARITY);

		if (eraseSize < cachedEraseSize)
			eraseSize = cachedEraseSize;
	}
	else
		eraseSize = imageLimit;
	if (eraseSize > imageLimit)
		eraseSize = imageLimit;
	return eraseSize;
}

static uint32_t dcAppPrvCachedImageCrc32(const struct DcAppImageHeader *hdr)
{
	uint32_t crc = 0xffffffffu;

	crc = dcAppPrvCrc32Update(crc, flashUncachedPtr(hdr->loadAddr + hdr->headerSize),
		hdr->imageSize - hdr->headerSize);
	return crc ^ 0xffffffffu;
}

static bool dcAppPrvVerifyCachedImage(const struct DcAppImageHeader *wanted, uint32_t runtime)
{
	const struct DcAppImageHeader *cached = dcAppPrvCachedHeaderAt(wanted->loadAddr);

	return dcAppPrvValidateHeader(cached, runtime) == DcAppResultOk &&
		!memcmp(cached, wanted, sizeof(*wanted)) &&
		dcAppPrvCachedImageCrc32(cached) == cached->crc32;
}

static bool dcAppPrvCachedBuildMatches(const struct DcAppImageHeader *wanted, uint32_t runtime)
{
	const struct DcAppImageHeader *cached = dcAppPrvCachedHeaderAt(wanted->loadAddr);

	return dcAppPrvValidateHeader(cached, runtime) == DcAppResultOk &&
		!memcmp(cached->buildId, wanted->buildId, sizeof(wanted->buildId)) &&
		dcAppPrvVerifyCachedImage(wanted, runtime);
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

static void dcAppPrvClearActiveCallbacks(void)
{
	mActiveAbort = NULL;
	mActiveRefresh = NULL;
}

static void dcAppPrvClearActiveAppContext(void)
{
	dcAppPrvClearActiveCallbacks();
	dcAppPrvClearActiveRamContext();
}

static void dcAppPrvSyncExecutableImage(const struct DcAppImageHeader *hdr)
{
	uint32_t imageLimit;

	if (dcAppPrvImageLimit(hdr, &imageLimit))
		flashSyncExecutableRange(hdr->loadAddr, imageLimit);
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

static void dcAppPrvCore1FifoDrain(void)
{
	while (sio_hw->fifo_st & SIO_FIFO_ST_VLD_BITS)
		(void)sio_hw->fifo_rd;
}

static void dcAppPrvCore1FifoWrite(uint32_t value)
{
	while (!(sio_hw->fifo_st & SIO_FIFO_ST_RDY_BITS))
		;
	sio_hw->fifo_wr = value;
	__asm volatile("sev" ::: "memory");
}

static uint32_t dcAppPrvCore1FifoRead(void)
{
	while (!(sio_hw->fifo_st & SIO_FIFO_ST_VLD_BITS))
		__asm volatile("wfe");
	return sio_hw->fifo_rd;
}

static void dcAppPrvCore1Main(void)
{
	DcAppCore1EntryF entry;
	void *context;

	__asm volatile("cpsid i\ndmb" ::: "memory");
	entry = mCore1Entry;
	context = (void*)mCore1Context;
	if (entry)
		entry(context);
	__asm volatile("dmb" ::: "memory");
	mCore1Done = true;
	__asm volatile("sev" ::: "memory");
	for (;;)
		__asm volatile("wfe");
}

void dcAppCore1ForceStop(void)
{
	if (!mCore1Owned)
		return;
	psm_hw->frce_off |= PSM_FRCE_OFF_PROC1_BITS;
	while (!(psm_hw->frce_off & PSM_FRCE_OFF_PROC1_BITS))
		;
	mCore1Entry = NULL;
	mCore1Context = NULL;
	mCore1Done = false;
	mCore1Owned = false;
	__asm volatile("dmb" ::: "memory");
}

bool dcAppCore1Start(DcAppCore1EntryF entry, void *context, void *stackTop)
{
	uint32_t commands[] = {0u, 0u, 1u, SCB->VTOR, (uint32_t)(uintptr_t)stackTop,
		(uint32_t)(uintptr_t)dcAppPrvCore1Main};
	uint32_t index = 0;

	if (!entry || !stackTop || mCore1Owned)
		return false;
	psm_hw->frce_off |= PSM_FRCE_OFF_PROC1_BITS;
	while (!(psm_hw->frce_off & PSM_FRCE_OFF_PROC1_BITS))
		;
	mCore1Entry = entry;
	mCore1Context = context;
	mCore1Done = false;
	mCore1Owned = true;
	__asm volatile("dmb" ::: "memory");
	psm_hw->frce_off &= ~PSM_FRCE_OFF_PROC1_BITS;
	while (psm_hw->frce_off & PSM_FRCE_OFF_PROC1_BITS)
		;

	while (index < sizeof(commands) / sizeof(commands[0])) {
		uint32_t value = commands[index];
		if (index < 2u) {
			dcAppPrvCore1FifoDrain();
			__asm volatile("sev" ::: "memory");
		}
		dcAppPrvCore1FifoWrite(value);
		index = dcAppPrvCore1FifoRead() == value ? index + 1u : 0u;
	}
	return true;
}

void dcAppCore1Join(void)
{
	if (!mCore1Owned)
		return;
	while (!mCore1Done)
		__asm volatile("wfe");
	dcAppCore1ForceStop();
}

bool dcAppGetAuxScratch(struct ToolWorkspaceSpan *spanP)
{
	if (!mActiveScratchValid)
		return false;
	if (spanP) {
		spanP->ptr = (void*)DCAPP_AUX_RAM_START;
		spanP->size = DCAPP_AUX_RAM_SIZE;
	}
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
	dcAppPrvClearActiveAppContext();
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

		if (result == DcAppResultIncompatible &&
				dcAppPrvHeaderLooksLikeThisRuntime(&hdr, runtime) &&
				!dcAppPrvHeaderHasCurrentBuildContract(&hdr))
			snprintf(msg, sizeof(msg), "Update /APPS: %s does not match this firmware",
				path[6] ? path + 6 : path);
		else
			snprintf(msg, sizeof(msg), "%s app is %s", dcAppPrvRuntimeName(runtime), dcAppResultName(result));
		result = dcAppPrvFail(result, msg);
		goto out;
	}
	if (fileSize != hdr.imageSize) {
		result = dcAppPrvFail(DcAppResultInvalid, "App file size does not match its header");
		goto out;
	}
	if (dcAppPrvCachedBuildMatches(&hdr, runtime)) {
		mLoadedHeader = *dcAppPrvCachedHeaderAt(hdr.loadAddr);
		mLoadedRuntime = runtime;
		result = DcAppResultOk;
		goto out;
	}

	if (!fatfsFileSeek(fil, 0)) {
		result = dcAppPrvFail(DcAppResultReadError, "Cannot rewind app file");
		goto out;
	}
	eraseSize = dcAppPrvCachedEraseSpan(&hdr);
	if (!eraseSize || !flashWrite(hdr.loadAddr, eraseSize, NULL, 0)) {
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
		if (!flashWrite(hdr.loadAddr + pos, 0, buf, writeSize)) {
			result = dcAppPrvFail(DcAppResultFlashError, "Cannot write app cache");
			goto out;
		}
	}
	crc ^= 0xffffffffu;
	if (crc != hdr.crc32) {
		result = dcAppPrvFail(DcAppResultInvalid, "App CRC check failed");
		goto out;
	}
	if (!dcAppPrvVerifyCachedImage(&hdr, runtime)) {
		result = dcAppPrvFail(DcAppResultFlashError, "App flash verify failed");
		goto out;
	}
	dcAppPrvSyncExecutableImage(&hdr);

	mLoadedHeader = *dcAppPrvCachedHeaderAt(hdr.loadAddr);
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
	dcAppPrvClearActiveAppContext();
	uiSetFnSettings(NULL, NULL);
	dcAppPrvSyncExecutableImage(&mLoadedHeader);
	audioPwmStop();
	audioPwmSetVolume(AUDIO_PWM_VOLUME_MAX);
	if (!dcAppPrvApplyAppRam(&mLoadedHeader))
		return dcAppPrvFail(DcAppResultInvalid, "Cannot initialize app RAM");

	dcAppPrvBeginActiveRamContext(&mLoadedHeader);
	entry = (DcAppEntryF)dcAppPrvImageFunc(mLoadedHeader.entryOffset);
	mActiveAbort = (DcAppVoidF)dcAppPrvImageFunc(mLoadedHeader.abortOffset);
	mActiveRefresh = (DcAppVoidF)dcAppPrvImageFunc(mLoadedHeader.refreshOffset);
	dcAppPrvDrawLaunchLoading(args, runtime, "Starting", "Initializing application");
	ret = entry(&mHostApi, args);
	dcAppCore1ForceStop();
	uiSetFnSettings(NULL, NULL);
	audioPwmStop();
	dcAppPrvClearActiveAppContext();
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
	dcAppPrvDrawLaunchLoading(args, (uint32_t)appId, "Loading application",
		"Preparing runtime");
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
	(void)irRemoteRxControl(IrRemoteRxControlEnd, NULL);
	if (mActiveAbort)
		mActiveAbort();
}

void dcAppRefreshActive(void)
{
	if (mActiveRefresh)
		mActiveRefresh();
}
