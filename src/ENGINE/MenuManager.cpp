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