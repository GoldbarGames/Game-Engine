#include "MenuScreen.h"

MenuScreen::MenuScreen(std::string n, Game& game)
{
	name = n;

	if (name == "Pause")
	{
		int startWidth = 200;
		int startHeight = 200;
		int distance = 200;

		buttons.emplace_back(new MenuButton("Resume Game", "assets/gui/menu.png",
			Vector2(startWidth, startHeight + (distance*0)), game));
		
		buttons.emplace_back(new MenuButton("Spellbook", "assets/gui/menu.png",
			Vector2(startWidth, startHeight + (distance * 1)), game));
		
		buttons.emplace_back(new MenuButton("Settings", "assets/gui/menu.png",
			Vector2(startWidth, startHeight + (distance * 2)), game));
		
		buttons.emplace_back(new MenuButton("Exit Game", "assets/gui/menu.png",
			Vector2(startWidth, startHeight + (distance * 3)), game));
	}
	else if (name == "Title")
	{

	}
	else if (name == "Settings")
	{
		buttons.emplace_back(new MenuButton("Back", "assets/gui/menu.png", Vector2(0, 0), game));
	}
	else if (name == "Spellbook")
	{
		buttons.emplace_back(new MenuButton("Back", "assets/gui/menu.png", Vector2(0, 0), game));
	}
	else
	{
		
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