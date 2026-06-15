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


#include "MainComponent.h"
#include "TilePiece.h"
#include "Board.h"
#include "Queue.h"
#include "Randomizer.h"
#include "ScoreWindow.h"



// ---- Helper types and constants ----

const float MainComponent::OOZE_THICKNESS(15.0f);
const int MainComponent::GUI_REFRESH_RATE(60);


// ---- Class Implementation ----

MainComponent::MainComponent()
	:	m_blockInteraction(0)
{
	// Create GUI component wich will work as a clickable hyperlink to our github.
	m_hyperlink = std::make_unique<juce::HyperlinkButton>(	juce::String("https://github.com/escalonely/PipeDreamer"),
															juce::URL("https://github.com/escalonely/PipeDreamer"));
	m_hyperlink->setColour(juce::HyperlinkButton::textColourId, juce::Colours::grey);
	addAndMakeVisible(m_hyperlink.get());

	setSize(900, 620);

	// Reset the countdown to the start of the round
	// (before ooze starts pumping out)
	m_countDown = Controller::GetInstance()->GetCurrentCountdown();

	// GUI-refreh rate
	startTimer(GUI_REFRESH_RATE);
}

MainComponent::~MainComponent()
{

}

int MainComponent::GetTileSize() const
{
	return m_tileSize;
}

void MainComponent::resized()
{
	// Scale tiles according to the game window's both width and height.
	int minDimension = std::min<int>(getLocalBounds().getWidth(), static_cast<int>(getLocalBounds().getHeight() * 1.4516f));
	m_tileSize = static_cast<int>(minDimension / 13.0f);

	m_fastForwardButtonRect = juce::Rectangle<float>(getLocalBounds().getWidth() / 18.0f, getLocalBounds().getHeight() / 1.1245f, 70.0f, 45.0f).toNearestInt();

	// Position the hyperlink
	m_hyperlink->setFont(GetFont(LABEL_VERSION), false /* do not resize */);
	m_hyperlink->setBounds(	static_cast<int>(getLocalBounds().getWidth() / 1.8f), 
							getLocalBounds().getHeight() - 50, 
							static_cast<int>(getLocalBounds().getWidth() / 2.4545f), 
							40);

	// Resize the ScoreWindow, if any.
	if (m_scoreWindow)
		m_scoreWindow->resized();
}

void MainComponent::timerCallback()
{
	const juce::ScopedLock lock(m_lock);

	Controller* controller(Controller::GetInstance());
	Controller::GameState state(controller->GetState());

	if (state == Controller::STATE_RUNNING)
	{
		// When it reaches 0, clicks are enabled again.
		if (m_blockInteraction > 0)
			m_blockInteraction--;

		// Countdown to start pumping ooze.
		if (m_countDown > 0)
		{
			// If fast-forward button is currently toggled on, decrease countdown faster.
			if (controller->GetFastForward())
				m_countDown -= 5;
			else
				m_countDown -= 1;
		}

		else
		{
			bool contained = controller->Pump();
			if (!contained)
			{
				// Ooze spill! 
				// Give the player a moment to see where the spill took place,
				// before covering up the board with the score window.
				startTimer(2000);
			}
		}
	}

	else if (state == Controller::STATE_STOPPED)
	{
		// Stop refreshing GUI.
		stopTimer();

		// Position the small AdvanceWindow in the middle of the window,
		// while the HighScoreWindow will take up the whole window.
		Controller::ScoreDetails details(controller->GetScoreDetails());
		juce::Point<int> windowOrigin(0, 0);
		if (details.advance)
			windowOrigin = juce::Point<int>(getLocalBounds().getWidth() / 3, getLocalBounds().getHeight() / 4);

		// Show scoreboard overlay.
		m_scoreWindow.reset(ScoreWindow::CreateScoreWindow(details));
		m_scoreWindow->addChangeListener(this);
		addAndMakeVisible(m_scoreWindow.get());
		m_scoreWindow->setTopLeftPosition(windowOrigin);
		m_scoreWindow->resized();
	}

	this->repaint();
}

void MainComponent::mouseDown(const juce::MouseEvent& event)
{
	const juce::ScopedLock lock(m_lock);

	Controller* controller(Controller::GetInstance());

	if ((controller->GetState() == Controller::STATE_RUNNING) &&
		(m_blockInteraction == 0))
	{
		juce::Point<int> clickPos = event.getMouseDownPosition();

		// If user clicked on the fast-forward button, toggle fast-forward state.
		// This increases the ooze amount in GetCurrentOozePerPump().
		if (m_fastForwardButtonRect.contains(clickPos))
		{
			controller->SetFastForward(!controller->GetFastForward());
		}

		else
		{
			Board* board(controller->GetBoard());
			bool replace(false);
			int boardHStartPos = static_cast<int>(getLocalBounds().getWidth() / 5.114f);
			int boardVStartPos = static_cast<int>(getLocalBounds().getHeight() / 7.75f);

			for (int i = 0; (i < board->GetNumCols()) && !replace; i++)
			{
				for (int j = 0; (j < board->GetNumRows()) && !replace; j++)
				{
					juce::Rectangle<int> tileRect(	boardHStartPos + i * (m_tileSize - 1),
													boardVStartPos + j * (m_tileSize - 1),
													m_tileSize, m_tileSize);
					if (tileRect.contains(clickPos))
					{
						// Default sound effect for placing pipes on the grid.
						Controller::SoundID soundID(Controller::SOUND_CLICK);

						TilePiece* clickedTile = board->GetTile(i, j);
						replace = (clickedTile->GetType() == TilePiece::TYPE_NONE);

						if (!replace)
						{
							Pipe* clickedPipe = dynamic_cast<Pipe*>(clickedTile);
							if ((clickedPipe != nullptr) &&
								(clickedPipe->IsEmpty()) &&		// Only empty tiles can be replaced.
								(!clickedPipe->IsStart()) &&	// Cannot replace starter tiles.
								(board->PopBomb()))				// Need bombs to replace existing pipe tiles.
							{
								replace = true;

								// Sound effect should be explosive instead.
								soundID = Controller::SOUND_EXPLODE;
							}
						}

						if (replace)
						{
							// Trigger approproate sound effect.
							Controller::GetInstance()->QueueSound(soundID);

							// Grab the next piece in the queue, and...
							TilePiece::Type newType = controller->GetQueue()->Pop();

							// ... place it on the board.
							board->ReplaceTile(i, j, newType);

							// To prevent accidental double-clicking disable actions for a few frames.
							static constexpr int framesInteractionBlocked = 5;
							m_blockInteraction += framesInteractionBlocked;
						}
					}
				}
			}
		}
	}
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source) 
{
	(void)source;

	const juce::ScopedLock lock(m_lock);

	if (m_scoreWindow != nullptr)
	{
		switch (m_scoreWindow->GetCommand())
		{
			case Controller::CMD_RESTART:
			case Controller::CMD_CONTINUE:
				{
					Controller::GetInstance()->Reset(m_scoreWindow->GetCommand());

					// Countdown to ooze pumping.
					m_countDown = Controller::GetInstance()->GetCurrentCountdown();

					// Restart GUI
					m_blockInteraction = 0;
					startTimer(GUI_REFRESH_RATE);
				}
				break;

			case Controller::CMD_QUIT:
				{
					juce::JUCEApplicationBase::quit();
				}
				break;
				
			default:
				break;
		}

		// This deletes the unique_ptr.
		m_scoreWindow = nullptr;
	}
}

