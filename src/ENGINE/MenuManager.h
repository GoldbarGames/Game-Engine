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
	mutable std::unordered_map<std::string, SDL_Scancode> defaultKeys;

	MenuManager();
	~MenuManager();
	virtual void Init(Game& game) const;
	virtual int GetFontSize() const; // TODO: Remove this when we read fonts in from a file?
	virtual void Update(Game& game) const;
};

#endif