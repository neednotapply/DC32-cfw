
/**
 *
 * @file jj1bonuslevel.cpp
 *
 * Part of the OpenJazz project
 *
 * @par History:
 * - 23rd August 2005: Created bonus.c
 * - 3rd February 2009: Renamed bonus.c to bonus.cpp
 * - 1st August 2012: Renamed bonus.cpp to jj1bonuslevel.cpp
 *
 * @par Licence:
 * Copyright (c) 2005-2017 AJ Thomson
 *
 * OpenJazz is distributed under the terms of
 * the GNU General Public License, version 2.0
 *
 * @par Description:
 * Deals with the loading, running and freeing of bonus levels.
 *
 */


#include "jj1bonuslevelplayer.h"
#include "jj1bonuslevel.h"

#include "game/game.h"
#include "game/gamemode.h"
#include "io/controls.h"
#include "io/file.h"
#include "io/gfx/font.h"
#include "io/gfx/paletteeffects.h"
#include "io/gfx/sprite.h"
#include "io/gfx/video.h"
#include "io/sound.h"
#include "io/log.h"
#include "util.h"
#ifdef DC32_OPENJAZZ
#include "apps/openjazz/openjazz_cache.h"
#include "apps/openjazz/openjazz_install.h"
#include "apps/openjazz/openjazz_memory.h"
#include "apps/openjazz/openjazz_prebuild.h"
#include "apps/port/port_runtime.h"
#include <stdio.h>
static_assert(sizeof(JJ1BonusLevel) <= DC32_OJ_LEVEL_OBJECT_LIMIT,
	"DC32 bonus level state exceeds the fixed level arena");

void* JJ1BonusLevel::operator new (size_t size) {
	void* ptr = dc32OjLevelArenaAcquire(size);

	if (!ptr)
		dc32OjFatalAllocation(size);
	return ptr;
}

void JJ1BonusLevel::operator delete (void* ptr) {
	dc32OjLevelArenaRelease(ptr);
}
#endif

#ifdef DC32_OPENJAZZ
unsigned char JJ1BonusLevel::getGridTile (int x, int y) const {
	unsigned int index = ((unsigned int)(y & 255) * BLW) + (unsigned int)(x & 255);
	unsigned int bit = index * 6u;
	unsigned int byte = bit >> 3;
	unsigned int value = gridTiles[byte] | ((unsigned int)gridTiles[byte + 1u] << 8);

	return (unsigned char)((value >> (bit & 7u)) & 63u);
}

void JJ1BonusLevel::setGridTile (int x, int y, unsigned char tile) {
	unsigned int index = ((unsigned int)(y & 255) * BLW) + (unsigned int)(x & 255);
	unsigned int bit = index * 6u;
	unsigned int byte = bit >> 3;
	unsigned int shift = bit & 7u;
	unsigned int value = gridTiles[byte] | ((unsigned int)gridTiles[byte + 1u] << 8);

	value = (value & ~(63u << shift)) | (((unsigned int)tile & 63u) << shift);
	gridTiles[byte] = (unsigned char)value;
	gridTiles[byte + 1u] = (unsigned char)(value >> 8);
}

unsigned char JJ1BonusLevel::getGridEvent (int x, int y) const {
	unsigned int index = ((unsigned int)(y & 255) * BLW) + (unsigned int)(x & 255);
	unsigned char value = gridEvents[index >> 1];

	return (index & 1u) ? value >> 4 : value & 15u;
}

void JJ1BonusLevel::setGridEvent (int x, int y, unsigned char event) {
	unsigned int index = ((unsigned int)(y & 255) * BLW) + (unsigned int)(x & 255);
	unsigned char *value = gridEvents + (index >> 1);

	if (index & 1u)
		*value = (unsigned char)((*value & 15u) | ((event & 15u) << 4));
	else
		*value = (unsigned char)((*value & 0xf0u) | (event & 15u));
}

static unsigned int dc32Le16(const unsigned char* value) {
	return (unsigned int)value[0] | ((unsigned int)value[1] << 8);
}

static bool dc32BonusSpriteFailure(const char* name, const char* reason) {
	if (!dc32OjCacheLastError()[0])
		dc32OjCacheBuildFailure(name, reason);
	return false;
}

