#include "BaseButton.h"

BaseButton::BaseButton() : Entity(glm::vec3(0,0,0))
{

}


BaseButton::~BaseButton()
{
	for (size_t i = 0; i < images.size(); i++)
	{
		if (images[i] != nullptr)
			delete_it(images[i]);
	}

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

	for (unsigned int i = 0; i < images.size(); i++)
	{
		if (images[i] != nullptr)
		{
			images[i]->color = c;
		}
	}

	// We must be careful here to not set the text color
	// to the same as the background color, so change alpha only
	if (text != nullptr)
	{
		Color txtCol = { text->color.r, text->color.g, text->color.b, c.a };
		text->SetColor(txtCol);
	}
}

Color BaseButton::GetImageColor(const unsigned int index)
{
	return (images.size() < index) ? images[index]->color : Color{ 255, 255, 255, 255 };
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
	const float imageCenter = (images[0]->frameHeight * scale.y) * 0.75f;
	text->SetPosition(position.x, position.y + textCenter - imageCenter);
	return text->position;
}

glm::vec3 BaseButton::AlignTextBottomY()
{
	const float textCenter = (text->GetTextHeight());
	const float imageCenter = (images[0]->frameHeight * scale.y) * 0.75f;
	text->SetPosition(position.x, position.y - textCenter + imageCenter);
	return text->position;
}