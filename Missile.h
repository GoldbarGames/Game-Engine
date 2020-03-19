#pragma once
#include "PhysicsEntity.h"

class Missile : public Entity
{
	Timer timeToLive;
public:
	bool destroyed = false;
	Missile(Vector2 pos);
	~Missile();
	void Update(Game& game);
	void UpdatePhysics(Game& game);
	bool CheckCollisions(Game& game);
	void SetVelocity(Vector2 newVelocity);
	void Destroy();
};

