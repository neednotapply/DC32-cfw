#include "gbCartHeader.h"
#include <string.h>
#include "printf.h"
#include "gbCore.h"
#include "mbc.h"
#include "gb.h"

#pragma GCC optimize ("Ofast")

#define WRAM_SZ						32768	//8K on DMG
#define VRAM_SZ						16384	//8K on DMG

//mem map
#define GB_MMAP_ROM_BANK0_ADDR		0x0000
#define GB_MMAP_ROM_BANK0_SIZE		0x4000
#define GB_MMAP_ROM_BANKx_ADDR		0x4000
#define GB_MMAP_ROM_BANKx_SIZE		0x4000
#define GB_MMAP_VRAM_BANKx_ADDR		0x8000
#define GB_MMAP_VRAM_BANKx_SIZE		0x2000
#define GB_MMAP_CARTRAM_BANKx_ADDR	0xa000
#define GB_MMAP_CARTRAM_BANKx_SIZE	0x2000
#define GB_MMAP_WRAM_BANK0_ADDR		0xc000
#define GB_MMAP_WRAM_BANK0_SIZE		0x1000
#define GB_MMAP_WRAM_BANKx_ADDR		0xd000
#define GB_MMAP_WRAM_BANKx_SIZE		0x1000
#define GB_MMAP_OAM_ADDR			0xfe00
#define GB_MMAP_OAM_SIZE			0x0100	//we make it bigger to make life easier
#define GB_MMAP_IOREGS_ADDR			0xff00
#define GB_MMAP_IOREGS_SIZE			0x0080
#define GB_MMAP_HRAM_ADDR			0xff80
#define GB_MMAP_HRAM_SIZE			0x0080



#define MOVABLE_ROM_OFST			0x4000
#define MOVABLE_VRAM_OFST			0x8000
#define MOVABLE_WRAM_OFST			0xD000



uint8_t *mReadPtrs[0x10000 / MEMMAP_MAP_CHUNK_SZ];
MbcReadF mReadFuncs[0x10000 / MEMMAP_MAP_CHUNK_SZ];
uint8_t *mWritePtrs[0x10000 / MEMMAP_MAP_CHUNK_SZ];
MbcWriteF mWriteFuncs[0x10000 / MEMMAP_MAP_CHUNK_SZ];

uint8_t mVRAM[VRAM_SZ];
uint8_t __attribute__((aligned(4))) mHRAM[HRAM_SIZE];
uint8_t __attribute__((aligned(4))) mOAM[256]; // really [160], but making it 256 makes life easier
static uint8_t __attribute__((aligned(4))) mWRAM[WRAM_SZ];

static uint8_t *mRam;
static const uint8_t *mRom;
static uint16_t mRamBank, mRomBank, mRomBankMask;
static uint8_t mMbcPrivate[9];
static uint32_t mMbcPrivate2;

void* mbcPrvGetWramBuf(void)
{
	return mWRAM;
}

static void mbcPrvFillPtrs(uint8_t **arr, uint16_t base, uint16_t size, uint8_t *ptr)		//assumes perfectly valid input
{
	arr += base / MEMMAP_MAP_CHUNK_SZ;
	
	while (size) {
		
		if (ptr)
			*arr++ = ptr - base;
		else
			*arr++ = NULL;
		size -= MEMMAP_MAP_CHUNK_SZ;
	}
}

static void mbcPrvFillBothPtrs(uint16_t base, uint16_t size, uint8_t *ptr)		//assumes perfectly valid input
{
	mbcPrvFillPtrs(mReadPtrs, base, size, ptr);
	mbcPrvFillPtrs(mWritePtrs, base, size, ptr);
}

static void mbcPrvFillWriteFunc(uint16_t base, uint16_t size, MbcWriteF wf)		//assumes perfectly valid input
{
	MbcWriteF *af = mWriteFuncs;
	uint8_t **ap = mWritePtrs;
	
	ap += base / MEMMAP_MAP_CHUNK_SZ;
	af += base / MEMMAP_MAP_CHUNK_SZ;
	
	while (size) {
		
		*ap++ = NULL;
		*af++ = wf;
		
		size -= MEMMAP_MAP_CHUNK_SZ;
	}
}

static void mbcPrvFillReadFunc(uint16_t base, uint16_t size, MbcReadF rf)		//assumes perfectly valid input
{
	MbcReadF *af = mReadFuncs;
	uint8_t **ap = mReadPtrs;
	
	ap += base / MEMMAP_MAP_CHUNK_SZ;
	af += base / MEMMAP_MAP_CHUNK_SZ;
	
	while (size) {
		
		*ap++ = NULL;
		*af++ = rf;
		
		size -= MEMMAP_MAP_CHUNK_SZ;
	}
}

void gbExtSetVramPage(uint8_t pg)
{
	mbcPrvFillBothPtrs(GB_MMAP_VRAM_BANKx_ADDR, GB_MMAP_VRAM_BANKx_SIZE, mVRAM + GB_MMAP_VRAM_BANKx_SIZE * pg);
}

void gbExtSetWramPage(uint8_t pg)
{
	if (!pg)
		pg = 1;
	
	mbcPrvFillBothPtrs(GB_MMAP_WRAM_BANKx_ADDR, GB_MMAP_WRAM_BANKx_SIZE, mWRAM + GB_MMAP_WRAM_BANKx_SIZE * pg);
}

