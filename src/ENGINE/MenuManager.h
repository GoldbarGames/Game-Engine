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

	MenuManager();
	~MenuManager();

	virtual void TogglePause(Game& game, bool toggle) const;
	virtual void Init(Game& game) const;
	virtual int GetFontSize() const; // TODO: Remove this when we read fonts in from a file?
	virtual void Update(Game& game) const;
};

#endif