#include "doom_dc32.h"

#include <string.h>
#include "audioPwm.h"
#include "deh_str.h"
#include "doom/sounds.h"
#include "doomtype.h"
#include "i_sound.h"
#include "w_wad.h"
#include "z_zone.h"

#ifndef DOOM_DC32_ENABLE_AUDIO
#define DOOM_DC32_ENABLE_AUDIO 0
#endif

#define DOOM_DC32_AUDIO_RATE 22050u
#define DOOM_DC32_AUDIO_WRITE_LIMIT 768u
#define ADPCM_BLOCK_SIZE 128
#define ADPCM_SAMPLES_PER_BLOCK_SIZE 249

#if DOOM_DC32_ENABLE_AUDIO

struct DoomDc32SoundChannel {
	const uint8_t *data;
	const uint8_t *dataEnd;
	uint32_t offset;
	uint32_t step;
	uint8_t left;
	uint8_t right;
	uint8_t decompressedSize;
	uint8_t alpha256;
	int lastSample;
	int8_t decompressed[ADPCM_SAMPLES_PER_BLOCK_SIZE];
};

static struct DoomDc32SoundChannel mChannels[NUM_SOUND_CHANNELS];
static boolean mSoundInitialized;
static boolean mUseSfxPrefix;

static const uint16_t mStepTable[89] = {
	7, 8, 9, 10, 11, 12, 13, 14,
	16, 17, 19, 21, 23, 25, 28, 31,
	34, 37, 41, 45, 50, 55, 60, 66,
	73, 80, 88, 97, 107, 118, 130, 143,
	157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658,
	724, 796, 876, 963, 1060, 1166, 1282, 1411,
	1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
	3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
	7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
	32767
};

static const int8_t mIndexTable[8] = {
	-1, -1, -1, -1, 2, 4, 6, 8
};

static int doomDc32ClipInt(int value, int minValue, int maxValue)
{
	if (value < minValue)
		return minValue;
	if (value > maxValue)
		return maxValue;
	return value;
}

static int doomDc32AdpcmDecodeBlock(int8_t *out, const uint8_t *in, int inSize)
{
	int samples = 1;
	int chunks;
	int32_t pcm;
	int index;

	if (inSize < 4)
		return 0;
	pcm = (int16_t)(in[0] | (in[1] << 8));
	*out++ = (int8_t)(pcm >> 8);
	index = in[2];
	if (index < 0 || index > 88 || in[3])
		return 0;
	inSize -= 4;
	in += 4;
	chunks = inSize / 4;
	samples += chunks * 8;
	while (chunks--) {
		for (int i = 0; i < 4; i++) {
			int step = mStepTable[index];
			int delta = step >> 3;
			uint8_t nibble = *in & 0xfu;

			if (nibble & 1)
				delta += step >> 2;
			if (nibble & 2)
				delta += step >> 1;
			if (nibble & 4)
				delta += step;
			if (nibble & 8)
				delta = -delta;
			pcm += delta;
			index = doomDc32ClipInt(index + mIndexTable[nibble & 7u], 0, 88);
			pcm = doomDc32ClipInt(pcm, -32768, 32767);
			out[i * 2] = (int8_t)(pcm >> 8);

			step = mStepTable[index];
			delta = step >> 3;
			nibble = *in >> 4;
			if (nibble & 1)
				delta += step >> 2;
			if (nibble & 2)
				delta += step >> 1;
			if (nibble & 4)
				delta += step;
			if (nibble & 8)
				delta = -delta;
			pcm += delta;
			index = doomDc32ClipInt(index + mIndexTable[nibble & 7u], 0, 88);
			pcm = doomDc32ClipInt(pcm, -32768, 32767);
			out[i * 2 + 1] = (int8_t)(pcm >> 8);
			in++;
		}
		out += 8;
	}
	return samples;
}

static void doomDc32StopChannel(int channel)
{
	if ((uint32_t)channel < NUM_SOUND_CHANNELS)
		mChannels[channel].decompressedSize = 0;
}

static boolean doomDc32ChannelPlaying(int channel)
{
	return (uint32_t)channel < NUM_SOUND_CHANNELS && mChannels[channel].decompressedSize != 0;
}

static void doomDc32DecompressChannel(struct DoomDc32SoundChannel *channel)
{
	int blockSize;
	int decoded;

	if (channel->data >= channel->dataEnd) {
		channel->decompressedSize = 0;
		return;
	}
	blockSize = (int)(channel->dataEnd - channel->data);
	if (blockSize > ADPCM_BLOCK_SIZE)
		blockSize = ADPCM_BLOCK_SIZE;
	decoded = doomDc32AdpcmDecodeBlock(channel->decompressed, channel->data, blockSize);
	if (decoded <= 0 || decoded > ADPCM_SAMPLES_PER_BLOCK_SIZE) {
		channel->decompressedSize = 0;
		return;
	}
	channel->decompressedSize = (uint8_t)decoded;
	channel->data += blockSize;
}

