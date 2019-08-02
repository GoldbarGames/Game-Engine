#pragma once
#include "SDL.h"

class Timer
{
private:
	Uint32 startTicks = 0; // the tick count when the timer is started
	Uint32 pausedTicks = 0; // the number of ticks when its paused
	Uint32 pausedTime = 0;
	bool paused = false;
	bool started = false;
	Uint32 endTime = 0; // the time at which to stop the timer, if any
public:
	Timer();
	~Timer();
	bool alwaysOn = false;
	bool loopAnimation = false;
	void Start(Uint32 duration = 0, bool loop=true); //milliseconds
	void Stop();
	void Pause(Uint32 ticks);
	void Unpause(Uint32 ticks = 0);
	Uint32 GetTicks();
	Uint32 GetAnimationTime();
	bool IsPaused();
	bool HasElapsed();
};

