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


#include "Controller.h"
#include "Board.h"
#include "Queue.h"
#include "Randomizer.h"


// ---- Helper types and constants ----

const int Controller::MIN_SCORE_TO_ADVANCE(200);


/**
 * Singleton initialization.
 */
Controller* Controller::m_singleton = nullptr;


// --- Controller class implementation ---

Controller::Controller()
{
	jassert(m_singleton == nullptr);
	m_singleton = this;

	// Create board
	m_board.reset(new Board(10, 7));

	// Create queue
	m_queue.reset(new Queue(5));

	// Initialize randomizer and store pointer
	// to ensure it is deleted on shutdown.
	m_randomizer = Randomizer::GetInstance();


	// Initialize max score 
	InitApplicationProperties();

	// Init sounds.
	InitAudio();
}

Controller::~Controller()
{
	ShutdownAudio();

	delete m_randomizer;

	m_singleton = nullptr;
}

Controller* Controller::GetInstance()
{
	if (m_singleton == nullptr)
	{
		m_singleton = new Controller();
	}
	return m_singleton;
}

Controller::GameState Controller::GetState() const
{
	return m_state;
}

Board* Controller::GetBoard() const
{
	return m_board.get();
}

Queue* Controller::GetQueue() const
{
	return m_queue.get();
}

void Controller::InitApplicationProperties()
{
	// Locate or create the appropriate properties file.
	juce::PropertiesFile::Options options;
	options.applicationName = ProjectInfo::projectName;
	options.filenameSuffix = ".settings";
	options.osxLibrarySubFolder = "Application Support";
	options.folderName = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory).getChildFile(ProjectInfo::projectName).getFullPathName();
	options.storageFormat = juce::PropertiesFile::storeAsXML;
	m_appProperties.setStorageParameters(options);

	// If first time playing the game: initialize max score with a few
	// fun, dummy entries to entice the player to beat their score :)
	juce::StringPairArray allData = m_appProperties.getUserSettings()->getAllProperties();
	if (allData.size() == 0)
	{
		m_appProperties.getUserSettings()->setValue("31.10.1984@00:00:00;MADMX", 420);
		m_appProperties.getUserSettings()->setValue("01.07.1982@00:00:00;FLYNN", 1234);
		m_appProperties.getUserSettings()->setValue("18.04.2021@00:00:00;BERNS", 1750);

		allData = m_appProperties.getUserSettings()->getAllProperties();
	}

	// Generate list of cached score entries.
	juce::StringArray allkeys = allData.getAllKeys();
	for (int i = 0; i < allData.size(); i++)
	{
		juce::String name(allkeys[i]);
		int score = m_appProperties.getUserSettings()->getIntValue(allkeys[i]);

		m_scoreHash.push_back(std::pair<juce::String, int>(name, score));
	}

	// Sort the list by score, in descending order.
	std::sort(m_scoreHash.begin(), m_scoreHash.end(), HigherScoreThan);
}

std::vector<scoreEntry> Controller::GetAugmentedScoreHash(const scoreEntry& newEntry) const
{
	// Create a temporary copy of the scoreHash: we don't want to modify 
	// the actual hash yet. That can be done with SaveScoreEntry().
	std::vector<scoreEntry> listCopy(m_scoreHash);

	// Don't bother adding zero scores to the list.
	if (newEntry.second > 0)
	{
		listCopy.push_back(newEntry);
		std::sort(listCopy.begin(), listCopy.end(), HigherScoreThan);
	}

	return listCopy;
}

void Controller::SaveScoreEntry(const scoreEntry& newEntry)
{
	// Insert new entry into the cached list
	m_scoreHash.push_back(newEntry);
	std::sort(m_scoreHash.begin(), m_scoreHash.end(), HigherScoreThan);

	// ... and also into the properties file.
	m_appProperties.getUserSettings()->setValue(newEntry.first, newEntry.second);
}

bool Controller::HigherScoreThan(std::pair<juce::String, int> const &a, std::pair<juce::String, int> const &b)
{
	return a.second > b.second;
}

