#ifndef SWITCH_H
#define SWITCH_H
#pragma once
#include "MyEntity.h"

class Game;

class Switch : public MyEntity
{
public:
	std::unordered_map<int, Entity*> collidingEntities;

	Switch(Vector2 pos);

	void OnTriggerStay(MyEntity& other, Game& game);
	void OnTriggerEnter(MyEntity& other, Game& game);
	void OnTriggerExit(MyEntity& other, Game& game);

	void Save(std::unordered_map<std::string, std::string>& map);

	static Entity* __stdcall Create(const Vector2& pos) { return neww Switch(pos); };
};

#endif