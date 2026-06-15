/*
===============================================================================

Copyright (C) 2021 Bernardo Escalona. All Rights Reserved.

  This file is part of Pipe Dreamer, found at:
  https://github.com/escalonely/PipeDreamer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

===============================================================================
*/


#pragma once

#include <vector>
#include <map>
#include "TilePiece.h"


// ---- Class Definition ----

/**
 * Class which represents the grid where pipe tiles can be placed.
 */
class Board
{
public:
	/**
	 * Max number of bombs available per round, used to replace existing pipe tiles.
	 */
	static const int MAX_NUM_BOMBS;

	/**
	 * Score points required to restore a used up bomb.
	 */
	static const int SCORE_FOR_FREE_BOMB;

	/**
	 * Class constructor.
	 */
	Board(int numCols, int numRows);

	/**
	 * Class destructor. Deletes all entries in m_tileMap.
	 */
	virtual ~Board();

	/**
	 * Get the type of the tile at the given coordinates.
	 * 
	 * @param col	Column of desired tile.
	 * @param row	Row of desired tile.
	 * @return The type of the desired tile. Can be TYPE_NONE if the tile is empty.
	 */
	TilePiece::Type GetTileType(int col, int row) const;

	/**
	 * Get the tile at the desired location.
	 */
	TilePiece* GetTile(int col, int row) const;

	void ReplaceTile(int col, int row, TilePiece::Type t);

	int GetNumRows() const;

	int GetNumCols() const;

	/**
	 * Pump ooze into the pipes on the board. This method is called at every tick.
	 * Whichever pipe is m_oozingTile currently, will have its Pump() method called,
	 * and thus the amount of ooze inside it increased.
	 * 
	 * @param amount	Amount of ooze to insert. The higher the level, 
	 *					the more ooze amount will be pumped every tick.
	 * @return	True if the ooze is still contained within m_oozingTile or it's neighbor.
	 *			False if the ooze has now spilled.
	 */
	bool Pump(float amount);

	/**
	 * Resets score, bombs, clears all tiles, and repositions starting tile
	 * to a random position.
	 */
	void Reset();

	void CreateRandomStart();

	TilePiece* FindNeighbor(TilePiece* p, Pipe::Direction d) /*const*/; // TODO: const deadlock

	/**
	 * Get the score gained so far in this round.
	 *
	 * @return	The score points on the board.
	 */
	int GetScoreValue() const;

	/**
	 * Get the number of bombs still available this round.
	 * Bombs are used to replace existing pipe tiles on the grid.
	 *
	 * @return	Number of bombs available.
	 */
	int GetNumBombs() const;

	/**
	 * Expend one of the available bombs, if available.
	 * The number of available bombs will be reduced by one.
	 *
	 * @return	True if at least one bomb was available prior to this call.
	 */
	bool PopBomb();

	/**
	 * Get the score gained so far, until one of the expended bombs is restored, in percent.
	 * Once this reaches 100, the number of available bombs will increase by one.
	 * After that, the score until the next restored bomb will be 0 again.
	 *
	 * @return	Score until next restored bomb, in percent.
	 */
	int GetPercentUntilFreeBomb();

	/**
	 * Get the pipe on the board, in which the ooze level is currently increasing.
	 */
	TilePiece* GetOozingTile() const;

private:
	/**
	 * Pipe piece in which the ooze level is currently increasing.
	 */
	TilePiece* m_oozingTile;

	int m_numCols;
	int m_numRows;

	typedef std::pair<int, int> Coord;
	std::map<Coord, TilePiece*> m_tileMap;

	int m_score;

	int m_numBombs;

	/**
	 * Score points until an used up bomb will be restored.
	 * Once SCORE_FOR_FREE_BOMB is reached, this counter is set back to 0.
	 */
	int m_scoreUntilFreeBomb;
};
