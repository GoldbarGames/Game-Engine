#pragma once

#include <string>

class Game;

class Spell
{
public:
	std::string name;
	bool isUnlocked;

	virtual void Cast(Game &game);

	Spell(std::string n);
	~Spell();
};

