#include "mbc.h"

#include <string.h>
#include "gbCartHeader.h"

bool mbcRomAnalyze(const void *rom, uint32_t *romSzExpectedP, uint32_t *ramSzExpectedP,
	enum RomColorSupport *colorSupportP, char *romNameP)
{
	const struct CartHeader *hdr = (const struct CartHeader*)rom;
	static const uint8_t magic[] = {
		0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
		0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
		0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
	};
	const uint8_t *csumStart = hdr->titleDMG, *csumEnd = &hdr->hdrCsum;
	uint32_t romSz, ramSz;
	uint8_t csum = 0;

	if (memcmp(magic, hdr->nintendoLogo, sizeof(hdr->nintendoLogo)))
		return false;
	while (csumStart != csumEnd)
		csum -= 1 + *csumStart++;
	if (csum != hdr->hdrCsum)
		return false;

	if (hdr->romSize <= 8)
		romSz = 32768u << hdr->romSize;
	else switch (hdr->romSize) {
		case 0x52:
			romSz = 0x120000u;
			break;
		case 0x53:
			romSz = 0x140000u;
			break;
		case 0x54:
			romSz = 0x180000u;
			break;
		default:
			return false;
	}

	switch (hdr->ramSize) {
		case 0x00:
			ramSz = 0;
			break;
		case 0x01:
			ramSz = 2u << 10;
			break;
		case 0x02:
			ramSz = 8u << 10;
			break;
		case 0x03:
			ramSz = 32u << 10;
			break;
		case 0x04:
			ramSz = 128u << 10;
			break;
		case 0x05:
			ramSz = 64u << 10;
			break;
		default:
			return false;
	}

	switch (hdr->cartType) {
		case 0x06:
			if (ramSz)
				return false;
			ramSz = 256;
			break;
		case 0x22:
			if (ramSz)
				return false;
			ramSz = (romSz == 0x100000u) ? 256u : 512u;
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
