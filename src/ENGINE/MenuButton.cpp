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
	name = function;

	text = new Text(game.theFont);
	text->SetText(txt);
	text->alignX = AlignmentX::CENTER;
	text->isRichText = false;
	text->GetSprite()->keepPositionRelativeToCamera = true;
	text->GetSprite()->keepScaleRelativeToCamera = true;

	if (filepath[0] == '@') // "@assets/gui/menu.png"
	{
		// Split filepath into 3 parts
		int extIndex = 0;
		for (int i = filepath.size(); i > 0; i--)
		{
			if (filepath[i] == '.')
			{				
				break;
			}
			extIndex++;
		}

		const std::string ext = filepath.substr(filepath.size() - extIndex, extIndex);

		const std::string file1 = filepath.substr(1, filepath.size() - extIndex - 1) + "1" + ext;
		const std::string file2 = filepath.substr(1, filepath.size() - extIndex - 1) + "2" + ext;
		const std::string file3 = filepath.substr(1, filepath.size() - extIndex - 1) + "3" + ext;

		images.emplace_back(new Sprite(1, game.spriteManager, file1, Renderer::GetTextShader(), glm::vec2(0, 0)));
		images.emplace_back(new Sprite(1, game.spriteManager, file2, Renderer::GetTextShader(), glm::vec2(0, 0)));
		images.emplace_back(new Sprite(1, game.spriteManager, file3, Renderer::GetTextShader(), glm::vec2(0, 0)));

	}
	else
	{
		images.emplace_back(new Sprite(1, game.spriteManager, filepath, Renderer::GetTextShader(), glm::vec2(0, 0)));
	}


	SetColor(col);


	//text->SetPosition(pos.x, pos.y + (image->GetRect()->h / 2) - (text->GetTextHeight()/2));
	AlignTextCenterY();
	text->SetScale(glm::vec2(Camera::MULTIPLIER, Camera::MULTIPLIER));

	// If this button has any text, scale the image to fit all the text inside it
	if (text->txt != "")
	{
		if (images.size() == 1)
		{
			scale = (game.renderer.CalculateScale(*images[0], text->GetTextWidth(), text->GetTextHeight(), text->scale));
		}
		else
		{
			// Calculate loop number based on text width
			loopImages = ((text->GetTextWidth() * text->GetScale().x) / images[1]->frameWidth) + 1;
		}
	}

	for (size_t i = 0; i < images.size(); i++)
	{
		if (images[i] != nullptr)
		{
			images[i]->keepPositionRelativeToCamera = true;
			images[i]->keepScaleRelativeToCamera = true;
		}
	}

}


MenuButton::~MenuButton()
{
	for (size_t i = 0; i < images.size(); i++)
	{
		if (images[i] != nullptr)
			delete_it(images[i]);
	}

	if (text != nullptr)
		delete_it(text);

	if (deleteBehindImages)
	{
		for (auto& s : behindImages)
		{
			if (s != nullptr)
				delete_it(s);
		}
	}

	if (deleteFrontImages)
	{
		for (auto& s : frontImages)
		{
			if (s != nullptr)
				delete_it(s);
		}
	}
}

void MenuButton::CalculateCollider()
{
	// Use a custom image for the collider instead of the one associated with the button
	if (colBoundsSprite != nullptr)
	{
		collider.scale.x = colBoundsSprite->GetSprite()->frameWidth * colBoundsSprite->scale.x;
		collider.scale.y = colBoundsSprite->GetSprite()->frameHeight * colBoundsSprite->scale.y;
		collider.CalculateCollider(position, rotation);
	}
	else if (images.size() > 0) // use the associated image and loop it
	{
		int w = images[0]->frameWidth * scale.x;
		int h = images[0]->frameHeight * scale.y;

		for (int i = 1; i < loopImages; i++)
		{
			w += images[0]->frameWidth * scale.x;
		}

		collider.scale.x = w * 1.75f; // TODO: Why does this magic number work?
		collider.scale.y = h;

		collider.CalculateCollider(position, rotation);
	}
	else
	{
		collider.CalculateCollider(position, rotation);
	}
}

void MenuButton::Render(const Renderer& renderer)
{	
	for (auto& s : behindImages)
	{
		s->Render(renderer);
	}

	//text->position = position + glm::vec3(w/3.0f, 0, 0);
	text->Render(renderer);

	if (images.size() == 1)
	{
		imagePosition = position;

		if (text != nullptr && text->isRichText)
		{
			switch (text->alignX)
			{
			default:
			case AlignmentX::LEFT:
				imagePosition.x += images[0]->frameWidth * scale.x;
				break;
			case AlignmentX::CENTER:
				imagePosition = position;
				break;
			case AlignmentX::RIGHT:
				imagePosition -= images[0]->frameWidth * scale.x;
				break;
			}
		}

		images[0]->Render(imagePosition, renderer, scale);
	}
	else
	{
		float w = images[0]->frameWidth * scale.x * Camera::MULTIPLIER;

		imagePosition = position - glm::vec3( (w * (loopImages + 1) / 2.0f), 0, 0);

		// Render the starting image
		images[0]->Render(imagePosition, renderer, scale);

		// Render the loops
		for (int i = 0; i < loopImages; i++)
		{
			images[1]->Render(imagePosition + glm::vec3(w, 0, 0), renderer, scale);
			w += images[1]->frameWidth * scale.x * Camera::MULTIPLIER;
		}

		// Render the ending image
		images[2]->Render(imagePosition + glm::vec3(w, 0, 0), renderer, scale);
		w += images[2]->frameWidth * scale.x * Camera::MULTIPLIER;


	}

	for (auto& s : frontImages)
	{
		s->Render(renderer);
	}

	//text->position = position + glm::vec3(w/3.0f, 0, 0);
	text->Render(renderer);


	//renderer.RenderDebugRect(hoverRect, glm::vec2(1, 1), { 255, 0, 0, 255 });
}

void MenuButton::Highlight(Game& game)
{
	for (size_t i = 0; i < images.size(); i++)
	{
		if (images[i] != nullptr)
			images[i]->isHovered = true;
	}

	if (text != nullptr)
		text->GetSprite()->isHovered = true;

	for (auto& s : behindImages)
	{
		s->GetSprite()->isHovered = true;
	}

	for (auto& s : frontImages)
	{
		s->GetSprite()->isHovered = true;
	}
}

void MenuButton::Unhighlight(Game& game)
{
	for (size_t i = 0; i < images.size(); i++)
	{
		if (images[i] != nullptr)
			images[i]->isHovered = false;
	}

	if (text != nullptr)
		text->GetSprite()->isHovered = false;

	for (auto& s : behindImages)
	{
		s->GetSprite()->isHovered = true;
	}

	for (auto& s : frontImages)
	{
		s->GetSprite()->isHovered = true;
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

void MenuButton::SetScale(const glm::vec2& newScale)
{
	text->SetScale(newScale);
	Entity::SetScale(newScale);
}