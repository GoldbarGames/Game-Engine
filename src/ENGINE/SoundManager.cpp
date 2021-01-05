#include "leak_check.h"
#include "SoundManager.h"
#include <iostream>
#include <fstream>
#include <iterator>
#include <sstream>
#include "Game.h"
#include "Logger.h"
#include "SoundTest.h"

// TODO: When you don't have any audio output on your device,
// it fails to load sounds/music. If you then plug in an audio output,
// it still fails to load, and you must restart the game to hear sound.
// So we need to detect when the output device changes.

void Sound::LoadFile(const std::string& newFilepath)
{
	if (chunk != nullptr)
	{
		Mix_FreeChunk(chunk);
	}

	filepath = newFilepath;
	chunk = Mix_LoadWAV(filepath.c_str());
}

bool SoundChannel::Play()
{
	if (sound->chunk != nullptr)
	{
		Mix_Volume(num, volume);
		Mix_PlayChannel(num, sound->chunk, loop);
	}
	else
	{
		// Log error, could not load file
		return false;
	}

	return true;
}

bool SoundChannel::Stop()
{
	if (sound->chunk != nullptr)
	{
		sound->filepath = Globals::NONE_STRING;
		Mix_HaltChannel(num);
	}
	else
	{
		// Log error
		return false;
	}

	return true;
}

SoundManager::SoundManager()
{
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
	volArray = { 0, 32, 64, 96, MIX_MAX_VOLUME };
}

SoundManager::~SoundManager()
{
	if (currentBGM != nullptr)
		Mix_FreeMusic(currentBGM);

	for (auto const& [num, channel] : sounds)
	{
		if (channel != nullptr)
			delete channel;
	}

	if (soundTest != nullptr)
		delete_it(soundTest);

	Mix_Quit();
}

void SoundManager::Init(Game* g)
{
	game = g;
	soundTest = neww SoundTest(*this);
}

bool SoundManager::IsPlayingSound(int channel)
{
	return Mix_Playing(channel);
}

bool SoundManager::IsPlayingBGM()
{
	return Mix_PlayingMusic();
}

bool SoundManager::LoadBGM(const std::string& bgm)
{
	Mix_Music* newBGM = Mix_LoadMUS(bgm.c_str());

	if (newBGM == nullptr)
	{
		if (game != nullptr)
		{
			game->logger.Log("ERROR: Failed to load BGM: " + bgm);
		}

		bgmFilepath = "None";
		return false;
	}

	if (currentBGM != nullptr)
		Mix_FreeMusic(currentBGM);

	//bgm = "bgm/" + bgm + ".ogg";
	currentBGM = newBGM;

	bgmFilepath = bgm;
	return true;
}

void SoundManager::PlayBGM(const std::string& bgm, bool loop)
{
	if (Mix_PausedMusic() && bgm == bgmFilepath)
	{
		Mix_ResumeMusic();
	}
	else if (LoadBGM(bgm))
	{
		// TODO: Define loop points to loop within a song
		if (loop)
			Mix_PlayMusic(currentBGM, -1);
		else
			Mix_PlayMusic(currentBGM, 1);
	}
}

void SoundManager::PauseBGM()
{
	Mix_PauseMusic();
}

void SoundManager::UnpauseBGM()
{
	Mix_ResumeMusic();
}

void SoundManager::StopBGM()
{
	Mix_HaltMusic();
	bgmFilepath = Globals::NONE_STRING;
}

void SoundManager::FadeInChannel(const std::string& filepath, uint32_t duration, int channel, bool loop)
{

}

void SoundManager::FadeOutChannel(uint32_t duration, int channel)
{
	if (channel < 0)
	{
		for (auto& [key, channel] : sounds)
		{
			if (channel != nullptr)
			{
				channel->sound->filepath = Globals::NONE_STRING;
				Mix_FadeOutChannel(channel->num, duration);
			}
		}
	}
	else 
	{
		if (sounds[channel] != nullptr)
		{
			sounds[channel]->sound->filepath = Globals::NONE_STRING;
		}

		Mix_FadeOutChannel(channel, duration);
	}	
}

void SoundManager::FadeInBGM(const std::string& bgm, uint32_t duration, bool loop)
{
	if (LoadBGM(bgm))
	{
		if (loop)
			Mix_FadeInMusic(currentBGM, -1, duration);
		else
			Mix_FadeInMusic(currentBGM, 1, duration);
	}		
}

