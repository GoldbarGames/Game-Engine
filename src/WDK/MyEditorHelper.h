#ifndef MYEDITORHELPER_H
#define MYEDITORHELPER_H
#pragma once

#include "../ENGINE/EditorHelper.h"

class Door;
class Ladder;
class NPC;
class Path;
class Platform;
class Renderer;
class Vector2;

#include <string>
#include <vector>
#include <map>

class MyEditorHelper : public EditorHelper
{
public:
	void Render(const Renderer& renderer);
	void CreateLevelEnd();
	void CreateLevelStart();
	void PlaceObject(Vector2& snappedPosition);
	void DeleteObject(bool shouldDeleteThis, Entity* entityToDelete);

	Door* currentDoor = nullptr;
	Ladder* currentLadder = nullptr;
	NPC* currentNPC = nullptr;
	Path* currentPath = nullptr;

	bool placingDoor = false;
	bool placingLadder = false;

	std::vector<Path*> loadListPaths;
	std::vector<Platform*> loadListMovingPlatforms;
	std::map<int, std::vector<Ladder*>> loadListLadderGroups;
	std::vector<Door*> loadListDoors;

private:
	void DestroyLadder(std::string startingState, Vector2 lastPosition);
};

#endif