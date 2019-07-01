#pragma once
#include "Entity.h"
#include "Vector2.h"

class Player : public Entity
{
public:
	Vector2 startPosition;
	Player();
	~Player();
	void Update(Game& game);
	void UpdatePhysics(Game& game);
	void CheckCollisions(Game& game, bool& collideX, bool& collideY);
	void ResetPosition();
};

