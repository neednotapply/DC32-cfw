
/**
 *
 * @file jj1levelload.cpp
 *
 * Part of the OpenJazz project
 *
 * @par History:
 * - 23rd August 2005: Created level.c
 * - 22nd July 2008: Created levelload.c from parts of level.c
 * - 3rd February 2009: Renamed levelload.c to levelload.cpp
 * - 18th July 2009: Created demolevel.cpp from parts of level.cpp and
 *                 levelload.cpp
 * - 19th July 2009: Added parts of levelload.cpp to level.cpp
 * - 28th June 2010: Created levelloadjj2.cpp from parts of levelload.cpp
 * - 1st August 2012: Renamed levelload.cpp to jj1levelload.cpp
 *
 * @par Licence:
 * Copyright (c) 2005-2017 AJ Thomson
 *
 * OpenJazz is distributed under the terms of
 * the GNU General Public License, version 2.0
 *
 * @par Description:
 * Deals with the loading of level data.
 *
 */


#include "jj1bullet.h"
#include "event/jj1event.h"
#include "jj1level.h"
#include "jj1levelplayer.h"

#include "game/game.h"
#include "io/file.h"
#include "io/gfx/font.h"
#include "io/gfx/sprite.h"
#include "io/gfx/video.h"
#include "io/sound.h"
#include "loop.h"
#include "util.h"
#include "io/log.h"

#include <string.h>
#ifdef DC32_OPENJAZZ
#include "apps/openjazz/openjazz_cache.h"
#include "apps/openjazz/openjazz_install.h"
#include "apps/openjazz/openjazz_memory.h"
#include "apps/openjazz/openjazz_prebuild.h"
#include <stdio.h>

struct OjGridSink {
	GridElement* grid;
	unsigned int offset;
	unsigned char pair[2];
};

static bool ojGridSink(void* user, const unsigned char* data, int length) {
	OjGridSink* sink = static_cast<OjGridSink*>(user);

	for (int i = 0; i < length; i++, sink->offset++) {
		sink->pair[sink->offset & 1u] = data[i];
		if (sink->offset & 1u) {
			unsigned int cell = sink->offset >> 1;
			unsigned int x = cell / LH;
			unsigned int y = cell % LH;
			GridElement& grid = sink->grid[y * LW + x];

			grid.tile = sink->pair[0];
			grid.bg = sink->pair[1] >> 7;
			grid.event = sink->pair[1] & 127;
			grid.hits = 0;
			grid.timeIndex = 0;
		}
	}
	return true;
}

struct OjMaskSink {
	unsigned char* mask;
	unsigned int capacity;
	unsigned int offset;
};

static bool ojMaskSink(void* user, const unsigned char* data, int length) {
	OjMaskSink* sink = static_cast<OjMaskSink*>(user);
	unsigned int remain = sink->capacity - sink->offset;

	if ((unsigned int)length > remain)
		length = (int)remain;
	memcpy(sink->mask + sink->offset, data, length);
	sink->offset += (unsigned int)length;
	return true;
}

struct OjPathLengthSink {
	unsigned short lengths[PATHS];
	unsigned int offset;
};

static bool ojPathLengthSink(void* user, const unsigned char* data, int length) {
	OjPathLengthSink* sink = static_cast<OjPathLengthSink*>(user);

	for (int i = 0; i < length; i++, sink->offset++) {
		unsigned int within = sink->offset & 511u;
		unsigned int path = sink->offset >> 9;

		if (path < PATHS && within < 2u) {
			if (!within)
				sink->lengths[path] = data[i];
			else
				sink->lengths[path] |= (unsigned short)data[i] << 8;
		}
	}
	return true;
}

struct OjPathDataSink {
	JJ1EventPath* paths;
	unsigned int offset;
};

static bool ojPathDataSink(void* user, const unsigned char* data, int length) {
	OjPathDataSink* sink = static_cast<OjPathDataSink*>(user);

	for (int i = 0; i < length; i++, sink->offset++) {
		unsigned int within = sink->offset & 511u;
		unsigned int path = sink->offset >> 9;

		if (path >= PATHS || within < 2u)
			continue;
		unsigned int point = (within - 2u) >> 1;
		if (point >= sink->paths[path].length)
			continue;
		if (within & 1u)
			sink->paths[path].x[point] = (signed char)data[i] << 2;
		else
			sink->paths[path].y[point] = (signed char)data[i];
	}
	return true;
}

struct OjRecordSink {
	unsigned char record[64];
	unsigned int recordSize;
	unsigned int offset;
	void (*consume)(void* target, unsigned int index, const unsigned char* record);
	void* target;
};

static bool ojRecordSink(void* user, const unsigned char* data, int length) {
	OjRecordSink* sink = static_cast<OjRecordSink*>(user);

	for (int i = 0; i < length; i++, sink->offset++) {
		unsigned int within = sink->offset % sink->recordSize;

		sink->record[within] = data[i];
		if (within + 1u == sink->recordSize)
			sink->consume(sink->target, sink->offset / sink->recordSize, sink->record);
	}
	return true;
}

static void ojConsumeEvent(void* target, unsigned int i, const unsigned char* buffer) {
	JJ1EventType* eventSet = static_cast<JJ1EventType*>(target);
	JJ1EventType& event = eventSet[i];

	event.difficulty = static_cast<difficultyType>(buffer[0]);
	event.reflection = buffer[2];
	event.movement = buffer[4];
	event.anims[E_LEFTANIM] = buffer[5];
	event.anims[E_RIGHTANIM] = buffer[6];
	event.magnitude = buffer[8];
	event.strength = buffer[9];
	event.modifier = buffer[10];
	event.points = buffer[11];
	event.bullet = buffer[12];
	event.bulletPeriod = buffer[13];
	event.speed = buffer[15] + 1;
	event.animSpeed = buffer[17] + 1;
	auto se = static_cast<SE::Type>(buffer[21]);
	event.sound = isValidSoundIndex(se) ? se : SE::NONE;
	event.multiA = buffer[22];
	event.multiB = buffer[23];
	event.pieceSize = buffer[24];
	event.pieces = buffer[25];
	event.angle = buffer[26];
	event.anims[E_LFINISHANIM] = buffer[28];
	event.anims[E_RFINISHANIM] = buffer[29];
	event.anims[E_LSHOOTANIM] = buffer[30];
	event.anims[E_RSHOOTANIM] = buffer[31];
}

struct OjAnimTarget {
	Anim* animSet;
	Sprite* spriteSet;
	int sprites;
	Sprite** frameSprites;
	signed char* frameX;
	signed char* frameY;
	unsigned int frameUsed;
	unsigned int frameCapacity;
	bool valid;
};

static void ojConsumeAnim(void* target, unsigned int i, const unsigned char* buffer) {
	OjAnimTarget* anims = static_cast<OjAnimTarget*>(target);
	Anim& anim = anims->animSet[i];
	unsigned int frames = buffer[6] > 19 ? 19 : buffer[6];

	if (anims->frameUsed + frames > anims->frameCapacity) {
		anims->valid = false;
		return;
	}
	anim.setFrameStorage(anims->frameSprites + anims->frameUsed,
		anims->frameX + anims->frameUsed, anims->frameY + anims->frameUsed,
		frames);
	anims->frameUsed += frames;
	anim.setData(frames, buffer[0], buffer[1], buffer[3], buffer[4],
		buffer[2], buffer[5]);
	for (unsigned int y = 0; y < frames; y++) {
		int sprite = buffer[7 + y];
		if (sprite > anims->sprites)
			sprite = anims->sprites;
		anim.setFrame(y, true);
		anim.setFrameData(anims->spriteSet + sprite, buffer[26 + y], buffer[45 + y]);
	}
}