static bool dc32LoadBonusSprite(File* file, Sprite* sprite, int index,
		bool showProgress) {
	unsigned char header[8];
	unsigned char* output = dc32OjCacheDecodePixels();
	unsigned char* mask = dc32OjCacheDecodeMask();
	char cacheName[16];
	unsigned int sourceWidth;
	unsigned int width;
	unsigned int height;
	unsigned int pixelsLength;
	unsigned int maskLength;
	unsigned int decodedLength;
	unsigned int encodedLength = 0;
	int payloadStart;
	int payloadEnd;
	int fileSize;

	snprintf(cacheName, sizeof(cacheName), "BONUSSPR.%03d", index);
	if (file->read(header, 1, sizeof(header)) != (int)sizeof(header))
		return dc32BonusSpriteFailure(cacheName, "truncated header");
	sourceWidth = dc32Le16(header);
	height = dc32Le16(header + 2);
	pixelsLength = dc32Le16(header + 4);
	maskLength = dc32Le16(header + 6);
	if (sourceWidth > SW || height > SH)
		return dc32BonusSpriteFailure(cacheName, "invalid dimensions");
	if (!sourceWidth) {
		if (height != 1u || pixelsLength != 0u || maskLength != 0u)
			return dc32BonusSpriteFailure(cacheName, "malformed blank sprite");
		if (sprite->useCachedPixels(cacheName))
			return true;
		output[0] = 0;
		if (!sprite->setCachedPixels(cacheName, output, 1, 1, 0))
			return dc32BonusSpriteFailure(cacheName, "cache store failed");
		return true;
	}
	if (!height)
		return dc32BonusSpriteFailure(cacheName, "zero height");

	payloadStart = file->tell();
	fileSize = file->getSize();
	if (payloadStart < 0 || payloadStart > fileSize)
		return dc32BonusSpriteFailure(cacheName, "invalid file position");
	if (pixelsLength != 0xffffu) {
		width = sourceWidth << 2;
		decodedLength = width * height;
		encodedLength = pixelsLength << 2;
		if (decodedLength > DC32_OJ_DECODE_PIXELS_CAPACITY ||
				maskLength != decodedLength >> 2)
			return dc32BonusSpriteFailure(cacheName, "invalid mask size");
		if (maskLength > (unsigned int)(fileSize - payloadStart) ||
				encodedLength >
					(unsigned int)(fileSize - payloadStart) - maskLength)
			return dc32BonusSpriteFailure(cacheName, "truncated masked stream");
		payloadEnd = payloadStart + (int)maskLength + (int)encodedLength;
	} else {
		width = sourceWidth;
		decodedLength = width * height;
		if (decodedLength > DC32_OJ_DECODE_PIXELS_CAPACITY ||
				decodedLength > (unsigned int)(fileSize - payloadStart))
			return dc32BonusSpriteFailure(cacheName, "truncated pixel stream");
		payloadEnd = payloadStart + (int)decodedLength;
	}
	if (sprite->useCachedPixels(cacheName)) {
		file->seek(payloadEnd, true);
		return file->tell() == payloadEnd ||
			dc32BonusSpriteFailure(cacheName, "seek failed");
	}
	if (showProgress) {
		char loadingName[32];

		snprintf(loadingName, sizeof(loadingName), "DECODE %s", cacheName);
		dc32OjLoadingAsset(loadingName);
	}
	if (pixelsLength != 0xffffu) {
		if (!file->loadPixelsInto(output, (int)decodedLength, 0, mask,
				DC32_OJ_DECODE_MASK_CAPACITY, (int)encodedLength))
			return dc32BonusSpriteFailure(cacheName, "invalid masked stream");
	} else if (file->read(output, 1, (int)decodedLength) !=
			(int)decodedLength) {
		return dc32BonusSpriteFailure(cacheName, "pixel read failed");
	}
	if (file->tell() != payloadEnd)
		return dc32BonusSpriteFailure(cacheName, "payload length mismatch");
	if (!sprite->setCachedPixels(cacheName, output, (int)width, (int)height, 0))
		return dc32BonusSpriteFailure(cacheName, "cache store failed");
	return true;
}

static bool dc32PrebuildBonusSprites(void) {
	FilePtr file;

	try {
		file = std::make_unique<File>("BONUS.000", PATH_TYPE_GAME);
	} catch (int) {
		dc32OjCacheBuildFailure("BONUS.000", "open failed");
		return false;
	}
	file->seek(2, true);
	int sprites = file->loadShort(256);
	if (sprites != 51) {
		dc32OjCacheBuildFailure("BONUS.000", "expected 51 sprites");
		return false;
	}
	for (int i = 0; i < sprites; i++) {
		Sprite sprite;
		if (!dc32LoadBonusSprite(file.get(), &sprite, i, true))
			return false;
	}
	return true;
}

