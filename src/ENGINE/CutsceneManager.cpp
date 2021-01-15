#include "CutsceneManager.h"
#include "Renderer.h"
#include "Game.h"
#include "globals.h"
#include <iterator>
#include "Logger.h"
#include "SoundManager.h"
#include "Textbox.h"
#include "DebugScreen.h"
#include "ParticleSystem.h"
#include "CutsceneFunctions.h"

CutsceneManager::CutsceneManager()
{

}

void CutsceneManager::Init(Game& g)
{
	game = &g;

	for (auto& [key, val] : tags)
	{
		if (val != nullptr)
			delete_it(val);
	}

	if (textbox != nullptr)
		delete_it(textbox);

	textbox = neww Textbox(g.spriteManager, g.renderer);

	commands.manager = this;
	inputTimeToWait = 100;

	nextLetterTimer.Start(1);
	autoReaderTimer.Start(1);
	inputTimer.Start(1);

	namesToColors[""] = currentColor;

	tags["b"] = neww TextTag();
	tags["i"] = neww TextTag();
	tags["bi"] = neww TextTag();
	tags["s"] = neww TextTag();

	ReadCutsceneFile();

	// This must ALWAYS come after reading the .define file
	LoadGlobalVariables();
}

CutsceneManager::~CutsceneManager()
{
	for (int i = 0; i < labels.size(); i++)
	{
		//delete labels[i];
	}

	for (int i = 0; i < gosubStack.size(); i++)
	{
		delete_it(gosubStack[i]);
	}

	for (int i = 0; i < backlog.size(); i++)
	{
		delete_it(backlog[i]);
	}

	for (auto& [key, val] : images)
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

void CutsceneManager::ReadCutsceneFile()
{
	std::ifstream fin;
	std::string line = "";
	std::string directory = "data/cutscenes/" + language + "/";
	std::string filepath = directory + game->currentGame + ".txt";
	std::string defineFilePath = directory + game->currentGame + ".define";

	try
	{
		// First try to read a separate define file
		std::cout << "Attempting to open " + defineFilePath << std::endl;
		fin.open(defineFilePath);
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
			ParseCutsceneFile();
		}
		else
		{
			std::string errorMessage = "ERROR: Failed to open " + defineFilePath;
			game->logger.Log(errorMessage);
			//throw std::exception(errorMessage.c_str());
		}

		// TODO: Probably want to optimize this at some point
		std::vector<std::string> filesToRead;
		filesToRead.emplace_back(filepath);

		for (int i = 0; i < commands.includeFilepaths.size(); i++)
		{
			filesToRead.emplace_back(commands.includeFilepaths[i]);
		}

		// Add together the contents of all the files into one big string
		data = "";
		for (const auto& file : filesToRead)
		{
			std::cout << "Attempting to open " + file << std::endl;
			fin.open(file);
			if (fin.is_open())
			{
				for (line; std::getline(fin, line); )
				{
					if (line[0] == '*')
						data += line + "*";
					else
						data += line + " ;";
				}
				fin.close();
			}
			else
			{
				// TODO: We should ultimately throw an error if it fails to read a file
				// that is critical for the game to work properly, so we need a way
				// to know which files a game should or should not have.

				std::string errorMessage = "ERROR: Failed to open " + file;
				game->logger.Log(errorMessage);
				//throw std::exception(errorMessage.c_str());
			}
		}
	}
	catch (std::exception ex)
	{
		game->logger.Log(ex.what());

		// TODO: Display error message on screen for players to send to the developer
	}
	
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
		if (!readingBacklog)
		{
			// Note: In order to clear the mouse state, we check the SDL event in the main game loop
			if (mouseState & (~previousMouseState) & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				previousMouseState = mouseState;
				clickedMidPage = true;
			}
		}
		else
		{

		}
	}

	if (useKeyboardControls)
	{
		if (!readingBacklog && !waitingForButton)
		{
			if ( ((input[readButton] || input[readButton2]) && inputTimer.HasElapsed()) 
				|| isSkipping
				|| (automaticallyRead && autoReaderTimer.HasElapsed()))
			{
				clickedMidPage = true;
			}
		}
	}
}

void CutsceneManager::OpenBacklog()
{
	if (!readingBacklog)
	{
		readingBacklog = true;
		backlogIndex = backlog.size() - 1;
		previousText = currentText;
		beforeBacklogText = previousText;
		beforeBacklogSpeaker = textbox->speaker->txt;
		ReadBacklog();

		if (backlogIndex > 0)
		{
			CutsceneFunctions::LoadSprite({ "ld", "998", backlogBtnUp, std::to_string(backlogBtnUpX), std::to_string(backlogBtnUpY) }, commands);
		}
		else
		{
			CutsceneFunctions::ClearSprite({ "cl", "998", "1" }, commands);
		}

		CutsceneFunctions::LoadSprite({ "ld", "999", backlogBtnDown, std::to_string(backlogBtnDownX), std::to_string(backlogBtnDownY) }, commands);

	}
	else if (inputTimer.HasElapsed())
	{
		backlogIndex--;
		if (backlogIndex < 0)
			backlogIndex = 0;
		ReadBacklog();
		inputTimer.Start(inputTimeToWait);

		if (backlogIndex > 0)
		{
			CutsceneFunctions::LoadSprite({ "ld", "998", backlogBtnUp, std::to_string(backlogBtnUpX), std::to_string(backlogBtnUpY) }, commands);
		}
		else
		{
			CutsceneFunctions::ClearSprite({ "cl", "998", "1" }, commands);
		}

		CutsceneFunctions::LoadSprite({ "ld", "999", backlogBtnDown, std::to_string(backlogBtnDownX), std::to_string(backlogBtnDownY) }, commands);
	}
}

void CutsceneManager::CloseBacklog()
{
	if (readingBacklog && inputTimer.HasElapsed())
	{
		backlogIndex++;
		if (backlogIndex >= backlog.size())
		{
			readingBacklog = false;
			isReadingNextLine = true;
			textbox->speaker->SetText(beforeBacklogSpeaker, currentColor);
			textbox->text->SetText(beforeBacklogText);
			int newIndex = letterIndex + lines[currentLabel->lineStart + lineIndex].textStart;
			textbox->SetCursorPosition(data[newIndex + 1] != '@');

			CutsceneFunctions::ClearSprite({ "cl", "998", "1" }, commands);
			CutsceneFunctions::ClearSprite({ "cl", "999", "1" }, commands);
		}
		else
		{
			ReadBacklog();

			CutsceneFunctions::LoadSprite({ "ld", "998", backlogBtnUp, std::to_string(backlogBtnUpX), std::to_string(backlogBtnUpY) }, commands);
			CutsceneFunctions::LoadSprite({ "ld", "999", backlogBtnDown, std::to_string(backlogBtnDownX), std::to_string(backlogBtnDownY) }, commands);
		}
		inputTimer.Start(inputTimeToWait);
	}	
}

