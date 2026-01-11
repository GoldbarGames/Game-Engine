#ifndef MENUMANAGER_H
#define MENUMANAGER_H
#pragma once

#include "leak_check.h"
#include <unordered_map>
#include <string>
#include <SDL2/SDL.h>

class Game;

class KINJO_API MenuManager
{
public:
	mutable bool shouldPopBackThisFrame = false;
	mutable std::unordered_map<std::string, SDL_Scancode> defaultKeys;
	mutable std::unordered_map<std::string, uint8_t> defaultButtons;

	int defaultFontSize = 24;

	MenuManager();
	~MenuManager();

	virtual void TogglePause(Game& game, bool toggle) const;
	virtual void Init(Game& game) const;
	virtual void Update(Game& game) const;

	virtual void CreateMenu(const std::string& menuName, Game& game) const = 0;
};

#endif