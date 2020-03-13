// Programmer: Anton Strickland (Goldbar Games)

#include "Game.h"
#include "Test.h"

static unsigned int allocationCount = 0;

void* operator new(size_t size)
{
	allocationCount++;
	//std::cout << allocationCount << " Allocating " << size << " bytes\n";
	return malloc(size);
}

int main(int argc, char *args[])
{
	Game game;

	// Load settings
	game.LoadSettings();

	game.LoadTitleScreen();

	game.SortEntities(game.entities);

	//game.CreateObjects();

	game.timer.Start();

	float updateInterval = 500; // update fps every X ms
	float fpsSum = 0.0f; // 
	float timeLeft = updateInterval; // time left before updating
	int frames = 0; // number of frames counted

	bool quit = false;

	int drawCallsLastFrame = 0;

	while (!quit)
	{
		game.renderer->drawCallsPerFrame = 0;

		game.CalcDt();

		//std::cout << "---" << std::endl;
		allocationCount = 0;

		//game.showFPS = true;
		if (game.showFPS)
		{
			timeLeft -= game.dt;
			fpsSum += 1000 / game.dt;
			if (timeLeft <= 0)
			{
				game.fpsText->SetText("FPS: " + std::to_string((int)(fpsSum / frames)));
				//std::cout << game.fpsText->txt << std::endl;
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
		else if (game.resetLevel)
		{
			game.editor->InitLevelFromFile(game.currentLevel);
			game.resetLevel = false;
		}

		quit = game.CheckInputs();

		game.CheckDeleteEntities();

		if (GetModeEdit())
		{
			if (game.openedMenus.size() > 0)
			{
				game.GetMenuInput();
			}
			else if (!game.getKeyboardInput)
			{
				game.HandleEditMode();
			}
		}
		else if (game.openedMenus.size() > 0)
		{
			game.GetMenuInput();
		}
		else
		{
			game.Update();
		}

		game.Render();

		if (game.renderer->drawCallsPerFrame != drawCallsLastFrame)
		{
			drawCallsLastFrame = game.renderer->drawCallsPerFrame;
			//std::cout << "Draw calls: " << game.renderer->drawCallsPerFrame << std::endl;
		}	

	}

	return 0;
}