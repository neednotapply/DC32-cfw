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
#include "MainComponent.h"
#include "Controller.h"


/**
 * Main application.
 */
class PipeDreamerApplication : public juce::JUCEApplication
{
public:
	/**
	 * Class constructor.
	 */
	PipeDreamerApplication()
	{
	}

	/** 
	 * Returns the application's name. 
	 */
	const juce::String getApplicationName() override 
	{
		return ProjectInfo::projectName; 
	}

	/** 
	 * Returns the application's version number. 
	 */
	const juce::String getApplicationVersion() override 
	{
		return ProjectInfo::versionString; 
	}
	
	/** 
	 * Returns false, multiple instances of the app are not allowed.
	 */
	bool moreThanOneInstanceAllowed() override 
	{ 
		return false;
	}

	/** 
	 * Called once when the application starts.
	 * After the method returns, the normal event-dispatch loop will be run 
	 * until the quit() method is called.
	 *
	 * @param commandLine	Parameter list, not including the name of the app.
	 */
	void initialise(const juce::String& commandLine) override
	{
		(void)commandLine;

		// TODO: think about useful commandline options
		// i.e.: starting level.

		// Store pointers to the MainWindow and Controller so we can delete them on shutdown.
		m_mainWindow.reset(new MainWindow(getApplicationName()));
		m_controller = Controller::GetInstance();
	}

	/**
	 * Called after the event-dispatch loop has been terminated, 
	 * to allow the application to clear up before exiting.
	 */
	void shutdown() override
	{
		// This deletes the unique_ptr.
		m_mainWindow = nullptr;
		delete m_controller;
	}

	/**
	 * Called when the operating system is trying to close the application.
	 */
	void systemRequestedQuit() override
	{
		quit();
	}

	/**
	 * Indicates that the user has tried to start up another instance of the app.
	 * This will be ignored.
	 */
	void anotherInstanceStarted(const juce::String& commandLine) override
	{
		(void)commandLine;
	}

	/**
	 * This class implements the desktop window that contains an instance of MainComponent.
	 */
	class MainWindow : public juce::DocumentWindow
	{
	public:
		/**
		 * Class constructor.
		 */
		MainWindow(juce::String name)
			: DocumentWindow(name, juce::Colours::black, DocumentWindow::allButtons)
		{
			setUsingNativeTitleBar(true);
			setContentOwned(new MainComponent(), true);

			// TODO: ResizableCornerComponent not working!
			setResizable(true, false);
			setResizeLimits(594, 414, 2560, 1440);
			centreWithSize(getWidth(), getHeight());

			setVisible(true);
		}

		/**
		 * This is called when the user tries to close this window.
		 * Just ask the app to quit when this happens.
		 */
		void closeButtonPressed() override
		{
			JUCEApplication::getInstance()->systemRequestedQuit();
		}

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
	};

private:
	/**
	 * Pointer to MainWindow instance.
	 */
	std::unique_ptr<MainWindow> m_mainWindow;

	/**
	 * Pointer to Controller singleton.
	 */
	Controller* m_controller;
};

/**
 * This macro generates the main() routine that launches the app.
 */
START_JUCE_APPLICATION(PipeDreamerApplication)
