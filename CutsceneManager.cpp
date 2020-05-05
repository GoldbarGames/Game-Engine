#include "CutsceneManager.h"
#include "Renderer.h"
#include "Game.h"
#include "globals.h"

CutsceneManager::CutsceneManager(Game& g)
{
	game = &g;
	textbox = new Textbox(g.spriteManager, g.renderer);

	commands.manager = this;

	nextLetterTimer.Start(1);

	std::ifstream fin;
	std::string directory = "data/" + language + "/cutscenes.txt";

	fin.open(directory);

	if (fin.is_open())
	{
		data = "";
		for (std::string line; std::getline(fin, line); )
		{
			data += line;
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
			else if (data[index] == ';')
			{
				//TODO: implement comments
			}
			else
			{
				std::string commandLine = "";

				// read until we hit the end of the line
				while (data[index] != ';')
				{
					std::string commandWord = ParseWord(data, ' ', index);
					commandLine += commandWord + " ";
					if (index >= data.length())
					{
						std::cout << "Error on line: " + commandWord;
					}
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

	//nextLetterTimer.Start(lettersPerFrame * delay);

	//TODO: Fix this, it no longer works properly with the corrected dt
	timer += (float)game->dt;

	if (waitingForButton && timer > delay)
	{
		timer -= delay;

		//TODO: We want to get the mouse/keyboard input here
		const Uint8* input = SDL_GetKeyboardState(NULL);

		if (input[SDL_SCANCODE_UP] || input[SDL_SCANCODE_W])
		{
			images[activeButtons[buttonIndex]]->GetSprite()->color = { 255, 255, 255, 255 };
			buttonIndex--;
			if (buttonIndex < 0)
				buttonIndex = activeButtons.size() - 1;
			images[activeButtons[buttonIndex]]->GetSprite()->color = { 255, 255, 0, 255 };
		}
		else if (input[SDL_SCANCODE_DOWN] || input[SDL_SCANCODE_S])
		{
			images[activeButtons[buttonIndex]]->GetSprite()->color = { 255, 255, 255, 255 };
			buttonIndex++;
			if (buttonIndex >= activeButtons.size())
				buttonIndex = 0;
			images[activeButtons[buttonIndex]]->GetSprite()->color = { 255, 255, 0, 255 };
		}
		else if (input[SDL_SCANCODE_SPACE])
		{
			// Return the result in the specified variable and resume reading
			unsigned int chosenSprite = activeButtons[buttonIndex];
			commands.numberVariables[buttonResult] = spriteButtons[chosenSprite];
			waitingForButton = false;
			isCarryingOutCommands = true;
			isReadingNextLine = true;
			activeButtons.clear();
			textbox->isReading = true;
		}

		return;
	}

	textbox->isReading = (timer > 0);

	while (timer > delay)
	{
		timer -= delay;

		if (isCarryingOutCommands)
		{
			if (commandIndex < 0)
				commandIndex = 0;

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
				textbox->UpdateText(currentText);
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
						textbox->UpdateText(currentText);
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
				return;
			}
		}
	}
	
}