static bool mbcFillDefaults(const void *romP, void *ramP)
{
	mRam = ramP;
	mRom = romP;
	
	mRomBank = 1;
	mRamBank = 0;
	
	mbcPrvFillPtrs(mReadPtrs, GB_MMAP_ROM_BANK0_ADDR, GB_MMAP_ROM_BANK0_SIZE, (uint8_t*)mRom);
	mbcPrvFillPtrs(mReadPtrs, GB_MMAP_ROM_BANKx_ADDR, GB_MMAP_ROM_BANKx_SIZE, (uint8_t*)mRom + GB_MMAP_ROM_BANK0_SIZE);
	mbcPrvFillBothPtrs(GB_MMAP_WRAM_BANK0_ADDR, GB_MMAP_WRAM_BANK0_SIZE, mWRAM);
	mbcPrvFillBothPtrs(GB_MMAP_OAM_ADDR, GB_MMAP_OAM_SIZE, mOAM);
	mbcPrvFillBothPtrs(GB_MMAP_OAM_ADDR, GB_MMAP_OAM_SIZE, mOAM);
	mbcPrvFillBothPtrs(GB_MMAP_HRAM_ADDR, GB_MMAP_HRAM_SIZE, mHRAM + 0x80);
	mbcPrvFillWriteFunc(GB_MMAP_IOREGS_ADDR, GB_MMAP_IOREGS_SIZE, gbIoWrite);
	mbcPrvFillReadFunc(GB_MMAP_IOREGS_ADDR, GB_MMAP_IOREGS_SIZE, gbIoRead);
	
	gbExtSetVramPage(0);
	gbExtSetWramPage(1);
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////// MBC  NONE //////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool mbcInitNone(const void *romP, void *ramP, uint_fast8_t features)
{
	if (features & CART_WITH_RAM)
		mbcPrvFillBothPtrs(GB_MMAP_CARTRAM_BANKx_ADDR, GB_MMAP_CARTRAM_BANKx_SIZE, ramP);
	
	//mMbcPrivate unused
	return mbcFillDefaults(romP, ramP);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// MBC 1 ////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//romBank and rambank are used as temp storage and do not represent complete bank numbers

#define MBC1_DATA_RAM_CS		0
#define MBC1_DATA_MODE			1

static void mbc1prvMapThings(void)
{
	uint8_t romBank0 = (mMbcPrivate[MBC1_DATA_MODE] ? 0x20 * mRamBank : 0) & mRomBankMask;
	uint8_t ramBank =  mMbcPrivate[MBC1_DATA_MODE] ? mRamBank : 0;
	uint8_t romBank1 = (mRomBank + 0x20 * mRamBank) & mRomBankMask;

	mbcPrvFillPtrs(mReadPtrs, GB_MMAP_ROM_BANK0_ADDR, GB_MMAP_ROM_BANK0_SIZE, (uint8_t*)mRom + GB_MMAP_ROM_BANKx_SIZE * romBank0);
	mbcPrvFillPtrs(mReadPtrs, GB_MMAP_ROM_BANKx_ADDR, GB_MMAP_ROM_BANKx_SIZE, (uint8_t*)mRom + GB_MMAP_ROM_BANKx_SIZE * mRomBank);
	mbcPrvFillBothPtrs(GB_MMAP_CARTRAM_BANKx_ADDR, GB_MMAP_CARTRAM_BANKx_SIZE, mMbcPrivate[MBC1_DATA_RAM_CS] ? mRam + GB_MMAP_CARTRAM_BANKx_SIZE * romBank1  :NULL);
}

static bool mbc1prvRamCs(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	mMbcPrivate[MBC1_DATA_RAM_CS] = ((data & 0x0f) == 0x0a);
	mbc1prvMapThings();
	
	return true;
}

static bool mbc1prvRomBankLo(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	data &= 0x1f;
	if (!data)
		data = 1;
	
	mRomBank = data;
	mbc1prvMapThings();
	
	return true;
}

static bool mbc1prvRomBankHiOrRamBank(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	mRamBank = data & 3;
	mbc1prvMapThings();

	return true;
}

static bool mbc1prvModeSet(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	mMbcPrivate[MBC1_DATA_MODE] = data & 1;
	mbc1prvMapThings();

	return true;
}

static bool mbcInitMBC1(const void *romP, void *ramP, uint_fast8_t features)
{
	(void)features;
	
	//ram is off by default
	mbcPrvFillWriteFunc(0x0000, 0x2000, mbc1prvRamCs);
	mbcPrvFillWriteFunc(0x2000, 0x2000, mbc1prvRomBankLo);
	mbcPrvFillWriteFunc(0x4000, 0x2000, mbc1prvRomBankHiOrRamBank);
	mbcPrvFillWriteFunc(0x6000, 0x2000, mbc1prvModeSet);
	
	mMbcPrivate[MBC1_DATA_MODE] = 0;
	mbc1prvMapThings();

	return mbcFillDefaults(romP, ramP);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// MBC 2 ////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MBC2_DATA_RAM_CS		0

#define MBC2_RAM_SZ		512	//ram is x4 also... so top nibble shouldn't matter


static uint8_t mbc2prvRamRead(uint16_t fullAddr)
{

	fullAddr %= MBC2_RAM_SZ;
	
	return mRam[fullAddr / 2] >> ((fullAddr % 2) * 4);
}

static bool mbc2prvRamWrite(uint16_t fullAddr, uint8_t val)
{
	uint8_t *dst;
	

	fullAddr %= MBC2_RAM_SZ;
	
	dst = mRam + (fullAddr / 2);
	if (fullAddr % 2)
		*dst = (*dst & 0x0f) + (val << 4);
	else
		*dst = (*dst & 0xf0) + (val & 0x0f);
	
	return true;
}

static void mbc2prvMapRamAsNeeded(void)
{
	mbcPrvFillReadFunc(GB_MMAP_CARTRAM_BANKx_ADDR, MBC2_RAM_SZ, mMbcPrivate[MBC2_DATA_RAM_CS] ? mbc2prvRamRead : NULL);
	mbcPrvFillWriteFunc(GB_MMAP_CARTRAM_BANKx_ADDR, MBC2_RAM_SZ, mMbcPrivate[MBC2_DATA_RAM_CS] ? mbc2prvRamWrite : NULL);
}

static bool mbc2prvRamCs(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	mMbcPrivate[MBC2_DATA_RAM_CS] = ((data & 0x0f) == 0x0a);
	mbc2prvMapRamAsNeeded();
	
	return true;
}

static bool mbc2prvRomBank(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
		
	mRomBank = data & mRomBankMask;
	if (!mRomBank)
		mRomBank = 1;
	mbcPrvFillPtrs(mReadPtrs, GB_MMAP_ROM_BANKx_ADDR, GB_MMAP_ROM_BANKx_SIZE, (uint8_t*)mRom + GB_MMAP_ROM_BANKx_SIZE * mRomBank);
	
	return true;
}

bool mbcInitMBC2(const void *romP, void *ramP, uint_fast8_t features)
{
	uint_fast16_t i;

	(void)features;
	
	//ram is off by default
	for (i = 0x0000; i <= 0x1e00; i += 0x200)
		mbcPrvFillWriteFunc(i, 0x0100, mbc2prvRamCs);
	for (i = 0x2100; i <= 0x3f00; i += 0x200)
		mbcPrvFillWriteFunc(i, 0x0100, mbc2prvRomBank);
	
	return mbcFillDefaults(romP, ramP);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// MBC 3 ////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MBC3_DATA_RAM_CS			0
#define MBC3_DATA_LATCH_VAL			1
#define MBC3_DATA_SECS				2
#define MBC3_DATA_MINS				3
#define MBC3_DATA_HOURS				4
#define MBC3_DATA_DAYS_LO			5
#define MBC3_DATA_DAYS_HI			6	//low bit is day counter, bit 7 is carry (sticky), bit 6 is HALT
#define MBC3_FEATURES				7

static uint8_t mbc3prvRtcRead(uint16_t fullAddr)
{
	(void)fullAddr;
	
	switch (mRamBank) {
		case 0x08:	return mMbcPrivate[MBC3_DATA_SECS];
		case 0x09:	return mMbcPrivate[MBC3_DATA_MINS];
		case 0x0a:	return mMbcPrivate[MBC3_DATA_HOURS];
		case 0x0b:	return mMbcPrivate[MBC3_DATA_DAYS_LO];
		case 0x0c:	return mMbcPrivate[MBC3_DATA_DAYS_HI];
		default:	return 0;
	}
}

static bool mbc3prvRtcWrite(uint16_t fullAddr, uint8_t val)
{
	uint32_t newTime;

	(void)fullAddr;
	
	switch (mRamBank) {
		case 0x08:
			mMbcPrivate[MBC3_DATA_SECS] = val & 0x3f;
			break;
		
		case 0x09:
			mMbcPrivate[MBC3_DATA_MINS] = val & 0x3f;
			break;
		
		case 0x0a:
			mMbcPrivate[MBC3_DATA_HOURS] = val & 0x1f;
			break;
		
		case 0x0b:
			mMbcPrivate[MBC3_DATA_DAYS_LO] = val;
			break;
		
		case 0x0c:
			mMbcPrivate[MBC3_DATA_DAYS_HI] = val & 0xc1;
			break;
		
		default:
			return false;
	}

	//"carry" becomes "512 days" which is reasonable
	newTime = mMbcPrivate[MBC3_DATA_SECS] + 60 * (mMbcPrivate[MBC3_DATA_MINS] + 60 * (mMbcPrivate[MBC3_DATA_HOURS] + 24 * (mMbcPrivate[MBC3_DATA_DAYS_LO] + 256 * ((mMbcPrivate[MBC3_DATA_DAYS_HI] & 0x01) + ((mMbcPrivate[MBC3_DATA_DAYS_HI] & 0x80) >> 6)))));
	gbExtRtcSet(newTime);
	
	return true;
}

static void mbc3prvMapRamAsNeeded(void)
{
	mbcPrvFillBothPtrs(GB_MMAP_CARTRAM_BANKx_ADDR, GB_MMAP_CARTRAM_BANKx_SIZE, NULL);
	mbcPrvFillReadFunc(GB_MMAP_CARTRAM_BANKx_ADDR, GB_MMAP_CARTRAM_BANKx_SIZE, NULL);
	mbcPrvFillWriteFunc(GB_MMAP_CARTRAM_BANKx_ADDR, GB_MMAP_CARTRAM_BANKx_SIZE, NULL);
	
	if (!mMbcPrivate[MBC3_DATA_RAM_CS])
		return;
	
	if (mRamBank < 4) {
		
		void *ramPtr = mRam + GB_MMAP_CARTRAM_BANKx_SIZE * mRamBank;

		mbcPrvFillBothPtrs(GB_MMAP_CARTRAM_BANKx_ADDR, GB_MMAP_CARTRAM_BANKx_SIZE, ramPtr);
	}
	
	if (mRamBank >= 0x08) {
		
		mbcPrvFillReadFunc(GB_MMAP_CARTRAM_BANKx_ADDR, GB_MMAP_CARTRAM_BANKx_SIZE, mbc3prvRtcRead);
		mbcPrvFillWriteFunc(GB_MMAP_CARTRAM_BANKx_ADDR, GB_MMAP_CARTRAM_BANKx_SIZE, mbc3prvRtcWrite);
	}
}

static bool mbc3prvRamCs(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	mMbcPrivate[MBC3_DATA_RAM_CS] = (data == 0x0a);
	mbc3prvMapRamAsNeeded();
	
	return true;
}

static bool mbc3prvRomBank(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	mRomBank = data & mRomBankMask;
	if (!mRomBank)
		mRomBank = 1;
	mbcPrvFillPtrs(mReadPtrs, GB_MMAP_ROM_BANKx_ADDR, GB_MMAP_ROM_BANKx_SIZE, (uint8_t*)mRom + GB_MMAP_ROM_BANKx_SIZE * mRomBank);
	
	return true;
}

static bool mbc3prvRamAndRtcBank(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	if ((data < 0x04 && (mMbcPrivate[MBC3_FEATURES] & CART_WITH_RAM)) || (data >= 0x08 && data <= 0x0c && (mMbcPrivate[MBC3_FEATURES] & CART_WITH_RTC))) {
		
		mRamBank = data;
		mbc3prvMapRamAsNeeded();
	}
	
	return true;
}

static bool mbc3prvLatch(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	if (data > 1)
		return false;
	
	if (!mMbcPrivate[MBC3_DATA_LATCH_VAL] && data) {
		
		uint32_t curTime = gbExtRtcGet();

		//we will only update our internal regs if HALT is clear and value has changed since we last updated them. This is to match MBC3 behaviour
		if (!(mMbcPrivate[MBC3_DATA_DAYS_HI] & 0x40) && curTime != mMbcPrivate2) {

			mMbcPrivate2 = curTime;

			mMbcPrivate[MBC3_DATA_SECS] = curTime % 60; curTime /= 60;
			mMbcPrivate[MBC3_DATA_MINS] = curTime % 60; curTime /= 60;
			mMbcPrivate[MBC3_DATA_HOURS] = curTime % 24; curTime /= 24;
			mMbcPrivate[MBC3_DATA_DAYS_LO] = curTime % 256; curTime /= 256;
			mMbcPrivate[MBC3_DATA_DAYS_HI] = (mMbcPrivate[MBC3_DATA_DAYS_HI] & 0x40) | (curTime & 1) | ((curTime >> 1) ? 0x80 : 0x00);		//halt flag preserved
		}
	}
	
	mMbcPrivate[MBC3_DATA_LATCH_VAL] = data;
	return true;
}

static bool mbcInitMBC3(const void *romP, void *ramP, uint_fast8_t features)
{
	//ram is off by default
	mbcPrvFillWriteFunc(0x0000, 0x2000, mbc3prvRamCs);
	mbcPrvFillWriteFunc(0x2000, 0x2000, mbc3prvRomBank);
	mbcPrvFillWriteFunc(0x4000, 0x2000, mbc3prvRamAndRtcBank);
	mbcPrvFillWriteFunc(0x6000, 0x2000, mbc3prvLatch);
	
	mMbcPrivate[MBC3_FEATURES] = features;

	mMbcPrivate2 = ~gbExtRtcGet();

	return mbcFillDefaults(romP, ramP);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// MBC 5 ////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MBC5_DATA_RAM_CS		0
#define MBC5_DATA_RAM_BANK_AND	1

static void mbc5prvMapRamAsNeeded(void)
{
	void *ramPtr = mMbcPrivate[MBC5_DATA_RAM_CS] ? mRam + GB_MMAP_CARTRAM_BANKx_SIZE * mRamBank : NULL;

	mbcPrvFillBothPtrs(GB_MMAP_CARTRAM_BANKx_ADDR, GB_MMAP_CARTRAM_BANKx_SIZE, ramPtr);
}

static bool mbc5prvRamCs(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	mMbcPrivate[MBC5_DATA_RAM_CS] = (data == 0x0a);
	mbc5prvMapRamAsNeeded();
	
	return true;
}

static bool mbc5prvRomBankLo(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	mRomBank = ((mRomBank & 0x100) + data) & mRomBankMask;
	mbcPrvFillPtrs(mReadPtrs, GB_MMAP_ROM_BANKx_ADDR, GB_MMAP_ROM_BANKx_SIZE, (uint8_t*)mRom + GB_MMAP_ROM_BANKx_SIZE * mRomBank);
	
	return true;
}

static bool mbc5prvRomBankHi(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	mRomBank = ((mRomBank & 0x0ff) + 0x100 * (data & 1)) & mRomBankMask;
	mbcPrvFillPtrs(mReadPtrs, GB_MMAP_ROM_BANKx_ADDR, GB_MMAP_ROM_BANKx_SIZE, (uint8_t*)mRom + GB_MMAP_ROM_BANKx_SIZE * mRomBank);
	
	return true;
}

static bool mbc5prvRamBank(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;

	if (data >= 0x10)
		return false;
	
	data &= mMbcPrivate[MBC5_DATA_RAM_BANK_AND];

	mRamBank = data;
	mbc5prvMapRamAsNeeded();
	
	return true;
}

static bool mbcInitMBC5(const void *romP, void *ramP, uint_fast8_t features)
{
	//if have rumble, half as much ram is present and bit 3 of bank select just enables or disables rumble motor
	mMbcPrivate[MBC5_DATA_RAM_BANK_AND] = (features & CART_WITH_RUMBLE) ? 0x07 : 0x0f;
	
	//ram is off by default
	mbcPrvFillWriteFunc(0x0000, 0x2000, mbc5prvRamCs);
	mbcPrvFillWriteFunc(0x2000, 0x1000, mbc5prvRomBankLo);
	mbcPrvFillWriteFunc(0x3000, 0x1000, mbc5prvRomBankHi);
	mbcPrvFillWriteFunc(0x4000, 0x2000, mbc5prvRamBank);
	
	return mbcFillDefaults(romP, ramP);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// MBC 7 ////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//MBC7 is a bitch because it bitbangs MICROWIRE and i am stuck emulating it...
//consider the commented-out prints a courtesy to ghelp you debug this if you dare touch it
//EEPROM is microwire, ORG=x16. whethere it is 256 or 512 bytes, either way it takes 8 bits of
// addr as per https://ww1.microchip.com/downloads/aemDocuments/documents/MPD/ProductDocuments/DataSheets/21712C.pdf
//Some games have 512 some have 256. Thereis nothing in the header to tell them apart.
//But all games with 256 have a 1MByte rom, so we use that to disambiguate

#define MBC7_DATA_UNLOCK_STATE			0		//{locked, lock1unlocked, unlocked}
#define MBC7_DATA_RAM_ADDR_BITS			1		//top bit indicates if weites are enabled
#define MBC7_DATA_X_LO					2
#define MBC7_DATA_X_HI					3
#define MBC7_DATA_Y_LO					4
#define MBC7_DATA_Y_HI					5
#define MBC7_DATA_MICROWIRE_REG			6		//the reg
#define MBC7_DATA_MICROWIRE_STATE		7		//low 4 bits are enum MicrowireState, top 4 are possibly command RXed or num bits of addr RXed
#define MBC7_DATA_MICROWIRE_CMD			8

#define MBC7_NUM_ADDR_BITS_SENT			8		//always

enum MicrowireCmd {
	MicrowireCmdWR,
	MicrowireCmdRD,
	MicrowireCmdERZ,
	MicrowireCmdEWDS,
	MicrowireCmdWRAL,
	MicrowireCmdERAL,
	MicrowireCmdEWEN,
};

enum MicrowireState {
	MicrowireBusy,
	MicrowireDeselected,
	MicrowireIdle,
	MicrowireCmdRx,
	MicrowireSubcmdRx,
	MicrowireAddrRx,
	MicrowireDummyCycle,
	MicrowireReadData,
	MicrowireDataRx,
	MicrowireCmdDone,
};

#define MW_BIT_CS						0x80
#define MW_BIT_CLK						0x40
#define MW_BIT_MOSI						0x02
#define MW_BIT_MISO						0x01

static uint8_t mbc7prvMicrowireSelected(uint8_t curMiso)
{
	uint8_t ret;

	if (mMbcPrivate2) {	//we were busy

		mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireBusy;
		ret = 0;
	}
	else {

		mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireIdle;
		ret = 1;		//ready
	}

	//pr("SELECT -> %d\n", ret);
	return ret;
}

static uint8_t mbc7prvMicrowireDeselected(uint8_t curMiso)
{
	//pr("DESELECT\n");
	
	//if we're busy, private 2 tracks how long for. if not busy, the answer is zero
	if (mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] != MicrowireBusy)
		mMbcPrivate2 = 0;

	mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireDeselected;

	return 0;	//released
}

static void mbc7prvMicrowireDataWrite(uint32_t addr, uint_fast16_t data)
{
	mRam[addr * 2 + 0] = data >> 8;
	mRam[addr * 2 + 1] = data;
}

static uint_fast16_t mbc7prvMicrowireDataRead(uint32_t addr)
{
	return 256 * mRam[addr * 2 + 0] + mRam[addr * 2 + 1];
}

static uint8_t mbc7prvMicrowireBit(uint8_t bit, uint8_t curMiso)
{
	uint_fast16_t i;

	switch (mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] & 0x0f) {
		
		case MicrowireBusy:
			if (!mMbcPrivate2) {
				mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireIdle;
				return 1;
			}
			else {
				mMbcPrivate2--;
				return 0;
			}
			break;

		case MicrowireDeselected:
			pr("unexpected bit while deselected: %d\n", bit);
			break;
		
		case MicrowireIdle:
			if (bit)
				mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = 0x10 + MicrowireCmdRx;	//when we shift the 0x10 out we know we got 4 bits
			break;

		case MicrowireCmdRx:
			mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = (mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] & 0x0f) + (mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] & 0x70) * 2 + 0x10 * bit;
			if (mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] & 0x40) {	//two bits RXed
				switch (((mMbcPrivate[MBC7_DATA_MICROWIRE_STATE]) >> 4) & 3) {
					case 0x00:	//a subcommand exists
						mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = 0x10 + MicrowireSubcmdRx;
						break;

					case 0x01:	//write
						mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireAddrRx;
						mMbcPrivate[MBC7_DATA_MICROWIRE_CMD] = MicrowireCmdWR;
						mMbcPrivate2 = 0;
						break;

					case 0x02:	//read
						mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireAddrRx;
						mMbcPrivate[MBC7_DATA_MICROWIRE_CMD] = MicrowireCmdRD;
						mMbcPrivate2 = 0;
						break;

					case 0x03:	//erase
						mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireAddrRx;
						mMbcPrivate[MBC7_DATA_MICROWIRE_CMD] = MicrowireCmdERZ;
						mMbcPrivate2 = 0;
						break;

					default:
						__builtin_unreachable();
						break;
				}
			}
			break;

		case MicrowireSubcmdRx:
			mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = (mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] & 0x0f) + (mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] & 0x70) * 2 + 0x10 * bit;
			if (mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] & 0x40) {	//two bits RXed
				switch (((mMbcPrivate[MBC7_DATA_MICROWIRE_STATE]) >> 4) & 3) {
					case 0x00:	//EWDS
						mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = 0x20 + MicrowireAddrRx;	//act like 2 bits of dummy addr already RXed
						mMbcPrivate[MBC7_DATA_MICROWIRE_CMD] = MicrowireCmdEWDS;
						mMbcPrivate2 = 0;
						break;

					case 0x01:	//WRAL
						mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = 0x20 + MicrowireAddrRx;	//act like 2 bits of dummy addr already RXed
						mMbcPrivate[MBC7_DATA_MICROWIRE_CMD] = MicrowireCmdWRAL;
						mMbcPrivate2 = 0;
						break;

					case 0x02:	//ERAL
						mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = 0x20 + MicrowireAddrRx;	//act like 2 bits of dummy addr already RXed
						mMbcPrivate[MBC7_DATA_MICROWIRE_CMD] = MicrowireCmdERAL;
						mMbcPrivate2 = 0;
						break;

					case 0x03:	//EWEN
						mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = 0x20 + MicrowireAddrRx;	//act like 2 bits of dummy addr already RXed
						mMbcPrivate[MBC7_DATA_MICROWIRE_CMD] = MicrowireCmdEWEN;
						mMbcPrivate2 = 0;
						break;

					default:
						__builtin_unreachable();
						break;
				}
			}
			break;
		
		case MicrowireAddrRx:
			mMbcPrivate2 = mMbcPrivate2 * 2 + bit;
			mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] += 0x10;
			if ((mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] >> 4) == MBC7_NUM_ADDR_BITS_SENT) {

				mMbcPrivate2 &= (1 << mMbcPrivate[MBC7_DATA_RAM_ADDR_BITS]) - 1;
				
				/*static const char *cmds[] = {
					[MicrowireCmdWR] = "WR",
					[MicrowireCmdRD] = "RD",
					[MicrowireCmdERZ] = "ERZ",
					[MicrowireCmdEWDS] = "EWDS",
					[MicrowireCmdWRAL] = "WRAL",
					[MicrowireCmdERAL] = "ERAL",
					[MicrowireCmdEWEN] = "EWEN",
				};
				pr(" $ CMD %s (ADDR = %u)\n", cmds[mMbcPrivate[MBC7_DATA_MICROWIRE_CMD]], mMbcPrivate2);
				*/

				switch (mMbcPrivate[MBC7_DATA_MICROWIRE_CMD]) {
					case MicrowireCmdEWDS:
						mMbcPrivate[MBC7_DATA_RAM_ADDR_BITS] &=~ 0x80;	//write disable;
						mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireCmdDone;
						break;

					case MicrowireCmdEWEN:
						mMbcPrivate[MBC7_DATA_RAM_ADDR_BITS] |= 0x80;	//write enable;
						mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireCmdDone;
						break;

					case MicrowireCmdERAL:
						if (mMbcPrivate[MBC7_DATA_RAM_ADDR_BITS] & 0x80) {
							
							for (i = 0; i < (mMbcPrivate[MBC7_DATA_RAM_ADDR_BITS] & 0x7f); i++)
								mbc7prvMicrowireDataWrite(i, 0xffff);

							mMbcPrivate2 = 20;	//busy for 20 checks
							mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireBusy;
						}
						else {
							
							mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireCmdDone;
						}
						break;

					case MicrowireCmdERZ:
						if (mMbcPrivate[MBC7_DATA_RAM_ADDR_BITS] & 0x80) {

							mbc7prvMicrowireDataWrite(mMbcPrivate2, 0xffff);
							mMbcPrivate2 = 2;	//busy for 2 checks
							mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireBusy;
						}
						else {
							
							mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireCmdDone;
						}
						break;

					case MicrowireCmdWR:
					case MicrowireCmdWRAL:
						mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireDataRx;
						break;

					case MicrowireCmdRD:
						mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireDummyCycle;
						break;

					default:
						__builtin_unreachable();
						break;
				}
			}
			break;
		
		case MicrowireDummyCycle:
			mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireReadData;
			break;

		case MicrowireReadData:
		//	if (!(mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] >> 4))
		//		pr(" $  data = 0x%04x\n", mbc7prvMicrowireDataRead(mMbcPrivate2));

			curMiso = (mbc7prvMicrowireDataRead(mMbcPrivate2) >> (15 - (mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] >> 4))) & 1;
			mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] += 0x10;
			if (!(mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] >> 4)) {	//word sent entirely
				
				//wrap around if we reach the end of the array
				if (++mMbcPrivate2 == (1u << (mMbcPrivate[MBC7_DATA_RAM_ADDR_BITS] & 0x7f)))
					mMbcPrivate2 = 0;
			}
			break;

		case MicrowireDataRx:	//we assume we never have addresses over 16 bits :)
			mMbcPrivate2 = mMbcPrivate2 * 2 + bit;
			mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] += 0x10;
			if (!(mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] >> 4)) {	//got the data word
				
		//		pr(" $  data = 0x%04x\n", (uint16_t)mMbcPrivate2);

				switch (mMbcPrivate[MBC7_DATA_MICROWIRE_CMD]) {

					case MicrowireCmdWR:
						if (mMbcPrivate[MBC7_DATA_RAM_ADDR_BITS] & 0x80) {

							mbc7prvMicrowireDataWrite(mMbcPrivate2 >> 16, (uint16_t)mMbcPrivate2);
							mMbcPrivate2 = 1;	//busy for 1 checks
							mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireBusy;
						}
						else {
							
							mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireCmdDone;
						}
						break;

					case MicrowireCmdWRAL:
						if (mMbcPrivate[MBC7_DATA_RAM_ADDR_BITS] & 0x80) {
						
							for (i = 0; i < (mMbcPrivate[MBC7_DATA_RAM_ADDR_BITS] & 0x7f); i++)
								mbc7prvMicrowireDataWrite(i,  (uint16_t)mMbcPrivate2);
							mMbcPrivate2 = 10;	//busy for 10 checks
							mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireBusy;
						}
						else {
							
							mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireCmdDone;
						}
						break;

					default:
						__builtin_unreachable();
						break;
				}
			}
			break;
	}

	//pr("BIT %u\n", bit);

	return curMiso;
}




