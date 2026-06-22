
/**
 *
 * @file sprite.cpp
 *
 * Part of the OpenJazz project
 *
 * @par History:
 * - 23rd August 2005: Created level.c
 * - 1st January 2006: Created events.c from parts of level.c
 * - 3rd February 2009: Renamed events.c to events.cpp and level.c to level.cpp,
 *                    created player.cpp
 * - 5th February 2009: Added parts of events.cpp and level.cpp to player.cpp
 * - 19th March 2009: Created sprite.cpp from parts of event.cpp and player.cpp
 * - 26th July 2009: Created anim.cpp from parts of sprite.cpp
 *
 * @par Licence:
 * Copyright (c) 2005-2013 AJ Thomson
 *
 * OpenJazz is distributed under the terms of
 * the GNU General Public License, version 2.0
 *
 */


#include "SDL_wrapper.h"
#include "video.h"
#include "sprite.h"
#ifdef DC32_OPENJAZZ
#include "apps/openjazz/openjazz_cache.h"
#endif
#include <string.h>


/**
 * Create a sprite.
 */
Sprite::Sprite () {

#ifdef DC32_OPENJAZZ
	pixelData = NULL;
	pixelWidth = pixelHeight = pixelPitch = 0;
	pixelKey = 0;
	ownsPixelData = false;
	pixelKeyEnabled = false;
#else
	pixels = NULL;
#endif
	xOffset = 0;
	yOffset = 0;

}


/**
 * Delete the sprite.
 */
Sprite::~Sprite () {

#ifdef DC32_OPENJAZZ
	if (ownsPixelData)
		delete[] pixelData;
#else
	video.destroySurface(pixels);
#endif

}


/**
 * Make the sprite blank.
 */
void Sprite::clearPixels () {

	unsigned char data;

#ifdef DC32_OPENJAZZ
	if (ownsPixelData)
		delete[] pixelData;
	pixelData = new unsigned char[1];
	pixelData[0] = 0;
	pixelWidth = pixelHeight = pixelPitch = 1;
	pixelKey = 0;
	pixelKeyEnabled = true;
	ownsPixelData = true;
#else
	video.destroySurface(pixels);

	data = 0;
	pixels = video.createSurface(&data, 1, 1);
	video.enableColorKey(pixels, 0);
#endif

}


void Sprite::setOffset (short int x, short int y) {

	xOffset = x;
	yOffset = y;

}


/**
 * Set new pixel data for the sprite.
 *
 * @param data The new pixel data
 * @param width The width of the sprite image
 * @param height The height of the sprite image
 * @param key The transparent pixel value
 */
void Sprite::setPixels (unsigned char *data, int width, int height, unsigned char key) {

#ifdef DC32_OPENJAZZ
	if (ownsPixelData)
		delete[] pixelData;
	pixelData = new unsigned char[width * height];
	memcpy(pixelData, data, width * height);
	pixelWidth = pixelPitch = (unsigned short)width;
	pixelHeight = (unsigned short)height;
	pixelKey = key;
	pixelKeyEnabled = true;
	ownsPixelData = true;
#else
	video.destroySurface(pixels);

	pixels = video.createSurface(data, width, height);
	video.enableColorKey(pixels, key);
#endif

}

#ifdef DC32_OPENJAZZ
void Sprite::cachePixels (const char* name) {

	Dc32OjSurfaceView view;

	if (dc32OjCacheStoreView(name, pixelData, pixelWidth, pixelHeight, pixelPitch,
			pixelKey, pixelKeyEnabled, &view)) {
		if (ownsPixelData)
			delete[] pixelData;
		pixelData = const_cast<unsigned char*>(view.pixels);
		pixelWidth = view.width;
		pixelHeight = view.height;
		pixelPitch = view.pitch;
		pixelKey = view.colorKey;
		pixelKeyEnabled = view.colorKeyEnabled;
		ownsPixelData = false;
	}

}

bool Sprite::useCachedPixels (const char* name) {
	Dc32OjSurfaceView view;

	if (!dc32OjCacheFindView(name, &view))
		return false;
	if (ownsPixelData)
		delete[] pixelData;
	pixelData = const_cast<unsigned char*>(view.pixels);
	pixelWidth = view.width;
	pixelHeight = view.height;
	pixelPitch = view.pitch;
	pixelKey = view.colorKey;
	pixelKeyEnabled = view.colorKeyEnabled;
	ownsPixelData = false;
	return true;
}

bool Sprite::setCachedPixels (const char* name, const unsigned char* data,
	int width, int height, unsigned char key) {
	Dc32OjSurfaceView view;

	if (!dc32OjCacheStoreView(name, data, (uint16_t)width, (uint16_t)height,
			(uint16_t)width, key, true, &view))
		return false;
	return useCachedPixels(name);
}
#endif


/**
 * Get the width of the sprite.
 *
 * @return The width
 */
int Sprite::getWidth () {

#ifdef DC32_OPENJAZZ
	return pixelWidth;
#else
	return pixels->w;
#endif

}


/**
 * Get the height of the sprite.
 *
 * @return The height
 */
int Sprite::getHeight() {

#ifdef DC32_OPENJAZZ
	return pixelHeight;
#else
	return pixels->h;
#endif

}


/**
 * Get the horizontal offset of the sprite.
 *
 * @return The horizontal offset
 */
int Sprite::getXOffset () {

	return xOffset;

}


