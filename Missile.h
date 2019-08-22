#pragma once
#include "PhysicsEntity.h"

class Missile : public PhysicsEntity
{
	Timer timeToLive;
public:
	float angle = 0;
	Missile(Vector2 pos);
	~Missile();
	void Update(Game& game);
	void UpdatePhysics(Game& game);
	void Render(Renderer* renderer, Vector2 cameraOffset);
	bool CheckCollisions(Game& game);
	void CalculateCollider(Vector2 cameraOffset);
	void Pause(Uint32 ticks) override;
	void Unpause(Uint32 ticks) override;
};

