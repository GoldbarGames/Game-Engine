#include "Texture.h"

Texture::Texture(const char* path)
{
	textureID = 0;
	width = 0;
	height = 0;
	bitDepth = 0;
	filePath = path;
}

Texture::~Texture()
{
	ClearTexture();
}

void Texture::LoadTexture()
{
	
}

void Texture::LoadTexture(SDL_Surface* surface, bool reset)
{
	if (reset)
		glDeleteTextures(1, &textureID);
	
	glGenTextures(1, &textureID);	
	glBindTexture(GL_TEXTURE_2D, textureID);

	int Mode = GL_RGB;

	if (surface->format->BytesPerPixel == 4) 
	{
		Mode = GL_RGBA;
	}
	else if (surface->format->BytesPerPixel == 1)
	{
		Mode = GL_DEPTH_COMPONENT;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	width = surface->w;
	height = surface->h;
	glTexImage2D(GL_TEXTURE_2D, 0, Mode, surface->w, surface->h, 0, Mode, GL_UNSIGNED_BYTE, surface->pixels);

	glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture::UseTexture()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::ClearTexture()
{
	glDeleteTextures(1, &textureID);
	textureID = 0;
	width = 0;
	height = 0;
	bitDepth = 0;
}