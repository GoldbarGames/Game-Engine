#ifndef NPC_H
#define NPC_H
#pragma once
#include "Entity.h"

class NPC : public Entity
{
public:
	std::string cutsceneLabel = "test1";
	int spriteIndex = 0;
	NPC(std::string n, Vector2 pos);
	~NPC();
	bool CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera = true);
	void OnTriggerStay(Entity* other, Game& game);
	void OnTriggerEnter(Entity* other, Game& game);
	void OnTriggerExit(Entity* other, Game& game);
	void ChangeCollider(float x, float y, float w, float h);
	void GetProperties(Renderer * renderer, FontInfo* font, std::vector<Property*>& properties);
	void SetProperty(std::string prop, std::string newValue);
	void Save(std::ostringstream& level);

	static Entity* __stdcall Create(const Vector2& pos) { return new NPC("", pos); };
};

#endif