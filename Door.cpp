#include "Door.h"
#include "Game.h"
#include "debug_state.h"

Door::Door(Vector2 pos, Vector2 dest) : Entity(pos)
{
	destination = dest;
	layer = DrawingLayer::OBJECT;
	drawOrder = 90;
	etype = "door";
}

Door::~Door()
{

}

bool Door::CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera)
{
	bool shouldSpawn = true;

	if (currentSprite == nullptr)
		return false;

	//TODO: Maybe there's a better way to initialize the bounds for the sprite
	SDL_Rect myBounds = *(GetBounds());
	if (useCamera)
	{
		myBounds.x = spawnPosition.x - game.camera.x;
		myBounds.y = spawnPosition.y - game.camera.y;
	}
	else
	{
		myBounds.x = spawnPosition.x;
		myBounds.y = spawnPosition.y;
	}


	SDL_Rect tileBelowMyBoundsLeft = myBounds;
	tileBelowMyBoundsLeft.y += myBounds.h;
	tileBelowMyBoundsLeft.w = TILE_SIZE * SCALE;
	tileBelowMyBoundsLeft.h = TILE_SIZE * SCALE;

	SDL_Rect tileBelowMyBoundsRight = myBounds;
	tileBelowMyBoundsRight.x += TILE_SIZE * SCALE;
	tileBelowMyBoundsRight.y += myBounds.h;
	tileBelowMyBoundsRight.w = TILE_SIZE * SCALE;
	tileBelowMyBoundsRight.h = TILE_SIZE * SCALE;

	bool hasGroundLeft = false;
	bool hasGroundRight = false;

	for (unsigned int i = 0; i < game.entities.size(); i++)
	{
		const SDL_Rect * theirBounds = game.entities[i]->GetBounds();

		// 1. Check to make sure that this door does NOT intersect with any other doors
		if (game.entities[i]->etype == "door")
		{
			if (SDL_HasIntersection(&myBounds, theirBounds))
			{
				shouldSpawn = false;
			}
		}

		// 2. Check to make sure that this door is one tile above a tile on the foreground layer
		if (game.entities[i]->impassable)
		{
			if (SDL_HasIntersection(&tileBelowMyBoundsLeft, theirBounds))
			{
				hasGroundLeft = true;
			}

			if (SDL_HasIntersection(&tileBelowMyBoundsRight, theirBounds))
			{
				hasGroundRight = true;
			}
		}
	}

	if (!hasGroundLeft || !hasGroundRight)
		shouldSpawn = false;

	return shouldSpawn;
}