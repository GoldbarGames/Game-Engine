#ifndef PHYSICSINFO_H
#define PHYSICSINFO_H
#pragma once

#include <vector>
#include "Vector2.h"

class Entity;
class Game;

class PhysicsInfo
{
protected:
	Entity* our = nullptr;
	
	Vector2 acceleration = Vector2(0, 0);

	std::vector<Entity*> thisFrameCollisions;
	std::vector<Entity*> prevFrameCollisions;
	
public:
	Vector2 startPosition = Vector2(0, 0);
	Vector2 velocity = Vector2(0, 0);
	const float maxHorizontalSpeed = 0.5f;
	float horizontalSpeed = 0.5f;
	int jumpsRemaining = 2;
	bool isGrounded = false;
	unsigned int mass = 1;

	bool hitByPushSpell = false;
	float totalDistancePushed = 0;	

	bool useGravity = true;
	bool canBePushed = false;
	bool standAboveGround = false;

	float jumpSpeed = -1.0f;

	Entity* prevParent = nullptr;
	Entity* parent = nullptr;

	//TODO: Can we make this so that only the player has this?
	// OR does this mean that if other characters jump, we use this bool?
	bool hadPressedJump = false;
	bool pressingJumpButton = false;
	bool canJump = true;

	bool shouldStickToGround = false;


	PhysicsInfo(Entity* entity);
	~PhysicsInfo();

	void PreviousFrameCollisions(Game& game);

	void SetVelocity(Vector2 newVelocity);
	float CalcTerminalVelocity();

	bool CheckCollisions(Game& game);
	bool CheckCollisionTrigger(Entity* collidedEntity, Game& game);

	void Update(Game& game);

	Vector2 CalcScaledPivot();

	void Push(Vector2 pushVelocity);

	float CalcCollisionVelocity(PhysicsInfo* their, bool x);
	bool IsEntityPushingOther(Entity* their, bool x);

	Entity* CheckPrevParent();

	bool CheckCollisionHorizontal(Entity* their, Game& game);

	bool CheckCollisionCeiling(Entity* their, Game& game);

	bool CheckVerticalJumpThru(Entity* their, Game& game);

	bool MoveVerticallyWithParent(Entity* their, Game& game);

	void Jump(Game& game);
};

#endif