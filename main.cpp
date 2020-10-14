// Programmer: Anton Strickland (Goldbar Games)

#include "leak_check.h"
#include "Game.h"

int main(int argc, char *args[])
{
#ifdef _WIN32
	//#ifdef MY_ENABLE_LEAK_CHECK /DMYENABLE_LEAK_CHECK
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//#endif
#endif

	// TODO: Move SDL Init outside of Game?

	{
		Game game("WDK");
		game.MainLoop();
	}

	// Call these outside the scope of the game
	// because we can't be sure of the destructor call order
	// (deleting TTF_Fonts after calling TTF_Quit = crash)
	TTF_Quit();
	SDL_Quit();
	IMG_Quit();

	//_CrtDumpMemoryLeaks();

	return 0;
}