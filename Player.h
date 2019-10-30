#pragma once
#include "PhysicsEntity.h"
#include "Vector2.h"
#include "Timer.h"
#include "Spell.h"

class Door;
class Ladder;
class Renderer;
class Goal;

class Player : public PhysicsEntity
{
private:	
	
	Timer missileTimer;
	Timer doorTimer;		
	Timer spellTimer;
public:
	Game* game = nullptr;
	Door* currentDoor = nullptr;
	Ladder* currentLadder = nullptr;
	NPC* currentNPC = nullptr;
	Goal* currentGoal = nullptr;

	
	bool canJump = true;
	Vector2 startPosition;

	//TODO: Make this more of a proper data structure
	std::vector<Spell*> spells;
	int spellIndex = 0;



	Player(Vector2 pos);
	~Player();
	void Update(Game& game);
	void UpdateNormally(Game& game);
	void UpdateAnimator();

	void Render(Renderer * renderer, Vector2 cameraOffset);

	void ResetPosition();

	void GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Property*>& properties);

	void SetProperty(std::string prop, std::string newValue);

	void GetMoveInput(const Uint8* input);
	void GetLadderInput(const Uint8* input);
	

	void CastSpellDebug(Game &game, const Uint8* input);
	void CheckJumpButton(const Uint8* input);

	void Save(std::ostringstream& level);
};

