//http://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
//http://web.archive.org/web/20010303020156/http://www.fh-karlsruhe.de/fbnw/html/Gameboy/Docs/Gbspec.txt
//https://gbdev.gg8.se/wiki/articles/CGB_Registers
//http://nocash.emubase.de/pandocs.htm#videodisplay
//http://slack.net/~ant/old/gb-tests/
//CGB TODO: HBLANK DMA, horiz & vert flip for BG tiles, BG priority
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "frontend.h"
#include "printf.h"
#include "gbCartHeader.h"
#include "gbCore.h"
#include "gb.h"
#include "settings.h"

#pragma GCC optimize ("Ofast")


CachedCgbPal cgbBgPals;
CachedCgbPal cgbFgPals;
CachedDmgPal dmgBgPal;
CachedDmgPal dmgFgPals[2];


static uint32_t divOfst;
static uint8_t mDmgPalette = GameBoyPaletteBw;

extern bool doubleSpeed;

enum GbDmgPalettePlane {
	GbDmgPaletteBg,
	GbDmgPaletteObj0,
	GbDmgPaletteObj1,
	GbDmgPaletteNumPlanes,
};

#define GB_DMG_PAL(c0, c1, c2, c3) {c0, c1, c2, c3}
#define GB_DMG_UNIFORM(c0, c1, c2, c3) {GB_DMG_PAL(c0, c1, c2, c3), GB_DMG_PAL(c0, c1, c2, c3), GB_DMG_PAL(c0, c1, c2, c3)}
// GBC boot palette combinations are stored here in BG, OBJ0, OBJ1 plane order.
#define GB_DMG_PLANES(bg, obj0, obj1) {bg, obj0, obj1}

#define GB_GBC_BOOT_PAL_0 GB_DMG_PAL(0xFFDF, 0xFD4C, 0x8180, 0x0000)
#define GB_GBC_BOOT_PAL_1 GB_DMG_PAL(0xFF18, 0xCCD0, 0x8345, 0x5981)
#define GB_GBC_BOOT_PAL_2 GB_DMG_PAL(0xFFDF, 0x8C5B, 0x5291, 0x0000)
#define GB_GBC_BOOT_PAL_3 GB_DMG_PAL(0xFFDF, 0x7FC6, 0x0400, 0x0000)
#define GB_GBC_BOOT_PAL_4 GB_DMG_PAL(0xFFDF, 0xFC10, 0x91C7, 0x0000)
#define GB_GBC_BOOT_PAL_5 GB_DMG_PAL(0xFFDF, 0xA514, 0x528A, 0x0000)
#define GB_GBC_BOOT_PAL_6 GB_DMG_PAL(0xFFDF, 0xFFC0, 0x7A40, 0x0000)
#define GB_GBC_BOOT_PAL_12 GB_DMG_PAL(0xFFD4, 0xFC92, 0x949F, 0x0000)
#define GB_GBC_BOOT_PAL_18 GB_DMG_PAL(0xFFDF, 0x57C0, 0xFA00, 0x0000)
#define GB_GBC_BOOT_PAL_24 GB_DMG_PAL(0xFFDF, 0xFFC0, 0xF800, 0x0000)
#define GB_GBC_BOOT_PAL_26 GB_DMG_PAL(0xFFDF, 0xFE40, 0x9B00, 0x0000)
#define GB_GBC_BOOT_PAL_27 GB_DMG_PAL(0x0000, 0x0410, 0xFEC0, 0xFFDF)
#define GB_GBC_BOOT_PAL_28 GB_DMG_PAL(0xFFDF, 0x651F, 0x001F, 0x0000)
#define GB_GBC_BOOT_PAL_29 GB_DMG_PAL(0xFFDF, 0x7FC6, 0x0318, 0x0000)

static uint16_t mActiveDmgPaletteColors[GbDmgPaletteNumPlanes][4] =
	GB_DMG_UNIFORM(0xFFDF, 0xAD55, 0x528A, 0x0000);

