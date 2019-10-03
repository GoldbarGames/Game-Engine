#pragma once

#include <vector>
#include "MenuButton.h"
#include "SpriteManager.h"


class Entity;

class MenuScreen
{	
public:	
	std::string name = "";
	BaseButton* selectedButton = nullptr;
	std::vector<BaseButton*> buttons;
	std::vector<Text*> texts;
	std::vector<Entity*> images;
	int selectedButtonIndex = 0;
	MenuScreen(std::string n, Game& game);
	~MenuScreen();
	void Render(Renderer* renderer);
	bool Update(Game& game);
	bool PressSelectedButton(Game& game);
	BaseButton* GetButtonByName(std::string buttonName);
};

