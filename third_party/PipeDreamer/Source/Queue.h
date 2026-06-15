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

#include "TilePiece.h"
#include <vector>


// ---- Class Definition ----

/**
 * Class which represents the pipe queue. Pipe tiles can be conceptually popped 
 * from the end of the queue, which will generate a new random pipe at the start of the queue.
 */
class Queue
{
public:
	Queue(int size);

	~Queue();

	void Reset();

	int GetSize() const;

	Pipe* GetTile(int pos) const;

	TilePiece::Type GetTileType(int pos) const;

	TilePiece::Type Pop();

protected:
	/**
	 * Underlying vector containing the Pipe objects.
	 */
	std::vector<Pipe*> m_buff;

	/**
	 * Index pointing to the end of the queue.
	 */
	int m_readPos;
};