bool dc32OjPrebuildBonusAssets(void) {
	FilePtr file;
	Dc32OjSurfaceView view;
	const void* paletteData;
	uint32_t paletteSize;

	dc32OjLoadingStage("Caching bonus sprites");
	if (!dc32PrebuildBonusSprites() ||
			!dc32OjCacheCheckpoint("Checking bonus sprites", 1007u,
				"BONUSSPR.050"))
		return false;
	try {
		file = std::make_unique<File>("BONUSY.000", PATH_TYPE_GAME);
	} catch (int) {
		return false;
	}
	dc32OjLoadingStage("Caching bonus background");
	if (dc32OjCacheFindView("BONUSBG", &view)) {
		file->skipRLE();
	} else {
		static const unsigned int SOURCE_WIDTH = 832u;
		static const unsigned int SOURCE_HEIGHT = 32u;
		static const unsigned int CACHE_WIDTH = 512u;
		static const unsigned int CACHE_HEIGHT = 20u;
		struct BackgroundSink {
			unsigned char row[CACHE_WIDTH];
			unsigned int offset;
			unsigned int rowsWritten;
		} sink = {{0}, 0, 0};
		auto backgroundSink = [](void* user, const unsigned char* data, int length) -> bool {
			BackgroundSink* sink = static_cast<BackgroundSink*>(user);

			for (int i = 0; i < length; i++, sink->offset++) {
				unsigned int row = sink->offset / SOURCE_WIDTH;
				unsigned int column = sink->offset % SOURCE_WIDTH;

				if (row < CACHE_HEIGHT && column < sizeof(sink->row))
					sink->row[column] = data[i];
				if (row < CACHE_HEIGHT && column == SOURCE_WIDTH - 1u) {
					if (!dc32OjCacheWriteSurface(sink->row, sizeof(sink->row)))
						return false;
					sink->rowsWritten++;
				}
			}
			return true;
		};
		if (!dc32OjCacheBeginSurface("BONUSBG", CACHE_WIDTH, CACHE_HEIGHT,
				CACHE_WIDTH, 0, false)) {
			dc32OjCacheBuildFailure("BONUSBG", "cache begin failed");
			return false;
		}
		if (!file->loadRLEStream(SOURCE_WIDTH * SOURCE_HEIGHT,
				backgroundSink, &sink) ||
				sink.offset != SOURCE_WIDTH * SOURCE_HEIGHT ||
				sink.rowsWritten != CACHE_HEIGHT) {
			dc32OjCacheBuildFailure("BONUSBG", "invalid 832x32 RLE");
			dc32OjCacheCancelSurface();
			return false;
		}
		if (!dc32OjCacheEndView(&view)) {
			if (!dc32OjCacheLastError()[0])
				dc32OjCacheBuildFailure("BONUSBG", "cache finalize failed");
			return false;
		}
	}
	if (!dc32OjCacheCheckpoint("Checking bonus background", 1008u,
			"BONUSBG"))
		return false;
	dc32OjLoadingStage("Caching bonus palette");
	if (dc32OjCacheFindBlob("BONUSPAL", &paletteData, &paletteSize)) {
		if (paletteSize != sizeof(SDL_Color) * MAX_PALETTE_COLORS)
			return false;
		file->skipRLE();
	} else {
		SDL_Color colors[MAX_PALETTE_COLORS];

		file->loadPalette(colors);
		if (!dc32OjCacheStoreBlob("BONUSPAL", colors, sizeof(colors)))
			return false;
	}
	if (!dc32OjCacheCheckpoint("Checking bonus palette", 1009u,
			"BONUSPAL"))
		return false;
	dc32OjLoadingStage("Caching bonus tiles");
	if (dc32OjCacheFindView("BONUSTILES", &view)) {
		file->skipRLE();
	} else {
		SDL_Surface* tiles = file->loadCachedSurface("BONUSTILES", 32, 32 * 60, 0);

		if (!tiles)
			return false;
		video.destroySurface(tiles);
	}
	return dc32OjCacheCheckpoint("Checking bonus tiles", 1010u,
		"BONUSTILES");
}
#endif

#include <string.h>


/**
 * Load sprites.
 *
 * @return Error code
 */
int JJ1BonusLevel::loadSprites () {

	FilePtr file;
#ifndef DC32_OPENJAZZ
	unsigned char* pixels;
#endif

	try {

		file = std::make_unique<File>("BONUS.000", PATH_TYPE_GAME);

	} catch (int e) {

		return e;

	}

	file->seek(2, true);

	sprites = file->loadShort(256);
#ifdef DC32_OPENJAZZ
	if (sprites != 51) {
		dc32OjCacheBuildFailure("BONUS.000", "expected 51 sprites");
		return E_FILE;
	}
	spriteSet = spriteSetStorage;
#else
	spriteSet = new Sprite[sprites];
#endif

	for (int i = 0; i < sprites; i++) {
#ifdef DC32_OPENJAZZ
		if (!dc32LoadBonusSprite(file.get(), spriteSet + i, i, false))
			return E_FILE;
#else
		// Load dimensions
		int width = file->loadShort(SW);
		int height = file->loadShort(SH);

		int pixelsLength = file->loadShort();
		int maskLength = file->loadShort();

		// Sprites can be either masked or not masked.
		if (pixelsLength != 0xFFFF) {

			// Masked
			width <<= 2;

			int pos = file->tell() + (pixelsLength << 2) + maskLength;

			// Read scrambled, masked pixel data
			unsigned char* pixels = file->loadPixels(width * height, 0);
			spriteSet[i].setPixels(pixels, width, height, 0);

			delete[] pixels;

			file->seek(pos, true);

		} else if (width) {

			// Not masked

			// Read pixel data
			unsigned char* pixels = file->loadBlock(width * height);
			spriteSet[i].setPixels(pixels, width, height, 0);

			delete[] pixels;

		} else {

			// Zero-length sprite

			// Create blank sprite
			spriteSet[i].clearPixels();

		}
#endif
	}

	return E_NONE;

}


