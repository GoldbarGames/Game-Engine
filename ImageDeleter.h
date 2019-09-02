#pragma once

struct ImageDeleter
{
	void operator()(SDL_Texture * image)
	{
		SDL_DestroyTexture(image);
	}
};