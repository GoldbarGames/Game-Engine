#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H
#pragma once

#include <SDL_mixer.h>
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
};

struct SoundChannel {
	const int num;
	Sound* sound;
	char volume;
	int loop;
	SoundChannel(int n, Sound* s, char v, int l) : num(n), sound(s), volume(v), loop(l) { }
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
	Uint32 volumeBGM = 20;
	Uint32 volumeSound = 20;
	std::vector<int> volArray;
	Game* game = nullptr;
public:
	std::unordered_map<int, SoundChannel*> sounds;
	std::unordered_map<std::string, std::string> bgmNames;
	Uint32 GetVolumeBGM();
	Uint32 GetVolumeSound();
	int bgmVolumeIndex = 0;
	int soundVolumeIndex = 0;
	Mix_Music* currentBGM = nullptr;
	std::string bgmFilepath = "";
	SoundManager();
	~SoundManager();
	void Init(Game* g);
	bool LoadBGM(const std::string& bgm);
	void PlayBGM(const std::string& bgm, bool loop = true);
	void StopBGM();
	void FadeInBGM(const std::string& bgm, Uint32 duration, bool loop = true);
	void FadeOutBGM(Uint32 duration);
	void SetVolumeBGM(int index);
	void PlaySound(const std::string& filepath, int channel = -1, int loop = 0);
	void ClearChannel(int channel);
	void SetVolumeSound(int index);
	void ReadMusicData(const std::string& dataFilePath);
	bool IsPlayingSound(int channel);
	bool IsPlayingBGM();
};

#endif