struct OjByteSink {
	unsigned char* output;
	unsigned int capacity;
	unsigned int offset;
};

static bool ojByteSink(void* user, const unsigned char* data, int length) {
	OjByteSink* sink = static_cast<OjByteSink*>(user);

	if (sink->offset + (unsigned int)length > sink->capacity)
		return false;
	memcpy(sink->output + sink->offset, data, length);
	sink->offset += (unsigned int)length;
	return true;
}

struct OjPlayerAnimSink {
	char* refs;
	Anim** anims;
	Anim* animSet;
	unsigned int offset;
	bool valid;
};

static bool ojPlayerAnimSink(void* user, const unsigned char* data, int length) {
	OjPlayerAnimSink* sink = static_cast<OjPlayerAnimSink*>(user);

	for (int i = 0; i < length; i++, sink->offset++) {
		if (!(sink->offset & 1u) && (sink->offset >> 1) < JJ1PANIMS) {
			unsigned int index = sink->offset >> 1;

			if (data[i] >= ANIMS) {
				sink->valid = false;
				continue;
			}
			sink->refs[index] = data[i];
			sink->anims[index] = sink->animSet + data[i];
		}
	}
	return true;
}
#endif


#define SKEY 254 /* Sprite colour key */


/**
 * Load the HUD graphical data.
 *
 * @return Error code
 */
