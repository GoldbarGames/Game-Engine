#include "SoundManager.h"

SoundManager::SoundManager()
{
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
	currentBGM = Mix_LoadMUS("bgm/Witchs_Waltz.ogg");

	volArray = { 0, 30, 60, 90, MIX_MAX_VOLUME };
}

SoundManager::~SoundManager()
{
	if (currentBGM != nullptr)
		Mix_FreeMusic(currentBGM);

	Mix_Quit();
}

void SoundManager::PlayBGM(std::string bgm, bool loop)
{
	if (currentBGM != nullptr)
		Mix_FreeMusic(currentBGM);

	bgm = "bgm/" + bgm + ".ogg";
	currentBGM = Mix_LoadMUS(bgm.c_str());

	if (currentBGM != nullptr)
	{
		if (loop)
			Mix_PlayMusic(currentBGM, -1);
		else
			Mix_PlayMusic(currentBGM, 1);
	}
		
}

void SoundManager::StopBGM()
{
	Mix_HaltMusic();
}

void SoundManager::FadeInBGM(std::string bgm, Uint32 duration, bool loop)
{
	if (currentBGM != nullptr)
		Mix_FreeMusic(currentBGM);

	bgm = "bgm/" + bgm + ".ogg";
	currentBGM = Mix_LoadMUS(bgm.c_str());

	if (currentBGM != nullptr)
	{
		if (loop)
			Mix_FadeInMusic(currentBGM, -1, duration);
		else
			Mix_FadeInMusic(currentBGM, 1, duration);
	}		
}

void SoundManager::FadeOutBGM(Uint32 duration)
{
	Mix_FadeOutMusic(duration);
}

void SoundManager::SetVolumeBGM(int index)
{
	bgmVolumeIndex = index;	
	volumeBGM = volArray[index];
	Mix_VolumeMusic(volumeBGM);
}

Uint32 SoundManager::GetVolumeBGM()
{
	return volumeBGM;
}

void SoundManager::PlaySound(std::string sound, int channel)
{
	// Don't do anything here, to avoid memory leaks
	if (channel < 0)
		return;

	if (sounds[channel] != nullptr)
	{
		Mix_FreeChunk(sounds[channel]);
	}

	sound = "se/" + sound + ".wav";
	sounds[channel] = Mix_LoadWAV(sound.c_str());
	Mix_VolumeChunk(sounds[channel], volumeSound);
	Mix_PlayChannel(channel, sounds[channel], 0);
}

void SoundManager::SetVolumeSound(int index)
{
	soundVolumeIndex = index;
	volumeSound = volArray[index];
}

Uint32 SoundManager::GetVolumeSound()
{
	return volumeSound;
}