static const uint16_t mGbDmgPaletteColors[GameBoyPaletteNumPalettes][GbDmgPaletteNumPlanes][4] = {
	[GameBoyPaletteBw] = GB_DMG_UNIFORM(0xFFDF, 0xAD55, 0x528A, 0x0000), // bw: Black & White
	[GameBoyPaletteDmg] = GB_DMG_UNIFORM(0x9DC1, 0x7502, 0x3306, 0x09C1), // dmg: Original Game Boy
	[GameBoyPaletteGbpocket] = GB_DMG_UNIFORM(0xC654, 0x8C8D, 0x4A87, 0x18C3), // gbpocket: Game Boy Pocket
	[GameBoyPaletteBgb] = GB_DMG_UNIFORM(0xE7DA, 0x8E0E, 0x334A, 0x08C4), // bgb: BGB Emulator
	[GameBoyPaletteGbli] = GB_DMG_UNIFORM(0x1ED9, 0x1E16, 0x1512, 0x0BCD), // gbli: Game Boy Light
	[GameBoyPaletteGrafixkidgray] = GB_DMG_UNIFORM(0xE6D9, 0xACD2, 0x734C, 0x2944), // grafixkidgray: Grafixkid Gray
	[GameBoyPaletteGrafixkidgreen] = GB_DMG_UNIFORM(0xDF96, 0xAE12, 0x7C8F, 0x4B0B), // grafixkidgreen: Grafixkid Green
	[GameBoyPaletteBlackzero] = GB_DMG_UNIFORM(0x7C02, 0x53C8, 0x3AC9, 0x2A07), // blackzero: Game Boy (Black Zero) palette
	[GameBoyPaletteGbcjp] = GB_DMG_PLANES(GB_GBC_BOOT_PAL_26, GB_GBC_BOOT_PAL_26, GB_GBC_BOOT_PAL_26), // gbcjp: Pocket Camera JP
	[GameBoyPaletteGbcu] = GB_DMG_PLANES(GB_GBC_BOOT_PAL_0, GB_GBC_BOOT_PAL_0, GB_GBC_BOOT_PAL_0), // gbcu: Brown
	[GameBoyPaletteGbcua] = GB_DMG_PLANES(GB_GBC_BOOT_PAL_4, GB_GBC_BOOT_PAL_3, GB_GBC_BOOT_PAL_28), // gbcua: Red
	[GameBoyPaletteGbcub] = GB_DMG_PLANES(GB_GBC_BOOT_PAL_1, GB_GBC_BOOT_PAL_0, GB_GBC_BOOT_PAL_0), // gbcub: Dark Brown
	[GameBoyPaletteGbcl] = GB_DMG_PLANES(GB_GBC_BOOT_PAL_28, GB_GBC_BOOT_PAL_4, GB_GBC_BOOT_PAL_3), // gbcl: Blue
	[GameBoyPaletteGbcla] = GB_DMG_PLANES(GB_GBC_BOOT_PAL_2, GB_GBC_BOOT_PAL_4, GB_GBC_BOOT_PAL_0), // gbcla: Dark Blue
	[GameBoyPaletteGbclb] = GB_DMG_PLANES(GB_GBC_BOOT_PAL_5, GB_GBC_BOOT_PAL_5, GB_GBC_BOOT_PAL_5), // gbclb: Gray
	[GameBoyPaletteGbcd] = GB_DMG_PLANES(GB_GBC_BOOT_PAL_12, GB_GBC_BOOT_PAL_12, GB_GBC_BOOT_PAL_12), // gbcd: Pale Yellow
	[GameBoyPaletteGbcda] = GB_DMG_PLANES(GB_GBC_BOOT_PAL_24, GB_GBC_BOOT_PAL_24, GB_GBC_BOOT_PAL_24), // gbcda: Orange
	[GameBoyPaletteGbcdb] = GB_DMG_PLANES(GB_GBC_BOOT_PAL_6, GB_GBC_BOOT_PAL_28, GB_GBC_BOOT_PAL_3), // gbcdb: Yellow
	[GameBoyPaletteGbcr] = GB_DMG_PLANES(GB_GBC_BOOT_PAL_18, GB_GBC_BOOT_PAL_18, GB_GBC_BOOT_PAL_18), // gbcr: Green
	[GameBoyPaletteGbceuus] = GB_DMG_PLANES(GB_GBC_BOOT_PAL_29, GB_GBC_BOOT_PAL_4, GB_GBC_BOOT_PAL_4), // gbceuus: Dark Green
	[GameBoyPaletteGbcrb] = GB_DMG_PLANES(GB_GBC_BOOT_PAL_27, GB_GBC_BOOT_PAL_27, GB_GBC_BOOT_PAL_27), // gbcrb: Reverse
	[GameBoyPaletteGbcPreferred] = GB_DMG_PLANES(GB_GBC_BOOT_PAL_29, GB_GBC_BOOT_PAL_4, GB_GBC_BOOT_PAL_4), // gbcpreferred: GBC boot ROM default/fallback
};

