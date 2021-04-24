#include "PathMover.h"
#include "Game.h"

bool PathMover::MoveAlongPath(Entity& entity, Game& game)
{
	if (currentPath == nullptr && pathID > -1)
	{
		// Find the path and assign it
		bool foundPath = false;
		for (int i = 0; i < game.entities.size(); i++)
		{
			if (game.entities[i]->id == pathID && game.entities[i]->etype == "path")
			{
				currentPath = static_cast<Path*>(game.entities[i]);
				foundPath = true;
				break;
			}
		}

		if (!foundPath)
		{
			pathID = -1;
		}
	}
	else if (currentPath != nullptr) // if we have the path, move to the next point
	{
		if (pathNodeID >= currentPath->nodes.size())
			return false;

		// If we just changed directions, wait a bit before moving again
		// TODO: Customize the delay time for each platform/path
		if (wasMovingForward != movingForwardOnPath)
		{
			delayCounter++;
			if (delayCounter < delayMax)
				return false;
		}

		delayCounter = 0;
		wasMovingForward = movingForwardOnPath;
		entity.lastPosition = entity.position;

		if (pathSpeed > 0)
		{
			LerpVector3(entity.position, currentPath->nodes[pathNodeID]->position, pathSpeed, 2.0f);
		}

		if (RoundToInt(entity.position) == RoundToInt(currentPath->nodes[pathNodeID]->position))
		{
			// loop for now
			if (movingForwardOnPath)
			{
				pathNodeID++;
				if (pathNodeID >= currentPath->nodes.size())
				{
					movingForwardOnPath = false;
					pathNodeID--;
				}
			}
			else
			{
				pathNodeID--;
				if (pathNodeID < 0)
				{
					movingForwardOnPath = true;
					pathNodeID++;
				}
			}

		}

		entity.CalculateCollider();
	}

	return true;
}

PathNode* PathMover::GetCurrentNode()
{
	if (currentPath != nullptr)
		return currentPath->nodes[pathNodeID];
	else
		return nullptr;
}