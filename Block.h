#pragma once
#include "PhysicsEntity.h"

class Block : public PhysicsEntity
{
public:
	int spriteIndex = 0;
	Block(Vector2 pos);
	~Block();
	//void Push(Vector2 direction, Game &game);
};
