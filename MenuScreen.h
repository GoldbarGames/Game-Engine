#pragma once

#include <vector>
#include "MenuButton.h"
#include "SpriteManager.h"


class Entity;

class MenuScreen
{
private:
	std::string name = "";
public:	
	MenuButton* selectedButton = nullptr;
	std::vector<MenuButton*> buttons;
	std::vector<Text*> texts;
	std::vector<Entity*> images;
	int selectedButtonIndex = 0;
	MenuScreen(std::string n, Game& game);
	~MenuScreen();
	void Render(Renderer* renderer);
	int Update();
	bool PressSelectedButton(Game& game);
};

