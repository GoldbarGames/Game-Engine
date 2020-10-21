#ifndef GUI_H
#define GUI_H
#pragma once

#include <vector>
#include <unordered_map>
#include "Text.h"

class Renderer;
class HealthComponent;
class Game;

class GUI
{
public:
	Game* game;
	std::unordered_map<std::string, Text*> texts;
	std::vector<std::string> textNames;

	virtual void Init(Game* g);
	virtual void RenderStart();
	virtual void Render(const Renderer& renderer);
	virtual void ResetText();

	~GUI();
};

#endif

