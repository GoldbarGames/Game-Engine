#include "Timer.h"

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

void Timer::Start()
{
	started = true;
	paused = false;

	startTicks = SDL_GetTicks();
	pausedTicks = 0;
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
	//If the timer is running and isn't already paused
	if (started && !paused)
	{
		paused = true;

		//Calculate the paused ticks
		pausedTicks = SDL_GetTicks() - startTicks;
		startTicks = 0;
	}
}

void Timer::Unpause()
{
	//If the timer is running and paused
	if (started && paused)
	{
		//Unpause the timer
		paused = false;

		//Reset the starting ticks
		startTicks = SDL_GetTicks() - pausedTicks;

		//Reset the paused ticks
		pausedTicks = 0;
	}
}

Uint32 Timer::GetTicks()
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

bool Timer::IsPaused()
{
	return paused && started;
}