/*
	//MOSI changes with clock falling edge
	//MISO is sampled after clock falling edge
	CS		_------------------------
	CLK		__-_-_-_-_-_-_-_-_-_-_-_-
	MOSI	___----__________________
	MISO	_________________________
*/

static uint8_t mbc7prvMicrowireTransact(uint_fast8_t prevVal, uint8_t writtenNewVal)
{
	uint8_t correctedNewVal = (writtenNewVal & 0xc2) | (prevVal & 0x01);
	uint8_t changed = prevVal ^ correctedNewVal;
	uint8_t rose = correctedNewVal & changed;
	uint8_t fell = prevVal & changed;
	uint8_t retVal = correctedNewVal;
	uint8_t retBit = !!(prevVal & MW_BIT_MISO);

	if (!changed)
		return correctedNewVal;

	if (changed & MW_BIT_CS) {	//we do not consider any changes at the same time as CS since nothing is defined to change at the same time as CS
		
		if (rose & MW_BIT_CS)
			retBit = mbc7prvMicrowireSelected(retBit);
		else
			retBit = mbc7prvMicrowireDeselected(retBit);
	}
	else if (fell & MW_BIT_CLK) {

		retBit = mbc7prvMicrowireBit(!!(correctedNewVal & MW_BIT_MOSI), retBit);
	}

	retVal = (retVal &~ MW_BIT_MISO) + (retBit ? MW_BIT_MISO : 0);

	//pr("mbc7prvMicrowireTransact(%02x, %02x) -> %02x\n", prevVal, writtenNewVal, retVal);

	return retVal;
}

