#ifndef COMPONENT_H
#define COMPONENT_H
#pragma once

class Game;

class Component
{
public:
	virtual void Update(Game& game) = 0;
};

#endif