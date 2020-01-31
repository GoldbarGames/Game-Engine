#pragma once
#include "PhysicsEntity.h"

class Bug : public Entity
{
public:
	int spriteIndex = 0;

	Bug(Vector2 pos);
	~Bug();

	void OnTriggerStay(Entity* other, Game& game);
	void OnTriggerEnter(Entity* other, Game& game);
	void OnTriggerExit(Entity* other, Game& game);

	void Save(std::ostringstream& level);
};

