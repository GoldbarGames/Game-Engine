#pragma once

#include <SDL.h>
#include <string>
#include "Sprite.h"
#include "Text.h"

class Game;

class MenuButton
{
private:
	int thickness = 2;
	SDL_Rect outlineHorizontal;
	SDL_Rect outlineVertical;
	SDL_Rect outlineCorners;
	Sprite* image = nullptr;
	Text* text;
	Vector2 position = Vector2(0, 0);	
public:
	std::string functionName = ""; // function to execute when button is pressed
	bool isSelected = false;
	MenuButton(std::string txt, std::string filepath, std::string function, Vector2 pos, Game& game);
	~MenuButton();
	void Render(SDL_Renderer* renderer);
	MenuButton* buttonPressedUp = nullptr;
	MenuButton* buttonPressedDown = nullptr;
	MenuButton* buttonPressedLeft = nullptr;
	MenuButton* buttonPressedRight = nullptr;
	void SetButtonsUpDownLeftRight(MenuButton* up, MenuButton* down, MenuButton* left, MenuButton* right);
};