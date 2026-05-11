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
#define NES_AUDIO_SAMPLE_RATE 11025

static const uint8_t *mRom;
static uint32_t mRomSize;
static uint8_t *mSaveRam;
static uint32_t mSaveRamSize;
static uint8_t *mPool;
static uint32_t mPoolPos;
static uint16_t *mLine;
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

const WORD NesPalette[64] = {
	0x39ce, 0x1071, 0x0015, 0x2013, 0x440e, 0x5402, 0x5000, 0x3c20,
	0x20a0, 0x0100, 0x0140, 0x00e2, 0x0ceb, 0x0000, 0x0000, 0x0000,
	0x5ef7, 0x01dd, 0x10fd, 0x401e, 0x5c17, 0x700b, 0x6ca0, 0x6521,
	0x45c0, 0x0240, 0x02a0, 0x0247, 0x0211, 0x0000, 0x0000, 0x0000,
	0x7fff, 0x1eff, 0x2e5f, 0x223f, 0x79ff, 0x7dd6, 0x7dcc, 0x7e67,
	0x7ae7, 0x4342, 0x2769, 0x2ff3, 0x03bb, 0x0000, 0x0000, 0x0000,
	0x7fff, 0x579f, 0x635f, 0x6b3f, 0x7f1f, 0x7f1b, 0x7ef6, 0x7f75,
	0x7f94, 0x73f4, 0x57d7, 0x5bf9, 0x4ffe, 0x0000, 0x0000, 0x0000,
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
	return APU_MAX_SAMPLES_PER_SYNC;
}

void InfoNES_SoundOutput(int samples, BYTE *wave1, BYTE *wave2, BYTE *wave3, BYTE *wave4, BYTE *wave5, BYTE *wave6)
{
	int32_t accum = 0;

	if (samples <= 0 || !wave1 || !wave2 || !wave3 || !wave4 || !wave5)
		return;
	for (int i = 0; i < samples; i++) {
		accum += wave1[i];
		accum += wave2[i];
		accum += wave3[i];
		accum += wave4[i];
		accum += wave5[i];
		if (wave6)
			accum += wave6[i];
	}
	accum /= samples;
	if (accum < 4)
		audioPwmStop();
	else
		(void)audioPwmTone(120 + (uint32_t)accum * 12u);
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

	if (setjmp(mAbort)) {
		InfoNES_Fin();
		audioPwmStop();
		return;
	}

	InfoNES_Main(INFONES_REGION_NTSC);
	audioPwmStop();
}

void nesPortDrawLine(uint16_t line, const uint16_t *pixels256)
{
	uint16_t *fb = (uint16_t*)dispGetFb();
	uint32_t y = line;

	if (!pixels256 || y >= DISP_HEIGHT)
		return;

	for (uint32_t x = 0; x < DISP_WIDTH; x++) {
		uint32_t sx = (x * NES_DISP_WIDTH) / DISP_WIDTH;
		fb[(DISP_HEIGHT - 1 - y) + x * DISP_WIDTH] = pixels256[sx] & 0x7fff;
	}
}