// Check input after textbox line has been fully read
void CutsceneManager::CheckKeys()
{
	const Uint8* input = SDL_GetKeyboardState(NULL);

	int mouseX, mouseY = 0;
	mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	if (useMouseControls && inputTimer.HasElapsed())
	{
		if (!readingBacklog && !atChoice)
		{
			// Note: In order to clear the mouse state, we check the SDL event in the main game loop
			if ( mouseState & (~previousMouseState) & SDL_BUTTON(SDL_BUTTON_LEFT) )
			{
				previousMouseState = mouseState;
				ReadNextLine();
			}
			else if ( (mouseState & (~previousMouseState) & SDL_BUTTON(SDL_BUTTON_RIGHT)) && rclickEnabled)
			{
				previousMouseState = mouseState;
				commandIndex--;
				CutsceneFunctions::GoSubroutine({ "", commands.buttonLabels[(unsigned int)SDL_SCANCODE_ESCAPE] }, commands);
			}
		}
		else // if we are reading the backlog, check to see if we clicked on its arrow buttons
		{
			mouseX *= Camera::MULTIPLIER;
			mouseY *= Camera::MULTIPLIER;

			SDL_Rect mouseRect;
			mouseRect.x = mouseX;
			mouseRect.y = mouseY;
			mouseRect.w = 1;
			mouseRect.h = 1;

			int hoveredButton = -1;
			bool clickedMouse = false;

			
			if (images[998] != nullptr)
			{
				// Up Arrow
				if (HasIntersection(images[998]->GetTopLeftBounds(), mouseRect))
				{
					images[998]->SetColor({ 128, 128, 128, 128 });
					hoveredButton = 998;
				}
				else
				{
					images[998]->SetColor({ 255, 255, 255, 255 });
				}
			}

			if (images[999] != nullptr)
			{
				// Down Arrow
				if (HasIntersection(images[999]->GetTopLeftBounds(), mouseRect))
				{
					images[999]->SetColor({ 128, 128, 128, 128 });
					hoveredButton = 999;
				}
				else
				{
					images[999]->SetColor({ 255, 255, 255, 255 });
				}
			}

			// TODO: Use a shader instead of changing color

			// TODO: Don't hardcode these numbers

			if (mouseState & (~previousMouseState) & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				previousMouseState = mouseState;
				clickedMouse = true;				
			}
			else if (mouseState & (~previousMouseState) & SDL_BUTTON(SDL_BUTTON_RIGHT))
			{
				previousMouseState = mouseState;

				// TODO: Maybe do something on right click?
			}

			if (hoveredButton > -1 && clickedMouse)
			{
				if (hoveredButton == 998) // Scroll up
				{
					OpenBacklog();
				}
				else if (hoveredButton == 999) // Scroll down
				{
					CloseBacklog();
				}
			}
		}
	}


	if (useKeyboardControls && inputTimer.HasElapsed())
	{
		if (readingBacklog)
		{
			if (input[SDL_SCANCODE_UP])
			{
				OpenBacklog();
			}
			else if (input[SDL_SCANCODE_DOWN])
			{
				CloseBacklog();
			}
		}
		else if (input[readButton] || input[readButton2] || isSkipping
			|| (automaticallyRead && autoReaderTimer.HasElapsed()))
		{
			ReadNextLine();
			inputTimer.Start(inputTimeToWait);
		}
		else if (input[SDL_SCANCODE_UP])
		{
			if (backlogEnabled)
			{
				if (backlog.size() > 0)
				{
					OpenBacklog();
				}
				inputTimer.Start(inputTimeToWait);
			}
		}
#if _DEBUG
		else if (game->freeCameraMode)
		{
			// do nothing
		}
		else if (input[SDL_SCANCODE_S]) // save game
		{
			SaveGame("file1.sav");
			inputTimer.Start(inputTimeToWait);
		}
		else if (input[SDL_SCANCODE_L]) // load game
		{
			LoadGame("file1.sav");
			ReadNextLine();
			isCarryingOutCommands = false;
			inputTimer.Start(inputTimeToWait);
		}
		else if (input[SDL_SCANCODE_P])
		{
			ReadCutsceneFile();
			ParseCutsceneFile();
			game->ResetLevel();
			inputTimer.Start(inputTimeToWait);
		}
		else if (input[SDL_SCANCODE_I]) // make checkpoint
		{

			SaveGame("checkpoint.sav");
			inputTimer.Start(inputTimeToWait);
			//checkpoint.labelIndex = labelIndex;
			//checkpoint.lineIndex = lineIndex;
		}
		else if (input[SDL_SCANCODE_O]) // load checkpoint
		{

			// Reload the script for any changes
			ReadCutsceneFile();
			ParseCutsceneFile();
			game->ResetLevel();

			// Jump to the checkpoint
			LoadGame("checkpoint.sav");
			ReadNextLine();
			isCarryingOutCommands = false;
			inputTimer.Start(inputTimeToWait);

			//labelIndex = checkpoint.labelIndex;
			//lineIndex = checkpoint.lineIndex;
			//commandIndex = 0;			
		}
		else if (input[SDL_SCANCODE_BACKSPACE])
		{
			//TODO: This is not perfect, it just breaks out of the cutscene and does not carry out commands
			EndCutscene();
			inputTimer.Start(inputTimeToWait);
		}
#endif
		else
		{
			for (auto const& button : commands.buttonLabels)
			{
				if (input[button.first] && commands.buttonLabelsActive[button.first])
				{
					commandIndex--;
					CutsceneFunctions::GoSubroutine({ button.second, button.second }, commands);
					inputTimer.Start(inputTimeToWait);
					break;
				}
			}
		}
	}

	
}

