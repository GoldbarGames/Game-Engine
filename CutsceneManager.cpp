#include "CutsceneManager.h"
#include "Renderer.h"
#include "Game.h"
#include "globals.h"
#include <iterator>

CutsceneManager::CutsceneManager(Game& g)
{
	game = &g;
	textbox = new Textbox(g.spriteManager, g.renderer);

	commands.manager = this;
	inputTimeToWait = 100;

	nextLetterTimer.Start(1);
	autoReaderTimer.Start(1);
	inputTimer.Start(1);

	namesToColors[""] = currentColor;

	std::ifstream fin;
	std::string directory = "data/" + language + "/cutscenes.txt";

	fin.open(directory);

	if (fin.is_open())
	{
		data = "";
		for (std::string line; std::getline(fin, line); )
		{
			data += line + " ;";
		}
	}
}

void CutsceneManager::CheckKeys()
{
	const Uint8* input = SDL_GetKeyboardState(NULL);

	if (input[SDL_SCANCODE_SPACE] || input[SDL_SCANCODE_RETURN] || input[skipButton]
		|| (automaticallyRead && autoReaderTimer.HasElapsed()))
	{
		ReadNextLine();
	}
	else if (input[SDL_SCANCODE_TAB])
	{
		//TODO: This is not perfect, it just breaks out of the cutscene and does not carry out commands
		// Also, should maybe disable this outside of development mode or make it an option
#if _DEBUG
		EndCutscene();
#endif
	}
	else if (input[SDL_SCANCODE_UP])
	{
		//textbox->Test();
	}
	else if (input[SDL_SCANCODE_S]) // save game
	{
		SaveGame();
	}
	else if (input[SDL_SCANCODE_L]) // load game
	{
		LoadGame();
		ReadNextLine();
	}
	else
	{
		for (auto const& button : commands.buttonLabels)
		{
			if (input[button.first]) //TODO: Also check if button is active
			{
				commandIndex--;
				commands.GoSubroutine({ button.second, button.second });
				break;
			}
		}
	}
}

void CutsceneManager::ParseScene()
{
	for (unsigned int i = 0; i < labels.size(); i++)
		delete labels[i];
	labels.clear();

	int index = 0;

	do
	{
		SceneLabel* newLabel = new SceneLabel;

		// Get label name
		index++; // begin with a *
		newLabel->name = ParseWord(data, '*', index);
		//cout << "Label name: " + newLabel->name;

		std::vector<std::string> commands;

		// Until end of file or new label...
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
				SceneLine* tempLine = new SceneLine(newText, newName);
				for (unsigned int i = 0; i < commands.size(); i++)
				{
					//std::cout << tempCommands[i] << std::endl;
					tempLine->commands.emplace_back(commands[i]);
				}

				// add the line
				newLabel->lines.emplace_back(tempLine);
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
				commands.emplace_back(commandLine);
			}
		}

		// If we have commands but no line before a new label,
		// add an empty text to that line
		if (commands.size() > 0)
		{
			SceneLine* tempLine = new SceneLine("", "");
			for (unsigned int i = 0; i < commands.size(); i++)
			{
				//std::cout << tempCommands[i] << std::endl;
				tempLine->commands.emplace_back(commands[i]);
			}

			// add the line
			newLabel->lines.emplace_back(tempLine);
			commands.clear();
		}

		// when we encounter a new label, add this one to the list
		labels.emplace_back(newLabel);

	} while (index < data.length());
}


void CutsceneManager::Render(Renderer * renderer)
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
		renderer->FadeOverlay(game->screenWidth, game->screenHeight);

		// Render the textbox above everything
		textbox->Render(renderer, game->screenWidth, game->screenHeight);
	}
	else // only draw the overlay, not text or box
	{
		renderer->FadeOverlay(game->screenWidth, game->screenHeight);
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
			&& line < labels[label]->lines.size()
			&& cmd < labels[label]->lines[line]->commands.size()
			&& labels[label]->lines[line]->commands[cmd][0] == '~')
		{
			found = true;
			currentLabel = JumpToLabel(labels[label]->name.c_str());
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
					line = labels[label]->lines.size() - 1;
				}
				cmd = labels[label]->lines[line]->commands.size() - 1;
			}
		}
	}

}

