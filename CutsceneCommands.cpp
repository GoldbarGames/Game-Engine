#include "CutsceneCommands.h"
#include "CutsceneManager.h"
#include "Game.h"
#include "Timer.h"
#include "Animator.h"
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

typedef int (CutsceneCommands::*FuncList)(CutsceneParameters parameters);

//Look Up Table
struct FuncLUT {
	char command[32];
	FuncList method;
};

std::vector<FuncLUT>cmd_lut = {
	{"~", &CutsceneCommands::DoNothing},
	{"add", &CutsceneCommands::AddNumberVariables},
	{"backlog", &CutsceneCommands::OpenBacklog},
	{"bg", &CutsceneCommands::LoadBackground },
	{"bgm", &CutsceneCommands::MusicCommand },
	{"btnwait", &CutsceneCommands::WaitForButton },
	{"camera", &CutsceneCommands::CameraFunction},
	{"controls", & CutsceneCommands::ControlBindings},
	{"choice", &CutsceneCommands::DisplayChoice },
	{"cl", &CutsceneCommands::ClearSprite },
	{"concat", &CutsceneCommands::ConcatenateStringVariables},
	{"defsub", &CutsceneCommands::DefineUserFunction},
	{"div", &CutsceneCommands::DivideNumberVariables},
	{"end", &CutsceneCommands::EndGame },
	{"fade", &CutsceneCommands::Fade },
	{"flip", &CutsceneCommands::FlipSprite },
	{"global", &CutsceneCommands::SetGlobalNumber},
	{"gosub", &CutsceneCommands::GoSubroutine },
	{"goto", &CutsceneCommands::GoToLabel },
	{"if", &CutsceneCommands::IfCondition },
	{"jumpb", &CutsceneCommands::JumpBack },
	{"jumpf", &CutsceneCommands::JumpForward },
	{"keybind", &CutsceneCommands::BindKeyToLabel },
	{"ld", &CutsceneCommands::LoadSprite },
	{"loadgame",&CutsceneCommands::LoadGame },
	{"me", &CutsceneCommands::MusicEffectCommand},
	{"mod", &CutsceneCommands::ModNumberVariables},
	{"mul", &CutsceneCommands::MultiplyNumberVariables},
	{"namebox", &CutsceneCommands::Namebox},
	{"numalias", &CutsceneCommands::SetNumAlias },
	{"random", &CutsceneCommands::RandomNumberVariable },
	{"reset", &CutsceneCommands::ResetGame },
	{"resolution", &CutsceneCommands::SetResolution },
	{"return", &CutsceneCommands::ReturnFromSubroutine },
	{"savegame",&CutsceneCommands::SaveGame },
	{"se", &CutsceneCommands::SoundCommand },
	{"set_velocity", &CutsceneCommands::SetVelocity },
	{"setnumvar", &CutsceneCommands::SetNumberVariable },
	{"setstrvar", &CutsceneCommands::SetStringVariable },
	{"spbtn", &CutsceneCommands::SetSpriteButton},
	{"sprite", &CutsceneCommands::SetSpriteProperty },	
	{"stralias", &CutsceneCommands::SetStringAlias },
	{"sub", &CutsceneCommands::SubtractNumberVariables},
	{"text", &CutsceneCommands::LoadText },
	{"textbox", &CutsceneCommands::Textbox },
	{"textcolor", &CutsceneCommands::TextColor },
	{"timer", &CutsceneCommands::TimerFunction},
	{"wait",& CutsceneCommands::Wait },
	{"window", &CutsceneCommands::WindowFunction }
};


// TODO: Implement these commands:
// * Store/retrieve/display values via variables
// * Variable operations (add, sub, mul, div, mod)
// * Jump to labels
// * If/else control flow, compare strings/numbers/variables
// For loops, while loops
// * Jump forward/back

// * Display text on the screen as an image/entity
// * Set images as clickable buttons
// * Dialogue options / choices
// * Music effects (ME) - works just like SE but with a loop
// * Ending the game window / restarting the game window

// - Save/load
// - Save screenshot as image
// * Playing animations (use state machines, set variables, etc.)
// - what about animations that involve each frame being its own file?
// - custom timers (we'll deal with this when we handle blinking animations)

