#pragma once
#include "Spell.h"

class SpellPop : public Spell
{
public:
	SpellPop(std::string n);
	~SpellPop();
	void Cast(Game &game);
};

