#include "arduboy/arduboy.h"

#include <stdint.h>
#include <string.h>

extern "C" {
#include "qspi.h"
}

#include "miniz.h"

#define ARDUBOY_ZIP_LOCAL_SIG 0x04034b50u
#define ARDUBOY_ZIP_CENTRAL_SIG 0x02014b50u
#define ARDUBOY_ZIP_EOCD_SIG 0x06054b50u
#define ARDUBOY_ZIP_METHOD_STORED 0u
#define ARDUBOY_ZIP_METHOD_DEFLATE 8u
#define ARDUBOY_ZIP_GP_ENCRYPTED 0x0001u
#define ARDUBOY_ZIP64_SENTINEL 0xffffffffu
#define ARDUBOY_FLASH_WRITE_CHUNK QSPI_WRITE_GRANULARITY

struct ArduboyZipMember {
	uint32_t dataOffset;
	uint32_t compressedSize;
	uint32_t uncompressedSize;
	uint16_t method;
};

struct ArduboyFlashWriter {
	uint32_t addr;
	uint32_t pos;
	uint32_t maxSize;
	uint32_t fill;
	uint8_t *buf;
};

static uint16_t arduboyPrvRd16(const uint8_t *p)
{
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t arduboyPrvRd32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool arduboyPrvEndsWithNoCase(const char *str, uint32_t len, const char *suffix)
{
	uint32_t suffixLen = 0;

	while (suffix[suffixLen])
		suffixLen++;
	if (len < suffixLen)
		return false;
	str += len - suffixLen;
	for (uint32_t i = 0; i < suffixLen; i++) {
		char a = str[i], b = suffix[i];

		if (a >= 'A' && a <= 'Z')
			a = (char)(a - 'A' + 'a');
		if (b >= 'A' && b <= 'Z')
			b = (char)(b - 'A' + 'a');
		if (a != b)
			return false;
	}
	return true;
}

static int arduboyPrvHexNibble(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	return -1;
}

static bool arduboyPrvHexByte(const char *src, uint8_t *byteP)
{
	int hi = arduboyPrvHexNibble(src[0]);
	int lo = arduboyPrvHexNibble(src[1]);

	if (hi < 0 || lo < 0)
		return false;
	*byteP = (uint8_t)((hi << 4) | lo);
	return true;
}

typedef bool (*ArduboyHexDataF)(uint32_t addr, const uint8_t *data, uint32_t len, void *userData);

static bool arduboyPrvHexParse(const void *rom, uint32_t size, ArduboyHexDataF dataF,
	void *userData, uint32_t *flashUsedP)
{
	const char *bytes = (const char*)rom;
	uint32_t pos = 0, base = 0, flashUsed = 0;

	if (!bytes || !size)
		return false;
	while (pos < size) {
		uint32_t lineStart, lineEnd;
		uint8_t count, addrHi, addrLo, type, checksum, data[255];
		uint32_t addr, sum;

		while (pos < size && (bytes[pos] == '\r' || bytes[pos] == '\n' || bytes[pos] == ' ' || bytes[pos] == '\t'))
			pos++;
		if (pos >= size)
			break;
		if (bytes[pos++] != ':')
			return false;
		lineStart = pos;
		while (pos < size && bytes[pos] != '\r' && bytes[pos] != '\n')
			pos++;
		lineEnd = pos;
		if (lineEnd - lineStart < 10)
			return false;
		if (!arduboyPrvHexByte(bytes + lineStart, &count) ||
				!arduboyPrvHexByte(bytes + lineStart + 2, &addrHi) ||
				!arduboyPrvHexByte(bytes + lineStart + 4, &addrLo) ||
				!arduboyPrvHexByte(bytes + lineStart + 6, &type))
			return false;
		if (lineEnd - lineStart != 10u + (uint32_t)count * 2u)
			return false;
		sum = count + addrHi + addrLo + type;
		for (uint32_t i = 0; i < count; i++) {
			if (!arduboyPrvHexByte(bytes + lineStart + 8 + i * 2, &data[i]))
				return false;
			sum += data[i];
		}
		if (!arduboyPrvHexByte(bytes + lineStart + 8 + count * 2, &checksum))
			return false;
		sum += checksum;
		if ((sum & 0xffu) != 0)
			return false;

		addr = ((uint32_t)addrHi << 8) | addrLo;
		if (type == 0) {
			uint32_t absolute = base + addr;

			if (absolute > ARDUBOY_FLASH_SIZE || count > ARDUBOY_FLASH_SIZE - absolute)
				return false;
			if (dataF && !dataF(absolute, data, count, userData))
				return false;
			if (flashUsed < absolute + count)
				flashUsed = absolute + count;
		}
		else if (type == 1) {
			if (flashUsedP)
				*flashUsedP = flashUsed;
			return flashUsed != 0;
		}
		else if (type == 2) {
			if (count != 2)
				return false;
			base = (((uint32_t)data[0] << 8) | data[1]) << 4;
		}
		else if (type == 4) {
			if (count != 2)
				return false;
			base = (((uint32_t)data[0] << 8) | data[1]) << 16;
		}
		else {
			return false;
		}
	}
	if (flashUsedP)
		*flashUsedP = flashUsed;
	return flashUsed != 0;
}

static bool arduboyPrvFindZipHexMember(const void *packageData, uint32_t packageSize, ArduboyZipMember *memberP)
{
	const uint8_t *zip = (const uint8_t*)packageData;
	uint32_t eocd = UINT32_MAX, entries, centralSize, centralOffset, p;

	if (!zip || packageSize < 22)
		return false;
	for (p = packageSize - 22; p != UINT32_MAX; p--) {
		if (arduboyPrvRd32(zip + p) == ARDUBOY_ZIP_EOCD_SIG) {
			eocd = p;
			break;
		}
		if (!p || packageSize - p > 66000u)
			break;
	}
	if (eocd == UINT32_MAX)
		return false;
	entries = arduboyPrvRd16(zip + eocd + 10);
	centralSize = arduboyPrvRd32(zip + eocd + 12);
	centralOffset = arduboyPrvRd32(zip + eocd + 16);
	if (centralOffset == ARDUBOY_ZIP64_SENTINEL || centralSize == ARDUBOY_ZIP64_SENTINEL ||
			centralOffset > packageSize || centralSize > packageSize - centralOffset)
		return false;
	p = centralOffset;
	for (uint32_t i = 0; i < entries && p + 46 <= centralOffset + centralSize; i++) {
		const uint8_t *cent = zip + p;
		uint16_t flags, method, nameLen, extraLen, commentLen;
		uint32_t compressedSize, uncompressedSize, localOffset, localNameLen, localExtraLen;
		const char *name;

		if (arduboyPrvRd32(cent) != ARDUBOY_ZIP_CENTRAL_SIG)
			return false;
		flags = arduboyPrvRd16(cent + 8);
		method = arduboyPrvRd16(cent + 10);
		compressedSize = arduboyPrvRd32(cent + 20);
		uncompressedSize = arduboyPrvRd32(cent + 24);
		nameLen = arduboyPrvRd16(cent + 28);
		extraLen = arduboyPrvRd16(cent + 30);
		commentLen = arduboyPrvRd16(cent + 32);
		localOffset = arduboyPrvRd32(cent + 42);
		if (p + 46u + nameLen + extraLen + commentLen > centralOffset + centralSize)
			return false;
		name = (const char*)(cent + 46);
		if (!(flags & ARDUBOY_ZIP_GP_ENCRYPTED) &&
				(method == ARDUBOY_ZIP_METHOD_STORED || method == ARDUBOY_ZIP_METHOD_DEFLATE) &&
				compressedSize != ARDUBOY_ZIP64_SENTINEL && uncompressedSize != ARDUBOY_ZIP64_SENTINEL &&
				arduboyPrvEndsWithNoCase(name, nameLen, ".hex")) {
			const uint8_t *local;
			uint32_t dataOffset;

			if (localOffset > packageSize || packageSize - localOffset < 30)
				return false;
			local = zip + localOffset;
			if (arduboyPrvRd32(local) != ARDUBOY_ZIP_LOCAL_SIG)
				return false;
			localNameLen = arduboyPrvRd16(local + 26);
			localExtraLen = arduboyPrvRd16(local + 28);
			dataOffset = localOffset + 30u + localNameLen + localExtraLen;
			if (dataOffset > packageSize || compressedSize > packageSize - dataOffset)
				return false;
			memberP->dataOffset = dataOffset;
			memberP->compressedSize = compressedSize;
			memberP->uncompressedSize = uncompressedSize;
			memberP->method = method;
			return true;
		}
		p += 46u + nameLen + extraLen + commentLen;
	}
	return false;
}

static bool arduboyPrvLooksLikePackage(const void *rom, uint32_t size)
{
	const uint8_t *bytes = (const uint8_t*)rom;

	return bytes && size >= 4 && arduboyPrvRd32(bytes) == ARDUBOY_ZIP_LOCAL_SIG;
}

extern "C" bool arduboyAnalyzeRom(const void *rom, uint32_t size, ArduboyRomInfo *info)
{
	uint32_t flashUsed = 0;
	bool isPackage = false;

	if (arduboyPrvLooksLikePackage(rom, size)) {
		ArduboyZipMember member;

		isPackage = true;
		if (!arduboyPrvFindZipHexMember(rom, size, &member))
			return false;
		flashUsed = member.uncompressedSize;
	}
	else if (!arduboyPrvHexParse(rom, size, NULL, NULL, &flashUsed))
		return false;
	if (info) {
		memset(info, 0, sizeof(*info));
		memcpy(info->name, "ARDUBOY", 8);
		info->romSize = size;
		info->saveRamSize = ARDUBOY_SAVE_RAM_SIZE;
		info->flashUsed = flashUsed;
		info->isPackage = isPackage;
	}
	return true;
}

static bool arduboyPrvFlashWriterFlush(ArduboyFlashWriter *writer, bool final)
{
	if (!writer->fill)
		return true;
	if (!final && writer->fill < ARDUBOY_FLASH_WRITE_CHUNK)
		return true;
	if (writer->fill < ARDUBOY_FLASH_WRITE_CHUNK)
		memset(writer->buf + writer->fill, 0, ARDUBOY_FLASH_WRITE_CHUNK - writer->fill);
	if (!flashWrite(writer->addr + writer->pos, 0, writer->buf, ARDUBOY_FLASH_WRITE_CHUNK))
		return false;
	writer->pos += ARDUBOY_FLASH_WRITE_CHUNK;
	writer->fill = 0;
	return true;
}

static bool arduboyPrvFlashWriterWrite(ArduboyFlashWriter *writer, const uint8_t *data, uint32_t len)
{
	while (len) {
		uint32_t now = ARDUBOY_FLASH_WRITE_CHUNK - writer->fill;

		if (writer->pos + writer->fill >= writer->maxSize)
			return false;
		if (now > len)
			now = len;
		if (now > writer->maxSize - writer->pos - writer->fill)
			now = writer->maxSize - writer->pos - writer->fill;
		memcpy(writer->buf + writer->fill, data, now);
		writer->fill += now;
		data += now;
		len -= now;
		if (writer->fill == ARDUBOY_FLASH_WRITE_CHUNK && !arduboyPrvFlashWriterFlush(writer, false))
			return false;
	}
	return true;
}

extern "C" bool arduboyExtractPackageToFlash(const void *packageData, uint32_t packageSize,
	uint32_t flashAddr, uint32_t maxSize, void *inflateDict, uint32_t inflateDictSize,
	void *writeBuf, uint32_t writeBufSize, uint32_t *hexSizeP)
{
	ArduboyZipMember member;
	const uint8_t *packageBytes = (const uint8_t*)packageData;
	ArduboyFlashWriter writer;

	if (!packageBytes || !writeBuf || writeBufSize < ARDUBOY_FLASH_WRITE_CHUNK)
		return false;
	if (!arduboyPrvFindZipHexMember(packageData, packageSize, &member))
		return false;
	if (member.uncompressedSize > maxSize)
		return false;

	memset(&writer, 0, sizeof(writer));
	writer.addr = flashAddr;
	writer.maxSize = maxSize;
	writer.buf = (uint8_t*)writeBuf;

	if (member.method == ARDUBOY_ZIP_METHOD_STORED) {
		if (!arduboyPrvFlashWriterWrite(&writer, packageBytes + member.dataOffset, member.uncompressedSize))
			return false;
	}
	else {
		size_t outSize;

		if (!inflateDict || inflateDictSize < 32768u)
			return false;
		outSize = tinfl_decompress_mem_to_mem(inflateDict, inflateDictSize,
			packageBytes + member.dataOffset, member.compressedSize,
			TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
		if (outSize == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED || outSize != member.uncompressedSize)
			return false;
		if (!arduboyPrvFlashWriterWrite(&writer, (const uint8_t*)inflateDict, (uint32_t)outSize))
			return false;
	}

	if (!arduboyPrvFlashWriterFlush(&writer, true))
		return false;
	if (hexSizeP)
		*hexSizeP = member.uncompressedSize;
	return true;
}
