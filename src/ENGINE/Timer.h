#ifndef TIMER_H
#define TIMER_H
#pragma once

#include <cstdint>
#include "leak_check.h"

class KINJO_API Timer
{
private:	
	uint32_t pausedTicks = 0; // the number of ticks when it is paused
	uint32_t pausedTime = 0;
	bool paused = false;
	bool started = false;
	uint32_t lastDuration = 0; // the last time that the duration was set
public:

	uint32_t startTicks = 0; // the tick count when the timer is started
	uint32_t endTime = 0; // the time at which to stop the timer, if any
	Timer();
	~Timer();
	bool alwaysOn = false;
	bool loopAnimation = false;
	void Start(uint32_t duration = 0, bool loopAnim=true); //milliseconds
	void Stop();
	void Pause();
	void Unpause();
	uint32_t GetTicks() const;
	uint32_t GetAnimationTime() const;
	bool IsPaused() const;
	bool HasElapsed() const;
	void Reset();
};

#endif