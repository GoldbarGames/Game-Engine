// Programmer: Anton Strickland (Goldbar Games)

#include "ENGINE/leak_check.h"
#include "ENGINE/Game.h"
#include "ENGINE/Editor.h"
#include "WDK/MyFileManager.h"
#include "WDK/MyEntityFactory.h"
#include "WDK/MyGUI.h"
#include "WDK/MyEditorHelper.h"

int main(int argc, char *args[])
{
#ifdef _WIN32
	//#ifdef MY_ENABLE_LEAK_CHECK /DMYENABLE_LEAK_CHECK
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//#endif
#endif

	// TODO: Custom editor / components for each game
	// TODO: How to deal with loading the first level?
	// TODO: Move SDL Init outside of Game?

	{
		// For your own custom entity types
		EntityFactory* e = MyEntityFactory::Get();
		GUI* gui = neww MyGUI();
		EditorHelper* helper = neww MyEditorHelper();
		FileManager* f = neww MyFileManager();

		Game game("WDK", "Witch Doctor Kaneko", "icon.png", *e, *f, *gui);
		game.editor->helper = helper;
		game.editor->helper->editor = game.editor;
		game.MainLoop();

		if (gui != nullptr)
			delete_it(gui);

		if (helper != nullptr)
			delete_it(helper);

		if (f != nullptr)
			delete_it(f);
	}

	// Call these outside the scope of the game
	// because we can't be sure of the destructor call order
	// (deleting TTF_Fonts after calling TTF_Quit = crash)
	TTF_Quit();
	SDL_Quit();
	IMG_Quit();

#if _DEBUG
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}