static void doomDc32SfxName(const sfxinfo_t *sfx, char *dst, uint32_t dstLen)
{
	const char *name;
	uint32_t pos = 0;

	if (sfx->link)
		sfx = sfx->link;
	name = DEH_String(sfx->name);
	if (!dstLen)
		return;
	if (mUseSfxPrefix && dstLen > 2u) {
		dst[pos++] = 'd';
		dst[pos++] = 's';
	}
	while (*name && pos + 1u < dstLen)
		dst[pos++] = *name++;
	dst[pos] = 0;
}

static boolean doomDc32InitChannelForSfx(struct DoomDc32SoundChannel *channel,
	const sfxinfo_t *sfxinfo, int pitch)
{
	int lumpNum = sfx_mut(sfxinfo)->lumpnum;
	int lumpLen;
	const uint8_t *data;
	uint32_t sampleRate;
	uint64_t rate;

	if (lumpNum < 0)
		return false;
	lumpLen = W_LumpLength(lumpNum);
	data = W_CacheLumpNum(lumpNum, PU_STATIC);
	if (lumpLen < 8 || data[0] != 0x03 || data[1] != 0x80)
		return false;
	sampleRate = (uint32_t)data[2] | ((uint32_t)data[3] << 8);
	if (!sampleRate)
		return false;
	memset(channel, 0, sizeof(*channel));
	channel->data = data + 8;
	channel->dataEnd = data + lumpLen;
	rate = sampleRate;
	if (pitch > 0 && pitch != NORM_PITCH)
		rate = (rate * (uint32_t)pitch) / NORM_PITCH;
	channel->step = (uint32_t)((rate << 16) / DOOM_DC32_AUDIO_RATE);
	if (!channel->step)
		channel->step = 1;
	channel->alpha256 = (uint8_t)((256u * 201u * sampleRate) /
		(201u * sampleRate + 64u * DOOM_DC32_AUDIO_RATE));
	doomDc32DecompressChannel(channel);
	channel->offset = 0;
	return channel->decompressedSize != 0;
}

static void doomDc32UpdateSoundParams(int channel, int vol, int sep)
{
	int left;
	int right;

	if (!mSoundInitialized || (uint32_t)channel >= NUM_SOUND_CHANNELS)
		return;
	left = ((254 - sep) * vol) / 127;
	right = (sep * vol) / 127;
	mChannels[channel].left = (uint8_t)doomDc32ClipInt(left, 0, 255);
	mChannels[channel].right = (uint8_t)doomDc32ClipInt(right, 0, 255);
}

static int doomDc32StartSound(should_be_const sfxinfo_t *sfxinfo, int channel, int vol, int sep, int pitch)
{
	if (!mSoundInitialized || (uint32_t)channel >= NUM_SOUND_CHANNELS)
		return -1;
	doomDc32StopChannel(channel);
	if (!doomDc32InitChannelForSfx(&mChannels[channel], sfxinfo, pitch))
		return -1;
	doomDc32UpdateSoundParams(channel, vol, sep);
	return channel;
}

static int doomDc32NextChannelSample(int channel)
{
	struct DoomDc32SoundChannel *ch = &mChannels[channel];
	uint32_t offsetEnd;
	int raw;
	int volume;
	int sample;

	if (!ch->decompressedSize)
		return 0;
	offsetEnd = (uint32_t)ch->decompressedSize << 16;
	if (ch->offset >= offsetEnd) {
		ch->offset = 0;
		doomDc32DecompressChannel(ch);
		if (!ch->decompressedSize)
			return 0;
		offsetEnd = (uint32_t)ch->decompressedSize << 16;
	}
	raw = ch->decompressed[ch->offset >> 16];
	ch->lastSample = ((256 - ch->alpha256) * ch->lastSample + ch->alpha256 * raw) / 256;
	sample = ch->lastSample;
	ch->offset += ch->step;
	if (ch->offset >= offsetEnd) {
		ch->offset -= offsetEnd;
		doomDc32DecompressChannel(ch);
		if (ch->decompressedSize) {
			offsetEnd = (uint32_t)ch->decompressedSize << 16;
			if (ch->offset >= offsetEnd)
				ch->offset = 0;
		}
	}
	volume = ((int)ch->left + (int)ch->right) >> 1;
	return (sample * volume) / 256;
}

static void doomDc32UpdateSound(void)
{
	uint32_t samples = 0;

	if (!mSoundInitialized)
		return;
	while (samples < DOOM_DC32_AUDIO_WRITE_LIMIT && audioPwmPcmCanWrite()) {
		int mix = 0;

		for (int ch = 0; ch < NUM_SOUND_CHANNELS; ch++)
			mix += doomDc32NextChannelSample(ch);
		mix = doomDc32ClipInt(mix, -128, 127);
		if (!audioPwmPcmWriteU8((uint8_t)(mix + 128)))
			break;
		samples++;
	}
}

static int doomDc32GetSfxLumpNum(should_be_const sfxinfo_t *sfxinfo)
{
	char name[9];

	doomDc32SfxName(sfxinfo, name, sizeof(name));
	return W_CheckNumForName(name);
}

