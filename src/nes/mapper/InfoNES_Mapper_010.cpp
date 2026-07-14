/*===================================================================*/
/*                                                                   */
/*                     Mapper 10 (MMC4)                              */
/*                                                                   */
/*===================================================================*/

struct Map10_Latch
{
	BYTE lo_bank;
	BYTE hi_bank;
	BYTE state;
};

static struct Map10_Latch mMap10Latch3;
static struct Map10_Latch mMap10Latch4;

static void Map10_SetChrBanks(uint_fast8_t first, BYTE bank)
{
	uint_fast8_t nPage;

	for (nPage = 0; nPage < 4; ++nPage)
		PPUBANK[first + nPage] = VROMPAGE(bank + nPage);
	InfoNES_SetupChr();
}

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 10                                             */
/*-------------------------------------------------------------------*/
void Map10_Init()
{
	int nPage;

	MapperInit = Map10_Init;
	MapperWrite = Map10_Write;
	MapperSram = Map0_Sram;
	MapperApu = Map0_Apu;
	MapperReadApu = Map0_ReadApu;
	MapperVSync = Map0_VSync;
	MapperHSync = Map0_HSync;
	MapperPPU = Map10_PPU;
	MapperRenderScreen = Map0_RenderScreen;

	SRAMBANK = SRAM;
	ROMBANK0 = ROMPAGE(0);
	ROMBANK1 = ROMPAGE(1);
	ROMBANK2 = ROMLASTPAGE(1);
	ROMBANK3 = ROMLASTPAGE(0);

	if (NesHeader.byVRomSize > 0) {
		for (nPage = 0; nPage < 8; ++nPage)
			PPUBANK[nPage] = VROMPAGE(nPage);
		InfoNES_SetupChr();
	}

	mMap10Latch3.state = 0xfe;
	mMap10Latch3.lo_bank = 0;
	mMap10Latch3.hi_bank = 0;
	mMap10Latch4.state = 0xfe;
	mMap10Latch4.lo_bank = 0;
	mMap10Latch4.hi_bank = 0;

	K6502_Set_Int_Wiring(1, 1);
}

/*-------------------------------------------------------------------*/
/*  Mapper 10 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map10_Write(WORD wAddr, BYTE byData)
{
	BYTE bank;

	switch (wAddr & 0xf000) {
		case 0xa000:
			bank = (BYTE)(byData % NesHeader.byRomSize) << 1;
			ROMBANK0 = ROMPAGE(bank);
			ROMBANK1 = ROMPAGE(bank + 1);
			break;

		case 0xb000:
			bank = (BYTE)(byData % (NesHeader.byVRomSize << 1)) << 2;
			mMap10Latch3.lo_bank = bank;
			if (mMap10Latch3.state == 0xfd)
				Map10_SetChrBanks(0, bank);
			break;

		case 0xc000:
			bank = (BYTE)(byData % (NesHeader.byVRomSize << 1)) << 2;
			mMap10Latch3.hi_bank = bank;
			if (mMap10Latch3.state == 0xfe)
				Map10_SetChrBanks(0, bank);
			break;

		case 0xd000:
			bank = (BYTE)(byData % (NesHeader.byVRomSize << 1)) << 2;
			mMap10Latch4.lo_bank = bank;
			if (mMap10Latch4.state == 0xfd)
				Map10_SetChrBanks(4, bank);
			break;

		case 0xe000:
			bank = (BYTE)(byData % (NesHeader.byVRomSize << 1)) << 2;
			mMap10Latch4.hi_bank = bank;
			if (mMap10Latch4.state == 0xfe)
				Map10_SetChrBanks(4, bank);
			break;

		case 0xf000:
			InfoNES_Mirroring((byData & 0x01) ? 0 : 1);
			break;
	}
}

/*-------------------------------------------------------------------*/
/*  Mapper 10 PPU Function                                           */
/*-------------------------------------------------------------------*/
void Map10_PPU(WORD wAddr)
{
	switch (wAddr & 0x3ff0) {
		case 0x0fd0:
			mMap10Latch3.state = 0xfd;
			Map10_SetChrBanks(0, mMap10Latch3.lo_bank);
			break;

		case 0x0fe0:
			mMap10Latch3.state = 0xfe;
			Map10_SetChrBanks(0, mMap10Latch3.hi_bank);
			break;

		case 0x1fd0:
			mMap10Latch4.state = 0xfd;
			Map10_SetChrBanks(4, mMap10Latch4.lo_bank);
			break;

		case 0x1fe0:
			mMap10Latch4.state = 0xfe;
			Map10_SetChrBanks(4, mMap10Latch4.hi_bank);
			break;
	}
}
