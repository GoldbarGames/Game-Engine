#ifndef TEXTURE_H
#define TEXTURE_H
#pragma once

#include <SDL2/SDL_image.h>
#include "leak_check.h"
#include <string>

class KINJO_API Texture
{
public:
	static int lastTextureID;
	static int lastActiveTexture;

	Texture(const std::string& path);
	~Texture();

	bool LoadTexture();
	void LoadTexture(unsigned int& buffer, int w, int h);
	// Point = nearest-neighbor, crisp pixel art (default; no mipmaps).
	// Smooth = linear + trilinear mipmaps, for text and continuously
	// scaled content (fixes grainy/choppy glyphs when scaled down).
	enum class Filter { Point, Smooth };

	void LoadTexture(SDL_Surface* surface, bool reset=false, Filter filter=Filter::Point);
	// srgb: store the pixels as sRGB-encoded colour, so shaders sample linear
	// values (colour art for a linear-workflow scene; see SpriteManager::GetImage).
	void LoadTexture(SDL_Surface* surface, bool reset, Filter filter, bool srgb);
	// From an image file's bytes: PNG, JPEG, etc. (SDL_image), or KTX2 (its
	// own mip chain, BC-compressed or not). False if they don't decode or this
	// GPU can't sample the KTX2 file's format.
	bool LoadFromFileData(const void* bytes, size_t size, Filter filter, bool srgb);
	// Bind to texture unit `unit` (0, 1, 2...). For compatibility, a raw
	// GL_TEXTUREn value is also accepted (older callers passed GL_TEXTURE0+n).
	void UseTexture(int unit = 0);
	void ClearTexture();
	int GetWidth() { return width; }
	int GetHeight() { return height; }
	const std::string& GetFilePath() { return filePath; } ;
private:
	std::string filePath = "";
	unsigned int textureID;
	int width, height;
};

#endif