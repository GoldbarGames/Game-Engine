#ifndef SWITCH_H
#define SWITCH_H
#pragma once
#include "../ENGINE/Entity.h"

class Game;

class Switch : public Entity
{
public:
	std::unordered_map<int, Entity*> collidingEntities;

	Switch(Vector2 pos);

	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);

	void Save(std::unordered_map<std::string, std::string>& map);

	static Entity* __stdcall Create(const Vector2& pos) { return neww Switch(pos); };
};

#endif