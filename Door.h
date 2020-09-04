#ifndef DOOR_H
#define DOOR_H
#pragma once

#include "Entity.h"

class Door : public Entity
{
	Vector2 destination = Vector2(0, 0);
public:	
	bool isLocked = false;
	std::string nextLevelName = "";
	Door(Vector2 pos, Vector2 dest);
	~Door();

	void Update(Game& game);

	void GetProperties(FontInfo* font, std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);

	bool CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera = true);
	Vector2 GetDestination();
	void SetDestination(Vector2 dest);

	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);

	void Save(std::ostringstream& level);

	static Entity* __stdcall Create(const Vector2& pos) { return new Door(pos, pos); };
};

#endif