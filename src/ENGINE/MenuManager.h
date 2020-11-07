#ifndef MENUMANAGER_H
#define MENUMANAGER_H
#pragma once

#include "leak_check.h"

class Game;

#ifdef MAKEDLL
#  define KINJO_API __declspec(dllexport)
#endif

class KINJO_API MenuManager
{
public:
	MenuManager();
	~MenuManager();
	virtual void Init(Game& game) const;
};

#endif