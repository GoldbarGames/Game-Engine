#ifndef SPELL_H
#define SPELL_H
#pragma once

#include <string>

class Game;

class Spell
{
public:
	int activeSpell = 0;
	std::string name = "";
	bool isUnlocked = true;

	bool Cast(Game &game);
	bool CastPush(Game& game);

	Spell();
	Spell(std::string n);
	~Spell();
};

#endif