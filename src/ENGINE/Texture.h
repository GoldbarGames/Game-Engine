#ifndef TEXTURE_H
#define TEXTURE_H
#pragma once

#include "opengl_includes.h"
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
	void LoadTexture(SDL_Surface* surface, bool reset=false);
	void UseTexture(int textureNum = GL_TEXTURE0);
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