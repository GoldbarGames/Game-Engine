#include "SpriteManager.h"
#include "SDL.h"
#include <SDL_image.h>
#include <GL/glew.h>

SpriteManager::SpriteManager()
{

}

SDL_Surface* SpriteManager::GetImage(std::string const& imagePath)
{
	if (images[imagePath].get() == nullptr)
		images[imagePath].reset(IMG_Load(imagePath.c_str()));
	return images[imagePath].get();
}

Vector2 SpriteManager::GetPivotPoint(std::string const& filename)
{
	return pivotPoints[filename];
}

SpriteManager::~SpriteManager()
{

}
