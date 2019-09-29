#pragma once
#include "PhysicsEntity.h"
#include "Vector2.h"
#include "Timer.h"


class Door;
class Ladder;
class Renderer;
class Goal;

class Player : public PhysicsEntity
{
private:	
	
	Timer missileTimer;
	Timer doorTimer;		
public:
	Game* game = nullptr;
	Door* currentDoor = nullptr;
	Ladder* currentLadder = nullptr;
	NPC* currentNPC = nullptr;
	Goal* currentGoal = nullptr;


	Vector2 startPosition;
	Player(Vector2 pos);
	~Player();
	void Update(Game& game);
	void UpdatePhysics(Game& game);
	void UpdateNormally(Game& game);

	void ResetPosition();

	void GetMoveInput(const Uint8* input);
	void GetLadderInput(const Uint8* input);
	

	void CastSpellDebug(Game &game, const Uint8* input);
	void CheckJumpButton(Game& game);
};

