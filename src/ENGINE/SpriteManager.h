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

// One glyph's placement inside a font atlas: its UV sub-rect + the rendered cell
// size (advance width x line height) that text layout advances by.
struct GlyphUV
{
	float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
	int cellW = 0, cellH = 0;
};

// All printable-ASCII glyphs of one (font, size) packed into a single texture, so
// a whole Latin string can be drawn in ONE batched draw call instead of one draw
// per glyph. Non-ASCII glyphs are NOT in the atlas and fall back to the per-glyph
// path (SpriteManager::GetTexture) - see the glyph-atlas plan.
struct FontAtlas
{
	Texture* texture = nullptr;                 // owns the packed atlas texture
	std::unordered_map<char, GlyphUV> glyphs;
	int lineHeight = 0;
};

class Renderer;

class KINJO_API SpriteManager
{
private:
	mutable std::unordered_map<std::string, Texture*> images;
	mutable std::unordered_map<std::string, std::vector<AnimState*>> animationStates;
	std::unordered_map<GlyphSurfaceData, Texture*, GlyphHashFunction> glyphTextures;
	std::unordered_map<std::string, Texture*> textImages;
	std::unordered_map<std::string, FontAtlas> fontAtlases;   // key: fontName + "|" + size
public:
	// Build (once) and return the ASCII glyph atlas for this font+size, or nullptr
	// if it can't be built. Glyphs not covered fall back to per-glyph GetTexture.
	FontAtlas* GetFontAtlas(TTF_Font* f, int size);
	Renderer* renderer = nullptr;
	std::vector<AnimState*> ReadAnimData(const std::string& dataFilePath) const;
	std::vector<AnimState*> ReadAnimData(const std::string& dataFilePath, std::unordered_map<std::string, std::string>& args) const;
	void ClearCache(std::string const& imagePath);
	// filter applies only on first load; the cached texture keeps whichever
	// filter it was created with, so load an image consistently everywhere
	Texture* GetImage(const std::string& imagePath,
		Texture::Filter filter = Texture::Filter::Point) const;
	Texture* GetTexture(TTF_Font* f, char c, int size);
	Texture* GetTexture(TTF_Font* f, const std::string& txt, int wrapWidth=0);
	void Init(Renderer* r);
	SpriteManager();
	~SpriteManager();
};

#endif