#pragma once
#include "PhysicsEntity.h"
#include "Vector2.h"
#include "Timer.h"

class Door;

class Player : public PhysicsEntity
{
private:
	int jumpsRemaining = 2;
	Vector2 pivotDistance = Vector2(0, 0);
	Vector2 previousPivot = Vector2(0, 0);
	Vector2 pivotDifference = Vector2(0, 0);
	const float maxHorizontalSpeed = 0.5f;
	Timer missileTimer;
	Timer doorTimer;	
	std::vector<Entity*> thisFrameCollisions;
	std::vector<Entity*> prevFrameCollisions;
public:
	Door* currentDoor = nullptr;
	Vector2 startPosition;
	Player(Vector2 pos);
	~Player();
	void Update(Game& game);
	void UpdatePhysics(Game& game);
	void Render(SDL_Renderer * renderer, Vector2 cameraOffset);
	void CheckCollisions(Game& game);
	void ResetPosition();
	void CalculateCollider(Vector2 cameraOffset);
	void GetMoveInput(const Uint8* input);
	void CheckCollisionTrigger(Entity* collidedEntity);
	Vector2 CalcScaledPivot();
};

