#ifndef SHROOM_H
#define SHROOM_H
#pragma once

#include "Entity.h"

class Shroom : public Entity
{
public:
	Shroom(const Vector2& pos);
	~Shroom();

	void Init(const std::string& n);
	void Update(Game& game);
	void Save(std::ostringstream& level);

	void OnTriggerEnter(Entity& other, Game& game);

	static Entity* __stdcall Create(const Vector2& pos) { return new Shroom(pos); };
};

#endif