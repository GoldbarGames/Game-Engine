#ifndef GUI_H
#define GUI_H
#pragma once

#include <vector>
#include <unordered_map>
#include "Text.h"

class Renderer;
class HealthComponent;
class Spell;
class Game;

class GUI
{
public:
	Game* game;
	Spell* playerSpell;
	std::vector<HealthComponent*> healthComponents;
	std::unordered_map<std::string, Text*> texts;
	std::vector<std::string> textNames;

	void Init(Game* g);
	void Render(const Renderer& renderer);
	void ResetText();

	~GUI();
};

#endif

