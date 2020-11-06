#ifndef PHYSICSCOMPONENT_H
#define PHYSICSCOMPONENT_H
#pragma once

#include <vector>
#include "../ENGINE/Vector2.h"
#include <SDL.h>

class MyEntity;
class Game;

class PhysicsComponent
{
protected:
	MyEntity* our = nullptr;
	
	Vector2 acceleration = Vector2(0, 0);	
public:
	std::vector<MyEntity*> thisFrameCollisions;
	std::vector<MyEntity*> prevFrameCollisions;

	Vector2 velocity = Vector2(0, 0);
	Vector2 previousVelocity = Vector2(0, 0);
	float maxVerticalSpeed = 1.5f;
	float maxHorizontalSpeed = 0.5f;
	float horizontalSpeed = 0.5f;
	int jumpsRemaining = 2;
	bool wasGrounded = false;
	bool isGrounded = false;
	unsigned int mass = 1;
	bool respawnOnDeath = false;

	bool useGravity = true;
	bool canBePushed = false;
	bool standAboveGround = false;
	bool isPushed = false;
	bool applyFriction = true;
	bool canBePickedUp = false;
	bool isPickedUp = false;

	float currentJumpSpeed = 0;
	float jumpSpeed = -1.0f;
	float windResistance = 1.0f;

	MyEntity* prevParent = nullptr;
	MyEntity* parent = nullptr;

	bool hadPressedJump = false;
	bool pressingJumpButton = false;
	bool canJump = true;
	bool jumped = false;
	bool shouldStickToGround = false;

	bool shouldCheckCollisions = true;
	bool hadCollisionsThisFrame = false;
	bool horizontalCollision = false;
	bool verticalCollision = false;

	SDL_Rect newBoundsHorizontal;
	SDL_Rect newBoundsVertical;
	SDL_Rect floorBounds;

	PhysicsComponent(MyEntity* entity);
	~PhysicsComponent();

	void PreviousFrameCollisions(Game& game);

	void SetVelocity(const Vector2& newVelocity);
	float CalcTerminalVelocity();

	bool CheckCollisions(Game& game);
	bool CheckCollisionTrigger(MyEntity* collidedEntity, Game& game);

	void Update(Game& game);

	void Push(const Vector2& pushVelocity);

	float CalcCollisionVelocity(PhysicsComponent* their, bool x);
	bool IsEntityPushingOther(const MyEntity& their);

	MyEntity* CheckPrevParent();

	bool CheckCollisionHorizontal(MyEntity* their, Game& game);

	bool CheckCollisionCeiling(Game& game);

	bool CheckVerticalJumpThru(MyEntity* their, Game& game);

	bool MoveVerticallyWithParent(MyEntity* their, Game& game);

	void Jump(Game& game);

	void ApplyFriction(float friction);
};

#endif