static void doomDc32PrecacheSounds(should_be_const sfxinfo_t *sounds, int numSounds)
{
	(void)sounds;
	(void)numSounds;
}

static void doomDc32ShutdownSound(void)
{
	if (!mSoundInitialized)
		return;
	mSoundInitialized = false;
	memset(mChannels, 0, sizeof(mChannels));
	audioPwmPcmStop();
}

void doomDc32SoundStopAll(void)
{
	doomDc32ShutdownSound();
}

static boolean doomDc32InitSound(boolean useSfxPrefix)
{
	mUseSfxPrefix = useSfxPrefix;
	memset(mChannels, 0, sizeof(mChannels));
	audioPwmSetVolume(AUDIO_PWM_VOLUME_MAX);
	if (!audioPwmPcmStart(DOOM_DC32_AUDIO_RATE))
		return false;
	mSoundInitialized = true;
	return true;
}

#else

static boolean mAudioDisabledLogged;

static void doomDc32StopChannel(int channel)
{
	(void)channel;
}

static boolean doomDc32ChannelPlaying(int channel)
{
	(void)channel;
	return false;
}

static void doomDc32UpdateSoundParams(int channel, int vol, int sep)
{
	(void)channel;
	(void)vol;
	(void)sep;
}

static int doomDc32StartSound(should_be_const sfxinfo_t *sfxinfo, int channel, int vol, int sep, int pitch)
{
	(void)sfxinfo;
	(void)channel;
	(void)vol;
	(void)sep;
	(void)pitch;
	return -1;
}

static void doomDc32UpdateSound(void)
{
}

static int doomDc32GetSfxLumpNum(should_be_const sfxinfo_t *sfxinfo)
{
	(void)sfxinfo;
	return -1;
}

static void doomDc32PrecacheSounds(should_be_const sfxinfo_t *sounds, int numSounds)
{
	(void)sounds;
	(void)numSounds;
}

static void doomDc32ShutdownSound(void)
{
}

void doomDc32SoundStopAll(void)
{
}

static boolean doomDc32InitSound(boolean useSfxPrefix)
{
	(void)useSfxPrefix;
	if (!mAudioDisabledLogged && doomDc32Host && doomDc32Host->log) {
		doomDc32Host->log("DOOM: audio disabled in DC32 safe mode\n");
		mAudioDisabledLogged = true;
	}
	return false;
}

#endif

static snddevice_t mSoundDevices[] = { SNDDEVICE_SB };
sound_module_t sound_pico_module = {
	.sound_devices = mSoundDevices,
	.num_sound_devices = 1,
	.Init = doomDc32InitSound,
	.Shutdown = doomDc32ShutdownSound,
	.GetSfxLumpNum = doomDc32GetSfxLumpNum,
	.Update = doomDc32UpdateSound,
	.UpdateSoundParams = doomDc32UpdateSoundParams,
	.StartSound = doomDc32StartSound,
	.StopSound = doomDc32StopChannel,
	.SoundIsPlaying = doomDc32ChannelPlaying,
	.CacheSounds = doomDc32PrecacheSounds,
};

void I_SetOPLDriverVer(opl_driver_ver_t ver)
{
	(void)ver;
}

static boolean doomDc32MusicInit(void)
{
	if (doomDc32Host && doomDc32Host->log)
#if DOOM_DC32_ENABLE_AUDIO
		doomDc32Host->log("DOOM: OPL music disabled; SFX enabled\n");
#else
		doomDc32Host->log("DOOM: music disabled in DC32 safe mode\n");
#endif
	return false;
}

static void doomDc32MusicShutdown(void)
{
}

static void doomDc32MusicSetVolume(int volume)
{
	(void)volume;
}

static void doomDc32MusicPause(void)
{
}

static void doomDc32MusicResume(void)
{
}

static void *doomDc32MusicRegister(should_be_const void *data, int len)
{
	(void)data;
	(void)len;
	return NULL;
}

static void doomDc32MusicUnregister(void *handle)
{
	(void)handle;
}

static void doomDc32MusicPlay(void *handle, boolean looping)
{
	(void)handle;
	(void)looping;
}

static void doomDc32MusicStop(void)
{
}

static boolean doomDc32MusicIsPlaying(void)
{
	return false;
}

static const snddevice_t mMusicDevices[] = { SNDDEVICE_SB };
const music_module_t music_opl_module = {
	.sound_devices = mMusicDevices,
	.num_sound_devices = 1,
	.Init = doomDc32MusicInit,
	.Shutdown = doomDc32MusicShutdown,
	.SetMusicVolume = doomDc32MusicSetVolume,
	.PauseMusic = doomDc32MusicPause,
	.ResumeMusic = doomDc32MusicResume,
	.RegisterSong = doomDc32MusicRegister,
	.UnRegisterSong = doomDc32MusicUnregister,
	.PlaySong = doomDc32MusicPlay,
	.StopSong = doomDc32MusicStop,
	.MusicIsPlaying = doomDc32MusicIsPlaying,
	.Poll = NULL,
};