static bool mbc7prvRamCs1(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	switch (mMbcPrivate[MBC7_DATA_UNLOCK_STATE]) {
		case 0:
		case 1:
			mMbcPrivate[MBC7_DATA_UNLOCK_STATE] = data == 0x0a ? 1 : 0;
			break;

		case 2:
			if (data != 0x0a)
				mMbcPrivate[MBC7_DATA_UNLOCK_STATE] = 0;
			break;
	}
	return true;
}

static bool mbc7prvRamCs2(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	switch (mMbcPrivate[MBC7_DATA_UNLOCK_STATE]) {
		case 0:
			break;

		case 1:
		case 2:
			mMbcPrivate[MBC7_DATA_UNLOCK_STATE] = data == 0x40 ? 2 : 1;
			break;
	}
	return true;
}

static bool mbc7prvRomBank(uint16_t fullAddr, uint8_t data)
{
	(void)fullAddr;
	
	mRomBank = data & mRomBankMask;
	mbcPrvFillPtrs(mReadPtrs, GB_MMAP_ROM_BANKx_ADDR, GB_MMAP_ROM_BANKx_SIZE, (uint8_t*)mRom + GB_MMAP_ROM_BANKx_SIZE * mRomBank);
	
	return true;
}

static uint8_t mbc7prvRamRegionRead(uint16_t fullAddr)
{
	uint8_t ret = 0xff;

	//unlocked?
	if (mMbcPrivate[MBC7_DATA_UNLOCK_STATE] == 2 && fullAddr < 0xb000) switch ((fullAddr & 0x00f0) >> 4) {

		case 0x02:
			ret = mMbcPrivate[MBC7_DATA_X_LO];
			break;
		
		case 0x03:
			ret = mMbcPrivate[MBC7_DATA_X_HI];
			break;
		
		case 0x04:
			ret = mMbcPrivate[MBC7_DATA_Y_LO];
			break;
		
		case 0x05:
			ret = mMbcPrivate[MBC7_DATA_Y_HI];
			break;
		
		case 0x06:
			ret = 0x00;
			break;

		case 0x08:
			if (mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] == MicrowireBusy) {	//waiting for busy can be done without clock

				if (mMbcPrivate2)
					mMbcPrivate2--;
				else {
					//pr(" BUSY OVER $$$$\n");
					mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireIdle;
					mMbcPrivate[MBC7_DATA_MICROWIRE_REG] |= MW_BIT_MISO;
				}
			}
			//pr("*** RD %02x\n", mMbcPrivate[MBC7_DATA_MICROWIRE_REG]);
			ret = mMbcPrivate[MBC7_DATA_MICROWIRE_REG];
			break;

		default:	//things that read 0xff
			break;
	}

	//pr("[%04x] -> %02x\n", fullAddr, ret);
	return ret;
}


