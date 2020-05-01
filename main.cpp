// Programmer: Anton Strickland (Goldbar Games)

#include "Game.h"
#include "Test.h"

static unsigned int allocationCount = 0;

void* operator new(size_t size)
{
	allocationCount++;
	//if (allocationCount < 1000)
	//	std::cout << allocationCount << " Allocating " << size << " bytes\n";
	return malloc(size);
}

void operator delete(void* p)
{
	free(p);
}

int main(int argc, char *args[])
{
	Game game;

	// Load settings
	game.LoadSettings();

	game.LoadTitleScreen();

	game.SortEntities(game.entities);

	game.timer.Start();

	const int updateInterval = 500; // update fps every X ms
	float fpsSum = 0.0f; // 
	float timeLeft = updateInterval; // time left before updating
	int frames = 0; // number of frames counted

	bool quit = false;

	int drawCallsLastFrame = 0;
	int previousNumberOfFrames = 0;

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
				int currentNumberOfFrames = (int)(fpsSum / frames);
				if (currentNumberOfFrames != previousNumberOfFrames)
				{
					game.fpsText->SetText("FPS: " + std::to_string(currentNumberOfFrames));
					previousNumberOfFrames = currentNumberOfFrames;
				}

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

		switch (game.state)
		{
		case GameState::RESET_LEVEL:
			game.editor->InitLevelFromFile(game.currentLevel);
			game.state = GameState::NORMAL;
			break;
		case GameState::LOAD_NEXT_LEVEL:
			game.LoadNextLevel();
			game.state = GameState::NORMAL;
			break;
		default:
			break;
		}

		quit = game.CheckInputs();
		game.CheckDeleteEntities();

		if (game.openedMenus.size() > 0)
		{
			game.GetMenuInput();
		}
		else if (GetModeEdit())
		{
			game.HandleEditMode();
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