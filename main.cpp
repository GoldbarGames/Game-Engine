// Programmer: Anton Strickland (Goldbar Games)

#include "Game.h"
#include "Test.h"
#include <deque>

int main(int argc, char *args[])
{
	Game game;

	game.LoadSettings();

	game.LoadTitleScreen();

	// game.SortEntities(entities);
	//Start counting frames per second
	int countedFrames = 0;
	game.timer.Start();

	float previousFPS = 0;

	std::deque<float> framesPerSeconds;

	const int FRAMES_SIZE = 100;
	for (int i = 0; i < FRAMES_SIZE; i++)
	{
		framesPerSeconds.push_back(0);
	}

	bool quit = false;
	while (!quit)
	{
		game.CalcDt();		

		if (game.showFPS)
		{
			//game.fpsLimit.Start();

			float fps = (1.0f / game.dt) * 1000;

			framesPerSeconds.push_back(fps);

			if (framesPerSeconds.size() > FRAMES_SIZE)
				framesPerSeconds.pop_front();			

			float sum = 0;
			for (int i = 0; i < framesPerSeconds.size(); i++)
				sum += framesPerSeconds[i];
			float averageFPS = sum / framesPerSeconds.size();

			if (averageFPS != previousFPS)
			{
				game.fpsText->SetText("FPS: " + std::to_string(averageFPS));
				previousFPS = averageFPS;
			}
		}

		if (game.showTimer)
		{
			game.timerText->SetText(std::to_string(game.timer.GetTicks() / 1000.0f));
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

		countedFrames++;
	}

	return 0;
}