static bool mbc7prvRamRegionWrite(uint16_t fullAddr, uint8_t data)
{
	//unlocked and in proper range?
	if (mMbcPrivate[MBC7_DATA_UNLOCK_STATE] == 2 && fullAddr < 0xb000) {
		uint16_t x, y;
	
		switch ((fullAddr & 0x00f0) >> 4) {

			case 0x00:				//latch clean
				mMbcPrivate[MBC7_DATA_X_LO] = 0x00;
				mMbcPrivate[MBC7_DATA_X_HI] = 0x80;
				mMbcPrivate[MBC7_DATA_Y_LO] = 0x00;
				mMbcPrivate[MBC7_DATA_Y_HI] = 0x80;
				break;

			case 0x01:				//latch values
				gbExtAccelRead(&x, &y);
				mMbcPrivate[MBC7_DATA_X_LO] = x;
				mMbcPrivate[MBC7_DATA_X_HI] = x >> 8;
				mMbcPrivate[MBC7_DATA_Y_LO] = y;
				mMbcPrivate[MBC7_DATA_Y_HI] = y >> 8;
				break;

			case 0x08:	//rememebe that bit 0 is readonly
				//pr("*** WR %02x\n", data);
				mMbcPrivate[MBC7_DATA_MICROWIRE_REG] = mbc7prvMicrowireTransact(mMbcPrivate[MBC7_DATA_MICROWIRE_REG], data);
				break;

			default:		//unused and readonly regs
				break;
		}
	}
	return true;
}