juce::Colour MainComponent::GetTileColourForLevel(int difficultyLevel)
{
	static const juce::Colour colorsPerLevel[] = {
		juce::Colour(125, 125, 125),	// Level 1
		juce::Colours::cadetblue,		// Level 2
		juce::Colours::darkkhaki,		// Level 3
		juce::Colour(140, 180, 90),		// Level 4
		juce::Colours::darkslategrey,	// Level 5
		juce::Colours::hotpink,			// Level 6
		juce::Colour(27, 122, 165),		// Level 7
		juce::Colours::coral,			// Level 8
		juce::Colours::blueviolet,		// Level 9
		juce::Colours::darkorange,		// Level 10
		juce::Colours::mediumseagreen,	// Level 11
		juce::Colours::orangered,		// Level 12
	};

	// difficultyLevel starts at 1
	int arraySize = sizeof(colorsPerLevel) / sizeof(*colorsPerLevel);
	difficultyLevel--;
	if (difficultyLevel >= arraySize)
		difficultyLevel = arraySize - 1;

	return colorsPerLevel[difficultyLevel];
}

juce::Font MainComponent::GetFont(LabelID labelID) const
{
	// Scale the font according to the game window's both width and height.
	int minDimension = std::min<int>(static_cast<int>(getLocalBounds().getWidth() / 1.4516f), getLocalBounds().getHeight());
	switch (labelID)
	{
		case LABEL_VERSION:
			return juce::Font("consolas", (minDimension * 18.0f / 620.0f), juce::Font::plain);
		case LABEL_SCORE:
			return juce::Font("consolas", (minDimension * 32.0f / 620.0f), juce::Font::plain);
		case LABEL_BSCORE:
			return juce::Font("consolas", (minDimension * 32.0f / 620.0f), juce::Font::bold);
	}

	return juce::Font();
}

void MainComponent::paint(juce::Graphics& g)
{
	Board* board(Controller::GetInstance()->GetBoard());
	Queue* queue(Controller::GetInstance()->GetQueue());

	int boardHStartPos = static_cast<int>(getLocalBounds().getWidth() / 5.114f);
	int boardVStartPos = static_cast<int>(getLocalBounds().getHeight() / 7.75f);

	// Background colour
	g.fillAll(juce::Colour(67, 67, 67));

	// Draw countdown to ooze.
	DrawOozeMeter(juce::Point<int>(static_cast<int>(getLocalBounds().getWidth() / 12.05f), 30), g);

	// Draw current level number and score
	DrawLevelAndScore(g);

	// Draw app info
	{
		juce::String infoText("Pipe Dreamer V");
		juce::String versionString(JUCE_STRINGIFY(JUCE_APP_VERSION));
		infoText << versionString;

		juce::Rectangle<int> textRect(boardHStartPos, getLocalBounds().getHeight() - 50, static_cast<int>(getLocalBounds().getWidth() / 5.625f), 40);

		g.setFont(GetFont(LABEL_VERSION));
		g.setColour(juce::Colours::grey);
		g.drawText(infoText, textRect, juce::Justification::left, false);
		//g.drawRect(textRect, 1);

		//textRect = juce::Rectangle<int>(getLocalBounds().getWidth() - 380, getLocalBounds().getHeight() - 50, 350, 40);
		//g.drawRect(textRect, 1);
	}

	// Draw bombs
	DrawBombs(juce::Point<int>(static_cast<int>(getLocalBounds().getWidth() / 1.6749f), 20), g);

	// Draw button to accelerate game speed.
	DrawFastForwardButton(g);

	// Draw tile queue: queueVStartPos is the origin ob the bottomest tile in the queue
	int queueVStartPos = static_cast<int>(getLocalBounds().getHeight() / 1.3757f);
	int queueHStartPos = static_cast<int>(getLocalBounds().getWidth() / 18);
	for (int i = 0; i < queue->GetSize(); i++)
	{
		// Pipe shape
		juce::Point<int> p(queueHStartPos, queueVStartPos - i * (m_tileSize - 1));
		DrawTile((queue->GetTile(i)), p, g);
		DrawCrossSecondWay((queue->GetTile(i)), p, g);
		DrawTileDecoration((queue->GetTile(i)), p, g);

		if (i == 0)
		{
			// Extra frame for the tile at the start of the queue.
			g.setColour(juce::Colours::limegreen);
			g.drawRect(queueHStartPos - 4, queueVStartPos - 4, m_tileSize + 8, m_tileSize + 8, 2);
			g.setColour(juce::Colours::black);
			g.drawRect(queueHStartPos - 6, queueVStartPos - 6, m_tileSize + 12, m_tileSize + 12, 2);
		}
	}

	// Position of the oozing tile. Needed for displaying spills below.
	juce::Point<int> oozingTileOrigin;

	// Draw board.
	for (int i = 0; i < board->GetNumCols(); i++)
	{
		for (int j = 0; j < board->GetNumRows(); j++)
		{
			juce::Point<int> p(boardHStartPos + i * (m_tileSize - 1), boardVStartPos + j * (m_tileSize - 1));

			TilePiece* tile = board->GetTile(i, j);

			DrawTile(tile, p, g);
			DrawOoze(tile, p, g);
			DrawCrossSecondWay(tile, p, g);
			DrawTileDecoration(tile, p, g);

			if (board->GetOozingTile() == tile)
				oozingTileOrigin = p;
		}
	}

	// Draw ooze spillage, if any.
	DrawSpill(oozingTileOrigin, g);
}

