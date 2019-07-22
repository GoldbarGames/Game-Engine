#pragma once
#include "SDL.h"

class Timer
{
private:
	Uint32 startTicks;
	Uint32 pausedTicks;
	bool paused;
	bool started;
public:
	Timer();
	~Timer();

	void Start();
	void Stop();
	void Pause();
	void Unpause();
	Uint32 GetTicks();
	bool IsPaused();
};