static const uint16_t mGbcBootPaletteWords[] = {
	0xFFDF, 0xFD4C, 0x8180, 0x0000, // 0
	0xFF18, 0xCCD0, 0x8345, 0x5981, // 1
	0xFFDF, 0x8C5B, 0x5291, 0x0000, // 2
	0xFFDF, 0x7FC6, 0x0400, 0x0000, // 3
	0xFFDF, 0xFC10, 0x91C7, 0x0000, // 4
	0xFFDF, 0xA514, 0x528A, 0x0000, // 5
	0xFFDF, 0xFFC0, 0x7A40, 0x0000, // 6
	0xFFDF, 0x7FC0, 0xB380, 0x0000, // 7
	0xFFDF, 0xAD50, 0x438F, 0x0000, // 8
	0xA4DF, 0xFFC0, 0x0300, 0x0000, // 9
	0xFFD9, 0x675D, 0x9C06, 0x5ACB, // 10
	0xB59F, 0xFFD2, 0xAAC8, 0x0000, // 11
	0xFFD4, 0xFC92, 0x949F, 0x0000, // 12
	0xFFD3, 0x959F, 0x648E, 0x01C7, // 13
	0x6FC0, 0xFFDF, 0xFA89, 0x0000, // 14
	0x56C0, 0xFC00, 0xFFC0, 0xFFDF, // 15
	0xFFDF, 0xFB80, 0x9200, 0x0000, // 16
	0xFE08, 0xFE80, 0x91C0, 0x4800, // 17
	0xFFDF, 0x57C0, 0xFA00, 0x0000, // 18
	0xFB0A, 0xD000, 0x6000, 0x0000, // 19
	0xFFDF, 0xFCC0, 0xF800, 0x0000, // 20
	0xFFDF, 0x07C0, 0x3400, 0x0240, // 21
	0xFFDF, 0x5DDF, 0xF800, 0x001F, // 22
	0xFFDF, 0xFFCF, 0x041F, 0xF800, // 23
	0xFFDF, 0xFFC0, 0xF800, 0x0000, // 24
	0xFFC0, 0xF800, 0x6000, 0x0000, // 25
	0xFFDF, 0xFE40, 0x9B00, 0x0000, // 26
	0x0000, 0x0410, 0xFEC0, 0xFFDF, // 27
	0xFFDF, 0x651F, 0x001F, 0x0000, // 28
	0xFFDF, 0x7FC6, 0x0318, 0x0000, // 29
};

static const uint8_t mGbcBootPaletteCombinationOffsets[] = {
	16, 16, 116, 72, 72, 72, 80, 80, 80, 96, 96, 96,
	36, 36, 36, 0, 0, 0, 108, 108, 108, 20, 20, 20,
	48, 48, 48, 104, 104, 104, 64, 32, 32, 16, 112, 112,
	16, 8, 8, 12, 16, 16, 16, 116, 116, 112, 16, 112,
	8, 68, 8, 64, 64, 32, 16, 16, 28, 16, 16, 72,
	16, 16, 80, 76, 76, 36, 15, 15, 44, 68, 68, 8,
	16, 16, 8, 16, 16, 12, 112, 112, 0, 12, 12, 0,
	0, 0, 4, 72, 88, 72, 80, 88, 80, 96, 88, 96,
	64, 88, 32, 68, 16, 52, 111, 0, 56, 111, 16, 60,
	76, 91, 36, 64, 112, 40, 16, 92, 112, 68, 88, 8,
	16, 0, 8, 16, 112, 12, 112, 12, 0, 12, 112, 16,
	84, 112, 16, 12, 112, 0, 100, 12, 112, 0, 112, 32,
	16, 12, 112, 112, 12, 24, 16, 112, 116,
};

static const uint8_t mGbcBootTitleChecksums[] = {
	0x00, 0x88, 0x16, 0x36, 0xD1, 0xDB, 0xF2, 0x3C, 0x8C, 0x92, 0x3D, 0x5C, 0x58, 0xC9, 0x3E, 0x70,
	0x1D, 0x59, 0x69, 0x19, 0x35, 0xA8, 0x14, 0xAA, 0x75, 0x95, 0x99, 0x34, 0x6F, 0x15, 0xFF, 0x97,
	0x4B, 0x90, 0x17, 0x10, 0x39, 0xF7, 0xF6, 0xA2, 0x49, 0x4E, 0x43, 0x68, 0xE0, 0x8B, 0xF0, 0xCE,
	0x0C, 0x29, 0xE8, 0xB7, 0x86, 0x9A, 0x52, 0x01, 0x9D, 0x71, 0x9C, 0xBD, 0x5D, 0x6D, 0x67, 0x3F,
	0x6B, 0xB3, 0x46, 0x28, 0xA5, 0xC6, 0xD3, 0x27, 0x61, 0x18, 0x66, 0x6A, 0xBF, 0x0D, 0xF4, 0xB3,
	0x46, 0x28, 0xA5, 0xC6, 0xD3, 0x27, 0x61, 0x18, 0x66, 0x6A, 0xBF, 0x0D, 0xF4, 0xB3,
};