void CutsceneManager::ParseCutsceneFile()
{
	if (data.size() < 1)
	{
		game->logger.Log("No cutscene data to parse.");
		return;
	}

	//for (unsigned int i = 0; i < labels.size(); i++)
	//	delete labels[i];
	labels.clear();

	// TODO: Read these numbers in before anything else
	// (can pre-process the file to get exact counts)
	labels.reserve(300);
	lines.reserve(5000);
	cmdStart.reserve(20000);
	cmdEnd.reserve(20000);

	int index = 0;
	int commandPart1 = 0;
	int commandPart2 = 0;

	int commandSize = 0;
	int label1 = 0, label2 = 0, text1 = 0, text2 = 0, speaker1 = 0, speaker2 = 0, command1 = 0, command2 = 0;
	std::vector<int> commandsStart;
	std::vector<int> commandsEnd;

	data.erase(std::remove(data.begin(), data.end(), '\t'), data.end());

	Timer timer;
	timer.Start(10);

	//std::cout << "TIMER START " << timer.GetTicks() << std::endl;

	// Instead of copying the strings to the data objects,
	// we save integers and use them to index the data string

	do
	{
		SceneLabel newLabel;

		// Get label name
		index++; // begin with a *
		//TODO: This is broken on the very first label?
		//newLabel.name = ParseWord(data, '*', index);
		//newLabel.name = Trim(newLabel.name);

		label1 = index;
		while (data[index] != '*' && index < data.length())
		{
			index++;
		}
		
		if (index >= data.length())
			break;

		label2 = index;

		newLabel.nameStart = label1;
		newLabel.nameEnd = label2;
		index++;
		//std::cout << "Label name: " + GetLabelName(newLabel) << std::endl;

		commandsStart.clear();
		commandsEnd.clear();

		// Until end of file or new label...
		while (index < data.length() && data[index] != '*')
		{
			// If a `, we have a text line (otherwise, command)
			if (data[index] == '`')
			{
				index++;

				text1 = index;

				// deal with names, if any
				if (data[index] == ':')
				{
					index++; // skip :
					speaker1 = index;
					while (data[index] != ':')
					{
						index++;
					}
					speaker2 = index;
					index++; // skip :
					index++; // skip space
				}

				// deal with the text
				while (data[index] != '`')
				{					
					index++;
				}

				text2 = index;

				// std::cout << "TEXT: " << data.substr(text1, text2 - text1) << std::endl;

				// add all commands for this line
				SceneLine newLine(text1, text2, speaker1, speaker2);

				newLine.commandsStart = cmdStart.size();
				newLine.commandsSize = commandsStart.size();

				cmdStart.insert(cmdStart.end(), commandsStart.begin(), commandsStart.end());
				cmdEnd.insert(cmdEnd.end(), commandsEnd.begin(), commandsEnd.end());

				// add the line
				if (newLabel.lineStart < 0)
					newLabel.lineStart = lines.size();
				newLabel.lineSize++;
				lines.emplace_back(newLine);

				//std::cout << "TEXT: " << data.substr(text1, text2 - text1) << std::endl;

				commandsStart.clear();
				commandsEnd.clear();
				index++;
			}
			else if (data[index] == '@' || data[index] == ' ')
			{
				// Don't do anything special here, but if we didn't check for this, it'd be seen as a command.
				// Also, the only difference between @ and \ is that \ clears the text, @ does not.
				index++;
			}
			else // we have a command
			{
				command1 = index;

				// read until we hit the end of the line
				bool foundColon = (data[index] == ':');

				while (!(data[index] == ';' || data[index] == '`'))
				{
					if (index >= data.length())
					{
						std::cout << "Error on line: " + data.substr(command1, index - command1);
						break;
					}
					else
					{
						index++;
					}

					/*
					char delimit = ' ';
					bool ignoreColon = false;
					for (int i = 0; i < command.size(); i++)
					{
						if (command[i] == ':' && !ignoreColon)
						{
							delimit = ':';
							break;
						}
						else if (command[i] == '[')
						{
							ignoreColon = true;
						}
						else if (command[i] == ']')
						{
							ignoreColon = false;
						}
					}*/

					// TODO: Ignore : inside of [ ] or " "
					if (data[index] == ':')
						foundColon = true;
				}

				command2 = index - 1;

				// This is so that we can run commands within a line of text,
				// the commands must be followed by an @
				if (data[index-1] == '@')
				{
					index--;
				}

				index++;

				commandSize = command2 - command1;

				// Splits up the string if multiple commands are on one line
				if (foundColon)
				{
					// but don't split this up if we are evaluating an if-statement; split it later
					if (commandSize > 2 && data[command1] == 'i' && data[command1+1] == 'f')
					{
						if (commandSize > 0)
						{
							commandsStart.emplace_back(command1);
							commandsEnd.emplace_back(command2);
							//std::cout << commandLine << std::endl;
						}
					}
					else
					{
						//std::string test = data.substr(command1, command2 - command1);
						
						commandPart1 = command1;
						commandPart2 = command1;

						while (commandPart2 < command2)
						{
							commandPart2++;
							
							char c2 = data[commandPart2];

							if (data[commandPart2] == ':' || commandPart2 >= command2)
							{
								if (commandSize > 0)
								{									
									//std::string test2 = data.substr(commandPart1, commandPart2 - commandPart1);
									commandsStart.emplace_back(commandPart1);
									commandsEnd.emplace_back(commandPart2);
									commandPart2++; // go past the :
									//std::cout << "COMMAND: " << data.substr(commandPart1, commandPart2 - commandPart1) << std::endl;
								}
								commandPart1 = commandPart2 + 1; // the + 1 is to remove leading space
							}
						}
					}					
				}
				else
				{
					if (commandSize > 0)
					{
						commandsStart.emplace_back(command1);
						commandsEnd.emplace_back(command2);
						//std::cout << "COMMAND: " << data.substr(command1, command2 - command1) << std::endl;
					}
				}				
						
			}
		}

		// If we have commands but no line before a new label,
		// add an empty text to that line
		if (commandsEnd.size() > 0)
		{
			// add all commands for this line
			SceneLine newLine(0, 0, 0, 0);

			newLine.commandsStart = cmdStart.size();
			newLine.commandsSize = commandsStart.size();

			cmdStart.insert(cmdStart.end(), commandsStart.begin(), commandsStart.end());
			cmdEnd.insert(cmdEnd.end(), commandsEnd.begin(), commandsEnd.end());

			// add the line
			if (newLabel.lineStart < 0)
				newLabel.lineStart = lines.size();
			newLabel.lineSize++;
			lines.emplace_back(newLine);			

			commandsStart.clear();
			commandsEnd.clear();
		}

		// when we encounter a new label, add this one to the list
		if (newLabel.lineSize > 0)
		{
			labels.emplace_back(newLabel);
		}			

	} while (index < data.length());

	//std::cout << "TIMER END " << timer.GetTicks() << std::endl;

	ExecuteDefineBlock("define");
}



void CutsceneManager::ExecuteDefineBlock(const char* configName)
{
	int cmdIndex = 0;

	SceneLabel* configLabel = JumpToLabel(configName);

	if (configLabel != nullptr)
	{
		while (cmdIndex < lines[configLabel->lineStart].commandsSize)
		{
			commands.ExecuteCommand(GetCommand(lines[configLabel->lineStart], cmdIndex));
			cmdIndex++;
		}
	}
	else
	{
		game->logger.Log("No configuration label found!");
	}
}

const std::string CutsceneManager::GetLabelName(const SceneLabel& label) const
{
	return data.substr(label.nameStart, label.nameEnd - label.nameStart);
}

const std::string CutsceneManager::GetCommand(const SceneLine& line, int index) const
{
	int c1 = cmdStart[line.commandsStart + index];
	int c2 = cmdEnd[line.commandsStart + index];
	return data.substr(c1, c2 - c1);
}

const std::string CutsceneManager::GetLineText(const SceneLine& line) const
{
	return data.substr(line.textStart, line.textEnd - line.textStart);
}

const std::string CutsceneManager::GetLineSpeaker(const SceneLine& line) const
{
	return data.substr(line.speakerStart, line.speakerEnd - line.speakerStart);
}

void CutsceneManager::RenderTextbox(const Renderer& renderer)
{
	// Render the overlay above all sprites
	renderer.FadeOverlay(game->screenWidth, game->screenHeight);
	renderer.overlaySprite->Render(glm::vec3(0, 0, 0), renderer, renderer.overlayScale);

	// Render the textbox above everything
	if (GetLabelName(currentLabel) != "title")
		textbox->Render(renderer, game->screenWidth, game->screenHeight);

	if (!isCarryingOutCommands && !isReadingNextLine)
	{
		textbox->clickToContinue->Render(renderer);
	}
}

void CutsceneManager::Render(const Renderer& renderer)
{
	if (watchingCutscene && renderCutscene)
	{
		bool renderedTextbox = false;

		// Render every sprite in the cutscene, both below and above the textbox
		for (imageIterator = images.begin(); imageIterator != images.end(); imageIterator++)
		{
			if (imageIterator->first > textboxImageNumber && !renderedTextbox)
			{
				renderedTextbox = true;
				RenderTextbox(renderer);
			}

			if (imageIterator->second != nullptr)
				imageIterator->second->Render(renderer);
		}

		// If we did not yet render the textbox, render it at the end
		if (!renderedTextbox)
		{
			RenderTextbox(renderer);
		}
		
	}
	else // only draw the overlay, not text or box
	{
		renderer.FadeOverlay(game->screenWidth, game->screenHeight);
		renderer.overlaySprite->Render(glm::vec3(0, 0, 0), renderer, renderer.overlayScale);
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
		// 
		//&& cmd < cmdStart[lines[labels[label]].commandsStart + index];
		 // + labels[labelIndex].lineStart

		std::cout << "Label " << label << ", Line " << line << ", Cmd " << cmd << std::endl;

		if (label < labels.size() 
			&& line < labels[labelIndex].lineSize
			&& GetCommand(lines[labels[labelIndex].lineStart + line], cmd)[0] == '~')
		{
			found = true;
			currentLabel = JumpToLabel(GetLabelName(labels[label]));
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
					line = labels[label].lineSize - 1;
				}
				
				cmd = lines[labels[label].lineStart].commandsSize - 1;
			}
		}
	}

}

