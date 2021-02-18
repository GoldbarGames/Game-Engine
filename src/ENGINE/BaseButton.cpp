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

void BaseButton::Highlight(Game& game)
{

}

void BaseButton::Unhighlight(Game& game)
{

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

void BaseButton::AlignTextCenterY()
{
	text->SetPosition(position.x, position.y);
}

void BaseButton::AlignTextTopY()
{
	const float textCenter = (text->GetTextHeight());
	const float imageCenter = (image->frameHeight * scale.y) * 0.75f;
	text->SetPosition(position.x, position.y + textCenter - imageCenter);
}

void BaseButton::AlignTextBottomY()
{
	const float textCenter = (text->GetTextHeight());
	const float imageCenter = (image->frameHeight * scale.y) * 0.75f;
	text->SetPosition(position.x, position.y - textCenter + imageCenter);
}