/**
 * Load the tileset.
 *
 * @param fileName Name of the file containing the tileset
 *
 * @return Error code
 */
int JJ1BonusLevel::loadTiles (char *fileName) {

	FilePtr file;
	unsigned char *pixels;
	unsigned char *sorted;
	int count, x, y;

	direction = 0;

	try {

		file = std::make_unique<File>(fileName, PATH_TYPE_GAME);

	} catch (int e) {

		return e;

	}

	// Load background
#ifdef DC32_OPENJAZZ
	background = dc32OjCacheFindSurface("BONUSBG");
	if (!background)
		return E_FILE;
	file->skipRLE();
#else
	pixels = file->loadRLE(832 * 20);
	sorted = new unsigned char[512 * 20];

	for (count = 0; count < 20; count++) memcpy(sorted + (count * 512), pixels + (count * 832), 512);

	background = video.createSurface(sorted, 512, 20);

	delete[] sorted;
	delete[] pixels;
#endif

	// Load palette
#ifdef DC32_OPENJAZZ
	const void* paletteData;
	uint32_t paletteSize;
	if (!dc32OjCacheFindBlob("BONUSPAL", &paletteData, &paletteSize) ||
			paletteSize != sizeof(palette))
		return E_FILE;
	memcpy(palette, paletteData, sizeof(palette));
	file->skipRLE();
#else
	file->loadPalette(palette);
#endif

	// Load tile graphics
#ifdef DC32_OPENJAZZ
	tileSet = file->loadCachedSurface("BONUSTILES", 32, 32 * 60, 0);
	if (!tileSet)
		return E_FILE;
	pixels = static_cast<unsigned char*>(tileSet->pixels);
#else
	pixels = file->loadRLE(1024 * 60);
	tileSet = video.createSurface(pixels, 32, 32 * 60);
#endif

	// Create mask
	for (count = 0; count < 60; count++) {

		memset(mask[count], 0, sizeof(mask[count]));

		for (y = 0; y < 32; y++) {

			for (x = 0; x < 32; x++) {

				if ((pixels[(count << 10) + (y << 5) + x] & 240) == 192) {
#ifdef DC32_OPENJAZZ
					mask[count][y >> 2] |= (unsigned char)(1u << (x >> 2));
#else
					mask[count][((y << 1) & 56) + ((x >> 2) & 7)] = 1;
#endif
				}

			}

		}

	}

#ifndef DC32_OPENJAZZ
	delete[] pixels;
#endif

#ifdef DC32_OPENJAZZ
	if (!dc32OjCacheIsSealed())
		return E_FILE;
#endif
	return E_NONE;

}


/**
 * Create a JJ1 bonus level.
 *
 * @param owner The current game
 * @param fileName Name of the file containing the level data.
 * @param multi Whether or not the level will be multi-player
 */
