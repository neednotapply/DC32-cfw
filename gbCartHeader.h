#ifndef _GB_CART_HEADER_H_
#define _GB_CART_HEADER_H_

#include <stdint.h>



struct CartHeader {
	uint8_t pad[0x100];
	uint8_t entryJmp[4];
	uint8_t nintendoLogo[0x30];
	union {
		uint8_t titleDMG[16];
		struct {
			uint8_t titleCGB[11];
			uint8_t manufCode[4];
			uint8_t cgbFlag;
		};
	};
	uint8_t newLicenseeCode[2];
	uint8_t sgbFlag;
	uint8_t cartType;
	uint8_t romSize;
	uint8_t ramSize;
	uint8_t destinationCode;
	uint8_t oldLicenseeCode;
	uint8_t maskRomVer;
	uint8_t hdrCsum;
	uint8_t romCsum[2];
};



#endif
