#ifndef BUG_H
#define BUG_H
#pragma once

#include "Entity.h"

class Bug : public Entity
{
public:
	Bug(const Vector2& pos);
	~Bug();

	void OnTriggerStay(Entity& other, Game& game);
	void OnTriggerEnter(Entity& other, Game& game);
	void OnTriggerExit(Entity& other, Game& game);

	void Save(std::ostringstream& level);

	static Entity* __stdcall Create(const Vector2& pos) { return new Bug(pos); };
};

#endif