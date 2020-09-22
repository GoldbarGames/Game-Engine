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

class Spell
{
public:
	int activeSpell = 0;
	std::vector<std::string> names;
	bool isUnlocked = true;
	bool isCasting = false;

	std::vector<Entity*> affectedEntities;

	Sprite* spellRangeSprite = nullptr;
	SDL_Rect spellRangeRect;

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

	Spell();
	~Spell();
};

#endif