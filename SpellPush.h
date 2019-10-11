#pragma once
#include "Spell.h"

class SpellPush : public Spell
{
public:
	SpellPush(std::string n);
	~SpellPush();
	void Cast(Game &game);
};

