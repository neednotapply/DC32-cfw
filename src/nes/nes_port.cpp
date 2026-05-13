#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern "C" {
#include "2350.h"
#include "audioPwm.h"
#include "dispDefcon.h"
#include "gb.h"
#include "mbc.h"
#include "memMap.h"
#include "pinoutRp2350defcon.h"
#include "settings.h"
#include "timebase.h"
#include "ui.h"

void nesPortInGameMenu(void);
}

#include "InfoNES.h"
#include "InfoNES_System.h"
#include "InfoNES_pAPU.h"
#include "nes.h"

#define NES_MAGIC0 'N'
#define NES_MAGIC1 'E'
#define NES_MAGIC2 'S'
#define NES_MAGIC3 0x1a
#define NES_LINE_WORDS 256
#define NES_FRAME_BASE_NTSC 60u
#define NES_FRAME_BASE_PAL 50u
#define NES_SAFE_TOP 8u
#define NES_SAFE_HEIGHT 224u
#define NES_SAFE_BOTTOM (NES_SAFE_TOP + NES_SAFE_HEIGHT)

static const uint8_t *mRom;
static uint32_t mRomSize;
static enum NesRegion mRegion;
static uint8_t *mSaveRam;
static uint32_t mSaveRamSize;
static uint8_t *mPool;
static uint32_t mPoolPos;
static uint16_t *mLine;
static uint16_t mDrawLeft;
static uint16_t mDrawTop;
static uint16_t mDrawWidth;
static uint16_t mDrawHeight;
static bool mDrawFlipped;
static jmp_buf mAbort;

bool IsNSF = false;
bool NsfIsPlaying = false;
bool FDS_AudioEnabled = false;
BYTE *NsfBank4K[8];
BYTE *fds_wave_buffer;
BYTE ApuFdsEnable;
BYTE (*mmc5_wave_buffers)[APU_MAX_SAMPLES_PER_SYNC];
BYTE (*vrc6_wave_buffers)[APU_MAX_SAMPLES_PER_SYNC];
BYTE (*s5b_wave_buffers)[APU_MAX_SAMPLES_PER_SYNC];

#define RGB565(r, g, b) (WORD)((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | ((b) >> 3))

const WORD NesPalette[64] = {
	RGB565(0x66, 0x66, 0x66), RGB565(0x00, 0x2a, 0x88), RGB565(0x14, 0x12, 0xa7), RGB565(0x3b, 0x00, 0xa4),
	RGB565(0x5c, 0x00, 0x7e), RGB565(0x6e, 0x00, 0x40), RGB565(0x6c, 0x07, 0x00), RGB565(0x56, 0x1d, 0x00),
	RGB565(0x33, 0x35, 0x00), RGB565(0x0b, 0x48, 0x00), RGB565(0x00, 0x52, 0x00), RGB565(0x00, 0x4f, 0x08),
	RGB565(0x00, 0x40, 0x4d), RGB565(0x00, 0x00, 0x00), RGB565(0x00, 0x00, 0x00), RGB565(0x00, 0x00, 0x00),
	RGB565(0xad, 0xad, 0xad), RGB565(0x15, 0x5f, 0xd9), RGB565(0x42, 0x40, 0xff), RGB565(0x75, 0x27, 0xfe),
	RGB565(0xa0, 0x1a, 0xcc), RGB565(0xb7, 0x1e, 0x7b), RGB565(0xb5, 0x31, 0x20), RGB565(0x99, 0x4e, 0x00),
	RGB565(0x6b, 0x6d, 0x00), RGB565(0x38, 0x87, 0x00), RGB565(0x0c, 0x93, 0x00), RGB565(0x00, 0x8f, 0x32),
	RGB565(0x00, 0x7c, 0x8d), RGB565(0x00, 0x00, 0x00), RGB565(0x00, 0x00, 0x00), RGB565(0x00, 0x00, 0x00),
	RGB565(0xff, 0xfe, 0xff), RGB565(0x64, 0xb0, 0xff), RGB565(0x92, 0x90, 0xff), RGB565(0xc6, 0x76, 0xff),
	RGB565(0xf3, 0x6a, 0xff), RGB565(0xfe, 0x6e, 0xcc), RGB565(0xfe, 0x81, 0x70), RGB565(0xea, 0x9e, 0x22),
	RGB565(0xbc, 0xbe, 0x00), RGB565(0x88, 0xd8, 0x00), RGB565(0x5c, 0xe4, 0x30), RGB565(0x45, 0xe0, 0x82),
	RGB565(0x48, 0xcd, 0xde), RGB565(0x4f, 0x4f, 0x4f), RGB565(0x00, 0x00, 0x00), RGB565(0x00, 0x00, 0x00),
	RGB565(0xff, 0xfe, 0xff), RGB565(0xc0, 0xdf, 0xff), RGB565(0xd3, 0xd2, 0xff), RGB565(0xe8, 0xc8, 0xff),
	RGB565(0xfb, 0xc2, 0xff), RGB565(0xfe, 0xc4, 0xea), RGB565(0xfe, 0xcc, 0xc5), RGB565(0xf7, 0xd8, 0xa5),
	RGB565(0xe4, 0xe5, 0x94), RGB565(0xcf, 0xef, 0x96), RGB565(0xbd, 0xf4, 0xab), RGB565(0xb3, 0xf3, 0xcc),
	RGB565(0xb5, 0xeb, 0xf2), RGB565(0xb8, 0xb8, 0xb8), RGB565(0x00, 0x00, 0x00), RGB565(0x00, 0x00, 0x00),
};

