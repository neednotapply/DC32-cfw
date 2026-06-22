
/**
 *
 * @file sprite.h
 *
 * Part of the OpenJazz project
 *
 * @par History:
 * - 23rd August 2005: Created OpenJazz.h
 * - 31st January 2006: Created level.h from parts of OpenJazz.h
 * - 19th March 2009: Created sprite.h from parts of level.h
 * - 26th July 2009: Created anim.h from parts of sprite.h
 *
 * @par Licence:
 * Copyright (c) 2005-2013 AJ Thomson
 *
 * OpenJazz is distributed under the terms of
 * the GNU General Public License, version 2.0
 *
 */


#ifndef _SPRITE_H
#define _SPRITE_H

#include "OpenJazz.h"

// Class

struct SDL_Surface;
struct SDL_Color;

/// Sprite
class Sprite {

	private:
	#ifdef DC32_OPENJAZZ
		unsigned char* pixelData;
		unsigned short pixelWidth;
		unsigned short pixelHeight;
		unsigned short pixelPitch;
		unsigned char  pixelKey;
		bool           ownsPixelData;
		bool           pixelKeyEnabled;
	#else
		SDL_Surface* pixels; ///< Sprite image
	#endif
		short int    xOffset; ///< Horizontal offset
		short int    yOffset; ///< Vertical offset

	public:
		Sprite              ();
		~Sprite             ();

		void clearPixels    ();
		void setOffset      (short int x, short int y);
		void setPixels      (unsigned char* data, int width, int height, unsigned char key);
#ifdef DC32_OPENJAZZ
		void cachePixels    (const char* name);
		bool useCachedPixels(const char* name);
		bool setCachedPixels(const char* name, const unsigned char* data,
			int width, int height, unsigned char key);
#endif
		int  getWidth       ();
		int  getHeight      ();
		int  getXOffset     ();
		int  getYOffset     ();
		void draw           (int x, int y, bool includeOffsets = true);
		void drawScaled     (int x, int y, fixed scale);
		void setPalette     (SDL_Color* palette, int start, int amount);
		void flashPalette   (int index);
		void restorePalette ();

};

#endif