void MainComponent::DrawLevelAndScore(juce::Graphics& g)
{
	Controller* controller(Controller::GetInstance());
	int playerScore = controller->GetBoard()->GetScoreValue();

	g.setFont(GetFont(LABEL_SCORE));
	g.setColour(juce::Colours::grey);

	int halfTile = m_tileSize / 2;
	juce::Rectangle<int> textRect(	static_cast<int>(getLocalBounds().getWidth() / 5.114f), 20, 
									static_cast<int>(getLocalBounds().getWidth() / 9.7826f), halfTile);
	g.drawText("Level:", textRect, juce::Justification::left, false);
	// g.drawRect(textRect, 1.0f); // frame

	textRect = juce::Rectangle<int>(	static_cast<int>(getLocalBounds().getWidth() / 2.8421f), 20, 
										static_cast<int>(getLocalBounds().getWidth() / 9.7826f), halfTile);
	g.drawText("Score:", textRect, juce::Justification::left, false);
	// g.drawRect(textRect, 1.0f); // frame

	// If score this round is high enough to advance to next difficulty level, highlight the number.
	if (playerScore >= Controller::MIN_SCORE_TO_ADVANCE)
	{
		g.setColour(juce::Colours::yellow);
		g.setFont(GetFont(LABEL_BSCORE));
	}
	textRect = juce::Rectangle<int>(static_cast<int>(getLocalBounds().getWidth() / 2.1880f), 20, 
									static_cast<int>(getLocalBounds().getWidth() / 5.625f), halfTile);
	g.drawText(juce::String(playerScore), textRect, juce::Justification::left, false);
	// g.drawRect(textRect, 1.0f); // frame

	// Show difficulty level number in this level's tile color.
	g.setColour(GetTileColourForLevel(controller->GetDifficultyLevel()));
	textRect = juce::Rectangle<int>(static_cast<int>(getLocalBounds().getWidth() / 3.375f), 20, 
									static_cast<int>(getLocalBounds().getWidth() / 17.31f), halfTile);
	g.drawText(juce::String(controller->GetDifficultyLevel()), textRect, juce::Justification::left, false);
	// g.drawRect(textRect, 1.0f); // frame
}

