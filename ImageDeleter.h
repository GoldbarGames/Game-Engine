#pragma once

#include "Texture.h"

struct ImageDeleter
{
	void operator()(Texture * image)
	{
		//SDL_DestroyTexture(image);
		image->ClearTexture();
	}
};