#include "SoundTest.h"
#include "SoundManager.h"
#include "Game.h"

SoundTest::SoundTest(SoundManager& m) : dialog(&m.game->spriteManager), 
										timelineRectangle(m.game->renderer.shaders[ShaderName::SolidColor]), 
										timelineLocation(m.game->renderer.shaders[ShaderName::SolidColor])
{
	manager = &m;

	dialog.position = glm::vec3(m.game->screenWidth, m.game->screenHeight, 0);

	timelineRectangle.color = { 0, 0, 0, 255 };
	timelineRectangle.keepPositionRelativeToCamera = true;
	timelineRectangle.keepScaleRelativeToCamera = true;

	timelineLocation.color = { 255, 255, 255, 255 };
	timelineLocation.keepPositionRelativeToCamera = true;
	timelineLocation.keepScaleRelativeToCamera = true;


	float buttonX = 200 * Camera::MULTIPLIER;
	float buttonY = 300 * Camera::MULTIPLIER; // (manager->game->screenHeight - buttonHeight) * Camera::MULTIPLIER

	const int buttonWidth = 50;
	const int buttonHeight = 50;
	const int buttonSpacing = 100;

	folderDirButton = neww EditorButton("DIR", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	folderDirButton->text->SetPosition(buttonX, buttonY);
	buttons.emplace_back(folderDirButton);
	buttonX += buttonWidth + buttonSpacing;

	loadBGMButton = neww EditorButton("BGM", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	loadBGMButton->text->SetPosition(buttonX, buttonY);
	buttons.emplace_back(loadBGMButton);
	buttonX += buttonWidth + buttonSpacing;

	playButton = neww EditorButton("|>", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	playButton->text->SetPosition(buttonX, buttonY);
	buttons.emplace_back(playButton);
	buttonX += buttonWidth + buttonSpacing;

	stepForwardButton = neww EditorButton("+", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	stepForwardButton->text->SetPosition(buttonX, buttonY);
	buttons.emplace_back(stepForwardButton);
	buttonX += buttonWidth + buttonSpacing;

	stepBackButton = neww EditorButton("-", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	stepBackButton->text->SetPosition(buttonX, buttonY);
	buttons.emplace_back(stepBackButton);
	buttonX += buttonWidth + buttonSpacing;

	setTimeButton = neww EditorButton("=", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	setTimeButton->text->SetPosition(buttonX, buttonY);
	buttons.emplace_back(setTimeButton);
	buttonX += buttonWidth + buttonSpacing;

	timerText = Text(m.game->theFont, "No song playing", true, true);
	timerText.SetPosition(600, 100);

	songText = Text(m.game->theFont, "No file", true, true);
	songText.SetPosition(600, 200);

	// TODO: Refactor this a lot so that we don't need to write all this
	// every time we reate a Dialog object

	dialog.text = neww Text(m.game->theFont, "");
	dialog.input = neww Text(m.game->theFont, "");

	dialog.text->SetPosition(dialog.position.x, dialog.position.y + 20);
	dialog.input->SetPosition(dialog.position.x, dialog.position.y + 70);

	dialog.sprite->SetShader(manager->game->renderer.shaders[ShaderName::SolidColor]);
	dialog.sprite->color = { 255, 0, 0, 255 };
	dialog.sprite->keepPositionRelativeToCamera = true;
	dialog.sprite->keepScaleRelativeToCamera = true;
	dialog.scale = (manager->game->renderer.CalculateScale(*dialog.sprite,
		dialog.text->GetTextWidth(), dialog.text->GetTextHeight() * 4, dialog.text->scale));

	dialog.text->GetSprite()->keepPositionRelativeToCamera = true;
	dialog.input->GetSprite()->keepPositionRelativeToCamera = true;
	dialog.text->GetSprite()->keepScaleRelativeToCamera = true;
	dialog.input->GetSprite()->keepScaleRelativeToCamera = true;

	std::vector<std::string> variables = manager->game->ReadStringsFromFile("data/soundtest.vars");
	
	if (variables.size() > 0)
		currentDir = variables[0];

	if (variables.size() > 1)
		currentBGM = variables[1];

	if (currentDir != "" || currentBGM != "")
		songText.SetText(currentDir + currentBGM);
}

SoundTest::~SoundTest()
{
	for (int i = 0; i < buttons.size(); i++)
	{
		if (buttons[i] != nullptr)
			delete_it(buttons[i]);
	}
}

void SoundTest::CreateDialog(const std::string& txt)
{
	dialog.text->SetText(txt);

	dialog.input->SetText("");
	dialog.visible = true;
	dialog.scale = (manager->game->renderer.CalculateScale(*dialog.sprite,
		dialog.text->GetTextWidth(), dialog.text->GetTextHeight() * 4, dialog.text->scale));
}

void SoundTest::UpdateTimerText()
{
	std::string timerStr = "";

	uint32_t timeElapsed = songTimer;

	uint32_t seconds = (timeElapsed / 1000) % 60;
	uint32_t minutes = (timeElapsed / 1000) / 60;

	float ms = timeElapsed / 1000.0f; // 123.456
	ms -= (int)ms; // 123.456 - 123 = 0.456

	int finalMS = ms * 1000; // 456

	// Minutes, Seconds, fractions of a second
	timerStr += std::to_string(minutes) + ":"
		+ std::to_string(seconds) + ":"
		+ std::to_string(finalMS);

	timerText.SetText(timerStr);
}

void SoundTest::AfterJumpDialog(const std::string& time)
{

	try
	{
		// Set the music to a specific time
		double setTime = std::stof(time);

		Mix_RewindMusic();
		if (Mix_SetMusicPosition(setTime) == -1)
		{
			manager->game->logger.Log("ERROR: Could not set time in music player: ");
		}
		else
		{
			songTimer = setTime * 1000;
			UpdateTimerText();
		}
	}
	catch (std::exception ex)
	{
		manager->game->logger.Log("ERROR: Could not set time in music player: ");
		manager->game->logger.Log(ex.what());
	}

}

void SoundTest::AfterDirDialog(const std::string& dir)
{
	currentDir = dir;
	songText.SetText(currentDir + currentBGM);

	std::ofstream fout;
	fout.open("data/soundtest.vars");
	if (fout.is_open())
	{
		fout << currentDir << std::endl;
		fout << currentBGM << std::endl;
	}
	fout.close();
}

void SoundTest::AfterFileDialog(const std::string& bgm)
{
	currentBGM = bgm;
	manager->PlayBGM(currentDir + currentBGM, 0);

	songText.SetText(currentDir + currentBGM);

	songTimer = 0.0f;

	isPaused = false;
	playButton->text->SetText("||");
	playButton->image->color = { 255, 255, 255, 255 };

	std::ofstream fout;
	fout.open("data/soundtest.vars");
	if (fout.is_open())
	{
		fout << currentDir << std::endl;
		fout << currentBGM << std::endl;
	}
	fout.close();
}

void SoundTest::Update(Game& game)
{

	dialog.Update(game.inputText);

	// If song is playing, update the timer text
	if (!isPaused && Mix_PlayingMusic())
	{
		songTimer += game.dt;
		UpdateTimerText();
	}



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
				if (clickedButton == folderDirButton)
				{
					CreateDialog("Type the name of the folder to look for BGMs:");
					game.StartTextInput(dialog, "sound_test_dir");
				}
				else if (clickedButton == loadBGMButton)
				{
					CreateDialog("Type the name of the BGM file to play:");
					game.StartTextInput(dialog, "sound_test_file");
				}
				else if (clickedButton == playButton)
				{
					isPaused = !isPaused;

					if (isPaused)
					{
						playButton->text->SetText("|>");
						playButton->image->color = { 128, 128, 128, 255 };
						manager->PauseBGM();
					}
					else if (Mix_PlayingMusic())
					{
						playButton->text->SetText("||");
						playButton->image->color = { 255, 255, 255, 255 };
						manager->UnpauseBGM();
					}
					else if (currentDir != "" && currentBGM != "")
					{
						manager->PlayBGM(currentDir + currentBGM, 0);

						songText.SetText(currentDir + currentBGM);

						songTimer = 0.0f;

						isPaused = false;
						playButton->text->SetText("||");
						playButton->image->color = { 255, 255, 255, 255 };
					}


				}
				else if (clickedButton == stepForwardButton && Mix_PlayingMusic())
				{
					// Move forward 5 seconds
					int length = 2;
					double skip = (songTimer + (length * 1000))/1000.0;

					if (Mix_SetMusicPosition(skip) == -1)
					{
						manager->game->logger.Log("ERROR: Could not step forward in music player: ");
					}
					else
					{
						songTimer += (length * 1000);
						UpdateTimerText();

						std::cout << "Stepped forward " << length << " seconds successfully!" << std::endl;
					}
				}
				else if (clickedButton == stepBackButton && Mix_PlayingMusic())
				{

					// Move back 5 seconds
					int length = 2;
					double skip = (songTimer - (length * 1000))/1000.0;

					if (Mix_SetMusicPosition(skip) == -1)
					{
						manager->game->logger.Log("ERROR: Could not step back in music player");
					}
					else
					{
						songTimer -= (length * 1000);
						UpdateTimerText();

						std::cout << "Stepped back " << length << " seconds successfully!" << std::endl;
					}
				}
				else if (clickedButton == setTimeButton && Mix_PlayingMusic())
				{
					CreateDialog("Type the timestamp (in seconds) to jump to:");
					game.StartTextInput(dialog, "sound_test_jump");					
				}
			}

		}
	}

	previousMouseState = mouseState;

	// TODO: for next time

	// - Add a timeline that shows where we are in the song

	// total length of the song
	
	// - Set points in the song that can be looped over and over
	// - Add a button to move from point to point

}

void SoundTest::Render(const Renderer& renderer)
{
	timerText.Render(renderer);
	songText.Render(renderer);


	int a = 600;
	int w = 400;
	int b = a + (w * Camera::MULTIPLIER);

	static float tlPosition = 0.0f;

	tlPosition = songTimer / 1000.0f; // - Get current time in the song (30)

	tlPosition /= 85; // - Divide by the total length of the song   (30/60 = 0.5f)

	tlPosition *= (b - a);		// - Multiply this number by (B-A)   (400-100 = 300 * 0.5 = 150)

	tlPosition += a;	// - Add this number back to A (150 + 100 = 250)

	tlPosition -= w; // To offset due to OpenGL drawing with center origin, we subtract half the width

	timelineRectangle.Render(glm::vec3(a, 400, 0), renderer, Vector2(w, 20));
	timelineLocation.Render(glm::vec3(tlPosition, 400, 0), renderer, Vector2(20, 20));


	for (int i = 0; i < buttons.size(); i++)
	{
		if (buttons[i] != nullptr)
		{
			buttons[i]->Render(renderer);
		}
	}

	dialog.Render(renderer);
}

void SoundTest::MusicFinished()
{

}