void CutsceneManager::JumpForward()
{
	for (unsigned int i = labelIndex; i < labels.size(); i++)
	{
		unsigned int j = 0;
		if (i == labelIndex)
		{
			j = lineIndex;
		}

		for (j; j < labels[i].lineSize; j++)
		{
			unsigned int k = 0;
			if (j == lineIndex)
			{
				k = commandIndex;
			}
			
			for (k; k < lines[labels[i].lineStart + j].commandsSize; k++)
			{
				std::string cmd = GetCommand(lines[labels[i].lineStart + j], k);

				if (cmd[0] == '~')
				{
					currentLabel = JumpToLabel(GetLabelName(labels[i]));
					labelIndex = i;
					lineIndex = j;
					commandIndex = k;
					return;
				}
			}
		}
	}
}

SceneLabel* CutsceneManager::JumpToLabel(const std::string& newLabelName)
{
	std::string newLabel = newLabelName;
	newLabel = Trim(newLabel);

	if (newLabel[0] == '*')
		newLabel = newLabel.substr(1, newLabel.size() - 1);

	for (unsigned int i = 0; i < labels.size(); i++)
	{
		if (GetLabelName(labels[i]) == newLabel)
		{
			labelIndex = i;

#if _DEBUG
			checkpoint.labelIndex = labelIndex;
			checkpoint.lineIndex = 0;
#endif

			return &labels[i];
		}
	}

	game->logger.Log("ERROR: Could not find cutscene label \"" + std::string(newLabel) + "\"");

	return nullptr;
}

