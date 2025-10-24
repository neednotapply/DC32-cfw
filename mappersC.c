#include <stdio.h>
#include "printf.h"
#include "gbCore.h"
#include "mbc.h"



uint8_t gbExtRead(uint16_t addr)
{
	uint8_t *base = mReadPtrs[addr / MEMMAP_MAP_CHUNK_SZ];
	
	if (base)
		return base[addr];
	
	if (mReadFuncs[addr / MEMMAP_MAP_CHUNK_SZ])
		return (mReadFuncs[addr / MEMMAP_MAP_CHUNK_SZ])(addr);
	
	#ifndef QUIET_MAPPERS
		pr("read of 0x%04X\n", addr);
		while(1);
	#endif
	return 0xff; 
}

void gbExtWrite(uint16_t addr, uint8_t val)
{
	uint8_t *base = mWritePtrs[addr / MEMMAP_MAP_CHUNK_SZ];
	
	if (base) {
		
		base[addr] = val;
		return;
	}
	
	if (mWriteFuncs[addr / MEMMAP_MAP_CHUNK_SZ] && (mWriteFuncs[addr / MEMMAP_MAP_CHUNK_SZ])(addr, val))
		return;
	
	#ifndef QUIET_MAPPERS
		pr("write of 0x%02x to 0x%04X\n", val, addr);
		while(1);
	#endif
}
