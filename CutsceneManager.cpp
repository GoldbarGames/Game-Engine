#include "CutsceneManager.h"
#include "Renderer.h"
#include "Game.h"
#include "globals.h"
#include <iterator>
#include "Logger.h"
#include "SoundManager.h"
#include "Textbox.h"

CutsceneManager::CutsceneManager()
{

}

void CutsceneManager::Init(Game& g)
{
	game = &g;
	textbox = neww Textbox(g.spriteManager, g.renderer);

	commands.manager = this;
	inputTimeToWait = 100;

	nextLetterTimer.Start(1);
	autoReaderTimer.Start(1);
	inputTimer.Start(1);

	namesToColors[""] = currentColor;

	LoadGlobalVariables();

	std::ifstream fin;

	bool testVN = true;
	std::string directory = "";
	std::string line = "";

	tags["b"] = neww TextTag();
	tags["i"] = neww TextTag();
	tags["bi"] = neww TextTag();
	tags["s"] = neww TextTag();

	if (testVN)
	{
		commands.pathPrefix = "assets\\arc\\";
		currentScript = "butler1";
		directory = "data/" + language + "/" + currentScript + ".txt";

		fin.open(directory);

		if (fin.is_open())
		{
			data = "";
			for (line; std::getline(fin, line); )
			{
				if (line[0] == '*')
					data += line + "*";
				else
					data += line + " ;";
			}
			fin.close();
			//PlayCutscene("define");
		}
	}
	else
	{
		commands.pathPrefix = "";
		currentScript = "cutscenes";
		directory = "data/" + language + "/" + currentScript + ".txt";

		fin.open(directory);

		if (fin.is_open())
		{
			data = "";
			for (line; std::getline(fin, line); )
			{
				data += line + " ;";
			}
			fin.close();
			//PlayCutscene("define");
		}
	}	
}

CutsceneManager::~CutsceneManager()
{
	for (int i = 0; i < labels.size(); i++)
	{
		//delete labels[i];
	}

	for (int i = 0; i < gosubStack.size(); i++)
	{
		delete gosubStack[i];
	}

	for (int i = 0; i < backlog.size(); i++)
	{
		delete backlog[i];
	}

	for (auto& [key, val] : images)
	{
		if (val != nullptr)
			delete_it(val);
	}


	for (auto& [key, val] : animatedImages)
	{
		if (val != nullptr)
			delete_it(val);
	}

	for (auto& [key, val] : tags)
	{
		if (val != nullptr)
			delete_it(val);
	}

	if (textbox != nullptr)
		delete_it(textbox);
}

void CutsceneManager::SetSpeakerText(const std::string& name)
{
	if (namesToNames.count(name) == 1)
	{
		textbox->speaker->SetText(namesToNames[name]);
	}
	else
	{
		textbox->speaker->SetText(name);
	}
}

//TODO: Find a way to make this less redundant
// in the case that the keys/conditions change
void CutsceneManager::CheckKeysWhileReading()
{
	const Uint8* input = SDL_GetKeyboardState(NULL);

	if (useMouseControls)
	{
		int mouseX, mouseY = 0;
		const Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);

		if (!readingBacklog)
		{
			// Note: In order to clear the mouse state, we check the SDL event in the main game loop
			if (mouseState & (~previousMouseState) & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				previousMouseState = SDL_BUTTON_LEFT;
				clickedMidPage = true;
			}
		}
	}

	if (useKeyboardControls)
	{
		if (!readingBacklog)
		{
			if (input[SDL_SCANCODE_SPACE] || input[SDL_SCANCODE_RETURN] || input[skipButton]
				|| (automaticallyRead && autoReaderTimer.HasElapsed()))
			{
				clickedMidPage = true;
			}
		}
	}


}

// Check input after textbox line has been fully read
void CutsceneManager::CheckKeys()
{
	const Uint8* input = SDL_GetKeyboardState(NULL);

	if (useMouseControls)
	{
		int mouseX, mouseY = 0;
		const Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);

		if (readingBacklog)
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_MOUSEWHEEL)
				{
					if (event.wheel.y > 0)
					{
						game->renderer.camera.Zoom(-0.1f, game->screenWidth, game->screenHeight);
					}
					else if (event.wheel.y < 0)
					{
						game->renderer.camera.Zoom(0.1f, game->screenWidth, game->screenHeight);
					}
				}
			}
			
		}
		else
		{
			// Note: In order to clear the mouse state, we check the SDL event in the main game loop
			if ( mouseState & (~previousMouseState) & SDL_BUTTON(SDL_BUTTON_LEFT) )
			{
				previousMouseState = SDL_BUTTON_LEFT;
				ReadNextLine();
			}
			else if ( (mouseState & (~previousMouseState) & SDL_BUTTON(SDL_BUTTON_RIGHT)) && rclickEnabled)
			{
				previousMouseState = SDL_BUTTON_RIGHT;
				commandIndex--;
				commands.GoSubroutine({ "", commands.buttonLabels[(unsigned int)SDL_SCANCODE_ESCAPE] });
			}
		}
	}

	if (useKeyboardControls)
	{
		if (readingBacklog)
		{
			if (input[SDL_SCANCODE_UP])
			{
				backlogIndex--;
				if (backlogIndex < 0)
					backlogIndex = 0;
				ReadBacklog();
				inputTimer.Start(inputTimeToWait);
			}
			else if (input[SDL_SCANCODE_DOWN])
			{
				backlogIndex++;
				if (backlogIndex >= backlog.size())
				{
					readingBacklog = false;
					textbox->speaker->SetText(currentLabel->lines[lineIndex].speaker, currentColor);
					textbox->text->SetText(currentText, currentColor);

					textbox->SetCursorPosition(letterIndex < currentLabel->lines[lineIndex].text.length());
				}
				else
				{
					ReadBacklog();
				}
				inputTimer.Start(inputTimeToWait);
			}
		}
		else if (input[SDL_SCANCODE_SPACE] || input[SDL_SCANCODE_RETURN] || input[skipButton]
			|| (automaticallyRead && autoReaderTimer.HasElapsed()))
		{
			// TODO: Make sure that you can't hold space/enter to advance the text
			ReadNextLine();
		}
		else if (input[SDL_SCANCODE_UP])
		{
			if (backlogEnabled)
			{
				readingBacklog = true;
				backlogIndex = backlog.size() - 1;
				ReadBacklog();
				inputTimer.Start(inputTimeToWait);
			}
		}
#if _DEBUG
		else if (input[SDL_SCANCODE_S]) // save game
		{
			SaveGame("file1.sav");
		}
		else if (input[SDL_SCANCODE_L]) // load game
		{
			LoadGame("file1.sav");
			ReadNextLine();
			isCarryingOutCommands = false;
		}
		else if (input[SDL_SCANCODE_TAB])
		{
			//TODO: This is not perfect, it just breaks out of the cutscene and does not carry out commands
			// Also, should maybe disable this outside of development mode or make it an option
			EndCutscene();
		}
#endif
		else
		{
			for (auto const& button : commands.buttonLabels)
			{
				if (input[button.first] && commands.buttonLabelsActive[button.first])
				{
					commandIndex--;
					commands.GoSubroutine({ button.second, button.second });
					break;
				}
			}
		}
	}

	
}

