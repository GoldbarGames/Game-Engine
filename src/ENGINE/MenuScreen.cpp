#include "MenuScreen.h"
#include "SettingsButton.h"
#include "Game.h"
#include "globals.h"
#include "Renderer.h"
#include "Editor.h"
#include "FileManager.h"

MenuAnimation::MenuAnimation(Entity* e)
{
	entity = e;

	MenuAnimKeyframe* initialFrame = new MenuAnimKeyframe();
	initialFrame->targetPosition = e->position;
	initialFrame->targetColor = glm::vec4(e->color.r, e->color.g, e->color.b, e->color.a);

	keyframes.emplace_back(initialFrame);
}

MenuAnimKeyframe::MenuAnimKeyframe()
{

}

MenuAnimKeyframe* MenuAnimation::CreateKeyframe(uint32_t duration)
{
	MenuAnimKeyframe* keyframe = new MenuAnimKeyframe(keyframes.back(), duration);
	keyframes.emplace_back(keyframe);
	return keyframe;
}

MenuAnimKeyframe::MenuAnimKeyframe(MenuAnimKeyframe* p, uint32_t d)
{
	previousFrame = p;
	duration = d;

	// Set properties to the same as the previous frame
	targetPosition = p->targetPosition;
	targetColor = p->targetColor;
}

void MenuAnimKeyframe::CalculateTime()
{
	if (previousFrame != nullptr)
		time = previousFrame->time + duration;
	else
		time = duration + Globals::CurrentTicks;
}

void MenuAnimKeyframe::Update(Entity* entity, uint32_t currentTime)
{
	// Can't update an entity that doesn't exist
	if (entity == nullptr)
		return;

	glm::vec3 newPosition = entity->position;

	LerpVector3(newPosition, previousFrame->targetPosition, targetPosition,
		currentTime, previousFrame->time, time);

	entity->SetPosition(newPosition);

	glm::vec4 newColorV4 = glm::vec4(entity->color.r, entity->color.g, entity->color.b, entity->color.a);

	LerpVector4(newColorV4, previousFrame->targetColor, targetColor,
		currentTime, previousFrame->time, time);

	Color newColor = { (uint8_t)newColorV4.r, (uint8_t)newColorV4.g, (uint8_t)newColorV4.b, (uint8_t)newColorV4.a };

	entity->SetColor(newColor);
}

MenuAnimation* MenuScreen::CreateEnterAnimation(Entity* entity)
{
	MenuAnimation* animation = new MenuAnimation(entity);
	enterAnimation.emplace_back(animation);
	return animation;
}

MenuAnimation* MenuScreen::CreateExitAnimation(Entity* entity)
{
	MenuAnimation* animation = new MenuAnimation(entity);
	exitAnimation.emplace_back(animation);
	return animation;
}

MenuScreen::MenuScreen(const std::string& n, Game& game)
{
	name = n;	
	_game = &game;
}

void MenuScreen::CreateMenu(const std::string& n, Game& game)
{

}

Entity* MenuScreen::AddImage(const std::string& filepath, const glm::vec3& pos, 
	const glm::vec2& scale, const Game& game, const int shader)
{
	Entity* image = new Entity(pos);
	image->GetSprite()->SetTexture(game.spriteManager.GetImage(filepath));

	if (shader > -1)
	{
		image->GetSprite()->SetShader(game.renderer.shaders[shader]);
	}

	image->SetScale(scale);
	image->GetSprite()->keepPositionRelativeToCamera = true;
	image->GetSprite()->keepScaleRelativeToCamera = true;
	images.emplace_back(image);
	return image;
}

MenuButton* MenuScreen::AddButton(const std::string& txt, const std::string& filepath,
	const int btnID, const glm::vec3& pos, Game& game, Color col)
{
	MenuButton* button = new MenuButton(txt, filepath, "", pos, game, col);
	button->btnID = btnID;
	buttons.emplace_back(button);
	return button;
}

MenuButton* MenuScreen::AddButtonOutlined(const std::string& txt, const std::string& filepath,
	const int btnID, const glm::vec3& pos, Game& game, Color col)
{
	MenuButton* button = AddButton(txt, filepath, btnID, pos, game, col);

	button->loopImages = 3;
	for (int k = 0; k < button->images.size(); k++)
	{
		button->images[k]->hoverShader = game.renderer.shaders[20];
	}
	button->text->SetShader(game.renderer.shaders[18]);
	button->text->GetSprite()->hoverShader = game.renderer.shaders[21];

	return button;
}

