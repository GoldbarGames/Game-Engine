#pragma once

#include <SDL.h>
#include <string>
#include "globals.h"

class Renderer;
class Game;

class BaseButton
{
public:
	std::string name = ""; // function to execute when button is pressed

	bool isSelected = false;
	bool pressedAnyKey = false;
	BaseButton* buttonPressedUp = nullptr;
	BaseButton* buttonPressedDown = nullptr;
	BaseButton* buttonPressedLeft = nullptr;
	BaseButton* buttonPressedRight = nullptr;
	BaseButton();
	~BaseButton();
	virtual void Render(Renderer* renderer);
	virtual BaseButton* Update(Game& game, const Uint8* currentKeyStates);

	void SetButtonsUpDownLeftRight(BaseButton* up = nullptr, BaseButton* down = nullptr, BaseButton* left = nullptr, BaseButton* right = nullptr);
	virtual void SetOptionColors(Color color);
};

