#ifndef PLAYER_H
#define PLAYER_H
#pragma once

#include "../ENGINE/Entity.h"
#include "../ENGINE/Vector2.h"
#include "../ENGINE/Timer.h"
#include "MyEntity.h"
#include "Spell.h"

class Door;
class Ladder;
class Renderer;
class NPC;
class Checkpoint;
class Decoration;
class MyGUI;


class Player : public MyEntity
{
private:	
	
	Timer timerSpellDebug;
	Timer doorTimer;
	bool pressingUp = false;
	bool pressingDown = false;
	bool pressingLeft = false;
	bool pressingRight = false;
	bool pressingRun = false;
public:
	Timer timerSpellOther;
	Game* game = nullptr;
	Door* currentDoor = nullptr;
	Ladder* currentLadder = nullptr;
	NPC* currentNPC = nullptr;
	Decoration* currentDecoration = nullptr;
	Checkpoint* currentCheckpoint = nullptr;

	Collider* closeRangeAttackCollider = nullptr;

	Spell spell;
	MyGUI* gui = nullptr;

	bool updatedAnimator = false;
	bool isDouble = false;

	bool canMove = true;

	Player(const Vector2& pos);
	~Player();
	void Update(Game& game);
	void UpdateNormally(Game& game);
	void UpdateAnimator();
	void UpdateSpellAnimation(const char* spellName);

	void RenderDebug(const Renderer& renderer);
	void Render(const Renderer& renderer);

	void ResetPosition();

	void GetProperties(std::vector<Property*>& properties);

	void SetProperty(const std::string& key, const std::string& newValue);

	void GetMoveInput(const Uint8* input);
	void GetLadderInput(const Uint8* input);

	void OnClickPressed(Uint32 mouseState, Game& game);
	
	void CastSpellDebug(Game &game, const Uint8* input);
	void CheckJumpButton(const Uint8* input);

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	static Entity* __stdcall Create(const Vector2& pos) { return neww Player(pos); };
};

#endif