void CutsceneManager::ParseScene()
{
	//for (unsigned int i = 0; i < labels.size(); i++)
	//	delete labels[i];
	labels.clear();

	int index = 0;
	int cmdLetterIndex = 0;
	int cmdLetterLength = 0;

	data.erase(std::remove(data.begin(), data.end(), '\t'), data.end());

	do
	{
		SceneLabel newLabel;

		// Get label name
		index++; // begin with a *
		//TODO: This is broken on the very first label?
		//newLabel->name = ParseWord(data, '*', index);
		newLabel.name = ParseWord(data, '*', index);
		//std::cout << "Label name: " + newLabel->name << std::endl;

		std::vector<std::string> commands;

		// Until end of file or neww label...
		while (index < data.length() && data[index] != '*')
		{
			// If a `, we have a text line (otherwise, command)
			if (data[index] == '`')
			{
				index++;

				std::string newText = "";
				std::string newName = "";

				// deal with names, if any
				if (data[index] == ':')
				{
					index++; // skip :
					while (data[index] != ':')
					{
						newName += data[index];
						index++;
					}
					index++; // skip :
					index++; // skip space
				}

				// deal with the text
				while (data[index] != '`')
				{
					newText += data[index];
					index++;
				}

				//std::cout << newText << std::endl;

				// add all commands for this line
				SceneLine tempLine(newText, newName);
				for (unsigned int i = 0; i < commands.size(); i++)
				{
					//std::cout << tempCommands[i] << std::endl;
					tempLine.commands.emplace_back(commands[i]);
					//std::cout << commands[i] << std::endl;
				}

				// add the line
				newLabel.lines.emplace_back(tempLine);
				commands.clear();
				index++;
			}
			else // we have a command
			{
				std::string commandLine = "";

				// read until we hit the end of the line
				bool endOfLine = (data[index] == ';');
				while (!endOfLine)
				{
					if (index >= data.length())
					{
						std::cout << "Error on line: " + commandLine;
						break;
					}
					else
					{
						commandLine += data[index];
						index++;
					}
					endOfLine = (data[index] == ';');
				}

				index++;

				if (commandLine.find(':') != std::string::npos)
				{
					// don't split this up if we are evaluating an if-statement; split it later
					if (commandLine.size() > 2 && commandLine[0] == 'i' && commandLine[1] == 'f')
					{
						if (commandLine != "" && commandLine != " ")
						{
							commands.emplace_back(commandLine);
							//std::cout << commandLine << std::endl;
						}
					}
					else
					{
						cmdLetterIndex = 0;
						cmdLetterLength = 0;
						while (cmdLetterIndex < commandLine.size())
						{
							cmdLetterIndex++;
							cmdLetterLength++;
							if (commandLine[cmdLetterIndex] == ':' || cmdLetterIndex >= commandLine.size())
							{
								if (commandLine != "" && commandLine != " ")
								{
									std::string str = commandLine.substr((cmdLetterIndex - cmdLetterLength), cmdLetterLength);
									commands.emplace_back(str);
									//std::cout << str << std::endl;
									cmdLetterIndex++;
								}
								cmdLetterLength = 0;
							}
						}
					}					
				}
				else
				{
					if (commandLine != "" && commandLine != " ")
					{
						commands.emplace_back(commandLine);
						//std::cout << commandLine << std::endl;
					}
				}				
						
			}
		}

		// If we have commands but no line before a neww label,
		// add an empty text to that line
		if (commands.size() > 0)
		{
			SceneLine tempLine = SceneLine(" ", " ");
			for (unsigned int i = 0; i < commands.size(); i++)
			{
				//std::cout << tempCommands[i] << std::endl;
				tempLine.commands.emplace_back(commands[i]);
			}

			// add the line
			newLabel.lines.emplace_back(tempLine);
			commands.clear();
		}

		// when we encounter a neww label, add this one to the list
		if (newLabel.lines.size() > 0)
		{
			labels.emplace_back(newLabel);
		}			

	} while (index < data.length());

	ParseConfig("define");
}



void CutsceneManager::ParseConfig(const char* configName)
{
	int cmdIndex = 0;

	SceneLabel* configLabel = JumpToLabel(configName);

	if (configLabel != nullptr)
	{
		while (cmdIndex < configLabel->lines[0].commands.size())
		{
			commands.ExecuteCommand(configLabel->lines[0].commands[cmdIndex]);
			cmdIndex++;
		}
	}
	else
	{
		game->logger.Log("No configuration label found!");
	}
}



void CutsceneManager::Render(const Renderer& renderer)
{
	if (watchingCutscene)
	{
		// Render every sprite in the cutscene
		for (imageIterator = images.begin(); imageIterator != images.end(); imageIterator++)
		{
			if (imageIterator->second != nullptr)
				imageIterator->second->Render(renderer);
		}

		// Render the overlay above all sprites
		renderer.FadeOverlay(game->screenWidth, game->screenHeight);
		renderer.overlaySprite->Render(Vector2(0, 0), renderer);

		// Render the textbox above everything
		if (currentLabel->name != "title")
			textbox->Render(renderer, game->screenWidth, game->screenHeight);

		if (!isCarryingOutCommands && !isReadingNextLine)
		{
			textbox->clickToContinue->Render(renderer);
		}
	}
	else // only draw the overlay, not text or box
	{
		renderer.FadeOverlay(game->screenWidth, game->screenHeight);
		renderer.overlaySprite->Render(Vector2(0, 0), renderer);
	}
}

