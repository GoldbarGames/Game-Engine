#include "Texture.h"
#include <iostream>

int Texture::lastTextureID = -1;
int Texture::lastActiveTexture = -1;

Texture::Texture(const std::string& path)
{
	textureID = 0;
	width = 0;
	height = 0;
	filePath = path;
}

Texture::~Texture()
{
	ClearTexture();
}

// This function is used exclusively for loading textures on imported 3D meshes
bool Texture::LoadTexture()
{
	//TODO: Make this work with PhysFS, possibly refactor it entirely

	SDL_Surface* surface = IMG_Load(filePath.c_str());
	if (surface == nullptr)
	{
		surface = IMG_Load("assets/gui/white.png");
		std::cout << "FAILED TO LOAD TEXTURE: " << filePath << std::endl;
		return false;
	}

	LoadTexture(surface);

	SDL_FreeSurface(surface);
	return true;
}

void Texture::LoadTexture(unsigned int& buffer, int w, int h)
{
	width = w;
	height = h;
	glGenTextures(1, &buffer);
	glBindTexture(GL_TEXTURE_2D, buffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);
	textureID = buffer;
}

void Texture::LoadTexture(SDL_Surface* surface, bool reset)
{
	if (reset)
		glDeleteTextures(1, &textureID);
	
	glGenTextures(1, &textureID);	
	glBindTexture(GL_TEXTURE_2D, textureID);
	//std::cout << textureID << std::endl;

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

	glTexImage2D(GL_TEXTURE_2D, 0, Mode, width, height, 0, Mode, GL_UNSIGNED_BYTE, surface->pixels);

	glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture::UseTexture(int textureNum)
{
	if (textureID != lastTextureID)
	{
		lastTextureID = textureID;

		if (lastActiveTexture != textureNum)
		{
			lastActiveTexture = textureNum;
			glActiveTexture(textureNum);
		}

		glBindTexture(GL_TEXTURE_2D, textureID);
	}
}

void Texture::ClearTexture()
{
	glDeleteTextures(1, &textureID);
	textureID = 0;
	width = 0;
	height = 0;
}