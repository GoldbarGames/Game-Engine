#include "SpriteManager.h"
#include "SDL.h"
#include <SDL_image.h>
#include <GL/glew.h>

SpriteManager::SpriteManager()
{
	//TODO: Find all PNG files in the assets folder automatically?
	//OR find a way to only load them when needed?

	LoadImage("assets/sprites/wdk_walk.png");
	LoadImage("assets/sprites/wdk_blink.png");
	LoadImage("assets/bg/bg.png");
	LoadImage("assets/sprites/floor.png");

	LoadImage("assets/tiles/housetiles5.png");

}

void SpriteManager::LoadImage(std::string const & image)
{
	images[image].reset(IMG_Load(image.c_str()));
}

SDL_Surface* SpriteManager::GetImage(std::string const& image)
{
	return images[image].get();
}

SpriteManager::~SpriteManager()
{

}
