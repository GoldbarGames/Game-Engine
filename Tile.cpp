#include "tile.h"
#include "debug_state.h"

using std::string;

Tile::Tile()
{

}

Tile::~Tile()
{

}

Tile::Tile(Vector2 frame, SDL_Surface * image, SDL_Renderer * renderer)
{
	texture = SDL_CreateTextureFromSurface(renderer, image);
	currentFrame = Vector2( (frame.x-1) * 24, (frame.y-1) * 24); // this converts from human coords to pixel coords

	// Set start position
	windowRect.x = 0;
	windowRect.y = 0;

	textureRect.x = currentFrame.x;
	textureRect.y = currentFrame.y;
	textureRect.w = 24;
	textureRect.h = 24;
}

void Tile::Animate()
{

}

const SDL_Rect* Tile::GetBounds()
{
	return &windowRect;
}

void Tile::Destroy()
{
	SDL_DestroyTexture(texture);
}

void Tile::Render(SDL_Renderer * renderer)
{
	windowRect.x = position.x;
	windowRect.y = position.y;
	windowRect.w = 24 * scale.x;
	windowRect.h = 24 * scale.y;

	//Animate(speed);
	SDL_RenderCopy(renderer, texture, &textureRect, &windowRect);

	if (GetModeDebug())
	{
		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
		SDL_RenderDrawRect(renderer, GetBounds());
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	}
}