void CutsceneManager::JumpBack()
{
	int label = labelIndex;
	int line = lineIndex;
	int cmd = commandIndex;

	bool found = false;
	
	while (!found)
	{
		std::cout << "Label " << label << ", Line " << line << ", Cmd " << cmd << std::endl;
		if (label < labels.size() 
			&& line < labels[label].lines.size()
			&& cmd < labels[label].lines[line].commands.size()
			&& labels[label].lines[line].commands[cmd][0] == '~')
		{
			found = true;
			currentLabel = JumpToLabel(labels[label].name.c_str());
			labelIndex = label;
			lineIndex = line;
			commandIndex = cmd;
		}
		else
		{
			cmd--;
			if (cmd < 0)
			{
				line--;
				if (line < 0)
				{
					label--;
					if (label < 0)
						return;
					line = labels[label].lines.size() - 1;
				}
				cmd = labels[label].lines[line].commands.size() - 1;
			}
		}
	}

}

void CutsceneManager::JumpForward()
{
	for (unsigned int i = 0; i < labels.size(); i++)
	{
		for (unsigned int j = 0; j < labels[i].lines.size(); j++)
		{
			for (unsigned int k = 0; k < labels[i].lines[j].commands.size(); k++)
			{
				if (labels[i].lines[j].commands[k][0] == '~')
				{
					currentLabel = JumpToLabel(labels[i].name.c_str());
					labelIndex = i;
					lineIndex = j;
					commandIndex = k;
				}
			}
		}
	}
}

SceneLabel* CutsceneManager::JumpToLabel(const char* newLabelName)
{
	for (unsigned int i = 0; i < labels.size(); i++)
	{
		if (labels[i].name == newLabelName)
		{
			labelIndex = i;
			return &labels[i];
		}
	}
	return nullptr;
}

void CutsceneManager::PlayCutscene(const char* labelName)
{
	if (labelName != "" && labelName != "null")
	{
		watchingCutscene = true;
		textbox->isReading = true;
		currentLabel = JumpToLabel(labelName);

		// if failed to load label, exit cutscenes
		if (currentLabel == nullptr)
		{
			watchingCutscene = false;
		}			

		lineIndex = -1;

		ReadNextLine();
	}	
}

void CutsceneManager::EndCutscene()
{
	currentText = "";
	textbox->text->SetText(currentText);
	watchingCutscene = false;
	isCarryingOutCommands = false;
	isReadingNextLine = false;
}

void CutsceneManager::PushCurrentSceneDataToStack()
{
	SceneData* newData = neww SceneData();
	newData->labelIndex = labelIndex;
	newData->labelName = currentLabel->name;
	newData->lineIndex = lineIndex;
	newData->lineText = currentLabel->lines[lineIndex].text;
	newData->commandIndex = commandIndex;

	gosubStack.push_back(newData);
}

SceneData* CutsceneManager::PopSceneDataFromStack()
{
	if (gosubStack.size() == 0)
	{
		return nullptr;
	}		

	SceneData* data = gosubStack[gosubStack.size() - 1];
	gosubStack.pop_back();

	labelIndex = data->labelIndex;
	lineIndex = data->lineIndex;
	commandIndex = data->commandIndex;
	letterIndex = 0;
	currentText = "";

	return data;
}

void CutsceneManager::ClearAllSprites()
{
	// Clear all normal sprites
	unsigned int num = commands.GetNumAlias("l");	
	if (images[num] != nullptr)
		delete images[num];
	images[num] = nullptr;

	num = commands.GetNumAlias("c");
	if (images[num] != nullptr)
		delete images[num];
	images[num] = nullptr;

	num = commands.GetNumAlias("r");
	if (images[num] != nullptr)
		delete images[num];
	images[num] = nullptr;
}

void CutsceneManager::ReadNextLine()
{
	if (waitingForButton)
		return;

	if (readingSameLine)
	{		
		letterIndex++;
		isReadingNextLine = true;
		readingSameLine = false;
		return;
	}

	if (currentLabel != nullptr)
	{
		//TODO: Save this data somehow
		if (lineIndex >= 0 && lineIndex < currentLabel->lines.size())
		{
			seenLabelsToMostRecentLine[labelIndex] = lineIndex;
		}			

		//TODO: Make sure to save the backlog when we save the game
		if (lineIndex >= 0 && labelIndex >= 0)
		{
			Vector2 lastPos = textbox->text->GetLastGlyphPosition();
			backlog.push_back(new BacklogData(labelIndex, lineIndex, textbox->text->txt.c_str(), lastPos.x, lastPos.y));
		}

		if (backlog.size() > backlogMaxSize)
		{
			delete backlog[0];
			backlog.erase(backlog.begin());
		}
		
		lineIndex++;	
		currentText = "";
		textbox->text->SetText(currentText);

		if (lineIndex >= currentLabel->lines.size())
		{			
			if (labelIndex < labels.size() - 1)
			{
				if (autoreturn)
				{
					EndCutscene();
				}
				else // go to the next label in sequence
				{
					labelIndex++;
					currentLabel = JumpToLabel(labels[labelIndex].name.c_str());
					lineIndex = -1;
					ReadNextLine();
				}
			}
		}
		else
		{
			commandIndex = -1;
			letterIndex = 0;
			isCarryingOutCommands = true;
			isReadingNextLine = true;
			//textbox->isReading = false;

			FlushCurrentColor();

			// If speaker of this line is same as last, instantly show it
			textbox->speaker->SetText(currentLabel->lines[lineIndex].speaker, currentColor);

			if (autosave)
			{
				SaveGame("auto.sav");
			}
		}
	}	
}

void CutsceneManager::FlushCurrentColor()
{
	if (currentLabel != nullptr)
	{
		if (namesToColors.count(currentLabel->lines[lineIndex].speaker))
		{
			currentColor = namesToColors[currentLabel->lines[lineIndex].speaker];
		}
		else
		{
			currentColor = namesToColors[""];
		}
	}		
}

void CutsceneManager::ReadBacklog()
{	
	//Color color = namesToColors[label->lines[lineIndex].speaker];
	if (backlogIndex < backlog.size())
	{
		if (backlog[backlogIndex]->labelIndex < labels.size())
		{
			SceneLabel* label = &labels[backlog[backlogIndex]->labelIndex];
			if (label != nullptr && backlog[backlogIndex]->lineIndex < label->lines.size())
			{
				SceneLine* line = &(label->lines[backlog[backlogIndex]->lineIndex]);
				textbox->speaker->SetText(line->speaker, backlogColor);
				textbox->text->SetText(line->text, backlogColor);

				textbox->SetCursorPosition(true, Vector2(backlog[backlogIndex]->lastX, backlog[backlogIndex]->lastY));
			}
		}	
		else
		{
			textbox->text->SetText(backlog[backlogIndex]->text, backlogColor);
		}		
	}
}

