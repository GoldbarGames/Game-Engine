#include "BaseButton.h"

BaseButton::BaseButton()
{

}


BaseButton::~BaseButton()
{

}


void BaseButton::Render(Renderer* renderer, GLuint uniformModel)
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