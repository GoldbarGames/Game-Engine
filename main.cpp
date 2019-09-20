// Programmer: Anton Strickland (Goldbar Games)

#include "Game.h"
#include "Test.h"

int main(int argc, char *args[])
{
	Game game;

	game.LoadSettings();

	game.LoadTitleScreen();

	// game.SortEntities(entities);
	//Start counting frames per second
	int countedFrames = 0;
	game.timer.Start();

	bool quit = false;
	while (!quit)
	{
		game.fpsLimit.Start();

		//Calculate and correct fps
		float avgFPS = countedFrames / (game.timer.GetTicks() / 1000.f);
		if (avgFPS > 2000000)
		{
			avgFPS = 0;
		}

		//fpsText->SetText("FPS: " + std::to_string(avgFPS));
		quit = game.CheckInputs();
		game.CalcDt();
		//timerText->SetText(std::to_string(timer.GetTicks()/1000.0f));
		game.CheckDeleteEntities();

		if (GetModeEdit())
		{
			if (!game.getKeyboardInput)
			{
				game.HandleEditMode();
			}
		}
		else if (game.openedMenus.size() > 0)
		{
			Uint32 ticks = game.timer.GetTicks();
			if (ticks > game.lastPressedKeyTicks + 100) //TODO: Check for overflow errors
			{
				// If we have pressed any key on the menu, add a delay between presses
				if (game.openedMenus[game.openedMenus.size() - 1]->Update(game))
					game.lastPressedKeyTicks = ticks;
			}
		}
		else
		{
			game.Update();
		}

		game.Render();

		countedFrames++;
	}

	return 0;
}