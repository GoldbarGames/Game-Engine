#ifndef TIMER_H
#define TIMER_H
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
	Uint32 lastDuration = 0; // the last time that the duration was set
public:
	Timer();
	~Timer();
	bool alwaysOn = false;
	bool loopAnimation = false;
	void Start(Uint32 duration = 0, bool loopAnim=true); //milliseconds
	void Stop();
	void Pause();
	void Unpause();
	Uint32 GetTicks() const;
	Uint32 GetAnimationTime() const;
	bool IsPaused() const;
	bool HasElapsed() const;
	void Reset();
};

#endif