#ifndef PLAYER_H
#define PLAYER_H
#pragma once

#include "Entity.h"
#include "Vector2.h"
#include "Timer.h"
#include "Spell.h"

class Door;
class Ladder;
class Renderer;
class Goal;
class NPC;

class Player : public Entity
{
private:	
	
	Timer timerSpellDebug;
	Timer doorTimer;
	Timer timerSpellOther;
	bool pressingUp = false;
	bool pressingDown = false;
	bool pressingLeft = false;
	bool pressingRight = false;
public:
	Game* game = nullptr;
	Door* currentDoor = nullptr;
	Ladder* currentLadder = nullptr;
	NPC* currentNPC = nullptr;
	Goal* currentGoal = nullptr;

	Collider* closeRangeAttackCollider = nullptr;

	bool castingSpell;
	Spell spell;

	bool updatedAnimator = false;

	Player(Vector2 pos);
	~Player();
	void Update(Game& game);
	void UpdateNormally(Game& game);
	void UpdateAnimator();
	void UpdateSpellAnimation(const char* spellName);

	void RenderDebug(Renderer* renderer);
	void Render(Renderer* renderer);

	void ResetPosition();

	void GetProperties(Renderer * renderer, FontInfo* font, std::vector<Property*>& properties);

	void SetProperty(std::string prop, std::string newValue);

	void GetMoveInput(const Uint8* input);
	void GetLadderInput(const Uint8* input);

	void OnClickPressed(Uint32 mouseState, Game& game);
	
	void CastSpellDebug(Game &game, const Uint8* input);
	void CheckJumpButton(const Uint8* input);

	void Save(std::ostringstream& level);

	static Entity* __stdcall Create(const Vector2& pos) { return new Player(pos); };
};

#endif
