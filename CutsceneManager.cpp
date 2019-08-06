#include "CutsceneManager.h"
using namespace std;

void CutsceneManager::ParseScene(string data)
{
	for (int i = 0; i < labels.size(); i++)
		delete labels[i];
	labels.clear();

	int index = 0;

	std::stringstream stream(data);

	do
	{
		SceneLabel* newLabel = new SceneLabel;

		// Get label name
		index++; // begin with a *
		newLabel->name = ParseWord(data, '*', index);
		//cout << "Label name: " + newLabel->name;

		std::vector<string> tempCommands;

		// Until end of file or new label...
		while (index < data.length() && data[index] != '*')
		{
			// If a `, we have a text line (otherwise, command)
			if (data[index] == '`')
			{
				index++;

				string newText = "";
				string newName = "";

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

				cout << newText << endl;

				// add all commands for this line
				SceneLine* tempLine = new SceneLine(newText, newName);
				for (int i = 0; i < tempCommands.size(); i++)
				{
					cout << tempCommands[i] << endl;
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
				string commandLine = "";

				// read until we hit the end of the line
				while (data[index] != ';')
				{
					string commandWord = ParseWord(data, ' ', index);
					commandLine += commandWord + " ";
					if (index >= data.length())
					{
						cout << "Error on line: " + commandWord;
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

string CutsceneManager::ParseWord(string text, char limit, int& index)
{
	string word = "";

	if (index >= text.length())
		return word;

	while (text[index] != limit)
	{
		word += text[index];
		index++;

		if (index >= text.length())
		{
			cout << "ERROR: Parsing word, index out of range: " + word << endl;
			break;
		}
	}

	index++; // move past the space/newline
	return word;
}

