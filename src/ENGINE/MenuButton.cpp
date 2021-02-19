#include "MenuButton.h"
#include "Game.h"
#include "Renderer.h"

// TODO: Should we instead make the Color parameter a member of the Game,
// so that we don't need to pass in a color everywhere and can just reference it
// and only change it when necessary for specific buttons?
MenuButton::MenuButton(const std::string& txt, const std::string& filepath, 
	const std::string& function, const glm::vec3& pos, Game& game, Color col)
{
	position = pos;

	image = new Sprite(1, game.spriteManager, filepath, game.renderer.shaders[ShaderName::GUI], Vector2(0,0));
	SetColor(col);

	text = new Text(game.theFont);

	text->alignX = AlignmentX::CENTER;
	text->isRichText = false;
	text->SetText(txt);

	//text->SetPosition(pos.x, pos.y + (image->GetRect()->h / 2) - (text->GetTextHeight()/2));
	AlignTextCenterY();
	text->SetScale(Vector2(Camera::MULTIPLIER, Camera::MULTIPLIER));	

	// If this button has any text, scale the image to fit all the text inside it
	if (text->txt != "")
	{
		scale = (game.renderer.CalculateScale(*image, text->GetTextWidth(), text->GetTextHeight(), text->scale));
	}
	
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

	for (auto& s : otherImages)
	{
		if (s != nullptr)
			delete_it(s);
	}
}

void MenuButton::Render(const Renderer& renderer)
{	
	if (text != nullptr && text->isRichText)
	{		
		switch (text->alignX)
		{
		default:
		case AlignmentX::LEFT:
			imagePosition = glm::vec3(position.x + (image->frameWidth * scale.x), position.y, 0);
			break;
		case AlignmentX::CENTER:
			imagePosition = position;
			break;
		case AlignmentX::RIGHT:
			imagePosition = glm::vec3(position.x - (image->frameWidth * scale.x), position.y, 0);
			break;
		}
	}
	else
	{
		imagePosition = position;
	}	

	image->Render(imagePosition, renderer, scale);

	for (auto& s : otherImages)
	{
		s->Render(renderer);
	}

	text->Render(renderer);
}

void MenuButton::Highlight(Game& game)
{
	if (image != nullptr)
		image->SetShader(game.renderer.shaders[ShaderName::Glow]);

	if (text != nullptr)
		text->GetSprite()->SetShader(game.renderer.shaders[ShaderName::Glow]);

	for (auto& s : otherImages)
	{
		s->GetSprite()->SetShader(game.renderer.shaders[ShaderName::Glow]);
	}
}

void MenuButton::Unhighlight(Game& game)
{
	if (image != nullptr)
		image->SetShader(game.renderer.shaders[ShaderName::GUI]);

	if (text != nullptr)
		text->GetSprite()->SetShader(game.renderer.shaders[ShaderName::GUI]);

	for (auto& s : otherImages)
	{
		s->GetSprite()->SetShader(game.renderer.shaders[ShaderName::GUI]);
	}
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