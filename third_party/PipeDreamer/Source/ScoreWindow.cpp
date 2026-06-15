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


#include <JuceHeader.h>
#include "ScoreWindow.h"
#include "MainComponent.h"


// ---- Helper types and constants ----

const int ScoreWindow::BUTTON_HEIGHT(56);
const int HighScoreWindow::MAX_SCORE_ROWS(12);


// --- ScoreWindow ---

ScoreWindow::ScoreWindow(Controller::ScoreDetails details)
	:	m_details(details),
		m_command(Controller::CMD_NONE)
{

}

ScoreWindow::~ScoreWindow()
{

}

ScoreWindow* ScoreWindow::CreateScoreWindow(Controller::ScoreDetails details)
{
	ScoreWindow* ret(nullptr);

	if (details.advance)
		ret = new AdvanceWindow(details);
	else
		ret = new HighScoreWindow(details);

	return ret;
}

Controller::Command ScoreWindow::GetCommand() const
{
	return m_command;
}

void ScoreWindow::paint(juce::Graphics& g)
{
	int buttonHeight = m_messageBoxRect.getHeight() / 5;

	juce::String messageText("You Lose! :(\n");
	juce::String buttonText("Restart Game");
	if (m_details.advance)
	{
		messageText = juce::String("You Leveled Up!\n");
		buttonText = juce::String("Continue to Next Level");
	}

	// Background colour (whole window)
	g.fillAll(MainComponent::GetTileColourForLevel(m_details.level));

	// Messagebox background
	g.setColour(juce::Colour(27, 27, 27));
	g.fillRect(m_messageBoxRect);

	// Frame around messagebox
	g.setColour(juce::Colours::black);
	g.drawRect(m_messageBoxRect, 4);

	// Build score text
	messageText << "Score: " << juce::String(m_details.score) << "\n";
	if (m_details.bonus > 0)
		messageText << "Level Bonus: " << juce::String(m_details.bonus) << "\n";
	if (m_details.carryover > 0)
		messageText << "Carry Over: " << juce::String(m_details.carryover) << "\n";
	if (m_details.total != m_details.score)
		messageText << "Total: " << juce::String(m_details.total);

	// Display score
	g.setFont(juce::Font("consolas", 32.0f, juce::Font::plain));
	g.setColour(juce::Colours::grey);
	juce::Rectangle<int> subRect(m_messageBoxRect.getX() + 5, m_messageBoxRect.getY() + 5, m_messageBoxRect.getWidth() - 10, buttonHeight * 3);
	g.drawFittedText(messageText, subRect, juce::Justification::centred, true);

	// Continue button
	g.drawFittedText(buttonText, m_okButtonRect, juce::Justification::centred, true);
	g.drawRect(m_okButtonRect);

	// Quit button
	g.drawFittedText("Quit", m_quitButtonRect, juce::Justification::centred, true);
	g.drawRect(m_quitButtonRect);
}

void ScoreWindow::mouseDown(const juce::MouseEvent& event)
{
	juce::Point<int> clickPos = event.getMouseDownPosition();
	if (m_okButtonRect.contains(clickPos))
	{
		if (m_details.advance)
			m_command = Controller::CMD_CONTINUE;
		else
			m_command = Controller::CMD_RESTART;
	}

	else if (m_quitButtonRect.contains(clickPos))
		m_command = Controller::CMD_QUIT;

	// If a button was clicked, send a callback.
	if (m_command != Controller::CMD_NONE)
		sendChangeMessage();
}


// --- AdvanceWindow ---

AdvanceWindow::AdvanceWindow(Controller::ScoreDetails details)
	: ScoreWindow(details)
{
	setSize(400, 320);

	// Initialize rectangles for the messageBox, and the two buttons
	m_messageBoxRect = juce::Rectangle<int>(5, 5, getLocalBounds().getWidth() - 10, getLocalBounds().getHeight() - 10);

	int buttonHeight = m_messageBoxRect.getHeight() / 5;

	m_okButtonRect = juce::Rectangle<int>(10, m_messageBoxRect.getY() + (buttonHeight * 3) + 10, m_messageBoxRect.getWidth() - 10, buttonHeight - 10);
	m_quitButtonRect = juce::Rectangle<int>(10, m_messageBoxRect.getY() + (buttonHeight * 4) + 5, m_messageBoxRect.getWidth() - 10, buttonHeight - 10); 
}

void AdvanceWindow::paint(juce::Graphics& g)
{
	ScoreWindow::paint(g);
}


// --- HighScoreWindow ---

