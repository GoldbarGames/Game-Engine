#include "SoundManager.h"

SoundManager::SoundManager()
{
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
	currentBGM = Mix_LoadMUS("assets/bgm/Witchs_Waltz.ogg");

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

	const char * music = bgm.c_str();

	currentBGM = Mix_LoadMUS(music);

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
	Uint32 vol = volArray[index];
	Mix_VolumeMusic(vol);
	volumeBGM = vol;
}

Uint32 SoundManager::GetVolume()
{
	return volumeBGM;
}