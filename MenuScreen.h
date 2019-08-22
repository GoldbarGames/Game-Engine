#pragma once

#include <vector>
#include "MenuButton.h"
#include "SpriteManager.h"

class MenuScreen
{
private:
	std::string name = "";
public:	
	MenuButton* selectedButton = nullptr;
	std::vector<MenuButton*> buttons;
	int selectedButtonIndex = 0;
	MenuScreen(std::string n, Game& game);
	~MenuScreen();
	void Render(Renderer* renderer);
	int Update();
	bool PressSelectedButton(Game& game);
};