static const uint8_t mGbcBootPaletteByTitleChecksum[] = {
	0x00, 0x04, 0x05, 0x23, 0x22, 0x03, 0x1F, 0x0F, 0x0A, 0x05, 0x13, 0x24, 0x87, 0x25, 0x1E, 0x2C,
	0x15, 0x20, 0x1F, 0x14, 0x05, 0x21, 0x0D, 0x0E, 0x05, 0x1D, 0x05, 0x12, 0x09, 0x03, 0x02, 0x1A,
	0x19, 0x19, 0x29, 0x2A, 0x1A, 0x2D, 0x2A, 0x2D, 0x24, 0x26, 0x9A, 0x2A, 0x1E, 0x29, 0x22, 0x22,
	0x05, 0x2A, 0x06, 0x05, 0x21, 0x19, 0x2A, 0x2A, 0x28, 0x02, 0x10, 0x19, 0x2A, 0x2A, 0x05, 0x00,
	0x27, 0x24, 0x16, 0x19, 0x06, 0x20, 0x0C, 0x24, 0x0B, 0x27, 0x12, 0x27, 0x18, 0x1F, 0x32, 0x11,
	0x2E, 0x06, 0x1B, 0x00, 0x2F, 0x29, 0x29, 0x00, 0x00, 0x13, 0x22, 0x17, 0x12, 0x1D,
};

#define GB_GBC_BOOT_FIRST_DUPLICATE_CHECKSUM 65u
static const char mGbcBootDuplicateFourthLetters[] = "BEFAARBEKEK R-URAR INAILICE R";

#undef GB_GBC_BOOT_PAL_29
#undef GB_GBC_BOOT_PAL_28
#undef GB_GBC_BOOT_PAL_27
#undef GB_GBC_BOOT_PAL_26
#undef GB_GBC_BOOT_PAL_24
#undef GB_GBC_BOOT_PAL_18
#undef GB_GBC_BOOT_PAL_12
#undef GB_GBC_BOOT_PAL_6
#undef GB_GBC_BOOT_PAL_5
#undef GB_GBC_BOOT_PAL_4
#undef GB_GBC_BOOT_PAL_3
#undef GB_GBC_BOOT_PAL_2
#undef GB_GBC_BOOT_PAL_1
#undef GB_GBC_BOOT_PAL_0
#undef GB_DMG_PLANES
#undef GB_DMG_UNIFORM
#undef GB_DMG_PAL

void gbIoInit(void)
{
	divOfst = 0;
}

static void gbPrvLoadDmgPaletteColors(const uint16_t colors[GbDmgPaletteNumPlanes][4])
{
	memcpy(mActiveDmgPaletteColors, colors, sizeof(mActiveDmgPaletteColors));
}

static void gbPrvLoadGbcBootPalette(uint_fast8_t combinationOffset)
{
	uint_fast8_t bg, obj0, obj1;

	if (combinationOffset + 2u >= sizeof(mGbcBootPaletteCombinationOffsets))
		combinationOffset = 0;

	obj0 = mGbcBootPaletteCombinationOffsets[combinationOffset];
	obj1 = mGbcBootPaletteCombinationOffsets[combinationOffset + 1u];
	bg = mGbcBootPaletteCombinationOffsets[combinationOffset + 2u];
	if (bg + 3u >= sizeof(mGbcBootPaletteWords) / sizeof(*mGbcBootPaletteWords) ||
		obj0 + 3u >= sizeof(mGbcBootPaletteWords) / sizeof(*mGbcBootPaletteWords) ||
		obj1 + 3u >= sizeof(mGbcBootPaletteWords) / sizeof(*mGbcBootPaletteWords)) {
		bg = 116;
		obj0 = 16;
		obj1 = 16;
	}

	memcpy(mActiveDmgPaletteColors[GbDmgPaletteBg],
		mGbcBootPaletteWords + bg,
		sizeof(mActiveDmgPaletteColors[GbDmgPaletteBg]));
	memcpy(mActiveDmgPaletteColors[GbDmgPaletteObj0],
		mGbcBootPaletteWords + obj0,
		sizeof(mActiveDmgPaletteColors[GbDmgPaletteObj0]));
	memcpy(mActiveDmgPaletteColors[GbDmgPaletteObj1],
		mGbcBootPaletteWords + obj1,
		sizeof(mActiveDmgPaletteColors[GbDmgPaletteObj1]));
}

