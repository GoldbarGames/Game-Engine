#ifndef MENUSCREEN_H
#define MENUSCREEN_H
#pragma once

#include <vector>
#include "MenuButton.h"
#include "SpriteManager.h"
#include "leak_check.h"

class Entity;

class KINJO_API MenuScreen
{	
public:	
	std::string name = "";
	BaseButton* selectedButton = nullptr;
	std::vector<BaseButton*> buttons;
	std::vector<Text*> texts;
	std::vector<Entity*> images;
	int selectedButtonIndex = 0;
	MenuScreen(const std::string& n, Game& game);
	~MenuScreen();
	
	void Render(const Renderer& renderer);
	bool Update(Game& game);
	
	virtual bool PressSelectedButton(Game& game);
	
	BaseButton* GetButtonByName(const std::string& buttonName);
	void AssignButtons(bool useLeftRight);
	bool FileExists(const std::string& filepath);
};

#endif