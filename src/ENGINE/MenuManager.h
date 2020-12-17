#ifndef MENUMANAGER_H
#define MENUMANAGER_H
#pragma once

#include "leak_check.h"

class Game;

class KINJO_API MenuManager
{
public:
	MenuManager();
	~MenuManager();
	virtual void Init(Game& game) const;
	virtual int GetFontSize() const; // TODO: Remove this when we read fonts in from a file?
};

#endif