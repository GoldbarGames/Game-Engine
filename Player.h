#pragma once
#include "PhysicsEntity.h"
#include "Vector2.h"
#include "Timer.h"


class Door;
class Ladder;
class Renderer;

class Player : public PhysicsEntity
{
private:	
	Vector2 pivotDistance = Vector2(0, 0);
	Vector2 previousPivot = Vector2(0, 0);
	Vector2 pivotDifference = Vector2(0, 0);
	const float maxHorizontalSpeed = 0.5f;
	Timer missileTimer;
	Timer doorTimer;	
public:
	Door* currentDoor = nullptr;
	Ladder* currentLadder = nullptr;
	Vector2 startPosition;
	Player(Vector2 pos);
	~Player();
	void Update(Game& game);
	void UpdatePhysics(Game& game);
	void Render(Renderer * renderer, Vector2 cameraOffset);

	void ResetPosition();

	void GetMoveInput(const Uint8* input);
	void GetLadderInput(const Uint8* input);
	
	Vector2 CalcScaledPivot();
	void CastSpellDebug(Game &game, const Uint8* input);
	void CheckJumpButton(Game& game);
};

