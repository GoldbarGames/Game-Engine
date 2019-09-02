#include "tile.h"
#include "debug_state.h"
#include "globals.h"
#include "Game.h"

using std::string;

Tile::Tile(Vector2 pos, Vector2 frame, SDL_Texture * image, Renderer * renderer) : Entity(pos)
{
	currentSprite = new Sprite(frame, image, renderer);
}

Tile::~Tile()
{

}

void Tile::Animate()
{

}

const SDL_Rect* Tile::GetBounds()
{
	return currentSprite->GetRect();
}

void Tile::Destroy()
{

}

bool Tile::CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera)
{
	for (unsigned int i = 0; i < game.entities.size(); i++)
	{
		if (game.entities[i]->GetPosition() == spawnPosition &&
			game.entities[i]->layer == layer &&
			game.entities[i]->etype == "tile")
		{
			return false;
		}
	}

	return true;
}