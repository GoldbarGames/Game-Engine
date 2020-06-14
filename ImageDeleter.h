#ifndef IMAGEDELETER_H
#define IMAGEDELETER_H
#pragma once

#include "Texture.h"

struct ImageDeleter
{
	void operator()(Texture* image)
	{
		image->ClearTexture();
	}
};

#endif