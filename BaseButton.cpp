#include "BaseButton.h"

BaseButton::BaseButton()
{

}


BaseButton::~BaseButton()
{

}


void BaseButton::Render(Renderer* renderer)
{

}

BaseButton* BaseButton::Update(const Uint8* currentKeyStates)
{
	return this;
}

void BaseButton::SetButtonsUpDownLeftRight(BaseButton* up, BaseButton* down, BaseButton* left, BaseButton* right)
{
	buttonPressedUp = up;
	buttonPressedDown = down;
	buttonPressedLeft = left;
	buttonPressedRight = right;
}