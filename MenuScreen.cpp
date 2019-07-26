#include "MenuScreen.h"
#include "Game.h"
#include "globals.h"

MenuScreen::MenuScreen(std::string n, Game& game)
{
	name = n;

	if (name == "Pause")
	{
		int startWidth = screenWidth / 2;
		int startHeight = 100;
		int distance = 120;

		MenuButton* buttonResume = new MenuButton("Resume", "assets/gui/menu.png",
			"Resume Game", Vector2(startWidth, startHeight + (distance * 0)), game);

		MenuButton* buttonSpellbook = new MenuButton("Spellbook", "assets/gui/menu.png",
			"Spellbook", Vector2(startWidth, startHeight + (distance * 1)), game);

		MenuButton* buttonSettings = new MenuButton("Settings", "assets/gui/menu.png",
			"Settings", Vector2(startWidth, startHeight + (distance * 2)), game);

		MenuButton* buttonExit = new MenuButton("Exit", "assets/gui/menu.png",
			"Exit Game", Vector2(startWidth, startHeight + (distance * 3)), game);

		buttonResume->SetButtonsUpDownLeftRight(buttonExit, buttonSpellbook, buttonExit, buttonSpellbook);
		buttonSpellbook->SetButtonsUpDownLeftRight(buttonResume, buttonSettings, buttonResume, buttonSettings);
		buttonSettings->SetButtonsUpDownLeftRight(buttonSpellbook, buttonExit, buttonSpellbook, buttonExit);
		buttonExit->SetButtonsUpDownLeftRight(buttonSettings, buttonResume, buttonSettings, buttonResume);

		buttons.emplace_back(buttonResume);		
		buttons.emplace_back(buttonSpellbook);		
		buttons.emplace_back(buttonSettings);		
		buttons.emplace_back(buttonExit);
	}
	else if (name == "Title")
	{

	}
	else if (name == "Settings")
	{
		buttons.emplace_back(new MenuButton("Back", "assets/gui/menu.png", "Back", Vector2(0, 0), game));
	}
	else if (name == "Spellbook")
	{
		buttons.emplace_back(new MenuButton("Back", "assets/gui/menu.png", "Back", Vector2(0, 0), game));
	}
	else
	{
		
	}

	// Automatically select the first button in the list
	// TODO: Maybe we want to select a different one?
	if (buttons.size() > 0)
	{
		selectedButton = buttons[0];
		selectedButton->isSelected = true;
	}		
}


MenuScreen::~MenuScreen()
{
}

void MenuScreen::Render(SDL_Renderer* renderer)
{
	for (int i = 0; i < buttons.size(); i++)
	{
		buttons[i]->Render(renderer);
	}
}

int MenuScreen::Update()
{
	const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

	if (currentKeyStates[SDL_SCANCODE_UP] || currentKeyStates[SDL_SCANCODE_W])
	{
		if (selectedButton->buttonPressedUp != nullptr)
		{
			selectedButton->isSelected = false;
			selectedButton = selectedButton->buttonPressedUp;
			selectedButton->isSelected = true;
			return 1;
		}		
	}
	else if (currentKeyStates[SDL_SCANCODE_DOWN] || currentKeyStates[SDL_SCANCODE_S])
	{
		if (selectedButton->buttonPressedDown != nullptr)
		{
			selectedButton->isSelected = false;
			selectedButton = selectedButton->buttonPressedDown;
			selectedButton->isSelected = true;
			return 1;
		}		
	}
	else if (currentKeyStates[SDL_SCANCODE_LEFT] || currentKeyStates[SDL_SCANCODE_A])
	{
		if (selectedButton->buttonPressedLeft != nullptr)
		{
			selectedButton->isSelected = false;
			selectedButton = selectedButton->buttonPressedLeft;
			selectedButton->isSelected = true;
			return 1;
		}		
	}
	else if (currentKeyStates[SDL_SCANCODE_RIGHT] || currentKeyStates[SDL_SCANCODE_D])
	{
		if (selectedButton->buttonPressedRight != nullptr)
		{
			selectedButton->isSelected = false;
			selectedButton = selectedButton->buttonPressedRight;
			selectedButton->isSelected = true;
			return 1;
		}		
	}

	return 0;
}

bool MenuScreen::PressSelectedButton(Game& game)
{
	if (selectedButton == nullptr)
		return false;

	if (selectedButton->functionName == "Resume Game")
	{
		game.openedMenus.pop_back();
	}
	else if (selectedButton->functionName == "Exit Game")
	{
		return true;
	}
	else if (selectedButton->functionName == "Settings")
	{

	}
	else if (selectedButton->functionName == "Spellbook")
	{

	}

	return false;
}