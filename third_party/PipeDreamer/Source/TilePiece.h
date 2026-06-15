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


// ---- Helper types and constants ----

static constexpr float MAX_OOZE_LEVEL = 100.0f;
static constexpr float MIN_OOZE_LEVEL = 0.0f;


// ---- Class definition ----

/**
 * Base class which represents all tiles, including Pipes as well as empty tiles on the Grid.
 */
class TilePiece
{
public:
	/**
	 * Score value of a regular pipe full of Ooze
	 */
	static const int PIPE_SCORE_VALUE;

	/**
	 * Additional score value of a cross-pipe full of Ooze on both ways.
	 */
	static const int CROSS_PIPE_SCORE_VALUE;


	/**
	 * Types of TilePiece.
	 */
	enum Type
	{
		TYPE_NONE = 0,
		TYPE_START_N,
		TYPE_START_S,
		TYPE_START_E,
		TYPE_START_W,
		TYPE_VERTICAL,
		TYPE_HORIZONTAL,
		TYPE_NW_ELBOW,
		TYPE_NE_ELBOW,
		TYPE_SE_ELBOW,
		TYPE_SW_ELBOW,
		TYPE_CROSS,
		TYPE_MAX
	};

	TilePiece();

	TilePiece(Type t);

	virtual ~TilePiece();

	static TilePiece* CreateTile(Type t = TilePiece::TYPE_NONE);

	Type GetType() const;

	bool IsStart() const;

	virtual int GetScoreValue() const;

protected:
	void SetType(Type t);

	Type	m_type;
};


/**
 * Class which represents Pipes through which Ooze can flow.
 * The member m_oozeLevel indicates how full of Ooze the pipe is (0 per default), 
 * and m_flowDirection indicates towards which opening the Ooze is flowing.
 */
class Pipe : public TilePiece
{
public:
	/**
	 * TODO
	 */
	enum Direction
	{
		DIR_NONE = 0,
		DIR_N,
		DIR_S,
		DIR_E,
		DIR_W,
	};

	Pipe(TilePiece::Type t);

	virtual ~Pipe();

	virtual float Pump(float amount);

	float GetOozeLevel() const;

	virtual bool IsFull() const;

	virtual bool IsEmpty() const;

	bool HasOpening(Pipe::Direction dir) const;

	static Pipe::Direction GetOppositeDirection(Pipe::Direction dir);

	virtual bool SetFlowEntry(Pipe::Direction dir);

	virtual Pipe::Direction GetFlowDirection() const;

	void Explode();

	int PopExplosion();

	int GetScoreValue() const override;

protected:
	/**
	 * TODO
	 */
	float m_oozeLevel;

	/**
	 * TODO
	 */
	int m_exploding;

	/**
	 * TODO
	 */
	Pipe::Direction m_flowDirection;
};

/**
 * Class which specifically represents Cross-Pipes. This type of Pipe requires special handling, 
 * because Ooze can flow through it twice: once vertically and once horizontally.
 */
class Cross : public Pipe
{
public:
	/**
	 * TODO
	 */
	enum Way
	{
		WAY_NONE = 0,
		WAY_VERTICAL,
		WAY_HORIZONTAL
	};

	Cross();

	float Pump(float amount) override;

	float GetOozeLevel(Way w) const;

	bool IsFull() const override;

	bool IsEmpty() const override;

	bool SetFlowEntry(Pipe::Direction dir) override;

	int GetScoreValue() const override;

	Way GetBackgroundWay() const;

protected:
	/**
	 * Replaces Pipe::m_oozeLevel, and keeps track of the Ooze fill level within
	 * the horizontal part of the Cross-Pipe.
	 */
	float m_horizOozeLevel;

	/**
	 * Replaces Pipe::m_oozeLevel, and keeps track of the Ooze fill level within
	 * the vertical part of the Cross-Pipe.
	 */
	float m_vertOozeLevel;

	bool m_horizWayFree;
	bool m_vertWayFree;

	/**
	 * Which way, horizontal or vertical, is drawn first and thus ends up 
	 * on the background. The second way, will then appear on the foreground. 
	 */
	Way m_backgroundWay;
};