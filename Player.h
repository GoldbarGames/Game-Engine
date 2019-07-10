#pragma once
#include "PhysicsEntity.h"
#include "Vector2.h"

class Player : public PhysicsEntity
{
public:
	Vector2 startPosition;
	Player();
	~Player();
	void Update(Game& game);
	void UpdatePhysics(Game& game);
	void Render(SDL_Renderer * renderer, Vector2 cameraOffset);
	void CheckCollisions(Game& game, bool& collideX, bool& collideY);
	void ResetPosition();
};

