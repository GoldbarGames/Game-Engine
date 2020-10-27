#ifndef SPELL_H
#define SPELL_H
#pragma once


#include <SDL.h>
#include <string>
#include <vector>

class Entity;
class Game;
class Renderer;
class Sprite;
class Missile;
class Player;

#include "../ENGINE/Timer.h"

class Spell
{
private:
	Timer timer;
public:
	int activeSpell = 0;
	std::vector<std::string> names;
	bool isUnlocked = true;
	bool isCasting = false;

	int specialFrame = 9999;

	bool isShort = false;
	float SHRINK_SIZE = 0.1f;

	bool isShieldUp = false;
	bool isPlantedSeed = false;
	bool isGrowingSeed = false;

	std::vector<Entity*> beanstalkParts;

	std::vector<Sprite*> spellIcons;
	std::vector<Entity*> affectedEntities;

	int counter = 0;

	Player* player = nullptr;
	Player* playerClone = nullptr;

	Sprite* spellRangeSprite = nullptr;
	SDL_Rect spellRangeRect;

	Missile* carryMissile = nullptr;

	void CycleSpells(Game& game);

	bool Cast(Game &game);
	bool CastPush(Game& game);
	bool CastPop(Game& game);
	bool CastFloat(Game& game);
	bool CastFreeze(Game& game);
	bool CastFlash(Game& game);
	bool CastDouble(Game& game);
	bool CastShort(Game& game);
	bool CastProtect(Game& game);
	bool CastReturn(Game& game);
	bool CastSeed(Game& game);
	bool CastBreak(Game& game);
	bool CastSearch(Game& game);
	bool CastTurbo(Game& game);
	bool CastCrypt(Game& game);
	bool CastCarry(Game& game);
	bool CastRead(Game& game);
	bool CastTouch(Game& game);
	bool CastJump(Game& game);
	bool CastSleep(Game& game);

	void Update(Game& game);
	void Render(const Renderer& renderer);
	void RenderDebug(const Renderer& renderer);

	Spell();
	Spell(Player* p);
	~Spell();
};

#endif