bool Controller::Pump()
{
	int oldScore = m_board->GetScoreValue();

	// Pump more ooze into the board!
	bool contained = m_board->Pump(GetCurrentOozePerPump());

	// Ooze is still contained in the pipeline.
	if (contained)
	{
		if ((oldScore < MIN_SCORE_TO_ADVANCE) &&
			(m_board->GetScoreValue() >= MIN_SCORE_TO_ADVANCE))
		{
			// The player just gained enough points 
			// to advance to the next level. Notify with a sound.
			QueueSound(SOUND_NOTIFY);
		}
	}
	
	// Ooze spill! 
	else
	{
		// Sound effect to trigger, depends on whether the player advanced to next level.
		SoundID soundID(SOUND_GAME_OVER);
		ScoreDetails details(GetScoreDetails());
		if (details.advance)
			soundID = SOUND_LEVEL_UP;

		// Trigger sound effect.
		QueueSound(soundID);

		// This round is over.
		m_state = STATE_STOPPED;
	}

	return contained;
}

Controller::ScoreDetails Controller::GetScoreDetails() const
{
	ScoreDetails details;
	details.score = m_board->GetScoreValue();

	// Carryover is the score gained from all previous levels.
	details.carryover = m_cumulativeScore;

	// Add level-based bonus. This mechanic helps ensure that players
	// who make it further into the game end up with higher score than 
	// players who just manage a very long pipe on level 1.
	details.bonus = 0;
	if (m_difficultyLevel > 1)
		details.bonus = m_difficultyLevel * m_difficultyLevel * 15;

	// Add score gained to the cumulative score.
	details.total = details.score + details.bonus + details.carryover;

	// If score is high enough, score window offers 
	// a button to continue to next level.
	details.level = m_difficultyLevel;
	details.advance = (details.score >= MIN_SCORE_TO_ADVANCE);

	return details;
}

int Controller::GetDifficultyLevel() const
{
	return m_difficultyLevel;
}

void Controller::Reset(Controller::Command cmd)
{
	// If re restart at lvl 1, clear total score
	if (cmd == Controller::CMD_RESTART)
	{
		m_difficultyLevel = 1;
		m_cumulativeScore = 0;
	}

	// Or advance to the next level
	else if (cmd == Controller::CMD_CONTINUE)
	{
		ScoreDetails details(GetScoreDetails());
		m_cumulativeScore = details.total;

		m_difficultyLevel += 1;
	}

	m_board->Reset();
	m_queue->Reset();
	m_fastForward = false;
	m_state = STATE_RUNNING;
}

float Controller::GetCurrentOozePerPump() const
{
	static const float oozePerLevel[] = {
		1.0F, // Level 1
		1.2F, // Level 2
		1.4F, // Level 3
		1.5F, // Level 4
		1.6F, // Level 5
		1.8F, // Level 6
		2.0F, // Level 7
		2.2F, // Level 8
		2.5F, // Level 9
		3.0F, // Level 10
		3.5F, // Level 11
		5.0F  // Level 12
	};

	// m_difficultyLevel starts at 1
	int arraySize = sizeof(oozePerLevel) / sizeof(*oozePerLevel);
	int level = m_difficultyLevel - 1;
	if (level >= arraySize)
		level = arraySize - 1;

	// If fast-forward button is currently toggled on, increase ooze per pump.
	if (m_fastForward)
		return oozePerLevel[level] * 10.0f;

	return oozePerLevel[level];
}

int Controller::GetCurrentCountdown() const
{
	static const int countdownPerLevel[] = {
		320,	// Level 1
		290,	// Level 2
		260,	// Level 3
		230,	// Level 4
		200,	// Level 5
		180,	// Level 6
		160,	// Level 7
		140,	// Level 8
		120,	// Level 9
		100,	// Level 10
		80,		// Level 11
		60		// Level 12
	};

	// m_difficultyLevel starts at 1
	int arraySize = sizeof(countdownPerLevel) / sizeof(*countdownPerLevel);
	int level = m_difficultyLevel - 1;
	if (level >= arraySize)
		level = arraySize - 1;

	return countdownPerLevel[level];
}

bool Controller::GetFastForward() const
{
	return m_fastForward;
}

void Controller::SetFastForward(bool fastForward)
{
	m_fastForward = fastForward;
}

