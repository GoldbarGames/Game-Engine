#ifndef SPRITEMANAGER_H
#define SPRITEMANAGER_H
#pragma once

#include "SDL.h"
#include <SDL_image.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include "ImageDeleter.h"
#include "Vector2.h"

#include "Texture.h"
#include <SDL_ttf.h>
#include "leak_check.h"

struct AnimState;

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

class Renderer;

class DECLSPEC SpriteManager
{
private:
	mutable std::unordered_map<std::string, Texture*> images;
	std::unordered_map<std::string, std::vector<AnimState*>> animationStates;
	std::unordered_map<GlyphSurfaceData, Texture*, GlyphHashFunction> glyphTextures;
	std::unordered_map<std::string, Texture*> textImages;
public:
	Renderer* renderer = nullptr;
	std::vector<AnimState*> ReadAnimData(const std::string& dataFilePath);
	std::vector<AnimState*> ReadAnimData(const std::string& dataFilePath, std::unordered_map<std::string, std::string>& args);
	Texture* GetImage(const std::string& imagePath) const;
	// TODO: Get rid of Color here, just make it white and then apply color using shaders
	Texture* GetTexture(TTF_Font* f, char c, SDL_Color col);
	Texture* GetTexture(TTF_Font* f, const std::string& txt, int wrapWidth=0);
	void Init(Renderer* r);
	SpriteManager();
	~SpriteManager();
};

#endif