static uint_fast8_t gbPrvGbcBootPaletteForRom(const void *rom, uint32_t romSize)
{
	const struct CartHeader *hdr = (const struct CartHeader*)rom;
	uint_fast8_t i, titleChecksum = 0;

	if (!rom || romSize < sizeof(struct CartHeader))
		return 0;
	if (hdr->oldLicenseeCode == 0x33) {
		if (hdr->newLicenseeCode[0] != '0' || hdr->newLicenseeCode[1] != '1')
			return 0;
	} else if (hdr->oldLicenseeCode != 0x01) {
		return 0;
	}

	for (i = 0; i < sizeof(hdr->titleDMG); i++)
		titleChecksum += hdr->titleDMG[i];

	for (i = 0; i < sizeof(mGbcBootTitleChecksums); i++) {
		if (titleChecksum != mGbcBootTitleChecksums[i])
			continue;
		if (i >= GB_GBC_BOOT_FIRST_DUPLICATE_CHECKSUM &&
			hdr->titleDMG[3] != mGbcBootDuplicateFourthLetters[i - GB_GBC_BOOT_FIRST_DUPLICATE_CHECKSUM])
			continue;
		return mGbcBootPaletteByTitleChecksum[i] & 0x7f;
	}

	return 0;
}

void gbSetDmgPalette(uint_fast8_t palette)
{
	mDmgPalette = palette < GameBoyPaletteNumPalettes ? palette : GameBoyPaletteBw;
	if (mDmgPalette == GameBoyPaletteGbcPreferred)
		gbPrvLoadGbcBootPalette(0);
	else
		gbPrvLoadDmgPaletteColors(mGbDmgPaletteColors[mDmgPalette]);
}

void gbSetDmgPaletteForRom(const void *rom, uint32_t romSize, uint_fast8_t palette)
{
	if (palette == GameBoyPaletteGbcPreferred) {
		mDmgPalette = GameBoyPaletteGbcPreferred;
		gbPrvLoadGbcBootPalette(gbPrvGbcBootPaletteForRom(rom, romSize));
		return;
	}

	gbSetDmgPalette(palette);
}

uint8_t gbIoRead(uint16_t addr)
{
	uint8_t *hram = mHRAM;
	uint8_t t, r = 0;
	int16_t v16;
	
	addr &= 0xff;
	//pr("IO RD 0xFF%02X ->", addr);
	
	switch (addr){
		
		case 0x00:	//JOYP
			t = ~gbExtGetKeys();
			if((hram[0x00] & 0x30) == 0x10) r = (hram[0x00] & 0xF0) | (t >> 4);
			else if((hram[0x00] & 0x30) == 0x20) r = (hram[0x00] & 0xF0) | (t & 0x0F);
			else if((hram[0x00] & 0x30) == 0x00) r = 0xF0;
			break;
		
		case 0x04:	//DIV
			return (gbGetCy() - divOfst) >> 6;
		
		case 0x69:
			r = hram[HRAM_INDEX_BG_PAL + (hram[0x68] & 63)];
			break;
			
		case 0x6B:
			r = hram[HRAM_INDEX_OB_PAL + (hram[0x6A] & 63)];
			break;
		
		case 0x4d:
			r = (gbIsDoubleSpeed() ? 0x80 : 0x00) + (hram[addr] & 1);
			break;
		
		case 0x10:
			r = 0x80 | hram[addr];
			break;

		case 0x11:
		case 0x16:
			r = 0x3f | hram[addr];
			break;

		case 0x13:	//WO audio regs
		case 0x15:
		case 0x18:
		case 0x1b:
		case 0x1d:
		case 0x1f:
		case 0x20:
			r = 0xff;
			break;

		case 0x14:
		case 0x19:
		case 0x1e:
		case 0x23:
			r = 0xbf | hram[addr];
			break;

		case 0x1a:
			r = 0x7f | hram[addr];
			break;

		case 0x1c:
			r = 0x9f | hram[addr];
			break;

		case 0x26:
			r = 0x70 | hram[addr];
			break;

		case 0x03:
		case 0x08 ... 0x0e:
		case 0x27 ... 0x2f:
		case 0x4c:
		case 0x4e:
		case 0x57 ... 0x67:
		case 0x6d ... 0x6f:
		case 0x71:
		case 0x76 ... 0x7f:
			(void)v16;
	#ifdef GB_HAVE_EXTRA_IO_REGS
			v16 = gbExtraIoRegRead(addr);
			if (v16 >= 0) {
				r = v16;
				break;
			}
	#endif
			accessFail(addr, 0, 0);
			r = 0;
			break;

		default:

	normal_read:
			r = hram[addr];
			break;
	}
	
	//pr("%02X\n", r);
	
	return r;
}




