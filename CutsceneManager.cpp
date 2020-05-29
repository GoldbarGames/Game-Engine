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

	LoadGlobalVariables();

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

// Check input after textbox line has been fully read
void CutsceneManager::CheckKeys()
{
	const Uint8* input = SDL_GetKeyboardState(NULL);

	if (useMouseControls)
	{
		int mouseX, mouseY = 0;
		const Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);

		if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
		{
			ReadNextLine();
		}
		else if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
		{
			commandIndex--;
			commands.GoSubroutine({ "", commands.buttonLabels[(unsigned int)SDL_SCANCODE_ESCAPE] });
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
					textbox->speaker->SetText(currentLabel->lines[lineIndex]->speaker, currentColor);
					textbox->text->SetText(currentText, currentColor);
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
			ReadNextLine();
		}
		else if (input[SDL_SCANCODE_UP])
		{
			readingBacklog = true;
			backlogIndex = backlog.size() - 1;
			ReadBacklog();
			inputTimer.Start(inputTimeToWait);
		}
#if _DEBUG
		else if (input[SDL_SCANCODE_S]) // save game
		{
			SaveGame();
		}
		else if (input[SDL_SCANCODE_L]) // load game
		{
			LoadGame();
			ReadNextLine();
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
				if (input[button.first]) //TODO: Also check if button is active
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

				if (commandLine != "" && commandLine != " ")
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
		//TODO: Make sure to save the backlog when we save the game
		if (lineIndex >= 0 && labelIndex >= 0)
			backlog.push_back( BacklogData(labelIndex, lineIndex, currentText.c_str()) );

		if (backlog.size() > backlogMaxSize)
			backlog.erase(backlog.begin());
		
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
				textbox->speaker->SetText(currentLabel->lines[lineIndex]->speaker, currentColor);
			else
				textbox->speaker->SetText("");
		}
	}	
}

void CutsceneManager::ReadBacklog()
{	
	//Color color = namesToColors[label->lines[lineIndex]->speaker];
	if (backlogIndex < backlog.size())
	{
		if (backlog[backlogIndex].labelIndex < labels.size())
		{
			SceneLabel* label = labels[backlog[backlogIndex].labelIndex];
			if (label != nullptr && backlog[backlogIndex].lineIndex < label->lines.size())
			{
				SceneLine* line = label->lines[backlog[backlogIndex].lineIndex];
				textbox->speaker->SetText(line->speaker, backlogColor);
				textbox->text->SetText(line->text, backlogColor);
			}
		}	

		textbox->text->SetText(backlog[backlogIndex].text, backlogColor);
	}
}

