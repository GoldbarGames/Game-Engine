#include "SoundTest.h"
#include "SoundManager.h"
#include "Game.h"
#include <sstream>

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


	float buttonX = 100 * Camera::MULTIPLIER;
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

	bgmUpButton = neww EditorButton("^", "Btn", glm::vec3(buttonX, buttonY + 150, 0), *manager->game);
	bgmUpButton->text->SetPosition(buttonX, buttonY + 150);
	buttons.emplace_back(bgmUpButton);

	bgmDownButton = neww EditorButton("v", "Btn", glm::vec3(buttonX, buttonY + 300, 0), *manager->game);
	bgmDownButton->text->SetPosition(buttonX, buttonY + 300);
	buttons.emplace_back(bgmDownButton);

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

	addLoopButton = neww EditorButton("L+", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	addLoopButton->text->SetPosition(buttonX, buttonY);
	buttons.emplace_back(addLoopButton);
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

	std::vector<std::string> variables = ReadStringsFromFile("data/soundtest.vars");
	
	// NOTE: This should go before the currentBGM is set because we end up using it here.
	if (variables.size() > 2)
	{		
		for (int v = 2; v < variables.size(); v++)
		{
			// We encounter a new loop here
			if (variables[v][0] == '*')
			{
				// Read until the end of the line
				currentBGM = "";
				for (int k = 2; k < variables[v].size(); k++)
				{
					currentBGM += variables[v][k];
				}
			}
			else if (variables[v].size() > 0) // otherwise, this is loop data
			{
				int index = 0;

				std::string n = ParseWord(variables[v], ' ', index);

				uint32_t st = std::stoi(ParseWord(variables[v], ' ', index));
				uint32_t et = std::stoi(ParseWord(variables[v], ' ', index));

				uint8_t r = std::stoi(ParseWord(variables[v], ' ', index));
				uint8_t g = std::stoi(ParseWord(variables[v], ' ', index));
				uint8_t b = std::stoi(ParseWord(variables[v], ' ', index));

				Color c = { r, g, b, 255 };

				CreateLoop(n, st, et, c);
			}
		}
	}

	if (variables.size() > 0)
		currentDir = variables[0];

	if (variables.size() > 1)
		currentBGM = variables[1];

	if (currentDir != "" || currentBGM != "")
		songText.SetText(currentDir + currentBGM);
}

SoundTest::~SoundTest()
{
	SaveData();

	for (int i = 0; i < buttons.size(); i++)
	{
		if (buttons[i] != nullptr)
			delete_it(buttons[i]);
	}

	for (const auto& [key, val] : soundLoops)
	{
		for (int i = 0; i < soundLoops[key].size(); i++)
		{
			if (soundLoops[key][i] != nullptr)
				delete_it(soundLoops[key][i]);
		}
	}
}

