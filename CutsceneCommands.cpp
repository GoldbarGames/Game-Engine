#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <filesystem>

#include "CutsceneCommands.h"
#include "CutsceneManager.h"
#include "Game.h"
#include "Timer.h"
#include "Animator.h"
#include "PhysicsComponent.h"

typedef int (CutsceneCommands::*FuncList)(CutsceneParameters parameters);

//Look Up Table
struct FuncLUT {
	char command[32];
	FuncList method;
};

std::vector<FuncLUT>cmd_lut = {
	{"~", &CutsceneCommands::DoNothing},
	{"add", &CutsceneCommands::AddNumberVariables},
	{"align", &CutsceneCommands::AlignCommand},
	{"animation", &CutsceneCommands::AnimationCommand},
	{"automode", &CutsceneCommands::AutoMode},
	{"autoreturn", &CutsceneCommands::AutoReturn},
	{"autosave", &CutsceneCommands::AutoSave},
	{"backlog", &CutsceneCommands::OpenBacklog},
	{"bg", &CutsceneCommands::LoadBackground },
	{"bgm", &CutsceneCommands::MusicCommand },
	{"btnwait", &CutsceneCommands::WaitForButton },
	{"camera", &CutsceneCommands::CameraFunction},
	{"controls", & CutsceneCommands::ControlBindings},
	{"choice", &CutsceneCommands::DisplayChoice },
	{"cl", &CutsceneCommands::ClearSprite },
	{"concat", &CutsceneCommands::ConcatenateStringVariables},
	{"ctc", &CutsceneCommands::SetClickToContinue},
	{"dec", &CutsceneCommands::DecrementVariable},
	{"defsub", &CutsceneCommands::DefineUserFunction},
	{"div", &CutsceneCommands::DivideNumberVariables},
	{"end", &CutsceneCommands::EndGame },
	{"errorlog", &CutsceneCommands::ErrorLog },
	{"fade", &CutsceneCommands::Fade },
	{"fileexist", &CutsceneCommands::FileExist},
	{"flip", &CutsceneCommands::FlipSprite },
	{"font", &CutsceneCommands::FontCommand},
	{"getname", &CutsceneCommands::GetResourceFilename},
	{"global", &CutsceneCommands::SetGlobalNumber},
	{"gosub", &CutsceneCommands::GoSubroutine },
	{"goto", &CutsceneCommands::GoToLabel },
	{"if", &CutsceneCommands::IfCondition },
	{"inc", &CutsceneCommands::IncrementVariable},
	{"input", &CutsceneCommands::InputCommand},
	{"itoa", &CutsceneCommands::IntToString },
	{"jumpb", &CutsceneCommands::JumpBack },
	{"jumpf", &CutsceneCommands::JumpForward },
	{"keybind", &CutsceneCommands::BindKeyToLabel },
	{"ld", &CutsceneCommands::LoadSprite },
	{"loadgame",&CutsceneCommands::LoadGame },
	{"lua", &CutsceneCommands::LuaCommand},
	{"me", &CutsceneCommands::MusicEffectCommand},
	{"mod", &CutsceneCommands::ModNumberVariables},
	{"mov", &CutsceneCommands::MoveVariables},
	{"mul", &CutsceneCommands::MultiplyNumberVariables},
	{"name", &CutsceneCommands::NameCommand},
	{"namedef", &CutsceneCommands::NameDefineCommand},
	{"namebox", &CutsceneCommands::Namebox},
	{"numalias", &CutsceneCommands::SetNumAlias },
	{"random", &CutsceneCommands::RandomNumberVariable },
	{"reset", &CutsceneCommands::ResetGame },
	{"resolution", &CutsceneCommands::SetResolution },
	{"return", &CutsceneCommands::ReturnFromSubroutine },
	{"savegame",&CutsceneCommands::SaveGame },
	{"screenshot",&CutsceneCommands::ScreenshotCommand },
	{"se", &CutsceneCommands::SoundCommand },
	{"set_velocity", &CutsceneCommands::SetVelocity },
	{"setnumvar", &CutsceneCommands::SetNumberVariable },
	{"setstrvar", &CutsceneCommands::SetStringVariable },
	{"spbtn", &CutsceneCommands::SetSpriteButton},
	{"sprite", &CutsceneCommands::SetSpriteProperty },
	{"stdout", &CutsceneCommands::Output },
	{"stralias", &CutsceneCommands::SetStringAlias },
	{"sub", &CutsceneCommands::SubtractNumberVariables},
	{"substr", &CutsceneCommands::SubstringVariables},
	{"tag", &CutsceneCommands::TagCommand},
	{"text", &CutsceneCommands::LoadText },
	{"textbox", &CutsceneCommands::Textbox },
	{"textcolor", &CutsceneCommands::TextColor },
	{"textspeed", &CutsceneCommands::TextSpeed },
	{"timer", &CutsceneCommands::TimerFunction},
	{"wait",& CutsceneCommands::Wait },
	{"window", &CutsceneCommands::WindowFunction }
};

// TODO: Need to handle instances of : in other spots (like in strings)
// TODO: Commands executed during the same textbox `like`@`this`\

// For loops, while loops

// Alpha image effects (apply alpha mask using shader to texture, entire screen?)
// Save/load (save backlog, erase savefiles, thumbnails, custom save notes)

// Animations for textbox and namebox

// - what about animations that involve each frame being its own file?
	// 1. construct the animation with a list of filenames in the order they will be drawn
	// 2. when rendering the sprite, don't divide the texture; instead, use the whole frame

// - custom timers (we'll deal with this when we handle blinking animations)

// map actions to buttons
// -> map buttons to keys

// Check if any physical buttons in the list have been pressed
// in order to carry out the action specified by the button

// Quake horizontal, vertical, both

CutsceneCommands::CutsceneCommands()
{
	//TODO: Add a command to define this via scripting
	buttonLabels[(unsigned int)SDL_SCANCODE_ESCAPE] = "pause_menu";

	numalias["bg"] = 0;
	numalias["l"] = 1;
	numalias["c"] = 2;
	numalias["r"] = 3;

	numalias["param1"] = 101;
	numalias["param2"] = 102;
	numalias["param3"] = 103;
	numalias["param4"] = 104;
	numalias["param5"] = 105;
	numalias["param6"] = 106;
	numalias["param7"] = 107;
	numalias["param8"] = 108;
	numalias["param9"] = 109;
}

CutsceneCommands::~CutsceneCommands()
{

}

bool CutsceneCommands::ExecuteCommand(std::string command)
{
	bool finished = true;
	// Replace all the bracketed spaces with underscores
	bool shouldReplace = false;
	for (int i = 0; i < command.size(); i++)
	{
		if (shouldReplace && command[i] == ' ')
		{
			command[i] = '`';
		}
		else if (command[i] == '[')
		{
			shouldReplace = true;
		}
		else if (command[i] == ']')
		{
			shouldReplace = false;
		}
	}

	std::istringstream ss(command);
	std::string token;
	std::vector<std::string> parameters;

	char delimit = (command.find(',') != std::string::npos) ? ',' : ' ';

	//TODO: Maybe instead of using getline, just check for COMMA or SPACE and accept either one
	if (delimit == ',')
	{
		std::getline(ss, token, ' ');
		parameters.push_back(token);

		//TODO: if commands -- you can't mix space separated and comma separated,
		// so just force space separated (it breaks if it uses commas)
		if (token != "if" && token != "choice")
		{
			while (std::getline(ss, token, delimit))
			{
				if (token != "")
				{
					parameters.push_back(token);
				}
			}
		}
		else
		{
			while (std::getline(ss, token, ' '))
			{
				if (token != "")
				{
					parameters.push_back(token);
				}
			}
		}
	}
	else
	{
		while (std::getline(ss, token, delimit))
		{
			if (token != "")
			{
				parameters.push_back(token);
			}
		}
	}

	if (parameters.size() > 0)
	{
		// Replace all the bracketed underscores with spaces again
		shouldReplace = false;
		for (int i = 0; i < parameters.size(); i++)
		{
			bool replaced = false;
			shouldReplace = false;
			for (int k = 0; k < parameters[i].size(); k++)
			{
				if (shouldReplace && parameters[i][k] == '`')
				{
					parameters[i][k] = ' ';
				}
				else if (parameters[i][k] == '[')
				{
					replaced = true;
					shouldReplace = true;
					parameters[i][k] = ' ';
				}
				else if (parameters[i][k] == ']')
				{
					replaced = true;
					shouldReplace = false;
					parameters[i][k] = ' ';
				}
			}

			if (replaced)
			{
				Trim(parameters[i]);
			}
			
		}

		bool commandFound = false;
		for (auto cmd : cmd_lut)
		{
			if (cmd.command == parameters[0])
			{
				commandFound = true;

				try
				{
					int errorCode = (this->*cmd.method)(parameters);

					if (errorCode != 0)
					{
						if (errorCode == -99)
						{
							manager->EndCutscene();
							return true;
						}
						else if (errorCode == -199) //TODO: Use enums
						{
							finished = false;
						}
						else
						{
							std::cout << "ERROR " << errorCode << ": ";
							for (int i = 0; i < parameters.size(); i++)
								std::cout << parameters[i] << " ";
							std::cout << std::endl;
						}
					}
				}
				catch (const std::exception &e)
				{
					std::cout << "EXCEPTION: " << e.what() << std::endl;
					manager->game->logger->Log(e.what());
				}

				break;
			}				
		}

		if (!commandFound)
		{
			// We want to check user-defined functions in here
			for (int k = 0; k < userDefinedFunctions.size(); k++)
			{
				// If function name exists, jump to the label with that name
				if (userDefinedFunctions[k]->functionName == parameters[0])
				{
					commandFound = true;

					bool foundLabel = false;
					for (int i = 0; i < manager->labels.size(); i++)
					{
						if (manager->labels[i]->name == parameters[0])
						{
							foundLabel = true;
							break;
						}
					}

					if (foundLabel)
					{
						//			myfunction 123 [test] 456  <-- parameters
						// defsub	myfunction %0  $45	  %33  <-- definition

						// Grab parameters and place their values in the corresponding variables
						for (int i = 0; i < userDefinedFunctions[k]->parameters.size(); i++)
						{
							int index = GetNumAlias(userDefinedFunctions[k]->parameters[i].substr(1,
								userDefinedFunctions[k]->parameters[i].size() - 1));

							// Set values for each variable as defined in the function definition
							switch (userDefinedFunctions[k]->parameters[i][0])
							{
							case '$':
								stringVariables[index] = ParseStringValue(parameters[i + 1]);
								break;
							case '%':
								numberVariables[index] = ParseNumberValue(parameters[i + 1]);
								break;
							default:
								// Do nothing, maybe error message?
								std::cout << "Invalid parameter definition for " << parameters[0] << " function";
								break;
							}
						}

						GoSubroutine({ parameters[0], parameters[0] });
					}

					break;
				}

			}

			if (!commandFound)
			{
				//std::cout << "ERROR: Command " << parameters[0] << " not found." << std::endl;
			}			
		}
	}

	return finished;
}

