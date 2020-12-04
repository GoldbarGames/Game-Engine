#include "leak_check.h"
#include "SoundManager.h"
#include <iostream>
#include <fstream>
#include <iterator>
#include <sstream>
#include "Game.h"
#include "Logger.h"

// TODO: When you don't have any audio output on your device,
// it fails to load sounds/music. If you then plug in an audio output,
// it still fails to load, and you must restart the game to hear sound.
// So we need to detect when the output device changes.

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
	volArray = { 0, 30, 60, 90, MIX_MAX_VOLUME };
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

	Mix_Quit();
}

void SoundManager::Init(Game* g)
{
	game = g;
}

bool SoundManager::IsPlayingSound(int channel)
{
	//TODO
	return false;
}

bool SoundManager::IsPlayingBGM()
{
	//TODO
	return false;
}

bool SoundManager::LoadBGM(const std::string& bgm)
{
	if (currentBGM != nullptr)
		Mix_FreeMusic(currentBGM);

	//bgm = "bgm/" + bgm + ".ogg";
	currentBGM = Mix_LoadMUS(bgm.c_str());

	if (currentBGM == nullptr)
	{
		if (game != nullptr)
		{
			game->logger.Log("ERROR: Failed to load BGM:" + bgm);
		}

		bgmFilepath = "";
		return false;
	}

	bgmFilepath = bgm;
	return true;
}

void SoundManager::PlayBGM(const std::string& bgm, bool loop)
{
	if (LoadBGM(bgm))
	{
		// TODO: Define loop points to loop within a song
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

void SoundManager::FadeInChannel(const std::string& filepath, Uint32 duration, int channel, bool loop)
{

}

void SoundManager::FadeOutChannel(uint32_t duration, int channel)
{
	if (channel < 0)
	{
		for (auto& [key, channel] : sounds)
		{
			Mix_FadeOutChannel(channel->num, duration);
		}
	}
	else
	{
		Mix_FadeOutChannel(channel, duration);
	}	
}

void SoundManager::FadeInBGM(const std::string& bgm, Uint32 duration, bool loop)
{
	if (LoadBGM(bgm))
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

void SoundManager::PlaySound(const std::string& filepath, int channel, int loop)
{
	// Don't do anything here, to avoid memory leaks
	if (channel < 0)
		return;

	if (sounds[channel] != nullptr)
	{
		delete sounds[channel];
	}

	//sound = "se/" + sound + ".wav";
	Sound* sound = neww Sound(filepath.c_str());
	SoundChannel* soundChannel = neww SoundChannel(channel, sound, volumeSound, loop);

	sounds[channel] = soundChannel;
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

void SoundManager::SetVolumeSound(int index)
{
	soundVolumeIndex = index;
	volumeSound = volArray[index];
}

Uint32 SoundManager::GetVolumeSound()
{
	return volumeSound;
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