static void *nesPortAllocAligned(uint32_t size, uint32_t align)
{
	uintptr_t base = (uintptr_t)mPool;
	uint32_t pos = (uint32_t)(((base + mPoolPos + align - 1) &~ (uintptr_t)(align - 1)) - base);
	void *ret;

	if (!mPool || pos > QSPI_RAM_SIZE_MAX || size > QSPI_RAM_SIZE_MAX - pos)
		return NULL;
	ret = mPool + pos;
	mPoolPos = pos + size;
	memset(ret, 0, size);
	return ret;
}

extern "C" void *nesPortAlloc(size_t size)
{
	return nesPortAllocAligned((uint32_t)size, 4);
}

extern "C" void nesPortFree(void *ptr)
{
	(void)ptr;
}

extern "C" void *_sbrk(ptrdiff_t incr)
{
	(void)incr;
	return (void*)-1;
}

extern "C" void *nesPortSaveRam(void)
{
	return mSaveRam;
}

extern "C" void *nesPortPpuRam(void)
{
	return mbcPrvGetVramBuf();
}

extern "C" void *nesPortChrBuf(void)
{
	return mbcPrvGetWramBuf();
}

uint32_t nesGetLoadedRomSize(void)
{
	return mRomSize;
}

static enum NesRegion nesPortDetectRegion(const uint8_t *bytes)
{
	if ((bytes[7] & 0x0c) == 0x08) {
		switch (bytes[12] & 0x03) {
			case 1:
				return NesRegionPal;
			case 3:
				return NesRegionDendy;
			default:
				return NesRegionNtsc;
		}
	}

	return (bytes[9] & 0x01) ? NesRegionPal : NesRegionNtsc;
}

static uint32_t nesPortDesiredFramerate(enum NesRegion region)
{
	static const uint8_t fpsVals[] = DISP_SPEED_SETTINGS;
	struct Settings settings;
	uint32_t base = (region == NesRegionNtsc) ? NES_FRAME_BASE_NTSC : NES_FRAME_BASE_PAL;

	settingsGet(&settings);
	if (settings.speed >= sizeof(fpsVals) / sizeof(*fpsVals))
		settings.speed = 1;
	return (base * fpsVals[settings.speed] + NES_FRAME_BASE_NTSC / 2) / NES_FRAME_BASE_NTSC;
}

static void nesPortUpdateDrawOptions(void)
{
	struct Settings settings;
	uint32_t logicalW = DISP_HEIGHT;
	uint32_t logicalH = DISP_WIDTH;

	settingsGet(&settings);
	mDrawWidth = settings.upscale ? logicalW : 288;
	mDrawHeight = settings.upscale ? logicalH : 216;
	mDrawLeft = (logicalW - mDrawWidth) / 2;
	mDrawTop = (logicalH - mDrawHeight) / 2;
	mDrawFlipped = settings.rotation;
}

