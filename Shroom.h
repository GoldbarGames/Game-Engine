#pragma once
#include "PhysicsEntity.h"
class Shroom : public Entity
{
public:
	int spriteIndex = 0;

	Shroom(Vector2 pos);
	~Shroom();

	void Save(std::ostringstream& level);

	void OnTriggerEnter(Entity* other, Game& game);
};