static void gbPrvRecalcCgbPal(CachedCgbPal dstPal, const uint8_t *srcPal, uint_fast8_t writtenOffset)
{
	uint_fast8_t idx = writtenOffset / 2;
	uint_fast16_t gbcolor = ((const uint16_t*)srcPal)[idx];
	uint16_t ourCol;
	
	ourCol = ((gbcolor & 0x001f) << 11) + ((gbcolor & 0x03e0) << 1) + ((gbcolor & 0x7c00) >> 10);
	dstPal[0][idx] = ourCol;	//a sleight of hand
}

static void gbPrvRecalcDmgPal(CachedDmgPal dst, uint_fast8_t regVal, uint_fast8_t plane)
{
	const uint16_t *colors = mActiveDmgPaletteColors[plane];
	
	dst[0] = colors[(regVal >> 0) & 3];
	dst[1] = colors[(regVal >> 2) & 3];
	dst[2] = colors[(regVal >> 4) & 3];
	dst[3] = colors[(regVal >> 6) & 3];
}

bool gbIoWrite(uint16_t addr, uint8_t val)
{
	uint8_t *hram = mHRAM;
	uint8_t i = 0, t;
	
	addr &= 0xff;
	
	//pr("IO WR 0xFF%02X <- %02X\n", addr, val);
	
	if(addr == 0x40){
		//fprintf(stderr, "LCD cfg set %02X from 0x%02X while row=%d (mode %d)\n", val, hram[0x40], hram[0x44], gbIoRead(hram, 0x41) & 3);
		
		if (!(val & 0x80)) {
			
			//fprintf(stderr, "off\n");
			//val |= 0x80;
		}
	}
	
	switch(addr){
		
		case 0x47:	//BG color clut
			gbPrvRecalcDmgPal(dmgBgPal, val, GbDmgPaletteBg);
			goto write_complete;
		
		case 0x48:	//OBJ[0] color clut
			gbPrvRecalcDmgPal(dmgFgPals[0], val, GbDmgPaletteObj0);
			goto write_complete;
		
		case 0x49:	//OBJ[1] color clut
			gbPrvRecalcDmgPal(dmgFgPals[1], val, GbDmgPaletteObj1);
			goto write_complete;
		
		case 0x02:	//SERIAL control reg
			if(val & 0x80) accessFail(addr, 1, val);
			else goto write_complete;
			break;
		
		case 0x04:	//DIV
			divOfst = gbGetCy();
			break;
		
		case 0x07:	//timer config
			val &= 7;	//asm code assumes no weird bits are set
			if (!(hram[0x07] & 0x4) && (val & 0x04))
				gbTmrCySetToCy();
			goto write_complete;
		
		case 0x46:	//DMA
			for (i = 0; i < 0xA0; i++)
				gbExtWrite(0xFE00 | i, gbExtRead((((uint16_t)val) << 8) | i));
			break;
		
		case 0x40:	//LCD config
			if ((hram[0x40] & 0x80) && !(val & 0x80)) {	//turning off
				
				hram[0x41] &= (uint8_t)~0x07;	//LY != LYC
				hram[0x44] = 0;
			}
			else if (!(hram[0x40] & 0x80) && (val & 0x80)) {	//turning on
				
				hram[0x41] = (hram[0x41] & (uint8_t)~0x03) | 2;	//reset state to mode 2
				hram[0x44] = 0;
				gbLcdCySetToCy();
			}
			
			goto write_complete;
		
		case 0x41:
			
			val = (val & 0xF8) | (hram[0x41] & 7);
			goto write_complete;
		
		case 0x4F:
			val |= 0xfe;
			gbExtSetVramPage(val & 1);
			goto write_complete;
		
		case 0x70:
			val &= 7;
			gbExtSetWramPage(val);
			goto write_complete;
		
		case 0x55:	//DMA start
			{
				uint16_t d = (((uint16_t)hram[0x53]) << 8) | hram[0x54];
				uint16_t s = (((uint16_t)hram[0x51]) << 8) | hram[0x52];
			
				d = (d & 0x1FF0) | 0x8000;
				s = s & 0xFFF0;
			
				if (val & 0x80){
					
				//	fprintf(stderr,"HBLANK DMA %x0 %02X%02X->%02X%02X\n", (val & 0x7F) + 1, hram[0x51], hram[0x52], 0x80|hram[0x53], hram[0x54]);
					
					
					val &= 0x7F;	//record that we need to do the DMA
					if (hram[0x40] & 0x80)	//hblank dma requested while screen is on - normal op, while off - treat it liek immediate. not sure if right...
						goto write_complete;
				}
				
				if (hram[0x55] != 0xFF) {	//cancel ongoing HBLANK DMA
					
				//	fprintf(stderr,"HBLANK DMA CANCEL\n");
					val = 0x80 + (hram[0x55] & 0x7f);
					goto write_complete;
				}
				
				//do the DMA instantly
				
				//	fprintf(stderr,"NEW DMA %x0 %02X%02X->%02X%02X\n", (val & 0x7F) + 1, hram[0x51], hram[0x52], 0x80|hram[0x53], hram[0x54]);
				
				do{
					
					i = 0x10;
					do{
					
						gbExtWrite(d++,gbExtRead(s++));
					
					}while(--i);
				}while(val--);
				//val will end up 0xFF as needed automatically due to the above subtract
				goto write_complete;
			}
		
		case 0x56:	//IR (we never recv anything)
			val = ((val >> 6) == 3 ? 0x02 : 0x00) | (val & 0xc1);
			goto write_complete;
		
		case 0x69:		//BG pal write
			t = hram[0x68];				//t = hramPtrIdx
			i = t & 63;					//desired index
			hram[HRAM_INDEX_BG_PAL + i] = val;
			gbPrvRecalcCgbPal(cgbBgPals, hram + HRAM_INDEX_BG_PAL, i);
			if (t & 0x80)
				hram[0x68] = (t + 1) & 0xBF;
			break;
			
		case 0x6B:		//OBJ pal write
			t = hram[0x6A];				//t = hramPtrIdx
			i = t & 63;					//desired index
			hram[HRAM_INDEX_OB_PAL + i] = val;
			gbPrvRecalcCgbPal(cgbFgPals, hram + HRAM_INDEX_OB_PAL, i);
			if (t & 0x80)
				hram[0x6A] = (t + 1) & 0xBF;
			break;
		
		case 0x4d:
			val &= 1;
			goto write_complete;
		
		case 0x27 ... 0x2F:	//nonexistent audio regs accept writes
			val = 0xff;
			goto write_complete;

		case 0x12:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if (!(val & 0xf8))			//if dac was turned off, the channel is now off
				hram[0x26] &=~ 0x01;
			goto write_complete;

		case 0x17:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if (!(val & 0xf8))			//if dac was turned off, the channel is now off
				hram[0x26] &=~ 0x02;
			goto write_complete;

		case 0x21:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if (!(val & 0xf8))			//if dac was turned off, the channel is now off
				hram[0x26] &=~ 0x08;
			goto write_complete;

		case 0x1a:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if (!(val & 0x80))			//if dac was turned off, the channel is now off
				hram[0x26] &=~ 0x04;
			goto write_complete;

		case 0x14:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if ((val & 0x80) && !(hram[0x26] & 0x01) && (hram[0x12] & 0xf8)) {	//trigger a channel that was off and whose dac allows it to come on
				hram[0x26] |= 0x01;	//it is on
				//pr("ch 1 on. NR52=0x%02x\n", hram[0x26]);
			}
			goto write_complete;

		case 0x19:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if ((val & 0x80) && !(hram[0x26] & 0x02) && (hram[0x17] & 0xf8)) {	//trigger a channel that was off and whose dac allows it to come on
				hram[0x26] |= 0x02;	//it is on
				//pr("ch 2 on. NR52=0x%02x\n", hram[0x26]);
			}
			goto write_complete;

		case 0x1e:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if ((val & 0x80) && !(hram[0x26] & 0x04) && (hram[0x1a] & 0x80))	//trigger a channel that was off and whose dac allows it to come on
				hram[0x26] |= 0x04;	//it is on
			goto write_complete;

		case 0x23:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			if ((val & 0x80) && !(hram[0x26] & 0x08) && (hram[0x21] & 0xf8))	//trigger a channel that was off and whose dac allows it to come on
				hram[0x26] |= 0x08;	//it is on
			goto write_complete;

		case 0x10:
		case 0x11:						//audio regs only take writes when audio is on
		case 0x13:
		case 0x15:
		case 0x16:
		case 0x18:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x22:
		case 0x24:
		case 0x25:
			if (!(hram[0x26] & 0x80))	//ignore writes while of
				break;
			goto write_complete;

		case 0x26:
			if (val & 0x80) {
				hram[0x26] |= 0x80;	//turning on
				gbAudioCySetToCy();
			}
			else if (hram[0x26] & 0x80) {	//turning off
				for (i = 0x10; i <= 0x26; i++)
					hram[i] = 0x00;
			}
			break;
		
		case 0x75:
			val &= 0x70;
			goto write_complete;

		case 0x03:
		case 0x08 ... 0x0e:
		case 0x4c:
		case 0x4e:
		case 0x57 ... 0x67:
		case 0x6d ... 0x6f:
		case 0x77 ... 0x7f:
	#ifdef GB_HAVE_EXTRA_IO_REGS
			if (gbExtraIoRegWrite(addr, val))
				break;
	#endif
			accessFail(addr, 1, val);
			return false;

		default:
	write_complete:
			hram[addr] = val;
			break;
	}
	return true;
}