SceneLabel* CutsceneManager::GetCurrentLabel()
{
	if (labelIndex >= 0 && labelIndex < labels.size())
		return &labels[labelIndex];
	else
		return nullptr;
}

SceneLine* CutsceneManager::GetCurrentLine()
{
	SceneLabel* label = GetCurrentLabel();
	if (label != nullptr)
	{
		if (lineIndex >= 0 && lineIndex < label->lines.size())
			return &label->lines[lineIndex];
		else
			return nullptr;
	}
	else
	{
		return nullptr;
	}
}

void CutsceneManager::Update()
{
	for (auto const& [key, image] : images)
	{
		if (image != nullptr)
			image->Update(*game);
	}

	for (auto const& [key, image] : animatedImages)
	{
		if (image != nullptr)
			image->Update(*game);
	}

	UpdateText();

	if (!isReadingNextLine && inputTimer.HasElapsed())
	{
		CheckKeys();
	}
	else
	{
		//TODO: If we press the button before the line has finished displaying,
		// then instantly show all the text (maybe a different button)
		CheckKeysWhileReading();
	}
}

void CutsceneManager::UpdateText()
{
	if (!printTimer.HasElapsed())
		return;

	const Uint8* input = SDL_GetKeyboardState(NULL);

	if (input[autoButton] && inputTimer.HasElapsed())
	{
		automaticallyRead = !automaticallyRead;
		inputTimer.Start(inputTimeToWait);
	}

	//TODO: Disable all of this if the keyboard controls are disabled
	// And also allow mouse control alternatives

	//TODO: Make this more customizable
	if (input[SDL_SCANCODE_0])
	{
		autoTimeIndex++;
		if (autoTimeIndex > 2)
			autoTimeIndex = 0;
	}
	if (input[SDL_SCANCODE_1])
	{
		autoTimeIndex = 0;
	}
	if (input[SDL_SCANCODE_2])
	{
		autoTimeIndex = 1;
	}
	if (input[SDL_SCANCODE_3])
	{
		autoTimeIndex = 2;
	}

	//TODO: Fix this, it no longer works properly with the corrected dt
	msGlyphTime += (float)game->dt;

	// If waiting for a button press... 
	// (the delay is so that the player doesn't press a button too quickly)
	if (waitingForButton && msGlyphTime > msDelayBetweenGlyphs)
	{
		msGlyphTime -= msDelayBetweenGlyphs;

		//TODO: Get mouse input for picking a choice
		//TODO: We want to get the mouse/keyboard input here

		if (inputTimer.HasElapsed())
		{
			if (input[SDL_SCANCODE_UP] || input[SDL_SCANCODE_W])
			{
				images[activeButtons[buttonIndex]]->SetColor({ 255, 255, 255, 255 });
				buttonIndex--;
				if (buttonIndex < 0)
					buttonIndex = activeButtons.size() - 1;
				images[activeButtons[buttonIndex]]->SetColor({ 255, 255, 0, 255 });
				inputTimer.Start(inputTimeToWait);
			}
			else if (input[SDL_SCANCODE_DOWN] || input[SDL_SCANCODE_S])
			{
				images[activeButtons[buttonIndex]]->SetColor({ 255, 255, 255, 255 });
				buttonIndex++;
				if (buttonIndex >= activeButtons.size())
					buttonIndex = 0;
				images[activeButtons[buttonIndex]]->SetColor({ 255, 255, 0, 255 });
				inputTimer.Start(inputTimeToWait);
			}
			else if (input[SDL_SCANCODE_SPACE] || input[SDL_SCANCODE_RETURN])
			{
				// Return the result in the specified variable and resume reading
				// TODO: This can be buggy if the btnwait variable is not reset beforehand
				unsigned int chosenSprite = activeButtons[buttonIndex];
				commands.numberVariables[buttonResult] = spriteButtons[chosenSprite];
				waitingForButton = false;
				isCarryingOutCommands = true;
				isReadingNextLine = true;
				textbox->isReading = true;

				// Remove the sprite buttons from the screen
				commands.ClearSprite({ "", std::to_string(choiceSpriteStartNumber) });   // bg
				commands.ClearSprite({ "", std::to_string(choiceSpriteStartNumber + 1) }); // question
				for (int i = 0; i < activeButtons.size(); i++)
				{
					commands.ClearSprite({ "", std::to_string(activeButtons[i]) });
				}

				activeButtons.clear();

				// Evaluate if statements
				if (choiceIfStatements.size() > 0)
				{
					for (int i = 0; i < choiceIfStatements.size(); i++)
					{
						commands.ExecuteCommand(choiceIfStatements[i]);
					}
				}

				inputTimer.Start(inputTimeToWait);
			}
		}

		return;
	}

	// render the textbox when not waiting
	textbox->isReading = (msGlyphTime > 0);

	if (input[skipButton])
		msDelayBetweenGlyphs = 0.0f;
	else
		msDelayBetweenGlyphs = msInitialDelayBetweenGlyphs;

	//TODO: Continue to execute functions that have not finished yet (moveto, lerp) (multi-threading?)

	int unfinishedIndex = 0;
	while (unfinishedIndex < unfinishedCommands.size())
	{
		bool finished = commands.ExecuteCommand(unfinishedCommands[unfinishedIndex]);

		if (finished)
		{
			unfinishedCommands.erase(unfinishedCommands.begin() + unfinishedIndex);
		}
		else
		{
			unfinishedIndex++;
		}
	}

	if (msGlyphTime > msDelayBetweenGlyphs)
	{
		if (currentLabel == nullptr)
		{
			std::cout << "ERROR - current label is null!" << std::endl;
			return;
		}

		if (commandIndex < 0)
		{
			commandIndex = 0;
		}

		if (lineIndex < 0)
		{
			lineIndex = 0;
			letterIndex = 0;
		}

		if (isCarryingOutCommands)
		{			
			if (commandIndex >= 0 && commandIndex < currentLabel->lines[lineIndex].commands.size())
			{
				//std::cout << currentLabel->lines[lineIndex].commands[commandIndex] << std::endl;
				printNumber = 0;
				do
				{
					if (!commands.ExecuteCommand(currentLabel->lines[lineIndex].commands[commandIndex]))
					{
						unfinishedCommands.push_back(currentLabel->lines[lineIndex].commands[commandIndex]);
					}
					commandIndex++;
					if (commandIndex >= currentLabel->lines[lineIndex].commands.size())
						break;
				} while (!autoprint && printNumber == 0);	
				game->updateScreenTexture = true;

				if (printNumber > 0 && !input[skipButton])
				{
					switch (printNumber)
					{
					case 42:
						printTimer.Start(1000);
						break;
					case 22:
						printTimer.Start(200);
						break;
					default:
						printTimer.Start(200);
						break;
					}
				}
			}
			else
			{
				isCarryingOutCommands = false;
			}
		}
		else if (isReadingNextLine)
		{
			bool displayAllText = (msDelayBetweenGlyphs == 0) || clickedMidPage;

			do
			{
				std::string result = ParseText(currentLabel->lines[lineIndex].text, letterIndex, currentColor, textbox->text);

				if (result.size() > 1)
				{
					for (int i = 0; i < result.size(); i++)
					{
						textbox->UpdateText(result[i], currentColor);
					}
				}
				else if (result.size() == 1)
				{
					textbox->UpdateText(result[0], currentColor);
				}

				if (currentText.length() == 1)
				{
					textbox->speaker->SetText(currentLabel->lines[lineIndex].speaker, currentColor);
				}

				//nextLetterTimer.Start(lettersPerFrame * delay);

				// Reached the 'click to continue' point
				if (letterIndex >= currentLabel->lines[lineIndex].text.length())
				{
					currentText = textbox->text->txt;
					isReadingNextLine = false;
					displayAllText = false;
					clickedMidPage = false;

					textbox->SetCursorPosition(true);
					//textbox->clickToContinue->Update(*game);
					//game->player->cutsceneInputTimer.Start(100);

					if (automaticallyRead)
					{
						autoTimeToWait = (textbox->text->glyphs.size() * autoTimeToWaitPerGlyph);
						autoReaderTimer.Start(autoTimeToWait);
					}


					return;
				}
				else if (currentLabel->lines[lineIndex].text[letterIndex] == '@')
				{
					currentText = textbox->text->txt;
					readingSameLine = true;
					isReadingNextLine = false;
					displayAllText = false;
					clickedMidPage = false;

					textbox->SetCursorPosition(false);
					//textbox->clickToContinue->Update(*game);

					if (automaticallyRead)
					{
						autoTimeToWait = (textbox->text->glyphs.size() * autoTimeToWaitPerGlyph);
						autoReaderTimer.Start(autoTimeToWait);
					}
				}


			} while (displayAllText);

		}
	}

	if (msDelayBetweenGlyphs > 0)
	{
		while (msGlyphTime > msDelayBetweenGlyphs)
		{
			msGlyphTime -= msDelayBetweenGlyphs;
		}
	}
}

