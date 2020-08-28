#ifndef SPELL_H
#define SPELL_H
#pragma once

#include <SDL.h>
#include <string>

class Game;
class Renderer;
class Sprite;

class Spell
{
public:
	int activeSpell = 0;
	std::string names[1] = { "push" };
	bool isUnlocked = true;
	bool isCasting = false;

	Sprite* spellRangeSprite = nullptr;
	SDL_Rect spellRangeRect;

	bool Cast(Game &game);
	bool CastPush(Game& game);

	void Render(const Renderer& renderer);

	Spell();
	~Spell();
};

#endif