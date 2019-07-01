#pragma once
#include "Game.h"

class GameEngine
{
private:
	Game game;
public:
	GameEngine();
	~GameEngine();
	void Run(std::string gameName);
};

