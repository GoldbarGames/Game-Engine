#ifndef MENUBUTTON_H
#define MENUBUTTON_H
#pragma once

#include <SDL.h>
#include <string>
#include "Sprite.h"
#include "BaseButton.h"

class Game;

class MenuButton : public BaseButton
{
private:
	int thickness = 2;
	Vector2 position = Vector2(0, 0);	
public:	
	MenuButton(std::string txt, std::string filepath, std::string function, Vector2 pos, Game& game);
	~MenuButton();
	void Render(Renderer* renderer);
	BaseButton* Update(Game& game, const Uint8* currentKeyStates);
	void SetOptionColors(Color color);
};

#endif