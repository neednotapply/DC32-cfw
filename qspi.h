#ifndef _QSPI_H_
#define _QSPI_H_


#include <stdbool.h>
#include <stdint.h>


#define QSPI_ERASE_GRANULARITY			0x1000
#define QSPI_WRITE_GRANULARITY			0x0100
bool flashWrite(uint32_t addr, uint32_t erzSz, const void *dataSrc, uint32_t writeSz);	//will erase as needed, sz must be >= 256

void flashBootInit(void);

uint64_t flashGetUid(void);	//preferably this woudl be done at boot and cached...

#endif
