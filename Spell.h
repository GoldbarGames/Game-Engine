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

	void Cast(Game &game);
	void CastPush(Game& game);

	Spell();
	Spell(std::string n);
	~Spell();
};

#endif