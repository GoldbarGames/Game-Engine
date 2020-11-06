#ifndef IMAGEDELETER_H
#define IMAGEDELETER_H
#pragma once

#include "Texture.h"
#include "globals.h"
#include "leak_check.h"
struct ImageDeleter
{
	void operator()(Texture* image)
	{
		image->ClearTexture();
		if (image != nullptr)
			delete_it(image);
	}
};

#endif