void SoundManager::FadeOutBGM(uint32_t duration)
{
	Mix_FadeOutMusic(duration);
	bgmFilepath = "None";
}

void SoundManager::SetVolumeBGMIndex(int index)
{
	bgmVolumeIndex = index;
	volumeBGM = volArray[index];
	Mix_VolumeMusic(volumeBGM);
}

void SoundManager::SetVolumeBGM(int newVolume)
{
	volumeBGM = newVolume;
	Mix_VolumeMusic(volumeBGM);
}

uint32_t SoundManager::GetVolumeBGM()
{
	return volumeBGM;
}

void SoundManager::PlaySound(const std::string& filepath, int channel, int loop)
{
	// Don't do anything here, to avoid memory leaks
	if (channel < 0)
		return;

	// TODO: Generate X number of channels ahead of time,
	// and then modify individual properties when needed
	if (sounds[channel] == nullptr)
	{
		Sound* sound = neww Sound(filepath.c_str());
		SoundChannel* soundChannel = neww SoundChannel(channel, sound, volumeSound, loop);
		sounds[channel] = soundChannel;
		sounds[channel]->sound->LoadFile(filepath);
	}
	else
	{
		sounds[channel]->sound->LoadFile(filepath);
	}

	//sound = "se/" + sound + ".wav";
	if (!sounds[channel]->Play())
	{
		if (game != nullptr)
		{
			game->logger.Log("ERROR: Failed to load sound:" + filepath);
		}
	}
}

void SoundManager::ClearChannel(int channel)
{
	if (sounds[channel] != nullptr)
	{
		delete sounds[channel];
	}
}

void SoundManager::SetVolumeSoundIndex(int index)
{
	soundVolumeIndex = index;
	volumeSound = volArray[index];
}

void SoundManager::SetVolumeSound(int newVolume)
{
	volumeSound = newVolume;
}

void SoundManager::SetVolumeSoundOnChannel(int newVolume, int channel)
{
	volumeSound = newVolume;

	if (sounds[channel] != nullptr)
	{
		sounds[channel]->volume = newVolume;
		sounds[channel]->prevVolume = newVolume;

		// TODO: A lot of this seems redundant. Can this be refactored better?
		Mix_Volume(sounds[channel]->num, sounds[channel]->volume);
	}
}

uint32_t SoundManager::GetVolumeSound()
{
	return volumeSound;
}

// TODO: This will only affect sounds that have already been played,
// and won't work correctly on new sounds. Need to fix that before enabling!
void SoundManager::ToggleAudio()
{
	disableAudio = !disableAudio;

	if (disableAudio)
	{
		prevVolumeBGM = volumeBGM;
		prevVolumeSound = volumeSound;
		volumeBGM = 0;
		volumeSound = 0;
		SetVolumeBGM(0);

		for (auto& [key, channel] : sounds)
		{
			if (channel != nullptr)
			{
				channel->volume = 0;
				channel->prevVolume = channel->volume;				
				Mix_Volume(channel->num, channel->volume);
			}
		}
	}
	else
	{
		volumeBGM = prevVolumeBGM;
		volumeSound = volumeSound;
		SetVolumeBGM(prevVolumeBGM);

		for (auto& [key, channel] : sounds)
		{
			if (channel != nullptr)
			{
				channel->volume = channel->prevVolume;
				Mix_Volume(channel->num, channel->volume);
			}
		}
	}
}

void SoundManager::ReadMusicData(const std::string& dataFilePath)
{
	// Get data from the file
	std::ifstream fin;
	fin.open(dataFilePath);

	std::string data = "";
	for (std::string line; std::getline(fin, line); )
	{
		data += line + "\n";
	}

	fin.close();

	// Go through the data and add all states
	std::stringstream ss{ data };

	char lineChar[256];
	ss.getline(lineChar, 256);

	std::string bgmName = "";

	int offsetX, offsetY, drawOrder = 0;
	std::string filepath = "";
	float parallax = 0.0f;
	int index = 0;


	while (ss.good() && !ss.eof())
	{
		std::istringstream buf(lineChar);
		std::istream_iterator<std::string> beg(buf), end;
		std::vector<std::string> tokens(beg, end);

		index = 0;
		if (tokens.size() == 0)
			break;

		bgmName = tokens[index + 1];
		bgmNames[tokens[index]] = bgmName;
		index++;

		ss.getline(lineChar, 256);
	}


}