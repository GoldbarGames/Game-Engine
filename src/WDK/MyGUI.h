#ifndef MYGUI_H
#define MYGUI_H
#pragma once
#include "../ENGINE/GUI.h"

class Spell;

class MyGUI : public GUI
{
public:
	Spell* playerSpell;

	void Init(Game* g);
	void Render(const Renderer& renderer);
	void ResetText();
};

#endif