
/**
 *
 * @file file.cpp
 *
 * Part of the OpenJazz project
 *
 * @par History:
 * - 23rd August 2005: Created main.c
 * - 22nd July 2008: Created util.c from parts of main.c
 * - 3rd February 2009: Renamed util.c to util.cpp
 * - 3rd February 2009: Created file.cpp from parts of util.cpp
 *
 * @par Licence:
 * Copyright (c) 2005-2017 AJ Thomson
 * Copyright (c) 2015-2026 Carsten Teibes
 *
 * OpenJazz is distributed under the terms of
 * the GNU General Public License, version 2.0
 *
 * @par Description:
 * Deals with files.
 *
 */


#include "file.h"

#include "io/gfx/video.h"
#include "util.h"
#include "io/log.h"

#include "SDL_wrapper.h"
#include <string.h>
#include <unistd.h>
#include <miniz.h>

#ifdef DC32_OPENJAZZ
#include "apps/openjazz/openjazz_install.h"
#include "apps/openjazz/openjazz_cache.h"
#endif

#if !(defined(_WIN32) || defined(WII) || defined(PSP))
    #define UPPERCASE_FILENAMES
    #define LOWERCASE_FILENAMES
#endif


/**
 * Try opening a file from the available paths.
 *
 * @param name File name
 * @param pathType Kind of directory
 * @param write Whether or not the file can be written to
 */
File::File (const char* name, int pathType, bool write) :
#ifdef DC32_OPENJAZZ
	file({}),
