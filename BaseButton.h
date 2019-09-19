#pragma once

#include <SDL.h>
#include <string>
class Renderer;

class BaseButton
{
public:
	std::string name = ""; // function to execute when button is pressed

	bool isSelected = false;
	BaseButton* buttonPressedUp = nullptr;
	BaseButton* buttonPressedDown = nullptr;
	BaseButton* buttonPressedLeft = nullptr;
	BaseButton* buttonPressedRight = nullptr;
	BaseButton();
	~BaseButton();
	virtual void Render(Renderer* renderer);
	virtual BaseButton* Update(const Uint8* currentKeyStates);

	void SetButtonsUpDownLeftRight(BaseButton * up = nullptr, BaseButton* down = nullptr, BaseButton* left = nullptr, BaseButton* right = nullptr);

};

