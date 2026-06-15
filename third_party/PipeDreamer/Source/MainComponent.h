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

#include <JuceHeader.h>


// ---- Forward declarations ----

class TilePiece;
class ScoreWindow;


// ---- Class Definition ----

/**
 * GUI Component that occupies the entire game window.
 */
class MainComponent  :	public juce::Component,
						public juce::Timer,
						public juce::ChangeListener
{
public:
	/**
	 * Thickness of ooze inside pipes, in pixels.
	 */
	static const float OOZE_THICKNESS;

	/**
	 * GUI refresh interval in milliseconds.
	 */
	static const int GUI_REFRESH_RATE;

	/**
	 * Class constructor.
	 */
    MainComponent();
	
	/**
	 * Class destructor.
	 */
	~MainComponent() override;

	/**
	 * Get the width & height of a tile piece, in pixels.
	 */
	int GetTileSize() const;

	/**
	 * Reimplemented from juce::Component.
	 */
	void paint(juce::Graphics&) override;

	/**
	 * Reimplemented from juce::Component.
	 */
	void resized() override;

	/**
	 * Reimplemented from juce::Component.
	 */
	void mouseDown(const juce::MouseEvent& event) override;

	/**
	 * Reimplemented from juce::Timer.
	 */
	void timerCallback() override;

	/**
	 * Reimplemented from juce::ChangeListener.
	 */
	void changeListenerCallback(juce::ChangeBroadcaster* source) override;

	/**
	 * Tiles (more specifically, Pipes) change their background colour
	 * depending on the current difficulty level.
	 *
	 * @param difficultyLevel	The difficuly level, starting at 1, to get the tile colour for.
	 * @return	The colour to be used for drawing Pipes at the given level.
	 */
	static juce::Colour GetTileColourForLevel(int difficultyLevel);

	/**
	 * ID of the label on the GUI.
	 */
	enum LabelID
	{
		LABEL_SCORE = 0,
		LABEL_BSCORE,
		LABEL_VERSION
	};

	/**
	 * Get the font to be used for the given label.
	 */
	juce::Font GetFont(LabelID labelID) const;

	/**
	 * Draw current level number and score.
	 *
	 * @param g			The graphics context used for drawing.
	 */
	void DrawLevelAndScore(juce::Graphics& g);

	/**
	 * Draw a tile piece itself, of any kind.
	 *
	 * @param tile		The tile piece to draw.
	 * @param origin	The point on the MainComponent window where the top-left corner 
	 *					of the tile being drawn will be located.
	 * @param g			The graphics context used for drawing.
	 */
	void DrawTile(TilePiece* tile, juce::Point<int> origin, juce::Graphics& g);

	/**
	 * Draw the ooze flowing through a pipe tile.
	 *
	 * @param tile		The tile piece to draw the ooze over.
	 * @param origin	The point on the MainComponent window where the top-left corner
	 *					of the tile being drawn will be located.
	 * @param g			The graphics context used for drawing.
	 */
	void DrawOoze(TilePiece* tile, juce::Point<int> origin, juce::Graphics& g);

	/**
	 * The DrawTile() and DrawOoze() methods only worry about the first of the "ways" of TYPE_CROSS pipes, 
	 * either the vertical or the horizontal way. This method should be used afterwards to draw the 
	 * second, foreground way of TYPE_CROSS pipes. This method will not affect tiles of any other type.
	 *
	 * @param tile		The TYPE_CROSS tile piece to draw.
	 * @param origin	The point on the MainComponent window where the top-left corner
	 *					of the tile being drawn will be located.
	 * @param g			The graphics context used for drawing.
	 */
	void DrawCrossSecondWay(TilePiece* tile, juce::Point<int> origin, juce::Graphics& g);

	/**
	 * Draw a tile piece's decoration elements, such as frame and explosion graphics.
	 *
	 * @param tile		The tile piece to draw the decorations for.
	 * @param origin	The point on the MainComponent window where the top-left corner
	 *					of the tile being drawn will be located.
	 * @param g			The graphics context used for drawing.
	 */
	void DrawTileDecoration(TilePiece* tile, juce::Point<int> origin, juce::Graphics& g);

	/**
	 * Draw ooze spill next to the last pipe on the pipeline.
	 *
	 * @param origin	The point on the MainComponent window where the top-left corner
	 *					of the tile which caused the spill.
	 * @param g			The graphics context used for drawing.
	 */
	void DrawSpill(juce::Point<int> origin, juce::Graphics& g);

	/**
	 * Draw the "ooze meter", i.e. the vial that indicates the countdown until ooze starts pumping out.
	 *
	 * @param origin	The point on the MainComponent window to use as origin.
	 * @param g			The graphics context used for drawing.
	 */
	void DrawOozeMeter(juce::Point<int> origin, juce::Graphics& g);

	/**
	 * Draw the bombs next to the board, used to replace pipe tiles.
	 *
	 * @param origin	The point on the MainComponent window to use as origin.
	 * @param g			The graphics context used for drawing.
	 */
	void DrawBombs(juce::Point<int> origin, juce::Graphics& g);

	/**
	 * Draw the fast-forward button. 
	 * The location is hard coded, see m_fastForwardButtonRect.
	 *
	 * @param g			The graphics context used for drawing.
	 */
	void DrawFastForwardButton(juce::Graphics& g);


private:
	/**
	 * Width & height of a tile piece, in pixels.
	 */
	int m_tileSize = 0;

	/**
	 * Subcomponent for displaying the player's score after each round.
	 */
	std::unique_ptr<ScoreWindow> m_scoreWindow;

	/**
	 * Number of timerCallback ticks until Ooze starts pumping out.
	 */
	int m_countDown;

	int m_blockInteraction;

	juce::CriticalSection m_lock;

	/**
	 * Hyperlink to the download URL.
	 */
	std::unique_ptr<juce::HyperlinkButton> m_hyperlink;

	/**
	 * Rectangle containing the fast-forward button.
	 */
	juce::Rectangle<int> m_fastForwardButtonRect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
