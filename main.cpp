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

	{
		Game game;
		game.MainLoop();
	}

	_CrtDumpMemoryLeaks();

	return 0;
}