HighScoreWindow::HighScoreWindow(Controller::ScoreDetails details)
	: ScoreWindow(details)
{
	Controller* controller = Controller::GetInstance();
	if (controller != nullptr)
	{
		// Get the high score list, including the new score just achieved by the player.
		scoreEntry newEntry("placeholder", m_details.total);
		std::vector<scoreEntry> scoreHash = controller->GetAugmentedScoreHash(newEntry);

		// Initialize local lists of cached names/scores.
		RefreshCachedScore(scoreHash);
	}
}

HighScoreWindow::~HighScoreWindow()
{
	// If player entered their name, but never pressed 
	// the enter key, save their score anyway.
	if (m_nameEditor && !m_nameEditor.get()->getText().isEmpty())
	{
		ExitTextEditor(*m_nameEditor.get());
	}
}

void HighScoreWindow::RefreshCachedScore(std::vector<scoreEntry>& scoreHash)
{
	m_nameCache.clear();
	m_scoreCache.clear();
	m_dateCache.clear();
	m_nameCache.reserve(scoreHash.size());
	m_scoreCache.reserve(scoreHash.size());
	m_dateCache.reserve(scoreHash.size());

	// Create a local, sorted cache of names, dates, and scores, for quick access during paint().
	int maxNumRows = MAX_SCORE_ROWS;
	std::vector<scoreEntry>::iterator iter = scoreHash.begin();
	while (iter != scoreHash.end() && maxNumRows > 0)
	{
		juce::String dateStr;
		juce::String nameStr(iter->first);
		if (nameStr == "placeholder")
		{ 
			// Instead of timestamp, make it even more obvious that the player should enter their name.
			dateStr = "New Score!";

			// Create and initialize TextEditor component, where the player can type his name.
			m_nameEditor.reset(new juce::TextEditor());
			m_nameEditor->setMultiLine(false);
			m_nameEditor->setReturnKeyStartsNewLine(false);
			m_nameEditor->setCaretVisible(true);
			juce::String filter("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890.-");
			m_nameEditor->setInputRestrictions(8, filter);
			m_nameEditor->setFont(juce::Font("consolas", 25.0f, juce::Font::bold)); // TODO fontsize const
			m_nameEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colours::black);
			m_nameEditor->setColour(juce::TextEditor::textColourId, juce::Colours::yellow);
			m_nameEditor->setColour(juce::TextEditor::outlineColourId, juce::Colours::yellow);
			m_nameEditor->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::white);
			m_nameEditor->setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::white);
			m_nameEditor->setTextToShowWhenEmpty("name", juce::Colours::grey);
			m_nameEditor->addListener(this);
			addAndMakeVisible(m_nameEditor.get());
		}
		else
		{
			// The first half of the name is actually a timestamp.
			// Split the timestamp from the name and cache each separately. Cut the daytime, only keep the date.
			dateStr = nameStr.upToFirstOccurrenceOf("@", false /* don't include separator */, true /* ignore case */);
			nameStr = nameStr.fromFirstOccurrenceOf(";", false /* don't include separator */, true /* ignore case */);
		}

		m_nameCache.push_back(nameStr);
		m_scoreCache.push_back(iter->second);
		m_dateCache.push_back(dateStr);

		++iter;
		maxNumRows--;
	}
}

void HighScoreWindow::resized()
{
	MainComponent* mainComp = dynamic_cast<MainComponent*>(getParentComponent());
	if (mainComp != nullptr)
	{
		// Resize to match the size of the MainComponent.
		setSize(mainComp->getLocalBounds().getWidth(), mainComp->getLocalBounds().getHeight());

		int tileSize(mainComp->GetTileSize());
		int buttonVPos = getLocalBounds().getHeight() - tileSize - (BUTTON_HEIGHT * 2) - 12;
		int rectWidth = 320;

		// Initialize rectangles for the messageBox, and the two buttons
		m_messageBoxRect = juce::Rectangle<int>(tileSize, tileSize, rectWidth, getLocalBounds().getHeight() - (tileSize * 2));
		m_okButtonRect = juce::Rectangle<int>(tileSize + 5, buttonVPos, rectWidth - 10, BUTTON_HEIGHT);
		m_quitButtonRect = juce::Rectangle<int>(tileSize + 5, buttonVPos + BUTTON_HEIGHT + 5, rectWidth - 10, BUTTON_HEIGHT);
	}
}