int CutsceneCommands::DoNothing(CutsceneParameters parameters)
{
	// Do nothing in this function!
	return 0;
}

int CutsceneCommands::MusicCommand(CutsceneParameters parameters)
{
	//TODO: Deal with custom loop times

	if (parameters[1] == "play")
	{
		manager->game->soundManager->PlayBGM(pathPrefix + ParseStringValue(parameters[2]), true);
	}
	else if (parameters[1] == "once")
	{
		manager->game->soundManager->PlayBGM(pathPrefix + ParseStringValue(parameters[2]), false);
	}
	else if (parameters[1] == "stop")
	{
		manager->game->soundManager->StopBGM();
	}
	else if (parameters[1] == "fadein")
	{
		manager->game->soundManager->FadeInBGM(pathPrefix + ParseStringValue(parameters[2]), ParseNumberValue(parameters[3]), true);
	}
	else if (parameters[1] == "fadeout")
	{
		manager->game->soundManager->FadeOutBGM(ParseNumberValue(parameters[3]));
	}
	else if (parameters[1] == "volume")
	{
		manager->game->soundManager->SetVolumeBGM(ParseNumberValue(parameters[3]));
	}

	return 0;
}

int CutsceneCommands::MusicEffectCommand(CutsceneParameters parameters)
{
	if (parameters[1] == "play")
	{
		//TODO: Deal with multiple channels
		manager->game->soundManager->PlaySound(pathPrefix + ParseStringValue(parameters[2]), ParseNumberValue(parameters[3]), -1);
	}
	else if (parameters[1] == "volume")
	{
		manager->game->soundManager->SetVolumeSound(ParseNumberValue(parameters[2]));
	}

	return 0;
}

int CutsceneCommands::SoundCommand(CutsceneParameters parameters)
{
	if (parameters[1] == "play")
	{
		//TODO: Deal with multiple channels
		manager->game->soundManager->PlaySound(pathPrefix + ParseStringValue(parameters[2]), ParseNumberValue(parameters[3]));
	}
	else if (parameters[1] == "volume")
	{
		manager->game->soundManager->SetVolumeSound(ParseNumberValue(parameters[2]));
	}

	return 0;
}

int CutsceneCommands::IfCondition(CutsceneParameters parameters)
{
	int index = 1;
	bool conditionIsTrue = false;

	do
	{
		bool leftHandIsNumber = false;
		bool rightHandIsNumber = false;

		std::string leftValueStr = "";
		std::string rightValueStr = "";
		int leftValueNum = 0;
		int rightValueNum = 0;

		// 1. First, check the left hand side - is it a string, number, strvar, or numvar?
		// 2. Then, check the operator (>, >=, <, <=, ==, !=)
		// 3. Then, check the right hand side - is it a string, number, strvar, or numvar?
		// 4. If left and right sides are the same type, evaluate the condition
		// 5. If the condition is true, execute the command that follows it
		// 6. If there's an &&, repeat for the next condition, and only execute if all true

		// NOTE: We don't ~really~ need OR because we can just do another IF on the next line
		std::string word = parameters[index];
		switch (parameters[index][0])
		{
		case '$': // string variable
			leftHandIsNumber = false;
			leftValueStr = ParseStringValue(word);
			break;
		case '%': // number variable
			leftHandIsNumber = true;
			leftValueNum = ParseNumberValue(word);
			break;
		default:
			if (parameters[index].find_first_not_of("-0123456789") == std::string::npos)
			{
				leftValueNum = ParseNumberValue(parameters[index]);
				leftHandIsNumber = true;
			}
			else
			{
				leftValueStr = ParseStringValue(parameters[index]);
				leftHandIsNumber = false;
			}

			break;
		}

		index += 2; // skip the operator

		switch (parameters[index][0])
		{
		case '$': // string variable
			rightHandIsNumber = false;
			rightValueStr = ParseStringValue(word);
			break;
		case '%': // number variable
			rightHandIsNumber = true;
			rightValueNum = ParseNumberValue(word);
			break;
		default:
			if (parameters[index].find_first_not_of("-0123456789") == std::string::npos)
			{
				rightValueNum = ParseNumberValue(parameters[index]);
				rightHandIsNumber = true;
			}
			else
			{
				rightValueStr = ParseStringValue(parameters[index]);
				rightHandIsNumber = false;
			}

			break;
		}

		// Don't do any comparison if they are not the same type
		if (leftHandIsNumber != rightHandIsNumber)
			return -1;

		index--; // go back to operator

		if (parameters[index] == ">")
		{
			if (leftHandIsNumber)
			{
				conditionIsTrue = (leftValueNum > rightValueNum);
			}
			else
			{
				conditionIsTrue = (leftValueStr > rightValueStr);
			}
		}
		else if (parameters[index] == ">=")
		{
			if (leftHandIsNumber)
			{
				conditionIsTrue = (leftValueNum >= rightValueNum);
			}
			else
			{
				conditionIsTrue = (leftValueStr >= rightValueStr);
			}
		}
		else if (parameters[index] == "<")
		{
			if (leftHandIsNumber)
			{
				conditionIsTrue = (leftValueNum < rightValueNum);
			}
			else
			{
				conditionIsTrue = (leftValueStr < rightValueStr);
			}
		}
		else if (parameters[index] == "<=")
		{
			if (leftHandIsNumber)
			{
				conditionIsTrue = (leftValueNum <= rightValueNum);
			}
			else
			{
				conditionIsTrue = (leftValueStr <= rightValueStr);
			}
		}
		else if (parameters[index] == "==" || parameters[index] == "=")
		{
			if (leftHandIsNumber)
			{
				conditionIsTrue = (leftValueNum == rightValueNum);
			}
			else
			{
				conditionIsTrue = (leftValueStr == rightValueStr);
			}
		}
		else if (parameters[index] == "!=")
		{
			if (leftHandIsNumber)
			{
				conditionIsTrue = (leftValueNum != rightValueNum);
			}
			else
			{
				conditionIsTrue = (leftValueStr != rightValueStr);
			}
		}

		// If the condition is true, check for other conditions
		if (conditionIsTrue)
		{
			index += 2;

			// If all conditions are true, execute the following commands
			if (parameters[index] != "&&")
			{
				std::string nextCommand = "";
				for (int i = index; i < parameters.size(); i++)
					nextCommand += (parameters[i] + " ");

				// split the command into multiple commands if necessary
				if (nextCommand.find(':') != std::string::npos)
				{
					std::vector<std::string> commands;
					int cmdLetterIndex = 0;
					int cmdLetterLength = 0;
					while (cmdLetterIndex < nextCommand.size())
					{
						cmdLetterIndex++;
						cmdLetterLength++;
						if (nextCommand[cmdLetterIndex] == ':' || cmdLetterIndex >= nextCommand.size())
						{
							if (nextCommand != "" && nextCommand != " ")
							{
								std::string str = nextCommand.substr((cmdLetterIndex - cmdLetterLength), cmdLetterLength);
								commands.emplace_back(str);
								cmdLetterIndex++;
							}
							cmdLetterLength = 0;
						}
					}

					for (int i = 0; i < commands.size(); i++)
					{
						ExecuteCommand(commands[i]);
					}
				}
				else
				{
					// If this is from a choice, don't evaluate any more
					manager->choiceIfStatements.clear();

					ExecuteCommand(nextCommand);
				}

				
			}
			else
			{
				conditionIsTrue = false;
				index++;
			}
		}
		else // else exit, do nothing
		{
			return 0;
		}

	} while (!conditionIsTrue);


	return 0;
}

int CutsceneCommands::DefineUserFunction(CutsceneParameters parameters)
{
	bool functionAlreadyExists = false;
	for (int i = 0; i < userDefinedFunctions.size(); i++)
	{
		if (userDefinedFunctions[i]->functionName == parameters[1])
		{
			functionAlreadyExists = true;
			break;
		}
	}

	//auto it = std::find(userDefinedFunctions.begin(), 
	//	userDefinedFunctions.end(), parameters[1]);
	//auto it = std::find_if(begin(userDefinedFunctions), end(userDefinedFunctions),
	//	[&](auto func) { return func.name == parameters[1]; });

	// If function name does not exist, add it to the list
	if (!functionAlreadyExists)
	{
		UserDefinedFunction* newFunction = new UserDefinedFunction;
		newFunction->functionName = parameters[1];

		for (int i = 2; i < parameters.size(); i++)
			newFunction->parameters.push_back(parameters[i]);

		userDefinedFunctions.push_back(newFunction);
	}

	return 0;
}

int CutsceneCommands::GoSubroutine(CutsceneParameters parameters)
{
	// Save our current spot in the text file
	manager->PushCurrentSceneDataToStack();

	// Jump to the specified label
	GoToLabel(parameters);

	return 0;
}

int CutsceneCommands::ReturnFromSubroutine(CutsceneParameters parameters)
{
	SceneData* data = manager->PopSceneDataFromStack();

	if (data == nullptr)
	{		
		manager->game->logger->Log("ERROR: Nowhere to return to!");
		return -99;
	}

	// Check the label name to see if it is a variable
	const std::string labelName = ParseStringValue(data->labelName);

	manager->currentLabel = manager->JumpToLabel(labelName.c_str());
	if (manager->currentLabel == nullptr)
	{
		if (data->labelIndex < manager->labels.size())
			manager->currentLabel = manager->labels[data->labelIndex];
		else
			std::cout << "ERROR: Could not find label " << labelName << std::endl;
	}

	manager->FlushCurrentColor();

	return 0;
}

