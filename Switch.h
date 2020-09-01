#ifndef SWITCH_H
#define SWITCH_H
#pragma once
#include "Entity.h"

class Game;

class Switch : public Entity
{
public:
	Switch(Vector2 pos);

	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);

	void Save(std::ostringstream& level);

	static Entity* __stdcall Create(const Vector2& pos) { return new Switch(pos); };
};

#endif