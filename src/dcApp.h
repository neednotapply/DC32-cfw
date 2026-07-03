#ifndef DC_APP_H
#define DC_APP_H

#include <stdbool.h>
#include <stdint.h>
#include "fatfs.h"
#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DCAPP_MAGIC             0x50414344u
#define DCAPP_ABI_VERSION       5u
#define DCAPP_HEADER_SIZE       256u
#define DCAPP_IMAGE_FLAG_LARGE_XIP 0x00000001u
#define DCAPP_CONTRACT_MAGIC    0x43444332u
#define DCAPP_CONTRACT_RESERVED_INDEX 0u
#define DCAPP_CONTRACT_HASH_RESERVED_INDEX 1u
#define DCAPP_CONTRACT_HASH_WORDS 4u
#define DCAPP_CONTRACT_RESERVED_WORDS (1u + DCAPP_CONTRACT_HASH_WORDS)

enum DcAppId {
	DcAppIdGameGb = GameRuntimeGb,
	DcAppIdGameNes = GameRuntimeNes,
	DcAppIdGameArduboy = GameRuntimeArduboy,
	DcAppIdToolIr = 100,
	DcAppIdToolImage = 101,
	DcAppIdToolMusic = 102,
	DcAppIdToolBadUsb = 103,
	DcAppIdToolAutoclicker = 104,
	DcAppIdToolGamepad = 105,
	DcAppIdPong = 200,
	DcAppIdTetris = 201,
	DcAppIdArkanoid = 202,
	DcAppIdFlappy = 203,
	DcAppIdTrex = 205,
	DcAppIdDoom = 206,
	DcAppIdChips = 207,
	DcAppIdScorch = 208,
	DcAppIdPipe = 209,
	DcAppIdSokoban = 211,
	DcAppIdOpenJazz = 212,
	DcAppIdSoccer = 213,
	DcAppIdStarfield = 220,
	DcAppIdSpiro = 221,
	DcAppIdCube = 222,
};

enum DcAppToolAction {
	DcAppToolActionMain,
	DcAppToolActionIrButton,
	DcAppToolActionIrPower,
	DcAppToolActionIrMute,
	DcAppToolActionImageFile,
	DcAppToolActionMusicFile,
	DcAppToolActionBadUsbFile,
};

struct DcAppImageHeader {
	uint32_t magic;
	uint16_t headerSize;
	uint16_t abiVersion;
	uint32_t runtime;
	uint32_t flags;
	uint32_t loadAddr;
	uint32_t imageSize;
	uint32_t dataLoadOffset;
	uint32_t dataAddr;
	uint32_t dataSize;
	uint32_t bssAddr;
	uint32_t bssSize;
	uint32_t entryOffset;
	uint32_t abortOffset;
	uint32_t refreshOffset;
	uint32_t appRamStart;
	uint32_t appRamSize;
	uint8_t buildId[32];
	uint32_t crc32;
	uint32_t reserved[39];
};

struct DcAppRunArgs {
	enum GameRuntime runtime;
	const void *rom;
	uint32_t romSize;
	void *saveRam;
	uint32_t saveRamSize;
	bool presentAsCgb;
	bool upscale;
	bool rotate;
	uint8_t gbPalette;
	uint32_t toolAction;
	struct Canvas *canvas;
	struct FatfsVol *vol;
	const struct FatFileLocator *locator;
	const char *name;
	const char *parentPath;
};

struct DcAppHostApi {
	uint32_t abiVersion;
	void (*log)(const char *fmt, ...);
	uint64_t (*getTime)(void);
	void (*delayMsec)(uint32_t msec);
	void *(*displayFb)(void);
	uint_fast8_t (*keysRaw)(void);
	uint_fast16_t (*uiKeysRaw)(void);
	enum UiGameAction (*gameMenu)(void);
	bool (*saveState)(void);
	bool (*flushSave)(bool force);
	bool (*flashWrite)(uint32_t addr, uint32_t eraseSize, const void *src, uint32_t writeSize);
	void (*abortActive)(void);
	void (*ledsTick)(void);
	bool (*portMenu)(struct Canvas *activeCanvas);
};

typedef int (*DcAppEntryF)(const struct DcAppHostApi *host, const struct DcAppRunArgs *args);
typedef void (*DcAppVoidF)(void);
typedef void (*DcAppCore1EntryF)(void *context);
struct ToolWorkspaceSpan;

struct DcAppCatalogEntry {
	enum DcAppId appId;
	const char *name;
	const char *path;
	bool launcherVisible;
};

enum DcAppResult {
	DcAppResultOk,
	DcAppResultMissing,
	DcAppResultReadError,
	DcAppResultInvalid,
	DcAppResultIncompatible,
	DcAppResultTooLarge,
	DcAppResultFlashError,
	DcAppResultNoLoadedApp,
};

extern const struct DcAppImageHeader dcAppImageHeader;

enum DcAppResult dcAppLoadGameRuntime(enum GameRuntime runtime);
enum DcAppResult dcAppRunLoadedRuntime(enum GameRuntime runtime, const struct DcAppRunArgs *args);
enum DcAppResult dcAppRunGameRuntime(enum GameRuntime runtime, const struct DcAppRunArgs *args);
enum DcAppResult dcAppRunTool(enum DcAppId appId, const struct DcAppRunArgs *args);
void dcAppAbortActive(void);
void dcAppRefreshActive(void);
bool dcAppGetActiveScratch(struct ToolWorkspaceSpan *spanP);
bool dcAppGetAuxScratch(struct ToolWorkspaceSpan *spanP);
bool dcAppCore1Start(DcAppCore1EntryF entry, void *context, void *stackTop);
void dcAppCore1Join(void);
void dcAppCore1ForceStop(void);
const struct DcAppCatalogEntry *dcAppCatalogEntries(uint_fast8_t *countP);
const struct DcAppCatalogEntry *dcAppCatalogFind(uint32_t appId);
const char *dcAppLastError(void);
void dcAppClearError(void);
const char *dcAppResultName(enum DcAppResult result);

#ifdef __cplusplus
}
#endif

#endif
