#pragma once
#include "Entity.h"

class PhysicsEntity : public Entity
{
protected:
	Vector2 velocity = Vector2(0, 0);
	Vector2 acceleration = Vector2(0, 0);

	float horizontalSpeed = 0.001f;

public:
	PhysicsEntity();
	~PhysicsEntity();
};