void Controller::InitAudio()
{
	// Enable support for WAV files and other commom formats.
	juce::AudioFormatManager audioFormatManager;
	audioFormatManager.registerBasicFormats();

	// Stereo output.
	m_audioDeviceManager.initialiseWithDefaultDevices(0, 2);

	// Registers audio callback to be used.
	m_audioDeviceManager.addAudioCallback(&m_audioPlayer);

	juce::AudioIODevice* audioDevice = m_audioDeviceManager.getCurrentAudioDevice();
	if (audioDevice)
	{
		for (int i = SOUND_CLICK; i < SoundID::SOUND_MAX; i++)
		{
			std::unique_ptr<juce::InputStream> inputStream;

			SoundID sId = static_cast<SoundID>(i);
			switch (sId)
			{
				case SOUND_CLICK:
					inputStream = std::make_unique<juce::MemoryInputStream>(BinaryData::tap_wav, BinaryData::tap_wavSize, true);
					break;
				case SOUND_EXPLODE:
					inputStream = std::make_unique<juce::MemoryInputStream>(BinaryData::explode_wav, BinaryData::explode_wavSize, true);
					break;
				case SOUND_NOTIFY:
					inputStream = std::make_unique<juce::MemoryInputStream>(BinaryData::notify_wav, BinaryData::notify_wavSize, true);
					break;
				case SOUND_LEVEL_UP:
					inputStream = std::make_unique<juce::MemoryInputStream>(BinaryData::win_wav, BinaryData::win_wavSize, true);
					break;
				case SOUND_GAME_OVER:
					inputStream = std::make_unique<juce::MemoryInputStream>(BinaryData::lose_wav, BinaryData::lose_wavSize, true);
					break;
				default:
					break;
			}

			// Create a new AudioSource which gets its data from the binary stream above.
			SoundSource source(new juce::AudioFormatReaderSource(audioFormatManager.createReaderFor(std::move(inputStream)), true));

			// Store this source in a map to be easily found later, in Controller::PlaySound().
			m_soundSources.insert(std::make_pair(sId, std::move(source)));
		}

		// Audio is to be mixed by m_audioMixer and passed on to 
		// the AudioSourcePlayer, which then streams it to the AudioIODevice.
		m_audioPlayer.setSource(&m_audioMixer);

		// Start the AudioThread, which will lay dormant 
		// until woken up by calls to QueueSound().
		m_audioThread.startThread();
	}
}

void Controller::ShutdownAudio()
{
	// -- TODO: are these really needed?
	for (std::map<SoundID, SoundSource>::iterator iter = m_soundSources.begin(); iter != m_soundSources.end(); ++iter)
	{
		iter->second->releaseResources();
	}
	m_audioMixer.releaseResources();
	m_audioMixer.removeAllInputs();
	// --


	// Exception in CriticalSection::enter() without this.
	m_audioPlayer.setSource(nullptr);

	// Attempts to stop the thread running. The threadShouldExit() method will return true,
	// and notify() will be called in case the thread is currently waiting.
	m_audioThread.stopThread(2000);
}

void Controller::QueueSound(SoundID soundID)
{
	m_audioThread.QueueSound(soundID);
}

void Controller::PlaySound(SoundID soundID)
{
	juce::AudioIODevice* audioDevice = m_audioDeviceManager.getCurrentAudioDevice();
	if (audioDevice && (m_soundSources.count(soundID) != 0))
	{
		// Set playhead back to the start of the sample, 
		// and add it to the mixer if not already added before.
		// TODO: adding immediately plays it, so not added in InitAudio().
		m_soundSources.at(soundID)->setNextReadPosition(0);
		m_audioMixer.addInputSource(m_soundSources.at(soundID).get(), false);
	}
}


// ----------------------------------------
// --- AudioThread class implementation ---

Controller::AudioThread::AudioThread()
	:	juce::Thread("AudioThread")
{

}

void Controller::AudioThread::run()
{
	while (!threadShouldExit())
	{
		// Just a safety net to avoid constantly triggering the sound,
		// should the wait() call not have immediate effect. 
		// TODO: need this?
		if (m_soundID != SOUND_NONE)
		{
			Controller::GetInstance()->PlaySound(m_soundID);

			m_soundID = SOUND_NONE;
		}

		// Stop the AudioThread until another thread calls notify().
		wait(-1); 
	}
}

void Controller::AudioThread::QueueSound(SoundID soundID)
{
	// Wake up the AudioThread, and let it know 
	// which sound is to be triggered once run() is called.
	m_soundID = soundID;

	notify();
}
