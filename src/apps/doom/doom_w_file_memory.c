#include "doom_dc32.h"

#include <stdint.h>
#include <string.h>
#include "i_system.h"
#include "w_file.h"

const uint8_t *whd_map_base = (const uint8_t*)DOOM_DC32_WHX_FLASH_ADDR;
extern const wad_file_class_t memory_wad_file;

static const wad_file_t mFile = {
	.file_class = &memory_wad_file,
	.length = 0,
	.mapped = (const uint8_t*)DOOM_DC32_WHX_FLASH_ADDR,
	.path = "<dc32-whx>",
};

static wad_file_t *W_Memory_OpenFile(const char *path)
{
	const uint8_t *mapped = (const uint8_t*)DOOM_DC32_WHX_FLASH_ADDR;

	(void)path;
	if (mapped[0] != 'I' || mapped[1] != 'W' || mapped[2] != 'H' || mapped[3] != 'X')
		I_Error("Expected DOOM WHX at flash staging address");
	return (wad_file_t*)&mFile;
}

static void W_Memory_CloseFile(wad_file_t *wad)
{
	(void)wad;
}

static size_t W_Memory_Read(wad_file_t *wad, unsigned int offset, void *buffer, size_t buffer_len)
{
	memcpy(buffer, wad->mapped + offset, buffer_len);
	return buffer_len;
}

const wad_file_class_t memory_wad_file = {
	W_Memory_OpenFile,
	W_Memory_CloseFile,
	W_Memory_Read,
};