void HighScoreWindow::paint(juce::Graphics& g)
{
	// Draws the player score breakdown panel.
	ScoreWindow::paint(g);

	int tileSize(0);
	MainComponent* mainComp = dynamic_cast<MainComponent*>(getParentComponent());
	if (mainComp != nullptr)
		tileSize = mainComp->GetTileSize();

	int rectWidth = (getLocalBounds().getWidth() - 320) - (tileSize * 2);

	// High-score box background colour
	g.setColour(juce::Colour(27, 27, 27));
	g.fillRect(juce::Rectangle<int>(tileSize + 320 - 2, tileSize, rectWidth, getLocalBounds().getHeight() - (tileSize * 2)));

	// Frame
	g.setColour(juce::Colours::black);
	g.drawRect(juce::Rectangle<int>(tileSize + 320 - 2, tileSize, rectWidth, getLocalBounds().getHeight() - (tileSize * 2)), 4);

	// Title
	g.setColour(juce::Colours::grey);
	g.setFont(juce::Font("consolas", 32.0f, juce::Font::bold));
	g.drawText("High Score", juce::Rectangle<int>(tileSize + 320 - 2, tileSize + 10, rectWidth, 60), juce::Justification::centred);

	int vPos = tileSize + 80;
	int hPosName = tileSize + 360;
	int hPosScore = tileSize + 460;
	int hPosDate = tileSize + 590;
	int fieldHeight = 30;
	int nameWidth = 100;
	int scoreWidth = 100;
	int dateWidth = 130;
	float fontSize = 25.0f; // TODO const
	g.setFont(juce::Font("consolas", 25.0f, juce::Font::plain));

	for (int i = 0; i < m_nameCache.size(); i++)
	{
		// Draw player name, OR position the TextEditor used to enter the players name
		g.setFont(juce::Font("consolas", fontSize, juce::Font::bold));
		if (m_nameCache[i] == "placeholder")
			m_nameEditor->setBounds(juce::Rectangle<int>(hPosName, vPos, nameWidth, fieldHeight));
		else
			g.drawText(m_nameCache[i], juce::Rectangle<int>(hPosName, vPos, nameWidth, fieldHeight), juce::Justification::left, false);

		// Frame around name
		//g.drawRect(juce::Rectangle<int>(hPosName, vPos, nameWidth, fieldHeight));

		// Draw score
		g.drawText(juce::String(m_scoreCache[i]), juce::Rectangle<int>(hPosScore, vPos, scoreWidth, fieldHeight), juce::Justification::right, false);
		//g.drawRect(juce::Rectangle<int>(hPosScore, vPos, scoreWidth, fieldHeight));

		// Draw date
		g.setFont(juce::Font("consolas", fontSize, juce::Font::plain));
		g.drawText(juce::String(m_dateCache[i]), juce::Rectangle<int>(hPosDate, vPos, dateWidth, fieldHeight), juce::Justification::right, false);
		//g.drawRect(juce::Rectangle<int>(hPosDate, vPos, dateWidth, fieldHeight));

		vPos += fieldHeight + 2;
	}
}

void HighScoreWindow::textEditorTextChanged(juce::TextEditor& editor)
{
	editor.setText(editor.getText().toUpperCase(), false /* do not trigger change signal */);
}

void HighScoreWindow::textEditorReturnKeyPressed(juce::TextEditor& editor)
{
	ExitTextEditor(editor);
}

void HighScoreWindow::textEditorEscapeKeyPressed(juce::TextEditor& editor)
{
	ExitTextEditor(editor);
}

void HighScoreWindow::textEditorFocusLost(juce::TextEditor& editor)
{
	ExitTextEditor(editor);
}

void HighScoreWindow::ExitTextEditor(juce::TextEditor& editor)
{
	Controller* controller = Controller::GetInstance();
	if (controller != nullptr)
	{
		// Timestamp in the format 29.04.2021@18:54:00
		juce::String nameStr(juce::Time::getCurrentTime().formatted("%d.%m.%Y@%H:%M:%S"));

		// Append name entered by the player.
		// Use default name if none was entered.
		if (editor.getText().isEmpty())
			nameStr << juce::String(";unknown");
		else
			nameStr << juce::String(";") << editor.getText();

		// Save the player's new score into the App Properties.
		scoreEntry newEntry(nameStr, m_details.total);
		controller->SaveScoreEntry(newEntry);

		// Get the now updated high score list. Send a zero-score entry as dummy.
		std::vector<scoreEntry> scoreHash = controller->GetAugmentedScoreHash(scoreEntry("",0));

		// Initialize local lists of cached names/scores.
		RefreshCachedScore(scoreHash);
	}

	// Delete TextEditor component, so that the entered name is 
	// displayed as normal text like the rest of the entries.
	m_nameEditor.reset(nullptr);

	repaint();
}