void MainComponent::DrawTile(TilePiece* tile, juce::Point<int> origin, juce::Graphics& g)
{
	if (tile->GetType() != TilePiece::TYPE_NONE)
	{
		Pipe* pipe = dynamic_cast<Pipe*>(tile);
		if (pipe != nullptr)
		{
			// Draw tile's Background color
			g.setColour(GetTileColourForLevel(Controller::GetInstance()->GetDifficultyLevel()));
			g.fillRect(origin.getX(), origin.getY(), m_tileSize, m_tileSize);

			juce::Line<int> line;
			float pipeThickness = m_tileSize / 3.5f;
			g.setColour(juce::Colours::black);

			// Ellipse rect used to have nice rounded corners in the elbow pipes.
			int halfTile = m_tileSize / 2;
			juce::Rectangle<float> elbowJoint(	origin.getX() + halfTile - (pipeThickness / 2.0f),
												origin.getY() + halfTile - (pipeThickness / 2.0f),
												pipeThickness, 
												pipeThickness);

			switch (pipe->GetType())
			{
			case TilePiece::TYPE_START_N:
				{
					line = juce::Line<int>(	origin.getX() + halfTile,
											origin.getY(),
											origin.getX() + halfTile,
											origin.getY() + halfTile);
					g.drawLine(line.toFloat(), pipeThickness);
					g.fillEllipse(elbowJoint);
				}
				break;

			case TilePiece::TYPE_START_S:
				{
					line = juce::Line<int>(	origin.getX() + halfTile,
											origin.getY() + halfTile,
											origin.getX() + halfTile,
											origin.getY() + m_tileSize);
					g.drawLine(line.toFloat(), pipeThickness);
					g.fillEllipse(elbowJoint);
				}
				break;

			case TilePiece::TYPE_START_E:
				{
					line = juce::Line<int>(	origin.getX() + halfTile,
											origin.getY() + halfTile,
											origin.getX() + m_tileSize,
											origin.getY() + halfTile);
					g.drawLine(line.toFloat(), pipeThickness);
					g.fillEllipse(elbowJoint);
				}
				break;

			case TilePiece::TYPE_START_W:
				{
					line = juce::Line<int>(	origin.getX() + halfTile,
											origin.getY() + halfTile,
											origin.getX(), 
											origin.getY() + halfTile);
					g.drawLine(line.toFloat(), pipeThickness);
					g.fillEllipse(elbowJoint);
				}
				break;

			case TilePiece::TYPE_VERTICAL:
				{
					line = juce::Line<int>(	origin.getX() + halfTile,
											origin.getY(), 
											origin.getX() + halfTile,
											origin.getY() + m_tileSize);
					g.drawLine(line.toFloat(), pipeThickness);
				}
				break;

			case TilePiece::TYPE_HORIZONTAL:
				{
					line = juce::Line<int>(	origin.getX(), 
											origin.getY() + halfTile,
											origin.getX() + m_tileSize,
											origin.getY() + halfTile);
					g.drawLine(line.toFloat(), pipeThickness);
				}
				break;

			case TilePiece::TYPE_NW_ELBOW:
				{
					line = juce::Line<int>(	origin.getX() + halfTile,
											origin.getY(), 
											origin.getX() + halfTile,
											origin.getY() + halfTile);
					g.drawLine(line.toFloat(), pipeThickness);
					line = juce::Line<int>(	origin.getX(), 
											origin.getY() + halfTile,
											origin.getX() + halfTile,
											origin.getY() + halfTile);
					g.drawLine(line.toFloat(), pipeThickness);
					g.fillEllipse(elbowJoint);
				}
				break;

			case TilePiece::TYPE_NE_ELBOW:
				{
					line = juce::Line<int>(	origin.getX() + halfTile,
											origin.getY(), 
											origin.getX() + halfTile,
											origin.getY() + halfTile);
					g.drawLine(line.toFloat(), pipeThickness);
					line = juce::Line<int>(	origin.getX() + halfTile,
											origin.getY() + halfTile,
											origin.getX() + m_tileSize,
											origin.getY() + halfTile);
					g.drawLine(line.toFloat(), pipeThickness);
					g.fillEllipse(elbowJoint);
				}
				break;

			case TilePiece::TYPE_SE_ELBOW:
				{
					line = juce::Line<int>(	origin.getX() + halfTile,
											origin.getY() + halfTile,
											origin.getX() + halfTile,
											origin.getY() + m_tileSize);
					g.drawLine(line.toFloat(), pipeThickness);
					line = juce::Line<int>(	origin.getX() + halfTile,
											origin.getY() + halfTile,
											origin.getX() + m_tileSize,
											origin.getY() + halfTile);
					g.drawLine(line.toFloat(), pipeThickness);
					g.fillEllipse(elbowJoint);
				}
				break;

			case TilePiece::TYPE_SW_ELBOW:
				{
					line = juce::Line<int>(	origin.getX() + halfTile,
											origin.getY() + halfTile,
											origin.getX() + halfTile,
											origin.getY() + m_tileSize);
					g.drawLine(line.toFloat(), pipeThickness);
					line = juce::Line<int>(	origin.getX(), 
											origin.getY() + halfTile,
											origin.getX() + halfTile,
											origin.getY() + halfTile);
					g.drawLine(line.toFloat(), pipeThickness);
					g.fillEllipse(elbowJoint);
				}
				break;

			case TilePiece::TYPE_CROSS:
				{
					Cross* crossTile = dynamic_cast<Cross*>(pipe);
					if (crossTile->GetBackgroundWay() == Cross::WAY_HORIZONTAL)
					{
						// Draw horizontal pipe first. 
						// Vertical pipe will be drawn in DrawCrossSecondWay()
						line = juce::Line<int>(	origin.getX(), 
												origin.getY() + halfTile,
												origin.getX() + m_tileSize,
												origin.getY() + halfTile);
					}
					else
					{
						// Draw vertica pipe first. 
						// Horizontal pipe will be drawn in DrawCrossSecondWay()
						line = juce::Line<int>(	origin.getX() + halfTile,
												origin.getY(), 
												origin.getX() + halfTile,
												origin.getY() + m_tileSize);
					}

					g.drawLine(line.toFloat(), pipeThickness);
				}
				break;

			default:
				break;
			}
		}
	}
}