//TODO: Change properties of the choices
// (font type, size, color, position, alignment, etc.)
int CutsceneCommands::DisplayChoice(CutsceneParameters parameters)
{
	if (parameters[1] == "bg")
	{
		choiceBGFilePath = ParseStringValue(parameters[2]);
		return 0;
	}

	unsigned int numberOfChoices = ParseNumberValue(parameters[1]);

	int index = 2;
	int spriteNumber = manager->choiceSpriteStartNumber;
	std::string choiceQuestion = ParseStringValue(parameters[index]);

	//TODO: Don't hardcode 1280 x 720
	LoadSprite({ "ld", std::to_string(spriteNumber), choiceBGFilePath, "1280", "720" });

	spriteNumber++;
	int choiceYPos = 280;
	//TODO: Don't hardcode 1280
	LoadText({"", std::to_string(spriteNumber), "1280", std::to_string(choiceYPos), choiceQuestion });
	AlignCommand({ "align", "x", "center", std::to_string(spriteNumber) });
	spriteNumber++;

	manager->choiceIfStatements.clear();
	
	for (int i = 0; i < numberOfChoices; i++)
	{	
		// Get the text and label for the choice
		index++;
		std::string choiceText = ParseStringValue(parameters[index]);
		index++;
		std::string choiceLabel = ParseStringValue(parameters[index]);

		// Display the choice as a text sprite on the screen
		std::string choiceNumber = std::to_string(spriteNumber + i);

		//TODO: Set the y pos relative to the screen resolution, don't hard-code these numbers
		choiceYPos = 400 + (120 * i);

		//TODO: Don't hardcode 1280
		LoadText({"", choiceNumber, "1280",
			std::to_string(choiceYPos), choiceText });

		AlignCommand({ "align", "x", "center", choiceNumber });

		// Make the text sprite act as a button
		SetSpriteButton({ "", choiceNumber , choiceNumber });
		if (i == 0)
		{
			manager->images[std::stoi(choiceNumber)]->SetColor({ 255, 255, 0, 255 });
		}

		// Wait for button input
		//TODO: Why is this number hard-coded to 42?
		std::string variableNumber = std::to_string(42);
		WaitForButton({ "",  variableNumber });

		// Construct the if-statement and store it
		//if %42 == 21 goto label_left ;

		manager->choiceIfStatements.push_back("if %" + variableNumber + " == " + choiceNumber + " goto " + choiceLabel + " ;");

		manager->inputTimer.Start(manager->inputTimeToWait);
	}

	return 0;
}

int CutsceneCommands::WaitForButton(CutsceneParameters parameters)
{
	// If there are no active buttons, you can't wait for a button
	if (manager->activeButtons.size() > 0)
	{
		// Get the variable number to store the result in
		manager->buttonResult = ParseNumberValue(parameters[1]);

		/* OLD
		if (parameters[1][0] == '%')
			manager->buttonResult = GetNumberVariable(GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1)));
		else
			manager->buttonResult = GetNumAlias(parameters[1]);
			*/

		// Change the state of the game to wait until a button has been pressed
		manager->waitingForButton = true;

		// Set the first button as highlighted
		manager->buttonIndex = 0;
		manager->images[manager->activeButtons[manager->buttonIndex]]->
			GetSprite()->color = { 255, 255, 0, 255 };

		manager->isCarryingOutCommands = true;
		manager->isReadingNextLine = true;
		manager->textbox->isReading = false;
		manager->textbox->text->SetText("");
	}

	return 0;
}

int CutsceneCommands::SetSpriteButton(CutsceneParameters parameters)
{
	unsigned int spriteNumber = ParseNumberValue(parameters[1]);
	unsigned int buttonNumber = ParseNumberValue(parameters[2]);

	manager->spriteButtons[spriteNumber] = buttonNumber;

	manager->activeButtons.push_back(spriteNumber);

	return 0;
}

int CutsceneCommands::GoToLabel(CutsceneParameters parameters)
{
	if (parameters[1][0] == '*') // remove leading * if there is one
	{
		manager->PlayCutscene(ParseStringValue(parameters[1].substr(1, parameters[1].size() - 1)).c_str());
	}
	else
	{
		// Check the label name to see if it is a variable
		manager->PlayCutscene(ParseStringValue(parameters[1]).c_str());
	}

	return 0;
}

int CutsceneCommands::JumpBack(CutsceneParameters parameters)
{
	// Search all game states going backwards until we find a ~ in the command list
	// If not found, do nothing and carry on as normal
	// Else, jump to that label and set command index to after the ~
	manager->JumpBack();
	return 0;
}

int CutsceneCommands::JumpForward(CutsceneParameters parameters)
{
	// Search all game states going forwards until we find a ~ in the command list
	// If not found, do nothing and carry on as normal
	// Else, jump to that label and set command index to after the ~
	manager->JumpForward();
	return 0;
}

int CutsceneCommands::EndGame(CutsceneParameters parameters)
{
	manager->game->shouldQuit = true;
	return 0;
}

int CutsceneCommands::ResetGame(CutsceneParameters parameters)
{
	manager->ClearAllSprites();
	// TODO: Clear BG
	manager->game->soundManager->StopBGM();
	// TODO: Stop all sound channels
	manager->PlayCutscene("start");
	return 0;
}

int CutsceneCommands::SaveGame(CutsceneParameters parameters)
{
	//TODO: Save all of these to a file:

	// Scene data, string/number variables, random seed, object information, user defined functions and aliases, settings, etc.
	// Possibly could simplify this by storing some things that won't change in a config file (functions, aliases)
	// For example, on the game's startup we load the config file and read it in
	// Then, when the player loads a save file, we don't have to deal with the stuff in the config file

	// savegame saves/ file1.sav
	// savegame file1.sav
	// savegame

	if (parameters.size() > 2)
		manager->SaveGame(parameters[2].c_str(), parameters[1].c_str());
	else if (parameters.size() > 1)
		manager->SaveGame(parameters[1].c_str());
	else
		manager->SaveGame("file1.sav");

	return 0;
}

int CutsceneCommands::LoadGame(CutsceneParameters parameters)
{
	//TODO: Load everything that was saved from a file

	if (parameters.size() > 2)
		manager->LoadGame(parameters[2].c_str(), parameters[1].c_str());
	else if (parameters.size() > 1)
		manager->LoadGame(parameters[1].c_str());
	else
		manager->LoadGame("file1.sav");

	return 0;
}

// If the parameter starts with a % sign 
// - if it is followed by a string, get the number associated with the string,
// and get the value of the variable of that number
// - if it is followed by a number, just get the value of the variable of that number
// otherwise, if it is a string, get the number associated with the string,
// or if it is a number, just use the number

//TODO: Should be able to add two strings together
int CutsceneCommands::ConcatenateStringVariables(CutsceneParameters parameters)
{
	unsigned int key = 0;
	std::string word1 = "";
	std::string word2 = "";
	
	if (parameters[1][0] == '$')
		key = GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1));
	else
		key = GetNumAlias(parameters[1]);

	word1 = ParseStringValue(parameters[1]);
	word2 = ParseStringValue(parameters[2]);

	stringVariables[key] = word1 + word2;

	// If global variable, save change to file
	if (key >= manager->globalStart)
		manager->SaveGlobalVariable(key, stringVariables[key], true);

	return 0;
}

//TODO: Check and see if both parameters are strings.
// If so, then call the function that adds strings together
int CutsceneCommands::AddNumberVariables(CutsceneParameters parameters)
{
	if (parameters[1][0] == '$')
	{
		ConcatenateStringVariables({ "concat", parameters[1], parameters[2] });
		return 0;
	}

	unsigned int key = 0;
	unsigned int number1 = 0;
	unsigned int number2 = 0;
	
	if (parameters[1][0] == '%')
		key = GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1));
	else
		key = GetNumAlias(parameters[1]);

	number1 = ParseNumberValue(parameters[1]);
	number2 = ParseNumberValue(parameters[2]);		
	
	numberVariables[key] = number1 + number2;

	// If global variable, save change to file
	if (key >= manager->globalStart)
		manager->SaveGlobalVariable(key, std::to_string(numberVariables[key]), true);

	return 0;
}

int CutsceneCommands::SubtractNumberVariables(CutsceneParameters parameters)
{
	unsigned int key = 0;
	unsigned int number1 = 0;
	unsigned int number2 = 0;

	key = GetNumAlias(parameters[1]);
	if (parameters[1][0] == '%')
		key = GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1));

	number1 = ParseNumberValue(parameters[1]);
	number2 = ParseNumberValue(parameters[2]);

	numberVariables[key] = number1 - number2;

	// If global variable, save change to file
	if (key >= manager->globalStart)
		manager->SaveGlobalVariable(key, std::to_string(numberVariables[key]), true);

	return 0;
}

int CutsceneCommands::MultiplyNumberVariables(CutsceneParameters parameters)
{
	unsigned int key = 0;
	unsigned int number1 = 0;
	unsigned int number2 = 0;

	key = GetNumAlias(parameters[1]);
	if (parameters[1][0] == '%')
		key = GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1));

	number1 = ParseNumberValue(parameters[1]);
	number2 = ParseNumberValue(parameters[2]);

	numberVariables[key] = number1 * number2;

	// If global variable, save change to file
	if (key >= manager->globalStart)
		manager->SaveGlobalVariable(key, std::to_string(numberVariables[key]), true);

	return 0;
}

int CutsceneCommands::DivideNumberVariables(CutsceneParameters parameters)
{
	unsigned int key = 0;
	unsigned int number1 = 0;
	unsigned int number2 = 0;

	key = GetNumAlias(parameters[1]);
	if (parameters[1][0] == '%')
		key = GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1));

	number1 = ParseNumberValue(parameters[1]);
	number2 = ParseNumberValue(parameters[2]);

	numberVariables[key] = number1 / number2;

	// If global variable, save change to file
	if (key >= manager->globalStart)
		manager->SaveGlobalVariable(key, std::to_string(numberVariables[key]), true);

	return 0;
}

