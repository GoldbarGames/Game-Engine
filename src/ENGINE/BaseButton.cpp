#include "BaseButton.h"

BaseButton::BaseButton() : Entity(glm::vec3(0,0,0))
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

void BaseButton::SetColor(Color c)
{
	color = c;

	if (image != nullptr)
	{
		image->color = c;
	}

	// We must be careful here to not set the text color
	// to the same as the background color, so change alpha only
	if (text != nullptr)
	{
		Color txtCol = { text->color.r, text->color.g, text->color.b, c.a };
		text->SetColor(txtCol);
	}
}

void BaseButton::SetOptionColors(Color c)
{

}

glm::vec3 BaseButton::AlignTextCenterY()
{
	text->SetPosition(position.x, position.y);
	return text->position;
}

glm::vec3 BaseButton::AlignTextTopY()
{
	const float textCenter = (text->GetTextHeight());
	const float imageCenter = (image->frameHeight * scale.y) * 0.75f;
	text->SetPosition(position.x, position.y + textCenter - imageCenter);
	return text->position;
}

glm::vec3 BaseButton::AlignTextBottomY()
{
	const float textCenter = (text->GetTextHeight());
	const float imageCenter = (image->frameHeight * scale.y) * 0.75f;
	text->SetPosition(position.x, position.y - textCenter + imageCenter);
	return text->position;
}