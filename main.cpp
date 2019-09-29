// Programmer: Anton Strickland (Goldbar Games)

#include "Game.h"
#include "Test.h"

int main(int argc, char *args[])
{
	Game game;

	game.LoadSettings();

	game.LoadTitleScreen();

	// game.SortEntities(entities);

	game.timer.Start();

	float updateInterval = 500; // update fps every X ms
	float fpsSum = 0.0f; // 
	float timeLeft = updateInterval; // time left before updating
	int frames = 0; // number of frames counted

	bool quit = false;

	while (!quit)
	{
		game.CalcDt();		

		if (game.showFPS)
		{
			timeLeft -= game.dt;
			fpsSum += 1000 / game.dt;
			if (timeLeft <= 0)
			{
				game.fpsText->SetText("FPS: " + std::to_string((int)(fpsSum / frames)));
				timeLeft = updateInterval;
				fpsSum = 0;
				frames = 0;
			}
			frames++;
		}

		if (game.showTimer)
		{
			game.timerText->SetText(std::to_string(game.timer.GetTicks() / 1000.0f));
		}

		if (game.goToNextLevel)
		{
			game.LoadNextLevel();
			game.goToNextLevel = false;
		}

		quit = game.CheckInputs();

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

	}

	return 0;
}