JJ1BonusLevel::JJ1BonusLevel (Game* owner, char * fileName, bool multi) : Level(owner) {

	Anim* pAnims[BPANIMS];
	unsigned short int soundRates[BSOUNDS];
	FilePtr file;
	unsigned char *buffer;
	char *string, *fileString;
	int count, x, y;

#ifdef DC32_OPENJAZZ
	memset(gridTiles, 0, sizeof(gridTiles));
	memset(gridEvents, 0, sizeof(gridEvents));
	dc32OjLoadingStage("Loading bonus font");
#endif
	try {

		font = new Font(true);

	} catch (int e) {

		throw;

	}
	#if DEBUG_FONTS
	font->saveAtlasAsBMP("bonusfont.bmp");
	#endif

	try {

		file = std::make_unique<File>(fileName, PATH_TYPE_GAME);

	} catch (int e) {

		delete font;

		throw;

	}

	// Load sprites
#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Attaching bonus sprites");
#endif
	count = loadSprites();

	if (count < 0) {

		delete font;

		throw count;

	}

	// Skip Editor data files
	file->seek(10 * 9, true);

	// Load tileset
	string = file->loadTerminatedString(8);
	fileString = createFileName(string, 0);
#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Attaching bonus graphics");
#endif
	x = loadTiles(fileString);
	delete[] string;
	delete[] fileString;

	if (x != E_NONE) throw x;

	// Skip Editor tileset files
	file->seek(9 + 13);

	// Load music
	fileString = file->loadTerminatedString(12);
	playMusic(fileString);
	delete[] fileString;


	// Load animations one record at a time.
#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Loading bonus animations");
	unsigned int frameUsed = 0;
#endif
	for (count = 0; count < BANIMS; count++) {
		unsigned char animRecord[64];

		if (file->read(animRecord, 1, sizeof(animRecord)) != (int)sizeof(animRecord))
			throw E_FILE;
		unsigned int frames = animRecord[6];
#ifdef DC32_OPENJAZZ
		if (frames > 19)
			frames = 19;
		if (frameUsed + frames > 41)
			throw E_FILE;
		animSet[count].setFrameStorage(animFrameSprites + frameUsed,
			animFrameX + frameUsed, animFrameY + frameUsed, frames);
		frameUsed += frames;
#endif
		animSet[count].setData(frames,
			animRecord[0], animRecord[1], animRecord[3], animRecord[4],
			animRecord[2], animRecord[5]);

		for (y = 0; y < (int)frames; y++) {

			// Get frame
			x = animRecord[7 + y];
			if (x > sprites){
				LOG_DEBUG("Clipping Sprite index %d > %d", x, sprites);
				x = sprites;
			}

			// Assign sprite and vertical offset
			animSet[count].setFrame(y, true);
			animSet[count].setFrameData(spriteSet + x,
				animRecord[26 + y], animRecord[45 + y]);

		}

	}


#if DEBUG_LOAD
	// Show animation names
	for (count = 0; count < BANIMS; count++) {
		char *tmpName = file->loadTerminatedString(15);
		if(strlen(tmpName)) {
			LOG_MAX("Animation id %d is named \"%s\"", count, tmpName);
		}
		delete[] tmpName;
	}
#else
	// Skip animation names
	file->seek(BANIMS * 16);
#endif


#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Loading bonus tile grid");
	struct BonusGridSink {
		JJ1BonusLevel* level;
		unsigned int offset;
		bool events;
	} tileSink = {this, 0, false};
	auto bonusGridSink = [](void* user, const unsigned char* data, int length) -> bool {
		BonusGridSink* sink = static_cast<BonusGridSink*>(user);

		for (int i = 0; i < length; i++, sink->offset++) {
			int x = sink->offset & 255u;
			int y = sink->offset >> 8;

			if (sink->events) {
				sink->level->setGridEvent(x, y, data[i]);
			} else {
				unsigned char tile = data[i] > 59 ? 59 : data[i];
				sink->level->setGridTile(x, y, tile);
			}
		}
		return true;
	};
	// Original bonus maps store 65,535 cells; the final cell is implicit zero.
	if (!file->loadRLEStream(BLW * BLH - 1, bonusGridSink, &tileSink))
		throw E_FILE;
#else
	// Load tiles
	buffer = file->loadRLE(BLW * BLH);
	for (y = 0; y < BLH; y++) {
		for (x = 0; x < BLW; x++) {
			unsigned char tile = buffer[x + (y * BLW)];
			if (tile > 59) {
				LOG_DEBUG("Clipping Tile index %d > %d", tile, 59);
				tile = 59;
			}
			grid[y][x].tile = tile;
		}
	}
	delete[] buffer;
#endif


	// Load event properties
#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Loading bonus events");
	struct BonusEventsSink {
		JJ1BonusEventType* events;
		unsigned char record[16];
		unsigned int offset;
	} eventsSink = {eventSet, {0}, 0};
	auto bonusEventsSink = [](void* user, const unsigned char* data, int length) -> bool {
		BonusEventsSink* sink = static_cast<BonusEventsSink*>(user);

		for (int i = 0; i < length; i++, sink->offset++) {
			unsigned int within = sink->offset & 15u;
			sink->record[within] = data[i];
			if (within == 15u) {
				unsigned int index = sink->offset >> 4;
				JJ1BonusEventType& event = sink->events[index];
				event.anim = sink->record[5];
				event.type = sink->record[7];
				event.passable = sink->record[8];
				event.used = sink->record[9];
				auto se = static_cast<SE::Type>(sink->record[13]);
				event.sound = isValidSoundIndex(se) ? se : SE::NONE;
				if (event.type == 7)
					event.passable = 1;
			}
		}
		return true;
	};
	if (!file->loadRLEStream(BEVENTS * 16, bonusEventsSink, &eventsSink))
		throw E_FILE;
#else
	buffer = file->loadRLE(BEVENTS * 16);
	for (count = 0; count < BEVENTS; count++) {
		eventSet[count].anim = buffer[(count * BEVENTS) + 5];
		eventSet[count].type = buffer[(count * BEVENTS) + 7];
		eventSet[count].passable = buffer[(count * BEVENTS) + 8];
		eventSet[count].used = buffer[(count * BEVENTS) + 9];

		auto se = static_cast<SE::Type>(buffer[(count * BEVENTS) + 13]);
		if (!isValidSoundIndex(se)) {
			eventSet[count].sound = SE::NONE;
			LOG_WARN("Event %d has invalid sound effect %d.", count, se);
		} else {
			eventSet[count].sound = se;
		}

		// This is a hack to make "exit signs" work in OJ
		if(eventSet[count].type == 7)
			eventSet[count].passable = 1;

#if DEBUG_LOAD
		//hexDump(nullptr, buffer + count * BEVENTS, 16);
		LOG_MAX("Event %d: anim=%d, type=%d, passable=%d, sound=%d",
			count, eventSet[count].anim, eventSet[count].type,
			eventSet[count].passable, eventSet[count].sound);
#endif
	}
	delete[] buffer;
#endif


	// Load event mapping
#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Loading bonus event grid");
	BonusGridSink eventGridSink = {this, 0, true};
	if (!file->loadRLEStream(BLW * BLH - 1, bonusGridSink, &eventGridSink))
		throw E_FILE;
#else
	buffer = file->loadRLE(BLW * BLH);

	for (y = 0; y < BLW; y++) {
		for (x = 0; x < BLH; x++) {

#ifdef DC32_OPENJAZZ
			setGridEvent(x, y, buffer[x + (y * BLW)]);
#else
			grid[y][x].event = buffer[x + (y * BLW)];
#endif

		}
	}

	delete[] buffer;
#endif


	// Load sound map
	for (count = 0; count < BSOUNDS; count++) {
		soundRates[count] = file->loadShort();
	}
	for (count = 0; count < BSOUNDS; count++) {
		char *tmpName = file->loadTerminatedString(8);
		resampleSound(count, tmpName, soundRates[count]);
		delete[] tmpName;
	}

	// Unknown marker
	if(file->loadShort() != 0xFFFF) {
		LOG_WARN("Invalid bonus level");
		throw E_FILE;
	}

	// Set the tick at which the level will end
	endTime = file->loadShort() * 1000;


	// Number of gems to collect
	items = file->loadShort();


	// The players' coordinates
	x = file->loadShort();
	y = file->loadShort();


	// Generate player's animation set references
	for (count = 0; count < BPANIMS; count++)
		pAnims[count] = animSet + count;


	createLevelPlayers(LT_JJ1BONUS, pAnims, NULL, false, x, y);


	// Palette animations

	// Spinny whirly thing
	paletteEffects = new RotatePaletteEffect(112, 16, F32, NULL);

	// Track sides
	paletteEffects = new RotatePaletteEffect(192, 16, F32, paletteEffects);

	// Bouncy things
	paletteEffects = new RotatePaletteEffect(240, 16, F32, paletteEffects);


	// Adjust panelBigFont to use bonus level palette
	panelBigFont->mapPalette(0, 32, 15, -16);


	multiplayer = multi;

	video.setTitle("Bonus Level");

}


