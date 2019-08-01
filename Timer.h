#pragma once
#include "SDL.h"

class Timer
{
private:
	Uint32 startTicks = 0; // the tick count when the timer is started
	Uint32 pausedTicks = 0; // the number of ticks when its paused
	bool paused = false;
	bool started = false;
	Uint32 endTime = 0; // the time at which to stop the timer, if any
public:
	Timer();
	~Timer();

	void Start(Uint32 duration = 0); //milliseconds
	void Stop();
	void Pause();
	void Unpause();
	Uint32 GetTicks();
	bool IsPaused();
	bool HasElapsed();
};

