#ifndef SPRITEMANAGER_H
#define SPRITEMANAGER_H
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include "ImageDeleter.h"

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

struct GlyphHashFunction {
	std::size_t operator()(const GlyphSurfaceData& k) const {
		std::size_t hash = std::hash<std::string>{}(k.fontName);
		hash = hash_combine(hash, std::hash<char>{}(k.glyph));
		hash = hash_combine(hash, std::hash<int>{}(k.size));
		hash = hash_combine(hash, std::hash<int>{}(k.color.r));
		hash = hash_combine(hash, std::hash<int>{}(k.color.g));
		hash = hash_combine(hash, std::hash<int>{}(k.color.b));
		hash = hash_combine(hash, std::hash<int>{}(k.color.a));
		return hash;
	}
private:
	template <typename T>
	std::size_t hash_combine(std::size_t seed, const T& v) const {
		return seed ^ (std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2));
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
	void ClearCache(std::string const& imagePath);
	Texture* GetImage(const std::string& imagePath) const;
	Texture* GetTexture(TTF_Font* f, char c, int size);
	Texture* GetTexture(TTF_Font* f, const std::string& txt, int wrapWidth=0);
	void Init(Renderer* r);
	SpriteManager();
	~SpriteManager();
};

#endif