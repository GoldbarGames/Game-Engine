#ifndef BASEBUTTON_H
#define BASEBUTTON_H
#pragma once
#include "leak_check.h"
#include <SDL2/SDL.h>
#include <string>
#include "globals.h"

#include "GL/glew.h"

#include "Text.h"

class Renderer;
class Game;

class KINJO_API BaseButton
{
public:
	Vector2 scale = Vector2(1, 1);
	Sprite* image = nullptr;
	std::string name = ""; // function to execute when button is pressed
	Text* text = nullptr;
	bool isSelected = false;
	bool pressedAnyKey = false;
	BaseButton* buttonPressedUp = nullptr;
	BaseButton* buttonPressedDown = nullptr;
	BaseButton* buttonPressedLeft = nullptr;
	BaseButton* buttonPressedRight = nullptr;
	BaseButton();
	virtual ~BaseButton();
	virtual void Render(const Renderer& renderer);
	virtual BaseButton* Update(Game& game, const Uint8* currentKeyStates);

	void SetButtonsUpDownLeftRight(BaseButton* up = nullptr, BaseButton* down = nullptr, BaseButton* left = nullptr, BaseButton* right = nullptr);
	virtual void SetOptionColors(Color color);
};


#endif