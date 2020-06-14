#ifndef BUG_H
#define BUG_H
#pragma once

#include "Entity.h"

class Bug : public Entity
{
public:
	Bug(Vector2 pos);
	~Bug();

	void OnTriggerStay(Entity* other, Game& game);
	void OnTriggerEnter(Entity* other, Game& game);
	void OnTriggerExit(Entity* other, Game& game);

	void Save(std::ostringstream& level);
};

#endif