std::string CutsceneManager::ParseText(const std::string& originalString, int& letterIndex, Color& textColor, Text* text)
{
	std::string result = "";

	// Handle word replacements here
	if (originalString[letterIndex] == '[')
	{
		letterIndex++;

		// Get everything until the next ] symbol
		int varNameIndex = letterIndex;
		std::string word = ParseWord(originalString, ']', letterIndex);

		// at this point we have the $variablename, check the first character to get the type

		if (word.length() != 0)
		{
			varNameIndex++;
			std::string variableName = ParseWord(originalString, ']', varNameIndex);
			std::string variableValue = "";
			switch (word[0])
			{
			case '$': // string
				variableValue = commands.GetStringVariable(commands.GetNumAlias(variableName));
				break;
			case '%': // number
				variableValue = std::to_string(commands.GetNumberVariable(commands.GetNumAlias(variableName)));
				break;
			default:
				break;
			}

			//letterIndex--;

			for (int valueIndex = 0; valueIndex < variableValue.length(); valueIndex++)
			{
				result += variableValue[valueIndex];
			}
			return result;
			
		}
	}
	// Handle color changes here
	else if (originalString[letterIndex] == '#')
	{
		if (originalString.size() > letterIndex + 1 && originalString[letterIndex + 1] == '#')
		{
			if (currentLabel != nullptr)
			{
				if (namesToColors.count(currentLabel->lines[lineIndex].speaker))
				{
					textColor = namesToColors[currentLabel->lines[lineIndex].speaker];
				}
				else
				{
					textColor = namesToColors[""];
				}
			}
			letterIndex++;
		}
		else
		{
			int finalColorIndex = letterIndex;
			do
			{
				finalColorIndex++;
				if (finalColorIndex >= originalString.size())
					break;

			}while (originalString[finalColorIndex] != '#');

			// Parse the hexadecimal color string
			textColor = ParseColorHexadecimal(originalString.substr(letterIndex, finalColorIndex).c_str());
			letterIndex = finalColorIndex;
		}
	}
	else if (originalString[letterIndex] == '<')
	{
		bool active = (originalString[letterIndex + 1] != '/');

		int tagIndex = active ? letterIndex + 1 : letterIndex + 2;

		std::string tagName = "";
		while (originalString[tagIndex] != '>')
		{
			tagName += originalString[tagIndex];
			tagIndex++;

			if (tagIndex >= originalString.size())
				break;
		}

		if (tags.count(tagName) == 1) // de/activate the tag
		{
			letterIndex = tagIndex;
			tags[tagName]->active = active;

			// Reset to regular font, then apply all changes one by one
			textbox->text->SetFont(text->currentFontInfo->GetRegularFont());

			if (tags["b"]->active && tags["i"]->active)
			{
				text->SetFont(text->currentFontInfo->GetBoldItalicsFont());
			}
			else if (tags["i"]->active)
			{
				text->SetFont(text->currentFontInfo->GetItalicsFont());
			}
			else if (tags["b"]->active)
			{
				text->SetFont(text->currentFontInfo->GetBoldFont());
			}
		}
		else if (tagName == "img")
		{
			std::string imageName = "";
			int imageIndex = tagIndex + 1;
			while(originalString[imageIndex] != '<')
			{
				imageName += originalString[imageIndex];
				imageIndex++;
				if (imageIndex >= originalString.size())
					break;
			}

			// Add the image to the text here
			//TODO: Check this for memory leaks!
			Sprite* sprite = game->CreateSprite(commands.ParseStringValue(imageName), ShaderName::Default);

			text->AddImage(sprite);
			letterIndex = imageIndex;
			letterIndex += 6; // /img>

			return result;
		}
		else if (tagName == "anim")
		{
			std::string animName = "";
			int animIndex = tagIndex + 1;
			while (originalString[animIndex] != '<')
			{
				animName += originalString[animIndex];
				animIndex++;
				if (animIndex >= originalString.size())
					break;
			}

			if (animatedImages[animName] != nullptr)
			{
				// Add the image to the text here
				//TODO: Check this for memory leaks!
				Sprite* sprite = animatedImages[animName]->GetSprite();
				text->AddImage(sprite);		
				text->GetLastGlyph()->animator = animatedImages[animName]->GetAnimator();
			}
			else
			{
				commands.ErrorLog({ "", "Error loading animation " + animName });
			}

			letterIndex = animIndex;
			while (originalString[letterIndex] != '>')
				letterIndex++;
			letterIndex++;
			//letterIndex += 7; // /anim>

			return result;
		}
		else if (tagName[0] == 's' && tagName.size() > 1)
		{
			std::string fontSize = "";
			for (int i = 1; i < tagName.size(); i++)
			{
				fontSize += tagName[i];
			}

			letterIndex = tagIndex;
			textbox->SetFontSize(std::stoi(fontSize));
		}
		else // do nothing different
		{
			result = originalString[letterIndex];
		}
	}
	else
	{
		result = originalString[letterIndex];
	}

	letterIndex++;
	
	return result;
}