// * Randomize a variable and re-seed the randomness
// * User-defined functions 
// * Get parameters (defsub myfunction %param1 $param2 %param3)
// * gosub (goto and return)
// * Change screen resolution (TODO: See camera.cpp constructor)
// Settings screen (sound volume, text speed, etc.)
// Check if a file exists (fileexist assets/myfile.png %0)
// * Change text color for menu selection

// * Enable / disable mouse controls (enable mouse, enable keyboard, enable both)
// * Bind keyboard keys to labels via script
// Custom key bindings (advance text, backlog, etc.)

// Display sprite of talking character in the textbox
// Highlight/dim speaking characters (map name of the character to the sprite via folder path)
// * Change textbox & namebox: position, sprite (animation), text, font type, font color
// font (size, style)

// Set the click to continue button image / animation
// Automatically position the CTC image

// Flip images horizontal/vertical
// * Change window caption and icon
// Change location of save data
// Alpha image effects
// * Skip button to skip text
// * Log button to read old text (in box vs. scroll)
// * Automode, adjust automode speeds (per letter and per line)
// Adjustable text speed (!sd)
// * Right-click (escape) subroutine
// Click-wait subroutine
// Keyboard input for variables
// * Global/persistent variables

// Camera operations (pan, zoom, rotate, orthographic/perspective, other stuff)
// - set the position, rotation, just like anything else
// - set the zoom factor, the projection stuff, perspective, etc.

// Output error logs
// Proper syntax checking and error handling
// Math functions (abs, sin, cos, tan, etc.)
// Changing the file/directory where the script file is read from
// Physics functions (position, velocity, acceleration, collision detection)
// Visual Editor, modify cutscene as it is running, replay it
// Declare arrays and lists of variables, more complex stuff
// * Timers, set/reset/stop/pause/unpause them
// Pop up a box with text for a time limit
// Get name of BGM currently playing (or just names of files being used in the scene)



// * Can assign color to a character's dialogue
// - TODO: Can use variables to get the color,
// - and can embed colors into text (## returns it to normal)
// - (this requires drawing the text one letter per sprite)
//textcolor default #ffffff ;
//textcolor BUTLER #ff0000 ;
//`:BUTLER: This is #ff0000red text ## and this is not.`

// Special features:
// - picture gallery
// - music player
// - encyclopedia

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