/**
 * Delete the JJ1 bonus level.
 */
JJ1BonusLevel::~JJ1BonusLevel () {

	// Restore panelBigFont palette
	panelBigFont->restorePalette();

	video.destroySurface(tileSet);
	video.destroySurface(background);

#ifndef DC32_OPENJAZZ
	delete[] spriteSet;
#endif

	delete font;

	resampleSounds();

	video.setTitle(NULL);

}


/**
 * Determine whether or not the given point is in the event area of its tile.
 *
 * @param x X-coordinate
 * @param y Y-coordinate
 *
 * @return True if in the event area
 */
bool JJ1BonusLevel::isEvent (fixed x, fixed y) {

	return ((x & 32767) > F12) && ((x & 32767) < F20) &&
		((y & 32767) > F12) && ((y & 32767) < F20);

}


/**
 * Determine whether or not the given point is solid.
 *
 * @param x X-coordinate
 * @param y Y-coordinate
 *
 * @return Solidity
 */
bool JJ1BonusLevel::checkMask (fixed x, fixed y) {

#ifdef DC32_OPENJAZZ
	unsigned char event = getGridEvent(FTOT(x), FTOT(y));
	unsigned char tile = getGridTile(FTOT(x), FTOT(y));

	if ((eventSet[event].used) && !(eventSet[event].passable) && isEvent(x, y))
		return true;
	return (mask[tile][(y >> 12) & 7] >> ((x >> 12) & 7)) & 1;
#else
	JJ1BonusLevelGridElement *ge = grid[FTOT(y) & 255] + (FTOT(x) & 255);

	// Bounce back
	if ((eventSet[ge->event].used) && !(eventSet[ge->event].passable) && isEvent(x, y))
		return true;

	// Check the mask in the tile in question
	return mask[ge->tile][((y >> 9) & 56) + ((x >> 12) & 7)];
#endif

}


