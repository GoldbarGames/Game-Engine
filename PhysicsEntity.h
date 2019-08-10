#pragma once
#include "Entity.h"

class PhysicsEntity : public Entity
{
protected:
	Vector2 velocity = Vector2(0, 0);
	Vector2 acceleration = Vector2(0, 0);
	float horizontalSpeed = 0.001f;

public:

	SDL_Rect* collider = nullptr;        // adjust the bounds this way
	SDL_Rect* collisionBounds = nullptr; // do not touch this until render time
	Vector2 startSpriteSize = Vector2(0,0); // initialize to starting sprite rectangle

	float colliderWidth = 1;
	float colliderHeight = 1;

	PhysicsEntity(Vector2 pos);
	~PhysicsEntity();

	Vector2 GetCenter();

	void CreateCollider(float startX, float startY, float x, float y, float w, float h);
	const SDL_Rect* GetColliderBounds();

	void SetVelocity(Vector2 newVelocity);

	void Pause(Uint32 ticks) override;
	void Unpause(Uint32 ticks) override;
};

