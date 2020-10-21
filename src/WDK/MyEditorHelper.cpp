#include "MyEditorHelper.h"
#include "../ENGINE/Entity.h"
#include "../ENGINE/Path.h"
#include "Ladder.h"
#include "Door.h"
#include "Platform.h"
#include "NPC.h"
#include "../ENGINE/Editor.h"
#include "../ENGINE/Game.h"
#include "../ENGINE/Renderer.h"

void MyEditorHelper::CreateLevelStart()
{
	loadListPaths.clear();
	loadListMovingPlatforms.clear();
	loadListLadderGroups.clear();
	loadListDoors.clear();
}

void MyEditorHelper::CreateLevelEnd()
{
	Entity::nextValidID = 2; // highestID + 1;

	for (auto const& [key, ladderGroup] : loadListLadderGroups)
	{
		Ladder* topLadder = nullptr;
		for (unsigned int i = 0; i < ladderGroup.size(); i++)
		{
			if (topLadder == nullptr || ladderGroup[i]->position.y < topLadder->position.y)
			{
				topLadder = ladderGroup[i];
			}
		}
		for (unsigned int i = 0; i < ladderGroup.size(); i++)
		{
			ladderGroup[i]->top = topLadder;
		}
	}

	if (editor->game->player != nullptr)
	{
		// Set the player's start position if we are entering from a door
		if (editor->game->nextDoorID > -1)
		{
			for (unsigned int i = 0; i < loadListDoors.size(); i++)
			{
				if (loadListDoors[i]->id == editor->game->nextDoorID)
				{
					editor->game->player->SetPosition(loadListDoors[i]->position + editor->game->player->GetSprite()->pivot);
				}
			}
			editor->game->nextDoorID = -1;
		}

		// Set the camera's position to the player's instantly
		editor->game->renderer.camera.FollowTarget(*editor->game, true);
	}


	// Match all platforms moving on paths with their assigned path
	for (unsigned int i = 0; i < loadListMovingPlatforms.size(); i++)
	{
		for (unsigned int k = 0; k < loadListPaths.size(); k++)
		{
			if (loadListMovingPlatforms[i]->pathID == loadListPaths[k]->id)
			{
				loadListMovingPlatforms[i]->currentPath = loadListPaths[k];
				loadListMovingPlatforms[i]->SetPosition(loadListPaths[k]->nodes[0]->point);
				break;
			}
		}
	}
}

void MyEditorHelper::Render(const Renderer& renderer)
{
	for (unsigned int i = 0; i < editor->game->entities.size(); i++)
	{
		if (editor->game->entities[i]->etype == "door")
		{
			Door* door = static_cast<Door*>(editor->game->entities[i]);

			Vector2 destPos = door->GetDestination();
			if (destPos.x != 0 && destPos.y != 0)
			{
				destPos = destPos;
				Vector2 doorCenter = door->GetCenter();
				Vector2 doorPos = door->GetPosition() + doorCenter;
				//SDL_RenderDrawLine(renderer->renderer, doorPos.x, doorPos.y, destPos.x + doorCenter.x, destPos.y + doorCenter.y);
			}
		}
	}

}