/**
 * Interpret data received from client/server
 *
 * @param buffer Received data
 */
void JJ1BonusLevel::receive (unsigned char* buffer) {

	switch (buffer[1]) {

		case MT_L_PROP:

			if (buffer[2] == 2) {

				if (stage == LS_NORMAL)
					endTime += buffer[3] * 1000;

			}

			break;

		case MT_L_GRID:

#ifdef DC32_OPENJAZZ
			if (buffer[4] == 0) setGridTile(buffer[2], buffer[3], buffer[5]);
			else if (buffer[4] == 2) setGridEvent(buffer[2], buffer[3], buffer[5]);
#else
			if (buffer[4] == 0) grid[buffer[3]][buffer[2]].tile = buffer[5];
			else if (buffer[4] == 2)
				grid[buffer[3]][buffer[2]].event = buffer[5];
#endif

			break;

		case MT_L_STAGE:

			stage = LevelStage(buffer[2]);

			break;

	}

}


/**
 * Level iteration.
 *
 * @return Error code
 */
int JJ1BonusLevel::step () {
	// Check if time has run out
	if (ticks > endTime) {
		playSound(SE::OW);

		return LOST;
	}

	// Apply controls to local player
	for (int i = 0; i < PCONTROLS; i++)
		localPlayer->setControl(i, controls.getState(i));

	// Process players
	for (int i = 0; i < nPlayers; i++) {

		JJ1BonusLevelPlayer* bonusPlayer = players[i].getJJ1BonusLevelPlayer();

		fixed playerX = bonusPlayer->getX();
		fixed playerY = bonusPlayer->getY();

		bonusPlayer->step(ticks, 16, this);

		if ((bonusPlayer->getZ() < FH) && isEvent(playerX, playerY)) {

			int gridX = FTOT(playerX) & 255;
			int gridY = FTOT(playerY) & 255;
			unsigned char event =
#ifdef DC32_OPENJAZZ
				getGridEvent(gridX, gridY);
#else
				grid[gridY][gridX].event;
#endif

			// Play sound if available
			playSound(eventSet[event].sound);

			switch (eventSet[event].type) {

				case 1: // Extra time

					addTimer(60);
#ifdef DC32_OPENJAZZ
					setGridEvent(gridX, gridY, 0);
#else
					grid[gridY][gridX].event = 0;
#endif

					break;

				case 2: // Gem

					bonusPlayer->addGem();
#ifdef DC32_OPENJAZZ
					setGridEvent(gridX, gridY, 0);
#else
					grid[gridY][gridX].event = 0;
#endif

					if (bonusPlayer->getGems() >= items) {

						players[i].addLife();

						return WON;

					}

					break;

				case 5: // Hand
				case 8:

					break;

				case 7: // Exit

					return LOST;

				default:

					// Do nothing

					break;

			}

		}

	}

	direction = localPlayer->getJJ1BonusLevelPlayer()->getDirection();

	return E_NONE;

}


/**
 * Draw the level.
 */
