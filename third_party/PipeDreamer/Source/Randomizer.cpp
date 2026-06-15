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


#include "Randomizer.h"
#include <assert.h>


// ---- Helper types and constants ----

/**
 * Singleton initialization.
 */
Randomizer* Randomizer::m_singleton = nullptr;


// --- Randomizer class implementation ---

Randomizer::Randomizer()
{
	assert(m_singleton == nullptr);
	m_singleton = this;

	// Seed randomizer
#ifdef DCAPP_BUILD
	static unsigned int seed = 0x50495045u;
	seed = seed * 1664525u + 1013904223u;
	m_mt = std::mt19937(seed);
#else
	std::random_device rd;
	m_mt = std::mt19937(rd());
#endif

}

Randomizer::~Randomizer()
{
	m_singleton = nullptr;
}

Randomizer* Randomizer::GetInstance()
{
	if (m_singleton == nullptr)
	{
		m_singleton = new Randomizer();
	}
	return m_singleton;
}

int Randomizer::GetWithinRange(int min, int max)
{
	Range r(min, max);
	if (m_distroMap.find(r) == m_distroMap.end())
	{
		Distro d = std::uniform_int_distribution<int>(min, max);
		m_distroMap.insert(std::pair<Range, Distro>(r, d));
	}

	int ret = (m_distroMap[r](m_mt));
	return ret;
}