void MyEditorHelper::PlaceObject(Vector2& snappedPosition)
{
	if (editor->objectMode == "path")
	{
		// If we do not have a path, create a neww one
		/*
		if (currentPath == nullptr)
		{
			currentPath = neww Path(snappedPosition);
			currentPath->AddPointToPath(snappedPosition);
			game->entities.push_back(currentPath);
			game->SortEntities(game->entities);
		}
		else // otherwise, add to the current path
		{
			currentPath->AddPointToPath(snappedPosition);
		}
		*/
	}
	else if (editor->objectMode == "door")
	{
		if (!placingDoor)
		{
			std::cout << "trying to spawn entrance" << std::endl;
			currentDoor = static_cast<Door*>(editor->game->SpawnEntity(editor->objectMode, snappedPosition, editor->entitySubtype));
			if (currentDoor != nullptr)
			{
				std::cout << "placing door set true" << std::endl;
				placingDoor = true;
				editor->game->SortEntities(editor->game->entities);
			}

		}
		else
		{
			std::cout << "trying to spawn destination" << std::endl;
			Door* destination = static_cast<Door*>(editor->game->SpawnEntity("door", snappedPosition, editor->entitySubtype));
			if (destination != nullptr)
			{
				std::cout << "placing door set false" << std::endl;
				placingDoor = false;
				currentDoor = nullptr;

				editor->game->SortEntities(editor->game->entities);
			}
		}

	}
	else if (editor->objectMode == "ladder")
	{
		if (!placingLadder)
		{
			std::cout << "trying to spawn ladder start" << std::endl;
			currentLadder = static_cast<Ladder*>(editor->game->SpawnEntity("ladder", snappedPosition, editor->entitySubtype));
			if (currentLadder != nullptr)
			{
				std::cout << "placing ladder set true" << std::endl;
				placingLadder = true;
				editor->game->SortEntities(editor->game->entities);
			}
		}
		else
		{
			// only spawn if the position we clicked at is on the same column as the ladder start
			if (snappedPosition.x == currentLadder->GetPosition().x)
			{
				std::cout << "trying to spawn ladder end" << std::endl;
				Ladder* ladderEnd = static_cast<Ladder*>(editor->game->SpawnEntity("ladder", snappedPosition, editor->entitySubtype));

				std::vector<Ladder*> ladderGroup;

				if (ladderEnd != nullptr)
				{
					std::cout << "placing ladder set false" << std::endl;
					placingLadder = false;

					// Define which edges of the ladder are the top and bottom
					bool ladderGoesUp = false;
					if (ladderEnd->GetPosition().y > currentLadder->GetPosition().y)
					{
						ladderEnd->GetAnimator()->SetState("bottom");
						currentLadder->GetAnimator()->SetState("top");
					}
					else
					{
						ladderEnd->GetAnimator()->SetState("top");
						currentLadder->GetAnimator()->SetState("bottom");
						ladderGoesUp = true;
					}

					ladderGroup.push_back(currentLadder);
					ladderGroup.push_back(ladderEnd);

					if (ladderGoesUp)
					{
						// Connect the two edges by spawning the middle parts
						while (snappedPosition.y < currentLadder->GetPosition().y)
						{
							ladderEnd = static_cast<Ladder*>(editor->game->SpawnEntity("ladder",
								snappedPosition, editor->entitySubtype));
							if (ladderEnd != nullptr)
								ladderGroup.push_back(ladderEnd);
							snappedPosition.y += TILE_SIZE * Camera::MULTIPLIER;
						}
					}
					else
					{
						// Connect the two edges by spawning the middle parts
						while (snappedPosition.y > currentLadder->GetPosition().y)
						{
							ladderEnd = static_cast<Ladder*>(editor->game->SpawnEntity("ladder",
								snappedPosition, editor->entitySubtype));
							if (ladderEnd != nullptr)
								ladderGroup.push_back(ladderEnd);
							snappedPosition.y -= TILE_SIZE * Camera::MULTIPLIER;
						}
					}

					currentLadder = nullptr;

					Ladder* topLadder = nullptr;
					for (unsigned int i = 0; i < ladderGroup.size(); i++)
					{
						if (topLadder == nullptr || ladderGroup[i]->position.y < topLadder->position.y)
						{
							topLadder = ladderGroup[i];
						}
					}
					for (unsigned int i = 0; i < ladderGroup.size(); i++)
					{
						ladderGroup[i]->top = topLadder;
					}

					editor->game->SortEntities(editor->game->entities);
				}
			}
		}
	}
	else
	{
		Entity* entity = editor->game->SpawnEntity(editor->objectMode, snappedPosition, editor->entitySubtype);
		if (entity != nullptr)
		{
			entity->Init(editor->game->entityTypes[editor->objectMode][editor->entitySubtype]);
			entity->rotation = editor->previewMap[editor->objectMode]->rotation;
			editor->game->SortEntities(editor->game->entities);
		}
	}
}

