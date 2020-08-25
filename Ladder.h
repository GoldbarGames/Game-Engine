#ifndef LADDER_H
#define LADDER_H
#pragma once

#include "Entity.h"

class Ladder : public Entity
{
public:
	void Render(const Renderer& renderer);

	Ladder(const Vector2& pos);
	~Ladder();

	Ladder* top = nullptr;

	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);

	void Save(std::ostringstream& level);

	static Entity* __stdcall Create(const Vector2& pos) { return new Ladder(pos); };
};

#endif