void SoundTest::ScrollCurrentBGM(bool up)
{

	std::vector<std::string> bgmList;

	fs::path path = fs::current_path().append(currentDir);
	for (const auto& entry : fs::directory_iterator(path))
	{
		bgmList.emplace_back(entry.path().filename().string());
	}

	int b = -1;
	for (int i = 0; i < bgmList.size(); i++)
	{
		if (bgmList[i] == currentBGM)
			b = i;
	}

	if (b > -1)
	{
		int newBGMIndex = up ? b - 1 : b + 1;

		if (newBGMIndex < 0)
			newBGMIndex = bgmList.size() - 1;

		if (newBGMIndex >= bgmList.size())
			newBGMIndex = 0;

		currentBGM = bgmList[newBGMIndex];

		AfterFileDialog(currentBGM);
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

std::string SoundTest::ConvertTimeToStringFromNumber(uint32_t time)
{
	std::string timeStr = "";

	uint32_t seconds = (time / 1000) % 60;
	uint32_t minutes = (time / 1000) / 60;

	float ms = time / 1000.0f; // 123.456
	ms -= (int)ms; // 123.456 - 123 = 0.456

	int finalMS = ms * 1000; // 456

	// Minutes, Seconds, fractions of a second
	timeStr += std::to_string(minutes) + ":"
		+ std::to_string(seconds) + ":"
		+ std::to_string(finalMS);

	return timeStr;
}

void SoundTest::UpdateTimerText()
{
	timerText.SetText(ConvertTimeToStringFromNumber(songTimer));
}


void SoundTest::AfterLoopDialog1(const std::string& time)
{

	try
	{
		if (time.find('.') != std::string::npos)
			soundLoops[currentBGM][currentlyModifyingLoop]->startTime = (int)(std::stof(time) * 1000);
		else
			soundLoops[currentBGM][currentlyModifyingLoop]->startTime = std::stoi(time);

		std::string loopString = std::to_string(currentlyModifyingLoop + 1) + ": ";
		loopString += ConvertTimeToStringFromNumber(soundLoops[currentBGM][currentlyModifyingLoop]->startTime) + " - "
			+ ConvertTimeToStringFromNumber(soundLoops[currentBGM][currentlyModifyingLoop]->endTime);
		soundLoops[currentBGM][currentlyModifyingLoop]->text->SetText(loopString, soundLoops[currentBGM][currentlyModifyingLoop]->color);

		CreateDialog("Type the end time for the loop:");
		manager->game->StartTextInput(dialog, "sound_test_loop_time2");
	}
	catch (std::exception ex)
	{
		manager->game->logger.Log("ERROR: Could not set loop time start in music player: ");
		manager->game->logger.Log(ex.what());
	}

}

void SoundTest::AfterLoopDialog2(const std::string& time)
{
	try
	{
		if (time.find('.') != std::string::npos)
			soundLoops[currentBGM][currentlyModifyingLoop]->endTime = (int)(std::stof(time) * 1000);
		else
			soundLoops[currentBGM][currentlyModifyingLoop]->endTime = std::stoi(time);

		std::string loopString = std::to_string(currentlyModifyingLoop + 1) + ": ";
		loopString += ConvertTimeToStringFromNumber(soundLoops[currentBGM][currentlyModifyingLoop]->startTime) + " - "
			+ ConvertTimeToStringFromNumber(soundLoops[currentBGM][currentlyModifyingLoop]->endTime);
		soundLoops[currentBGM][currentlyModifyingLoop]->text->SetText(loopString, soundLoops[currentBGM][currentlyModifyingLoop]->color);

		CreateDialog("Type the color for the loop:");
		manager->game->StartTextInput(dialog, "sound_test_loop_color");
	}
	catch (std::exception ex)
	{
		manager->game->logger.Log("ERROR: Could not set loop time end in music player: ");
		manager->game->logger.Log(ex.what());
	}
}

void SoundTest::AfterLoopDialog3(const std::string& color)
{
	try
	{
		// Technically there are multiple ways of representing color.
		// But for now, let's just do it like this: 255 255 255 (no alpha)

		std::istringstream iss(color);
		std::string s = "";

		getline(iss, s, ' ');
		int r = std::stoi(s);

		getline(iss, s, ' ');
		int g = std::stoi(s);

		getline(iss, s, ' ');
		int b = std::stoi(s);

		Color newColor = { (uint8_t)r, (uint8_t)g, (uint8_t)b, 255 };

		soundLoops[currentBGM][currentlyModifyingLoop]->color = newColor;
		soundLoops[currentBGM][currentlyModifyingLoop]->text->SetColor(newColor);

		currentlyModifyingLoop = -1;
	}
	catch (std::exception ex)
	{
		manager->game->logger.Log("ERROR: Could not set loop color in music player: ");
		manager->game->logger.Log(ex.what());
	}
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

void SoundTest::SaveData()
{
	std::ofstream fout;
	fout.open("data/soundtest.vars");
	if (fout.is_open())
	{
		fout << currentDir << std::endl;
		fout << currentBGM << std::endl;

		for (const auto& [key, loops] : soundLoops)
		{
			fout << "* " << key << std::endl;
			for (int i = 0; i < loops.size(); i++)
			{
				fout << loops[i]->name << " "
					<< loops[i]->startTime << " "
					<< loops[i]->endTime << " "
					<< (int)loops[i]->color.r << " "
					<< (int)loops[i]->color.g << " "
					<< (int)loops[i]->color.b << " "
					<< std::endl;
			}
		}
	}
	fout.close();
}

void SoundTest::AfterDirDialog(const std::string& dir)
{
	currentDir = dir;
	songText.SetText(currentDir + currentBGM);
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
}

void SoundTest::CreateLoop(const std::string& name, uint32_t st, uint32_t et, Color c)
{
	SoundLoop* newLoop = neww SoundLoop(name);

	newLoop->startTime = st;
	newLoop->endTime = et;
	newLoop->color = c;

	int buttonX = 1600;
	int buttonY = 100 + (200 * soundLoops[currentBGM].size());

	std::string loopString = std::to_string(soundLoops[currentBGM].size() + 1) + ": ";

	loopString += ConvertTimeToStringFromNumber(newLoop->startTime) + " - "
		+ ConvertTimeToStringFromNumber(newLoop->endTime);

	newLoop->text = neww Text(manager->game->theFont, loopString, true, true);
	newLoop->text->SetColor(newLoop->color);
	newLoop->text->SetPosition(buttonX, buttonY);

	buttonX += 300;
	newLoop->modifyButton = neww EditorButton("M", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	newLoop->modifyButton->text->SetPosition(buttonX, buttonY);

	buttonX += 150;
	newLoop->removeButton = neww EditorButton("R", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	newLoop->removeButton->text->SetPosition(buttonX, buttonY);

	buttonX += 150;
	newLoop->jumpButton = neww EditorButton("J", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	newLoop->jumpButton->text->SetPosition(buttonX, buttonY);

	buttonX += 150;
	newLoop->selectButton = neww EditorButton("S", "Btn", glm::vec3(buttonX, buttonY, 0), *manager->game);
	newLoop->selectButton->text->SetPosition(buttonX, buttonY);

	soundLoops[currentBGM].emplace_back(newLoop);
}

void SoundTest::SetSelectedLoopFromName(const std::string& bgmName, const std::string& loopName)
{
	for (int i = 0; i < soundLoops[bgmName].size(); i++)
	{
		if (soundLoops[bgmName][i]->name == loopName)
		{
			selectedLoop = i;
			return;
		}
	}

	manager->game->logger.Log("ERROR: Could not set loop " + loopName + " for BGM " + bgmName);
}

void SoundTest::Update(Game& game)
{
	if (Mix_PlayingMusic())
	{
		songTimer += game.dt;
		if (selectedLoop > -1)
		{
			if (songTimer >= soundLoops[currentBGM][selectedLoop]->endTime)
			{
				AfterJumpDialog(std::to_string(soundLoops[currentBGM][selectedLoop]->startTime / 1000.0));
			}
		}
	}
}

void SoundTest::UpdateSoundMode(Game& game)
{
	dialog.Update(game.inputText);

	// If song is playing, update the timer text
	if (!isPaused && Mix_PlayingMusic())
	{
		songTimer += game.dt;
		UpdateTimerText();

		if (selectedLoop > -1)
		{
			if (songTimer >= soundLoops[currentBGM][selectedLoop]->endTime)
			{
				AfterJumpDialog(std::to_string(soundLoops[currentBGM][selectedLoop]->startTime / 1000.0));
			}
		}
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
				else if (clickedButton == bgmUpButton)
				{
					ScrollCurrentBGM(true);
				}
				else if (clickedButton == bgmDownButton)
				{
					ScrollCurrentBGM(false);
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
					int length = 5;
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
					int length = 5;
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
				else if (clickedButton == addLoopButton)
				{
					uint32_t st = manager->game->randomManager.RandomRange(5, 15) * 1000;
					uint32_t et = st + (manager->game->randomManager.RandomRange(20, 40) * 1000);

					int r = manager->game->randomManager.RandomRange(100, 235);
					int g = manager->game->randomManager.RandomRange(100, 235);
					int b = manager->game->randomManager.RandomRange(100, 235);

					Color c = { (uint8_t)r, (uint8_t)g, (uint8_t)b, 255 };

					CreateLoop(std::to_string(soundLoops[currentBGM].size() + 1), st, et, c);
				}
			}
			else // check if we clicked a loop button
			{
				for (unsigned int i = 0; i < soundLoops[currentBGM].size(); i++)
				{
					// If clicked, modify loop i
					if (soundLoops[currentBGM][i]->modifyButton->IsPointInsideButton(mouseX, mouseY))
					{
						currentlyModifyingLoop = i;
						CreateDialog("Type the start time for the loop:");
						game.StartTextInput(dialog, "sound_test_loop_time1");
						break;
					}

					// If clicked, jump to loop i
					if (soundLoops[currentBGM][i]->jumpButton->IsPointInsideButton(mouseX, mouseY))
					{
						AfterJumpDialog(std::to_string( (soundLoops[currentBGM][i]->endTime/1000) - 2));
						break;
					}

					// If clicked, select loop i
					if (soundLoops[currentBGM][i]->selectButton->IsPointInsideButton(mouseX, mouseY))
					{
						if (selectedLoop == i)
							selectedLoop = -1;
						else
							selectedLoop = i;

						for (int k = 0; k < soundLoops[currentBGM].size(); k++)
						{
							if (k == i)
								soundLoops[currentBGM][k]->selectButton->color = { 64, 64, 64, 255 };
							else
								soundLoops[currentBGM][k]->selectButton->color = { 255, 255, 255, 255 };
						}

						break;
					}

					// If clicked, remove loop i
					if (soundLoops[currentBGM][i]->removeButton->IsPointInsideButton(mouseX, mouseY))
					{
						// NOTE: Be careful here, because this will change the numbers of the loops
						// and therefore might change what is going on in the game.
						soundLoops[currentBGM].erase(soundLoops[currentBGM].begin() + i);

						// Recalculate the positions (and numbers) of all the remaining loops

						int buttonX = 1600;
						int buttonY = 100;

						for (int k = 0; k < soundLoops[currentBGM].size(); k++)
						{
							buttonX = 1600;
							std::string loopString = std::to_string(k + 1) + ": ";
							loopString += ConvertTimeToStringFromNumber(soundLoops[currentBGM][k]->startTime) + " - "
								+ ConvertTimeToStringFromNumber(soundLoops[currentBGM][k]->endTime);
							soundLoops[currentBGM][k]->text->SetText(loopString, soundLoops[currentBGM][k]->color);
							soundLoops[currentBGM][k]->text->SetPosition(buttonX, buttonY);

							buttonX += 300;
							soundLoops[currentBGM][k]->modifyButton->position = glm::vec3(buttonX, buttonY, 0);
							soundLoops[currentBGM][k]->modifyButton->text->SetPosition(buttonX, buttonY);

							buttonX += 150;
							soundLoops[currentBGM][k]->removeButton->position = glm::vec3(buttonX, buttonY, 0);
							soundLoops[currentBGM][k]->removeButton->text->SetPosition(buttonX, buttonY);

							buttonX += 150;
							soundLoops[currentBGM][k]->jumpButton->position = glm::vec3(buttonX, buttonY, 0);
							soundLoops[currentBGM][k]->jumpButton->text->SetPosition(buttonX, buttonY);

							buttonX += 150;
							soundLoops[currentBGM][k]->selectButton->position = glm::vec3(buttonX, buttonY, 0);
							soundLoops[currentBGM][k]->selectButton->text->SetPosition(buttonX, buttonY);

							buttonY += 200;
						}

						break;
					}
				}
			}
		}
	}

	previousMouseState = mouseState;

	// TODO: total length of the song

}

float SoundTest::CalcTimelinePosition(float time, float a, float b, float w)
{
	float tlPosition = time / 1000.0f; // - Get current time in the song (30)

	tlPosition /= 85; // - Divide by the total length of the song   (30/60 = 0.5f)

	tlPosition *= (b - a);		// - Multiply this number by (B-A)   (400-100 = 300 * 0.5 = 150)

	tlPosition += a;	// - Add this number back to A (150 + 100 = 250)

	tlPosition -= w; // To offset due to OpenGL drawing with center origin, we subtract half the width

	return tlPosition;
}

void SoundTest::Render(const Renderer& renderer)
{
	timerText.Render(renderer);
	songText.Render(renderer);

	for (int i = 0; i < buttons.size(); i++)
	{
		if (buttons[i] != nullptr)
		{
			buttons[i]->Render(renderer);
		}
	}


	int a = 600;
	int y = 400;
	int w = 400;
	int b = a + (w * Camera::MULTIPLIER);

	float tlPos = 0;

	timelineRectangle.Render(glm::vec3(a, y, 0), renderer, Vector2(w, 20));

	for (int i = 0; i < soundLoops[currentBGM].size(); i++)
	{
		bool hideOtherLoops = !isPaused && Mix_PlayingMusic();

		if ((hideOtherLoops && i == selectedLoop) || (!hideOtherLoops))
		{
			timelineLocation.color = soundLoops[currentBGM][i]->color;

			tlPos = CalcTimelinePosition(soundLoops[currentBGM][i]->startTime, a, b, w);
			timelineLocation.Render(glm::vec3(tlPos, y, 0), renderer, Vector2(20, 20));

			tlPos = CalcTimelinePosition(soundLoops[currentBGM][i]->endTime, a, b, w);
			timelineLocation.Render(glm::vec3(tlPos, y, 0), renderer, Vector2(20, 20));
		}	

		soundLoops[currentBGM][i]->text->Render(renderer);

		if (soundLoops[currentBGM][i]->modifyButton != nullptr)
		{
			soundLoops[currentBGM][i]->modifyButton->Render(renderer);
		}

		if (soundLoops[currentBGM][i]->removeButton != nullptr)
		{
			soundLoops[currentBGM][i]->removeButton->Render(renderer);
		}

		if (soundLoops[currentBGM][i]->selectButton != nullptr)
		{
			soundLoops[currentBGM][i]->selectButton->Render(renderer);
		}

		if (soundLoops[currentBGM][i]->jumpButton != nullptr)
		{
			soundLoops[currentBGM][i]->jumpButton->Render(renderer);
		}
	}

	tlPos = CalcTimelinePosition(songTimer, a, b, w);
	timelineLocation.color = { 255, 255, 255, 255 };
	timelineLocation.Render(glm::vec3(tlPos, y, 0), renderer, Vector2(20, 20));

	dialog.Render(renderer);
}

void SoundTest::MusicFinished()
{

}