struct state {		//only used for reporting and onlty relevant to the ASM core
	union{
		uint16_t hl;	//0x04
		struct{
			uint8_t l;	//0x04
			uint8_t h;	//0x05
		};
	};
	union{
		uint16_t bc;	//0x00
		struct{
			uint8_t c;	//0x00
			uint8_t b;	//0x01
		};
	};
	union{
		uint16_t de;	//0x02
		struct{
			uint8_t e;	//0x02
			uint8_t d;	//0x03
		};
	};
	
	uint8_t intWait;	//0x06	must be 0xFF or 0x00
	uint8_t intsOn;		//0x07	must be 0xFF or 0x00
	uint8_t intsOnNext;	//0x08	must be 0xFF or 0x00
	uint8_t zVal;		//0x09	VALUE based on WHICH the Z reg would be set, if we had one
	uint8_t fH;			//0x0a	actual "H" flag	MUST be 0 or 1 only
	uint8_t fN;			//0x0b	actual "N" flag	MUST be 0 or 1 only
	uint8_t fC;			//0x0c	actual "C" flag MUST be 0 or 1 only
	uint8_t actAsCgb;	//0x0d
	uint8_t dblSpeed;	//0x0e	only may contain 0 (slow) or 32 (fast)
	uint8_t dblSpeed2;	//0x0f	only may contain 11 (slow) or 12 (fast)
	uint8_t sndStep;	//0x10	only may contain evens 0..14