void CutsceneCommands::ExecuteCommand(std::string command)
{
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

	// Break up the command string into a vector of strings, one word each
	//TODO: Is this the best way to do this?
	std::stringstream ss(command);
	std::istream_iterator<std::string> begin(ss);
	std::istream_iterator<std::string> end;

	std::vector<std::string> parameters(begin, end);
	//std::cout << command << std::endl;
	//std::copy(parameters.begin(), parameters.end(), std::ostream_iterator<std::string>(std::cout, "\n"));

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
				int errorCode = (this->*cmd.method)(parameters);

				if (errorCode != 0)
				{
					std::cout << "ERROR " << errorCode << ": ";
					for (int i = 0; i < parameters.size(); i++)
						std::cout << parameters[i] << " ";
					std::cout << std::endl;
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

					break;
				}

			}

			if (!commandFound)
			{
				std::cout << "ERROR: Command " << parameters[0] << " not found." << std::endl;
			}			
		}
	}
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
			word = parameters[index].substr(1, word.size() - 1);
			leftValueStr = GetStringVariable(GetNumAlias(word));
			break;
		case '%': // number variable
			leftHandIsNumber = true;
			word = parameters[index].substr(1, word.size() - 1);
			leftValueNum = GetNumberVariable(GetNumAlias(word));
			break;
		default:
			if (parameters[index].find_first_not_of("-0123456789") == std::string::npos)
			{
				leftValueNum = std::stoi(parameters[index]);
				leftHandIsNumber = true;
			}
			else
			{
				leftValueStr = parameters[index];
				leftHandIsNumber = false;
			}

			break;
		}

		index += 2; // skip the operator

		switch (parameters[index][0])
		{
		case '$': // string variable
			rightHandIsNumber = false;
			word = parameters[index].substr(1, word.size() - 1);
			rightValueStr = GetStringVariable(GetNumAlias(word));
			break;
		case '%': // number variable
			rightHandIsNumber = true;
			word = parameters[index].substr(1, word.size() - 1);
			rightValueNum = GetNumberVariable(GetNumAlias(word));
			break;
		default:
			if (parameters[index].find_first_not_of("-0123456789") == std::string::npos)
			{
				rightValueNum = std::stoi(parameters[index]);
				rightHandIsNumber = true;
			}
			else
			{
				rightValueStr = parameters[index];
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
		else if (parameters[index] == "==")
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

			// If all conditions are true, execute the following command
			if (parameters[index] != "&&")
			{
				std::string nextCommand = "";
				for (int i = index; i < parameters.size(); i++)
					nextCommand += (parameters[i] + " ");

				// If this is from a choice, don't evaluate any more
				manager->choiceIfStatements.clear();

				ExecuteCommand(nextCommand);
			}
			else
			{
				conditionIsTrue = false;
				index++;
			}
		}
		else // else exit, do nothing
		{
			return -2;
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

int CutsceneCommands::DisplayChoice(CutsceneParameters parameters)
{
	unsigned int numberOfChoices = ParseNumberValue(parameters[1]);

	int index = 2;
	int spriteNumber = manager->choiceQuestionNumber;
	std::string choiceQuestion = ParseStringValue(parameters[index]);

	int choiceYPos = 100;
	LoadText({"", std::to_string(spriteNumber), "1280", std::to_string(choiceYPos), choiceQuestion });
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
		choiceYPos = 300 + (200 * i);
		//TODO: Set the width to the center of the screen

		LoadText({"", choiceNumber, "1280",
			std::to_string(choiceYPos), choiceText });

		// Make the text sprite act as a button
		SetSpriteButton({ "", choiceNumber , choiceNumber });

		// Wait for button input
		std::string variableNumber = std::to_string(42);
		WaitForButton({ "",  variableNumber });

		// Construct the if-statement and store it
		//if %42 == 21 goto label_left ;

		manager->choiceIfStatements.push_back("if %" + variableNumber + " == " + choiceNumber + " goto " + choiceLabel + " ;");

		manager->inputTimer.Start(1000);
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
	// Check the label name to see if it is a variable
	manager->PlayCutscene(ParseStringValue(parameters[1]).c_str());

	/*
	switch (parameters[1][0])
	{
	case '$': // string variable
		manager->PlayCutscene(GetStringVariable(GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1))).c_str());
		break;
	default:
		manager->PlayCutscene(parameters[1].c_str());
		break;
	}
	*/

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


	manager->SaveGame();

	return 0;
}

int CutsceneCommands::LoadGame(CutsceneParameters parameters)
{
	//TODO: Load everything that was saved from a file

	manager->LoadGame();

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

	key = GetNumAlias(parameters[1]);
	if (parameters[1][0] == '$')
		key = GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1));

	word1 = ParseStringValue(parameters[1]);
	word2 = ParseStringValue(parameters[2]);

	stringVariables[key] = word1 + word2;

	// If global variable, save change to file
	if (key >= manager->globalStart)
		manager->SaveGlobalVariable(key, stringVariables[key]);

	return 0;
}


int CutsceneCommands::AddNumberVariables(CutsceneParameters parameters)
{
	unsigned int key = 0;
	unsigned int number1 = 0;
	unsigned int number2 = 0;

	key = GetNumAlias(parameters[1]);
	if (parameters[1][0] == '%')
		key = GetNumAlias(parameters[1].substr(1, parameters[1].size() - 1));

	number1 = ParseNumberValue(parameters[1]);
	number2 = ParseNumberValue(parameters[2]);		
	
	numberVariables[key] = number1 + number2;

	// If global variable, save change to file
	if (key >= manager->globalStart)
		manager->SaveGlobalVariable(key, numberVariables[key]);

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
		manager->SaveGlobalVariable(key, numberVariables[key]);

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
		manager->SaveGlobalVariable(key, numberVariables[key]);

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
		manager->SaveGlobalVariable(key, numberVariables[key]);

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
		manager->SaveGlobalVariable(key, numberVariables[key]);

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
			manager->SaveGlobalVariable(key, numberVariables[key]);
	}
	else // no offset
	{
		unsigned int key = ParseNumberValue(parameters[1]);
		unsigned int maxNumber = ParseNumberValue(parameters[2]);

		numberVariables[key] = (rand() % maxNumber);

		// If global variable, save change to file
		if (key >= manager->globalStart)
			manager->SaveGlobalVariable(key, numberVariables[key]);
	}

	return 0;
}

