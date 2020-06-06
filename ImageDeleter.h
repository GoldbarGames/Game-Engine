#pragma once

#include "Texture.h"

struct ImageDeleter
{
	void operator()(Texture* image)
	{
		image->ClearTexture();
	}
};