#else
	file(nullptr), filePath(nullptr), forWriting(write) {
#endif
#ifdef DC32_OPENJAZZ
	filePath(nullptr), forWriting(write) {
#endif

	Path* path = gamePaths.paths;

	while (path) {

		// skip other paths
		if (pathType != PATH_TYPE_ANY && (path->pathType & pathType) != pathType) {
			path = path->next;
			continue;
		}

		// only allow certain write paths
		if (!write || (pathType & (PATH_TYPE_CONFIG|PATH_TYPE_TEMP)) > 0) {
			if (open(path->path, name, write)) return;
		} else LOG_FATAL("Not allowed to write to %s", name);

		path = path->next;

	}

	LOG_WARN("Could not open file: %s", name);

	throw E_FILE;

}


/**
 * Delete the file object.
 */
File::~File () {

#ifdef DC32_OPENJAZZ
	dc32OjFileClose(&file);
#else
	fclose(file);
#endif

	LOG_TRACE("Closed file: %s", filePath);

	delete[] filePath;

}


/**
 * Try opening a file from the given path
 *
 * @param path Directory path
 * @param name File name
 * @param write Whether or not the file can be written to
 */
bool File::open (const char* path, const char* name, bool write) {
	const char *fileMode = write ? "wb": "rb";

#ifdef DC32_OPENJAZZ
	(void)path;
	(void)fileMode;
	dc32OjLoadingAsset(name);
	filePath = createString(name);
	if (dc32OjUserFileOpen(name, write, &file)) {
		LOG_DEBUG("Opened badge save file: %s", filePath);
		return true;
	}
	if (!write && dc32OjFileOpen(name, &file)) {
		LOG_DEBUG("Opened packed game file: %s", filePath);
		return true;
	}
	delete[] filePath;
	filePath = nullptr;
	return false;
#else
	// Create the file path for the given directory
	filePath = createString(path, name);

	// Open the file from the path
	file = fopen(filePath, fileMode);

#ifdef UPPERCASE_FILENAMES
	if (!file) {
		uppercaseString(filePath + strlen(path));
		file = fopen(filePath, fileMode);
	}
#endif

#ifdef LOWERCASE_FILENAMES
	if (!file) {
		lowercaseString(filePath + strlen(path));
		file = fopen(filePath, fileMode);
	}
#endif

	if (file) {
		LOG_DEBUG("Opened file: %s", filePath);

		return true;
	}

	delete[] filePath;

	return false;
#endif
}


/**
 * Check if a file exists.
 *
 * @param fileName The file to check
 * @param pathType Kind of directory
 *
 * @return Whether or not the file exists
 */
bool File::exists (const char * fileName, int pathType) {
#ifdef DC32_OPENJAZZ
	(void)pathType;
	return dc32OjUserFileExists(fileName) || dc32OjFileExists(fileName);
#else
	/* FIXME: This currently duplicates code from Constructor/Destructor
	 * and open() method. Maybe merge this into a common function.
	 */

	bool exists = false;

	Path* path = gamePaths.paths;
	while (path) {
		// skip other paths
		if (pathType != PATH_TYPE_ANY && (path->pathType & pathType) != pathType) {
			path = path->next;
			continue;
		}

		// Create the file path for the given directory
		char *fullPath = createString(path->path, fileName);

		// Open the file from the path
		int res = access(fullPath, F_OK);

#ifdef UPPERCASE_FILENAMES
		if (res != 0) {
			uppercaseString(fullPath + strlen(path->path));
			res = access(fullPath, F_OK);
		}
#endif

#ifdef LOWERCASE_FILENAMES
		if (res != 0) {
			lowercaseString(fullPath + strlen(path->path));
			res = access(fullPath, F_OK);
		}
#endif

		delete[] fullPath;

		if(res == 0) {
			exists = true;

			break;
		}

		path = path->next;
	}

	return exists;
#endif
}



/**
 * Get the size of the file.
 *
 * @return The size of the file
 */
int File::getSize () {

#ifdef DC32_OPENJAZZ
	return (int)dc32OjFileSize(&file);
#else
	int pos, size;

	pos = ftell(file);

	fseek(file, 0, SEEK_END);

	size = ftell(file);

	fseek(file, pos, SEEK_SET);

	return size;
#endif

}


/**
 * Get the current read/write location within the file.
 *
 * @return The current location
 */
int File::tell () {

#ifdef DC32_OPENJAZZ
	return (int)dc32OjFileTell(&file);
#else
	return ftell(file);
#endif

}


/**
 * Set the read/write location within the file.
 *
 * @param offset The new offset
 * @param reset Whether to offset from the current location or the start of the file
 */
void File::seek (int offset, bool reset) {

#ifdef DC32_OPENJAZZ
	(void)dc32OjFileSeek(&file, offset, reset ? SEEK_SET: SEEK_CUR);
#else
	fseek(file, offset, reset ? SEEK_SET: SEEK_CUR);
#endif

}

int File::seek (int offset, int origin) {
#ifdef DC32_OPENJAZZ
	return dc32OjFileSeek(&file, offset, origin) ? 0 : -1;
#else
	return fseek(file, offset, origin);
#endif
}

int File::read (void *buffer, int size, int count) {
#ifdef DC32_OPENJAZZ
	return (int)(dc32OjFileRead(&file, buffer, (uint32_t)(size * count)) / (uint32_t)size);
#else
	return fread(buffer, size, count, file);
#endif
}

#ifdef DC32_OPENJAZZ
bool File::readExact (void *buffer, int length) {

	return buffer && length >= 0 && read(buffer, 1, length) == length;

}
#endif


/**
 * Load an unsigned char from the file.
 *
 * @return The value read
 */
unsigned char File::loadChar () {

#ifdef DC32_OPENJAZZ
	unsigned char ch = 0;
	(void)dc32OjFileRead(&file, &ch, 1);
	return ch;
#else
	return fgetc(file);
#endif

}


void File::storeChar (const unsigned char val) {

#ifdef DC32_OPENJAZZ
	(void)dc32OjFileWrite(&file, &val, 1);
#else
	fputc(val, file);
#endif

}


/**
 * Load an unsigned short int from the file.
 *
 * @return The value read
 */
unsigned short int File::loadShort () {

	unsigned short int val;

	val = loadChar();
	val += loadChar() << 8;

	return val;

}


/**
 * Load an unsigned short int with an upper limit from the file.
 *
 * @return The value read
 */
unsigned short int File::loadShort (unsigned short int max) {

	unsigned short int val;

	val = loadShort();

	if (val > max) {

		LOG_ERROR("Oversized value %d>%d in file %s", val, max, filePath);

		return max;

	}

	return val;

}


void File::storeShort (const unsigned short int val) {

#ifdef DC32_OPENJAZZ
	unsigned char data[2] = {
		(unsigned char)(val & 255),
		(unsigned char)(val >> 8),
	};
	(void)dc32OjFileWrite(&file, data, sizeof(data));
#else
	fputc(val & 255, file);
	fputc(val >> 8, file);
#endif

}


/**
 * Load a signed int from the file.
 *
 * @return The value read
 */
signed int File::loadInt () {

	unsigned int val;

	val = loadChar();
	val += loadChar() << 8;
	val += loadChar() << 16;
	val += loadChar() << 24;

	return *((signed int *)&val);

}


void File::storeInt (const signed int val) {

#ifdef DC32_OPENJAZZ
	unsigned int uval = *((const unsigned int *)&val);
	unsigned char data[4] = {
		(unsigned char)(uval & 255),
		(unsigned char)((uval >> 8) & 255),
		(unsigned char)((uval >> 16) & 255),
		(unsigned char)(uval >> 24),
	};
	(void)dc32OjFileWrite(&file, data, sizeof(data));
#else
	unsigned int uval;

	uval = *((unsigned int *)&val);

	fputc(uval & 255, file);
	fputc((uval >> 8) & 255, file);
	fputc((uval >> 16) & 255, file);
	fputc(uval >> 24, file);
#endif

}


/**
 * Load a string with given length from the file.
 *
 * @return The new string
 */
char * File::loadString (int length) {
	char *string = new char[length + 1];
	int res = read(string, 1, length);

	if (res != length)
		LOG_ERROR("Could not read whole string (%d of %d bytes read)", res, length);

	string[length] = '\0';

	return string;
}


void File::storeString (const char *string) {
	int length = strlen(string);

	storeData(string, length);
}


void File::storeData (const void* data, int length) {

#ifdef DC32_OPENJAZZ
	if (!forWriting || length < 0 ||
			dc32OjFileWrite(&file, data, (uint32_t)length) != (uint32_t)length)
		LOG_ERROR("Could not write OpenJazz file: %s", filePath ? filePath : "");
#else
	if(!forWriting) {
		LOG_ERROR("File %s not opened for writing!", filePath);
		return;
	}

	fwrite (data, length, 1, file);
#endif

}


/**
 * Load a block of uncompressed data from the file.
 *
 * @param length The length of the block
 *
 * @return Buffer containing the block of data
 */
unsigned char * File::loadBlock (int length) {

	unsigned char *buffer;

	buffer = new unsigned char[length];

	int res = read(buffer, 1, length);

	if (res != length)
		LOG_ERROR("Could not read whole block (%d of %d bytes read)", res, length);

	return buffer;

}


/**
 * Load a block of RLE compressed data from the file.
 *
 * @param length The length of the uncompressed block
 * @param checkSize Whether or not the RLE block is terminated
 *
 * @return Buffer containing the uncompressed data
 */
unsigned char* File::loadRLE (int length, bool checkSize) {
	unsigned char* buffer = new unsigned char[length];

	loadRLEInto(buffer, length, checkSize);
	return buffer;
}


void File::loadRLEInto (unsigned char* buffer, int length, bool checkSize) {
	int size = 0;

	if (!buffer || length <= 0)
		return;
	if (checkSize) {
		// Determine the offset that follows the block
		size = loadShort();
	}
	int start = tell();

	int pos = 0;
	while (pos < length) {
		unsigned char code = loadChar();
		unsigned char amount = code & 127;

		if (code & 128) { // repeat
			unsigned char value = loadChar();

			if (pos + amount >= length) break;

			memset(buffer + pos, value, amount);
			pos += amount;
		} else if (amount) { // copy
			if (pos + amount >= length) break;

			read(buffer + pos, 1, amount);
			pos += amount;
		} else { // end marker
			buffer[pos++] = loadChar();
			break;
		}
	}

	if (checkSize) {
		if (tell() != start + size)
			LOG_DEBUG("RLE block has incorrect size: %d vs. %d", tell() - start, size);

		seek(start + size, true);
	} else {
		LOG_MAX("RLE block was %d bytes long", tell() - start);
	}

}

#ifdef DC32_OPENJAZZ
static bool ojLoadRLEStream(File* file, int emitLength, File::RLESink sink,
		void* user, bool prefix, bool checkSize) {
	unsigned char input[256];
	unsigned char output[256];
	unsigned char header[2];
	int blockSize;
	int blockStart;
	int blockEnd;
	int inputPos = 0;
	int inputUsed = 0;
	int compressedRead = 0;
	int compressedUsed = 0;
	int outputUsed = 0;
	int decoded = 0;
	int emitted = 0;
	bool terminated = false;
	bool ok = file && sink && emitLength >= 0;

	if (!ok)
		return false;
	if (checkSize) {
		if (file->read(header, 1, sizeof(header)) != (int)sizeof(header))
			return false;
		blockSize = header[0] | (header[1] << 8);
	} else {
		blockSize = file->getSize() - file->tell();
	}
	blockStart = file->tell();
	if (blockSize < 2 || blockStart < 0 ||
			blockSize > file->getSize() - blockStart)
		return false;
	blockEnd = blockStart + blockSize;
	auto flush = [&]() {
		if (outputUsed && !sink(user, output, outputUsed))
			ok = false;
		outputUsed = 0;
	};
	auto emit = [&](unsigned char value) {
		if (!prefix && decoded >= emitLength) {
			ok = false;
			return;
		}
		if (emitted < emitLength) {
			output[outputUsed++] = value;
			emitted++;
			if (outputUsed == (int)sizeof(output))
				flush();
		}
		decoded++;
	};
	auto readByte = [&](unsigned char& value) {
		if (compressedUsed >= blockSize) {
			ok = false;
			return;
		}
		if (inputPos == inputUsed) {
			int now = blockSize - compressedRead;

			if (now > (int)sizeof(input))
				now = sizeof(input);
			if (now <= 0 || file->read(input, 1, now) != now) {
				ok = false;
				return;
			}
			compressedRead += now;
			inputPos = 0;
			inputUsed = now;
		}
		value = input[inputPos++];
		compressedUsed++;
	};
	while (ok && compressedUsed < blockSize && !terminated) {
		unsigned char code = 0;
		unsigned char amount;

		readByte(code);
		if (!ok)
			break;
		amount = code & 127;
		if (code & 128) {
			unsigned char value = 0;

			readByte(value);
			if (!ok)
				break;
			while (amount-- && ok)
				emit(value);
		} else if (amount) {
			while (amount-- && ok) {
				unsigned char value = 0;

				readByte(value);
				if (ok)
					emit(value);
			}
		} else {
			unsigned char value = 0;

			readByte(value);
			if (ok)
				emit(value);
			terminated = true;
		}
	}
	flush();
	if (!terminated || compressedUsed != blockSize)
		ok = false;
	if (file->tell() != blockEnd)
		file->seek(blockEnd, true);
	return ok && emitted == emitLength &&
		(prefix ? decoded >= emitLength : decoded == emitLength);
}

bool File::loadRLEStream (int length, RLESink sink, void* user, bool checkSize) {
	return ojLoadRLEStream(this, length, sink, user, false, checkSize);
}

bool File::loadRLEPrefixStream (int prefixLength, RLESink sink, void* user) {
	return ojLoadRLEStream(this, prefixLength, sink, user, true, true);
}
#endif


/**
 * Skip past a block of RLE compressed data in the file.
 */
void File::skipRLE () {

	int next;

	next = loadChar();
	next += loadChar() << 8;

	seek(next, SEEK_CUR);

}


/**
 * Load a block of LZ compressed data from the file.
 *
 * @param compressedLength The length of the compressed block
 * @param length The length of the uncompressed block
 *
 * @return Buffer containing the uncompressed data
 */
unsigned char* File::loadLZ (int compressedLength, int length) {

	unsigned char* compressedBuffer;
	unsigned char* buffer;

	compressedBuffer = loadBlock(compressedLength);

	buffer = new unsigned char[length];

	uncompress(buffer, (unsigned long int *)&length, compressedBuffer, compressedLength);

	delete[] compressedBuffer;

	return buffer;

}


/**
 * Load a terminated string from the file.
 *
 * @param maxSize maximum length of field in the file
 *
 * @return The new string
 */
char * File::loadTerminatedString (int maxSize) {
	char *string;
	int length = loadChar();

	if (maxSize > 0 && length > maxSize) {
		LOG_WARN("Trimming oversized terminated string (%d > %d)", length, maxSize);
		length = maxSize;
	}

	if (length) {
		string = loadString(length);
	} else {
		string = new char[1];
		string[0] = '\0';
	}

	// Skip until end of field
	if(maxSize > length) {
		seek(maxSize - length);
	}

	return string;
}


/**
 * Load a 8.3 file name from the file.
 *
 * @return The new string
 */
char * File::loadFileName () {
	int length = 0;
	char *string = new char[13];

	for (int i = 0; i < 9; i++) {
		string[i] = loadChar();

		if (string[i] == '.') {
			string[++i] = loadChar();
			string[++i] = loadChar();
			string[++i] = loadChar();
			i++;

			break;
		}

		length = i;
	}

	string[length] = 0;

	return string;
}


/**
 * Load RLE compressed graphical data from the file.
 *
 * @param width The width of the image to load
 * @param height The height of the image to load
 * @param checkSize Whether or not the RLE block is terminated
 *
 * @return SDL surface containing the loaded image
 */
SDL_Surface* File::loadSurface (int width, int height, bool checkSize) {

	SDL_Surface* surface;
	unsigned char* pixels;

	pixels = loadRLE(width * height, checkSize);

	surface = video.createSurface(pixels, width, height);

	delete[] pixels;

	return surface;

}


/**
 * Load a block of scrambled pixel data from the file.
 *
 * @param length The length of the block
 *
 * @return Buffer containing the de-scrambled data
 */
unsigned char* File::loadPixels  (int length) {

	unsigned char* pixels;
	unsigned char* sorted;
	int count;

	sorted = new unsigned char[length];

	pixels = loadBlock(length);

	// Rearrange pixels in correct order
	for (count = 0; count < length; count++) {

		sorted[count] = pixels[(count >> 2) + ((count & 3) * (length >> 2))];

	}

	delete[] pixels;

	return sorted;

}


/**
 * Load a block of scrambled and masked pixel data from the file.
 *
 * @param length The length of the block
 * @param key The transparent pixel value
 *
 * @return Buffer containing the de-scrambled data
 */
unsigned char* File::loadPixels (int length, int key) {

	unsigned char* pixels;
	unsigned char* sorted;
	unsigned char mask = 0;
	int count;


	sorted = new unsigned char[length];
	pixels = new unsigned char[length];


	// Read the mask
	// Each mask pixel is either 0 or 1
	// Four pixels are packed into the lower end of each byte
	for (count = 0; count < length; count++) {

		if (!(count & 3)) mask = loadChar();
		pixels[count] = (mask >> (count & 3)) & 1;

	}

	// Pixels are loaded if the corresponding mask pixel is 1, otherwise
	// the transparent index is used. Pixels are scrambled, so the mask
	// has to be scrambled the same way.
	for (count = 0; count < length; count++) {

		sorted[(count >> 2) + ((count & 3) * (length >> 2))] = pixels[count];

	}

	// Read pixels according to the scrambled mask
	for (count = 0; count < length; count++) {

		// Use the transparent pixel
		pixels[count] = key;

		if (sorted[count] == 1) {

			// The unmasked portions are transparent, so no masked
			// portion should be transparent.
			while (pixels[count] == key) pixels[count] = loadChar();

		}

	}

	// Rearrange pixels in correct order
	for (count = 0; count < length; count++) {

		sorted[count] = pixels[(count >> 2) + ((count & 3) * (length >> 2))];

	}

	delete[] pixels;

	return sorted;

}


/**
 * Load a palette from the file.
 *
 * @param palette The palette to be filled with loaded colours
 * @param rle Whether or not the palette data is RLE-encoded
 */
void File::loadPalette (SDL_Color* palette, bool rle) {

	unsigned char* buffer;
	int count;

	if (rle) buffer = loadRLE(MAX_PALETTE_COLORS * 3);
	else buffer = loadBlock(MAX_PALETTE_COLORS * 3);

	for (count = 0; count < MAX_PALETTE_COLORS; count++) {

		// Palette entries are 6-bit
		// Shift them upwards to 8-bit, and fill in the lower 2 bits
		palette[count].r = (buffer[count * 3] << 2) + (buffer[count * 3] >> 4);
		palette[count].g = (buffer[(count * 3) + 1] << 2) + (buffer[(count * 3) + 1] >> 4);
		palette[count].b = (buffer[(count * 3) + 2] << 2) + (buffer[(count * 3) + 2] >> 4);

	}

	delete[] buffer;

}


PathMgr::PathMgr():
	paths(NULL), has_config(false), has_temp(false) {

};

PathMgr::~PathMgr() {
	delete paths;
};

bool PathMgr::add(char* newPath, int newPathType) {

#ifdef DC32_OPENJAZZ
	newPathType &= PATH_TYPE_GAME | PATH_TYPE_CONFIG;
	if (!newPathType) {
		delete[] newPath;
		return false;
	}
	if (!newPath || !strlen(newPath)) {
		delete[] newPath;
		newPath = createString("");
	}
	if (has_config)
		newPathType &= ~PATH_TYPE_CONFIG;
	if (newPathType & PATH_TYPE_CONFIG)
		has_config = true;
	if (!newPathType) {
		delete[] newPath;
		return false;
	}
	paths = new Path(paths, newPath, newPathType);
	return true;
#else
	// Check for CWD
	if(!strlen(newPath)) {
		delete[] newPath;

		char cwd[1024];
		if (getcwd(cwd, sizeof(cwd)) != NULL) {
			newPath = createString(cwd);
		} else {
			LOG_WARN("Could not get current working directory!");
			return false;
		}
	}

	// Append a directory separator if necessary
	if (newPath[strlen(newPath) - 1] != OJ_DIR_SEP) {
		char* tmp = createString(newPath, OJ_DIR_SEP_STR);
		delete[] newPath;
		newPath = tmp;
	}

	// all paths need to be readable
	if (access(newPath, R_OK) != 0) {
		LOG_TRACE("Path '%s' is not readable, ignoring!", newPath);
		delete[] newPath;
		return false;
	}

	// ignore, if already present
	if(has_config) newPathType &= ~PATH_TYPE_CONFIG;
	if(has_temp) newPathType &= ~PATH_TYPE_TEMP;

	// config and temp dir need to be writeable
	if ((newPathType & (PATH_TYPE_CONFIG|PATH_TYPE_TEMP)) > 0) {
		if (access(newPath, W_OK) != 0) {
			LOG_WARN("Path '%s' is not writeable, disabling!", newPath);

			newPathType &= ~PATH_TYPE_CONFIG;
			newPathType &= ~PATH_TYPE_TEMP;
		}
	}

	if(newPathType == PATH_TYPE_INVALID) {
		delete[] newPath;
		return false;
	}

	// we only need one directory for these
	if (newPathType & PATH_TYPE_CONFIG) has_config = true;
	if (newPathType & PATH_TYPE_TEMP) has_temp = true;

	// Finally add
	paths = new Path(paths, newPath, newPathType);

	return true;
#endif
}

#ifdef DC32_OPENJAZZ
bool File::loadPixelsInto (unsigned char* output, int length) {
	unsigned char input[256];

	if (!output || length <= 0 || (length & 3))
		return false;
	int quarter = length >> 2;
	for (int plane = 0; plane < 4; plane++) {
		int offset = 0;

		while (offset < quarter) {
			int now = quarter - offset;

			if (now > (int)sizeof(input))
				now = sizeof(input);
			if (read(input, 1, now) != now)
				return false;
			for (int i = 0; i < now; i++)
				output[((offset + i) << 2) + plane] = input[i];
			offset += now;
		}
	}
	return true;
}

bool File::loadPixelsInto (unsigned char* output, int length, int key,
	unsigned char* mask, int maskCapacity, int encodedLength) {
	unsigned char input[256];
	int inputPos = 0;
	int inputUsed = 0;
	int encodedRemaining = encodedLength;

	if (!output || !mask || length <= 0 || (length & 3) ||
			encodedLength < 0)
		return false;
	int quarter = length >> 2;
	if (quarter > maskCapacity || read(mask, 1, quarter) != quarter)
		return false;
	memset(output, key, length);
	for (int source = 0; source < length; source++) {
		int plane = source / quarter;

		if (mask[source % quarter] & (1u << plane)) {
			unsigned char value = (unsigned char)key;

			while (value == key) {
				if (inputPos == inputUsed) {
					inputUsed = encodedRemaining;
					if (inputUsed > (int)sizeof(input))
						inputUsed = sizeof(input);
					if (!inputUsed || read(input, 1, inputUsed) != inputUsed)
						return false;
					encodedRemaining -= inputUsed;
					inputPos = 0;
				}
				value = input[inputPos++];
			}
			output[(source % quarter) * 4 + plane] = value;
		}
	}
	while (inputPos < inputUsed)
		if (input[inputPos++] != key)
			return false;
	while (encodedRemaining) {
		int now = encodedRemaining;

		if (now > (int)sizeof(input))
			now = sizeof(input);
		if (read(input, 1, now) != now)
			return false;
		for (int i = 0; i < now; i++)
			if (input[i] != key)
				return false;
		encodedRemaining -= now;
	}
	return true;
}

SDL_Surface* File::loadCachedSurface (const char* cacheName, int width, int height,
	unsigned char key, bool checkSize) {
	SDL_Surface *surface;
	unsigned char output[256];
	int blockSize = 0;
	int start;
	int outputUsed = 0;
	int pos = 0;
	int length = width * height;
	bool ok = true;

	if (checkSize)
		blockSize = loadShort();
	start = tell();
	surface = dc32OjCacheFindSurface(cacheName);
	if (surface) {
		if (checkSize)
			seek(start + blockSize, true);
		else {
			while (pos < length) {
				unsigned char code = loadChar();
				unsigned char amount = code & 127;

				if (code & 128)
					(void)loadChar();
				else if (amount)
					seek(amount, false);
				else {
					(void)loadChar();
					break;
				}
				pos += amount;
			}
		}
		return surface;
	}
	if (!dc32OjCacheBeginSurface(cacheName, (uint16_t)width, (uint16_t)height,
			(uint16_t)width, key, true))
		return nullptr;
	auto flush = [&]() {
		if (outputUsed && !dc32OjCacheWriteSurface(output, (uint32_t)outputUsed))
			ok = false;
		outputUsed = 0;
	};
	auto emit = [&](unsigned char value) {
		output[outputUsed++] = value;
		pos++;
		if (outputUsed == (int)sizeof(output))
			flush();
	};
	while (ok && pos < length) {
		unsigned char code = loadChar();
		unsigned char amount = code & 127;

		if (code & 128) {
			unsigned char value = loadChar();

			if (pos + amount > length) {
				ok = false;
				break;
			}
			while (amount--)
				emit(value);
		} else if (amount) {
			if (pos + amount > length) {
				ok = false;
				break;
			}
			while (amount--) {
				emit(loadChar());
				if (!ok)
					break;
			}
		} else {
			emit(loadChar());
			break;
		}
	}
	flush();
	if (checkSize)
		seek(start + blockSize, true);
	if (!ok || pos != length) {
		dc32OjCacheCancelSurface();
		return nullptr;
	}
	return dc32OjCacheEndSurface();
}
#endif

const char *PathMgr::getTemp() {
	Path* path = gamePaths.paths;

	while (path) {

		// skip other paths
		if ((path->pathType & PATH_TYPE_TEMP) != PATH_TYPE_TEMP) {
			path = path->next;
			continue;
		}

		return path->path;

	}

	LOG_WARN("Could not find temp path");
	return "";
}

/**
 * Create a new directory path object.
 *
 * @param newNext Next path
 * @param newPath The new path
 */
Path::Path (Path* newNext, char* newPath, int newPathType) {

	char pathInfo[10] = {};
	if(newPathType & PATH_TYPE_SYSTEM) strcat(pathInfo, "S");
	if(newPathType & PATH_TYPE_CONFIG) strcat(pathInfo, "C");
	if(newPathType & PATH_TYPE_GAME) strcat(pathInfo, "G");
	if(newPathType & PATH_TYPE_TEMP) strcat(pathInfo, "T");
	if(newPathType & PATH_TYPE_ANY) strcat(pathInfo, "A");

	LOG_DEBUG("Adding '%s' to the path list [%s]", newPath, pathInfo);

	next = newNext;
	path = newPath;
	pathType = newPathType;

}


/**
 * Delete the directory path object.
 */
Path::~Path () {

	if (next) delete next;
	delete[] path;

}
