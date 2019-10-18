#pragma once
#include "Entity.h"

class Door : public Entity
{
	Vector2 destination = Vector2(0, 0);
	bool isLocked = false;
public:	
	int spriteIndex = 0;
	Door(Vector2 pos, Vector2 dest);
	~Door();
	bool CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera = true);
	Vector2 GetDestination();
	void SetDestination(Vector2 dest);

	void OnTriggerStay(Entity* other, Game& game);
	void OnTriggerEnter(Entity* other, Game& game);
	void OnTriggerExit(Entity* other, Game& game);

	void Save(std::ostringstream& level);
};

