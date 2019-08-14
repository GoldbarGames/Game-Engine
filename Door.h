#pragma once
#include "Entity.h"

class Door : public Entity
{
	Vector2 destination = Vector2(0, 0);
	bool isLocked = false;
public:	
	Door(Vector2 pos, Vector2 dest);
	~Door();
	bool CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera = true);
	Vector2 GetDestination();
	void SetDestination(Vector2 dest);

	void OnTriggerStay(Entity* other);
	void OnTriggerEnter(Entity* other);
	void OnTriggerExit(Entity* other);
};

