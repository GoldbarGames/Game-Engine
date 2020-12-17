#ifndef SPRITEMANAGER_H
#define SPRITEMANAGER_H
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include "ImageDeleter.h"
#include "Vector2.h"

#include "Texture.h"
#include <SDL2/SDL_ttf.h>
#include "leak_check.h"

struct AnimState;

struct GlyphSurfaceData
{
	std::string fontName = "";
	char glyph = 'x';
	int size = 1;
	SDL_Color color = { 255, 255, 255, 255 };

	bool operator==(const GlyphSurfaceData& other) const
	{
		if (fontName != other.fontName)
			return false;

		if (glyph != other.glyph)
			return false;

		if (size != other.size)
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

class Renderer;

class KINJO_API SpriteManager
{
private:
	mutable std::unordered_map<std::string, Texture*> images;
	mutable std::unordered_map<std::string, std::vector<AnimState*>> animationStates;
	std::unordered_map<GlyphSurfaceData, Texture*, GlyphHashFunction> glyphTextures;
	std::unordered_map<std::string, Texture*> textImages;
public:
	Renderer* renderer = nullptr;
	std::vector<AnimState*> ReadAnimData(const std::string& dataFilePath) const;
	std::vector<AnimState*> ReadAnimData(const std::string& dataFilePath, std::unordered_map<std::string, std::string>& args) const;
	Texture* GetImage(const std::string& imagePath) const;
	// TODO: Get rid of Color here, just make it white and then apply color using shaders
	Texture* GetTexture(TTF_Font* f, char c, int size, SDL_Color col);
	Texture* GetTexture(TTF_Font* f, const std::string& txt, int wrapWidth=0);
	void Init(Renderer* r);
	SpriteManager();
	~SpriteManager();
};

#endif