void CutsceneManager::LoadGlobalVariables()
{
	std::ifstream fin;
	std::vector<std::string> globalDataNumbers;
	std::vector<std::string> globalDataStrings;

	int globalSection = 0;

	fin.open("saves/globals.sav");
	for (std::string line; std::getline(fin, line); )
	{
		if (line == "@ GLOBAL_STRINGS")
		{
			globalSection = 1;
			continue;
		}
		else if (line == "@ GLOBAL_NUMBERS")
		{
			globalSection = 2;
			continue;
		}

		switch (globalSection)
		{
		case 1:
			globalDataStrings.push_back(line + "\n");
			break;
		case 2:
			globalDataNumbers.push_back(line + "\n");
			break;
		default:
			break;
		}

	}
	fin.close();


	// 2. Parse the vectors of strings and put them into the variable maps

	std::string dataKey = "";
	std::string dataValue = "";
	int index = 0;

	for (int i = 0; i < globalDataStrings.size(); i++)
	{
		index = 0;
		dataKey = ParseWord(globalDataStrings[i], ' ', index);
		dataValue = ParseWord(globalDataStrings[i], '\n', index);
		commands.SetStringVariable({ "", dataKey, dataValue });
	}

	for (int i = 0; i < globalDataNumbers.size(); i++)
	{
		index = 0;
		dataKey = ParseWord(globalDataNumbers[i], ' ', index);
		dataValue = ParseWord(globalDataNumbers[i], '\n', index);
		commands.SetNumberVariable({ "", dataKey, dataValue, "no_alias" });
	}

}

void CutsceneManager::SaveGlobalVariable(unsigned int key, const std::string& value, bool isNumber)
{
	// 1. Load the global variables into a data structure from the file
	std::ifstream fin;
	std::vector<std::string> globalDataNumbers;
	std::vector<std::string> globalDataStrings;

	int globalSection = 0;

	fin.open("saves/globals.sav");
	for (std::string line; std::getline(fin, line); )
	{
		if (line == "@ GLOBAL_STRINGS")
		{
			globalSection = 1;
			continue;
		}			
		else if (line == "@ GLOBAL_NUMBERS")
		{
			globalSection = 2;
			continue;
		}			

		switch (globalSection)
		{
		case 1:
			globalDataStrings.push_back(line + "\n");
			break;
		case 2:
			globalDataNumbers.push_back(line + "\n");
			break;
		default:
			break;
		}
		
	}
	fin.close();

	if (isNumber)
	{
		ModifyGlobalVariableVector(globalDataNumbers, key, value);
	}
	else
	{
		ModifyGlobalVariableVector(globalDataStrings, key, value);
	}

	// 4. Save the DS to the file
	std::ofstream fout;
	fout.open("saves/globals.sav");

	fout << "@ GLOBAL_STRINGS" << std::endl;
	for (int i = 0; i < globalDataStrings.size(); i++)
	{
		fout << globalDataStrings[i];
	}

	fout << "@ GLOBAL_NUMBERS" << std::endl;
	for (int i = 0; i < globalDataNumbers.size(); i++)
	{
		fout << globalDataNumbers[i];
	}

	fout.close();

}

void CutsceneManager::ModifyGlobalVariableVector(std::vector<string>& globalData, unsigned int key, const std::string& value)
{
	bool foundData = false;
	int index = 0;
	for (int i = 0; i < globalData.size(); i++)
	{
		index = 0;
		if (ParseWord(globalData[i], ' ', index) == std::to_string(key))
		{
			// 2. If the global variable is already in the DS, update its value in the DS
			globalData[i] = std::to_string(key) + " " + value + "\n";
			foundData = true;
			break;
		}
	}

	if (!foundData)
	{
		// 3. Else, add the global variable and its value to the DS
		globalData.push_back(std::to_string(key) + " " + value + "\n");
	}
}


//TODO: Regarding saving/loading...
// * Window title/icon
// * Play the BGM and other sounds
// + Controller bindings and settings
// + Random seed
// Function definitions
// + Gosub stack
// Timer values
// Textbox customization
// Backlog customization
// Window resolution
// Other variables in the manager

