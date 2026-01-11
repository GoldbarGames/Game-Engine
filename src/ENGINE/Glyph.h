#ifndef GLYPH_H
#define GLYPH_H
#pragma once

#include <string>
#include "Sprite.h"
#include "leak_check.h"
struct Glyph
{
	char letter = ' ';
	Sprite sprite;
	Animator* animator = nullptr; // never delete these
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::vec2 scale = glm::vec2(1, 1);
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