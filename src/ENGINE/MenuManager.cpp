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

void MenuManager::Init(Game& game) const
{
	std::vector<std::string> menuNames = game.ReadStringsFromFile("data/lists/menuScreens.list");
	for (int i = 0; i < menuNames.size(); i++)
	{
		game.allMenus[menuNames[i]] = neww MenuScreen(menuNames[i], game);
	}
}