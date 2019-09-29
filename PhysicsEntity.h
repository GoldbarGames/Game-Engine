#pragma once
#include "Entity.h"

class PhysicsEntity : public Entity
{
protected:
	Vector2 velocity = Vector2(0, 0);
	Vector2 acceleration = Vector2(0, 0);
	const float maxHorizontalSpeed = 0.5f;
	float horizontalSpeed = 0.05f;
	std::vector<Entity*> thisFrameCollisions;
	std::vector<Entity*> prevFrameCollisions;
	int jumpsRemaining = 2;
public:

	bool usePhysics = true;

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

	void CheckCollisions(Game& game);
	void CalculateCollider(Vector2 cameraOffset);
	void CheckCollisionTrigger(Entity* collidedEntity, Game& game);

	void Update(Game& game);
	void Render(Renderer * renderer, Vector2 cameraOffset);
	Vector2 CalcScaledPivot();
};