int CutsceneCommands::ModNumberVariables(CutsceneParameters parameters)
{
	unsigned int key = 0;
	unsigned int number1 = 0;
	unsigned int number2 = 0;

	key = GetNumAlias(parameters[1]);
	if (parameters[1][0] == '%')
		key = GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1));

	number1 = ParseNumberValue(parameters[1]);
	number2 = ParseNumberValue(parameters[2]);

	numberVariables[key] = number1 % number2;

	// If global variable, save change to file
	if (key >= manager->globalStart)
		manager->SaveGlobalVariable(key, std::to_string(numberVariables[key]), true);

	return 0;
}

int CutsceneCommands::RandomNumberVariable(CutsceneParameters parameters)
{
	if (parameters[1] == "seed")
	{
		if (parameters[2] == "time")
		{
			randomSeed = (int)time(0);
			srand(randomSeed);
		}
		else
		{
			randomSeed = ParseNumberValue(parameters[2]);
			srand(randomSeed);
		}
	}
	else if (parameters[1] == "range")
	{
		unsigned int key = ParseNumberValue(parameters[2]);
		unsigned int minNumber = ParseNumberValue(parameters[3]);
		unsigned int maxNumber = ParseNumberValue(parameters[4]);

		numberVariables[key] = (rand() % maxNumber) + minNumber;

		// If global variable, save change to file
		if (key >= manager->globalStart)
			manager->SaveGlobalVariable(key, std::to_string(numberVariables[key]), true);
	}
	else // no offset
	{
		unsigned int key = ParseNumberValue(parameters[1]);
		unsigned int maxNumber = ParseNumberValue(parameters[2]);

		numberVariables[key] = (rand() % maxNumber);

		// If global variable, save change to file
		if (key >= manager->globalStart)
			manager->SaveGlobalVariable(key, std::to_string(numberVariables[key]), true);
	}

	return 0;
}

int CutsceneCommands::SubstringVariables(CutsceneParameters parameters)
{
	unsigned int key = 0;

	if (parameters[1][0] == '$')
	{
		key = numalias[parameters[1].substr(1, parameters[1].size() - 1)];
	}
	else
	{
		key = numalias[parameters[1]];
	}

	std::string value = ParseStringValue(parameters[2]);

	stringVariables[key] = value.substr(std::stoi(parameters[3]), std::stoi(parameters[4]));

	return 0;
}

int CutsceneCommands::MoveVariables(CutsceneParameters parameters)
{
	if (parameters[1][0] == '$')
	{
		SetStringVariable({ "mov", parameters[1].substr(1, parameters[1].size() - 1), parameters[2] });
	}
	else if (parameters[1][0] == '%')
	{
		SetNumberVariable({ "mov", parameters[1].substr(1, parameters[1].size() - 1), parameters[2] });
	}
	else
	{
		return -1;
	}

	return 0;
}

int CutsceneCommands::SetNumberVariable(CutsceneParameters parameters)
{
	unsigned int key = ParseNumberValue(parameters[1]);
	unsigned int value = ParseNumberValue(parameters[2]);

	if (parameters.size() > 3 && parameters[3] == "no_alias")
	{
		value = std::stoi(parameters[2]);
	}

	numberVariables[key] = value;

	// If global variable, save change to file
	if (key >= manager->globalStart)
		manager->SaveGlobalVariable(key, std::to_string(numberVariables[key]), true);

	return 0;
}

int CutsceneCommands::SetStringVariable(CutsceneParameters parameters)
{
	unsigned int key = ParseNumberValue(parameters[1]);

	std::string value = parameters[2];
	if (value[0] == '[')
	{
		int varNameIndex = 1;
		value = ParseWord(value, ']', varNameIndex);
	}
	else
	{
		value = ParseStringValue(value);
	}

	stringVariables[key] = value;

	// If global variable, save change to file
	if (key >= manager->globalStart)
		manager->SaveGlobalVariable(key, stringVariables[key], false);

	return 0;
}

int CutsceneCommands::GetNumberVariable(const unsigned int key)
{
	if (numberVariables.find(key) == numberVariables.end())
	{
		return key;
	}
	else
	{
		return numberVariables[key];
	}
}

std::string CutsceneCommands::GetStringVariable(const unsigned int key)
{
	if (stringVariables.find(key) == stringVariables.end())
	{
		return "";
	}
	else
	{
		return stringVariables[key];
	}
}

int CutsceneCommands::SetStringAlias(CutsceneParameters parameters)
{
	stralias[parameters[1]] = parameters[2];
	return 0;
}

std::string CutsceneCommands::GetStringAlias(const std::string& key)
{
	if (stralias.find(key) == stralias.end())
	{
		return key;
	}
	else
	{
		return stralias[key];
	}
}

int CutsceneCommands::SetNumAlias(CutsceneParameters parameters)
{
	numalias[parameters[1]] = ParseNumberValue(parameters[2]);
	return 0;
}

std::string CutsceneCommands::ParseStringValue(const std::string& parameter)
{
	std::string value = "";

	// Get the variable number to store the result in
	if (parameter[0] == '$')
		value = GetStringVariable(GetNumAlias(parameter.substr(1, parameter.size() - 1)));
	else
		value = GetStringAlias(parameter);

	value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());

	return value;
}

int CutsceneCommands::ParseNumberValue(const std::string& parameter)
{
	int value = 0;

	// Get the variable number to store the result in
	if (parameter[0] == '%' || parameter[0] == '$')
		value = GetNumberVariable(GetNumAlias(parameter.substr(1, parameter.size() - 1)));
	else
		value = GetNumAlias(parameter);

	return value;
}

int CutsceneCommands::GetNumAlias(const std::string& key)
{
	if (numalias.find(key) == numalias.end())
	{
		try
		{
			return std::stoi(key);
		}
		catch (const std::exception& ex)
		{			
			std::cout << "ERROR: Numalias not defined for " << key << std::endl;
			std::cout << ex.what() << std::endl;
			return 0; //TODO: Should this be changed to -1?
		}
	}
	else
	{
		return numalias[key];
	}
}

int CutsceneCommands::LoadBackground(CutsceneParameters parameters)
{
	LoadSprite({ "", parameters[0], parameters[1] });

	return 0;
}

int CutsceneCommands::ClearSprite(CutsceneParameters parameters)
{
	if (parameters[1] == "a")
	{
		manager->ClearAllSprites();
	}
	else
	{
		unsigned int imageNumber = ParseNumberValue(parameters[1]);

		if (manager->images[imageNumber] != nullptr)
			delete manager->images[imageNumber];

		manager->images[imageNumber] = nullptr;
	}

	return 0;
}

int CutsceneCommands::LoadSprite(CutsceneParameters parameters)
{
	Vector2 pos = Vector2(0, 0);

	bool isStandingImage = parameters[1] == "l" || parameters[1] == "c" || parameters[1] == "r";

	if (!isStandingImage && parameters[1] != "bg")
	{
		const unsigned int x = ParseNumberValue(parameters[3]);
		const unsigned int y = ParseNumberValue(parameters[4]);
		pos = Vector2(x, y);
	}

	std::string filepath = pathPrefix + ParseStringValue(parameters[2]);
	unsigned int imageNumber = ParseNumberValue(parameters[1]);

	//TODO: Don't delete/new, just grab from entity pool and reset
	if (manager->images[imageNumber] != nullptr)
		delete manager->images[imageNumber];

	manager->images[imageNumber] = new Entity(pos);

	manager->images[imageNumber]->SetSprite(new Sprite(1, manager->game->spriteManager,
		filepath, manager->game->renderer->shaders[ShaderName::Default], Vector2(0, 0)));

	if (isStandingImage)
	{
		int halfScreenWidth = ((manager->game->screenWidth * 2) / 2);
		int spriteX = 0; // (manager->game->screenWidth / 5) * 3;
		int spriteY = manager->game->screenHeight;

		switch (parameters[1][0])
		{
			case 'l':
				spriteX = halfScreenWidth - (halfScreenWidth / 2);
				spriteY = (manager->game->screenHeight * 2) - 
					(manager->images[imageNumber]->GetSprite()->frameHeight);

				pos = Vector2(spriteX + manager->game->renderer->guiCamera.position.x,
					spriteY + manager->game->renderer->guiCamera.position.y);
				break;
			case 'c':
				spriteX = halfScreenWidth; // +(sprites['c']->frameWidth / 2);
				spriteY = (manager->game->screenHeight * 2) -
					(manager->images[imageNumber]->GetSprite()->frameHeight);

				pos = Vector2(spriteX + manager->game->renderer->guiCamera.position.x,
					spriteY + manager->game->renderer->guiCamera.position.y);

				break;
			case 'r':
				spriteX = halfScreenWidth + (halfScreenWidth / 2);
				spriteY = (manager->game->screenHeight * 2) -
					(manager->images[imageNumber]->GetSprite()->frameHeight);

				pos = Vector2(spriteX + manager->game->renderer->guiCamera.position.x,
					spriteY + manager->game->renderer->guiCamera.position.y);

				break;
			default:
				break;
		}
		manager->images[imageNumber]->SetPosition(pos);
	}
	else if (parameters[1] == "bg")
	{
		manager->ClearAllSprites();

		int halfScreenWidth = ((manager->game->screenWidth * 2) / 2);
		int spriteY = manager->game->screenHeight;

		int spriteX = halfScreenWidth; // +(sprites['c']->frameWidth / 2);
		spriteY = (manager->game->screenHeight * 2) -
			(manager->images[imageNumber]->GetSprite()->frameHeight);

		pos = Vector2(spriteX + manager->game->renderer->guiCamera.position.x,
			spriteY + manager->game->renderer->guiCamera.position.y);

		manager->images[imageNumber]->SetPosition(pos);
	}

	manager->images[imageNumber]->drawOrder = imageNumber;
	manager->images[imageNumber]->GetSprite()->keepPositionRelativeToCamera = true;
	manager->images[imageNumber]->GetSprite()->keepScaleRelativeToCamera = true;

	return 0;
}

