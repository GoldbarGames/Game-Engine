#ifndef SPELL_H
#define SPELL_H
#pragma once

#include <string>

class Game;

class Spell
{
public:
	int activeSpell = 0;
	std::string names[1] = { "push" };
	bool isUnlocked = true;

	bool Cast(Game &game);
	bool CastPush(Game& game);

	Spell();
	~Spell();
};

#endif