int CutsceneCommands::SetNumberVariable(CutsceneParameters parameters)
{
	unsigned int key = ParseNumberValue(parameters[1]);
	unsigned int value = ParseNumberValue(parameters[2]);

	if (GetNumberVariable(value) == 0)
		numberVariables[key] = value;
	else
		numberVariables[key] = value; // GetNumberVariable(value);

	//TODO: To set a variable to the value of another variable,
	// check if the number starts with % or it is a string

	// If global variable, save change to file
	if (key >= manager->globalStart)
		manager->SaveGlobalVariable(key, numberVariables[key]);

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
		manager->SaveGlobalVariable(key, stringVariables[key]);

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
	std::string key = parameters[1];
	std::string value = parameters[2];
	stralias[key] = value;
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
	std::string key = parameters[1];
	unsigned int value = ParseNumberValue(parameters[2]);
	//TODO: Check for errors
	numalias[key] = value;
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

	return value;
}

unsigned int CutsceneCommands::ParseNumberValue(const std::string& parameter)
{
	unsigned int value = 0;

	// Get the variable number to store the result in
	if (parameter[0] == '%')
		value = GetNumberVariable(GetNumAlias(parameter.substr(1, parameter.size() - 1)));
	else
		value = GetNumAlias(parameter);

	return value;
}

unsigned int CutsceneCommands::GetNumAlias(const std::string& key)
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
		std::stoi(parameters[index]),
		std::stoi(parameters[index + 1]),
		std::stoi(parameters[index + 2]),
		std::stoi(parameters[index + 3]),
	};

	if (manager->images[imageNumber] != nullptr)
		delete manager->images[imageNumber];

	//TODO: Also save/load in the font type/size/style for this text object
	manager->images[imageNumber] = new Text(manager->game->renderer,
		manager->game->theFont, text, textColor);

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

	//TODO: Parse text string to get and set the color
	//TODO: Deal with individual glyphs
	Color textColor = { 255, 255, 255, 255 };
	if (text.size() > 1 && text[0] == '#')
	{
		textColor = ParseColorHexadecimal(text.substr(0, 8).c_str());
		newText = new Text(manager->game->renderer,
			manager->game->theFont, text.substr(9, text.size()-8), textColor);
	}
	else
	{
		newText = new Text(manager->game->renderer,
			manager->game->theFont, text, textColor);
	}

	newText->SetPosition(pos.x, pos.y); // use the Text SetPosition function, not Entity

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
		Color color = { ParseNumberValue(parameters[3]), ParseNumberValue(parameters[4]),
			ParseNumberValue(parameters[5]), ParseNumberValue(parameters[6]) };

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
			ReadAnimData(ParseStringValue(parameters[4]), animStates);

			for (int i = 0; i < animStates.size(); i++)
			{
				animStates[i]->sprite->keepPositionRelativeToCamera = true;
				animStates[i]->sprite->keepScaleRelativeToCamera = true;
			}

			Animator* newAnim = new Animator(AnimType::Player, animStates, ParseStringValue(parameters[5]));
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
	PhysicsEntity* entity = nullptr;

	for (unsigned int i = 0; i < manager->game->entities.size(); i++)
	{
		if (manager->game->entities[i]->name == parameters[1])
		{
			entity = dynamic_cast<PhysicsEntity*>(manager->game->entities[i]);

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
	int ms = ParseNumberValue(parameters[1]);
	manager->timer -= ms;
	manager->textbox->isReading = false;
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

			}
		}
	}
	else if (parameters[1] == "color")
	{
		manager->textbox->nameSprite->color = ParseColorFromParameters(parameters, 2);
	}
	else if (parameters[1] == "position")
	{
		manager->textbox->speaker->SetPosition((int)ParseNumberValue(parameters[2]),
			(int)ParseNumberValue(parameters[3]));
	}
	else if (parameters[1] == "image")
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

			}
			else if (parameters[3] == "style")
			{

			}
		}
	}
	else if (parameters[1] == "color")
	{
		manager->textbox->boxSprite->color = ParseColorFromParameters(parameters, 2);
	}
	else if (parameters[1] == "position")
	{
		manager->textbox->text->SetPosition((int)ParseNumberValue(parameters[2]),
			(int)ParseNumberValue(parameters[3]));
	}
	else if (parameters[1] == "image")
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
			manager->game->renderer->targetColor = { (int)ParseNumberValue(parameters[2]),
				(int)ParseNumberValue(parameters[3]),
				(int)ParseNumberValue(parameters[4]),
				(int)ParseNumberValue(parameters[5])};
		}
		else
		{
			manager->game->renderer->targetColor = { (int)ParseNumberValue(parameters[2]),
				(int)ParseNumberValue(parameters[3]),
				(int)ParseNumberValue(parameters[4]),
				255 };
		}

	}

	return 0;
}

