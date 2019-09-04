#pragma once
#include "PhysicsEntity.h"

class NPC : public PhysicsEntity
{
public:
	std::string name = "";
	NPC(std::string n, Vector2 pos);
	~NPC();
	bool CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera = true);
	void OnTriggerStay(Entity* other);
	void OnTriggerEnter(Entity* other);
	void OnTriggerExit(Entity* other);
	void ChangeCollider(float x, float y, float w = 0.75f, float h = 0.9f);
};