// IMPORTANT: When center is true, pass in game.screenWidth as x
Text* MenuScreen::AddText(FontInfo* font, const std::string& message,
	int x, int y, float sx, float sy, bool center)
{
	Text* text = new Text(font, message, true, true);

	if (center)
	{
		int cx = x - (text->GetTextWidth() / 2);
		text->SetPosition(cx, y);
	}
	else
	{
		text->SetPosition(x, y);
	}


	text->SetScale(glm::vec2(sx, sy));
	texts.emplace_back(text);
	
	return text;
}

Text* MenuScreen::AddTextOutlined(FontInfo* font, const std::string& message,
	int x, int y, float sx, float sy, bool center)
{
	Text* text = AddText(font, message, x, y, sx, sy, center);

	text->GetSprite()->shader = _game->renderer.shaders[18];

	return text;
}

bool MenuScreen::FileExists(const std::string& filepath)
{
	std::fstream fin;
	fin.open(filepath);
	if (!fin.good())
		return false;
	else
		fin.close();
	return true;
}

void MenuScreen::AssignButtons(bool useLeftRight, bool useUpDown)
{
	for (unsigned int i = 0; i < buttons.size(); i++)
	{
		int prevIndex = i - 1;
		int nextIndex = i + 1;

		if (prevIndex < 0)
			prevIndex = buttons.size() - 1;

		if (nextIndex >= buttons.size())
			nextIndex = 0;

		if (useLeftRight && useUpDown)
		{
			buttons[i]->SetButtonsUpDownLeftRight(buttons[prevIndex], buttons[nextIndex],
				buttons[prevIndex], buttons[nextIndex]);
		}
		else if (!useLeftRight && useUpDown)
		{
			buttons[i]->SetButtonsUpDownLeftRight(buttons[prevIndex], buttons[nextIndex],
				nullptr, nullptr);
		}
		else if (useLeftRight && !useUpDown)
		{
			buttons[i]->SetButtonsUpDownLeftRight(nullptr, nullptr,
				buttons[prevIndex], buttons[nextIndex]);
		}
		else
		{
			buttons[i]->SetButtonsUpDownLeftRight(nullptr, nullptr,
				nullptr, nullptr);
		}

	}
}


void MenuScreen::ResetMenu()
{
	for (size_t i = 0; i < buttons.size(); i++)
	{
		if (rememberLastButton && buttons[i] == selectedButton)
			lastButtonIndex = i;

		if (buttons[i] != nullptr)
			delete_it(buttons[i]);
	}
	buttons.clear();

	for (size_t i = 0; i < texts.size(); i++)
	{
		if (texts[i] != nullptr)
			delete_it(texts[i]);
	}
	texts.clear();

	for (size_t i = 0; i < images.size(); i++)
	{
		if (images[i] != nullptr)
			delete_it(images[i]);
	}
	images.clear();

	for (auto& anim : enterAnimation)
	{
		for (auto& keyframe : anim->keyframes)
		{
			if (keyframe != nullptr)
				delete_it(keyframe);
		}
		if (anim != nullptr)
			delete_it(anim);
	}
	enterAnimation.clear();

	for (auto& anim : exitAnimation)
	{
		for (auto& keyframe : anim->keyframes)
		{
			if (keyframe != nullptr)
				delete_it(keyframe);
		}
		if (anim != nullptr)
			delete_it(anim);
	}
	exitAnimation.clear();

	selectedButton = nullptr;
	lastButton = nullptr;
}

MenuScreen::~MenuScreen()
{
	ResetMenu();
}

void MenuScreen::HighlightSelectedButton(Game& game)
{
	if (selectedButton != nullptr)
		selectedButton->Highlight(game);
}

void MenuScreen::UnhighlightSelectedButton(Game& game)
{
	if (selectedButton != nullptr)
		selectedButton->Unhighlight(game);
}

void MenuScreen::Render(const Renderer& renderer)
{
	for (unsigned int i = 0; i < images.size(); i++)
	{
		images[i]->Render(renderer);
	}

	for (unsigned int i = 0; i < buttons.size(); i++)
	{
		buttons[i]->Render(renderer);
	}

	for (unsigned int i = 0; i < texts.size(); i++)
	{
		texts[i]->Render(renderer);
	}
}

