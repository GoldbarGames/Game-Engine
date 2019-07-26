#pragma once

#include <vector>
#include "MenuButton.h"
#include "SpriteManager.h"

class MenuScreen
{
private:
	std::string name = "";
public:	
	std::vector<MenuButton*> buttons;
	int selectedButtonIndex = 0;
	MenuScreen(std::string n, Game& game);
	~MenuScreen();
	void Render(SDL_Renderer* renderer);
};