void CutsceneManager::PlayCutscene(const char* labelName)
{
	if (labelName != "" && labelName != "null")
	{
		watchingCutscene = true;
		textbox->isReading = true;
		SceneLabel* newLabel = JumpToLabel(labelName);

		// if failed to load label, stay in current label
		if (newLabel == nullptr)
		{
			// if current label is null, just end the cutscene
			if (currentLabel == nullptr)
			{
				watchingCutscene = false;
			}
		}
		else
		{
			currentLabel = newLabel;
			lineIndex = -1;
			commandIndex = -1;
		}

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
	newData->labelName = GetLabelName(currentLabel);
	newData->lineIndex = lineIndex;
	newData->lineText = GetLineText(lines[lineIndex]);
	newData->commandIndex = commandIndex;

#if _DEBUG
	if (!isTravelling)
	{
		std::cout << "PUSH LABEL " << newData->labelName << std::endl;
	}
#endif


	gosubStack.push_back(newData);
}

bool CutsceneManager::PopSceneDataFromStack(SceneData& data)
{
	if (gosubStack.size() > 0)
	{
		data = *(gosubStack[gosubStack.size() - 1]);
		delete_it(gosubStack[gosubStack.size() - 1]);
		gosubStack.pop_back();

		labelIndex = data.labelIndex;
		lineIndex = data.lineIndex;
		commandIndex = data.commandIndex;
		letterIndex = 0;
		//currentText = data.lineText;

#if _DEBUG

		if (!isTravelling)
		{
			std::cout << "POP LABEL " << data.labelName << std::endl;
		}

#endif

		return true;
	}

	return false;
}

void CutsceneManager::ClearAllSprites()
{
	// Clear all normal sprites
	int num = commands.GetNumAlias("l");
	if (images[num] != nullptr)
		delete_it(images[num]);

	num = commands.GetNumAlias("c");
	if (images[num] != nullptr)
		delete_it(images[num]);

	num = commands.GetNumAlias("r");
	if (images[num] != nullptr)
		delete_it(images[num]);
}

void CutsceneManager::ReadNextLine()
{
	game->debugScreen->updatedLine = false;

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
		if (lineIndex >= 0 && lineIndex < currentLabel->lineSize)
		{
			seenLabelsToMostRecentLine[labelIndex] = lineIndex;
		}			

		// Only clear the text (and name) if we encounter a slash
		int newIndex = letterIndex + lines[currentLabel->lineStart + lineIndex].textStart;
		if (data[newIndex + 1] == '\\')
		{
			ClearPage();
		}
		else
		{
			textbox->SetCursorPosition(false);
		}

		lineIndex++;
		if (lineIndex >= currentLabel->lineSize)
		{			
			if (labelIndex < labels.size() - 1)
			{
				if (autoreturn)
				{
					EndCutscene();
				}
				else // go to the next label in sequence
				{
					std::cout << "NEXT LABEL " << GetLabelName(labels[labelIndex]) <<  std::endl;
					labelIndex++;
					currentLabel = JumpToLabel(GetLabelName(labels[labelIndex]));
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
			//textbox->speaker->SetText(GetLineSpeaker(lines[currentLabel->lineStart + lineIndex]), currentColor);

			if (autosave)
			{
				SaveGame("auto.sav");
			}
		}
	}	
}

void CutsceneManager::FlushCurrentColor(const std::string& speakerName)
{
	std::string sn = speakerName;
	if (speakerName == "")
	{
		sn = GetLineSpeaker(lines[currentLabel->lineStart + lineIndex]);
	}

	if (currentLabel != nullptr)
	{
		if (namesToColors.count(sn))
		{
			currentColor = namesToColors[sn];
		}
		else
		{
			currentColor = namesToColors[""];
		}
	}		
}

void CutsceneManager::ReadBacklog()
{		
	if (backlogIndex < backlog.size())
	{
		textbox->speaker->SetText(backlog[backlogIndex]->speaker, backlogColor);
		textbox->text->SetText(backlog[backlogIndex]->text, backlogColor);
		textbox->SetCursorPosition(true);
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
		if (lineIndex >= 0 && lineIndex < label->lineSize)
			return &lines[label->lineStart + lineIndex];		
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

	if (isTravelling)
	{
		static int prevLabelIndex = 0;

		if (labelIndex != prevLabelIndex)
		{
			prevLabelIndex = labelIndex;
			std::cout << "Label: " << GetLabelName(labels[labelIndex]) << std::endl;
		}

		if (GetLabelName(labels[labelIndex]) == endTravelLabel)
		{
			isTravelling = false;
			autoChoice = 0;
		}
	}

	for (auto& [key, val] : timerCommands)
	{
		if (val.size() > 0 && timers[key]->HasElapsed())
		{
			for (auto& cmd : val)
			{
				commands.ExecuteCommand(cmd);
			}

			val.clear();
		}
	}

	UpdateText();

	if (!isReadingNextLine && inputTimer.HasElapsed())
	{
		CheckKeys();
	}
}

void CutsceneManager::UpdateText()
{
	if (!printTimer.HasElapsed())
		return;

	const Uint8* input = SDL_GetKeyboardState(NULL);

	int mouseX, mouseY = 0;
	mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	// TODO: Disable all of this if the keyboard controls are disabled
	// And also allow mouse control alternatives
	if (inputTimer.HasElapsed())
	{
		if (input[autoButton])
		{
			automaticallyRead = !automaticallyRead;
			inputTimer.Start(inputTimeToWait);
		}

		//TODO: Make this more customizable
		if (input[SDL_SCANCODE_0])
		{
			autoTimeIndex++;
			if (autoTimeIndex > 2)
				autoTimeIndex = 0;
			inputTimer.Start(inputTimeToWait);
		}
		if (input[SDL_SCANCODE_1])
		{
			autoTimeIndex = 0;
			inputTimer.Start(inputTimeToWait);
		}
		if (input[SDL_SCANCODE_2])
		{
			autoTimeIndex = 1;
			inputTimer.Start(inputTimeToWait);
		}
		if (input[SDL_SCANCODE_3])
		{
			autoTimeIndex = 2;
			inputTimer.Start(inputTimeToWait);
		}
	}

#if _DEBUG
	if (input[SDL_SCANCODE_TAB] && inputTimer.HasElapsed())
	{
		autoskip = !autoskip;
		std::cout << "1 TOGGLE AUTOSKIP " << autoskip << std::endl;
		inputTimer.Start(inputTimeToWait);
	}
#endif

	isSkipping = input[skipButton] || input[skipButton2] || autoskip;
	if (disableSkip)
		isSkipping = false;
	if (isTravelling)
		isSkipping = true;

	//std::cout << "Label: " << command << std::endl;

	//TODO: Fix this? it no longer works properly with the corrected dt
	msGlyphTime += (float)game->dt;

	// If waiting for a button press... 
	// (the delay is so that the player doesn't press a button too quickly)
	if (waitingForButton)// && msGlyphTime > msDelayBetweenGlyphs)
	{
		//msGlyphTime -= msDelayBetweenGlyphs;

		// We want to get the mouse/keyboard input here		
		if (inputTimer.HasElapsed() || isTravelling)
		{
			mouseX *= Camera::MULTIPLIER;
			mouseY *= Camera::MULTIPLIER;

			SDL_Rect mouseRect;
			mouseRect.x = mouseX;
			mouseRect.y = mouseY;
			mouseRect.w = 1;
			mouseRect.h = 1;

			int hoveredButton = -1;

			// TODO: Have the option to use a shader instead of changing color

			game->CheckController(false);

			bool controllerPluggedIn = game->controller != nullptr;

			for (int i = 0; i < SDL_NumJoysticks(); i++)
			{
				if (SDL_IsGameController(i))
				{
					controllerPluggedIn = true;
					break;
				}
			}

			if (!controllerPluggedIn)
			{
				for (int i = 0; i < activeButtons.size(); i++)
				{
					int index = activeButtons[i];

					if (images[index] != nullptr)
					{
						if (HasIntersection(images[index]->GetTopLeftBounds(), mouseRect))
						{
							images[index]->SetColor({ 255, 255, 0, 255 });
							hoveredButton = i;
						}
						else
						{
							images[index]->SetColor({ 255, 255, 255, 255 });
						}
					}
				}
			}
			else
			{
				for (int i = 0; i < activeButtons.size(); i++)
				{
					int index = activeButtons[i];

					if (images[index] != nullptr)
					{
						images[index]->SetColor({ 255, 255, 255, 255 });
					}

					if (buttonIndex > -1 && images[activeButtons[buttonIndex]] != nullptr)
					{
						images[activeButtons[buttonIndex]]->SetColor({ 255, 255, 0, 255 });
					}
					
				}
			}

			bool clickedMouse = false;
			if (mouseState & (~previousMouseState) & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				previousMouseState = mouseState;
				if (hoveredButton > -1)
				{
					buttonIndex = hoveredButton;
					clickedMouse = true;
				}
				else if (waitingForClick)
				{
					clickedMouse = true;
				}
			}
			else if (mouseState & (~previousMouseState) & SDL_BUTTON(SDL_BUTTON_RIGHT))
			{
				previousMouseState = mouseState;
				buttonIndex = -1;
				clickedMouse = true;
			}

			if (input[SDL_SCANCODE_UP] || input[SDL_SCANCODE_W])
			{
				if (buttonIndex > -1 && images[activeButtons[buttonIndex]] != nullptr)
				{
					images[activeButtons[buttonIndex]]->SetColor({ 255, 255, 255, 255 });
				}

				buttonIndex--;
				if (buttonIndex < 0)
					buttonIndex = activeButtons.size() - 1;

				if (buttonIndex > -1 && images[activeButtons[buttonIndex]] != nullptr)
				{
					images[activeButtons[buttonIndex]]->SetColor({ 255, 255, 0, 255 });
				}

				inputTimer.Start(inputTimeToWait);
			}
			else if (input[SDL_SCANCODE_DOWN] || input[SDL_SCANCODE_S])
			{
				if (buttonIndex > -1 && images[activeButtons[buttonIndex]] != nullptr)
				{
					images[activeButtons[buttonIndex]]->SetColor({ 255, 255, 255, 255 });
				}

				buttonIndex++;
				if (buttonIndex >= activeButtons.size())
					buttonIndex = 0;

				if (buttonIndex > -1 && images[activeButtons[buttonIndex]] != nullptr)
				{
					images[activeButtons[buttonIndex]]->SetColor({ 255, 255, 0, 255 });
				}

				inputTimer.Start(inputTimeToWait);
			}
			else if (input[readButton] || input[readButton2] || clickedMouse)
			{
				MakeChoice();
			}
			else if (autoChoice > 0) // Don't automatically select a menu option, but do automatically make choices
			{
				if (!atChoice)
				{
					if (input[readButton] || input[readButton2] || clickedMouse)
					{
						MakeChoice();
					}
				}
				else // TODO: Make sure this still works for super fast travel
				{
					// Automatically select a choice (if enabled)
					buttonIndex = autoChoice - 1;

					// Stay within bounds
					if (buttonIndex < 0)
						buttonIndex = 0;
					if (buttonIndex >= activeButtons.size())
						buttonIndex = activeButtons.size() - 1;

					MakeChoice();
				}
				
			}
		}

		return;
	}
	else
	{
		// render the textbox when not waiting
		textbox->isReading = (msGlyphTime > 0);

		if (isSkipping)
			msDelayBetweenGlyphs = 0.0f;
		else
			msDelayBetweenGlyphs = msInitialDelayBetweenGlyphs;

		//TODO: Continue to execute functions that have not finished yet (moveto, lerp) (multi-threading?)

		int unfinishedIndex = 0;
		// TODO: This is including functions that should not be included

		/*
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
		*/

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

			if (waitingForButton)
				return;

			if (isCarryingOutCommands)
			{
				if (commandIndex >= 0 && commandIndex < lines[currentLabel->lineStart + lineIndex].commandsSize)
				{
					if (lines[currentLabel->lineStart + lineIndex].commandsSize > 1)
					{
						textbox->shouldRender = false;
					}

					//std::cout << currentLabel->lines[lineIndex].commands[commandIndex] << std::endl;
					printNumber = 0;
					do
					{

#if _DEBUG
						if (input[SDL_SCANCODE_TAB] && inputTimer.HasElapsed())
						{
							autoskip = !autoskip;
							std::cout << "2 TOGGLE AUTOSKIP " << autoskip << std::endl;
							inputTimer.Start(inputTimeToWait);
						}
#endif


						std::string command = GetCommand(lines[currentLabel->lineStart + lineIndex], commandIndex);

						if (!commands.ExecuteCommand(command))
						{
							unfinishedCommands.push_back(GetCommand(lines[currentLabel->lineStart + lineIndex], commandIndex));
						}
						commandIndex++;

						if (!isTravelling)
						{
							// TODO: What happens if current label is nullptr here?
							if (currentLabel == nullptr)
								return;

							if (commandIndex >= lines[currentLabel->lineStart + lineIndex].commandsSize)
							{
								// Allow falling through subroutines to other labels up until the next return
								// TODO: This breaks any button menus, though!
								/*
								int length = lines[currentLabel->lineStart + lineIndex].GetTextLength();
								if (lineIndex == currentLabel->lineSize - 1 && length == 0)
								{
									labelIndex++;
									currentLabel = &labels[labelIndex];
								}
								else
								{
									break;
								}
								*/
								break;
							}


							if (waitingForButton)
								break;
						}
						else
						{
							printNumber = 0;

							if (commandIndex >= lines[currentLabel->lineStart + lineIndex].commandsSize)
							{

								// We must call MakeChoice here because it will never be reached otherwise
								if (waitingForButton)
								{
									atChoice = true;
									MakeChoice();
									commandIndex = 0; // this MUST be here to make sure we don't go backwards
								}
								else
								{
									ReadNextLine();

									static int prevLabelIndex = 0;

									if (labelIndex != prevLabelIndex)
									{
										prevLabelIndex = labelIndex;
										std::cout << "Label: " << GetLabelName(labels[labelIndex]) << std::endl;
									}
								}

							}

							if (GetLabelName(labels[labelIndex]) == endTravelLabel)
							{
								isTravelling = false;
								autoChoice = 0;
								printNumber = 1;
							}
						}

						// run all commands until we hit a print or wait command
					} while (!autoprint && printNumber == 0 && msGlyphTime > 0);

					if (!isTravelling)
					{
						game->updateScreenTexture = true;

						if (printNumber > 0 && !isSkipping)
						{
							if (printEffects.count(printNumber) != 0)
							{
								printTimer.Start(printEffects[printNumber].delay);
							}
							else
							{
								printTimer.Start(1);
								game->logger.Log("ERROR: No print effect found for " + std::to_string(printNumber));
							}
						}
					}

				}
				else
				{
					isCarryingOutCommands = false;

					if (isTravelling) // don't read text when travelling
					{
						isReadingNextLine = false;
					}
				}
			}
			else if (isReadingNextLine)
			{
				textbox->shouldRender = true;

				CheckKeysWhileReading();

				if (readingBacklog)
				{
					isReadingNextLine = false;
					return;
				}

				bool displayAllText = (msDelayBetweenGlyphs == 0) || clickedMidPage;

				do
				{
					if (currentLabel == nullptr)
					{
						game->logger.Log("ERROR: Current label is NULL!");
						return;
					}

					if (commands.lineBreaks > 0)
					{
						for (int i = 0; i < commands.lineBreaks; i++)
						{
							textbox->UpdateText('\n', currentColor);
						}
						commands.lineBreaks = 0;
					}

					int newIndex = letterIndex + lines[currentLabel->lineStart + lineIndex].textStart;

					if (newIndex >= lines[currentLabel->lineStart + lineIndex].textEnd)
					{
						isReadingNextLine = false;
						return;
					}

					std::string result = ParseText(data, newIndex, currentColor, textbox->text);
					letterIndex = newIndex - lines[currentLabel->lineStart + lineIndex].textStart;

					if (playSoundsOnText)
					{
						game->soundManager.PlaySound(textSounds[GetLineSpeaker(GetCurrentLine())]);
					}

					if (result.size() > 1)
					{
						for (int i = 0; i < result.size(); i++)
						{
							textbox->UpdateText(result[i], currentColor);
						}
					}
					else if (result.size() == 1)
					{
						if (result[0] < 0)
						{
							textbox->shouldRender = false;
							ReadNextLine();
							continue;
						}
						else
						{
							textbox->UpdateText(result[0], currentColor);
						}
					}

					//nextLetterTimer.Start(lettersPerFrame * delay);

					// Reached the 'click to continue' point
					if (letterIndex >= lines[currentLabel->lineStart + lineIndex].GetTextLength())
					{
						previousText = currentText;
						currentText = textbox->text->txt;
						isReadingNextLine = false;
						displayAllText = false;
						clickedMidPage = false;

						textbox->SetCursorPosition(data[newIndex + 1] != '@');
						//textbox->clickToContinue->Update(*game);
						//game->player->cutsceneInputTimer.Start(100);

						if (automaticallyRead)
						{
							autoTimeToWait = (textbox->text->glyphs.size() * autoTimeToWaitPerGlyph);
							autoReaderTimer.Start(autoTimeToWait);
						}

						return;
					}

				} while (displayAllText);

			}
		}


		// Wait a number of milliseconds before showing the next glyph
		if (!waitingForButton && msDelayBetweenGlyphs > 0 && !isSkipping)
		{
			while (msGlyphTime > msDelayBetweenGlyphs)
			{
				msGlyphTime -= msDelayBetweenGlyphs;
			}
		}
	}

	
}

void CutsceneManager::MakeChoice()
{
	if (waitingForClick)
	{
		waitingForClick = false;
		waitingForButton = false;
		return;
	}

	if (buttonIndex < 0)
	{
		if (buttonIndex == -1)
		{
			if (!atChoice) // TODO: Handle right-click at choices
			{
				// Only proceed if any of the following commands deal with -1
				int tempCommandIndex = commandIndex;
				std::string command = "";

				do
				{
					command = GetCommand(lines[currentLabel->lineStart + lineIndex], tempCommandIndex);

					if (command.find("= -1 ") != std::string::npos && command.find("if %") != std::string::npos)
					{
						commands.numberVariables[buttonResult] = -1;
						waitingForButton = false;
						inputTimer.Start(inputTimeToWait);
						return;
					}

					tempCommandIndex++;

				} while (tempCommandIndex < lines[currentLabel->lineStart + lineIndex].commandsSize);

			}
		}

		return;
	}

	// Return the result in the specified variable and resume reading
	// TODO: This can be buggy if the btnwait variable is not reset beforehand
	unsigned int chosenSprite = activeButtons[buttonIndex];
	commands.numberVariables[buttonResult] = spriteButtons[chosenSprite];
	waitingForButton = false;

	if (atChoice) // TODO: Maybe a way to toggle this via script to auto clear sprites?
	{
		atChoice = false;
		isCarryingOutCommands = true;
		isReadingNextLine = true;
		textbox->isReading = true;

		// Remove the sprite buttons from the screen
		CutsceneFunctions::ClearSprite({ "", std::to_string(choiceSpriteStartNumber) }, commands);   // bg
		CutsceneFunctions::ClearSprite({ "", std::to_string(choiceSpriteStartNumber + 1) }, commands); // question
		for (int i = 0; i < activeButtons.size(); i++)
		{
			CutsceneFunctions::ClearSprite({ "", std::to_string(activeButtons[i]) }, commands);
		}

		int result = -198;
		int responseNumber = 0;

		// Evaluate if statements
		if (choiceIfStatements.size() > 0)
		{
			for (int i = 0; i < choiceIfStatements.size(); i++)
			{
				int r = commands.ExecuteCommand(choiceIfStatements[i]);
				result = std::max(r, result);
				if (result > 0)
				{
					responseNumber = i;
				}				
			}
		}

		// Store the choice prompt and selected response
		if (currentChoice > -1)
		{
			SelectedChoiceData selectedChoice;
			selectedChoice.choiceNumber = currentChoice;
			selectedChoice.responseNumber = responseNumber;
			selectedChoices.emplace_back(selectedChoice);
			commands.stringVariables[buttonResult] = Trim(allChoices[currentChoice].responses[responseNumber]);
		}

		// TODO: This is not actually reached when clicking buttons, defeating the purpose of writing it
		if (result < 0)
		{
			game->logger.Log("ERROR: Button was pressed, but satisfied no conditions!");
		}

		ClearPage();
	}

	activeButtons.clear();

	inputTimer.Start(inputTimeToWait);
}

void CutsceneManager::ClearPage()
{
	//TODO: Make sure to save the backlog when we save the game		

	// Before we clear the textbox, add to the backlog
	// (We save strings because the text might have commands in between. 
	// TODO: Can we do better?)

	backlog.push_back(new BacklogData(currentText, textbox->speaker->txt));

	// If backlog is full, remove the oldest entry
	if (backlog.size() > backlogMaxSize)
	{
		delete_it(backlog[0]);
		backlog.erase(backlog.begin());
	}

	// Clear the textbox
	currentText = "";
	textbox->fullTextString = "";
	textbox->text->SetText(currentText);
	textbox->speaker->SetText(GetLineSpeaker(lines[currentLabel->lineStart + lineIndex + 1]), currentColor);
}

// TODO: Color tags inside of brackets does not work (such as: text [#ff0000#Example#00ff00#Test])
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
			case '^': // key mapping
				variableValue = game->inputManager.GetMappedKeyAsString(variableName);
				break;
			default:
				variableValue = word;
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
				if (namesToColors.count(GetLineSpeaker(lines[currentLabel->lineStart + lineIndex])))
				{
					textColor = namesToColors[GetLineSpeaker(lines[currentLabel->lineStart + lineIndex])];
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
			textColor = ParseColorHexadecimal(originalString.substr(letterIndex, finalColorIndex - letterIndex).c_str());
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

			// Add the image to the text here

			//TODO: Implement animated icons in text
			//Sprite* sprite = animatedImages[animName]->GetSprite();
			//text->AddImage(sprite);		
			//text->GetLastGlyph()->animator = animatedImages[animName]->GetAnimator();

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
	std::vector<std::string> globalDataArrays;
	std::vector<std::string> globalDataArraySlots;

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
		else if (line == "@ GLOBAL_ARRAYS")
		{
			globalSection = 3;
			continue;
		}
		else if (line == "@ GLOBAL_ARRAYS_SLOTS")
		{
			globalSection = 4;
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
		case 3:
			globalDataArrays.push_back(line + "\n");
			break;
		case 4:
			globalDataArraySlots.push_back(line + "\n");
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
		CutsceneFunctions::SetStringVariable({ "", dataKey, dataValue }, commands);
	}

	for (int i = 0; i < globalDataNumbers.size(); i++)
	{
		index = 0;
		dataKey = ParseWord(globalDataNumbers[i], ' ', index);
		dataValue = ParseWord(globalDataNumbers[i], '\n', index);
		CutsceneFunctions::SetNumberVariable({ "", dataKey, dataValue, "no_alias" }, commands);
	}

	for (int i = 0; i < globalDataArrays.size(); i++)
	{
		index = 0;
		dataKey = ParseWord(globalDataArrays[i], ' ', index);
		dataValue = ParseWord(globalDataArrays[i], '\n', index);

		std::stringstream ss(dataValue);
		std::istream_iterator<std::string> begin(ss);
		std::istream_iterator<std::string> end;

		std::vector<std::string> splitValues(begin, end);

		unsigned int arrIndex = std::stoi(dataKey);
		commands.arrayVariables[arrIndex].clear();
		commands.arrayVariables[arrIndex].reserve(splitValues.size());

		for (int k = 0; k < splitValues.size(); k++)
		{
			commands.arrayVariables[arrIndex].emplace_back(std::stoi(splitValues[k]));
		}
	}

	for (int i = 0; i < globalDataArraySlots.size(); i++)
	{
		index = 0;
		dataKey = ParseWord(globalDataArraySlots[i], ' ', index);
		dataValue = ParseWord(globalDataArraySlots[i], '\n', index);

		commands.arrayNumbersPerSlot[std::stoi(dataKey)] = std::stoi(dataValue);
	}

}

void CutsceneManager::SaveGlobalVariable(unsigned int key, const std::string& value, bool isNumber)
{
	// 1. Load the global variables into a data structure from the file
	std::ifstream fin;
	std::vector<std::string> globalDataNumbers;
	std::vector<std::string> globalDataStrings;
	std::vector<std::string> globalDataArrays;
	std::vector<std::string> globalDataArraySlots;

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
		else if (line == "@ GLOBAL_ARRAYS")
		{
			globalSection = 3;
			continue;
		}
		else if (line == "@ GLOBAL_ARRAYS_SLOTS")
		{
			globalSection = 4;
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
		case 3:
			globalDataArrays.push_back(line + "\n");
			break;
		case 4:
			globalDataArraySlots.push_back(line + "\n");
			break;
		default:
			break;
		}
	}

	fin.close();

	if (isNumber)
	{
		// Deal with arrays here
		if (value == "?")
		{
			std::string newValue = "";
			for (int i = 0; i < commands.arrayVariables[key].size(); i++)
			{
				newValue += std::to_string(commands.arrayVariables[key][i]) + " ";
			}
			//std::cout << newValue << std::endl;
			ModifyGlobalVariableVector(globalDataArrays, key, newValue);
			ModifyGlobalVariableVector(globalDataArraySlots, key, std::to_string(commands.arrayNumbersPerSlot[key]));
		}
		else
		{
			ModifyGlobalVariableVector(globalDataNumbers, key, value);
		}
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

	fout << "@ GLOBAL_ARRAYS" << std::endl;
	for (int i = 0; i < globalDataArrays.size(); i++)
	{
		fout << globalDataArrays[i];
	}

	fout << "@ GLOBAL_ARRAYS_SLOTS" << std::endl;
	for (int i = 0; i < globalDataArraySlots.size(); i++)
	{
		fout << globalDataArraySlots[i];
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
		{ SaveSections::SEEN_CHOICES, "@ SEEN_CHOICES"},
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
			fout << GetLabelName(currentLabel) << " ";
			fout << labelIndex << " ";
			fout << lineIndex << " ";
			fout << commandIndex << " ";

			if (textbox->speaker->txt != "")
			{
				fout << textbox->speaker->txt << std::endl;
			}

			break;
		case SaveSections::SEEN_CHOICES:

			for (auto const& choice : selectedChoices)
			{
				fout << choice.choiceNumber << " " << choice.responseNumber << std::endl;
			}

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
				fout << (gosubStack[i]->lineIndex + 1) << " ";
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
					if (entity->etype == "text")
					{
						//TODO: Store font, size, style, etc. (for each letter)
						Text* text = static_cast<Text*>(entity);
						fout << var.first  // key
							<< " "
							<< entity->etype
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
							<< (int)text->GetSprite()->color.r
							<< " "
							<< (int)text->GetSprite()->color.g
							<< " "
							<< (int)text->GetSprite()->color.b
							<< " "
							<< (int)text->GetSprite()->color.a
							<< std::endl;
					}
					else if (entity->etype == "particlesystem")
					{
						ParticleSystem* ps = dynamic_cast<ParticleSystem*>(entity);

						fout << var.first  // key
							<< " "
							<< entity->etype
							<< " "
							<< ps->position.x
							<< " "
							<< ps->position.y
							<< " "
							<< ps->rotation.x
							<< " "
							<< ps->rotation.y
							<< " "
							<< ps->rotation.z
							<< " "
							<< ps->scale.x
							<< " "
							<< ps->scale.y
							<< " "
							<< ps->nextParticleVelocity.x
							<< " "
							<< ps->nextParticleVelocity.y
							<< " "
							<< ps->nextParticleTimeToLive
							<< " "
							<< ps->nextParticleColliderWidth
							<< " "
							<< ps->nextParticleColliderHeight
							<< " "
							<< (ps->spawnTimer.endTime - ps->spawnTimer.startTicks)
							<< " "
							<< ps->nextParticleSpriteFilename.size();

						for (int i = 0; i < ps->nextParticleSpriteFilename.size(); i++)
						{
							fout << " " << ps->nextParticleSpriteFilename[i];
						}

						fout << std::endl;
					}
					else
					{
						std::string fname = entity->GetSprite()->GetFileName();
						if (fname.size() < 1 || fname == " ")
							fname = "image";

						fout << var.first  // key
							<< " "
							<< fname
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
							<< " "
							<< entity->GetSprite()->shader->GetNameString()
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
						<< (int)var.second.r
						<< " "
						<< (int)var.second.g
						<< " "
						<< (int)var.second.b
						<< " "
						<< (int)var.second.a
						<< std::endl;
				}
				else
				{
					fout << "_"  // key
						<< " "
						<< (int)var.second.r
						<< " "
						<< (int)var.second.g
						<< " "
						<< (int)var.second.b
						<< " "
						<< (int)var.second.a
						<< std::endl;
				}
			}
			break;
		case SaveSections::OTHER_STUFF:

			// Save the window title
			fout << "window title " << '<' << game->windowTitle << '>' << std::endl;

			// Save the window icon
			fout << "window icon " << '<' << game->windowIconFilepath << '>' << std::endl;

			// Save the random seed
			fout << "random seed " << commands.randomSeed << std::endl;

			// Save controller bindings
			fout << "controls mouse " << useMouseControls << std::endl;
			fout << "controls keyboard " << useKeyboardControls << std::endl;

			// Save the currently playing BGM
			fout << "bgm " << '<' << game->soundManager.bgmFilepath << '>' << std::endl;

			// Save the autochoice setting
			fout << "autochoice " << autoChoice << std::endl;

			// Save other looped sounds
			for (auto const& [num, channel] : game->soundManager.sounds)
			{
				if (channel != nullptr)
				{
					if (channel->loop == -1)
						fout << "me " << channel->num << " " << '<' << channel->sound->filepath << '>' << std::endl;
					//else - no need to play sounds on load
					//	fout << "se " << channel->num << " " << '<' << channel->sound->filepath << '>' << " " << channel->loop << std::endl;
				}
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
	// Clear everything off the screen before we load the game
	for (const auto& [key, val] : images)
	{
		CutsceneFunctions::ClearSprite({ "clear", std::to_string(key), "0" }, commands);
	}

	// Clear backlog text
	backlog.clear();

	// Clear selected choices
	selectedChoices.clear();

	// Stop all sounds
	game->soundManager.StopBGM();
	game->soundManager.FadeOutChannel(-1);

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

	// Temporarily remove the path prefix
	// to correctly load sprites
	std::string pathPrefix = commands.pathPrefix;
	commands.pathPrefix = "";

	// While we have lines to examine
	// Check the first character for an @ symbol
	// If there is one, change the current section
	// Otherwise, interpret the line based on the current section

	std::map<std::string, SaveSections> sections = {
		{ "@ CONFIG_OPTIONS", SaveSections::CONFIG_OPTIONS},
		{ "@ STORY_DATA", SaveSections::STORY_DATA},
		{ "@ SEEN_CHOICES", SaveSections::SEEN_CHOICES},
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
	std::string currentStringAlias = "";
	SelectedChoiceData choiceData;

	while (index < dataLines.size())
	{
		// Replace all the bracketed spaces with underscores
		bool shouldReplace = false;
		for (int i = 0; i < dataLines[index].size(); i++)
		{
			if (shouldReplace && dataLines[index][i] == ' ')
			{
				dataLines[index][i] = '`';
			}
			else if (dataLines[index][i] == '<')
			{
				shouldReplace = true;
			}
			else if (dataLines[index][i] == '>')
			{
				shouldReplace = false;
			}
		}

		std::stringstream ss(dataLines[index]);
		std::istream_iterator<std::string> begin(ss);
		std::istream_iterator<std::string> end;

		std::vector<std::string> lineParams(begin, end);
		//std::copy(lineParams.begin(), lineParams.end(), std::ostream_iterator<std::string>(std::cout, " "));

		// Replace all the bracketed underscores with spaces again
		shouldReplace = false;
		for (int i = 0; i < lineParams.size(); i++)
		{
			bool replaced = false;
			shouldReplace = false;
			for (int k = 0; k < lineParams[i].size(); k++)
			{
				if (shouldReplace && lineParams[i][k] == '`')
				{
					lineParams[i][k] = ' ';
				}
				else if (lineParams[i][k] == '<')
				{
					replaced = true;
					shouldReplace = true;
					lineParams[i][k] = ' ';
				}
				else if (lineParams[i][k] == '>')
				{
					replaced = true;
					shouldReplace = false;
					lineParams[i][k] = ' ';
				}
			}

			Trim(lineParams[i]);
		}

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
				lineIndex = std::stoi(lineParams[2]);
				commandIndex = std::stoi(lineParams[3]);
				isCarryingOutCommands = false;

				if (lineParams.size() > 4)
				{
					SetSpeakerText(lineParams[4]);
				}
				else
				{
					SetSpeakerText("");
				}

				currentLabel = JumpToLabel(labelName.c_str());
				if (currentLabel == nullptr)
				{
					if (labelIndex < labels.size())
						currentLabel = &labels[labelIndex];
				}
				break;
			case SaveSections::SEEN_CHOICES:
				choiceData.choiceNumber = std::stoi(lineParams[0]);
				choiceData.responseNumber = std::stoi(lineParams[1]);
				selectedChoices.emplace_back(choiceData);
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
				// This is done so that we can use filepaths with spaces in them
				currentStringAlias = dataLines[index].substr(lineParams[0].size() + 1, dataLines[index].size() - lineParams[0].size());
											
				commands.stralias[lineParams[0]] = currentStringAlias;
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
					CutsceneFunctions::LoadTextFromSaveFile(lineParams, commands);
				}
				else if (lineParams[1] == "particlesystem")
				{
					// particle system create %var2 %var1 0

					// Spawn the particle system
					CutsceneFunctions::ParticleCommand({"particle", "system", "create", lineParams[0], lineParams[2], lineParams[3] }, commands);

					// Modify its values
					ParticleSystem* ps = dynamic_cast<ParticleSystem*>(images[std::stoi(lineParams[0])]);

					ps->rotation = glm::vec3(
						std::stoi(lineParams[4]),
						std::stoi(lineParams[5]),
						std::stoi(lineParams[6]));

					ps->scale = Vector2(
						std::stoi(lineParams[7]),
						std::stoi(lineParams[8]));

					ps->nextParticleVelocity.x = std::stof(lineParams[9]);
					ps->nextParticleVelocity.y = std::stof(lineParams[10]);

					ps->nextParticleTimeToLive = std::stof(lineParams[11]);

					ps->nextParticleColliderWidth = std::stof(lineParams[12]);
					ps->nextParticleColliderHeight = std::stof(lineParams[13]);

					ps->spawnTimer.Start(std::stoi(lineParams[14]));

					int numberOfSprites = std::stoi(lineParams[15]);
					ps->nextParticleSpriteFilename.reserve(numberOfSprites);
					for (int i = 0; i < numberOfSprites; i++)
					{
						ps->nextParticleSpriteFilename.emplace_back(lineParams[16 + i]);
					}

				}
				else // load sprite object
				{
					lineParams.insert(lineParams.begin(), "");
					CutsceneFunctions::LoadSprite(lineParams, commands);
					Entity* entity = images[std::stoi(lineParams[1])];

					entity->rotation = glm::vec3(
						std::stoi(lineParams[5]), 
						std::stoi(lineParams[6]), 
						std::stoi(lineParams[7]));
					
					entity->scale = Vector2(
						std::stoi(lineParams[8]), 
						std::stoi(lineParams[9]));

					if (lineParams.size() > 10)
					{
						if (lineParams[10] == Globals::NONE_STRING)
						{
							entity->GetSprite()->SetShader(game->renderer.shaders[ShaderName::Default]);
						}
						else
						{
							entity->GetSprite()->SetShader(commands.customShaders[lineParams[10]]);
						}
					}

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

				if (lineParams[0] == "bgm" && lineParams.size() > 1)
				{
					game->soundManager.PlayBGM(lineParams[1]);
				}
				else if (lineParams[0] == "me" && lineParams.size() > 2)
				{
					game->soundManager.PlaySound(lineParams[2], std::stoi(lineParams[1]), -1);
				}
				else if (lineParams[0] == "se" && lineParams.size() > 3)
				{
					// no need to play sounds on load
					//game->soundManager.PlaySound(lineParams[2], std::stoi(lineParams[1]), std::stoi(lineParams[3]));
				}
				else if (lineParams[0] == "window")
				{
					CutsceneFunctions::WindowFunction(lineParams, commands);
				}
				else if (lineParams[0] == "random")
				{
					CutsceneFunctions::RandomNumberVariable(lineParams, commands);
				}
				else if (lineParams[0] == "autochoice")
				{
					autoChoice = std::stoi(lineParams[1]);
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

	commands.pathPrefix = pathPrefix;

	// This is necessary to advance the text after loading.
	// Because we can't fall through labels, it presents an empty textbox.
	// So we read the next line to resume playing.
	ReadNextLine();
}
