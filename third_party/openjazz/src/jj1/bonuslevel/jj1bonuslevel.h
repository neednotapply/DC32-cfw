
/**
 *
 * @file jj1bonuslevel.h
 *
 * Part of the OpenJazz project
 *
 * @par History:
 * - 3rd February 2009: Created bonus.h
 * - 1st August 2012: Renamed bonus.h to jj1bonuslevel.h
 *
 * @par Licence:
 * Copyright (c) 2009-2017 AJ Thomson
 *
 * OpenJazz is distributed under the terms of
 * the GNU General Public License, version 2.0
 *
 */


#ifndef _BONUS_H
#define _BONUS_H

#include "io/gfx/anim.h"
#ifdef DC32_OPENJAZZ
#include "io/gfx/sprite.h"
#endif
#include "level/level.h"


// Constants

// General
#define BLW    256 /* Bonus level width */
#define BLH    256 /* Bonus level height */
#define BANIMS  32
#define BEVENTS 16
#define BSOUNDS 16

#define T_BONUS_END 2000


// Datatype

/// JJ1 bonus level grid element
typedef struct {

	unsigned char tile;  ///< Indexes the tile set
	unsigned char event; ///< Event type

} JJ1BonusLevelGridElement;

/// JJ1 bonus level event type
typedef struct {

	unsigned char anim; ///< Index of animation
	unsigned char type; ///< Type: time, gem, pole-bounce, exit, hand-bounce
	unsigned char passable; ///< Whether Jazz bounces back
	unsigned char used; ///< Whether this is a real event
	SE::Type      sound; ///< The sound played on the appropriate trigger

} JJ1BonusEventType;

// Classes

class Font;

/// JJ1 bonus level
class JJ1BonusLevel : public Level {

	private:
		SDL_Surface*             tileSet; ///< Tile images
		SDL_Surface*             background; ///< Background image
		Font*                    font; ///< On-screen message font
		Sprite*                  spriteSet; ///< Sprite images
#ifdef DC32_OPENJAZZ
		Sprite                   spriteSetStorage[51]; ///< Fixed bonus sprite descriptors
#endif
		Anim                     animSet[BANIMS]; ///< Animations
#ifdef DC32_OPENJAZZ
		Sprite*                  animFrameSprites[41]; ///< Exact bonus frame pool
		signed char              animFrameX[41];
		signed char              animFrameY[41];
#endif
		JJ1BonusEventType        eventSet[BEVENTS]; ///< Event types
#ifdef DC32_OPENJAZZ
		unsigned char            gridTiles[((BLH * BLW * 6) + 7) / 8 + 1]; ///< Packed 6-bit tile indices
		unsigned char            gridEvents[(BLH * BLW) / 2]; ///< Packed 4-bit event indices
#else
		JJ1BonusLevelGridElement grid[BLH][BLW]; ///< Level grid
#endif
	#ifdef DC32_OPENJAZZ
		unsigned char            mask[60][8]; ///< Packed 8x8 tile masks
	#else
		char                     mask[60][64]; ///< Tile masks (at most 60 tiles, all with 8 * 8 masks)
	#endif
		fixed                    direction; ///< Player's direction

		JJ1BonusLevel(const JJ1BonusLevel&); // non construction-copyable
		JJ1BonusLevel& operator=(const JJ1BonusLevel&); // non copyable

		int  loadSprites ();
		int  loadTiles   (char* fileName);
#ifdef DC32_OPENJAZZ
		unsigned char getGridTile (int x, int y) const;
		void setGridTile (int x, int y, unsigned char tile);
		unsigned char getGridEvent (int x, int y) const;
		void setGridEvent (int x, int y, unsigned char event);
#endif
		bool isEvent     (fixed x, fixed y);
		int  step        ();
		void draw        ();

	public:
	#ifdef DC32_OPENJAZZ
		static void* operator new (size_t size);
		static void operator delete (void* ptr);
	#endif
		JJ1BonusLevel  (Game* owner, char* fileName, bool multi);
		~JJ1BonusLevel () override;

		bool checkMask (fixed x, fixed y);
		void receive   (unsigned char* buffer) override;
		int  play      ();

};

#endif

