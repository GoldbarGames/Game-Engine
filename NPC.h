#ifndef NPC_H
#define NPC_H
#pragma once
#include "Entity.h"

class NPC : public Entity
{
public:
	std::string cutsceneLabel = "test1";
	int spriteIndex = 0;
	NPC(const std::string& n, const Vector2& pos);
	~NPC();
	bool CanSpawnHere(const Vector2& spawnPosition, Game& game, bool useCamera = true);
	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);
	void ChangeCollider(float x, float y, float w, float h);
	void GetProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);
	void Save(std::ostringstream& level);

	static Entity* __stdcall Create(const Vector2& pos) { return new NPC("", pos); };
};

#endif