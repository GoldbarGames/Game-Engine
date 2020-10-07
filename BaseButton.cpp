#include "BaseButton.h"

BaseButton::BaseButton()
{

}


BaseButton::~BaseButton()
{
	if (image != nullptr)
		delete_it(image);

	if (text != nullptr)
		delete_it(text);
}


void BaseButton::Render(const Renderer& renderer)
{

}

BaseButton* BaseButton::Update(Game& game, const Uint8* currentKeyStates)
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

void BaseButton::SetOptionColors(Color color)
{

}