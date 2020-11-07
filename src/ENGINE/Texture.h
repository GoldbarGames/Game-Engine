#ifndef TEXTURE_H
#define TEXTURE_H
#pragma once

#include <GL/glew.h>
#include <SDL_image.h>
#include "leak_check.h"

class KINJO_API Texture
{
public:
	Texture(const char* path);
	~Texture();

	void LoadTexture(unsigned int& buffer, int w, int h);
	void LoadTexture(SDL_Surface* surface, bool reset=false);
	void UseTexture();
	void ClearTexture();
	int GetWidth() { return width; }
	int GetHeight() { return height; }
	const char* GetFilePath() { return filePath; } ;
private:
	GLuint textureID;
	int width, height;
	const char* filePath;
};

#endif