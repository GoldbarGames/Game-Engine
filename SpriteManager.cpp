#include "SpriteManager.h"
#include "SDL.h"
#include <SDL_image.h>
#include <GL/glew.h>

SpriteManager::SpriteManager()
{

}

SDL_Surface* SpriteManager::GetImage(std::string const& image)
{
	if (images[image].get() == nullptr)
		images[image].reset(IMG_Load(image.c_str()));
	return images[image].get();
}

Vector2 SpriteManager::GetPivotPoint(std::string const& filename)
{
	return pivotPoints[filename];
}

SpriteManager::~SpriteManager()
{

}