int CutsceneCommands::SetResolution(CutsceneParameters parameters)
{
	//TODO: Maybe place this command in a config file to start the window in a certain resolution?
	//TEST: Allow setting resolution based on variable values
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
		else if (parameters[1] == "color")
		{
			if (parameters[2][0] == '#')
			{
				manager->backlogColor = ParseColorHexadecimal(parameters[2].c_str());
			}
			else if (parameters.size() == 5)
			{
				manager->backlogColor = { (int)ParseNumberValue(parameters[2]),
					(int)ParseNumberValue(parameters[3]) ,
					(int)ParseNumberValue(parameters[4]),
					255 };
			}
			else if(parameters.size() == 6)
			{
				manager->backlogColor = { (int)ParseNumberValue(parameters[2]),
					(int)ParseNumberValue(parameters[3]) ,
					(int)ParseNumberValue(parameters[4]),
					(int)ParseNumberValue(parameters[5]) };
			}
		}
	}
	else
	{
		//TODO: Allow the script to set other backlog properties
		manager->readingBacklog = true;
		manager->backlogIndex = manager->backlog.size() - 1;
		manager->ReadBacklog();
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
	if (parameters[1] == "zoom")
	{

	}
	else if (parameters[1] == "set")
	{
		if (parameters[2] == "position")
		{

		}
		else if (parameters[2] == "rotation")
		{

		}
		else if (parameters[2] == "pitch")
		{

		}
		else if (parameters[2] == "yaw")
		{

		}
		else if (parameters[2] == "projection")
		{
		if (parameters[3] == "orthographic")
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
	}
	else if (parameters[1] == "pan")
	{

	}
	else if (parameters[1] == "rotate")
	{

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


//TODO: Only read this data once at the beginning and then store it for lookup later
void CutsceneCommands::ReadAnimData(std::string dataFilePath, std::vector<AnimState*>& animStates)
{
	// Get anim data from the file
	std::ifstream fin;
	fin.open(dataFilePath);

	std::string animData = "";
	for (std::string line; std::getline(fin, line); )
	{
		animData += line + "\n";
	}

	fin.close();

	// Go through the data and add all states

	std::stringstream ss{ animData };

	char lineChar[256];
	ss.getline(lineChar, 256);

	try
	{
		while (ss.good() && !ss.eof())
		{
			std::istringstream buf(lineChar);
			std::istream_iterator<std::string> beg(buf), end;
			std::vector<std::string> tokens(beg, end);

			int index = 0;

			std::string stateName = tokens[index++];
			int stateSpeed = std::stoi(tokens[index++]);
			int spriteStartFrame = std::stoi(tokens[index++]);
			int spriteEndFrame = std::stoi(tokens[index++]);
			int spriteFrameWidth = std::stoi(tokens[index++]);
			int spriteFrameHeight = std::stoi(tokens[index++]);

			std::string spriteFilePath = tokens[index++];
			int spritePivotX = std::stoi(tokens[index++]);
			int spritePivotY = std::stoi(tokens[index++]);

			animStates.push_back(new AnimState(stateName, stateSpeed,
				new Sprite(spriteStartFrame, spriteEndFrame, spriteFrameWidth, spriteFrameHeight,
					manager->game->spriteManager, spriteFilePath,
					manager->game->renderer->shaders[ShaderName::Default],
					Vector2(spritePivotX, spritePivotY))));

			ss.getline(lineChar, 256);
		}
	}
	catch (const std::exception& ex)
	{
		const char* message = ex.what();
		std::cout << message << std::endl;
	}

	
}