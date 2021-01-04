// Programmer: Anton Strickland (Goldbar Games)

#include "ENGINE/leak_check.h"
#include "ENGINE/Game.h"
#include "ENGINE/Editor.h"
#include "ENGINE/FileManager.h"
#include "ENGINE/EntityFactory.h"
#include "ENGINE/GUI.h"
#include "ENGINE/EditorHelper.h"
#include "ENGINE/MenuManager.h"

int main(int argc, char *args[])
{
#ifdef _WIN32
	//#ifdef MY_ENABLE_LEAK_CHECK /DMYENABLE_LEAK_CHECK
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//#endif
#endif

	{
		// For your own custom entity types
		EntityFactory* e = EntityFactory::Get();
		GUI* gui = neww GUI();
		EditorHelper* helper = neww EditorHelper();
		FileManager* f = neww FileManager();
		MenuManager* m = neww MenuManager();

		Game game("WDK", "Witch Doctor Kaneko", "icon.png", true, *e, *f, *gui, *m);
		game.editor->helper = helper;
		game.editor->helper->editor = game.editor;
		game.MainLoop();

		if (gui != nullptr)
			delete_it(gui);

		if (helper != nullptr)
			delete_it(helper);

		if (f != nullptr)
			delete_it(f);

		if (m != nullptr)
			delete_it(m);
	}

#if _DEBUG
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}