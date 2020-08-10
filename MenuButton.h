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
	Vector2 imagePosition = Vector2(0, 0);
public:	
	MenuButton(const std::string& txt, const std::string& filepath, 
		const std::string& function, Vector2 pos, Game& game);
	~MenuButton();
	void Render(const Renderer& renderer);
	BaseButton* Update(Game& game, const Uint8* currentKeyStates);
	void SetOptionColors(Color color);
};

#endif