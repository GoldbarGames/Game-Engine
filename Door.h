#ifndef DOOR_H
#define DOOR_H
#pragma once

#include "Entity.h"

class Door : public Entity
{
	Door* attachedDoor = nullptr;
public:	
	int destinationID = -1;
	bool isLocked = false;
	std::string nextLevelName = "none";
	Door(Vector2 pos);
	~Door();

	void Update(Game& game);

	void GetProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);

	bool CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera = true);
	Vector2 GetDestination();

	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	static Entity* __stdcall Create(const Vector2& pos) { return new Door(pos); };
};

#endif