#include "MenuManager.h"
#include <vector>
#include <string>
#include "Game.h"
#include "MenuScreen.h"

MenuManager::MenuManager()
{

}

MenuManager::~MenuManager()
{

}

void MenuManager::TogglePause(Game& game, bool toggle) const
{
	game.isPaused = toggle;

	// TODO: Should the MenuManager be the one to have the list of open menus?
	// If so, it would break this code, because it wouldn't be const,
	// so it's kind of a lucky coincidence for now

	if (game.isPaused)
	{
		game.openedMenus.emplace_back(game.allMenus["Pause"]);
		uint32_t ticks = Globals::CurrentTicks;
		for (unsigned int i = 0; i < game.entities.size(); i++)
			game.entities[i]->Pause(ticks);
	}
	else
	{
		game.openedMenus.clear();
		for (unsigned int i = 0; i < game.entities.size(); i++)
			game.entities[i]->Unpause(Globals::CurrentTicks);
	}
}


void MenuManager::Init(Game& game) const
{
	std::vector<std::string> menuNames = ReadStringsFromFile("data/lists/menuScreens.list");
	for (int i = 0; i < menuNames.size(); i++)
	{
		game.allMenus[menuNames[i]] = new MenuScreen(menuNames[i], game);
	}
}

int MenuManager::GetFontSize() const
{
	return 24;
}

void MenuManager::Update(Game& game) const
{

}