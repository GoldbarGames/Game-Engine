#ifndef GLYPH_H
#define GLYPH_H
#pragma once

#include <string>
#include "Vector2.h"
#include "Sprite.h"

struct Glyph
{
	Sprite* sprite = nullptr;
	Animator* animator = nullptr; // never delete these
	Vector2 position = Vector2(0, 0);
	bool shouldDeleteSprite = true;
	//float width = 0; Should we save width and height to avoid calculations?
	// Maybe if the value is negative, recalculate, otherwise use what is there?

	~Glyph()
	{
		if (shouldDeleteSprite)
			delete_it(sprite);
	}
};

struct GlyphSurfaceData
{
	std::string fontName = "";
	char glyph = 'x';
	SDL_Color color = { 255, 255, 255, 255 };

	bool operator==(const GlyphSurfaceData& other) const
	{
		if (fontName != other.fontName)
			return false;

		if (glyph != other.glyph)
			return false;

		if (color.r != other.color.r)
			return false;

		if (color.g != other.color.g)
			return false;

		if (color.b != other.color.b)
			return false;

		if (color.a != other.color.a)
			return false;

		return true;
	}

};

class GlyphHashFunction
{
public:
	std::size_t operator()(const GlyphSurfaceData& k) const
	{
		return ((std::hash<std::string>()(k.fontName)
			^ (std::hash<char>()(k.glyph) << 1)) >> 1)
			^ (std::hash<int>()(k.color.r) << 1)
			^ (std::hash<int>()(k.color.g) << 1)
			^ (std::hash<int>()(k.color.b) << 1)
			^ (std::hash<int>()(k.color.a) << 1);
	}
};

#endif