void CutsceneManager::SaveGame(const char* filename, const char* path)
{
	std::ofstream fout;

	// 1. Save the story data (label, line, command)
	// (the information necessary for us to find our place in the story)

	//TODO: Make sure to save the gosub stack as well
	fout.open(std::string(path) + filename);

	std::map<SaveSections, std::string> sections = {
		{ SaveSections::CONFIG_OPTIONS, "@ CONFIG_OPTIONS"},
		{ SaveSections::STORY_DATA, "@ STORY_DATA"},
		{ SaveSections::SEEN_LINES, "@ SEEN_LINES"},
		{ SaveSections::GOSUB_STACK, "@ GOSUB_STACK"},
		{ SaveSections::ALIAS_STRINGS, "@ ALIAS_STRINGS"},
		{ SaveSections::ALIAS_NUMBERS, "@ ALIAS_NUMBERS"},
		{ SaveSections::LOCAL_STRINGS, "@ LOCAL_STRINGS"},
		{ SaveSections::LOCAL_NUMBERS, "@ LOCAL_NUMBERS"},
		{ SaveSections::LOCAL_OBJECTS, "@ LOCAL_OBJECTS"},
		{ SaveSections::NAMES_TO_COLORS, "@ NAMES_TO_COLORS"},
		{ SaveSections::OTHER_STUFF, "@ OTHER_STUFF"}
	};

	for (auto const& x : sections)
	{
		fout << x.second << std::endl;

		switch (x.first)
		{
		case SaveSections::CONFIG_OPTIONS:
			fout << "global_start " << globalStart << std::endl;
			break;
		case SaveSections::STORY_DATA:
			fout << currentLabel->name << " ";
			fout << labelIndex << " ";
			fout << lineIndex << " ";
			fout << commandIndex << std::endl;
			break;
		case SaveSections::SEEN_LINES:

			for (auto const& var : seenLabelsToMostRecentLine)
			{
				fout << var.first << " " << var.second << std::endl;
			}

			break;
		case SaveSections::GOSUB_STACK:

			for (int i = gosubStack.size() - 1; i >= 0; i--)
			{
				fout << gosubStack[i]->labelName << " ";
				fout << gosubStack[i]->labelIndex << " ";
				fout << gosubStack[i]->lineIndex << " ";
				fout << gosubStack[i]->commandIndex << std::endl;
			}

			break;
		case SaveSections::ALIAS_STRINGS:
			// 4. Save string aliases (keys and values)
			for (auto const& var : commands.stralias)
			{
				fout << var.first << " " << var.second << std::endl;
			}
			break;
		case SaveSections::ALIAS_NUMBERS:
			// 5. Save number aliases (keys and values)
			for (auto const& var : commands.numalias)
			{
				fout << var.first << " " << var.second << std::endl;
			}
			break;
		case SaveSections::LOCAL_STRINGS:
			// 2. Save string variables (keys and values)
			for (auto const& var : commands.stringVariables)
			{
				if (var.first < globalStart)
				{
					fout << var.first  // key
						<< " "
						<< var.second  // value 
						<< std::endl;
				}
			}
			break;
		case SaveSections::LOCAL_NUMBERS:
			// 3. Save number variables (keys and values)
			for (auto const& var : commands.numberVariables)
			{
				if (var.first < globalStart)
				{
					fout << var.first  // key
						<< " "
						<< var.second  // value 
						<< std::endl;
				}
			}
			break;
		case SaveSections::LOCAL_OBJECTS:
			// 6. Save object information (sprite number, filepath/text, position, rotation, color, etc.)
			for (auto const& var : images)
			{
				Entity* entity = var.second;

				if (entity != nullptr)
				{
					std::string s = typeid(*entity).name();
					std::cout << s << std::endl;
					if (s == "class Text")
					{
						//TODO: Store font, size, style, etc. (for each letter)
						Text* text = static_cast<Text*>(entity);
						fout << var.first  // key
							<< " "
							<< "text"
							<< " "
							<< text->position.x
							<< " "
							<< text->position.y
							<< " "
							<< text->rotation.x
							<< " "
							<< text->rotation.y
							<< " "
							<< text->rotation.z
							<< " "
							<< text->scale.x
							<< " "
							<< text->scale.y
							<< " "
							<< text->GetTextString()
							<< " "
							<< text->textColor.r
							<< " "
							<< text->textColor.g
							<< " "
							<< text->textColor.b
							<< " "
							<< text->textColor.a
							<< std::endl;
					}
					else
					{
						fout << var.first  // key
							<< " "
							<< entity->GetSprite()->filename
							<< " "
							<< entity->position.x
							<< " "
							<< entity->position.y
							<< " "
							<< entity->rotation.x
							<< " "
							<< entity->rotation.y
							<< " "
							<< entity->rotation.z
							<< " "
							<< entity->scale.x
							<< " "
							<< entity->scale.y
							<< std::endl;
					}
				}
			}
			break;
		case SaveSections::NAMES_TO_COLORS:
			// 7. Save other important things (random seed, music volume, textbox color, etc.)
			for (auto const& var : namesToColors)
			{
				if (var.first != "")
				{
					fout << var.first  // key
						<< " "
						<< var.second.r
						<< " "
						<< var.second.g
						<< " "
						<< var.second.b
						<< " "
						<< var.second.a
						<< std::endl;
				}
				else
				{
					fout << "_"  // key
						<< " "
						<< var.second.r
						<< " "
						<< var.second.g
						<< " "
						<< var.second.b
						<< " "
						<< var.second.a
						<< std::endl;
				}
			}
			break;
		case SaveSections::OTHER_STUFF:

			// Save the window title
			fout << "window title " << game->windowTitle << std::endl;

			// Save the window icon
			fout << "window icon " << game->windowIconFilepath << std::endl;

			// Save the random seed
			fout << "random seed " << commands.randomSeed << std::endl;

			// Save controller bindings
			fout << "controls mouse " << useMouseControls << std::endl;
			fout << "controls keyboard " << useKeyboardControls << std::endl;

			// Save the currently playing BGM
			fout << "bgm " << game->soundManager.bgmFilepath << std::endl;

			// Save other looped sounds
			for (auto const& [num, channel] : game->soundManager.sounds)
			{
				if (channel->loop == -1)
					fout << "me " << channel->num << " " << channel->sound->filepath << std::endl;
				else
					fout << "se " << channel->num << " " << channel->sound->filepath << " " << channel->loop << std::endl;
			}

			break;
		default:
			break;
		}
	}

	fout.close();

}

