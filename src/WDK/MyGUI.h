#ifndef MYGUI_H
#define MYGUI_H
#pragma once
#include "../ENGINE/GUI.h"

class Spell;

class MyGUI : public GUI
{
public:
	std::vector<HealthComponent*> healthComponents;
	Spell* playerSpell;

	void Init(Game* g);
	void RenderStart();
	void Render(const Renderer& renderer);
	void ResetText();
};

#endif