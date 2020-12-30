#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H
#pragma once

#include <SDL2/SDL_mixer.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "leak_check.h"
class Game;

struct Sound {
	Mix_Chunk* chunk;
	std::string filepath;

	Sound(std::string f): chunk(Mix_LoadWAV(f.c_str())), filepath(f) { }

	~Sound()
	{
		if (chunk != nullptr)
			Mix_FreeChunk(chunk);
	}

	void LoadFile(const std::string& newFilepath);
};

struct SoundChannel {
	const int num;
	Sound* sound;
	char volume;
	char prevVolume;
	int loop;
	SoundChannel(int n, Sound* s, char v, int l) : num(n), sound(s), volume(v), prevVolume(v), loop(l) { }
	~SoundChannel()
	{
		if (sound != nullptr)
			delete sound;
	}

	bool Play();
	bool Stop();
};

class KINJO_API SoundManager
{
private:
	uint32_t volumeBGM = 64;
	uint32_t volumeSound = 64;
	uint32_t prevVolumeBGM = 64;
	uint32_t prevVolumeSound = 64;
	std::vector<int> volArray;
	Game* game = nullptr;
public:
	bool disableAudio = false;
	std::unordered_map<int, SoundChannel*> sounds;
	std::unordered_map<std::string, std::string> bgmNames;
	uint32_t GetVolumeBGM();
	uint32_t GetVolumeSound();
	int bgmVolumeIndex = 0;
	int soundVolumeIndex = 0;
	Mix_Music* currentBGM = nullptr;
	std::string bgmFilepath = "None";
	SoundManager();
	~SoundManager();
	void Init(Game* g);
	bool LoadBGM(const std::string& bgm);
	void PlayBGM(const std::string& bgm, bool loop = true);
	void StopBGM();

	void FadeInBGM(const std::string& bgm, uint32_t duration, bool loop = true);
	void FadeOutBGM(uint32_t duration);

	void FadeInChannel(const std::string& filepath, uint32_t duration, int channel = -1, bool loop = true);
	void FadeOutChannel(uint32_t duration, int channel = -1);

	void SetVolumeBGM(int newVolume);
	void SetVolumeBGMIndex(int index);
	void PlaySound(const std::string& filepath, int channel = -1, int loop = 0);
	void ClearChannel(int channel);
	void SetVolumeSound(int index);
	void SetVolumeSoundIndex(int index);
	void SetVolumeSoundOnChannel(int newVolume, int channel);
	void ReadMusicData(const std::string& dataFilePath);
	bool IsPlayingSound(int channel);
	bool IsPlayingBGM();

	void ToggleAudio();
};

#endif