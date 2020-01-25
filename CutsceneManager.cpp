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

	unsigned int index = 0;

	do
	{
		SceneLabel* newLabel = new SceneLabel;

		// Get label name
		index++; // begin with a *
		newLabel->name = ParseWord(data, '*', index);
		//cout << "Label name: " + newLabel->name;

		std::vector<std::string> tempCommands;

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
				for (unsigned int i = 0; i < tempCommands.size(); i++)
				{
					//std::cout << tempCommands[i] << std::endl;
					tempLine->commands.emplace_back(tempCommands[i]);
				}

				// add the line
				newLabel->lines.emplace_back(tempLine);
				tempCommands.clear();
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
				tempCommands.emplace_back(commandLine);
			}
		}

		// when we encounter a new label, add this one to the list
		labels.emplace_back(newLabel);

	} while (index < data.length());
}

std::string CutsceneManager::ParseWord(std::string text, char limit, unsigned int& index)
{
	std::string word = "";

	if (index >= text.length())
		return word;

	while (text[index] != limit)
	{
		word += text[index];
		index++;

		if (index >= text.length())
		{
			std::cout << "ERROR: Parsing word, index out of range: " + word << std::endl;
			break;
		}
	}

	index++; // move past the space/newline
	return word;
}

void CutsceneManager::Render(Renderer * renderer)
{
	textbox->Render(renderer);
}

SceneLabel* CutsceneManager::JumpToLabel(std::string newLabelName)
{
	for (unsigned int i = 0; i < labels.size(); i++)
	{
		if (labels[i]->name == newLabelName)
		{
			return labels[i];
		}
	}
	return nullptr;
}

void CutsceneManager::PlayCutscene(std::string labelName)
{
	if (labelName != "" && labelName != "null")
	{
		game->watchingCutscene = true;

		currentLabel = JumpToLabel(labelName);

		// if failed to load label, exit cutscenes
		if (currentLabel == nullptr)
			game->watchingCutscene = false;

		lineIndex = -1;

		ReadNextLine();
	}	
}

void CutsceneManager::ReadNextLine()
{
	if (currentLabel != nullptr)
	{
		lineIndex++;	
		currentText = "";

		if (lineIndex >= currentLabel->lines.size())
		{
			game->watchingCutscene = false;
		}
		else
		{
			commandIndex = 0;
			letterIndex = 0;
			isCarryingOutCommands = true;
			isReadingNextLine = true;
			textbox->speaker->SetText(currentLabel->lines[lineIndex]->speaker);
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

	SceneLine* line = currentLabel->lines[lineIndex];

	while (timer > delay)
	{
		timer -= delay;

		if (isCarryingOutCommands)
		{
			if (commandIndex >= 0 && commandIndex < line->commands.size())
			{				
				commands.ExecuteCommand(line->commands[commandIndex]);
				commandIndex++;
			}
			else
			{
				isCarryingOutCommands = false;
			}
		}
		else if (isReadingNextLine)
		{
			currentText += line->text[letterIndex];
			textbox->text->SetTextWrapped(currentText, textbox->boxWidth);
			textbox->text->GetSprite()->keepScaleRelativeToCamera = true;
			//TODO: If we want to modify the textbox's text shader, do so here
			//textbox->text->GetSprite()->SetShader(game->renderer->shaders["fade-in-out"]);

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