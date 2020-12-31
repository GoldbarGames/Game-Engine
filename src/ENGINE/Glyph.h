#ifndef GLYPH_H
#define GLYPH_H
#pragma once

#include <string>
#include "Vector2.h"
#include "Sprite.h"
#include "leak_check.h"
struct Glyph
{
	char letter = ' ';
	Sprite sprite;
	Animator* animator = nullptr; // never delete these
	Vector2 position = Vector2(0, 0);
	Vector2 scale = Vector2(1, 1);
	//bool shouldDeleteSprite = true;
	//float width = 0; Should we save width and height to avoid calculations?
	// Maybe if the value is negative, recalculate, otherwise use what is there?

	Glyph()
	{

	}

	~Glyph()
	{
		//if (shouldDeleteSprite)
		//delete_it(sprite);
	}
};



#endif