/**
 * Get the vertical offset of the sprite.
 *
 * @return The vertical offset
 */
int Sprite::getYOffset () {

	return yOffset;

}


/**
 * Set the sprite's palette, or a portion thereof.
 *
 * @param palette New palette
 * @param start First colour to change
 * @param amount Number of colours to change
 */
void Sprite::setPalette (SDL_Color *palette, int start, int amount) {

#ifdef DC32_OPENJAZZ
	SDL_Surface surface;
	dc32OjSdlInitReadOnlySurface(&surface, pixelData, pixelWidth, pixelHeight,
		pixelPitch, pixelKey, pixelKeyEnabled);
	video.setSurfacePalette(&surface, palette + start, start, amount);
#else
	video.setSurfacePalette(pixels, palette + start, start, amount);
#endif

}


/**
 * Map the whole of the sprite's palette to one index.
 *
 * @param index The index to use
 */
void Sprite::flashPalette (int index) {

	SDL_Color palette[MAX_PALETTE_COLORS];
	for (int i = 0; i < MAX_PALETTE_COLORS; i++) {
		palette[i].r = palette[i].g = palette[i].b = index;
#if OJ_SDL3 || OJ_SDL2
		palette[i].a = 0xFF;
#endif
	}

#ifdef DC32_OPENJAZZ
	SDL_Surface surface;
	dc32OjSdlInitReadOnlySurface(&surface, pixelData, pixelWidth, pixelHeight,
		pixelPitch, pixelKey, pixelKeyEnabled);
	video.setSurfacePalette(&surface, palette, 0, MAX_PALETTE_COLORS);
#else
	video.setSurfacePalette(pixels, palette, 0, MAX_PALETTE_COLORS);
#endif

}


/**
 * Restore the sprite's palette to its original state.
 */
void Sprite::restorePalette () {

#ifdef DC32_OPENJAZZ
	SDL_Surface surface;
	dc32OjSdlInitReadOnlySurface(&surface, pixelData, pixelWidth, pixelHeight,
		pixelPitch, pixelKey, pixelKeyEnabled);
	video.restoreSurfacePalette(&surface);
#else
	video.restoreSurfacePalette(pixels);
#endif

}


/**
 * Draw the sprite
 *
 * @param x The x-coordinate at which to draw the sprite
 * @param y The y-coordinate at which to draw the sprite
 * @param includeOffsets Whether or not to include the sprite's offsets
 */
void Sprite::draw (int x, int y, bool includeOffsets) {

	SDL_Rect dst;

	dst.x = x;
	dst.y = y;

	if (includeOffsets) {

		dst.x += xOffset;
		dst.y += yOffset;

	}

#ifdef DC32_OPENJAZZ
	SDL_Surface surface;
	if (dc32OjSdlInitReadOnlySurface(&surface, pixelData, pixelWidth, pixelHeight,
			pixelPitch, pixelKey, pixelKeyEnabled))
		SDL_BlitSurface(&surface, NULL, canvas, &dst);
#else
	SDL_BlitSurface(pixels, NULL, canvas, &dst);
#endif

}


/**
 * Draw the sprite scaled
 *
 * @param x The x-coordinate at which to draw the sprite
 * @param y The y-coordinate at which to draw the sprite
 * @param scale The amount by which to scale the sprite
 */
void Sprite::drawScaled (int x, int y, fixed scale) {

	int width, height, fullWidth, fullHeight;
	int dstX, dstY;
	int srcX, srcY;

#ifdef DC32_OPENJAZZ
	unsigned char key = pixelKey;
#else
	unsigned char key = video.getColorKey(pixels);
#endif

	fullWidth = FTOI(getWidth() * scale);
	if (x < -(fullWidth >> 1)) return; // Off-screen
	if (x + (fullWidth >> 1) > canvasW) width = canvasW + (fullWidth >> 1) - x;
	else width = fullWidth;

	fullHeight = FTOI(getHeight() * scale);
	if (y < -(fullHeight >> 1)) return; // Off-screen
	if (y + (fullHeight >> 1) > canvasH) height = canvasH + (fullHeight >> 1) - y;
	else height = fullHeight;

	if (SDL_MUSTLOCK(canvas)) SDL_LockSurface(canvas);

	if (y < (fullHeight >> 1)) {

		srcY = (fullHeight >> 1) - y;
		dstY = 0;

	} else {

		srcY = 0;
		dstY = y - (fullHeight >> 1);

	}

	while (srcY < height) {

#ifdef DC32_OPENJAZZ
		unsigned char* srcRow = pixelData + (pixelPitch * DIV(srcY, scale));
#else
		unsigned char* srcRow = static_cast<unsigned char*>(pixels->pixels) + (pixels->pitch * DIV(srcY, scale));
#endif
		unsigned char* dstRow = static_cast<unsigned char*>(canvas->pixels) + (canvas->pitch * dstY);

		if (x < (fullWidth >> 1)) {

			srcX = (fullWidth >> 1) - x;
			dstX = 0;

		} else {

			srcX = 0;
			dstX = x - (fullWidth >> 1);

		}

		while (srcX < width) {

			unsigned char pixel = srcRow[DIV(srcX, scale)];
			if (pixel != key) dstRow[dstX] = pixel;

			srcX++;
			dstX++;

		}

		srcY++;
		dstY++;

	}

	if (SDL_MUSTLOCK(canvas)) SDL_UnlockSurface(canvas);

}
