#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "frontend.h"
#include "printf.h"
#include "gbCore.h"
#include "gb.h"

#pragma GCC optimize ("Ofast")


#define VRAM_BASE					0x8000
#define VRAM_BANK_SIZE				0x2000
	
static uint8_t mFrameDithering;

static uint16_t gbSwapBits(uint16_t v)
{
	#if defined(THUMB_VER) && THUMB_VER >= 4

		asm(
			"	rbit	%0, %0		\n\t"
			"	rev		%0, %0		\n\t"
			:"+r"(v)
		);

		return v;
	#else

		static const uint8_t swaptbl[] = {
			0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
			0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
			0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
			0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
			0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
			0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
			0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
			0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
			0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
			0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
			0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
			0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
			0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
			0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
			0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
			0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff, 	
		};
	
		return swaptbl[v & 0xff] + (swaptbl[v >> 8] << 8);

	#endif
}

void gbSetFrameDithering(uint_fast8_t n)
{
	mFrameDithering = n;
}

void __attribute__((noinline)) gbDrawDisp(uint8_t *restrict hram, uint8_t lineNum, bool presentsAsCgb)
{
	static uint8_t frameSkipCtr;
	
	if (!lineNum) {
		if (++frameSkipCtr == mFrameDithering)
			frameSkipCtr = 0;
	}
	
	if (!frameSkipCtr){
	
		const uint8_t *vramBank0 = mVRAM, *vramBank1 = vramBank0 + VRAM_BANK_SIZE;
		PIXFMT scrBuf[160 + 7 + 7 + 2], *scr = scrBuf + 8;	//+ up to 7 on both sides for BG drawing + 1 to allow an extra tile to be drawn on the end, extra + 1 to align the draw area to a word
		uint8_t bgToOamPrioBuf[160 + 7 + 7 + 2], *bgToOamPrio = bgToOamPrioBuf + 8;
		uint_fast8_t i;
		
		if (presentsAsCgb || (hram[0x40] & 1)) {	//display background and maybe the window
			
			uint_fast16_t tiles = ((hram[0x40] & 0x10) ? 0x8000 : 0x8800) - VRAM_BASE;
			uint_fast8_t er, ec, tIdx, nTiles, tileBasedCol, winStartX;
			uint_fast8_t addend = (hram[0x40] & 0x10) ? 0x00: 0x80;
			uint_fast16_t map, mapRow;
			uint8_t *bgToOamPrioDst;
			PIXFMT *dst;
			
			//get background info
			map = ((hram[0x40] & 0x08) ? 0x9C00 : 0x9800) - VRAM_BASE;
			er = (uint8_t)(hram[0x42] + lineNum);		//wraparound at 256 rows
			ec = hram[0x43];
			
			//each tile is 8 tall, given that we know the effective row (32 tiles), we can offset the map to point to that row right away
			mapRow = map + (uint_fast16_t)(er / 8) * 32;
			
			//we want to draw entire tiles at a time so we offset as needed and maybe overdraw a little (buffer space allows)
			nTiles = 160 / 8 + 1;						//draw enough tiles to fill the row
			dst = scr - (ec % 8);						//if we're not drawing a full tile, draw in our pre-screen buffer space
			bgToOamPrioDst = bgToOamPrio - (ec % 8);	//same as above
			tileBasedCol = ec / 8;	//sort out the tile colume we're on
			for (tIdx = 0; tIdx < nTiles; tIdx++, tileBasedCol = (tileBasedCol + 1) % 32) {
				
				uint_fast16_t mapAtTile = mapRow + tileBasedCol;
				uint_fast8_t tileNo = (uint8_t)(vramBank0[mapAtTile] + addend), rowInTile;
				uint_fast8_t tileProp = vramBank1[mapAtTile] * (unsigned)presentsAsCgb;
				const PIXFMT *pal = presentsAsCgb ? cgbBgPals[tileProp & 7] : dmgBgPal;
				uint_fast16_t tileDataAddr, tileData;
				
				rowInTile = er % 8;
				if (tileProp & 0x40)			//vflip?
					rowInTile = 7 - rowInTile;
				
				tileDataAddr = tiles + 2 * rowInTile + (uint_fast16_t)tileNo * 16;
				tileData = *(const uint16_t*)(vramBank0 + VRAM_BANK_SIZE * ((tileProp >> 3) & 1) + tileDataAddr);
				
				if (tileProp & 0x20)			//hflip?
					tileData = gbSwapBits(tileData);
				
				for (i = 0; i < 8; i++, tileData <<= 1) {
					
					uint_fast8_t idx = ((uint16_t)((tileData & 0x8080) * 0x81)) >> 14;
					
					*dst++ = pal[idx];
					*bgToOamPrioDst++ = tileProp;	//we care for top bit only
				}
			}
	
			//Window may need to be shown
			if ((hram[0x40] & 0x20) && lineNum >= hram[0x4A] && (winStartX = hram[0x4B]) <= 166){
				
				map = ((hram[0x40] & 0x40) ? 0x9c00 : 0x9800) - VRAM_BASE;
				er = hram[HRAM_INDEX_WINLINE]++;	//to allow behaviour in spec
				dst = scr + winStartX - 7;
				bgToOamPrioDst = bgToOamPrio + winStartX - 7;	//same as above
				nTiles = (167 - winStartX + 7) / 8;
				
				//each tile is 8 tall, given that we know the effective row (32 tiles), we can offset the map to point to that row right away
				mapRow = map + (uint_fast16_t)(er / 8) * 32;
				
				for (tIdx = 0; tIdx < nTiles; tIdx++) {
					
					uint_fast16_t mapAtTile = mapRow + tIdx;
					uint_fast8_t tileNo = (uint8_t)(vramBank0[mapAtTile] + addend), rowInTile;
					uint_fast8_t tileProp = vramBank1[mapAtTile] * (unsigned)presentsAsCgb;
					const PIXFMT *pal = presentsAsCgb ? cgbBgPals[tileProp & 7] : dmgBgPal;
					uint_fast16_t tileDataAddr, tileData;
					
					rowInTile = er % 8;
					if (tileProp & 0x40)			//vflip?
						rowInTile = 7 - rowInTile;
					
					tileDataAddr = tiles + 2 * rowInTile + (uint_fast16_t)tileNo * 16;
					tileData = *(const uint16_t*)(vramBank0 + VRAM_BANK_SIZE * ((tileProp >> 3) & 1) + tileDataAddr);
					
					if (tileProp & 0x20)			//hflip?
						tileData = gbSwapBits(tileData);
					
					for (i = 0; i < 8; i++, tileData <<= 1) {
						
						uint_fast8_t idx = ((uint16_t)((tileData & 0x8080) * 0x81)) >> 14;
						
						*dst++ = pal[idx];
						*bgToOamPrioDst++ |= tileProp;			//we care for top bit only
					}
				}
			}
		}
		else {
			
			//background/window are off - erase the screen and mark as background color, it also has no attributes
			for (i = 0; i < 160; i++) {
				scr[i] = 0xffff | BG_FLAG_UNDER_OBJS;	//white and under objects
				bgToOamPrio[i] = 0;
			}
		}
	
		//we may have sprites to draw
		if (hram[0x40] & 2) {
			
			const uint8_t *oam = mOAM, *spriteAttrForSort = oam;
			uint_fast8_t spriteHeight = (hram[0x40] & 4) ? 16 : 8;
			const uint8_t *spriteOrder[40], **spritesEnd, **spritesCur;
			
			//first find the sprites that are on this line
			spritesEnd = spriteOrder;
			for (i = 0; i < 40; i++, spriteAttrForSort += 4) {
				
				uint_fast8_t sY = spriteAttrForSort[0], sX = spriteAttrForSort[1];
				
				if (!sY || !sX || sX > 168 || sY > (unsigned)(lineNum + 16) || sY + spriteHeight <= lineNum + 16u)
					continue;
				
				*spritesEnd++ = spriteAttrForSort;
			}
			//for CGB we're done, earlier-in-the-table sprites win
			
			if (!presentsAsCgb) {	//DMG: leftmost sprite wins (selection sort is good enough for 40 items)
				
				uint_fast8_t spritesFound = spritesEnd - spriteOrder;
				uint_fast8_t maxSpritesToOutput = spritesFound > 10 ? 10 : spritesFound;

				for (i = 0; i < maxSpritesToOutput; i++) {
					
					uint_fast8_t j, bestIdx = i, bestX = spriteOrder[i][1];
					
					for (j = i + 1; j < (unsigned)(spritesEnd - spriteOrder); j++) {
					
						uint_fast8_t curX = spriteOrder[j][1];
						
						if (curX < bestX) {
							bestX = curX;
							bestIdx = j;
						}
					}
					
					if (bestIdx != i) {
						
						const uint8_t *t;
						t = spriteOrder[i];
						spriteOrder[i] = spriteOrder[bestIdx];
						spriteOrder[bestIdx] = t;
					}
				}
				spritesEnd = spriteOrder + i;
			}
			else {

				//10 sprites max per line
				if (spritesEnd - spriteOrder > 10)
					spritesEnd = spriteOrder + 10;
			}

			//we sorted them from most important to least. draw in reverse order to make sure most important ones win
			for (spritesCur = spritesEnd; spritesCur != spriteOrder; ) {
				
				const uint8_t *sprite = *--spritesCur;
				uint_fast8_t sY = sprite[0];
				uint_fast8_t sX = sprite[1];
				uint_fast8_t sT = sprite[2];
				uint_fast8_t sA = sprite[3];
				uint_fast16_t tileDataVal, tileDataPtr;
				const uint8_t *bgToOamPrioPtr;
				const PIXFMT *pal;
				uint_fast8_t er;
				PIXFMT *dst;
				
				//limit for 16-high sprites
				if (hram[0x40] & 4)
					sT &= 0xfe;
				
				//calculate Y location of the drawing
				er = lineNum + 16 - sY;			//"er" is the sprite row we care about
				if (sA & 0x40)					//correct "er" for flipY
					er = spriteHeight - er - 1;
				
				//find the tile and read the useful row of it
				tileDataPtr = 0x8000 + 16 * (uint_fast16_t)sT + 2 * (uint16_t)er - VRAM_BASE;
				
				//CGB has a second bank we might use)
				tileDataPtr += VRAM_BANK_SIZE * ((unsigned)presentsAsCgb & (sA >> 3));
				
				//get the data
				tileDataVal = *(const uint16_t*)(vramBank0 + tileDataPtr);
				
				//flip tile in X-direction as needed
				if (sA & 0x20)
					tileDataVal = gbSwapBits(tileDataVal);
				
				//find the proper palette
				pal = presentsAsCgb ? cgbFgPals[sA & 7] : dmgFgPals[(sA & 0x10) ? 1 : 0];
				
				//get dest pointer. we're allowed to overdraw and it makes life easier
				dst = scr + sX - 8;
				bgToOamPrioPtr = bgToOamPrio + sX - 8;	//same as above
				
				//draw the columns
				for (i = 0; i < 8; i++, tileDataVal <<= 1) {
				
					uint_fast8_t idx = ((uint16_t)((tileDataVal & 0x8080) * 0x81)) >> 14;
					
					/*
						DMG mode:
							 sA & 0x80 is clear then sprite is always rendered above the background and the window.
							 However if it is set,  then the sprite is hidden behind the background and window unless
							 the colour of the background or window is white, then it is still rendered on top.

						CGB mode:
						    If the BG color index is 0, the OBJ will always have priority;
						    Otherwise, if LCDC bit 0 is clear, the OBJ will always have priority;
						    Otherwise, if both the BG Attributes and the OAM Attributes have bit 7 clear, the OBJ will have priority;
						    Otherwise, BG will have priority.
					*/
					if (idx) {
						
						if ((dst[i] & BG_FLAG_UNDER_OBJS) || !((sA | bgToOamPrioPtr[i]) >> 7))
							dst[i] = pal[idx];
					}
				}
			}
		}
	
		//clear marker bit (though we could keep it, it is just an lsb)
	//	for (i = 0; i < 160; i++)
	//		scr[i] &=~ BG_FLAG_UNDER_OBJS;
		
		gbDrawLine(lineNum, scr);
	}
}