static bool mbcInitMBC7(const void *romP, void *ramP, uint_fast8_t ramAddrBits, uint_fast8_t features)
{
	//if have rumble, half as much ram is present and bit 3 of bank select just enables or disables rumble motor
	mMbcPrivate[MBC7_DATA_UNLOCK_STATE] = 0;
	mMbcPrivate[MBC7_DATA_RAM_ADDR_BITS] = ramAddrBits - 1;	//addresses are word addresses not byte addresses
	mMbcPrivate[MBC7_DATA_MICROWIRE_REG] = 0;	//microwire ret
	mMbcPrivate[MBC7_DATA_MICROWIRE_STATE] = MicrowireDeselected;

	//ram is off by default
	mbcPrvFillWriteFunc(0x0000, 0x2000, mbc7prvRamCs1);
	mbcPrvFillWriteFunc(0x2000, 0x2000, mbc7prvRomBank);
	mbcPrvFillWriteFunc(0x4000, 0x2000, mbc7prvRamCs2);
	mbcPrvFillReadFunc(GB_MMAP_CARTRAM_BANKx_ADDR, GB_MMAP_CARTRAM_BANKx_SIZE, mbc7prvRamRegionRead);
	mbcPrvFillWriteFunc(GB_MMAP_CARTRAM_BANKx_ADDR, GB_MMAP_CARTRAM_BANKx_SIZE, mbc7prvRamRegionWrite);

	return mbcFillDefaults(romP, ramP);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////// DISPATCH ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////



bool mbcRomAnalyze(const void *rom, uint32_t *romSzExpectedP, uint32_t *ramSzExpectedP, enum RomColorSupport *colorSupportP, char *romNameP)			//needs sizeof(struct CartHeader) bytes at least
{
	const struct CartHeader *hdr = (const struct CartHeader*)rom;
	static const uint8_t magic[] = {
		0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
		0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
		0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
	};
	const uint8_t *csumStart = hdr->titleDMG, *csumEnd = &hdr->hdrCsum;
	uint32_t romSz, ramSz;	//as per header
	uint8_t csum = 0;
	
	if (memcmp(magic, hdr->nintendoLogo, sizeof(hdr->nintendoLogo)))
		return false;
	
	while (csumStart != csumEnd)
		csum -= 1 + *csumStart++;
	
	if (csum != hdr->hdrCsum)
		return false;
	
	if (hdr->romSize <= 8)
		romSz = 32768 << hdr->romSize;
	else switch (hdr->romSize) {
		case 0x52:
			romSz = 0x120000;
			break;
		
		case 0x53:
			romSz = 0x140000;
			break;
		
		case 0x54:
			romSz = 0x180000;
			break;
		
		default:
			return false;
	}
	
	switch (hdr->ramSize) {
		case 0x00:
			ramSz = 0;
			break;
		
		case 0x01:
			ramSz = 2 << 10;
			break;
		
		case 0x02:
			ramSz = 8 << 10;
			break;
		
		case 0x03:
			ramSz = 32 << 10;
			break;
		
		case 0x04:
			ramSz = 128 << 10;
			break;
		
		case 0x05:
			ramSz = 64 << 10;
			break;
		
		default:
			return false;
	}

	switch (hdr->cartType) {		//special carts that have no ram marking in header (since it is not simple sram) but have special ram
		case 0x06:
			if (ramSz)
				return false;
			ramSz = 256;	//x4 bit wide...
			break;
		
		case 0x22:
			if (ramSz)
				return false;
			ramSz = (romSz == 0x100000) ? 256 : 512;
			break;
			
		default:
			break;
	}

	if (romSzExpectedP)
		*romSzExpectedP = romSz;
	if (ramSzExpectedP)
		*ramSzExpectedP = ramSz;
	
	if (colorSupportP) {
		
		if (!(hdr->cgbFlag & 0x80))
			*colorSupportP = RomNoColor;
		else if (!(hdr->cgbFlag & 0x40))
			*colorSupportP = RomColorEnhanced;
		else
			*colorSupportP = RomColorRequired;
	}
	
	if (romNameP) {
		
		memset(romNameP, 0, ROM_NAME_LEN);
		if (hdr->cgbFlag & 0x80)
			memcpy(romNameP, hdr->titleCGB, sizeof(hdr->titleCGB));
		else
			memcpy(romNameP, hdr->titleDMG, sizeof(hdr->titleDMG));
	}
	
	return true;
}

bool mbcInit(const void *rom, uint32_t *romSzExpectedP, void *ram, uint32_t *ramSzExpectedP)
{
	const struct CartHeader *hdr = (const struct CartHeader*)rom;
	uint32_t romSz, nBanks;
	uint_fast8_t ramSzShift;

	if (!mbcRomAnalyze(rom, &romSz, ramSzExpectedP, NULL, NULL))
		return false;

	if (romSzExpectedP)
		*romSzExpectedP = romSz;
	
	nBanks = romSz / GB_MMAP_ROM_BANKx_SIZE;
	mRomBankMask = (1 << (31 - __builtin_clz(nBanks))) - 1 ;	//round to power of 2 to make a mask

	memset(mReadPtrs, 0, sizeof(mReadPtrs));
	memset(mReadFuncs, 0, sizeof(mReadFuncs));
	memset(mWritePtrs, 0, sizeof(mWritePtrs));
	memset(mWriteFuncs, 0, sizeof(mWriteFuncs));
	memset(mVRAM, 0, sizeof(mVRAM));
	memset(mHRAM, 0, sizeof(mHRAM));
	memset(mOAM, 0, sizeof(mOAM));
	memset(mWRAM, 0, sizeof(mWRAM));

	//hasRam, hasBattery, hasRtc
	switch (hdr->cartType) {
		case 0x00:	return mbcInitNone(rom, ram, CART_BASIC);
		case 0x01:	return mbcInitMBC1(rom, ram, CART_BASIC);
		case 0x02:	return mbcInitMBC1(rom, ram, CART_WITH_RAM);
		case 0x03:	return mbcInitMBC1(rom, ram, CART_WITH_RAM | CART_WITH_BATTERY);
		case 0x05:	return mbcInitMBC2(rom, ram, CART_WITH_RAM);
		case 0x06:	return mbcInitMBC2(rom, ram, CART_WITH_RAM | CART_WITH_BATTERY);
		case 0x08:	return mbcInitNone(rom, ram, CART_WITH_RAM);
		case 0x09:	return mbcInitNone(rom, ram, CART_WITH_RAM | CART_WITH_BATTERY);
		case 0x0f:	return mbcInitMBC3(rom, ram, CART_WITH_BATTERY | CART_WITH_RTC);
		case 0x10:	return mbcInitMBC3(rom, ram, CART_WITH_RAM | CART_WITH_BATTERY | CART_WITH_RTC);
		case 0x11:	return mbcInitMBC3(rom, ram, CART_BASIC);
		case 0x12:	return mbcInitMBC3(rom, ram, CART_WITH_RAM);
		case 0x13:	return mbcInitMBC3(rom, ram, CART_WITH_RAM | CART_WITH_BATTERY);
		case 0x19:	return mbcInitMBC5(rom, ram, CART_BASIC);
		case 0x1a:	return mbcInitMBC5(rom, ram, CART_WITH_RAM);
		case 0x1b:	return mbcInitMBC5(rom, ram, CART_WITH_RAM | CART_WITH_BATTERY);
		case 0x1c:	return mbcInitMBC5(rom, ram, CART_WITH_RUMBLE);
		case 0x1d:	return mbcInitMBC5(rom, ram, CART_WITH_RAM | CART_WITH_RUMBLE);
		case 0x1e:	return mbcInitMBC5(rom, ram, CART_WITH_RAM | CART_WITH_BATTERY | CART_WITH_RUMBLE);
		case 0x22:	return mbcInitMBC7(rom, ram, (romSz == 0x100000) ? 8 : 9, CART_WITH_RAM | CART_WITH_BATTERY | CART_WITH_ACCELEROMETER);
		default:	return false;
	}
}
