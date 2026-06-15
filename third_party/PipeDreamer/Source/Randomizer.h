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

#include <random>
#include <map>


// ---- Class Definition ----

/**
 * Helper class that takes care of generating random numbers. 
 */
class Randomizer
{
public:
	Randomizer();

	~Randomizer();

	/**
	 * Returns the one and only instance of Randomizer. If it doesn't exist yet, it is created.
	 *
	 * @return The Randomizer singleton object.
	 */
	static Randomizer* GetInstance();

	/**
	 * Generate a random number x such that: min <= x <= max.
	 *
	 * @param min	Range minimum.
	 * @param max	Range maximum.
	 * @return A random number within the closed interval [min, max].
	 */
	int GetWithinRange(int min, int max);

protected:
	/**
	 * The one and only instance of Randomizer.
	 */
	static Randomizer* m_singleton;

	typedef std::pair<int, int> Range;
	typedef std::uniform_int_distribution<int> Distro;

	/**
	 * Commonly used distributions are cached here.
	 * TODO: explain better.
	 */
	std::map<Range, Distro> m_distroMap;

	std::mt19937 m_mt;
};