int CutsceneCommands::LoadTextFromSaveFile(CutsceneParameters parameters)
{
	unsigned int imageNumber = ParseNumberValue(parameters[0]);
	const unsigned int x = ParseNumberValue(parameters[2]);
	const unsigned int y = ParseNumberValue(parameters[3]);
	Vector2 pos = Vector2(x, y);

	glm::vec3 rotation = glm::vec3(
		std::stoi(parameters[4]),
		std::stoi(parameters[5]),
		std::stoi(parameters[6]));

	Vector2 scale = Vector2(std::stoi(parameters[7]), std::stoi(parameters[8]));

	std::string text = parameters[9];

	int index = 10;
	if (text[0] == '[')
	{		
		while (parameters[index][0] != ']')
		{
			text += " " + parameters[index];
			index++;
		}		
	}
	index++;

	Color textColor = {
		(uint8_t)std::stoi(parameters[index]),
		(uint8_t)std::stoi(parameters[index + 1]),
		(uint8_t)std::stoi(parameters[index + 2]),
		(uint8_t)std::stoi(parameters[index + 3]),
	};

	if (manager->images[imageNumber] != nullptr)
		delete manager->images[imageNumber];

	//TODO: Also save/load in the font type/size/style for this text object
	Text* newText = new Text(manager->game->renderer, manager->game->theFont, text, textColor);

	newText->isRichText = false;

	manager->images[imageNumber] = newText;

	manager->images[imageNumber]->SetPosition(pos);
	manager->images[imageNumber]->rotation = rotation;
	manager->images[imageNumber]->scale = scale;
	manager->images[imageNumber]->drawOrder = imageNumber;
	manager->images[imageNumber]->GetSprite()->keepPositionRelativeToCamera = true;
	manager->images[imageNumber]->GetSprite()->keepScaleRelativeToCamera = true;

	return 0;
}

int CutsceneCommands::LoadText(CutsceneParameters parameters)
{
	Vector2 pos = Vector2(0, 0);

	// text 9 [Hello, world!] 0 0 ;
	//TODO: Make sure text color works (#)
	//TODO: Make sure variables work (%, $)

	unsigned int imageNumber = ParseNumberValue(parameters[1]);
	const unsigned int x = ParseNumberValue(parameters[2]);
	const unsigned int y = ParseNumberValue(parameters[3]);
	pos = Vector2(x, y);

	std::string text = parameters[4];	
	for(int i = 5; i < parameters.size(); i++)
		text += (parameters[i]);

	//TODO: Don't delete/new, just grab from entity pool and reset
	if (manager->images[imageNumber] != nullptr)
		delete manager->images[imageNumber];

	Text* newText = nullptr;
	Color textColor = { 255, 255, 255, 255 };

	int letterIndex = 0;
	std::string finalText = "";

	newText = new Text(manager->game->renderer, manager->game->theFont, "", textColor);
	newText->SetPosition(pos.x, pos.y); // use the Text SetPosition function, not Entity
	newText->isRichText = true;

	while (letterIndex < text.size())
	{		
		finalText = manager->ParseText(text, letterIndex, textColor, newText);
		for (int i = 0; i < finalText.size(); i++)
		{
			newText->AddText(finalText[i], textColor);
			newText->SetPosition(pos.x, pos.y);
		}
	}
	
	manager->images[imageNumber] = newText;
	manager->images[imageNumber]->drawOrder = imageNumber;
	manager->images[imageNumber]->GetSprite()->keepPositionRelativeToCamera = true;
	manager->images[imageNumber]->GetSprite()->keepScaleRelativeToCamera = true;

	// Color the text yellow when we hover the mouse over it or select with keyboard
	//manager->images[imageNumber]->GetSprite()->color = { 255, 255, 0, 255 } ;

	return 0;
}

// Parameters is the list of RGBA values,
// OR it only contains one string, which is the hexadecimal string
Color CutsceneCommands::ParseColorFromParameters(const std::vector<std::string>& parameters, const int index)
{
	Color color = { 255, 255, 255, 255 };
	std::vector<std::string> colorParams { begin(parameters) + index, end(parameters) };

	if (colorParams[0][0] == '#') // check to see if it is hexadecimal
	{
		color = ParseColorHexadecimal(colorParams[0].c_str());
	}
	else // then it must be RGB or RGBA decimal such as 255 255 255 or 255 255 255 255
	{
		// Note: blue and red are swapped for endianness
		color.b = ParseNumberValue(colorParams[0]);
		color.g = ParseNumberValue(colorParams[1]);
		color.r = ParseNumberValue(colorParams[2]);

		if (colorParams.size() > 3)
			color.a = ParseNumberValue(colorParams[3]);
	};

	return color;
}

// Assign color of text to a speaking character
int CutsceneCommands::TextColor(CutsceneParameters parameters)
{
	std::string characterName = parameters[1];

	//TODO: Error checking?
	Color color = ParseColorFromParameters(parameters, 2);

	if (characterName == "default")
		manager->namesToColors[""] = color;
	else
		manager->namesToColors[characterName] = color;

	manager->FlushCurrentColor();

	return 0;
}

int CutsceneCommands::SetSpriteProperty(CutsceneParameters parameters)
{
	unsigned int imageNumber = ParseNumberValue(parameters[1]);

	//TODO: Maybe make a manager->GetImage(imageNumber) function for error handling
	Entity* entity = manager->images[imageNumber];
	if (entity == nullptr)
		return 1; //TODO: Error log

	//Sprite* sprite = manager->images[imageNumber]->GetSprite();
	if (entity->GetSprite() == nullptr)
		return 2; //TODO: Error log

	const std::string spriteProperty = ParseStringValue(parameters[2]);

	if (spriteProperty == "color")
	{
		Color color = { (uint8_t)ParseNumberValue(parameters[3]), 
			(uint8_t)ParseNumberValue(parameters[4]),
			(uint8_t)ParseNumberValue(parameters[5]), 
			(uint8_t)ParseNumberValue(parameters[6]) };

		entity->GetSprite()->color = color;
	}
	else if (spriteProperty == "scale")
	{
		// Because animators have different sprites for each animation state,
		// we want to change the scale of the entity and then apply that scale
		// to whatever sprite is currently being animated
		entity->scale = Vector2(ParseNumberValue(parameters[3]), ParseNumberValue(parameters[4]));
		entity->SetSprite(entity->GetSprite());
	}
	else if (spriteProperty == "rotate")
	{
		entity->rotation = glm::vec3(ParseNumberValue(parameters[3]), 
			ParseNumberValue(parameters[4]), ParseNumberValue(parameters[5]));
		entity->SetSprite(entity->GetSprite());
	}
	else if (spriteProperty == "shader")
	{	
		//TODO: Fix this so that it works with enums
		//if (manager->game->renderer->GetShaderFromString(parameters[3]) != nullptr)
		//	sprite->shader = manager->game->renderer->GetShaderFromString(parameters[3]);
		//TODO: Log and display error if cannot find shader?
	}
	else if (spriteProperty == "animator")
	{
		const std::string animAction = ParseStringValue(parameters[3]);

		if (animAction == "=") // set the sprite's animator equal to this one
		{
			if (entity->GetAnimator() != nullptr)
				delete entity->GetAnimator();

			Sprite* test = entity->GetSprite();

			std::vector<AnimState*> animStates;
			manager->game->spriteManager->ReadAnimData(ParseStringValue(parameters[4]), animStates);

			for (int i = 0; i < animStates.size(); i++)
			{
				animStates[i]->sprite->keepPositionRelativeToCamera = true;
				animStates[i]->sprite->keepScaleRelativeToCamera = true;
			}

			Animator* newAnim = new Animator("player", animStates, ParseStringValue(parameters[5]));
			entity->SetAnimator(newAnim);
		}
		else if (entity->GetAnimator() == nullptr)
		{
			std::cout << "Error, sprite " << imageNumber << " does not have animator" << std::endl;
		}
		else if (animAction == "state") // change the animator's state
		{
			entity->GetAnimator()->SetState(parameters[4].c_str());
		}
		else if (animAction == "bool") // change the animator's bool var
		{
			int num = ParseNumberValue(parameters[5]);

			//TODO: Don't just hardcode these values, make it work with cutscene vars too
			if (parameters[5] == "true" || parameters[5] == "True" || num > 0)
				entity->GetAnimator()->SetBool(parameters[4].c_str(), true);
			else
				entity->GetAnimator()->SetBool(parameters[4].c_str(), false);
		}
		else if (animAction == "int") // change the animator's int var
		{
			entity->GetAnimator()->SetInt(parameters[4].c_str(), ParseNumberValue(parameters[5]));
		}
		else if (animAction == "float") // change the animator's float var
		{
			//TODO: This will not work because the parse function doesn't get floats
			entity->GetAnimator()->SetInt(parameters[4].c_str(), ParseNumberValue(parameters[5]));
		}
	}

	return 0;
}

//TODO: Maybe put this code somewhere so it can be used
// both by the cutscene system and the level editor properties?
int CutsceneCommands::SetVelocity(CutsceneParameters parameters)
{
	PhysicsComponent* entity = nullptr;

	for (unsigned int i = 0; i < manager->game->entities.size(); i++)
	{
		if (manager->game->entities[i]->name == parameters[1])
		{
			entity = dynamic_cast<PhysicsComponent*>(manager->game->entities[i]);

			if (entity != nullptr)
			{
				unsigned int x = ParseNumberValue(parameters[2]);
				unsigned int y = ParseNumberValue(parameters[3]);
				entity->SetVelocity(Vector2(x * 0.001f, y * 0.001f));
			}
			break;
		}
	}

	return 0;
}

int CutsceneCommands::Wait(CutsceneParameters parameters)
{
	manager->msGlyphTime -= ParseNumberValue(parameters[1]); // wait for a certain amount of time (milliseconds)
	manager->textbox->isReading = false; // don't render the textbox while waiting
	return 0;
}

// namedef but Butler
// namedef bu2 Butler
// TODO: How to handle multiple textboxes? Add a function for setting the speaker's text?
int CutsceneCommands::NameDefineCommand(CutsceneParameters parameters)
{
	manager->namesToNames[ParseStringValue(parameters[1])] = ParseStringValue(parameters[2]);
	return 0;
}

