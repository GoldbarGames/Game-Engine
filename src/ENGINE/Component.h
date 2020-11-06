#ifndef COMPONENT_H
#define COMPONENT_H
#pragma once
#include "leak_check.h"
class Game;

class DECLSPEC Component
{
public:
	virtual void Update(Game& game) = 0;
};

#endif