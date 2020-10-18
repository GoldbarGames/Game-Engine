#ifndef LADDER_H
#define LADDER_H
#pragma once

#include "../ENGINE/Entity.h"

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

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	static Entity* __stdcall Create(const Vector2& pos) { return neww Ladder(pos); };
};

#endif