int JJ1Level::loadPanel () {

	FilePtr file;
#ifdef DC32_OPENJAZZ
	SDL_Surface* rawPanel;
	const unsigned char* pixels;
	bool complete;
	bool buildPanelBG[2];
#else
	unsigned char* pixels;
#endif
	try {

		file = std::make_unique<File>("PANEL.000", PATH_TYPE_GAME);

	} catch (int e) {

		return e;

	}

#ifdef DC32_OPENJAZZ
	panel = dc32OjCacheFindSurface("PANELHUD");
	complete = panel != nullptr;
	for (int type = 0; type < 6; type++) {
		char cacheName[16];

		snprintf(cacheName, sizeof(cacheName), "PANELAMMO%d", type);
		panelAmmo[type] = dc32OjCacheFindSurface(cacheName);
		complete = complete && panelAmmo[type] != nullptr;
	}
	panelBG[0] = dc32OjCacheFindSurface("PANELBG0");
	panelBG[1] = dc32OjCacheFindSurface("PANELBG1");
	buildPanelBG[0] = panelBG[0] == nullptr;
	buildPanelBG[1] = panelBG[1] == nullptr;
	complete = complete && panelBG[0] != nullptr && panelBG[1] != nullptr;
	if (complete)
		return E_NONE;

	/*
	 * PANEL.000 expands to 46 KB, but the cart heap only needs the final
	 * 10 KB HUD and small ammo/border surfaces. Stream the composite decode
	 * into XIP and derive one missing SRAM surface at a time.
	 */
	rawPanel = file->loadCachedSurface("PANELRAW", 1, 46272, 0);
	if (!rawPanel)
		return E_FILE;
	pixels = static_cast<const unsigned char*>(rawPanel->pixels);
#else
	pixels = file->loadRLE(46272);
#endif

	// Create the panel background
#ifdef DC32_OPENJAZZ
	if (!panel) {
#endif
		panel = video.createSurface(pixels, SW, TTOI(1));
#ifdef DC32_OPENJAZZ
		if (!dc32OjCachePromoteSurfaceChecked("PANELHUD", &panel)) {
			video.destroySurface(rawPanel);
			return E_FILE;
		}
	}
#endif


	// De-scramble the panel's ammo graphics
	unsigned char* sorted = new unsigned char[64 * 26];

	for (int type = 0; type < 6; type++) {
#ifdef DC32_OPENJAZZ
		if (panelAmmo[type])
			continue;
#endif
		for (int y = 0; y < 26; y++) {
			for (int x = 0; x < 64; x++)
				sorted[(y * 64) + x] = pixels[(type * 64 * 32) + (y * 64) + (x >> 2) + ((x & 3) << 4) + (55 * 320)];
		}
		panelAmmo[type] = video.createSurface(sorted, 64, 26);
#ifdef DC32_OPENJAZZ
		char cacheName[16];

		snprintf(cacheName, sizeof(cacheName), "PANELAMMO%d", type);
		if (!dc32OjCachePromoteSurfaceChecked(
				cacheName, &panelAmmo[type])) {
			delete[] sorted;
			video.destroySurface(rawPanel);
			return E_FILE;
		}
#endif
	}

	delete[] sorted;
#ifdef DC32_OPENJAZZ
	video.destroySurface(rawPanel);
#else
	delete[] pixels;
#endif

	// Create the panel borders in FPS HUD mode
#ifdef DC32_OPENJAZZ
	if (!panelBG[0])
		panelBG[0] = video.createSurface(nullptr, TTOI(1), TTOI(1));
	if (!panelBG[1])
		panelBG[1] = video.createSurface(nullptr, TTOI(1), TTOI(1));
#else
	panelBG[0] = video.createSurface(nullptr, TTOI(1), TTOI(1));
	panelBG[1] = video.createSurface(nullptr, TTOI(1), TTOI(1));
#endif

	constexpr int halfTile = (TTOI(1) >> 1);

	// Copy parts of the panel
	SDL_Rect src = { 176, 0, halfTile, halfTile };
	SDL_Rect dst;

#ifdef DC32_OPENJAZZ
	if (buildPanelBG[0]) {
#else
	{
#endif
		// left side gets shiny part
		dst.x = dst.y = 0; // first row
		SDL_BlitSurface(panel, &src, panelBG[0], &dst);
		dst.y = halfTile; // second row
		SDL_BlitSurface(panel, &src, panelBG[0], &dst);

		// add mirrored copy
		src.y = 0;
		src.w = 1;
		src.h = TTOI(1);
		dst.y = 0;
		for (int x = 0; x < halfTile; x++) {
			src.x = x;
			dst.x = TTOI(1) - x - 1;
			SDL_BlitSurface(panelBG[0], &src, panelBG[0], &dst);
		}
#ifdef DC32_OPENJAZZ
		if (!dc32OjCachePromoteSurfaceChecked("PANELBG0", &panelBG[0]))
			return E_FILE;
#endif
	}

#ifdef DC32_OPENJAZZ
	if (buildPanelBG[1]) {
#else
	{
#endif
		// right side is only metal
		src.x = 199;
		src.w = 9;
		src.h = 12;
		for (int x = 0; x < TTOI(1); x += 9) {
			src.y = 0;
			dst.x = x;

			dst.y = 0; // first row
			SDL_BlitSurface(panel, &src, panelBG[1], &dst);
			src.y = 2; // second row
			dst.y = 12;
			SDL_BlitSurface(panel, &src, panelBG[1], &dst);
			src.y = 8; // third row
			dst.y = 24;
			SDL_BlitSurface(panel, &src, panelBG[1], &dst);
		}
#ifdef DC32_OPENJAZZ
		if (!dc32OjCachePromoteSurfaceChecked("PANELBG1", &panelBG[1]))
			return E_FILE;
#endif
	}

#ifdef DC32_OPENJAZZ
	(void)dc32OjCacheCommit();
#endif
	return E_NONE;
}


/**
 * Load a sprite.
 *
 * @param file File from which to load the sprite data
 * @param sprite Sprite that will receive the loaded data
 */
void JJ1Level::loadSprite (File* file, Sprite* sprite) {

	unsigned char* pixels;
	int pos, maskOffset;
	int width, height;

	// Load dimensions
	width = file->loadShort() << 2;
	height = file->loadShort();

	file->seek(2, false);

	maskOffset = file->loadShort();

	pos = file->loadShort() << 2;

	// Sprites can be either masked or not masked.
	if (maskOffset) {

		// Masked

		height++;

		// Skip to mask
		file->seek(maskOffset, false);

		// Find the end of the data
		pos += file->tell() + ((width >> 2) * height);

		// Read scrambled, masked pixel data
		pixels = file->loadPixels(width * height, SKEY);
		sprite->setPixels(pixels, width, height, SKEY);

		delete[] pixels;

		file->seek(pos, true);

	} else if (width) {

		// Not masked

		// Read scrambled pixel data
		pixels = file->loadPixels(width * height);
		sprite->setPixels(pixels, width, height, SKEY);

		delete[] pixels;

	}

}

#ifdef DC32_OPENJAZZ
static bool dc32SpriteMarker (File* file) {
	if (file->loadChar() == 0xFF) {
		file->seek(1, false);
		return false;
	}
	file->seek(-1, false);
	return true;
}

static bool dc32SkipSprite (File* file) {
	int width = file->loadShort() << 2;
	int height = file->loadShort();

	file->seek(2, false);
	int maskOffset = file->loadShort();
	int pixelOffset = file->loadShort() << 2;
	if (maskOffset) {
		height++;
		file->seek(maskOffset, false);
		int end = pixelOffset + file->tell() + ((width >> 2) * height);

		file->seek(end, true);
	} else if (width) {
		file->seek(width * height, false);
	}
	return true;
}

static bool dc32LoadSprite (File* file, Sprite* sprite, const char* cacheName) {
	unsigned char* output = dc32OjCacheDecodePixels();
	unsigned char* mask = dc32OjCacheDecodeMask();
	int width = file->loadShort() << 2;
	int height = file->loadShort();
	int length;

	file->seek(2, false);
	int maskOffset = file->loadShort();
	int pixelOffset = file->loadShort() << 2;
	if (maskOffset) {
		height++;
		length = width * height;
		if (length <= 0 || length > (int)DC32_OJ_DECODE_PIXELS_CAPACITY)
			return false;
		file->seek(maskOffset, false);
		int end = pixelOffset + file->tell() + ((width >> 2) * height);

		if (!file->loadPixelsInto(output, length, SKEY, mask,
				DC32_OJ_DECODE_MASK_CAPACITY, pixelOffset))
			return false;
		file->seek(end, true);
		return sprite->setCachedPixels(cacheName, output, width, height, SKEY);
	}
	if (!width) {
		output[0] = 0;
		return sprite->setCachedPixels(cacheName, output, 1, 1, SKEY);
	}
	length = width * height;
	if (length <= 0 || length > (int)DC32_OJ_DECODE_PIXELS_CAPACITY ||
			!file->loadPixelsInto(output, length))
		return false;
	return sprite->setCachedPixels(cacheName, output, width, height, SKEY);
}

static bool dc32BlankSprite (Sprite* sprite, const char* cacheName) {
	unsigned char* output = dc32OjCacheDecodePixels();

	output[0] = 0;
	return sprite->setCachedPixels(cacheName, output, 1, 1, 0);
}

static bool dc32PrebuildSpriteSet(const char* fileName) {
	FilePtr specFile;
	FilePtr mainFile;

	try {
		specFile = std::make_unique<File>(fileName, PATH_TYPE_GAME);
		mainFile = std::make_unique<File>("MAINCHAR.000", PATH_TYPE_GAME);
	} catch (int) {
		return false;
	}
	int count = specFile->loadShort(256);
	if (count != 231)
		return false;
	specFile->seek(count * 2, false);
	mainFile->seek(2, true);
	for (int i = 0; i < count; i++) {
		char cacheName[16];
		Sprite sprite;
		bool mainHasSprite = dc32SpriteMarker(mainFile.get());
		bool specHasSprite = dc32SpriteMarker(specFile.get());

		snprintf(cacheName, sizeof(cacheName), "S%.3s.%03d",
			fileName + strlen(fileName) - 3, i);
		if (sprite.useCachedPixels(cacheName)) {
			if ((mainHasSprite && !dc32SkipSprite(mainFile.get())) ||
					(specHasSprite && !dc32SkipSprite(specFile.get())))
				return false;
		} else if (specHasSprite) {
			if ((mainHasSprite && !dc32SkipSprite(mainFile.get())) ||
					!dc32LoadSprite(specFile.get(), &sprite, cacheName))
				return false;
		} else if (mainHasSprite) {
			if (!dc32LoadSprite(mainFile.get(), &sprite, cacheName))
				return false;
		} else if (!dc32BlankSprite(&sprite, cacheName)) {
			return false;
		}
		if (specFile->tell() >= specFile->getSize()) {
			for (int j = i + 1; j < count; j++) {
				snprintf(cacheName, sizeof(cacheName), "S%.3s.%03d",
					fileName + strlen(fileName) - 3, j);
				if (!sprite.useCachedPixels(cacheName) &&
						!dc32BlankSprite(&sprite, cacheName))
					return false;
			}
			break;
		}
	}
	char cacheName[16];
	Sprite blank;
	snprintf(cacheName, sizeof(cacheName), "S%.3s.%03d",
		fileName + strlen(fileName) - 3, count);
	return blank.useCachedPixels(cacheName) ||
		dc32BlankSprite(&blank, cacheName);
}

static bool dc32PrebuildBlockSet(const char* fileName) {
	FilePtr file;
	SDL_Color colors[MAX_PALETTE_COLORS];
	const void* cached;
	uint32_t cachedSize;
	char paletteName[16];

	try {
		file = std::make_unique<File>(fileName, PATH_TYPE_GAME);
	} catch (int) {
		return false;
	}
	snprintf(paletteName, sizeof(paletteName), "PAL%.3sA",
		fileName + strlen(fileName) - 3);
	if (dc32OjCacheFindBlob(paletteName, &cached, &cachedSize)) {
		if (cachedSize != sizeof(colors))
			return false;
		file->skipRLE();
	} else {
		file->loadPalette(colors);
		if (!dc32OjCacheStoreBlob(paletteName, colors, sizeof(colors)))
			return false;
	}
	paletteName[6] = 'B';
	if (dc32OjCacheFindBlob(paletteName, &cached, &cachedSize)) {
		if (cachedSize != sizeof(colors))
			return false;
		file->skipRLE();
	} else {
		file->loadPalette(colors);
		if (!dc32OjCacheStoreBlob(paletteName, colors, sizeof(colors)))
			return false;
	}
	file->skipRLE();
	Dc32OjSurfaceView cachedTiles;
	if (dc32OjCacheFindView(fileName, &cachedTiles))
		return cachedTiles.width == TTOI(1);
	int tileDataStart = file->tell();
	int tiles = 0;
	for (int i = 0; i < TSETS; i++) {
		char marker[2];

		if (file->read(marker, 1, sizeof(marker)) != (int)sizeof(marker))
			return false;
		if (!memcmp(marker, "ok", 2)) {
			for (int j = 0; j < TNUM; j++)
				file->skipRLE();
			tiles += TNUM;
		} else if (memcmp(marker, "  ", 2)) {
			return false;
		}
	}
	if (tiles <= 0 || file->tell() != file->getSize())
		return false;
	file->seek(tileDataStart, true);
	if (!dc32OjCacheBeginSurface(fileName, TTOI(1), TTOI(tiles),
			TTOI(1), TKEY, true))
		return false;
	for (int i = 0; i < TSETS; i++) {
		char marker[2];

		if (file->read(marker, 1, sizeof(marker)) != (int)sizeof(marker)) {
			dc32OjCacheCancelSurface();
			return false;
		}
		if (!memcmp(marker, "ok", 2)) {
			for (int j = 0; j < TNUM; j++) {
				unsigned char pixels[TTOI(1) * TTOI(1)];

				file->loadRLEInto(pixels, sizeof(pixels));
				if (!dc32OjCacheWriteSurface(pixels, sizeof(pixels))) {
					dc32OjCacheCancelSurface();
					return false;
				}
			}
		}
	}
	return dc32OjCacheEndView(&cachedTiles);
}

static bool dc32PrebuildPanel(void) {
	FilePtr file;
	SDL_Surface* raw = NULL;
	SDL_Surface* panel = NULL;
	Dc32OjSurfaceView view;
	unsigned char* sorted = dc32OjCacheDecodePixels();
	bool ok = false;

	try {
		file = std::make_unique<File>("PANEL.000", PATH_TYPE_GAME);
	} catch (int) {
		return false;
	}
	raw = file->loadCachedSurface("PANELRAW", 1, 46272, 0);
	if (!raw)
		goto cleanup;
	panel = video.createSurface(
		static_cast<const unsigned char*>(raw->pixels), SW, TTOI(1));
	if (!panel)
		goto cleanup;
	if (!dc32OjCacheFindView("PANELHUD", &view) &&
			!dc32OjCacheStoreView("PANELHUD", panel->pixels, SW, TTOI(1), SW,
				0, false, &view))
		goto cleanup;
	for (int type = 0; type < 6; type++) {
		char name[16];

		snprintf(name, sizeof(name), "PANELAMMO%d", type);
		if (dc32OjCacheFindView(name, &view))
			continue;
		for (int y = 0; y < 26; y++)
			for (int x = 0; x < 64; x++)
				sorted[y * 64 + x] =
					((const unsigned char*)raw->pixels)[
						type * 64 * 32 + y * 64 + (x >> 2) +
						((x & 3) << 4) + 55 * 320];
		if (!dc32OjCacheStoreView(name, sorted, 64, 26, 64, 0, false, &view))
			goto cleanup;
	}
	for (int side = 0; side < 2; side++) {
		char name[16];

		snprintf(name, sizeof(name), "PANELBG%d", side);
		if (dc32OjCacheFindView(name, &view))
			continue;
		SDL_Surface* bg = video.createSurface(nullptr, TTOI(1), TTOI(1));
		if (!bg)
			goto cleanup;
		SDL_Rect src = {side ? 199 : 176, 0, side ? 9 : 16, side ? 12 : 16};
		SDL_Rect dst = {};
		if (!side) {
			SDL_BlitSurface(panel, &src, bg, &dst);
			dst.y = 16;
			SDL_BlitSurface(panel, &src, bg, &dst);
			src.y = 0;
			src.w = 1;
			src.h = 32;
			dst.y = 0;
			for (int x = 0; x < 16; x++) {
				src.x = x;
				dst.x = 31 - x;
				SDL_BlitSurface(bg, &src, bg, &dst);
			}
		} else {
			for (int x = 0; x < 32; x += 9) {
				src.y = 0;
				dst.x = x;
				dst.y = 0;
				SDL_BlitSurface(panel, &src, bg, &dst);
				src.y = 2;
				dst.y = 12;
				SDL_BlitSurface(panel, &src, bg, &dst);
				src.y = 8;
				dst.y = 24;
				SDL_BlitSurface(panel, &src, bg, &dst);
			}
		}
		if (!dc32OjCacheStoreView(name, bg->pixels, 32, 32, bg->pitch,
				0, false, &view)) {
			video.destroySurface(bg);
			goto cleanup;
		}
		video.destroySurface(bg);
	}
	ok = true;

cleanup:
	video.destroySurface(panel);
	video.destroySurface(raw);
	return ok;
}

bool dc32OjPrebuildNormalAssets(void) {
	static const char* const blocks[] = {
		"BLOCKS.000", "BLOCKS.001", "BLOCKS.002"
	};
	static const uint32_t blockCounts[] = {22u, 25u, 28u};
	static const char* const sprites[] = {
		"SPRITES.000", "SPRITES.001", "SPRITES.002", "SPRITES.018"
	};
	static const char* const spriteLast[] = {
		"S000.231", "S001.231", "S002.231", "S018.231"
	};
	static const uint32_t spriteCounts[] = {260u, 492u, 724u, 956u};

	dc32OjLoadingStage("Caching HUD graphics");
	if (!dc32PrebuildPanel() ||
			!dc32OjCacheCheckpoint("Checking HUD cache", 19u, "PANELBG1"))
		return false;
	for (unsigned int i = 0; i < sizeof(blocks) / sizeof(blocks[0]); i++) {
		dc32OjLoadingStage(blocks[i]);
		if (!dc32PrebuildBlockSet(blocks[i]) ||
				!dc32OjCacheCheckpoint("Checking block cache",
					blockCounts[i], blocks[i]))
			return false;
	}
	for (unsigned int i = 0; i < sizeof(sprites) / sizeof(sprites[0]); i++) {
		dc32OjLoadingStage(sprites[i]);
		if (!dc32PrebuildSpriteSet(sprites[i]) ||
				!dc32OjCacheCheckpoint("Checking sprite cache",
					spriteCounts[i], spriteLast[i]))
			return false;
	}
	return true;
}
#endif


/**
 * Load sprites.
 *
 * @param fileName Name of the file containing the level-specific sprites
 *
 * @return Error code
 */
int JJ1Level::loadSprites (char * fileName) {

	// Open fileName
	FilePtr specFile;
	try {

		specFile = std::make_unique<File>(fileName, PATH_TYPE_GAME);

	} catch (int e) {

		return e;

	}


	// This function loads all the sprites, not just those in fileName
	FilePtr mainFile;
	try {

		mainFile = std::make_unique<File>("MAINCHAR.000", PATH_TYPE_GAME);

	} catch (int e) {

		return e;

	}


	sprites = specFile->loadShort(256);

	// Include space in the sprite set for the blank sprite at the end
#ifdef DC32_OPENJAZZ
	if (sprites != 231)
		return E_FILE;
	spriteSet = spriteSetStorage;
#else
	spriteSet = new Sprite[sprites + 1];
#endif


	// Read offsets
	unsigned char* buffer = specFile->loadBlock(sprites * 2);

	for (int i = 0; i < sprites; i++)
		spriteSet[i].setOffset(buffer[i] << 2, buffer[sprites + i]);

	delete[] buffer;


	// Skip to where the sprites start in mainchar.000
	mainFile->seek(2, true);

#ifdef DC32_OPENJAZZ
	/*
	 * Decode the final composed sprite directly into the fixed cache workspace.
	 * This avoids hundreds of variable-sized SRAM pixel allocations, while the
	 * cache-hit path advances both source files without decoding anything.
	 */
	for (int i = 0; i < sprites; i++) {
		char cacheName[16];
		bool mainHasSprite = dc32SpriteMarker(mainFile.get());
		bool specHasSprite = dc32SpriteMarker(specFile.get());

		snprintf(cacheName, sizeof(cacheName), "S%.3s.%03d",
			fileName + strlen(fileName) - 3, i);
		if (spriteSet[i].useCachedPixels(cacheName)) {
			if ((mainHasSprite && !dc32SkipSprite(mainFile.get())) ||
					(specHasSprite && !dc32SkipSprite(specFile.get())))
				return E_FILE;
		} else if (specHasSprite) {
			if ((mainHasSprite && !dc32SkipSprite(mainFile.get())) ||
					!dc32LoadSprite(specFile.get(), spriteSet + i, cacheName))
				return E_FILE;
		} else if (mainHasSprite) {
			if (!dc32LoadSprite(mainFile.get(), spriteSet + i, cacheName))
				return E_FILE;
		} else if (!dc32BlankSprite(spriteSet + i, cacheName)) {
			return E_FILE;
		}
		if (specFile->tell() >= specFile->getSize()) {
			for (int j = i + 1; j < sprites; j++) {
				snprintf(cacheName, sizeof(cacheName), "S%.3s.%03d",
					fileName + strlen(fileName) - 3, j);
				if (!spriteSet[j].useCachedPixels(cacheName) &&
						!dc32BlankSprite(spriteSet + j, cacheName))
					return E_FILE;
			}
			break;
		}
	}
	{
		char cacheName[16];

		snprintf(cacheName, sizeof(cacheName), "S%.3s.%03d",
			fileName + strlen(fileName) - 3, sprites);
		if (!spriteSet[sprites].useCachedPixels(cacheName) &&
				!dc32BlankSprite(spriteSet + sprites, cacheName))
			return E_FILE;
	}
#else
	// Loop through all the sprites to be loaded
	for (int i = 0; i < sprites; i++) {

		bool loaded = false;

		if (mainFile->loadChar() == 0xFF) {

			// Go to the next sprite/file indicator
			mainFile->seek(1, false);

		} else {

			// Return to the start of the sprite
			mainFile->seek(-1, false);

			// Load the individual sprite data
			loadSprite(mainFile.get(), spriteSet + i);

			loaded = true;

		}

		if (specFile->loadChar() == 0xFF) {

			// Go to the next sprite/file indicator
			specFile->seek(1, false);

		} else {

			// Return to the start of the sprite
			specFile->seek(-1, false);

			// Load the individual sprite data
			loadSprite(specFile.get(), spriteSet + i);

			loaded = true;

		}

		/* If both fileName and mainchar.000 have file indicators, create a
		blank sprite */
		if (!loaded) spriteSet[i].clearPixels();

#ifdef DC32_OPENJAZZ
		{
			char cacheName[16];

			snprintf(cacheName, sizeof(cacheName), "S%.3s.%03d",
				fileName + strlen(fileName) - 3, i);
			spriteSet[i].cachePixels(cacheName);
		}
#endif


		// Check if the next sprite exists
		// If not, create blank sprites for the remainder
		if (specFile->tell() >= specFile->getSize()) {

			for (i++; i < sprites; i++) {

				spriteSet[i].clearPixels();
#ifdef DC32_OPENJAZZ
				char cacheName[16];

				snprintf(cacheName, sizeof(cacheName), "S%.3s.%03d",
					fileName + strlen(fileName) - 3, i);
				spriteSet[i].cachePixels(cacheName);
#endif

			}

		}

	}

	// Include a blank sprite at the end
	spriteSet[sprites].clearPixels();
#ifdef DC32_OPENJAZZ
	{
		char cacheName[16];

		snprintf(cacheName, sizeof(cacheName), "S%.3s.%03d",
			fileName + strlen(fileName) - 3, sprites);
		spriteSet[sprites].cachePixels(cacheName);
	}
#endif
#endif

	return E_NONE;

}


/**
 * Load the tileset.
 *
 * @param fileName Name of the file containing the tileset
 *
 * @return The number of tiles loaded
 */
int JJ1Level::loadTiles (char* fileName) {

	FilePtr file;

	try {

		file = std::make_unique<File>(fileName, PATH_TYPE_GAME);

	} catch (int e) {

		return e;

	}

	// Load the palette
#ifdef DC32_OPENJAZZ
	const void* paletteData;
	uint32_t paletteSize;
	char paletteName[16];

	snprintf(paletteName, sizeof(paletteName), "PAL%.3sA",
		fileName + strlen(fileName) - 3);
	if (!dc32OjCacheFindBlob(paletteName, &paletteData, &paletteSize) ||
			paletteSize != sizeof(palette))
		return E_FILE;
	memcpy(palette, paletteData, sizeof(palette));
	file->skipRLE();
	paletteName[6] = 'B';
	if (!dc32OjCacheFindBlob(paletteName, &paletteData, &paletteSize) ||
			paletteSize != sizeof(skyPalette))
		return E_FILE;
	memcpy(skyPalette, paletteData, sizeof(skyPalette));
	file->skipRLE();
#else
	file->loadPalette(palette);

	// Load the background palette
	file->loadPalette(skyPalette);
#endif

	/* Skip the second, sometimes identical, background palette
	   FIXME: These are actually alternating, needs rewritten `SkyPaletteEffect` */
	file->skipRLE();

	// Load the tile pixel indices
	int tiles = 0;
#ifdef DC32_OPENJAZZ
	int tileDataStart = file->tell();
	SDL_Surface *cachedTiles = dc32OjCacheFindSurface(fileName);

	for (int i = 0; i < TSETS; i++) {
		char marker[3] = {};

		file->read(marker, 1, 2);
		if (!strncmp(marker, "ok", 2)) {
			for (int j = 0; j < TNUM; j++)
				file->skipRLE();
			tiles += TNUM;
		} else if (strncmp(marker, "  ", 2)) {
			return E_FILE;
		}
	}
	if (file->getSize() != file->tell())
		return E_FILE;
	if (cachedTiles) {
		if (cachedTiles->w != TTOI(1) || cachedTiles->h != TTOI(tiles)) {
			video.destroySurface(cachedTiles);
			return E_FILE;
		}
		tileSet = cachedTiles;
		return tiles;
	}
	file->seek(tileDataStart, true);
	if (!dc32OjCacheBeginSurface(fileName, TTOI(1), TTOI(tiles),
			TTOI(1), TKEY, true))
		return E_FILE;
	for (int i = 0; i < TSETS; i++) {
		char marker[3] = {};

		file->read(marker, 1, 2);
		if (!strncmp(marker, "ok", 2)) {
			for (int j = 0; j < TNUM; j++) {
				unsigned char pixels[TTOI(1) * TTOI(1)];

				file->loadRLEInto(pixels, sizeof(pixels));
				if (!dc32OjCacheWriteSurface(pixels, sizeof(pixels))) {
					dc32OjCacheCancelSurface();
					return E_FILE;
				}
			}
		}
	}
	tileSet = dc32OjCacheEndSurface();
	if (!tileSet)
		return E_FILE;
	return tiles;
#else
	unsigned char* pixels[TSETS * TNUM] = {0};

	for (int i = 0; i < TSETS; i++) {
		// Check if this tileset is enabled
		char * marker = file->loadString(2);
		if(strncmp(marker, "ok", 2) == 0) {
			for(int j = 0; j < TNUM; j++) {
				// Read the RLE pixels
				pixels[i * TNUM + j] = file->loadRLE(TTOI(1) * TTOI(1));
			}
			tiles += TNUM;
			LOG_MAX("Loaded tileset %d", i);
		} else if (strncmp(marker, "  ", 2) == 0) { // Empty tilesets have marker of 2 spaces
			LOG_TRACE("Skipping empty tileset %d", i);
		}
		delete[] marker;
	}

	if (file->getSize() != file->tell()) {
		LOG_WARN("Tileset data is corrupted");

		for (int i = 0; i < tiles; i++)
			delete[] pixels[i];

		return E_FILE;
	}

	LOG_TRACE("Loaded %d tiles", tiles);

	// Create combined buffer
	unsigned char* buffer = new unsigned char[TTOI(1) * TTOI(tiles)];
	for (int i = 0; i < tiles; i++) {
		memcpy(buffer + TTOI(1) * TTOI(1) * i,
			pixels[i], TTOI(1) * TTOI(1));
		delete[] pixels[i];
	}

	tileSet = video.createSurface(buffer, TTOI(1), TTOI(tiles));
	video.enableColorKey(tileSet, TKEY);
	delete[] buffer;

#if OJ_DEBUG
	// create an empty tileset for showing the mask
	maskedTileset = video.createSurface(nullptr, TTOI(1), TTOI(tiles));
	video.enableColorKey(maskedTileset, TKEY);
#endif

	return tiles;
#endif
}


/**
 * Load the level.
 *
 * @param fileName Name of the file containing the level data
 * @param checkpoint Whether or not the player(s) will start at a checkpoint
 *
 * @return Error code
 */
int JJ1Level::load (char* fileName, bool checkpoint) {
	unsigned char* buffer;

	// Load font
#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Loading level font");
#endif
	try {

		font = new Font(false);

	} catch (int e) {

		return e;

	}
	#if DEBUG_FONTS
	font->saveAtlasAsBMP("levelfont.bmp");
	#endif

	// Load panel
#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Loading HUD");
#endif
	int res = loadPanel();
	if (res < 0) {
		delete font;

		return res;
	}


	// Show loading screen

	// Open planet.### file
	char* string = nullptr;
	if (!strcmp(fileName, LEVEL_FILE)) {

		// Using the downloaded level file

		string = createString("DOWNLOADED");

	} else {

		// Load the planet's name from the planet.### file

		FilePtr planetFile;
		string = createFileName("PLANET", fileName + strlen(fileName) - 3);

		try {

			planetFile = std::make_unique<File>(string, PATH_TYPE_GAME);

		} catch (int e) {

			planetFile = nullptr;

		}

		delete[] string;

		if (planetFile) {

			planetFile->seek(2, true);
			string = planetFile->loadTerminatedString();

		} else {

			string = createString("CUSTOM");

		}

	}

	char* levelname = new char[strlen(string) + 14];
	strcpy(levelname, string);
	delete[] string;

	switch (fileName[5]) {
		case '0':
			strcat(levelname, " LEVEL ONE");
			break;

		case '1':
			strcat(levelname, " LEVEL TWO");
			break;

		case '2':
			strcat(levelname, " SECRET LEVEL");
			break;

		default:
			strcat(levelname, " LEVEL");
			break;
	}

	video.setPalette(menuPalette);
	video.clearScreen(0);

	const char *loadingString = "LOADING ";
	int stringWidth = fontmn2->getStringWidth(loadingString) + fontmn2->getStringWidth(levelname);
	Point pos = fontmn2->showString(loadingString, (canvasW - stringWidth) >> 1, canvasH >> 1);
	fontmn2->showString(levelname, pos.x, canvasH >> 1);

	camelcaseString(levelname);
	video.setTitle(levelname);
	delete[] levelname;

	if (::loop(NORMAL_LOOP) == E_QUIT) return E_QUIT;

	// Open level file
#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Opening level data");
	dc32OjLoadingAsset(fileName);
#endif
	FilePtr file;
	try {

		if (!strcmp(fileName, LEVEL_FILE))
			// use downloaded file
			file = std::make_unique<File>(fileName, PATH_TYPE_TEMP);
		else
			file = std::make_unique<File>(fileName, PATH_TYPE_GAME);

	} catch (int e) {

		deletePanel();
		delete font;

		return e;

	}

	// Checking level file header
	char *identifier1 = file->loadString(2);
	char identifier2 = file->loadChar();
	if (strncmp(identifier1, "DD", 2) != 0 || identifier2 != 0x1A) {
		LOG_ERROR("Level not valid!");
		delete[] identifier1;
		return E_FILE;
	}
	delete[] identifier1;

	// Load the blocks.### extension

	// Skip past all level data
	file->seek(39, true);
	file->skipRLE();
	file->skipRLE();
	file->skipRLE();
	file->skipRLE();
	file->skipRLE();
	file->skipRLE();
	file->skipRLE();
	file->skipRLE();
	file->seek(598, false);
	file->skipRLE();
	file->seek(4, false);
	file->skipRLE();
	file->skipRLE();
	file->seek(25, false);
	file->skipRLE();
	file->seek(3, false);

	// Load the level number
	levelNum = file->loadChar() ^ 210;

	// Load the world number
	worldNum = file->loadChar() ^ 4;

	// Load 100% counters
	nEnemies[0] = file->loadShort(); // Easy
	nEnemies[1] = nEnemies[0]; // Medium is same as Easy
	nEnemies[2] = file->loadShort(); // Hard
	nEnemies[3] = file->loadShort(); // Turbo
	nItems = file->loadShort();

	// Load tile set from appropriate blocks.###

	// Load tile set extension
	char *ext = file->loadTerminatedString(3);

	// Create tile set file name
	if (!strcmp(ext, "999")) string = createFileName("BLOCKS", worldNum);
	else string = createFileName("BLOCKS", ext);

	delete[] ext;

#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Attaching tile atlas");
#endif
	int tiles = loadTiles(string);

	delete[] string;

	if (tiles < 0) {

		deletePanel();
		delete font;

		return tiles;

	}


	// Load sprite set from corresponding Sprites.###

	string = createFileName("SPRITES", worldNum);
#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Attaching sprite set");
#endif
	res = loadSprites(string);

	delete[] string;

	if (res < 0) {
		video.destroySurface(tileSet);
#if OJ_DEBUG
		video.destroySurface(maskedTileset);
#endif
		deletePanel();
		delete font;

		return res;
	}


	// Skip to tile and event reference data
	file->seek(39, true);

	// Load tile and event references

#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Loading level grid");
	OjGridSink gridSink = {&grid[0][0], 0, {0, 0}};
	if (!file->loadRLEStream(LW * LH * 2, ojGridSink, &gridSink))
		return E_FILE;
#else
	buffer = file->loadRLE(LW * LH * 2);

	// Create grid from data
	for (int x = 0; x < LW; x++) {
		for (int y = 0; y < LH; y++) {
			grid[y][x].tile = buffer[(y + (x * LH)) << 1];
			grid[y][x].bg = buffer[((y + (x * LH)) << 1) + 1] >> 7;
			grid[y][x].event = buffer[((y + (x * LH)) << 1) + 1] & 127;
			grid[y][x].hits = 0;
			grid[y][x].time = 0;
		}
	}

	delete[] buffer;
#endif

	// Ignore tile transparency settings, these are applied based on event type/behaviour

	file->skipRLE();


	// Load mask data

#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Loading collision masks");
	memset(mask, 0, sizeof(mask));
	OjMaskSink maskSink = {&mask[0][0], (unsigned int)sizeof(mask), 0};
	if (!file->loadRLEStream(MASKS, ojMaskSink, &maskSink))
		return E_FILE;
#else
	buffer = file->loadRLE(MASKS); // TODO: find out how the 16 extra masks are used
	for (int i = 0; i < tiles; i++) {
		for (int y = 0; y < 8; y++) {
			for (int x = 0; x < 8; x++)
				mask[i][(y << 3) + x] = (buffer[(i << 3) + y] >> x) & 1;
		}
	}
	delete[] buffer;
#endif

#if OJ_DEBUG
	// This let's us see the mask of the tile graphics during gameplay

	if (SDL_MUSTLOCK(maskedTileset)) SDL_LockSurface(maskedTileset);

	for (int i = 0; i < tiles; i++) {
		for (int y = 0; y < 32; y++) {
			for (int x = 0; x < 32; x++) {

				bool masked = (mask[i][((y >> 2) << 3) + (x >> 2)] == 1);
				((char *)(maskedTileset->pixels))[(i * 1024) + (y * 32) + x] =
					masked ? 88 : TKEY; // orange

			}
		}
	}

	if (SDL_MUSTLOCK(maskedTileset)) SDL_UnlockSurface(maskedTileset);
#endif

	// Load special event path
#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Loading event paths");
	int pathBlockStart = file->tell();
	OjPathLengthSink pathLengthSink = {};
	if (!file->loadRLEStream(PATHS << 9, ojPathLengthSink, &pathLengthSink))
		return E_FILE;
	int pathUsed = 0;
	for (int type = 0; type < PATHS; type++) {
		path[type].length = pathLengthSink.lengths[type];
		if (path[type].length < 1) path[type].length = 1;
		if (pathUsed + path[type].length > 294) {
			return E_FILE;
		}
		path[type].x = pathXPool + pathUsed;
		path[type].y = pathYPool + pathUsed;
		pathUsed += path[type].length;
	}
	file->seek(pathBlockStart, true);
	OjPathDataSink pathDataSink = {path, 0};
	if (!file->loadRLEStream(PATHS << 9, ojPathDataSink, &pathDataSink))
		return E_FILE;
#else
	buffer = file->loadRLE(PATHS << 9);
	for (int type = 0; type < PATHS; type++) {
		path[type].length = buffer[type << 9] + (buffer[(type << 9) + 1] << 8);
		if (path[type].length < 1) path[type].length = 1;
		path[type].x = new short int[path[type].length];
		path[type].y = new short int[path[type].length];

		for (int i = 0; i < path[type].length; i++) {

			path[type].x[i] = reinterpret_cast<signed char*>(buffer)[(type << 9) + (i << 1) + 3] << 2;
			path[type].y[i] = reinterpret_cast<signed char*>(buffer)[(type << 9) + (i << 1) + 2];

		}

	}

	delete[] buffer;
#endif


	// Load event set
#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Loading event definitions");
	OjRecordSink eventSink = {{0}, ELENGTH, 0, ojConsumeEvent, eventSet};
	if (!file->loadRLEStream(EVENTS * ELENGTH, ojRecordSink, &eventSink))
		return E_FILE;
#else
	buffer = file->loadRLE(EVENTS * ELENGTH);
	for (int i = 0; i < EVENTS; i++)
		ojConsumeEvent(eventSet, i, buffer + i * ELENGTH);
#endif

	// Process grid

	enemies = items = 0;

	for (int x = 0; x < LW; x++) {
		for (int y = 0; y < LH; y++) {

			int type = grid[y][x].event;
			if (type) {
				// If the event hurts and can be killed, it is an enemy
				// Anything else that scores is an item
				if ((eventSet[type].modifier == 0) && eventSet[type].strength) enemies++;
				else if (eventSet[type].points) items++;
			}
		}
	}

#ifndef DC32_OPENJAZZ
	delete[] buffer;
#endif

#if DEBUG_LOAD
	// Show event names
	buffer = file->loadRLE(EVENTS * LONGNAME);

	for (int i = 0; i < EVENTS; i++) {
		char displayName[LONGNAME] = {0};
		strncpy(displayName, reinterpret_cast<char *>(buffer + i * LONGNAME + 1), buffer[i * LONGNAME]);
		displayName[LONGNAME-1] = '\0';

		if (strlen(displayName)) {
			LOG_MAX("Event id %d is named \"%s\"", i, displayName);
		}
	}
	delete[] buffer;
#else
	// Skip (usually empty) event names
	file->skipRLE();
#endif

	// Load animation set

#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Loading animations");
	OjAnimTarget animTarget = {
		animSet, spriteSet, sprites,
		animFrameSprites, animFrameX, animFrameY, 0, 531, true
	};
	OjRecordSink animSink = {{0}, 64, 0, ojConsumeAnim, &animTarget};
	if (!file->loadRLEStream(ANIMS << 6, ojRecordSink, &animSink) ||
			!animTarget.valid || animTarget.frameUsed > 531)
		return E_FILE;
#else
	buffer = file->loadRLE(ANIMS << 6);
	OjAnimTarget animTarget = {animSet, spriteSet, sprites};
	for (int i = 0; i < ANIMS; i++)
		ojConsumeAnim(&animTarget, i, buffer + (i << 6));
	delete[] buffer;
#endif

#if DEBUG_LOAD
	// Show animation names
	buffer = file->loadRLE(ANIMS * LONGNAME);

	for (int i = 0; i < ANIMS; i++) {
		char displayName[LONGNAME] = {0};
		strncpy(displayName, reinterpret_cast<char *>(buffer + i * LONGNAME + 1), buffer[i * LONGNAME]);
		displayName[LONGNAME-1] = '\0';

		if (strlen(displayName)) {
			LOG_MAX("Animation id %d is named \"%s\"", i, displayName);
		}
	}
	delete[] buffer;

	// Show level block names
	for (int i = 0; i < 16; i++) {
		char *tmpName = file->loadTerminatedString(SHORTNAME);
		if (strlen(tmpName)) {
			LOG_MAX("Level block id %d is named \"%s\"", i, tmpName);
		}
		delete[] tmpName;
	}

	// Skip compression info
	file->seek(9);
#else
	// Skip (usually empty) animation names
	file->skipRLE();

	// Skip level block names and compression info
	file->seek(16 * (SHORTNAME + 1) + 9);
#endif

	// Load sound map
	unsigned short int soundRates[SOUNDS];
	for (int i = 0; i < SOUNDS; i++) {
		soundRates[i] = file->loadShort();
	}
	for (int i = 0; i < SOUNDS; i++) {
		char *tmpName = file->loadTerminatedString(SHORTNAME);
		resampleSound(i, tmpName, soundRates[i]);
		delete[] tmpName;
	}

	// Music file
	musicFile = file->loadTerminatedString(12);
#if DEBUG_LOAD
	if (strlen(musicFile)) {
		LOG_MAX("Music is \"%s\"", musicFile);
	}
#endif

	// Skip (usually empty) level start cutscene
	file->seek(13);

	// End of episode cutscene
	sceneFile = file->loadTerminatedString(12);
#if DEBUG_LOAD
	if (strlen(sceneFile)) {
		LOG_MAX("End scene is \"%s\"", sceneFile);
	}
#endif

	// Skip level editor tileset files
	file->seek(39);

	// The players' initial coordinates
	unsigned char startX = file->loadShort(LW);
	unsigned char startY = file->loadShort(LH) + 1;

	// Next level
	int l = file->loadChar();
	int w = file->loadChar();
	setNext(l, w);

	// jump height
	jumpHeight = (file->loadShort() - 0xFFFF) / 2;
	if (jumpHeight != -5)
		LOG_TRACE("Uncommon jumpHeight: %i", jumpHeight);

	// skip some unknown level
	file->seek(2);

	// Thanks to Doubble Dutch for the water level bytes
	waterLevelTarget = ITOF(file->loadShort() + 17);
	waterLevel = waterLevelTarget - F8;
	waterLevelSpeed = -80000;

	// Jazz animation speed
	animSpeed = file->loadChar();
	if (animSpeed != 119)
		LOG_TRACE("Uncommon animationSpeed: %i", animSpeed);

	// Skip an unknown value (end marker?)
	file->seek(2);


	// Thanks to Feline and the JCS94 team for the next bits:

	// Load player's animation set references (always left + right)
	Anim* pAnims[JJ1PANIMS];
#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Loading player mappings");
	OjPlayerAnimSink playerAnimSink = {
		playerAnims, pAnims, animSet, 0, true
	};
	if (!file->loadRLEPrefixStream(JJ1PANIMS * 2, ojPlayerAnimSink,
			&playerAnimSink) ||
			playerAnimSink.offset != JJ1PANIMS * 2 ||
			!playerAnimSink.valid)
		return E_FILE;
#else
	buffer = file->loadRLE(JJ1PANIMS * 2);
#endif
	string = new char[MTL_P_ANIMS + JJ1PANIMS];

#ifndef DC32_OPENJAZZ
	for (int i = 0; i < JJ1PANIMS; i++) {
		playerAnims[i] = buffer[i << 1];
		pAnims[i] = animSet + playerAnims[i];
	}

	delete[] buffer;
#endif
	for (int i = 0; i < JJ1PANIMS; i++)
		string[MTL_P_ANIMS + i] = playerAnims[i];

	if (multiplayer) {

		string[0] = MTL_P_ANIMS + JJ1PANIMS;
		string[1] = MT_P_ANIMS;
		string[2] = 0;
		game->send(reinterpret_cast<unsigned char*>(string));

	}

	delete[] string;


#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Creating level players");
#endif
	createLevelPlayers(LT_JJ1, pAnims, NULL, checkpoint, startX, startY);


	// Load miscellaneous animations
	for (int i = 0; i < JJ1MANIMS; i++) {
		miscAnims[i] = file->loadChar();
	}


	// Load bullet set
#ifdef DC32_OPENJAZZ
	dc32OjLoadingStage("Loading bullet mappings");
	OjByteSink bulletSink = {
		reinterpret_cast<unsigned char*>(bulletSet),
		(unsigned int)sizeof(bulletSet), 0
	};
	if (!file->loadRLEStream(BULLETS * BLENGTH, ojByteSink, &bulletSink))
		return E_FILE;
#else
	buffer = file->loadRLE(BULLETS * BLENGTH);

	for (int i = 0; i < BULLETS; i++) {

		memcpy(bulletSet[i], buffer + (i * BLENGTH), BLENGTH);

	}

	delete[] buffer;
#endif

#if DEBUG_LOAD
	// Show attack names
	buffer = file->loadRLE(BULLETS * 21);

	for (int i = 0; i < BULLETS; i++) {
		char displayName[20] = {0};
		strncpy(displayName, reinterpret_cast<char *>(buffer + i * 21 + 1), buffer[i * 21]);
		displayName[20-1] = '\0';

		if (strlen(displayName)) {
			LOG_MAX("Attack id %d is named \"%s\"", i, displayName);
		}
	}
	delete[] buffer;
#else
	// Skip (usually empty) attack names
	file->skipRLE();
#endif

#ifdef DC32_OPENJAZZ
	unsigned char trailer[1 + 2 + JJ1LSOUNDS + JJ1LANIMS];

	dc32OjLoadingStage("Reading level trailer");
	if (file->getSize() - file->tell() != (int)sizeof(trailer) + 25 ||
			!file->readExact(trailer, sizeof(trailer)) ||
			file->tell() != file->getSize() - 25)
		return E_FILE;
	int type = trailer[0];
	if (type != 0 && type != PE_SKY && type != PE_2D &&
			type != PE_1D && type != PE_WATER)
		return E_FILE;
	if (trailer[1] > 1 || (trailer[1] && trailer[2] >= 240))
		return E_FILE;
	skyOrb = trailer[1] ? trailer[2] : 0;
	for (int i = 0; i < JJ1LSOUNDS; i++) {
		auto sound = static_cast<SE::Type>(trailer[3 + i]);

		if (!isValidSoundIndex(sound))
			return E_FILE;
		levelSounds[i] = trailer[3 + i];
	}
	for (int i = 0; i < JJ1LANIMS; i++) {
		unsigned char animation = trailer[3 + JJ1LSOUNDS + i];

		if (animation >= ANIMS)
			return E_FILE;
		levelAnims[i] = animation;
	}
	dc32OjLoadingStage("Preparing palette effects");
#else
	// Load level properties (magic)
	// First byte is the background palette effect type
	int type = file->loadChar();
#endif
	sky = false;

	switch (type) {
		case PE_SKY:
			sky = true;

			// Sky background effect
			paletteEffects = new SkyPaletteEffect(156, 100, FH, skyPalette, NULL);

			break;

		case PE_2D:
			// Parallaxing background effect
			paletteEffects = new P2DPaletteEffect(128, 64, FE, NULL);

			break;

		case PE_1D:
			// Diagonal stripes "parallaxing" background effect
			paletteEffects = new P1DPaletteEffect(128, 32, FH, NULL);

			break;

		case PE_WATER:
			// The deeper below water, the darker it gets
			paletteEffects = new WaterPaletteEffect(TTOF(32), NULL);

			break;

		default:
			// No effect
			paletteEffects = NULL;
			LOG_TRACE("Unknown palette effect: %d", type);

			break;
	}

	// Palette animations
	// These are applied to every level without a conflicting background effect
	// As a result, there are a few levels with things animated that shouldn't
	// be

	// In Diamondus: The red/yellow palette animation
	paletteEffects = new RotatePaletteEffect(112, 4, F32, paletteEffects);

	// In Diamondus: The waterfall palette animation
	paletteEffects = new RotatePaletteEffect(116, 8, F16, paletteEffects);

	// The following were discoverd by Unknown/Violet

	paletteEffects = new RotatePaletteEffect(124, 3, F16, paletteEffects);

	if ((type != PE_1D) && (type != PE_2D))
		paletteEffects = new RotatePaletteEffect(132, 8, F16, paletteEffects);

	if ((type != PE_SKY) && (type != PE_2D))
		paletteEffects = new RotatePaletteEffect(160, 32, -F16, paletteEffects);

	if (type != PE_SKY) {

		paletteEffects = new RotatePaletteEffect(192, 32, -F32, paletteEffects);
		paletteEffects = new RotatePaletteEffect(224, 16, F16, paletteEffects);

	}

	// Level fade-in/white-in effect
	if (checkpoint) paletteEffects = new FadeInPaletteEffect(T_START, paletteEffects);
	else paletteEffects = new WhiteInPaletteEffect(T_START, paletteEffects);
#ifdef DC32_OPENJAZZ
	{
		char paletteStatus[32];

		snprintf(paletteStatus, sizeof(paletteStatus), "Effects ready: %u",
			(unsigned int)dc32OjPaletteEffectSlotsUsed());
		dc32OjLoadingAsset(paletteStatus);
		dc32OjLoadingStage("Closing level data");
		file.reset();
	}
#endif


#ifndef DC32_OPENJAZZ
	// Check if a sun/star/distant planet, etc. is visible
	skyOrb = file->loadChar();

	// If so, find out which tile it uses or skip it
	if (skyOrb) skyOrb = file->loadChar();
	else file->loadChar();

	// Load level sound effects
	for (int i = 0; i < JJ1LSOUNDS; i++) {
		levelSounds[i] = file->loadChar();
#if 1//DEBUG_LOAD
		if (i == LSND_NOTHING1 || i == LSND_UNKNOWN6 || i == LSND_UNKNOWN9) {
			if (levelSounds[i])
				LOG_MAX("Level Sound %d is %d", i, levelSounds[i]);
		}
#endif
	}

	// Load level animations (shield gems, board, bird, shiver/slide)
	for (int i = 0; i < JJ1LANIMS; i++) {
		levelAnims[i] = file->loadChar();
#if 1//DEBUG_LOAD
		if (i == LA_NOTHING1 || i == LA_NOTHING2 || i == LA_NOTHING3) {
			if (levelAnims[i])
				LOG_MAX("Level Animation %d is %d", i, levelAnims[i]);
		}
#endif
	}
	if (levelAnims[LA_UNKNOWN9] != 31)
		LOG_TRACE("Uncommon level animation 9: %i", levelAnims[LA_UNKNOWN9]);
#endif

	// And that's us done!


	// Set the tick at which the level will end
	endTime = (5 - +getDifficulty()) * 2 * 60 * 1000;


	events = nullptr;
	bullets = nullptr;

	energyBar = 0;
	ammoType = 0;
	ammoOffset = -1;

	return E_NONE;

}