void JJ1BonusLevel::draw () {

	JJ1BonusLevelPlayer *bonusPlayer;
	Anim *anim;
	SDL_Rect dst;
	int x, y;

	// Draw the background

	for (x = -(direction & 1023); x < canvasW; x += background->w) {

		dst.x = x;
		dst.y = (canvasH >> 1) - 4;
		SDL_BlitSurface(background, NULL, canvas, &dst);

	}

	x = 171;

	for (y = (canvasH >> 1) - 5; (y >= 0) && (x > 128); y--)
		video.drawRect(0, y, canvasW, 1, x--);

	if (y > 0)
		video.drawRect(0, 0, canvasW, y + 1, 128);


	bonusPlayer = localPlayer->getJJ1BonusLevelPlayer();


	// Draw the ground

	fixed playerX = bonusPlayer->getX();
	fixed playerY = bonusPlayer->getY();
	fixed playerSin = fSin(direction);
	fixed playerCos = fCos(direction);

	if (SDL_MUSTLOCK(canvas)) SDL_LockSurface(canvas);

	for (y = 1; y <= (canvasH >> 1) - 15; y++) {

		fixed distance = DIV(ITOF(800), ITOF(92) - (ITOF(y * 84) / ((canvasH >> 1) - 16)));
		fixed sideX = MUL(distance, playerCos);
		fixed sideY = MUL(distance, playerSin);
		fixed fwdX = playerX + MUL(distance - F16, playerSin) - (sideX >> 1);
		fixed fwdY = playerY - MUL(distance - F16, playerCos) - (sideY >> 1);

		unsigned char* row = static_cast<unsigned char*>(canvas->pixels) + (canvas->pitch * (canvasH - y));

		for (x = 0; x < canvasW; x++) {

			fixed nX = ITOF(x) / canvasW;
			int levelX = FTOI(fwdX + MUL(nX, sideX));
			int levelY = FTOI(fwdY + MUL(nX, sideY));

			row[x] = static_cast<unsigned char*>(tileSet->pixels)
				[(
#ifdef DC32_OPENJAZZ
					getGridTile(ITOT(levelX), ITOT(levelY))
#else
					grid[ITOT(levelY) & 255][ITOT(levelX) & 255].tile
#endif
					<< 10) +
					((levelY & 31) * tileSet->pitch) + (levelX & 31)];

		}

	}

	if (SDL_MUSTLOCK(canvas)) SDL_UnlockSurface(canvas);


	// Draw nearby events

	for (y = -6; y < 6; y++) {
		int evY = (((direction - FQ) & 512)? y: -y);
		fixed sY = TTOF(evY) + F16 - (playerY & 32767);

		for (x = -6; x < 6; x++) {
			int evX = ((direction & 512)? x: -x);
			fixed sX = TTOF(evX) + F16 - (playerX & 32767);

			fixed divisor = F16 + MUL(sX, playerSin) - MUL(sY, playerCos);

			if (FTOI(divisor) > 8) {
				unsigned char event =
#ifdef DC32_OPENJAZZ
					getGridEvent(evX + FTOT(playerX), evY + FTOT(playerY));
#else
					grid[(evY + FTOT(playerY)) & 255][(evX + FTOT(playerX)) & 255].event;
#endif

				if (eventSet[event].type > 0) {
					anim = animSet + eventSet[event].anim;

					anim->setFrame(ticks / 75, true);
					fixed nX = DIV(MUL(sX, playerCos) + MUL(sY, playerSin), divisor);
					anim->drawScaled(ITOF(FTOI(nX * canvasW) + (canvasW >> 1)),
						ITOF(canvasH >> 1), DIV(F64 * canvasW / SW, divisor));
				}
			}
		}
	}


	// Show the player
	bonusPlayer->draw(ticks);


	// Show gem count
	font->showString("*", 0, 0);
	font->showNumber(bonusPlayer->getGems() / 10, 50, 0);
	font->showNumber(bonusPlayer->getGems() % 10, 68, 0);
	font->showString("/", 65, 0);
	font->showNumber(items, 124, 0);


	// Show time remaining
	if (endTime > ticks) x = (endTime - ticks) / 1000;
	else x = 0;
	font->showNumber(x / 60, 250, 0);
	font->showString(":", 247, 0);
	font->showNumber((x / 10) % 6, 274, 0);
	font->showNumber(x % 10, 291, 0);

}


/**
 * Play the level.
 *
 * @return Error code
 */
int JJ1BonusLevel::play () {

#ifdef DC32_OPENJAZZ
	dc32OjRequireGameplayHeap();
#endif
	bool pmenu, pmessage;
	int option;
	unsigned int returnTime;

	tickOffset = globalTicks;
	ticks = T_STEP;
	steps = 0;
	pmessage = pmenu = false;
	option = 0;
	returnTime = 0;

	video.setPalette(palette);

	while (true) {

		int ret = loop(pmenu, option, pmessage);
		if (ret < 0) return ret;

		// Check if level has been won
		if (returnTime && (ticks > returnTime)) {

			if (localPlayer->getJJ1BonusLevelPlayer()->getGems() >= items) {

				if (playScene("BONUS.0SC") == E_QUIT) return E_QUIT;

				return WON;

			}

			return LOST;

		}


		// Process frame-by-frame activity

		while ((getTimeChange() >= T_STEP) && (stage == LS_NORMAL)) {

			ret = step();
			steps++;
			if (ret < 0) return ret;
			else if (ret) {

				stage = LS_END;
				paletteEffects = new WhiteOutPaletteEffect(T_BONUS_END, paletteEffects);
				returnTime = ticks + T_BONUS_END;

			}

		}


		// Draw the graphics

		if ((ticks < returnTime) && !paused) direction += (ticks - prevTicks) * T_BONUS_END / (returnTime - ticks);

		draw();

		// If paused, draw "PAUSE"
		if (pmessage && !pmenu)
			font->showString("pause", (canvasW >> 1) - 44, 32);


		// Draw statistics, menu etc.
		drawOverlay(0, pmenu, option, 0, 31, 16);

	}

	return E_NONE;

}
