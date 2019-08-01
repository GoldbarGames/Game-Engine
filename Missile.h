#pragma once
#include "PhysicsEntity.h"

class Missile : public PhysicsEntity
{
public:
	Missile();
	~Missile();
	void Update(Game& game);
	void UpdatePhysics(Game& game);
	void Render(SDL_Renderer* renderer, Vector2 cameraOffset);
	bool CheckCollisions(Game& game);
	void CalculateCollider(Vector2 cameraOffset);
};

