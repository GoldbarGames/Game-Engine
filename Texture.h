#ifndef TEXTURE_H
#define TEXTURE_H
#pragma once

#include <GL/glew.h>
#include <SDL_image.h>

class Texture
{
public:
	Texture(const char* path);
	~Texture();

	void LoadTexture();
	void LoadTexture(SDL_Surface* surface, bool reset=false);
	void UseTexture();
	void ClearTexture();
	int GetWidth() { return width; }
	int GetHeight() { return height; }
private:
	GLuint textureID;
	int width, height, bitDepth;
	bool rgba;

	const char* filePath;
};

#endif