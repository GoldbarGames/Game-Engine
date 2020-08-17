#include "Timer.h"
#include <iostream>

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
	// is calling this function multiple times per frame really a good idea?
	Uint32 totalTime = SDL_GetTicks();
	return SDL_GetTicks() >= endTime;
}

void Timer::Reset()
{
	Start(lastDuration);
}

void Timer::Start(Uint32 duration, bool loopAnim)
{
	started = true;
	paused = false;

	loopAnimation = loopAnim;
	startTicks = SDL_GetTicks();
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

		Uint32 sdl_ticks = SDL_GetTicks();
		
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
		Uint32 sdl_ticks = SDL_GetTicks();
		Uint32 pauseDuration = sdl_ticks - pausedTicks;
		
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

Uint32 Timer::GetAnimationTime() const
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

Uint32 Timer::GetTicks() const
{
	//The actual timer time
	Uint32 time = 0;

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
			time = SDL_GetTicks() - startTicks;
		}
	}

	return time;
}

bool Timer::IsPaused() const
{
	return paused && started;
}