#include "MyFileManager.h"
#include "../ENGINE/Game.h"
#include "../ENGINE/CutsceneManager.h"
#include "../ENGINE/CutsceneCommands.h"
#include "MyEntity.h"
#include "../ENGINE/HealthComponent.h"

void MyFileManager::SaveFile(const std::string& filename) const
{
	try
	{
		game->cutsceneManager.commands.SetStringVariable({ "", "201", game->currentLevel });
		game->cutsceneManager.commands.SetNumberVariable({ "", "202", std::to_string(game->player->position.x) });
		game->cutsceneManager.commands.SetNumberVariable({ "", "203", std::to_string(game->player->position.y) });

		MyEntity* myEntity = dynamic_cast<MyEntity*>(game->player);
		if (myEntity)
		{
			game->cutsceneManager.commands.SetNumberVariable({ "", "204", std::to_string(myEntity->health->GetMaxHP()) });
			game->cutsceneManager.commands.SetNumberVariable({ "", "205", std::to_string(myEntity->health->GetCurrentHP()) });
		}

		game->cutsceneManager.SaveGame(filename.c_str());
	}
	catch (std::exception ex)
	{
		std::cout << "ERROR SAVING FILE: " << ex.what() << std::endl;
	}
}

void MyFileManager::LoadFile(const std::string& filename) const
{
	try
	{
		game->cutsceneManager.LoadGame(filename.c_str());
		game->nextLevel = game->cutsceneManager.commands.stringVariables[201];
		game->openedMenus.clear();
		game->LoadLevel(game->nextLevel); // , 1, 1);
		game->loadingFromSaveFile = true;
	}
	catch (std::exception ex)
	{
		std::cout << "ERROR LOADING FILE: " << ex.what() << std::endl;
	}
}
