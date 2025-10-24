#ifndef _MBC_H_
#define _MBC_H_

#define MEMMAP_MAP_CHUNK_SHIFT		7		//big arrays, but fast...
#define MEMMAP_MAP_CHUNK_SZ			(1 << MEMMAP_MAP_CHUNK_SHIFT)


#ifndef ASSEMBLY
	#include <stdbool.h>
	#include <stdint.h>
	
	
	typedef uint8_t (*MbcReadF)(uint16_t fullAddr);
	typedef bool (*MbcWriteF)(uint16_t fullAddr, uint8_t data);
	
	extern uint8_t *mReadPtrs[0x10000 / MEMMAP_MAP_CHUNK_SZ];
	extern MbcReadF mReadFuncs[0x10000 / MEMMAP_MAP_CHUNK_SZ];
	extern uint8_t *mWritePtrs[0x10000 / MEMMAP_MAP_CHUNK_SZ];
	extern MbcWriteF mWriteFuncs[0x10000 / MEMMAP_MAP_CHUNK_SZ];
	
	
	
	#define CART_BASIC					0x00
	#define CART_WITH_RAM				0x01
	#define CART_WITH_BATTERY			0x02
	#define CART_WITH_RTC				0x04	//technically implies battery
	#define CART_WITH_RUMBLE			0x08
	#define CART_WITH_ACCELEROMETER		0x10
	
	enum RomColorSupport {
		RomNoColor,
		RomColorEnhanced,
		RomColorRequired,
	};
	
	#define ROM_NAME_LEN		16
	
	bool mbcRomAnalyze(const void *rom, uint32_t *romSzExpectedP, uint32_t *ramSzExpectedP, enum RomColorSupport *colorSupportP, char *romNameP);			//needs sizeof(struct CartHeader) bytes at least
	bool mbcInit(const void *rom, uint32_t *romSzExpectedP, void *ram, uint32_t *ramSzExpectedP);				//analyzes AND init MBC
	
	void* mbcPrvGetWramBuf(void);	//we use WRAM for a buffer
	
#endif


#endif