void CutsceneManager::JumpForward()
{
	for (unsigned int i = 0; i < labels.size(); i++)
	{
		for (unsigned int j = 0; j < labels[i]->lines.size(); j++)
		{
			for (unsigned int k = 0; k < labels[i]->lines[j]->commands.size(); k++)
			{
				if (labels[i]->lines[j]->commands[k][0] == '~')
				{
					currentLabel = JumpToLabel(labels[i]->name.c_str());
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
		if (labels[i]->name == newLabelName)
		{
			labelIndex = i;
			return labels[i];
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
			watchingCutscene = false;

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
	SceneData* newData = new SceneData();
	newData->labelIndex = labelIndex;
	newData->labelName = currentLabel->name;
	newData->lineIndex = lineIndex;
	newData->lineText = currentLabel->lines[lineIndex]->text;
	newData->commandIndex = commandIndex;

	gosubStack.push_back(newData);
}

SceneData* CutsceneManager::PopSceneDataFromStack()
{
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

	if (currentLabel != nullptr)
	{
		//TODO: keep track of other information such as speaker name, etc.
		backlog.push(currentText);

		if (backlog.size() > backlogMaxSize)
		{
			backlog.pop();
		}

		lineIndex++;	
		currentText = "";
		textbox->text->SetText(currentText);

		if (lineIndex >= currentLabel->lines.size())
		{
			watchingCutscene = false;
			//TODO: Maybe instead of ending the cutscene,
			// go to the next label in sequence?
		}
		else
		{
			commandIndex = -1;
			letterIndex = 0;
			isCarryingOutCommands = true;
			isReadingNextLine = true;
			//textbox->isReading = false;

			currentColor = namesToColors[currentLabel->lines[lineIndex]->speaker];

			// If speaker of this line is same as last, instantly show it
			if (textbox->speaker->txt == currentLabel->lines[lineIndex]->speaker)
				textbox->speaker->SetText(currentLabel->lines[lineIndex]->speaker);
			else
				textbox->speaker->SetText("");
		}
	}	
}

void CutsceneManager::Update()
{
	//int lettersPerFrame = 2;
	float delay = 10.0f;

	const Uint8* input = SDL_GetKeyboardState(NULL);

	if (input[autoButton] && inputTimer.HasElapsed())
	{
		automaticallyRead = !automaticallyRead;
		inputTimer.Start(inputTimeToWait);
	}

	//TODO: Make this more customizable
	//TODO: Add a delay between letters as well.
	//TODO: Try to estimate the length of text to match human reading speed
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
		
	//nextLetterTimer.Start(lettersPerFrame * delay);

	//TODO: Fix this, it no longer works properly with the corrected dt
	timer += (float)game->dt;

	if (waitingForButton && timer > delay)
	{
		timer -= delay;

		//TODO: We want to get the mouse/keyboard input here

		if (inputTimer.HasElapsed())
		{
			if (input[SDL_SCANCODE_UP] || input[SDL_SCANCODE_W])
			{
				images[activeButtons[buttonIndex]]->GetSprite()->color = { 255, 255, 255, 255 };
				buttonIndex--;
				if (buttonIndex < 0)
					buttonIndex = activeButtons.size() - 1;
				images[activeButtons[buttonIndex]]->GetSprite()->color = { 255, 255, 0, 255 };
				inputTimer.Start(inputTimeToWait);
			}
			else if (input[SDL_SCANCODE_DOWN] || input[SDL_SCANCODE_S])
			{
				images[activeButtons[buttonIndex]]->GetSprite()->color = { 255, 255, 255, 255 };
				buttonIndex++;
				if (buttonIndex >= activeButtons.size())
					buttonIndex = 0;
				images[activeButtons[buttonIndex]]->GetSprite()->color = { 255, 255, 0, 255 };
				inputTimer.Start(inputTimeToWait);
			}
			else if (input[SDL_SCANCODE_SPACE])
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
				commands.ClearSprite({ "", std::to_string(choiceQuestionNumber) });
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

	textbox->isReading = (timer > 0);

	if (input[skipButton])
		delay = 0.0f;

	while (timer > delay)
	{
		timer -= delay;

		if (isCarryingOutCommands)
		{
			if (commandIndex < 0)
				commandIndex = 0;

			if (lineIndex < 0)
				lineIndex = 0;

			if (commandIndex >= 0 && commandIndex < currentLabel->lines[lineIndex]->commands.size())
			{				
				commands.ExecuteCommand(currentLabel->lines[lineIndex]->commands[commandIndex]);
				commandIndex++;
			}
			else
			{
				isCarryingOutCommands = false;
			}
		}
		else if (isReadingNextLine)
		{
			SceneLine* line = currentLabel->lines[lineIndex];
			if (line->text[letterIndex] != '[')
			{
				currentText += line->text[letterIndex];
				textbox->UpdateText(currentText, currentColor);
			}
			else // Handle special conditions here, like inserting variables into the text
			{
				letterIndex++;

				int varNameIndex = letterIndex;
				// Get everything until the next ] symbol
				std::string word = ParseWord(line->text, ']', letterIndex);
				
				// at this point we have the $variablename
				// so we need to check the first character to get the type

				if (word.length() != 0)
				{
					varNameIndex++;
					std::string variableName = ParseWord(line->text, ']', varNameIndex);
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

					//currentText += variableValue;

					//TODO: Make the word appear one letter at a time
					int valueIndex = 0;
					while (valueIndex < variableValue.length())
					{
						currentText += variableValue[valueIndex];
						textbox->UpdateText(currentText, currentColor);
						valueIndex++;
					};

					letterIndex--;

				}
			}

			if (currentText.length() == 1)
			{
				textbox->speaker->SetText(currentLabel->lines[lineIndex]->speaker);
			}

			//nextLetterTimer.Start(lettersPerFrame * delay);
			letterIndex++;

			if (letterIndex >= line->text.length())
			{
				isReadingNextLine = false;
				//game->player->cutsceneInputTimer.Start(100);

				if (automaticallyRead)
					autoReaderTimer.Start(autoTimeToWait[autoTimeIndex]);
			
				return;
			}
		}
		else if (delay == 0.0f)
		{
			break;
		}
	}	
}



void CutsceneManager::SaveGame()
{
	std::ofstream fout;

	// 1. Save the scene data (label, line, command)
	//TODO: Make sure to save the gosub stack as well
	fout.open("saves/test.sav");
	
	fout << currentLabel->name << std::endl;
	fout << "text" << std::endl;
	//fout << currentLabel->lines[lineIndex]->text << std::endl;
	fout << labelIndex << std::endl;
	fout << lineIndex << std::endl;
	fout << commandIndex << std::endl;

	fout.close();

	// 2a. Save global variables
	fout.open("saves/globals.sav");

	for (auto const& var : commands.stringVariables)
	{
		if (var.first >= globalStart)
		{
			fout << var.first  // key
				<< " "
				<< var.second  // value 
				<< std::endl;
		}
	}

	for (auto const& var : commands.numberVariables)
	{
		if (var.first >= globalStart)
		{
			fout << var.first  // key
				<< " "
				<< var.second  // value 
				<< std::endl;
		}
	}

	fout.close();

	// 2. Save string variables (keys and values)
	fout.open("saves/strvars.sav");
	for (auto const& var : commands.stringVariables)
	{
		fout << var.first  // key
			<< " "
			<< var.second  // value 
			<< std::endl;
	}
	fout.close();

	// 3. Save number variables (keys and values)
	fout.open("saves/numvars.sav");
	for (auto const& var : commands.numberVariables)
	{
		fout << var.first  // key
			<< " "
			<< var.second  // value 
			<< std::endl;
	}
	fout.close();

	// 4. Save string aliases (keys and values)
	fout.open("saves/stralias.sav");
	for (auto const& var : commands.stralias)
	{
		fout << var.first  // key
			<< " "
			<< var.second  // value 
			<< std::endl;
	}
	fout.close();

	// 5. Save number aliases (keys and values)
	fout.open("saves/numalias.sav");
	for (auto const& var : commands.numalias)
	{
		fout << var.first  // key
			<< " "
			<< var.second  // value 
			<< std::endl;
	}
	fout.close();

	// 6. Save object information (sprite number, filepath/text, position, rotation, color, etc.)
	fout.open("saves/objects.sav");
	for (auto const& var : images)
	{
		Entity* entity = var.second;

		if (var.second != nullptr)
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
				<< std::endl;
		}
	}
	fout.close();

	// 7. Save other important things (random seed, music volume, textbox color, etc.)
	fout.open("saves/textcolors.sav");
	for (auto const& var : namesToColors)
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
	fout.close();

}

void CutsceneManager::LoadGame()
{
	std::ifstream fin;
	std::string data = "";

	fin.open("saves/test.sav");
	for (std::string line; std::getline(fin, line); )
	{
		data += line + "\n";
	}
	fin.close();

	std::stringstream ss(data);
	std::istream_iterator<std::string> begin(ss);
	std::istream_iterator<std::string> end;

	std::vector<std::string> saveData(begin, end);
	std::copy(saveData.begin(), saveData.end(), std::ostream_iterator<std::string>(std::cout, "\n"));

	// These aren't actually stored in the cutscene manager,
	// but we might want to use them to more accurately find the load point
	std::string labelName = saveData[0];
	std::string lineText = saveData[1];
	
	labelIndex = std::stoi(saveData[2]);
	lineIndex = std::stoi(saveData[3]) - 1;
	commandIndex = std::stoi(saveData[4]);

	currentLabel = JumpToLabel(labelName.c_str());
	if (currentLabel == nullptr)
	{
		if (labelIndex < labels.size())
			currentLabel = labels[labelIndex];
		else
			std::cout << "ERROR: Could not find label " << labelName << std::endl;
	}

	// 2. Load string variables (keys and values)
	fin.open("saves/strvars.sav");

	fin.close();

	// 3. Load number variables (keys and values)
	fin.open("saves/numvars.sav");

	fin.close();

	// 4. Load string aliases (keys and values)
	fin.open("saves/stralias.sav");

	fin.close();

	// 5. Load number aliases (keys and values)
	fin.open("saves/numalias.sav");

	fin.close();

	// 6. Load object information (sprite number, filepath/text, position, rotation, color, etc.)
	fin.open("saves/objects.sav");

	fin.close();

	// 7. Load other important things (random seed, music volume, textbox color, etc.)
	fin.open("saves/textcolors.sav");
	data = "";
	for (std::string line; std::getline(fin, line); )
	{
		data += line + "\n";
	}
	fin.close();

}