void CutsceneManager::Update()
{
	for (auto const& [key, image] : images)
	{
		if (image != nullptr)
			image->Update(*game);
	}

	//int lettersPerFrame = 2;
	float delay = 10.0f;

	const Uint8* input = SDL_GetKeyboardState(NULL);

	if (input[autoButton] && inputTimer.HasElapsed())
	{
		automaticallyRead = !automaticallyRead;
		inputTimer.Start(inputTimeToWait);
	}

	//TODO: Disable all of this if the keyboard controls are disabled
	// And also allow mouse control alternatives

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
				textbox->speaker->SetText(currentLabel->lines[lineIndex]->speaker, currentColor);
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

std::vector<string> CutsceneManager::GetVectorOfStringsFromFile(const char* filepath)
{
	std::ifstream fin;
	std::string data = "";

	fin.open(filepath);
	for (std::string line; std::getline(fin, line); )
	{
		data += line + "\n";
	}
	fin.close();

	std::stringstream ss(data);
	std::istream_iterator<std::string> begin(ss);
	std::istream_iterator<std::string> end;

	std::vector<std::string> globalData(begin, end);
	std::copy(globalData.begin(), globalData.end(), std::ostream_iterator<std::string>(std::cout, "\n"));

	return globalData;
}

void CutsceneManager::LoadGlobalVariables()
{
	std::vector<std::string> globalData = GetVectorOfStringsFromFile("saves/globals-num.dat");

	for (int i = 0; i < globalData.size(); i += 2)
	{
		commands.SetNumberVariable({ "", globalData[i], globalData[i+1] });
	}

	globalData = GetVectorOfStringsFromFile("saves/globals-str.dat");

	for (int i = 0; i < globalData.size(); i += 2)
	{
		commands.SetStringVariable({ "", globalData[i], globalData[i + 1] });
	}
}

void CutsceneManager::SaveGlobalVariable(unsigned int key, const std::string& value)
{
	// 1. Load the global variables into a data structure from the file
	std::ifstream fin;
	std::vector<std::string> globalData;

	fin.open("saves/globals-str.dat");
	for (std::string line; std::getline(fin, line); )
	{
		globalData.push_back(line + "\n");
	}
	fin.close();

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

	// 4. Save the DS to the file
	std::ofstream fout;
	fout.open("saves/globals-str.dat");

	for (int i = 0; i < globalData.size(); i++)
	{
		fout << globalData[i];
	}

	fout.close();
}

void CutsceneManager::SaveGlobalVariable(unsigned int key, unsigned int value)
{
	// 1. Load the global variables into a data structure from the file
	std::ifstream fin;
	std::vector<std::string> globalData;

	fin.open("saves/globals-num.dat");
	for (std::string line; std::getline(fin, line); )
	{
		globalData.push_back(line + "\n");
	}
	fin.close();

	bool foundData = false;
	int index = 0;
	for (int i = 0; i < globalData.size(); i++)
	{
		index = 0;
		if (ParseWord(globalData[i], ' ', index) == std::to_string(key))
		{
			// 2. If the global variable is already in the DS, update its value in the DS
			globalData[i] = std::to_string(key) + " " + std::to_string(value) + "\n";
			foundData = true;
			break;
		}
	}

	if (!foundData)
	{
		// 3. Else, add the global variable and its value to the DS
		globalData.push_back(std::to_string(key) + " " + std::to_string(value) + "\n");
	}

	// 4. Save the DS to the file
	std::ofstream fout;
	fout.open("saves/globals-num.dat");

	for (int i = 0; i < globalData.size(); i++)
	{
		fout << globalData[i];
	}

	fout.close();
}



void CutsceneManager::SaveGame()
{
	std::ofstream fout;

	// 1. Save the story data (label, line, command)
	// (the information necessary for us to find our place in the story)

	//TODO: Make sure to save the gosub stack as well
	fout.open("saves/file1.sav");

	std::map<SaveSections, std::string> sections = {
		{ SaveSections::CONFIG_OPTIONS, "@ CONFIG_OPTIONS"},
		{ SaveSections::STORY_DATA, "@ STORY_DATA"},
		{ SaveSections::GLOBAL_STRINGS, "@ GLOBAL_STRINGS"},
		{ SaveSections::GLOBAL_NUMBERS, "@ GLOBAL_NUMBERS"},
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
		case SaveSections::GLOBAL_STRINGS:
			// 2a. Save global variables
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
			break;
		case SaveSections::GLOBAL_NUMBERS:
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
			break;
		case SaveSections::ALIAS_STRINGS:
			// 4. Save string aliases (keys and values)
			for (auto const& var : commands.stralias)
			{
				fout << var.first  // key
					<< " "
					<< var.second  // value 
					<< std::endl;
			}
			break;
		case SaveSections::ALIAS_NUMBERS:
			// 5. Save number aliases (keys and values)
			for (auto const& var : commands.numalias)
			{
				fout << var.first  // key
					<< " "
					<< var.second  // value 
					<< std::endl;
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
			break;
		default:
			break;
		}
	}

	fout.close();

}

void CutsceneManager::LoadGame()
{
	std::ifstream fin;
	//std::string data = "";
	std::vector<std::string> dataLines;

	fin.open("saves/file1.sav");
	for (std::string line; std::getline(fin, line); )
	{
		//data += line + "\n";
		dataLines.push_back(line);
	}
	fin.close();

	// While we have lines to examine
	// Check the first character for an @ symbol
	// If there is one, change the current section
	// Otherwise, interpret the line based on the current section

	std::map<std::string, SaveSections> sections = {
		{ "@ CONFIG_OPTIONS", SaveSections::CONFIG_OPTIONS},
		{ "@ STORY_DATA", SaveSections::STORY_DATA},
		{ "@ GLOBAL_STRINGS", SaveSections::GLOBAL_STRINGS},
		{ "@ GLOBAL_NUMBERS", SaveSections::GLOBAL_NUMBERS},
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

	while (index < dataLines.size())
	{
		std::stringstream ss(dataLines[index]);
		std::istream_iterator<std::string> begin(ss);
		std::istream_iterator<std::string> end;

		std::vector<std::string> lineParams(begin, end);
		std::copy(lineParams.begin(), lineParams.end(), std::ostream_iterator<std::string>(std::cout, " "));

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

				currentLabel = JumpToLabel(labelName.c_str());
				if (currentLabel == nullptr)
				{
					if (labelIndex < labels.size())
						currentLabel = labels[labelIndex];
					else
						std::cout << "ERROR: Could not find label " << labelName << std::endl;
				}
				break;
			case SaveSections::ALIAS_STRINGS:
				commands.stralias[lineParams[0]] = lineParams[1];
				break;
			case SaveSections::ALIAS_NUMBERS:
				commands.numalias[lineParams[0]] = std::stoi(lineParams[1]);
				break;
			case SaveSections::GLOBAL_STRINGS:
			case SaveSections::LOCAL_STRINGS:
				commands.stringVariables[std::stoi(lineParams[0])] = lineParams[1];
				break;
			case SaveSections::GLOBAL_NUMBERS:
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
					images[std::stoi(lineParams[1])]->rotation = glm::vec3(
						std::stoi(lineParams[5]), 
						std::stoi(lineParams[6]), 
						std::stoi(lineParams[7]));
				}
				break;
			case SaveSections::NAMES_TO_COLORS:
				if (lineParams[0] == "_")
				{
					lineParams[0] = "";
				}

				namesToColors[lineParams[0]] = {
				std::stoi(lineParams[1]),
				std::stoi(lineParams[2]),
				std::stoi(lineParams[3]),
				std::stoi(lineParams[4])
				};

				break;
			case SaveSections::OTHER_STUFF:
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