// name NAME
int CutsceneCommands::NameCommand(CutsceneParameters parameters)
{
	manager->overwriteName = false;
	manager->SetSpeakerText(ParseStringValue(parameters[1]));

	SceneLine* line = manager->GetCurrentLine();
	if (line != nullptr)
	{
		line->speaker = ParseStringValue(parameters[1]);
	}

	return 0;
}

int CutsceneCommands::Namebox(CutsceneParameters parameters)
{
	if (parameters[1] == "on")
	{
		manager->textbox->shouldRender = true;
	}
	else if (parameters[1] == "off")
	{
		manager->textbox->shouldRender = false;
	}
	else if (parameters[1] == "text")
	{
		if (parameters[2] == "color")
		{
			manager->textbox->speaker->GetSprite()->color = ParseColorFromParameters(parameters, 2);
		}
		else if (parameters[2] == "contents")
		{
			//TODO: This probably won't work well because the text will be overwritten
			// once all the commands are finished executing.
			manager->textbox->speaker->SetText(ParseStringValue(parameters[2]));
		}
		else if (parameters[2] == "font")
		{
			//TODO: Add a separate command for loading new fonts?
			if (parameters[3] == "type")
			{
				manager->textbox->ChangeNameFont(ParseStringValue(parameters[4]));
			}
			else if (parameters[3] == "size")
			{

			}
			else if (parameters[3] == "style")
			{
				// probably just do this using text tags < >
			}
		}
	}
	else if (parameters[1] == "color")
	{
		manager->textbox->nameObject->GetSprite()->color = ParseColorFromParameters(parameters, 2);
	}
	else if (parameters[1] == "position")
	{
		Vector2 newPos = Vector2(ParseNumberValue(parameters[2]), ParseNumberValue(parameters[3]));
		manager->textbox->speaker->SetPosition(newPos.x, newPos.y);
		manager->textbox->nameObject->SetPosition(newPos);
	}
	else if (parameters[1] == "sprite")
	{
		manager->textbox->ChangeNameSprite(ParseStringValue(parameters[2]));
	}

	return 0;
}

int CutsceneCommands::Textbox(CutsceneParameters parameters)
{
	if (parameters[1] == "on")
	{
		manager->textbox->shouldRender = true;
	}
	else if (parameters[1] == "off")
	{
		manager->textbox->shouldRender = false;
	}
	else if (parameters[1] == "text")
	{
		if (parameters[2] == "color")
		{
			manager->textbox->text->GetSprite()->color = ParseColorFromParameters(parameters, 2);
		}
		else if (parameters[2] == "contents")
		{
			//TODO: This probably won't work well because the text will be overwritten
			// once all the commands are finished executing.
			manager->textbox->text->SetText(ParseStringValue(parameters[2]));
		}
		else if (parameters[2] == "font")
		{
			//TODO: Add a separate command for loading new fonts?
			if (parameters[3] == "type")
			{
				manager->textbox->ChangeBoxFont(ParseStringValue(parameters[4]));
			}
			else if (parameters[3] == "size")
			{
				manager->textbox->currentFontInfo->ChangeFontSize(ParseNumberValue(parameters[4]));
				//manager->textbox->textFont = manager->textbox->currentFontInfo->GetRegularFont();
			}
			else if (parameters[3] == "style")
			{
				// probably just do this using text tags < >
			}
		}
	}
	else if (parameters[1] == "color")
	{
		manager->textbox->boxObject->GetSprite()->color = ParseColorFromParameters(parameters, 2);
	}
	else if (parameters[1] == "position")
	{
		Vector2 newPos = Vector2(ParseNumberValue(parameters[2]), ParseNumberValue(parameters[3]));
		manager->textbox->text->SetPosition(newPos.x, newPos.y);
		manager->textbox->boxObject->SetPosition(newPos);
	}
	else if (parameters[1] == "sprite")
	{
		manager->textbox->ChangeBoxSprite(ParseStringValue(parameters[2]));
	}

	return 0;
}

int CutsceneCommands::Fade(CutsceneParameters parameters)
{
	manager->game->renderer->changingOverlayColor = true;

	if (parameters[1] == "clear")
	{
		manager->game->renderer->targetColor = Color { 0, 0, 0, 0 };
	}
	else if (parameters[1] == "white")
	{
		manager->game->renderer->targetColor = Color{ 255, 255, 255, 255 };
	}
	else if (parameters[1] == "black")
	{
		manager->game->renderer->targetColor = Color{0, 0, 0, 255 };
	}
	else
	{
		if (parameters.size() > 5)
		{
			manager->game->renderer->targetColor = { 
				(uint8_t)ParseNumberValue(parameters[2]),
				(uint8_t)ParseNumberValue(parameters[3]),
				(uint8_t)ParseNumberValue(parameters[4]),
				(uint8_t)ParseNumberValue(parameters[5])};
		}
		else
		{
			manager->game->renderer->targetColor = { 
				(uint8_t)ParseNumberValue(parameters[2]),
				(uint8_t)ParseNumberValue(parameters[3]),
				(uint8_t)ParseNumberValue(parameters[4]),
				255 };
		}

	}

	return 0;
}

int CutsceneCommands::SetResolution(CutsceneParameters parameters)
{
	//TODO: Maybe place this command in a config file to start the window in a certain resolution?
	const int width = ParseNumberValue(parameters[1]);
	const int height = ParseNumberValue(parameters[2]);

	manager->game->SetScreenResolution(width, height);
	
	return 0;
}

// This number indicates the first global variable slot
// All variable slots before this number are local
// Local variables are only saved within the current save file
// Global variables are saved across all save files
int CutsceneCommands::SetGlobalNumber(CutsceneParameters parameters)
{
	manager->globalStart = ParseNumberValue(parameters[1]);
	return 0;
}

int CutsceneCommands::OpenBacklog(CutsceneParameters parameters)
{
	if (parameters.size() > 1)
	{
		if (parameters[1] == "size")
		{
			manager->backlogMaxSize = ParseNumberValue(parameters[2]);
			//TODO: Save this to a settings file
		}
		else if (parameters[1] == "pages")
		{
			manager->backlogMaxSize = ParseNumberValue(parameters[2]);
		}
		else if (parameters[1] == "open")
		{
			manager->readingBacklog = true;
			manager->backlogIndex = manager->backlog.size() - 1;
			manager->ReadBacklog();
		}
		else if (parameters[1] == "enable")
		{
			manager->backlogEnabled = true;
		}
		else if (parameters[1] == "disable")
		{
			manager->backlogEnabled = false;
		}
		else if (parameters[1] == "color") //TODO: Should this be the same for both name and text?
		{
			if (parameters[2][0] == '#')
			{
				manager->backlogColor = ParseColorHexadecimal(parameters[2].c_str());
			}
			else if (parameters.size() == 5)
			{
				manager->backlogColor = { 
					(uint8_t)ParseNumberValue(parameters[4]),
					(uint8_t)ParseNumberValue(parameters[3]),
					(uint8_t)ParseNumberValue(parameters[2]),
					255 };
			}
			else if(parameters.size() == 6)
			{
				manager->backlogColor = { 
					(uint8_t)ParseNumberValue(parameters[4]),
					(uint8_t)ParseNumberValue(parameters[3]),
					(uint8_t)ParseNumberValue(parameters[2]),
					(uint8_t)ParseNumberValue(parameters[5])};
			}
		}
	}

	return 0;
}

int CutsceneCommands::FlipSprite(CutsceneParameters parameters)
{
	if (parameters.size() > 2)
	{		
		int spriteNum = ParseNumberValue(parameters[1]);
		Vector2 scale = manager->images[spriteNum]->scale;

		std::string direction = ParseStringValue(parameters[2]);

		if (direction == "h" || direction == "horizontal")
		{			
			SetSpriteProperty({ "", parameters[1], "scale", std::to_string(-scale.x), "1"});
		}
		else if (direction == "v" || direction == "vertical")
		{
			SetSpriteProperty({ "", parameters[1], "scale", "1", std::to_string(-scale.y) });
		}
		else if (direction == "b" || direction == "both")
		{
			SetSpriteProperty({ "", parameters[1], "scale", std::to_string(-scale.x), std::to_string(-scale.y) });
		}
	}
	
	return 0;
}

int CutsceneCommands::TimerFunction(CutsceneParameters parameters)
{
	//TODO: Display errors for syntax errors rather than crashing

	unsigned int timerNumber = ParseNumberValue(parameters[2]);

	if (parameters[1] == "start")
	{		
		if (manager->timers.count(timerNumber) != 1)
			manager->timers[timerNumber] = new Timer();			

		unsigned int timerDuration = ParseNumberValue(parameters[3]);
		manager->timers[timerNumber]->Start(timerDuration);
	}
	else if (parameters[1] == "pause")
	{
		if (manager->timers.count(timerNumber) == 1)
		{
			manager->timers[timerNumber]->Pause();
		}
	}
	else if (parameters[1] == "unpause")
	{
		if (manager->timers.count(timerNumber) == 1)
		{
			manager->timers[timerNumber]->Unpause();
		}
	}
	else if (parameters[1] == "reset")
	{
		if (manager->timers.count(timerNumber) == 1)
		{
			manager->timers[timerNumber]->Reset();
		}
	}
	else if (parameters[1] == "elapsed") //TODO: Change this word?
	{
		if (manager->timers.count(timerNumber) == 1)
		{
			// Get the amount of time that has elapsed since the timer started
			SetNumberVariable({ "", parameters[3], std::to_string(manager->timers[timerNumber]->GetTicks()) });
		}
	}
	else if (parameters[1] == "delete")
	{
		if (manager->timers.count(timerNumber) == 1)
		{
			delete manager->timers[timerNumber];
			manager->timers.erase(timerNumber);
		}
	}

	return 0;
}

