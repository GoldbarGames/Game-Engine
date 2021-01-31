#include "MenuScreen.h"
#include "SettingsButton.h"
#include "Game.h"
#include "globals.h"
#include "Renderer.h"
#include "Editor.h"
#include "FileManager.h"

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
		if (selectedButton->image != nullptr)
			selectedButton->image->SetShader(game.renderer.shaders[ShaderName::Glow]);
		if (lastButton->image != nullptr)
			lastButton->image->SetShader(game.renderer.shaders[ShaderName::GUI]);
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