bool nesAnalyzeRom(const void *rom, uint32_t size, struct NesRomInfo *info)
{
	const uint8_t *bytes = (const uint8_t*)rom;
	uint32_t prgSize, chrSize, trainerSize, expectedSize;
	uint8_t mapper;

	if (!bytes || size < sizeof(struct NesHeader_tag))
		return false;
	if (bytes[0] != NES_MAGIC0 || bytes[1] != NES_MAGIC1 || bytes[2] != NES_MAGIC2 || bytes[3] != NES_MAGIC3)
		return false;

	prgSize = (uint32_t)bytes[4] * 0x4000u;
	chrSize = (uint32_t)bytes[5] * 0x2000u;
	trainerSize = (bytes[6] & 0x04) ? 512u : 0u;
	expectedSize = sizeof(struct NesHeader_tag) + trainerSize + prgSize + chrSize;
	if (!prgSize || expectedSize > size)
		return false;

	mapper = (uint8_t)((bytes[6] >> 4) | (bytes[7] & 0xf0));
	if (mapper != 0 && mapper != 1 && mapper != 2 && mapper != 3 && mapper != 4 && mapper != 7)
		return false;

	if (info) {
		memset(info, 0, sizeof(*info));
		info->romSize = expectedSize;
		info->saveRamSize = NES_SAVE_RAM_SIZE;
		info->mapper = mapper;
		info->region = nesPortDetectRegion(bytes);
		info->hasSaveRam = (bytes[6] & 0x02) != 0;
		info->name[0] = 'N';
		info->name[1] = 'E';
		info->name[2] = 'S';
		info->name[3] = ' ';
		info->name[4] = 'M';
		if (mapper >= 100) {
			info->name[5] = (char)('0' + mapper / 100);
			info->name[6] = (char)('0' + (mapper / 10) % 10);
			info->name[7] = (char)('0' + mapper % 10);
			info->name[8] = 0;
		}
		else if (mapper >= 10) {
			info->name[5] = (char)('0' + mapper / 10);
			info->name[6] = (char)('0' + mapper % 10);
			info->name[7] = 0;
		}
		else {
			info->name[5] = (char)('0' + mapper);
			info->name[6] = 0;
		}
	}
	return true;
}

static uint_fast8_t nesPortKeysToPad(void)
{
	uint_fast8_t keys = uiGetKeysRaw();
	uint_fast8_t pad = 0;

	if (keys & KEY_BIT_A) pad |= 0x01;
	if (keys & KEY_BIT_B) pad |= 0x02;
	if (keys & KEY_BIT_SEL) pad |= 0x04;
	if (keys & KEY_BIT_START) pad |= 0x08;
	if (keys & KEY_BIT_UP) pad |= 0x10;
	if (keys & KEY_BIT_DOWN) pad |= 0x20;
	if (keys & KEY_BIT_LEFT) pad |= 0x40;
	if (keys & KEY_BIT_RIGHT) pad |= 0x80;
	return pad;
}

int InfoNES_ReadRom(const char *pszFileName)
{
	(void)pszFileName;
	if (!nesAnalyzeRom(mRom, mRomSize, NULL))
		return -1;
	memcpy(&NesHeader, mRom, sizeof(NesHeader));
	ROM = (BYTE*)(mRom + sizeof(NesHeader));
	if (NesHeader.byInfo1 & 0x04)
		ROM += 512;
	VROM = NesHeader.byVRomSize ? ROM + (NesHeader.byRomSize * 0x4000u) : NULL;
	return 0;
}

void InfoNES_ReleaseRom(void)
{
	ROM = NULL;
	VROM = NULL;
}

int InfoNES_Menu(void)
{
	return InfoNES_Load("flash") == 0 ? 0 : -1;
}

void InfoNES_PadState(DWORD *pad1, DWORD *pad2, DWORD *system)
{
	uint32_t gpio = sio_hw->gpio_in;

	if (!(gpio & (1u << PIN_BTN_CENTER))) {
		nesPortInGameMenu();
		while (!(sio_hw->gpio_in & (1u << PIN_BTN_CENTER)));
		gpio = sio_hw->gpio_in;
	}

	*pad1 = nesPortKeysToPad();
	*pad2 = 0;
	*system = 0;
}

void InfoNES_PreDrawLine(int line)
{
	(void)line;
	InfoNES_SetLineBuffer(mLine, NES_LINE_WORDS);
}

void InfoNES_PostDrawLine(int line)
{
	nesPortDrawLine((uint16_t)line, mLine);
}

int InfoNES_LoadFrame(void)
{
	dispPrvFrameCtrWait();
	return 0;
}

void InfoNES_MessageBox(const char *pszMsg, ...)
{
	(void)pszMsg;
}

void InfoNES_Error(const char *pszMsg, ...)
{
	(void)pszMsg;
}

