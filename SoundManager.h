#pragma once
#include <SDL_mixer.h>
#include <string>
#include <vector>
#include <unordered_map>

class SoundManager
{
private:
	Uint32 volumeBGM = 20;
	Uint32 volumeSound = 20;
	std::vector<int> volArray;
	std::unordered_map<int, Mix_Chunk*> sounds;
public:

	Uint32 GetVolumeBGM();
	Uint32 GetVolumeSound();
	int bgmVolumeIndex = 0;
	int soundVolumeIndex = 0;
	Mix_Music* currentBGM = nullptr;
	SoundManager();
	~SoundManager();
	void PlayBGM(std::string bgm, bool loop = true);
	void StopBGM();
	void FadeInBGM(std::string bgm, Uint32 duration, bool loop = true);
	void FadeOutBGM(Uint32 duration);
	void SetVolumeBGM(int index);
	void PlaySound(std::string sound, int channel = -1);
	void SetVolumeSound(int index);
};