void MyEditorHelper::DeleteObject(bool shouldDeleteThis, Entity* entityToDelete)
{
	int ladderIndex = -1;

	// If this entity is a path, check all points in the path
	// (This must be dealt with outside of the shouldDelete section
	// because each path contains multiple points that must each be
	// deleted individually if any of them have been clicked on)
	if (entityToDelete->etype == "path")
	{
		Path* path = dynamic_cast<Path*>(entityToDelete);
		if (path->IsPointInPath(entityToDelete->position))
		{
			path->RemovePointFromPath(entityToDelete->position);

			// Only if there are no points in the path do we remove the path
			if (path->nodes.size() == 0)
			{
				//game->ShouldDeleteEntity(i);
				return;
			}
		}
	}
	else if (entityToDelete->etype == "switch")
	{
		for (int k = 0; k < editor->game->entities.size(); k++)
		{
			if (entityToDelete->etype == "platform")
			{
				Platform* platform = static_cast<Platform*>(editor->game->entities[k]);
				if (platform->attachedSwitch == entityToDelete)
				{
					platform->attachedSwitch = nullptr;
				}
			}
		}
	}

	if (shouldDeleteThis)
	{
		if (entityToDelete->etype == "door")
		{
			// Save destination and delete the entry door
			Door* door = static_cast<Door*>(entityToDelete);
			Vector2 dest = door->GetDestination();

			// Only delete if both doors have been placed
			if (dest != Vector2(0, 0))
			{
				//game->ShouldDeleteEntity(i);

				// Delete the exit door
				for (unsigned int k = 0; k < editor->game->entities.size(); k++)
				{
					if (editor->game->entities[k]->GetPosition() == dest)
					{
						editor->game->ShouldDeleteEntity(k);
						return;
					}
				}
			}
		}
		else if (entityToDelete->etype == "ladder")
		{
			//ladderIndex = i;
		}
		else
		{
			editor->game->ShouldDeleteEntity(entityToDelete);
			return;
		}


	}

	// TODO: This never executes, thus we can't (instantly) delete ladders!
	if (ladderIndex >= 0 && !placingLadder)
	{
		std::string startingState = editor->game->entities[ladderIndex]->GetAnimator()->currentState->name;
		Vector2 lastPosition = editor->game->entities[ladderIndex]->GetPosition();
		editor->game->ShouldDeleteEntity(ladderIndex);

		if (startingState == "top")
			DestroyLadder("top", lastPosition);
		else if (startingState == "bottom")
			DestroyLadder("bottom", lastPosition);
		else if (startingState == "middle")
		{
			DestroyLadder("top", lastPosition);
			DestroyLadder("bottom", lastPosition);
		}
	}
}

void MyEditorHelper::DestroyLadder(std::string startingState, Vector2 lastPosition)
{
	bool exit = false;
	while (!exit)
	{
		// go over all the entities and check to see if there is one at the next position
		// if it is, and it is a ladder, then delete it and keep going
		// otherwise, we are done, and can exit the loop

		if (startingState == "top")
			lastPosition.y += editor->GRID_SIZE;
		else if (startingState == "bottom")
			lastPosition.y -= editor->GRID_SIZE;

		exit = true;

		for (unsigned int i = 0; i < editor->game->entities.size(); i++)
		{
			if (editor->game->entities[i]->etype == "ladder")
			{
				if (RoundToInt(editor->game->entities[i]->GetPosition()) == RoundToInt(lastPosition))
				{
					editor->game->ShouldDeleteEntity(i);
					exit = false;
					break;
				}
			}
		}
	}
}