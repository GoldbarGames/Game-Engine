#ifndef TEXTURE_H
#define TEXTURE_H
#pragma once

#include <GL/glew.h>
#include <SDL2/SDL_image.h>
#include "leak_check.h"
#include <string>

class KINJO_API Texture
{
public:
	Texture(const std::string& path);
	~Texture();

	void LoadTexture(unsigned int& buffer, int w, int h);
	void LoadTexture(SDL_Surface* surface, bool reset=false);
	void UseTexture();
	void ClearTexture();
	int GetWidth() { return width; }
	int GetHeight() { return height; }
	const std::string& GetFilePath() { return filePath; } ;
private:
	std::string filePath = "";
	GLuint textureID;
	int width, height;
};

#endif