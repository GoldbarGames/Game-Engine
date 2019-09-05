#include "CutsceneManager.h"
#include "Renderer.h"
#include "Game.h"
#include "globals.h"

CutsceneManager::CutsceneManager(Game& g)
{
	game = &g;
	textbox = new Textbox(g.spriteManager, g.renderer);

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
	for (int i = 0; i < labels.size(); i++)
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
				for (int i = 0; i < tempCommands.size(); i++)
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

std::string CutsceneManager::ParseWord(std::string text, char limit, int& index)
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
	for (int i = 0; i < labels.size(); i++)
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
	game->watchingCutscene = true;

	currentLabel = JumpToLabel(labelName);

	lineIndex = 0;

	textbox->text->SetTextWrapped(currentLabel->lines[lineIndex]->text, textbox->boxWidth * SCALE);
	textbox->speaker->SetText(currentLabel->lines[lineIndex]->speaker);
}

void CutsceneManager::ReadNextLine()
{
	if (currentLabel != nullptr)
	{
		lineIndex++;

		if (lineIndex >= currentLabel->lines.size())
		{
			game->watchingCutscene = false;
		}
		else
		{
			textbox->text->SetTextWrapped(currentLabel->lines[lineIndex]->text, textbox->boxWidth * SCALE);
			textbox->speaker->SetText(currentLabel->lines[lineIndex]->speaker);
		}
	}	
}