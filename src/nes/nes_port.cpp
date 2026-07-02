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
#include "printf.h"
#include "settings.h"
#include "timebase.h"
#include "ui.h"

void nesPortInGameMenu(void);
}

#include "InfoNES.h"
#include "InfoNES_System.h"
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
#define NES_PRESENT_INVALID 0xffffu
#define NES_PRESENT_NORMAL_WIDTH 288u
#define NES_PRESENT_NORMAL_HEIGHT 216u
#define NES_PRESENT_UPSCALED_WIDTH DISP_HEIGHT
#define NES_PRESENT_UPSCALED_HEIGHT DISP_WIDTH

static const uint8_t *mRom;
static uint32_t mRomSize;
static enum NesRegion mRegion;
static uint8_t *mSaveRam;
static uint32_t mSaveRamSize;
static uint8_t *mPool;
static uint16_t mLine[NES_LINE_WORDS];
static uint16_t mDrawLeft;
static uint16_t mDrawTop;
static uint16_t mDrawWidth;
static uint16_t mDrawHeight;
static bool mDrawFlipped;
static uint8_t *mFrame;
#ifdef DCAPP_BUILD
static uint8_t mPpuRam[0x4000u];
#endif
static uint16_t mPresentSrcX[DISP_HEIGHT];
static uint16_t mPresentSrcRow[DISP_WIDTH];
static uint16_t mPresentRowFirst;
static uint16_t mPresentRowEnd;
static uint16_t mPresentColFirst;
static uint16_t mPresentColEnd;
static uint64_t mNextEmulatedFrame;
static uint32_t mEmulatedFrameTicks;
static void (*mLedsTick)(void);
static jmp_buf mAbort;

#ifdef NES_PERF_PROFILE
#define NES_PERF_WINDOW_FRAMES 300u
static uint64_t mPerfSectionStart[2];
static uint64_t mPerfSectionTicks[2];
static uint64_t mPerfPresentTicks;
static uint64_t mPerfWorkTicks;
static uint64_t mPerfWindowStart;
static uint64_t mPerfLastBoundary;
static uint32_t mPerfWorkSamples[NES_PERF_WINDOW_FRAMES];
static uint32_t mPerfFrames;
static uint32_t mPerfRenderedFrames;
static uint32_t mPerfLateFrames;

static uint32_t nesPortPerfWorkP95(void)
{
	for (uint32_t i = 1; i < NES_PERF_WINDOW_FRAMES; i++) {
		uint32_t sample = mPerfWorkSamples[i];
		uint32_t j = i;
		while (j > 0 && mPerfWorkSamples[j - 1] > sample) {
			mPerfWorkSamples[j] = mPerfWorkSamples[j - 1];
			j--;
		}
		mPerfWorkSamples[j] = sample;
	}
	return mPerfWorkSamples[(NES_PERF_WINDOW_FRAMES * 95u + 99u) / 100u - 1u];
}
#endif

bool IsNSF = false;
bool NsfIsPlaying = false;
bool FDS_AudioEnabled = false;
BYTE *NsfBank4K[8];

const WORD NesPalette[64] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
};

#define RGB565(r, g, b) (uint16_t)((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | ((b) >> 3))

