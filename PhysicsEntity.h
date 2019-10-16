#pragma once
#include "Entity.h"

class PhysicsEntity : public Entity
{
protected:
	Vector2 velocity = Vector2(0, 0);
	Vector2 acceleration = Vector2(0, 0);
	const float maxHorizontalSpeed = 0.5f;
	float horizontalSpeed = 0.5f;
	std::vector<Entity*> thisFrameCollisions;
	std::vector<Entity*> prevFrameCollisions;
	int jumpsRemaining = 2;
public:

	bool usePhysics = true;
	bool canBePushed = false;

	SDL_Rect* collider = nullptr;        // adjust the bounds this way
	Vector2 startSpriteSize = Vector2(1,1); // initialize to starting sprite rectangle

	float colliderWidth = 1;
	float colliderHeight = 1;

	//TODO: Can we make this so that only the player has this?
	// OR does this mean that if other characters jump, we use this bool?
	bool hadPressedJump;
	bool pressingJumpButton;

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

	virtual void Push(Vector2 pushVelocity);
};

