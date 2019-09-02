#include "Renderer.h"
#include "SpriteManager.h"

SpriteManager::SpriteManager(Renderer* r)
{
	renderer = r;
}

SDL_Texture* SpriteManager::GetImage(std::string const& imagePath)
{
	if (images[imagePath].get() == nullptr)
	{
		SDL_Surface * surface = IMG_Load(imagePath.c_str());
		SDL_Texture * texture = renderer->CreateTextureFromSurface(surface);
		images[imagePath].reset(texture);
		SDL_FreeSurface(surface);
	}
		
	return images[imagePath].get();
}

Vector2 SpriteManager::GetPivotPoint(std::string const& filename)
{
	return pivotPoints[filename];
}

SpriteManager::~SpriteManager()
{

}