void CutsceneManager::LoadGame(const char* filename, const char* path)
{
	std::ifstream fin;
	//std::string data = "";
	std::vector<std::string> dataLines;

	fin.open(std::string(path) + filename);
	for (std::string line; std::getline(fin, line); )
	{
		//data += line + "\n";
		dataLines.push_back(line);
	}
	fin.close();

	gosubStack.clear();

	// While we have lines to examine
	// Check the first character for an @ symbol
	// If there is one, change the current section
	// Otherwise, interpret the line based on the current section

	std::map<std::string, SaveSections> sections = {
		{ "@ CONFIG_OPTIONS", SaveSections::CONFIG_OPTIONS},
		{ "@ STORY_DATA", SaveSections::STORY_DATA},
		{ "@ SEEN_LINES", SaveSections::SEEN_LINES},
		{ "@ GOSUB_STACK", SaveSections::GOSUB_STACK},
		{ "@ ALIAS_STRINGS", SaveSections::ALIAS_STRINGS},
		{ "@ ALIAS_NUMBERS", SaveSections::ALIAS_NUMBERS},
		{ "@ LOCAL_STRINGS", SaveSections::LOCAL_STRINGS},
		{ "@ LOCAL_NUMBERS", SaveSections::LOCAL_NUMBERS},
		{ "@ LOCAL_OBJECTS", SaveSections::LOCAL_OBJECTS},
		{ "@ NAMES_TO_COLORS", SaveSections::NAMES_TO_COLORS},
		{ "@ OTHER_STUFF",  SaveSections::OTHER_STUFF}
	};

	int index = 0;
	std::string currentSection = "";
	std::string labelName = "";
	SceneData* gosubData = nullptr;

	while (index < dataLines.size())
	{
		std::stringstream ss(dataLines[index]);
		std::istream_iterator<std::string> begin(ss);
		std::istream_iterator<std::string> end;

		std::vector<std::string> lineParams(begin, end);
		//std::copy(lineParams.begin(), lineParams.end(), std::ostream_iterator<std::string>(std::cout, " "));

		if (dataLines[index][0] == '@')
		{
			currentSection = dataLines[index];
		}
		else
		{
			SaveSections section = sections[currentSection];
			switch (section)
			{
			case SaveSections::CONFIG_OPTIONS:

				//TODO: Deal with other config options later
				if (lineParams[0] == "global_start")
					globalStart = std::stoi(lineParams[1]);

				break;
			case SaveSections::STORY_DATA:
				// These aren't actually stored in the cutscene manager,
				// but we might want to use them to more accurately find the load point

				labelName = lineParams[0];
				//std::string lineText = saveData[1];
				labelIndex = std::stoi(lineParams[1]);
				lineIndex = std::stoi(lineParams[2]) - 1;
				commandIndex = std::stoi(lineParams[3]);
				isCarryingOutCommands = false;

				currentLabel = JumpToLabel(labelName.c_str());
				if (currentLabel == nullptr)
				{
					if (labelIndex < labels.size())
						currentLabel = &labels[labelIndex];
					else
						std::cout << "ERROR: Could not find label " << labelName << std::endl;
				}
				break;
			case SaveSections::SEEN_LINES:
				seenLabelsToMostRecentLine[std::stoi(lineParams[0])] = std::stoi(lineParams[1]);
				break;
			case SaveSections::GOSUB_STACK:

				gosubData = neww SceneData();
				gosubData->labelName = lineParams[0];
				gosubData->labelIndex = std::stoi(lineParams[1]);
				gosubData->lineIndex = std::stoi(lineParams[2]) - 1;
				gosubData->commandIndex = std::stoi(lineParams[3]);
				gosubStack.push_back(gosubData);

				break;
			case SaveSections::ALIAS_STRINGS:
				commands.stralias[lineParams[0]] = lineParams[1];
				break;
			case SaveSections::ALIAS_NUMBERS:
				commands.numalias[lineParams[0]] = std::stoi(lineParams[1]);
				break;
			case SaveSections::LOCAL_STRINGS:
				commands.stringVariables[std::stoi(lineParams[0])] = lineParams[1];
				break;
			case SaveSections::LOCAL_NUMBERS:
				commands.numberVariables[std::stoi(lineParams[0])] = std::stoi(lineParams[1]);
				break;
			case SaveSections::LOCAL_OBJECTS:
				if (lineParams[1] == "text") // load text object
				{
					commands.LoadTextFromSaveFile(lineParams);
				}
				else // load sprite object
				{
					lineParams.insert(lineParams.begin(), "");
					commands.LoadSprite(lineParams);
					Entity* entity = images[std::stoi(lineParams[1])];

					entity->rotation = glm::vec3(
						std::stoi(lineParams[5]), 
						std::stoi(lineParams[6]), 
						std::stoi(lineParams[7]));
					
					entity->scale = Vector2(
						std::stoi(lineParams[8]), 
						std::stoi(lineParams[9]));

					entity->SetSprite(*entity->GetSprite());
				}
				break;
			case SaveSections::NAMES_TO_COLORS:
				if (lineParams[0] == "_")
				{
					lineParams[0] = "";
				}
				else
				{
					namesToColors[lineParams[0]] = {
					(uint8_t)std::stoi(lineParams[1]),
					(uint8_t)std::stoi(lineParams[2]),
					(uint8_t)std::stoi(lineParams[3]),
					(uint8_t)std::stoi(lineParams[4])
					};
				}

				break;
			case SaveSections::OTHER_STUFF:

				if (lineParams[0] == "bgm")
				{
					game->soundManager.PlayBGM(lineParams[1]);
				}
				else if (lineParams[0] == "me")
				{
					game->soundManager.PlaySound(lineParams[2], std::stoi(lineParams[1]));
				}
				else if (lineParams[0] == "se")
				{
					game->soundManager.PlaySound(lineParams[2], std::stoi(lineParams[1]), std::stoi(lineParams[3]));
				}
				else if (lineParams[0] == "window")
				{
					commands.WindowFunction(lineParams);
				}
				else if (lineParams[0] == "random")
				{
					commands.RandomNumberVariable(lineParams);
				}
				else if (lineParams[0] == "controls")
				{
					if (lineParams[1] == "mouse")
					{
						useMouseControls = std::stoi(lineParams[2]);
					}
					else if (lineParams[1] == "keyboard")
					{
						useKeyboardControls = std::stoi(lineParams[2]);
					}
				}

				break;
			default:
				break;
			}
		}

		index++;
	}

	// 2. Load string variables (keys and values)
	// 3. Load number variables (keys and values)
	// 4. Load string aliases (keys and values)
	// 5. Load number aliases (keys and values)
	// 6. Load object information (sprite number, filepath/text, position, rotation, color, etc.)
	// 7. Load other important things (random seed, music volume, textbox color, etc.)


}