static const uint16_t NesRgbPalette[64] = {
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

#ifndef DCAPP_BUILD
extern "C" void *_sbrk(ptrdiff_t incr)
{
	(void)incr;
	return (void*)-1;
}
#endif

extern "C" void nesPortFree(void *ptr)
{
	(void)ptr;
}

extern "C" void *nesPortSaveRam(void)
{
	return mSaveRam;
}

extern "C" void *nesPortPpuRam(void)
{
#ifdef DCAPP_BUILD
	memset(mPpuRam, 0, sizeof(mPpuRam));
	return mPpuRam;
#else
	return mbcPrvGetVramBuf();
#endif
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
	uint_fast8_t speed;

	settingsGet(&settings);
	speed = settings.nesSpeed;
	if (speed >= sizeof(fpsVals) / sizeof(*fpsVals))
		speed = settings.speed;
	return (base * fpsVals[speed] + NES_FRAME_BASE_NTSC / 2) / NES_FRAME_BASE_NTSC;
}

static void nesPortResetFramePacing(void)
{
	uint32_t fps = nesPortDesiredFramerate(mRegion);

	if (!fps)
		fps = NES_FRAME_BASE_NTSC;
	mEmulatedFrameTicks = TICKS_PER_SECOND / fps;
	mNextEmulatedFrame = getTime() + mEmulatedFrameTicks;
#ifdef NES_PERF_PROFILE
	mPerfWindowStart = getTime();
	mPerfLastBoundary = mPerfWindowStart;
	memset(mPerfSectionTicks, 0, sizeof(mPerfSectionTicks));
	mPerfPresentTicks = 0;
	mPerfWorkTicks = 0;
	mPerfFrames = 0;
	mPerfRenderedFrames = 0;
	mPerfLateFrames = 0;
#endif
}

static void nesPortUpdateDrawOptions(void)
{
	struct Settings settings;
	uint32_t logicalW = DISP_HEIGHT;
	uint32_t logicalH = DISP_WIDTH;
	uint32_t i;

	settingsGet(&settings);
	mDrawWidth = settings.nesUpscale ? NES_PRESENT_UPSCALED_WIDTH : NES_PRESENT_NORMAL_WIDTH;
	mDrawHeight = settings.nesUpscale ? NES_PRESENT_UPSCALED_HEIGHT : NES_PRESENT_NORMAL_HEIGHT;
	mDrawLeft = (logicalW - mDrawWidth) / 2;
	mDrawTop = (logicalH - mDrawHeight) / 2;
	mDrawFlipped = settings.rotation;

	mPresentRowFirst = DISP_HEIGHT;
	mPresentRowEnd = 0;
	for (i = 0; i < DISP_HEIGHT; i++) {
		uint32_t logicalX = i;
		uint16_t srcX = NES_PRESENT_INVALID;

		if (logicalX >= mDrawLeft && logicalX < mDrawLeft + mDrawWidth) {
			uint32_t dx = logicalX - mDrawLeft;

			if (mDrawFlipped)
				dx = mDrawWidth - 1 - dx;
			srcX = (uint16_t)((dx * NES_DISP_WIDTH) / mDrawWidth);
			if (mPresentRowFirst == DISP_HEIGHT)
				mPresentRowFirst = (uint16_t)i;
			mPresentRowEnd = (uint16_t)(i + 1);
		}
		mPresentSrcX[i] = srcX;
	}

	mPresentColFirst = DISP_WIDTH;
	mPresentColEnd = 0;
	for (i = 0; i < DISP_WIDTH; i++) {
		uint32_t logicalY = DISP_WIDTH - 1 - i;
		uint16_t srcRow = NES_PRESENT_INVALID;

		if (logicalY >= mDrawTop && logicalY < mDrawTop + mDrawHeight) {
			uint32_t dy = logicalY - mDrawTop;

			if (mDrawFlipped)
				dy = mDrawHeight - 1 - dy;
			srcRow = (uint16_t)(((dy * NES_SAFE_HEIGHT) / mDrawHeight) * NES_DISP_WIDTH);
			if (mPresentColFirst == DISP_WIDTH)
				mPresentColFirst = (uint16_t)i;
			mPresentColEnd = (uint16_t)(i + 1);
		}
		mPresentSrcRow[i] = srcRow;
	}

	memset(dispGetFb(), 0, DISP_WIDTH * DISP_HEIGHT * DISP_BPP / 8);
}

void nesRefreshDisplayOptions(void)
{
	dispSetFramerate(nesPortDesiredFramerate(mRegion));
	nesPortUpdateDrawOptions();
	nesPortResetFramePacing();
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

static void nesPortPresentFrame(void)
{
	uint16_t *fb = (uint16_t*)dispGetFb();
#ifdef NES_PERF_PROFILE
	uint64_t perfStart = getTime();
#endif

	dispPrvWaitForScanoutStart();

	for (uint32_t py = mPresentRowFirst; py < mPresentRowEnd; py++) {
		uint32_t sx = mPresentSrcX[py];
		uint16_t *dst = fb + py * DISP_WIDTH + mPresentColFirst;

		for (uint32_t px = mPresentColFirst; px < mPresentColEnd; px++) {
			uint32_t srcOfst = mPresentSrcRow[px] + sx;

			*dst++ = NesRgbPalette[mFrame[srcOfst] & 0x3f];
		}
	}
#ifdef NES_PERF_PROFILE
	mPerfPresentTicks += getTime() - perfStart;
#endif
}

int InfoNES_LoadFrame(void)
{
	nesPortPresentFrame();
#ifdef NES_PERF_PROFILE
	mPerfRenderedFrames++;
#endif
	SRAMwritten = false;
	return 0;
}

void InfoNES_FrameBoundary(void)
{
	uint64_t now = getTime();

	if (mLedsTick)
		mLedsTick();

#ifdef NES_PERF_PROFILE
	uint64_t workTicks = now - mPerfLastBoundary;
	mPerfWorkSamples[mPerfFrames] = workTicks > UINT32_MAX ? UINT32_MAX : (uint32_t)workTicks;
	mPerfWorkTicks += workTicks;
	mPerfFrames++;
	if (now > mNextEmulatedFrame)
		mPerfLateFrames++;
#endif

	while (now < mNextEmulatedFrame)
		now = getTime();
	if (now - mNextEmulatedFrame > (uint64_t)mEmulatedFrameTicks * 4u)
		mNextEmulatedFrame = now;
	mNextEmulatedFrame += mEmulatedFrameTicks;
#ifdef NES_PERF_PROFILE
	mPerfLastBoundary = getTime();
	if (mPerfFrames >= NES_PERF_WINDOW_FRAMES) {
		uint64_t elapsed = mPerfLastBoundary - mPerfWindowStart;
		uint32_t fps100 = elapsed ? (uint32_t)((uint64_t)mPerfFrames * TICKS_PER_SECOND * 100u / elapsed) : 0;
		uint32_t render100 = elapsed ? (uint32_t)((uint64_t)mPerfRenderedFrames * TICKS_PER_SECOND * 100u / elapsed) : 0;
		uint32_t workP95 = nesPortPerfWorkP95();
		prRaw("NES perf emu=%u.%02u render=%u.%02u work=%u p95=%u cpu=%u ppu=%u present=%u late=%u\n",
			fps100 / 100u, fps100 % 100u, render100 / 100u, render100 % 100u,
			(unsigned)(mPerfWorkTicks / mPerfFrames),
			(unsigned)workP95,
			(unsigned)(mPerfSectionTicks[INFONES_PERF_CPU] / mPerfFrames),
			(unsigned)(mPerfRenderedFrames ? mPerfSectionTicks[INFONES_PERF_PPU] / mPerfRenderedFrames : 0),
			(unsigned)(mPerfRenderedFrames ? mPerfPresentTicks / mPerfRenderedFrames : 0),
			(unsigned)mPerfLateFrames);
		memset(mPerfSectionTicks, 0, sizeof(mPerfSectionTicks));
		mPerfPresentTicks = 0;
		mPerfWorkTicks = 0;
		mPerfFrames = 0;
		mPerfRenderedFrames = 0;
		mPerfLateFrames = 0;
		mPerfWindowStart = mPerfLastBoundary;
	}
#endif
}

#ifdef NES_PERF_PROFILE
void InfoNES_PerfStart(enum InfoNES_PerfSection section)
{
	mPerfSectionStart[section] = getTime();
}

void InfoNES_PerfStop(enum InfoNES_PerfSection section)
{
	mPerfSectionTicks[section] += getTime() - mPerfSectionStart[section];
}
#endif

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

void nesAbort(void)
{
	longjmp(mAbort, 1);
}

void nesRun(const void *rom, uint32_t romSize, void *saveRam, uint32_t saveRamSize,
	void (*ledsTick)(void))
{
	struct NesRomInfo info;

	if (!nesAnalyzeRom(rom, romSize, &info))
		return;

	mRom = (const uint8_t*)rom;
	mRomSize = info.romSize;
	mRegion = info.region;
	mSaveRam = (uint8_t*)saveRam;
	mSaveRamSize = saveRamSize;
	mLedsTick = ledsTick;
	mPool = (uint8_t*)CART_RAM_ADDR_IN_RAM;
	static_assert(NES_SAVE_RAM_SIZE + NES_DISP_WIDTH * NES_SAFE_HEIGHT <= QSPI_RAM_SIZE_MAX,
		"NES save RAM and frame buffer must fit the cartridge SRAM window");
	mFrame = mPool + NES_SAVE_RAM_SIZE;
	if (mSaveRam != mPool && mSaveRam && mSaveRamSize)
		memcpy(mPool, mSaveRam, mSaveRamSize > NES_SAVE_RAM_SIZE ? NES_SAVE_RAM_SIZE : mSaveRamSize);
	mSaveRam = mPool;
	mSaveRamSize = NES_SAVE_RAM_SIZE;
	memset(mFrame, 0, NES_DISP_WIDTH * NES_SAFE_HEIGHT);

	audioPwmStop();
	nesPortUpdateDrawOptions();
	dispSetFramerate(nesPortDesiredFramerate(mRegion));
	dispPrvFrameCtrReset();
	nesPortResetFramePacing();

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
	uint32_t srcY = line;
	uint32_t cropY;

	if (!pixels256 || srcY >= NES_DISP_HEIGHT)
		return;

	if (!srcY)
		memset(mFrame, 0, NES_DISP_WIDTH * NES_SAFE_HEIGHT);

	if (srcY < NES_SAFE_TOP || srcY >= NES_SAFE_BOTTOM)
		return;

	cropY = srcY - NES_SAFE_TOP;
	for (uint32_t x = 0; x < NES_DISP_WIDTH; x++)
		mFrame[cropY * NES_DISP_WIDTH + x] = (uint8_t)(pixels256[x] & 0x3f);
}