int CutsceneCommands::CameraFunction(CutsceneParameters parameters)
{
	if (parameters[1] == "target")
	{
		//TODO
	}
	else if (parameters[1] == "zoom")
	{
		//TODO: Refactor the camera.Zoom function to make orthoZoom private
		if (parameters[2] == "set")
		{
			manager->game->renderer->camera.orthoZoom = ParseNumberValue(parameters[3]);
			manager->game->renderer->camera.Zoom(0, manager->game->screenWidth, manager->game->screenHeight);
		}
		else if (parameters[2] == "add")
		{
			manager->game->renderer->camera.orthoZoom += ParseNumberValue(parameters[3]);
			manager->game->renderer->camera.Zoom(0, manager->game->screenWidth, manager->game->screenHeight);
		}
		else if (parameters[2] == "sub")
		{
			manager->game->renderer->camera.orthoZoom -= ParseNumberValue(parameters[3]);
			manager->game->renderer->camera.Zoom(0, manager->game->screenWidth, manager->game->screenHeight);
		}
		else if (parameters[2] == "lerp")
		{
			//TODO
		}
	}
	else if (parameters[1] == "position")
	{
		if (parameters[2] == "set")
		{
			manager->game->renderer->camera.position = glm::vec3(ParseNumberValue(parameters[3]), 
				ParseNumberValue(parameters[4]), ParseNumberValue(parameters[5]));
		}
		else if (parameters[2] == "moveto") //TODO: Last parameter is time or speed?
		{
			const float SPEED = std::stof(parameters[6]);

			if (manager->game->renderer->camera.position.x != ParseNumberValue(parameters[3]))
			{
				if (ParseNumberValue(parameters[3]) > manager->game->renderer->camera.position.x)
					manager->game->renderer->camera.position.x += SPEED;
				else
					manager->game->renderer->camera.position.x -= SPEED;
			}

			if (manager->game->renderer->camera.position.y != ParseNumberValue(parameters[4]))
			{
				if (ParseNumberValue(parameters[4]) > manager->game->renderer->camera.position.y)
					manager->game->renderer->camera.position.y += SPEED;
				else
					manager->game->renderer->camera.position.y -= SPEED;
			}

			if (manager->game->renderer->camera.position.z != ParseNumberValue(parameters[5]))
			{
				if (ParseNumberValue(parameters[5]) > manager->game->renderer->camera.position.z)
					manager->game->renderer->camera.position.z += SPEED;
				else
					manager->game->renderer->camera.position.z -= SPEED;
			}

			//TODO: Round these to ints if they are not even
			glm::vec3 intPos = manager->game->renderer->camera.position;

			intPos.x = (int)intPos.x;
			intPos.y = (int)intPos.y;
			intPos.z = (int)intPos.z;

			if (intPos != glm::vec3((int)ParseNumberValue(parameters[3]),
				(int)ParseNumberValue(parameters[4]), (int)ParseNumberValue(parameters[5])))
			{
				return -199;
			}
			else
			{
				manager->game->renderer->camera.position = intPos;
			}
		}
		else if (parameters[2] == "add")
		{
			manager->game->renderer->camera.position += glm::vec3(ParseNumberValue(parameters[3]),
				ParseNumberValue(parameters[4]), ParseNumberValue(parameters[5]));
		}
		else if (parameters[2] == "sub")
		{
			manager->game->renderer->camera.position -= glm::vec3(ParseNumberValue(parameters[3]),
				ParseNumberValue(parameters[4]), ParseNumberValue(parameters[5]));
		}
		else if (parameters[2] == "lerp")
		{
			//TODO
		}
	}
	else if (parameters[1] == "rotation")
	{
		//TODO: Refactor the camera.Zoom function to make angle private
		if (parameters[2] == "set")
		{
			manager->game->renderer->camera.angle = ParseNumberValue(parameters[3]);
			manager->game->renderer->camera.Zoom(0, manager->game->screenWidth, manager->game->screenHeight);
		}
		else if (parameters[2] == "add")
		{
			manager->game->renderer->camera.angle += ParseNumberValue(parameters[3]);
			manager->game->renderer->camera.Zoom(0, manager->game->screenWidth, manager->game->screenHeight);
		}
		else if (parameters[2] == "sub")
		{
			manager->game->renderer->camera.angle -= ParseNumberValue(parameters[3]);
			manager->game->renderer->camera.Zoom(0, manager->game->screenWidth, manager->game->screenHeight);
		}
		else if (parameters[2] == "lerp")
		{
			//TODO
		}
	}
	else if (parameters[1] == "pitch")
	{
		if (parameters[2] == "set")
		{
			manager->game->renderer->camera.pitch = ParseNumberValue(parameters[3]);
		}
		else if (parameters[2] == "add")
		{
			manager->game->renderer->camera.pitch += ParseNumberValue(parameters[3]);
		}
		else if (parameters[2] == "sub")
		{
			manager->game->renderer->camera.pitch -= ParseNumberValue(parameters[3]);
		}
		else if (parameters[2] == "lerp")
		{
			//TODO
		}
	}
	else if (parameters[1] == "yaw")
	{
		if (parameters[2] == "set")
		{
			manager->game->renderer->camera.yaw = ParseNumberValue(parameters[3]);
		}
		else if (parameters[2] == "add")
		{
			manager->game->renderer->camera.yaw += ParseNumberValue(parameters[3]);
		}
		else if (parameters[2] == "sub")
		{
			manager->game->renderer->camera.yaw -= ParseNumberValue(parameters[3]);
		}
		else if (parameters[2] == "lerp")
		{
			//TODO
		}
	}
	else if (parameters[1] == "roll")
	{
		if (parameters[2] == "set")
		{
			manager->game->renderer->camera.roll = ParseNumberValue(parameters[3]);
		}
		else if (parameters[2] == "add")
		{
			manager->game->renderer->camera.roll += ParseNumberValue(parameters[3]);
		}
		else if (parameters[2] == "sub")
		{
			manager->game->renderer->camera.roll -= ParseNumberValue(parameters[3]);
		}
		else if (parameters[2] == "lerp")
		{
			//TODO
		}
	}
	else if (parameters[1] == "projection")
	{
		if (parameters[2] == "orthographic")
		{
			manager->game->renderer->camera.useOrthoCamera = true;
			manager->game->renderer->camera.Zoom(0, manager->game->screenWidth, manager->game->screenHeight);
		}
		else if (parameters[2] == "perspective")
		{
			manager->game->renderer->camera.useOrthoCamera = false;
			manager->game->renderer->camera.Zoom(0, manager->game->screenWidth, manager->game->screenHeight);
		}
	}

	return 0;
}

int CutsceneCommands::WindowFunction(CutsceneParameters parameters)
{
	if (parameters.size() > 1)
	{
		if (parameters[1] == "icon")
		{			
			manager->game->windowIconFilepath = ParseStringValue(parameters[2]);
			SDL_SetWindowIcon(manager->game->window, IMG_Load(ParseStringValue(parameters[2]).c_str()));
		}
		else if (parameters[1] == "title")
		{
			manager->game->windowTitle = ParseStringValue(parameters[2]);
			SDL_SetWindowTitle(manager->game->window, ParseStringValue(parameters[2]).c_str());
		}
	}

	return 0;
}

int CutsceneCommands::ControlBindings(CutsceneParameters parameters)
{
	if (parameters.size() > 1)
	{
		//TODO: Toggle function?
		if (parameters[1] == "mouse")
		{
			manager->useMouseControls = (parameters[2] == "on");
		}
		else if (parameters[1] == "keyboard")
		{
			manager->useKeyboardControls = (parameters[2] == "on");
		}
	}

	return 0;
}

int CutsceneCommands::BindKeyToLabel(CutsceneParameters parameters)
{
	if (parameters.size() > 2)
	{
		std::string labelName = ParseStringValue(parameters[2]);

		if (parameters[1].size() > 1)
		{
			// Handle special cases / full words here (spacebar, enter, etc.)
			if (parameters[1] == "spacebar")
			{
				buttonLabels[SDL_SCANCODE_SPACE] = labelName;
			}
			else if (parameters[1] == "enter" || parameters[1] == "return")
			{
				buttonLabels[SDL_SCANCODE_RETURN] = labelName;
			}
		}
		else
		{
			//TODO: Handle other single character keys

			// Get the ASCII value of the character
			int letter = parameters[1][0];

			// ASCII A starts at 65, SDL starts at 4, so 65-4 = 61 = offset
			// ASCII a starts at 97, SDL starts at 4, so 97-4 = 93 = offset
			// ASCII 1 starts at 49, SDL starts at 30, so 49 - 30 = 19 = offset
			
			if (letter > 64 && letter < 91) // A
			{
				buttonLabels[letter - 61] = labelName;
			}
			else if (letter > 96 && letter < 123) // a
			{
				buttonLabels[letter - 93] = labelName;
			}
			else if (letter > 48 && letter < 58) // 1
			{
				buttonLabels[letter - 19] = labelName;
			}
			else if (letter == 48) // 0
			{
				buttonLabels[SDL_SCANCODE_0] = labelName;
			}
		}
	}

	return 0;
}

int CutsceneCommands::SetClickToContinue(CutsceneParameters parameters)
{
	if (parameters[1] == "state")
	{
		AnimState* state = manager->textbox->clickToContinue->GetAnimator()->GetState(parameters[2]);
		
		int index = 2;
		std::string stateName = parameters[index++];
		int stateSpeed = std::stoi(parameters[index++]);
		int spriteStartFrame = std::stoi(parameters[index++]);
		int spriteEndFrame = std::stoi(parameters[index++]);
		int spriteFrameWidth = std::stoi(parameters[index++]);
		int spriteFrameHeight = std::stoi(parameters[index++]);

		std::string spriteFilePath = parameters[index++];
		int spritePivotX = std::stoi(parameters[index++]);
		int spritePivotY = std::stoi(parameters[index++]);

		if (state->sprite != nullptr)
			delete state->sprite;

		state->name = stateName;
		state->speed = stateSpeed;
		state->sprite = new Sprite(spriteStartFrame, spriteEndFrame, spriteFrameWidth, spriteFrameHeight,
			manager->game->spriteManager, spriteFilePath, manager->game->renderer->shaders[ShaderName::Default],
			Vector2(spritePivotX, spritePivotY));

		state->sprite->keepPositionRelativeToCamera = true;
		state->sprite->keepScaleRelativeToCamera = true;
	}
	else if (parameters[1] == "animator")
	{
		//TODO: Swap out the animator
	}

	return 0;
}

