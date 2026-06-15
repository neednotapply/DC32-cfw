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


#include "Queue.h"
#include "Randomizer.h"


// ---- Class Implementation ----

Queue::Queue(int size)
{
	m_buff.reserve(size);
	Randomizer* rand = Randomizer::GetInstance();

	// Fill initial queue with random tiles, of any type between VERTICAL and CROSS.
	for (int i = 0; i < size; ++i)
	{
		TilePiece::Type t(static_cast<TilePiece::Type>(rand->GetWithinRange(TilePiece::TYPE_VERTICAL, TilePiece::TYPE_CROSS)));
		m_buff.push_back(dynamic_cast<Pipe*>(TilePiece::CreateTile(t)));
	}

	// This will move with every Pop.
	m_readPos = 0;
}

Queue::~Queue()
{
	for (int i = 0; i < m_buff.size(); ++i)
		delete m_buff[i];

}

void Queue::Reset()
{
	Randomizer* rand = Randomizer::GetInstance();
	for (int i = 0; i < m_buff.size(); ++i)
	{
		delete m_buff[i];

		TilePiece::Type t(static_cast<TilePiece::Type>(rand->GetWithinRange(TilePiece::TYPE_VERTICAL, TilePiece::TYPE_CROSS)));
		m_buff[i] = dynamic_cast<Pipe*>(TilePiece::CreateTile(t));
	}

	m_readPos = 0;
}

int Queue::GetSize() const
{
	return static_cast<int>(m_buff.size());
}

Pipe* Queue::GetTile(int pos) const
{
	int idx = (m_readPos + pos) % m_buff.size();

	return m_buff[idx];
}

TilePiece::Type Queue::GetTileType(int pos) const
{
	int idx = (m_readPos + pos) % m_buff.size();

	return m_buff[idx]->GetType();
}

TilePiece::Type Queue::Pop()
{
	TilePiece::Type currentType(m_buff[m_readPos]->GetType());

	// Randomize type of m_buff[m_readPos]
	Randomizer* rand = Randomizer::GetInstance();
	TilePiece::Type t(static_cast<TilePiece::Type>(rand->GetWithinRange(TilePiece::TYPE_VERTICAL, TilePiece::TYPE_CROSS)));

	delete m_buff[m_readPos];
	m_buff[m_readPos] = dynamic_cast<Pipe*>(TilePiece::CreateTile(t));

	// Move read position
	m_readPos = (m_readPos + 1) % m_buff.size();

	return currentType;
}