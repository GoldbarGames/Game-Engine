#pragma once

#include <GL/glew.h>
#include <SDL_image.h>

class Texture
{
public:
	Texture();
	Texture(const char* path, bool alpha = true);
	~Texture();

	void LoadTexture();
	void LoadTexture(SDL_Surface* surface);
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

