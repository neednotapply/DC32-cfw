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
#include "Controller.h"


// ---- Class definition ----

/**
 * Base class for the component displaying the player's score at the end of each round.
 */
class ScoreWindow  :	public juce::Component,
						public juce::ChangeBroadcaster
{
public:
	/**
	 * Height of buttons, in pixels.
	 */
	static const int BUTTON_HEIGHT;

	/**
	 * Class destructor.
	 */
	~ScoreWindow() override;

	/**
	 * Create an AdvanceWindow or a HighScoreWindow depending on the ScoreDetails.
	 * 
	 * @param details	The ScoreDetails which will be displayed on the component.
	 * @return	Pointer to a new AdvanceWindow or a HighScoreWindow depending on the ScoreDetails.
	 */
	static ScoreWindow* CreateScoreWindow(Controller::ScoreDetails details);

	Controller::Command GetCommand() const;

	/**
	 * Overriden method of juce::Component.
	 */
	void paint(juce::Graphics&) override;

	/**
	 * Overriden method of juce::Component.
	 */
	void mouseDown(const juce::MouseEvent& event) override;

protected:
	/**
	 * Class constructor.
	 */
	ScoreWindow(Controller::ScoreDetails details);

	Controller::ScoreDetails m_details;

	Controller::Command m_command;

	juce::Rectangle<int> m_messageBoxRect;

	juce::Rectangle<int> m_okButtonRect;

	juce::Rectangle<int> m_quitButtonRect;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScoreWindow)
};


/**
 * Small window which displays the gained score, and offers a button to advance to the next level.
 */
class AdvanceWindow : public ScoreWindow
{
	friend class ScoreWindow;

public:
	void paint(juce::Graphics&) override;

protected:
	/**
	 * Class constructor.
	 */
	AdvanceWindow(Controller::ScoreDetails details);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvanceWindow)
};


/**
 * Window which displays the gained score, and also displays the high-score table.
 * This window is shown in the "game-over" scenario, when advancing to the next level isn't possible.
 */
class HighScoreWindow :	public ScoreWindow,
						public juce::TextEditor::Listener
{
	friend class ScoreWindow;

public:
	/**
	 * Class destructor.
	 */
	~HighScoreWindow() override;

	/**
	 * Overriden method of juce::Component.
	 */
	void paint(juce::Graphics&) override;

	/**
	 * Reimplemented from juce::Component.
	 */
	void resized() override;

	/**
	 * Max number of entries to show in the high score table.
	 */
	static const int MAX_SCORE_ROWS;

	/** 
	 * Overriden from juce::TextEditor::Listener
	 * Called when the user changes the text in some way. 
	 *
	 * @param editor	The TextEditor component being edited.
	 */
	void textEditorTextChanged(juce::TextEditor& editor) override;

	/**
	 * Overriden from juce::TextEditor::Listener
	 * Called when the user presses the return key. 
	 */
	void textEditorReturnKeyPressed(juce::TextEditor& editor) override;

	/** 
	 * Overriden from juce::TextEditor::Listener
	 * Called when the user presses the escape key.
	 */
	void textEditorEscapeKeyPressed(juce::TextEditor& editor) override;

	/** 
	 * Overriden from juce::TextEditor::Listener
	 * Called when the text editor loses focus.
	 */
	void textEditorFocusLost(juce::TextEditor& editor) override;


protected:
	/**
	 * TODO
	 */
	void RefreshCachedScore(std::vector<scoreEntry>& scoreHash);

	/**
	 * TODO
	 */
	void ExitTextEditor(juce::TextEditor& editor);

	/**
	 * Class constructor.
	 */
	HighScoreWindow(Controller::ScoreDetails details);

	/**
	 * Field for entering player name. Gets created only if the score gained is high enough.
	 */
	std::unique_ptr<juce::TextEditor> m_nameEditor;

	/**
	 * List of player names obtained from Controller::GetAugmentedScoreHash 
	 * and cached here for quick access during paint().
	 */
	std::vector<juce::String> m_nameCache;

	/**
	 * List of player scores obtained from Controller::GetAugmentedScoreHash
	 * and cached here for quick access during paint().
	 */
	std::vector<int> m_scoreCache;

	/**
	 * List of score dates obtained from Controller::GetAugmentedScoreHash
	 * and cached here for quick access during paint().
	 */
	std::vector<juce::String> m_dateCache;


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HighScoreWindow)
};
