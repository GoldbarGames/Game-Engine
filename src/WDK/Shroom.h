#ifndef SHROOM_H
#define SHROOM_H
#pragma once

#include "../ENGINE/Entity.h"

class Shroom : public Entity
{
public:
	Shroom(const Vector2& pos);
	~Shroom();

	void Init(const std::string& n);
	void Update(Game& game);

	void OnTriggerEnter(Entity& other, Game& game);

	void Save(std::unordered_map<std::string, std::string>& map);

	static Entity* __stdcall Create(const Vector2& pos) { return neww Shroom(pos); };
};

#endif