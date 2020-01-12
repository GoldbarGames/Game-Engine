#pragma once
#include "Entity.h"

class PhysicsEntity : public Entity
{
protected:
	Vector2 startPosition = Vector2(0, 0);
	
	Vector2 acceleration = Vector2(0, 0);
	const float maxHorizontalSpeed = 0.5f;
	float horizontalSpeed = 0.5f;
	std::vector<Entity*> thisFrameCollisions;
	std::vector<Entity*> prevFrameCollisions;
	int jumpsRemaining = 2;
public:

	Vector2 velocity = Vector2(0, 0);

	unsigned int mass = 1;

	bool hitByPushSpell = false;
	float totalDistancePushed = 0;	

	bool useGravity = true;
	bool canBePushed = false;
	bool standAboveGround = false;

	const float JUMP_SPEED = -0.75f;

	PhysicsEntity* prevParent = nullptr;

	//TODO: Can we make this so that only the player has this?
	// OR does this mean that if other characters jump, we use this bool?
	bool hadPressedJump = false;
	bool pressingJumpButton = false;
	bool canJump = true;

	bool shouldStickToGround = false;

	PhysicsEntity* parent = nullptr;

	PhysicsEntity(Vector2 pos);
	~PhysicsEntity();

	Vector2 GetCenter();

	
	const SDL_Rect* GetColliderBounds();

	void PreviousFrameCollisions(Game& game);

	void SetVelocity(Vector2 newVelocity);

	void Pause(Uint32 ticks) override;
	void Unpause(Uint32 ticks) override;

	void CheckCollisions(Game& game);
	//void CalculateCollider(Vector2 cameraOffset);
	void CheckCollisionTrigger(Entity* collidedEntity, Game& game);

	void Update(Game& game);
	void Render(Renderer * renderer);
	Vector2 CalcScaledPivot();

	virtual void Push(Vector2 pushVelocity);

	float CalcCollisionVelocity(PhysicsEntity* other, bool x);
	bool IsEntityPushingOther(PhysicsEntity* other, bool x);

	PhysicsEntity* CheckPrevParent();

	bool CheckCollisionHorizontal(Entity* entity, Game& game);

	bool CheckCollisionVertical(Entity* entity, Game& game, SDL_Rect floorBounds);

	bool CheckCollisionCeiling(Entity* entity, Game& game);

	bool CheckVerticalJumpThru(Entity* entity, Game& game);

	bool MoveVerticallyWithParent(Entity* entity, Game& game);
};

