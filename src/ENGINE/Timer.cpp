#include "Timer.h"
#include "globals.h"
#include <iostream>
#include <SDL2/SDL.h>

Timer::Timer()
{
	startTicks = 0;
	pausedTicks = 0;

	paused = false;
	started = false;
}

Timer::~Timer()
{

}


bool Timer::HasElapsed() const
{
	return Globals::CurrentTicks >= endTime;
}

void Timer::Reset()
{
	Start(lastDuration);
}

void Timer::Start(uint32_t duration, bool loopAnim)
{
	started = true;
	paused = false;

	loopAnimation = loopAnim;
	startTicks = Globals::CurrentTicks;
	pausedTicks = 0;
	lastDuration = duration;
	endTime = startTicks + duration;
}

void Timer::Stop()
{
	started = false;
	paused = false;

	startTicks = 0;
	pausedTicks = 0;
}

void Timer::Pause()
{
	if (alwaysOn)
		return;

	//If the timer is running and isn't already paused
	if (started && !paused)
	{
		paused = true;

		uint32_t sdl_ticks = Globals::CurrentTicks;
		
		//Calculate the paused ticks
		pausedTicks = sdl_ticks - startTicks;
		pausedTime = sdl_ticks;

		/*
		std::cout << "sdl_ticks: " << sdl_ticks << std::endl;
		std::cout << "ticks: " << ticks << std::endl;
		std::cout << "startTicks: " << startTicks << std::endl;
		std::cout << "pausedTicks: " << pausedTicks << std::endl;
		std::cout << "endTime: " << endTime << std::endl;
		*/

		startTicks = 0;
	}
}

void Timer::Unpause()
{
	if (alwaysOn)
		return;

	//If the timer is running and paused
	if (started && paused)
	{
		//Unpause the timer
		paused = false;

		//Reset the starting ticks
		uint32_t sdl_ticks = Globals::CurrentTicks;
		uint32_t pauseDuration = sdl_ticks - pausedTicks;
		
		if (sdl_ticks >= 0)
			pauseDuration = sdl_ticks - pausedTicks;

		startTicks = pauseDuration;

		/*
		std::cout << "sdl_ticks: " << sdl_ticks << std::endl;
		std::cout << "ticks: " << ticks << std::endl;		
		std::cout << "pausedTicks: " << pausedTicks << std::endl;
		std::cout << "pauseDuration: " << pauseDuration << std::endl;
		std::cout << "endTime: " << endTime << std::endl;
		*/

		// if the timer has an end, add to the end the time that was skipped
		if (endTime > 0)
		{
			endTime += sdl_ticks - pausedTime;
		}

		//std::cout << "new end: " << endTime << std::endl;

		//Reset the paused ticks
		pausedTicks = 0;
	}
}

uint32_t Timer::GetAnimationTime() const
{
	if (endTime > 0 && !loopAnimation && HasElapsed())
	{
		return 0;
	}
	else
	{
		return GetTicks();
	}
}

uint32_t Timer::GetTicks() const
{
	//The actual timer time
	uint32_t time = 0;

	//If the timer is running
	if (started)
	{
		//If the timer is paused
		if (paused)
		{
			//Return the number of ticks when the timer was paused
			time = pausedTicks;
		}
		else
		{
			//Return the current time minus the start time
			time = Globals::CurrentTicks - startTicks;
		}
	}

	return time;
}

bool Timer::IsPaused() const
{
	return paused && started;
}