#ifndef GUI_H
#define GUI_H
#pragma once

#include <vector>

class Renderer;
class HealthComponent;
class Spell;

class GUI
{
public:
	Spell* playerSpell;
	std::vector<HealthComponent*> healthComponents;

	void Render(const Renderer& renderer);
};

#endif