int CutsceneCommands::ScreenshotCommand(CutsceneParameters parameters)
{
	if (parameters.size() > 1)
	{
		manager->game->SaveScreenshot(parameters[1]);
	}
	else
	{
		manager->game->SaveScreenshot();
	}

	return 0;
}

int CutsceneCommands::LuaCommand(CutsceneParameters parameters)
{
	return 0;
}

int CutsceneCommands::ErrorLog(CutsceneParameters parameters)
{
	if (parameters.size() > 1)
	{
		manager->game->logger->Log(parameters[1].c_str());
	}
	
	return 0;
}

// font name path
int CutsceneCommands::FontCommand(CutsceneParameters parameters)
{
	// Load fonts from files

	if (manager->textbox->fonts.count(parameters[1]) == 1)
	{
		delete manager->textbox->fonts[parameters[1]];
	}

	manager->textbox->fonts[parameters[1]] = new FontInfo((parameters[2] + "/" + parameters[1] + "-Regular.ttf").c_str(), 24);
	manager->textbox->fonts[parameters[1]]->SetBoldFont((parameters[2] + "/" + parameters[1] + "-Bold.ttf").c_str());
	manager->textbox->fonts[parameters[1]]->SetItalicsFont((parameters[2] + "/" + parameters[1] + "-Italic.ttf").c_str());
	manager->textbox->fonts[parameters[1]]->SetBoldItalicsFont((parameters[2] + "/" + parameters[1] + "-BoldItalic.ttf").c_str());

	return 0;
}

int CutsceneCommands::GetResourceFilename(CutsceneParameters parameters)
{
	// getfilename $myvar bgm
	unsigned int varNum = ParseNumberValue(parameters[1]);

	if (parameters[2] == "bgm")
	{
		stringVariables[varNum] = manager->game->soundManager->bgmFilepath;
	}
	else if (parameters[2] == "sound")
	{
		stringVariables[varNum] = manager->game->soundManager->sounds[ParseNumberValue(parameters[3])]->sound->filepath;
	}
	else if (parameters[2] == "sprite")
	{
		stringVariables[varNum] = manager->images[ParseNumberValue(parameters[3])]->GetSprite()->filename;
	}
	else if (parameters[2] == "script")
	{
		stringVariables[varNum] = manager->currentScript;
	}
	else if (parameters[2] == "level")
	{
		stringVariables[varNum] = manager->game->currentLevel;
	}
	else if (parameters[2] == "label")
	{
		stringVariables[varNum] = manager->currentLabel->name;
	}
	else if (parameters[2] == "text")
	{
		stringVariables[varNum] = manager->textbox->text->txt;
	}

	return 0;
}

int CutsceneCommands::TagCommand(CutsceneParameters parameters)
{
	if (parameters[1] == "define")
	{
		if (manager->tags.count(parameters[2]) != 1)
		{
			manager->tags[parameters[2]] = new TextTag();
		}		
	}

	return 0;
}

int CutsceneCommands::IntToString(CutsceneParameters parameters)
{
	int stringVariableIndex = ParseNumberValue(parameters[1]);
	stringVariables[stringVariableIndex] = std::to_string(ParseNumberValue(parameters[2]));
	return 0;
}

int CutsceneCommands::IncrementVariable(CutsceneParameters parameters)
{
	numberVariables[ParseNumberValue(parameters[1])]++;
	return 0;
}

int CutsceneCommands::DecrementVariable(CutsceneParameters parameters)
{
	numberVariables[ParseNumberValue(parameters[1])]--;
	return 0;
}

int CutsceneCommands::Output(CutsceneParameters parameters)
{
	bool shouldOutput = false;

	if (shouldOutput)
	{
		if (parameters[1] == "str")
		{
			std::cout << parameters[2] << ": " << ParseStringValue(parameters[2]) << std::endl;
		}
		else if (parameters[1] == "num")
		{
			std::cout << parameters[2] << ": " << ParseNumberValue(parameters[2]) << std::endl;
		}
		else
		{
			std::cout << "ERROR: Failed to define output type (str/num); cannot log output." << std::endl;
		}
	}

	return 0;
}

int CutsceneCommands::FileExist(CutsceneParameters parameters)
{
	numberVariables[ParseNumberValue(parameters[1])] = std::filesystem::exists(parameters[2]);

	return 0;
}

int CutsceneCommands::TextSpeed(CutsceneParameters parameters)
{
	manager->msDelayBetweenGlyphs = ParseNumberValue(parameters[1]);

	return 0;
}

int CutsceneCommands::AutoMode(CutsceneParameters parameters)
{
	if (parameters.size() > 1)
	{
		if (parameters[1] == "on")
		{
			manager->automaticallyRead = true;
		}
		else if (parameters[1] == "off")
		{
			manager->automaticallyRead = false;
		}
		else if (parameters[1] == "speed")
		{
			manager->autoTimeToWaitPerGlyph = ParseNumberValue(parameters[2]);
		}
	}	

	return 0;
}

// align x center textbox
// align x left 1
int CutsceneCommands::AlignCommand(CutsceneParameters parameters)
{
	Text* text = manager->textbox->text;

	if (parameters[3] != "textbox")
	{
		if (parameters[3] == "namebox")
		{
			text = manager->textbox->speaker;
		}
		else
		{
			text = static_cast<Text*>(manager->images[ParseNumberValue(parameters[3])]);
		}		
	}	

	if (text == nullptr)
		return -1;

	if (parameters[1] == "x")
	{
		if (parameters[2] == "left")
		{
			text->alignX = AlignmentX::LEFT;
		}
		else if (parameters[2] == "center")
		{
			text->alignX = AlignmentX::CENTER;
		}
		else if (parameters[2] == "right")
		{
			text->alignX = AlignmentX::RIGHT;
		}
	}
	else if (parameters[1] == "y")
	{
		if (parameters[2] == "top")
		{
			text->alignY = AlignmentY::TOP;
		}
		else if (parameters[2] == "center")
		{
			text->alignY = AlignmentY::CENTER;
		}
		else if (parameters[2] == "bottom")
		{
			text->alignY = AlignmentY::BOTTOM;
		}
	}

	text->SetPosition(text->GetPosition().x, text->GetPosition().y);

	return 0;
}

int CutsceneCommands::InputCommand(CutsceneParameters parameters)
{
	return 0;
}

int CutsceneCommands::AutoSave(CutsceneParameters parameters)
{
	//TODO: Save this setting and remember it when you load the game
	// (Should probably just create a save function for the manager
	// and look through the manager's variables and save the important ones)

	if (parameters[1] == "on")
	{
		manager->autosave = true;
	}
	else if (parameters[1] == "off")
	{
		manager->autosave = false;
	}

	return 0;
}

int CutsceneCommands::AutoReturn(CutsceneParameters parameters)
{
	//TODO: Save this setting and remember it when you load the game

	if (parameters[1] == "on")
	{
		manager->autoreturn = true;
	}
	else if (parameters[1] == "off")
	{
		manager->autoreturn = false;
	}

	return 0;
}

int CutsceneCommands::AnimationCommand(CutsceneParameters parameters)
{
	const std::string animationName = parameters[1];

	if (manager->animatedImages.count(animationName) != 1)
	{
		manager->animatedImages[animationName] = new Entity(Vector2(0, 0));
	}

	if (parameters[2] == "state")
	{
		int index = 3;
		std::string stateName = parameters[index++];
		AnimState* state = manager->animatedImages[animationName]->GetAnimator()->GetState(stateName);		
		int stateSpeed = std::stoi(parameters[index++]);
		int spriteStartFrame = std::stoi(parameters[index++]);
		int spriteEndFrame = std::stoi(parameters[index++]);
		int spriteFrameWidth = std::stoi(parameters[index++]);
		int spriteFrameHeight = std::stoi(parameters[index++]);

		std::string spriteFilePath = ParseStringValue(parameters[index++]);
		int spritePivotX = std::stoi(parameters[index++]);
		int spritePivotY = std::stoi(parameters[index++]);

		if (state->sprite != nullptr)
			delete state->sprite;

		state->name = stateName;
		state->speed = stateSpeed;
		state->sprite = new Sprite(spriteStartFrame, spriteEndFrame, spriteFrameWidth, spriteFrameHeight,
			manager->game->spriteManager, spriteFilePath, manager->game->renderer->shaders[ShaderName::Default],
			Vector2(spritePivotX, spritePivotY));

		state->sprite->keepPositionRelativeToCamera = true;
		state->sprite->keepScaleRelativeToCamera = true;
	}
	else if (parameters[2] == "set")
	{
		//TODO: This will never work as long as the state machine is active
		// because unless you also set the conditions to get to this state,
		// it will just flow back into whatever state it was in before (or can get to)
		if (parameters[3] == "state") 
		{
			manager->animatedImages[animationName]->GetAnimator()->SetState(parameters[4].c_str());
			manager->animatedImages[animationName]->GetAnimator()->Update(manager->animatedImages[animationName]);
		}
		else if (parameters[3] == "bool")
		{
			manager->animatedImages[animationName]->GetAnimator()->SetBool(parameters[4].c_str(), parameters[5] == "true");
			manager->animatedImages[animationName]->GetAnimator()->Update(manager->animatedImages[animationName]);
		}
		else if (parameters[3] == "machine")
		{
			std::vector<AnimState*> animStates;
			manager->game->spriteManager->ReadAnimData(parameters[4], animStates);

			Animator* anim1 = new Animator("cursor", animStates, parameters[5]);
			anim1->SetBool("endOfPage", false);
			anim1->SetRelativeAllStates(true);
			//anim1->SetScaleAllStates(Vector2(0.5f, 0.5f));

			manager->animatedImages[animationName]->SetAnimator(anim1);
			manager->animatedImages[animationName]->GetAnimator()->Update(manager->animatedImages[animationName]);
			manager->animatedImages[animationName]->GetSprite()->SetScale(Vector2(0.5f, 0.5f));

			
		}
	}

	return 0;
}