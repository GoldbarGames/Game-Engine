#include "MenuButton.h"
#include "Game.h"
#include "Renderer.h"

MenuButton::MenuButton(const std::string& txt, const std::string& filepath, 
	const std::string& function, const Vector2& pos, Game& game)
{
	position = pos;

	image = neww Sprite(1, game.spriteManager, filepath, game.renderer.shaders[ShaderName::GUI], Vector2(0,0));
	image->color = { 200, 55, 161, 255 };

	text = neww Text(game.theFont);

	text->alignX = AlignmentX::CENTER;
	text->isRichText = false;
	text->SetText(txt);

	//text->SetPosition(pos.x, pos.y + (image->GetRect()->h / 2) - (text->GetTextHeight()/2));
	text->SetPosition(pos.x, pos.y - (text->GetTextHeight() / 4));
	text->SetScale(Vector2(Camera::MULTIPLIER, Camera::MULTIPLIER));	

	scale = (game.renderer.CalculateScale(*image, text->GetTextWidth(), text->GetTextHeight(), text->scale));
	
	name = function;

	// What if I want to scale the button to a particular width and height 
	// independent of the image? ANSWER: See the EditorButton

	image->keepPositionRelativeToCamera = true;
	image->keepScaleRelativeToCamera = true;
	
	text->GetSprite()->keepPositionRelativeToCamera = true;
	text->GetSprite()->keepScaleRelativeToCamera = true;
}


MenuButton::~MenuButton()
{
	if (image != nullptr)
		delete_it(image);

	if (text != nullptr)
		delete_it(text);
}

void MenuButton::Render(const Renderer& renderer)
{	
	if (text != nullptr && text->isRichText)
	{		
		switch (text->alignX)
		{
		default:
		case AlignmentX::LEFT:
			imagePosition = Vector2(position.x + (image->frameWidth * scale.x), position.y);
			break;
		case AlignmentX::CENTER:
			imagePosition = position;
			break;
		case AlignmentX::RIGHT:
			imagePosition = Vector2(position.x - (image->frameWidth * scale.x), position.y);
			break;
		}
	}
	else
	{
		imagePosition = position;
	}	

	image->Render(imagePosition, renderer, scale);
	text->Render(renderer);
}


BaseButton* MenuButton::Update(Game& game, const Uint8* currentKeyStates)
{
	pressedAnyKey = true;

	if (currentKeyStates[SDL_SCANCODE_UP] || currentKeyStates[SDL_SCANCODE_W])
	{
		if (buttonPressedUp != nullptr)
		{
			return buttonPressedUp;
		}
	}
	else if (currentKeyStates[SDL_SCANCODE_DOWN] || currentKeyStates[SDL_SCANCODE_S])
	{
		if (buttonPressedDown != nullptr)
		{
			return buttonPressedDown;
		}
	}
	else if (currentKeyStates[SDL_SCANCODE_LEFT] || currentKeyStates[SDL_SCANCODE_A])
	{
		if (buttonPressedLeft != nullptr)
		{
			return buttonPressedLeft;
		}
	}
	else if (currentKeyStates[SDL_SCANCODE_RIGHT] || currentKeyStates[SDL_SCANCODE_D])
	{
		if (buttonPressedRight != nullptr)
		{
			return buttonPressedRight;
		}
	}

	pressedAnyKey = false;

	return this;
}

void MenuButton::SetOptionColors(Color color)
{
	text->SetText(text->txt, color);
}