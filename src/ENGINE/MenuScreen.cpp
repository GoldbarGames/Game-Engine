#include "MenuScreen.h"
#include "SettingsButton.h"
#include "Game.h"
#include "globals.h"
#include "Renderer.h"
#include "Editor.h"
#include "FileManager.h"

void MenuAnimKeyframe::Update(uint32_t currentTime)
{
	// Can't update an entity that doesn't exist
	if (entity == nullptr)
		return;

	if (setPosition)
	{
		glm::vec3 newPosition = entity->position;

		LerpVector3(newPosition, previousFrame->targetPosition, targetPosition, 
			currentTime, previousFrame->time, time);

		entity->SetPosition(newPosition);
	}

	if (setColor)
	{
		glm::vec4 newColorV4 = glm::vec4(entity->color.r, entity->color.g, entity->color.b, entity->color.a);

		LerpVector4(newColorV4, previousFrame->targetColor, targetColor,
			currentTime, previousFrame->time, time);

		Color newColor = { (uint8_t)newColorV4.r, (uint8_t)newColorV4.g, (uint8_t)newColorV4.b, (uint8_t)newColorV4.a };

		entity->SetColor(newColor);
	}

}


MenuScreen::MenuScreen(const std::string& n, Game& game)
{
	name = n;	
}

void MenuScreen::CreateMenu(const std::string& n, Game& game)
{

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

//TODO: Maybe rename this to a better name
void MenuScreen::AssignButtons(bool useLeftRight)
{
	for (unsigned int i = 0; i < buttons.size(); i++)
	{
		int prevIndex = i - 1;
		int nextIndex = i + 1;

		if (prevIndex < 0)
			prevIndex = buttons.size() - 1;

		if (nextIndex >= buttons.size())
			nextIndex = 0;

		if (useLeftRight)
			buttons[i]->SetButtonsUpDownLeftRight(buttons[prevIndex], buttons[nextIndex], 
				buttons[prevIndex], buttons[nextIndex]);
		else
			buttons[i]->SetButtonsUpDownLeftRight(buttons[prevIndex], buttons[nextIndex], 
				nullptr, nullptr);
	}
}


void MenuScreen::ResetMenu()
{
	for (int i = 0; i < buttons.size(); i++)
	{
		if (buttons[i] != nullptr)
			delete_it(buttons[i]);
	}
	buttons.clear();

	for (int i = 0; i < texts.size(); i++)
	{
		if (texts[i] != nullptr)
			delete_it(texts[i]);
	}
	texts.clear();

	for (int i = 0; i < images.size(); i++)
	{
		if (images[i] != nullptr)
			delete_it(images[i]);
	}
	images.clear();

	for (auto& keyframe : enterAnimation)
	{
		if (keyframe != nullptr)
			delete_it(keyframe);
	}
	enterAnimation.clear();

	for (auto& keyframe : exitAnimation)
	{
		if (keyframe != nullptr)
			delete_it(keyframe);
	}
	exitAnimation.clear();
}

MenuScreen::~MenuScreen()
{
	ResetMenu();
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

bool MenuScreen::Update(Game& game)
{
	const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
	BaseButton* lastButton = selectedButton;

	// Don't crash if there is no button in this menu
	if (selectedButton == nullptr)
		return false;

	selectedButton->isSelected = false;
	selectedButton = selectedButton->Update(game, currentKeyStates);
	selectedButton->isSelected = true;

	if (selectedButton != lastButton)
	{
		selectedButton->Highlight(game);
		lastButton->Unhighlight(game);
	}

	return (lastButton->pressedAnyKey);
}

bool MenuScreen::PressSelectedButton(Game& game)
{
	if (selectedButton == nullptr)
		return false;

	return false;
}


BaseButton* MenuScreen::GetButtonByName(const std::string& buttonName)
{
	for (int i = 0; i < buttons.size(); i++)
	{
		if (buttons[i]->name == buttonName)
			return buttons[i];
	}

	return nullptr;
}