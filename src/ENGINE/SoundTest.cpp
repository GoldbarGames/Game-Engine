#include "SoundTest.h"
#include "SoundManager.h"
#include "Game.h"

SoundTest::SoundTest(SoundManager& m)
{
	manager = &m;

	float buttonX = 200 * Camera::MULTIPLIER;
	float buttonY = 200 * Camera::MULTIPLIER; // (manager->game->screenHeight - buttonHeight) * Camera::MULTIPLIER

	const int buttonWidth = 50;
	const int buttonHeight = 50;
	const int buttonSpacing = 100;

	loadBGMButton = neww EditorButton("BGM", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	buttons.emplace_back(loadBGMButton);
	buttonX += buttonWidth + buttonSpacing;

	playButton = neww EditorButton("|>", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	buttons.emplace_back(playButton);
	buttonX += buttonWidth + buttonSpacing;

	stepForwardButton = neww EditorButton("+", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	buttons.emplace_back(stepForwardButton);
	buttonX += buttonWidth + buttonSpacing;

	stepBackButton = neww EditorButton("-", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	buttons.emplace_back(stepBackButton);
	buttonX += buttonWidth + buttonSpacing;

	setTimeButton = neww EditorButton("=", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	buttons.emplace_back(setTimeButton);
	buttonX += buttonWidth + buttonSpacing;
}

SoundTest::~SoundTest()
{
	for (int i = 0; i < buttons.size(); i++)
	{
		if (buttons[i] != nullptr)
			delete_it(buttons[i]);
	}
}

void SoundTest::Update(Game& game)
{
	int mouseX = 0;
	int mouseY = 0;

	// TODO: Maybe make a consistent way of getting mouse input for editor buttons?
	// Because right now we are essentially duplicating the Editor's code
	static uint32_t previousMouseState = 0;

	const uint32_t mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		// Because the buttons were multiplied, we must multiply the mouse coords also
		mouseX *= Camera::MULTIPLIER;
		mouseY *= Camera::MULTIPLIER;

		EditorButton* clickedButton = nullptr;

		if (!(previousMouseState & SDL_BUTTON(SDL_BUTTON_LEFT) ))
		{
			// Get the button that was clicked, if any
			for (unsigned int i = 0; i < buttons.size(); i++)
			{
				if (buttons[i]->IsPointInsideButton(mouseX, mouseY))
				{
					clickedButton = buttons[i];
					break;
				}
			}

			if (clickedButton != nullptr)
			{
				if (clickedButton == playButton)
				{
					isPaused = !isPaused;
					playButton->text->SetText(isPaused ? "|>" : "||");
					isPaused ? manager->PauseBGM() : manager->UnpauseBGM();

					if (isPaused)
					{
						playButton->text->SetText("|>");
						playButton->image->color = { 128, 128, 128, 255 };
						manager->PauseBGM();
						timer.Pause();
					}
					else
					{
						playButton->text->SetText("||");
						playButton->image->color = { 255, 255, 255, 255 };
						manager->UnpauseBGM();
						timer.Unpause();
					}


				}
				else if (clickedButton == loadBGMButton)
				{
					// TODO: Display a dialog box for loading the BGM

					manager->PlayBGM("assets/arc/bgm/the_butler_did_it.ogg");
					timer.Start(0);

				}
				else if (clickedButton == stepBackButton)
				{
					// Move back 5 seconds
					int length = 5;
					int t = timer.GetTicks() / 1000;
					double skip = t - length;
					if (Mix_SetMusicPosition(skip) == -1)
					{
						manager->game->logger.Log("ERROR: Could not step back in music player");
					}
					else
					{
						timer.startTicks -= length * 1000;
					}
				}
				else if (clickedButton == stepForwardButton)
				{
					// Move forward 5 seconds
					int length = 5;
					int t = timer.GetTicks() / 1000;
					double skip = t + length;
					if (Mix_SetMusicPosition(skip) == -1)
					{
						manager->game->logger.Log("ERROR: Could not step forward in music player: ");
					}
					else
					{
						timer.startTicks += length * 1000;
					}
				}
				else if (clickedButton == setTimeButton)
				{
					// Set the music to a specific time
					double setTime = 60.0;

					// in order to make the timer think that more time has elapsed,
					// we must subtract time from its starting point
					timer.startTicks - (int)(setTime * 1000);

					Mix_RewindMusic();
					if (Mix_SetMusicPosition(60.0) == -1)
					{
						manager->game->logger.Log("ERROR: Could not set time in music player: ");
					}
				}
			}

		}
	}

	previousMouseState = mouseState;


	// TODO: for next time

	// - Make sure that the timer resets when the song ends/loops
	// - Make sure that skipping does not interfere with pausing and unpausing the song
	// - Figure out why the text is not showing on the buttons
	// - Set points in the song that can be looped over and over
	// - Add a button to move from point to point


}

void SoundTest::Render(const Renderer& renderer)
{
	for (int i = 0; i < buttons.size(); i++)
	{
		if (buttons[i] != nullptr)
			buttons[i]->Render(renderer);
	}
}