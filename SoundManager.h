#pragma once
#include <SDL_mixer.h>
#include <string>
#include <vector>

class SoundManager
{
private:
	Uint32 volumeBGM = 20;
	std::vector<int> volArray;
public:

	Uint32 GetVolume();
	int bgmVolumeIndex = 0;
	Mix_Music* currentBGM = nullptr;
	SoundManager();
	~SoundManager();
	void PlayBGM(std::string bgm, bool loop = true);
	void StopBGM();
	void FadeInBGM(std::string bgm, Uint32 duration, bool loop = true);
	void FadeOutBGM(Uint32 duration);
	void SetVolumeBGM(int index);
};

