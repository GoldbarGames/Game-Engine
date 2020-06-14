#include "SpellPush.h"
#include "Game.h"

SpellPush::SpellPush(std::string n) : Spell(n)
{
	//TODO: Actually implement unlocking of spells
	// and only casting the ones that are unlocked
	isUnlocked = true;
}

SpellPush::~SpellPush()
{

}

void SpellPush::Cast(Game& game)
{

}