	uint32_t tmrCy;		//0x14
	uint32_t lcdCy;		//0x18
	uint32_t sndCy;		//0x1c
};


#define OFST_ACT_CGB		0x0d
#define OFST_DBL_SPEED		0x0e		//must only contain 0 or 32
#define OFST_SND_STEP		0x0f		//stores step * 2 (only evens)

#define OFST_TMR_CY			0x10
#define OFST_LCD_CY			0x14
#define OFST_SND_CY			0x18
#define SIZEOF_STATE		0x1c



void report(struct state* state, uint32_t a, uint32_t pc, uint32_t sp, uint32_t cy, uint32_t minusLcdCy, uint32_t minusTmrCy, uint32_t minusSndCy)
{
	(void)minusLcdCy;
	(void)minusTmrCy;
	(void)minusSndCy;
		
	//if(doLog)
	{
		pr("[%04x] A=%02x BC=%04x DE=%04x HL=%04x SP=%04x F=%02x @ %lu (lcd %lu, tmr %lu, snd %lu)\n",
			pc, a, state->bc, state->de, state->hl, sp, 
				(state->zVal ? 0x00 : 0x80) + 
				(state->fN ? 0x40 : 0x00) + 
				(state->fH ? 0x20 : 0x00) +
				(state->fC ? 0x10 : 0x00), cy, -minusLcdCy, -minusTmrCy, -minusSndCy
		);
	}
	
	if (a >> 8)
		pr("invalid A\n");
	else if (sp >> 16)
		pr("invalid SP\n");
	else if (pc >> 16)
		pr("invalid PC\n");
	else if (state->fC > 1)
		pr("invalid fC\n");
	else if (state->fN > 1)
		pr("invalid fN\n");
	else if (state->fH > 1)
		pr("invalid fH\n");
	else if (state->intWait != 0x00 && state->intWait != 0xff)
		pr("invalid intWait\n");
	else if (state->intsOn != 0x00 && state->intsOn != 0xff)
		pr("invalid intsOn\n");
	else if (state->intsOnNext != 0x00 && state->intsOnNext != 0xff)
		pr("invalid intsOnNext\n");
	else
		return;
	
	while(1);
}