void MenuScreen::ResetSelectedButton(Game& game)
{
	selectedButton = nullptr;
	for (size_t i = 0; i < buttons.size(); i++)
	{
		buttons[i]->isSelected = (i == defaultButtonIndex);

		if (buttons[i]->isSelected)
		{
			selectedButton = buttons[i];
			selectedButtonIndex = i;
			buttons[i]->Highlight(game);
		}
		else
		{
			buttons[i]->Unhighlight(game);
		}
	}
}

bool MenuScreen::Update(Game& game)
{
	// Don't crash if there is no button in this menu
	if (buttons.size() == 0)
		return false;

	lastButton = selectedButton;

	if (useMouse)
	{
		SDL_Rect mouseRect;

		previousMouseState = mouseState;
		mouseState = SDL_GetMouseState(&mouseRect.x, &mouseRect.y);

		glm::vec3 worldPosition = game.ConvertFromScreenSpaceToWorldSpace(glm::vec2(mouseRect.x, mouseRect.y)) - game.renderer.camera.position;

		mouseRect.x = worldPosition.x * Camera::MULTIPLIER;
		mouseRect.y = worldPosition.y * Camera::MULTIPLIER;
		mouseRect.w = 1;
		mouseRect.h = 1;

		bool selectedAnyButton = false;
		for (size_t i = 0; i < buttons.size(); i++)
		{
			MenuButton* mb = dynamic_cast<MenuButton*>(buttons[i]);

			if (mb != nullptr)
			{
				mb->CalculateCollider();

				SDL_Rect rect = *mb->GetBounds();

				mb->hoverRect = rect;

				// For some reason, only the rendering must be cut in half like so:
				mb->hoverRect.x *= 0.5f;
				mb->hoverRect.y *= 0.5f;
				mb->hoverRect.w *= 0.5f;
				mb->hoverRect.h *= 0.5f;

				rect.x -= rect.w;
				rect.y -= rect.h;
				rect.w *= Camera::MULTIPLIER;
				rect.h *= Camera::MULTIPLIER;

				// 640,  400, 96,  32
				// 1088, 736, 384, 128

				buttons[i]->isSelected = HasIntersection(mouseRect, rect);

				if (buttons[i]->isSelected)
				{
					selectedButton = buttons[i];
					selectedAnyButton = true;
				}
			}
			else // settings button
			{
				
			}

		}



		if (!selectedAnyButton)
		{
			selectedButton = nullptr;
			if (lastButton != nullptr)
			{
				lastButton->Unhighlight(game);
				lastButton = nullptr;
			}
		}

		if (selectedButton != nullptr)
		{
			SDL_Rect rect = *selectedButton->GetBounds();
			rect.x -= rect.w;
			rect.y -= rect.h;
			rect.w *= Camera::MULTIPLIER;
			rect.h *= Camera::MULTIPLIER;

			if (HasIntersection(mouseRect, rect))
			{

				bool holdingLeft = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT));
				bool holdingRight = (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT));
				bool holdingMiddle = (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE));

				bool pressedLeft = !(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT));
				bool pressedRight = !(previousMouseState & SDL_BUTTON(SDL_BUTTON_RIGHT));
				bool pressedMiddle = !(previousMouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE));

				if (holdingLeft || holdingRight || holdingMiddle)
				{
					if (pressedLeft || pressedMiddle || pressedRight)
					{
						//hovered->OnClickPressed(mouseState, *this);
					}
					else
					{
						//hovered->OnClick(mouseState, *this);
					}
				}
				else if (!pressedLeft)
				{
					PressSelectedButton(game);
				}
			}
		}		

	}
	else
	{
		const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
		selectedButton->isSelected = false;
		selectedButton = selectedButton->Update(game, currentKeyStates);
		selectedButton->isSelected = true;
	}

	if (selectedButton != lastButton)
	{
		selectedButton->Highlight(game);
		if (lastButton != nullptr)
		{
			lastButton->Unhighlight(game);
		}
	}

	// Don't crash if there is no button in this menu
	if (lastButton == nullptr)
		return false;

	return (lastButton->pressedAnyKey);	
}

// Game developer should override this method
bool MenuScreen::PressSelectedButton(Game& game)
{
	if (selectedButton == nullptr)
		return false;

	return false;
}

BaseButton* MenuScreen::GetButtonByName(const std::string& buttonName)
{
	for (size_t i = 0; i < buttons.size(); i++)
	{
		if (buttons[i]->name == buttonName)
			return buttons[i];
	}

	return nullptr;
}