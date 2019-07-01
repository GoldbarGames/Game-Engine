#pragma once

struct ImageDeleter
{
	void operator()(SDL_Surface * surf)
	{
		SDL_FreeSurface(surf);
	}
};