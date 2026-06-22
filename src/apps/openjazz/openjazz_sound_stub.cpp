#include "io/sound.h"

static int gMusicVolume = MAX_VOLUME / 2;
static int gSoundVolume = MAX_VOLUME / 4;
static MusicTempo gMusicTempo = MusicTempo::NORMAL;

void openAudio()
{
}

void closeAudio()
{
}

void playMusic(const char *fileName, bool restart)
{
	(void)fileName;
	(void)restart;
}

void pauseMusic(bool pause)
{
	(void)pause;
}

void stopMusic()
{
}

int getMusicVolume()
{
	return gMusicVolume;
}

void setMusicVolume(int volume)
{
	gMusicVolume = volume;
}

MusicTempo getMusicTempo()
{
	return gMusicTempo;
}

void setMusicTempo(MusicTempo tempo)
{
	gMusicTempo = tempo;
}

bool loadSounds(const char *fileName)
{
	(void)fileName;
	return false;
}

void resampleSound(int index, const char *name, int rate)
{
	(void)index;
	(void)name;
	(void)rate;
}

void resampleSounds()
{
}

void freeSounds()
{
}

void playSound(SE::Type se)
{
	(void)se;
}

bool isSoundPlaying(SE::Type se)
{
	(void)se;
	return false;
}

int getSoundVolume()
{
	return gSoundVolume;
}

void setSoundVolume(int volume)
{
	gSoundVolume = volume;
}
