#include "Door.h"
#include "Game.h"
#include "debug_state.h"

Door::Door(Vector2 pos, Vector2 dest) : Entity(pos)
{
	destination = dest;
	layer = DrawingLayer::OBJECT;
	drawOrder = 90;
	etype = "door";
	trigger = true;
}

Door::~Door()
{

}

void Door::OnTriggerStay(Entity* other, Game& game)
{

}

void Door::OnTriggerEnter(Entity* other, Game& game)
{
	if (other->etype == "player")
	{
		Player* player = static_cast<Player*>(other);
		player->currentDoor = this;
	}
}

void Door::OnTriggerExit(Entity* other, Game& game)
{
	if (other->etype == "player")
	{
		Player* player = static_cast<Player*>(other);
		player->currentDoor = nullptr;
	}
}

Vector2 Door::GetDestination()
{
	return destination;
}

void Door::SetDestination(Vector2 dest)
{
	destination = dest;
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
		myBounds.x = (int)spawnPosition.x;
		myBounds.y = (int)spawnPosition.y;
	}
	else
	{
		myBounds.x = (int)spawnPosition.x;
		myBounds.y = (int)spawnPosition.y;
	}

	int SCALE = Renderer::GetScale();

	SDL_Rect tileBelowMyBoundsLeft = myBounds;
	tileBelowMyBoundsLeft.y += myBounds.h;
	tileBelowMyBoundsLeft.w = game.editor->GRID_SIZE * SCALE;
	tileBelowMyBoundsLeft.h = game.editor->GRID_SIZE * SCALE;

	SDL_Rect tileBelowMyBoundsRight = myBounds;
	tileBelowMyBoundsRight.x += game.editor->GRID_SIZE * SCALE;
	tileBelowMyBoundsRight.y += myBounds.h;
	tileBelowMyBoundsRight.w = game.editor->GRID_SIZE * SCALE;
	tileBelowMyBoundsRight.h = game.editor->GRID_SIZE * SCALE;

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

		//TODO: Check to make sure that we can't place a door inside a solid tile

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
		shouldSpawn = true; 

	//TODO: Set this to false. It fails right now because the other entities sprites
	// have not had their window rects set, so they are at 0,0 which would always fail
	// to cause a collision and therefore shouldSpawn always becomes false

	return shouldSpawn;
}

void Door::Save(std::ostringstream& level)
{
	int SCALE = Renderer::GetScale();
	Vector2 pos = GetPosition();

	level << std::to_string(id) << " " << etype << " " << (pos.x / SCALE) << " " <<
		(pos.y / SCALE) << " " << (GetDestination().x / SCALE) <<
		" " << (GetDestination().y / SCALE) << " " << spriteIndex << "" << std::endl;
}