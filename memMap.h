#ifndef _MEM_MAP_H_
#define _MEM_MAP_H_




#define CART_RAM_ADDR_IN_RAM		((void*)0x20000000)

//flash addrs

#define QSPI_SETTINGS_START			0x100C0000
#define QSPI_SETTINGS_LEN			0x0001F000

#define QSPI_FILENAME_START			0x100DF000	//filename to save savegames correctly (one 4K block used)
#define QSPI_FILENAME_MAXLEN		0x00001000

#define QSPI_RAM_COPY_START			0x100E0000	//128K max, but we support 64K max
#ifndef QSPI_RAM_SIZE_MAX
#define QSPI_RAM_SIZE_MAX			0x00010000
#endif

#define QSPI_ROM_START				0x10100000
#define QSPI_ROM_SIZE_MAX			0x00300000


#endif