void MainComponent::DrawOoze(TilePiece* tile, juce::Point<int> origin, juce::Graphics& g)
{
	if (tile->GetType() != TilePiece::TYPE_NONE)
	{
		Pipe* pipe = dynamic_cast<Pipe*>(tile);
		if ((pipe != nullptr) &&
			(!pipe->IsEmpty()))
		{
			static const float oozealfThickness = OOZE_THICKNESS / 2.0f;
			bool overHalf(pipe->GetOozeLevel() >= 50.0f);
			bool underHalf(pipe->GetOozeLevel() < 50.0f);
			int fill = static_cast<int>(m_tileSize * pipe->GetOozeLevel() / MAX_OOZE_LEVEL);

			juce::Line<int> line;
			g.setColour(juce::Colours::limegreen);

			// Ellipse rect used to have nice rounded corners in the elbow pipes.
			int halfTile = m_tileSize / 2;
			juce::Rectangle<float> elbowJoint(	origin.getX() + halfTile - oozealfThickness,
												origin.getY() + halfTile - oozealfThickness,
												OOZE_THICKNESS, 
												OOZE_THICKNESS);

			switch (pipe->GetType())
			{
			case TilePiece::TYPE_START_N:
				{
					if (overHalf)
					{
						line = juce::Line<int>(	origin.getX() + halfTile,
												origin.getY() + m_tileSize - fill,
												origin.getX() + halfTile,
												origin.getY() + halfTile);
						g.drawLine(line.toFloat(), OOZE_THICKNESS);
						g.fillEllipse(elbowJoint);
					}
				}
				break;

			case TilePiece::TYPE_START_S:
				{
					if (overHalf)
					{
						line = juce::Line<int>(	origin.getX() + halfTile,
												origin.getY() + halfTile,
												origin.getX() + halfTile,
												origin.getY() + fill);
						g.drawLine(line.toFloat(), OOZE_THICKNESS);
						g.fillEllipse(elbowJoint);
					}
				}
				break;

			case TilePiece::TYPE_START_E:
				{
					if (overHalf)
					{
						line = juce::Line<int>(	origin.getX() + halfTile,
												origin.getY() + halfTile,
												origin.getX() + fill, 
												origin.getY() + halfTile);
						g.drawLine(line.toFloat(), OOZE_THICKNESS);
						g.fillEllipse(elbowJoint);
					}
				}
				break;

			case TilePiece::TYPE_START_W:
				{
					if (overHalf)
					{
						line = juce::Line<int>(	origin.getX() + m_tileSize - fill,
												origin.getY() + halfTile,
												origin.getX() + halfTile,
												origin.getY() + halfTile);
						g.drawLine(line.toFloat(), OOZE_THICKNESS);
						g.fillEllipse(elbowJoint);
					}
				}
				break;

			case TilePiece::TYPE_VERTICAL:
				{
					if (pipe->GetFlowDirection() == Pipe::DIR_N)
						line = juce::Line<int>(	origin.getX() + halfTile,
												origin.getY() + m_tileSize - fill,
												origin.getX() + halfTile,
												origin.getY() + m_tileSize);
					else if (pipe->GetFlowDirection() == Pipe::DIR_S)
						line = juce::Line<int>(	origin.getX() + halfTile,
												origin.getY(), 
												origin.getX() + halfTile,
												origin.getY() + fill);

					g.drawLine(line.toFloat(), OOZE_THICKNESS);
				}
				break;

			case TilePiece::TYPE_HORIZONTAL:
				{
					if (pipe->GetFlowDirection() == Pipe::DIR_E)
						line = juce::Line<int>(	origin.getX(), 
												origin.getY() + halfTile,
												origin.getX() + fill, 
												origin.getY() + halfTile);
					else if (pipe->GetFlowDirection() == Pipe::DIR_W)
						line = juce::Line<int>(	origin.getX() + m_tileSize - fill,
												origin.getY() + halfTile,
												origin.getX() + m_tileSize,
												origin.getY() + halfTile);

					g.drawLine(line.toFloat(), OOZE_THICKNESS);
				}
				break;

			case TilePiece::TYPE_NW_ELBOW:
				{
					if (pipe->GetFlowDirection() == Pipe::DIR_N)
					{
						if (underHalf)
						{
							line = juce::Line<int>(	origin.getX(), 
													origin.getY() + halfTile,
													origin.getX() + fill, 
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
						}
						else
						{
							line = juce::Line<int>(	origin.getX(), 
													origin.getY() + halfTile,
													origin.getX() + halfTile,
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							line = juce::Line<int>(	origin.getX() + halfTile,
													origin.getY() + m_tileSize - fill,
													origin.getX() + halfTile,
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							g.fillEllipse(elbowJoint);
						}
					}
					else if (pipe->GetFlowDirection() == Pipe::DIR_W)
					{
						if (underHalf)
						{
							line = juce::Line<int>(	origin.getX() + halfTile,
													origin.getY(), 
													origin.getX() + halfTile,
													origin.getY() + fill);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
						}
						else
						{
							line = juce::Line<int>(	origin.getX() + halfTile,
													origin.getY(), 
													origin.getX() + halfTile,
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							line = juce::Line<int>(	origin.getX() + m_tileSize - fill,
													origin.getY() + halfTile,
													origin.getX() + halfTile,
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							g.fillEllipse(elbowJoint);
						}
					}
				}
				break;

			case TilePiece::TYPE_NE_ELBOW:
				{
					if (pipe->GetFlowDirection() == Pipe::DIR_N)
					{
						if (underHalf)
						{
							line = juce::Line<int>(	origin.getX() + m_tileSize - fill,
													origin.getY() + halfTile, 
													origin.getX() + m_tileSize,
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
						}
						else
						{
							line = juce::Line<int>(	origin.getX() + halfTile, 
													origin.getY() + halfTile, 
													origin.getX() + m_tileSize,
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							line = juce::Line<int>(	origin.getX() + halfTile, 
													origin.getY() + m_tileSize - fill,
													origin.getX() + halfTile, 
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							g.fillEllipse(elbowJoint);
						}
					}
					else if (pipe->GetFlowDirection() == Pipe::DIR_E)
					{
						if (underHalf)
						{
							line = juce::Line<int>(	origin.getX() + halfTile, 
													origin.getY(), 
													origin.getX() + halfTile, 
													origin.getY() + fill);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
						}
						else
						{
							line = juce::Line<int>(	origin.getX() + halfTile, 
													origin.getY(), 
													origin.getX() + halfTile, 
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							line = juce::Line<int>(	origin.getX() + halfTile, 
													origin.getY() + halfTile, 
													origin.getX() + fill, 
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							g.fillEllipse(elbowJoint);
						}
					}
				}
				break;

			case TilePiece::TYPE_SE_ELBOW:
				{
					if (pipe->GetFlowDirection() == Pipe::DIR_S)
					{
						if (underHalf)
						{
							line = juce::Line<int>(	origin.getX() + m_tileSize - fill,
													origin.getY() + halfTile, 
													origin.getX() + m_tileSize,
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
						}
						else
						{
							line = juce::Line<int>(	origin.getX() + halfTile, 
													origin.getY() + halfTile, 
													origin.getX() + m_tileSize,
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							line = juce::Line<int>(	origin.getX() + halfTile, 
													origin.getY() + halfTile, 
													origin.getX() + halfTile, 
													origin.getY() + fill);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							g.fillEllipse(elbowJoint);
						}
					}
					else if (pipe->GetFlowDirection() == Pipe::DIR_E)
					{
						if (underHalf)
						{
							line = juce::Line<int>(	origin.getX() + halfTile, 
													origin.getY() + m_tileSize - fill,
													origin.getX() + halfTile, 
													origin.getY() + m_tileSize);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
						}
						else
						{
							line = juce::Line<int>(	origin.getX() + halfTile, 
													origin.getY() + halfTile, 
													origin.getX() + halfTile, 
													origin.getY() + m_tileSize);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							line = juce::Line<int>(	origin.getX() + halfTile, 
													origin.getY() + halfTile, 
													origin.getX() + fill, 
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							g.fillEllipse(elbowJoint);
						}
					}
				}
				break;

			case TilePiece::TYPE_SW_ELBOW:
				{
					if (pipe->GetFlowDirection() == Pipe::DIR_S)
					{
						if (underHalf)
						{
							line = juce::Line<int>(	origin.getX(), 
													origin.getY() + halfTile, 
													origin.getX() + fill, 
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
						}
						else
						{
							line = juce::Line<int>(	origin.getX(), 
													origin.getY() + halfTile, 
													origin.getX() + halfTile, 
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							line = juce::Line<int>(	origin.getX() + halfTile, 
													origin.getY() + halfTile, 
													origin.getX() + halfTile, 
													origin.getY() + fill);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							g.fillEllipse(elbowJoint);
						}
					}
					else if (pipe->GetFlowDirection() == Pipe::DIR_W)
					{
						if (underHalf)
						{
							line = juce::Line<int>(	origin.getX() + halfTile, 
													origin.getY() + m_tileSize - fill,
													origin.getX() + halfTile, 
													origin.getY() + m_tileSize);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
						}
						else
						{
							line = juce::Line<int>(	origin.getX() + halfTile, 
													origin.getY() + halfTile, 
													origin.getX() + halfTile, 
													origin.getY() + m_tileSize);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							line = juce::Line<int>(	origin.getX() + m_tileSize - fill,
													origin.getY() + halfTile, 
													origin.getX() + halfTile, 
													origin.getY() + halfTile);
							g.drawLine(line.toFloat(), OOZE_THICKNESS);
							g.fillEllipse(elbowJoint);
						}
					}
				}
				break;

			case TilePiece::TYPE_CROSS:
				{
					Cross* crossTile = dynamic_cast<Cross*>(pipe);
					if (crossTile->GetBackgroundWay() == Cross::WAY_HORIZONTAL)
					{
						// Draw horizontally flowing ooze first.
						fill = static_cast<int>(m_tileSize * crossTile->GetOozeLevel(Cross::WAY_HORIZONTAL) / MAX_OOZE_LEVEL);
						if (fill > 0)
						{
							if (pipe->GetFlowDirection() == Pipe::DIR_E)
							{
								line = juce::Line<int>(	origin.getX(),
														origin.getY() + halfTile,
														origin.getX() + fill,
														origin.getY() + halfTile);
							}
							else
							{
								line = juce::Line<int>(	origin.getX() + m_tileSize - fill,
														origin.getY() + halfTile,
														origin.getX() + m_tileSize,
														origin.getY() + halfTile);
							}

							g.drawLine(line.toFloat(), OOZE_THICKNESS);
						}
					}

					else
					{
						// Draw vertically flowing ooze first.
						fill = static_cast<int>(m_tileSize * crossTile->GetOozeLevel(Cross::WAY_VERTICAL) / MAX_OOZE_LEVEL);
						if (fill > 0)
						{
							if (pipe->GetFlowDirection() == Pipe::DIR_N)
							{
								line = juce::Line<int>(	origin.getX() + halfTile,
														origin.getY() + m_tileSize - fill,
														origin.getX() + halfTile,
														origin.getY() + m_tileSize);
							}
							else
							{
								line = juce::Line<int>(	origin.getX() + halfTile,
														origin.getY(),
														origin.getX() + halfTile,
														origin.getY() + fill);
							}

							g.drawLine(line.toFloat(), OOZE_THICKNESS);
						}
					}
				}
				break;

			default:
				break;
			}
		}
	}
}

void MainComponent::DrawCrossSecondWay(TilePiece* tile, juce::Point<int> origin, juce::Graphics& g)
{
	if (tile->GetType() == TilePiece::TYPE_CROSS)
	{
		Cross* crossTile = dynamic_cast<Cross*>(tile);

		juce::Line<int> line;
		int halfTile = m_tileSize / 2;
		float pipeThickness = m_tileSize / 3.5f;
		int pipeHalfThickness = static_cast<int>(m_tileSize / 6.0f);
		g.setColour(juce::Colours::black);

		// Draw pipe first
		if (crossTile->GetBackgroundWay() == Cross::WAY_HORIZONTAL)
		{
			// Vertical pipe.
			line = juce::Line<int>(	origin.getX() + halfTile, 
									origin.getY(), 
									origin.getX() + halfTile, 
									origin.getY() + m_tileSize);
		}
		else
		{
			// Horizontal pipe. 
			line = juce::Line<int>(	origin.getX(), 
									origin.getY() + halfTile, 
									origin.getX() + m_tileSize,
									origin.getY() + halfTile);
		}
		g.drawLine(line.toFloat(), pipeThickness);

		// Little lines along the pipe, which make the separation between horizontal and vertical 
		// components of the cross-pipe more visually obvious.
		float littleLineThickness = (m_tileSize * 5.0f) / 70.0f;
		g.setColour(GetTileColourForLevel(Controller::GetInstance()->GetDifficultyLevel()));
		if (crossTile->GetBackgroundWay() == Cross::WAY_HORIZONTAL)
		{
			// Vertical little lines
			line = juce::Line<int>(	static_cast<int>(origin.getX() + halfTile - pipeHalfThickness - 1),
									origin.getY() + 1,
									static_cast<int>(origin.getX() + halfTile - pipeHalfThickness - 1),
									origin.getY() + m_tileSize - 1);
			g.drawLine(line.toFloat(), littleLineThickness);
			line = juce::Line<int>( static_cast<int>(origin.getX() + halfTile + pipeHalfThickness + 1),
									origin.getY() + 1,
									static_cast<int>(origin.getX() + halfTile + pipeHalfThickness + 1),
									origin.getY() + m_tileSize - 1);
			g.drawLine(line.toFloat(), littleLineThickness);
		}
		else
		{
			// Horizontal little lines
			line = juce::Line<int>(	origin.getX() + 1,
									origin.getY() + halfTile - static_cast<int>(pipeHalfThickness) - 1,
									origin.getX() + m_tileSize - 1,
									origin.getY() + halfTile - static_cast<int>(pipeHalfThickness) - 1);
			g.drawLine(line.toFloat(), littleLineThickness);
			line = juce::Line<int>(	origin.getX() + 1,
									origin.getY() + halfTile + static_cast<int>(pipeHalfThickness) + 1,
									origin.getX() + m_tileSize - 1,
									origin.getY() + halfTile + static_cast<int>(pipeHalfThickness) + 1);
			g.drawLine(line.toFloat(), littleLineThickness);
		}

		// Draw ooze
		g.setColour(juce::Colours::limegreen);
		int fill;

		// Vertically flowing ooze
		if (crossTile->GetBackgroundWay() == Cross::WAY_HORIZONTAL)
		{
			fill = static_cast<int>(m_tileSize * crossTile->GetOozeLevel(Cross::WAY_VERTICAL) / MAX_OOZE_LEVEL);
			if (fill > 0)
			{
				if (crossTile->GetFlowDirection() == Pipe::DIR_N)
				{
					line = juce::Line<int>(	origin.getX() + halfTile,
											origin.getY() + m_tileSize - fill,
											origin.getX() + halfTile,
											origin.getY() + m_tileSize);
				}
				else
				{
					line = juce::Line<int>(	origin.getX() + halfTile,
											origin.getY(),
											origin.getX() + halfTile,
											origin.getY() + fill);
				}

				g.drawLine(line.toFloat(), OOZE_THICKNESS);
			}
		}

		// Hoizontally flowing ooze
		else
		{
			fill = static_cast<int>(m_tileSize * crossTile->GetOozeLevel(Cross::WAY_HORIZONTAL) / MAX_OOZE_LEVEL);
			if (fill > 0)
			{
				if (crossTile->GetFlowDirection() == Pipe::DIR_E)
				{
					line = juce::Line<int>(	origin.getX(),
											origin.getY() + halfTile,
											origin.getX() + fill,
											origin.getY() + halfTile);
				}
				else
				{
					line = juce::Line<int>(	origin.getX() + m_tileSize - fill,
											origin.getY() + halfTile,
											origin.getX() + m_tileSize,
											origin.getY() + halfTile);
				}

				g.drawLine(line.toFloat(), OOZE_THICKNESS);
			}
		}
	}
}

void MainComponent::DrawTileDecoration(TilePiece* tile, juce::Point<int> origin, juce::Graphics& g)
{
	// Frame around tile
	g.setColour(juce::Colours::white);
	g.drawRect(origin.getX(), origin.getY(), m_tileSize, m_tileSize, 1);

	if (tile->GetType() != TilePiece::TYPE_NONE)
	{
		Pipe* pipe = dynamic_cast<Pipe*>(tile);
		if (pipe != nullptr)
		{
			// If this tile has an explosion on it, draw it.
			int exp = pipe->PopExplosion();
			if (exp > 0)
			{
				juce::Path starPath;
				int halfTile = m_tileSize / 2;
				starPath.addStar(juce::Point<float>(static_cast<float>(	origin.getX() + halfTile),
																		static_cast<float>(origin.getY() + halfTile)),
																		7,								// Number of peaks
																		static_cast<float>(exp * 4),	// Inner radius
																		static_cast<float>(exp * 8),	// Outer radius
																		static_cast<float>(exp * 2));	// Rotation angle
				g.setColour(juce::Colours::orangered);
				g.fillPath(starPath);
			}
		}
	}
}

void MainComponent::DrawSpill(juce::Point<int> origin, juce::Graphics& g)
{
	if (Controller::GetInstance()->GetState() == Controller::STATE_STOPPED)
	{
		Pipe* oozingPipe = dynamic_cast<Pipe*>(Controller::GetInstance()->GetBoard()->GetOozingTile());
		if (oozingPipe != nullptr)
		{
			int halfTile = m_tileSize / 2;
			int qt(m_tileSize / 4);
			Pipe::Direction spillDir = oozingPipe->GetFlowDirection();
			juce::Rectangle<int> bigRec;
			juce::Rectangle<int> smlRec;
			switch (spillDir)
			{
			case Pipe::DIR_N:
				bigRec = juce::Rectangle<int>(origin.getX(), origin.getY() - m_tileSize, m_tileSize, m_tileSize);
				smlRec = juce::Rectangle<int>(origin.getX() + qt, origin.getY() - halfTile, halfTile, halfTile);
				break;
			case Pipe::DIR_S:
				bigRec = juce::Rectangle<int>(origin.getX(), origin.getY() + m_tileSize, m_tileSize, m_tileSize);
				smlRec = juce::Rectangle<int>(origin.getX() + qt, origin.getY() + m_tileSize, halfTile, halfTile);
				break;
			case Pipe::DIR_E:
				bigRec = juce::Rectangle<int>(origin.getX() + m_tileSize, origin.getY(), m_tileSize, m_tileSize);
				smlRec = juce::Rectangle<int>(origin.getX() + m_tileSize, origin.getY() + qt, halfTile, halfTile);
				break;
			case Pipe::DIR_W:
				bigRec = juce::Rectangle<int>(origin.getX() - m_tileSize, origin.getY(), m_tileSize, m_tileSize);
				smlRec = juce::Rectangle<int>(origin.getX() - halfTile, origin.getY() + qt, halfTile, halfTile);
				break;
			}

			g.setColour(juce::Colour(0x88008000)); // transparent green
			g.fillEllipse(bigRec.toFloat());
			g.setColour(juce::Colours::limegreen);
			g.fillEllipse(smlRec.toFloat());
		}
	}
}

void MainComponent::DrawOozeMeter(juce::Point<int> origin, juce::Graphics& g)
{
	Board* board(Controller::GetInstance()->GetBoard());

	// Draw empty vial (background)
	//static constexpr int vialHeight = 118;
	int vialHeight = static_cast<int>(getLocalBounds().getHeight() / 5.2542f);
	juce::Rectangle<int> vialRect(origin.getX(), origin.getY(), 22, vialHeight);
	g.setColour(juce::Colours::black);
	g.fillRect(vialRect);

	// Ooze inside the vial.
	g.setColour(juce::Colours::limegreen);
	int oozeMaxHeight = vialHeight - 6;
	int oozeHeight;
	if (m_countDown > 0)
	{
		// Starts at 0, goes to vialHeight		
		oozeHeight = static_cast<int>(oozeMaxHeight - ((m_countDown * oozeMaxHeight) / Controller::GetInstance()->GetCurrentCountdown()));
	}
	else if (board->GetScoreValue() < Controller::MIN_SCORE_TO_ADVANCE)
	{
		// Starts at vialHeight, goes to 0.
		oozeHeight = static_cast<int>(((Controller::MIN_SCORE_TO_ADVANCE - (board->GetScoreValue())) * oozeMaxHeight) / Controller::MIN_SCORE_TO_ADVANCE);
	}
	else
	{
		// Full yellow vial.
		g.setColour(juce::Colours::yellow);
		oozeHeight = oozeMaxHeight;
	}
	g.fillRect(juce::Rectangle<int>(origin.getX() + 3, origin.getY() + 3 + oozeMaxHeight - oozeHeight, 16, oozeHeight));

	// Vial outline and markings.
	g.setColour(juce::Colours::black);
	g.drawLine(origin.getX() + 15.0f, 1.0f + origin.getY() + vialHeight * 0.25f,	origin.getX() + 22.0f, 1.0f + origin.getY() + vialHeight * 0.25f,	2.0f);
	g.drawLine(origin.getX() + 10.0f, 1.0f + origin.getY() + vialHeight * 0.5f,		origin.getX() + 22.0f, 1.0f + origin.getY() + vialHeight * 0.5f,	2.0f);
	g.drawLine(origin.getX() + 15.0f, 1.0f + origin.getY() + vialHeight * 0.75f,	origin.getX() + 22.0f, 1.0f + origin.getY() + vialHeight * 0.75f,	2.0f);
	g.setColour(juce::Colours::white);
	g.drawLine(origin.getX() + 15.0f, origin.getY() + vialHeight * 0.25f,	origin.getX() + 22.0f,	origin.getY() + vialHeight * 0.25f,	1.0f);
	g.drawLine(origin.getX() + 10.0f, origin.getY() + vialHeight * 0.5f,	origin.getX() + 22.0f,	origin.getY() + vialHeight * 0.5f,	1.0f);
	g.drawLine(origin.getX() + 15.0f, origin.getY() + vialHeight * 0.75f,	origin.getX() + 22.0f,	origin.getY() + vialHeight * 0.75f,	1.0f);
	g.drawRect(vialRect);
}

void MainComponent::DrawBombs(juce::Point<int> p, juce::Graphics& g)
{
	Board* board(Controller::GetInstance()->GetBoard());
	int halfTile = m_tileSize / 2;

	for (int i = 0; i < Board::MAX_NUM_BOMBS; i++)
	{
		// Draw the bombs that are still available
		if (i < board->GetNumBombs())
		{
			g.setColour(juce::Colours::red);
			g.fillEllipse(juce::Rectangle<int>(p.getX() + i * (m_tileSize - 1), p.getY(), halfTile, halfTile).toFloat());
		}

		// Draw the one used up bomb, which will soon become available again.
		else if (i == board->GetNumBombs())
		{
			g.setColour(juce::Colour(static_cast<juce::uint8>(67 + board->GetPercentUntilFreeBomb()), 67, 67));
			g.fillEllipse(juce::Rectangle<int>(p.getX() + i * (m_tileSize - 1), p.getY(), halfTile, halfTile).toFloat());
		}

		g.setColour(juce::Colours::white);
		g.drawEllipse(juce::Rectangle<int>(p.getX() + i * (m_tileSize - 1), p.getY(), halfTile, halfTile).toFloat(), 1.0f);
	}
}

void MainComponent::DrawFastForwardButton(juce::Graphics& g)
{
	float radius = 13;
	juce::Point<float> origin(m_fastForwardButtonRect.getX() + 24.0f, m_fastForwardButtonRect.getY() + 23.0f);

	juce::Path ffwdPath;
	ffwdPath.addPolygon(juce::Point<float>(static_cast<float>(origin.getX()), static_cast<float>(origin.getY())), 3, radius, -0.52f);
	ffwdPath.addPolygon(juce::Point<float>(origin.getX() + 19.0f, static_cast<float>(origin.getY())), 3, radius, -0.52f);

	float thickness = 1.5f;
	juce::Colour iconColour(juce::Colours::grey);
	juce::Colour frameColour(juce::Colour(27, 27, 27));
	if (Controller::GetInstance()->GetFastForward())
	{
		thickness = 2.5f;
		iconColour = juce::Colours::red;
		frameColour = juce::Colours::black;
	}

	// Draw ff icon (two little triangles)
	g.setColour(iconColour);
	g.fillPath(ffwdPath);
	g.setColour(juce::Colours::white);
	g.strokePath(ffwdPath, juce::PathStrokeType(thickness, juce::PathStrokeType::curved));

	// Frame around button
	g.setColour(frameColour);
	g.drawRect(m_fastForwardButtonRect.toFloat(), thickness);
}