void InfoNES_DebugPrint(const char *pszMsg)
{
	(void)pszMsg;
}

void InfoNES_SoundInit(void)
{
}

int InfoNES_SoundOpen(int samples_per_sync, int sample_rate)
{
	(void)samples_per_sync;
	(void)sample_rate;
	return 0;
}

void InfoNES_SoundClose(void)
{
	audioPwmStop();
}

int InfoNES_GetSoundBufferSize(void)
{
	return 0;
}

void InfoNES_SoundOutput(int samples, BYTE *wave1, BYTE *wave2, BYTE *wave3, BYTE *wave4, BYTE *wave5, BYTE *wave6)
{
	(void)samples;
	(void)wave1;
	(void)wave2;
	(void)wave3;
	(void)wave4;
	(void)wave5;
	(void)wave6;
}

void nesAbort(void)
{
	longjmp(mAbort, 1);
}

void nesRun(const void *rom, uint32_t romSize, void *saveRam, uint32_t saveRamSize)
{
	struct NesRomInfo info;

	if (!nesAnalyzeRom(rom, romSize, &info))
		return;

	mRom = (const uint8_t*)rom;
	mRomSize = info.romSize;
	mRegion = info.region;
	mSaveRam = (uint8_t*)saveRam;
	mSaveRamSize = saveRamSize;
	mPool = (uint8_t*)CART_RAM_ADDR_IN_RAM;
	mPoolPos = NES_SAVE_RAM_SIZE;
	if (mSaveRam != mPool && mSaveRam && mSaveRamSize)
		memcpy(mPool, mSaveRam, mSaveRamSize > NES_SAVE_RAM_SIZE ? NES_SAVE_RAM_SIZE : mSaveRamSize);
	mSaveRam = mPool;
	mSaveRamSize = NES_SAVE_RAM_SIZE;

	mLine = (uint16_t*)nesPortAllocAligned(NES_LINE_WORDS * sizeof(uint16_t), 4);
	if (!mLine)
		return;

	APU_Mute = 1;
	audioPwmStop();
	nesPortUpdateDrawOptions();
	dispSetFramerate(nesPortDesiredFramerate(mRegion));
	dispPrvFrameCtrReset();

	if (setjmp(mAbort)) {
		InfoNES_Fin();
		audioPwmStop();
		dispSetFramerate(nesPortDesiredFramerate(NesRegionNtsc));
		return;
	}

	InfoNES_Main(mRegion == NesRegionPal ? INFONES_REGION_PAL : (mRegion == NesRegionDendy ? INFONES_REGION_DENDY : INFONES_REGION_NTSC));
	audioPwmStop();
	dispSetFramerate(nesPortDesiredFramerate(NesRegionNtsc));
}

void nesPortDrawLine(uint16_t line, const uint16_t *pixels256)
{
	uint16_t *fb = (uint16_t*)dispGetFb();
	uint32_t srcY = line;
	uint32_t cropY;
	uint32_t y0, y1;

	if (!pixels256 || srcY >= NES_DISP_HEIGHT || !mDrawWidth || !mDrawHeight)
		return;

	if (!srcY)
		memset(fb, 0, DISP_WIDTH * DISP_HEIGHT * sizeof(*fb));

	if (srcY < NES_SAFE_TOP || srcY >= NES_SAFE_BOTTOM)
		return;

	cropY = srcY - NES_SAFE_TOP;
	y0 = (cropY * mDrawHeight) / NES_SAFE_HEIGHT;
	y1 = ((cropY + 1) * mDrawHeight) / NES_SAFE_HEIGHT;
	for (uint32_t dy = y0; dy < y1; dy++) {
		uint32_t logicalY = mDrawTop + dy;
		uint32_t outY = mDrawFlipped ? (mDrawTop + mDrawHeight - 1 - dy) : logicalY;

		if (outY >= DISP_WIDTH)
			continue;

		for (uint32_t dx = 0; dx < mDrawWidth; dx++) {
			uint32_t sx = (dx * NES_DISP_WIDTH) / mDrawWidth;
			uint32_t outX = mDrawFlipped ? (mDrawLeft + mDrawWidth - 1 - dx) : (mDrawLeft + dx);

			if (outX < DISP_HEIGHT)
				fb[outX * DISP_WIDTH + (DISP_WIDTH - 1 - outY)] = pixels256[sx];
		}
	}
}
