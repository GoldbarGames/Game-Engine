#include "tile.h"
#include "debug_state.h"
#include "globals.h"

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
	currentFrame = Vector2( (frame.x-1) * TILE_SIZE, (frame.y-1) * TILE_SIZE); // this converts from human coords to pixel coords

	// Set start position
	windowRect.x = 0;
	windowRect.y = 0;

	textureRect.x = currentFrame.x;
	textureRect.y = currentFrame.y;
	textureRect.w = TILE_SIZE;
	textureRect.h = TILE_SIZE;
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
	windowRect.w = TILE_SIZE * SCALE;
	windowRect.h = TILE_SIZE * SCALE;

	//Animate(speed);
	SDL_RenderCopy(renderer, texture, &textureRect, &windowRect);

	if (GetModeDebug())
	{
		if (impassable)
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		else
			SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

		SDL_RenderDrawRect(renderer, GetBounds());
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	}
}