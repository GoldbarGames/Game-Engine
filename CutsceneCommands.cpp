#include "CutsceneCommands.h"
#include "CutsceneManager.h"
#include "Game.h"
#include <iostream>
#include <iterator>
#include <sstream>

typedef int (CutsceneCommands::*FuncList)(CutsceneParameters parameters);

//Look Up Table
static struct FuncLUT {
	char command[32];
	FuncList method;
};

std::vector<FuncLUT>cmd_lut = {
	{"add", &CutsceneCommands::AddNumberVariables},
	{"bg", &CutsceneCommands::LoadBackground },
	{"bgm", &CutsceneCommands::MusicCommand },
	{"cl", &CutsceneCommands::ClearSprite },
	{"div", &CutsceneCommands::DivideNumberVariables},
	{"fade", &CutsceneCommands::Fade },
	{"goto", &CutsceneCommands::GoToLabel },
	{"if", &CutsceneCommands::IfCondition },
	{"ld", &CutsceneCommands::LoadSprite },
	{"mod", &CutsceneCommands::ModNumberVariables},
	{"mul", &CutsceneCommands::MultiplyNumberVariables},
	{"numalias", &CutsceneCommands::SetNumAlias },
	{"se", &CutsceneCommands::SoundCommand },
	{"set_velocity", &CutsceneCommands::SetVelocity },
	{"setnumvar", &CutsceneCommands::SetNumberVariable },
	{"setstrvar", &CutsceneCommands::SetStringVariable },
	{"sprite", &CutsceneCommands::SetSpriteProperty },	
	{"stralias", &CutsceneCommands::SetStringAlias },
	{"sub", &CutsceneCommands::SubtractNumberVariables},
	{"textbox", &CutsceneCommands::Textbox },
	{"wait",& CutsceneCommands::Wait }
};

// TODO: Implement these commands:
// * Store/retrieve/display values via variables
// * Variable operations (add, sub, mul, div, mod)
// * Jump to labels
// * If/else control flow, compare strings/numbers/variables
// For loops, while loops
// Jump forward/back
// Dialogue options / choices
// Camera operations (pan, zoom, rotate, orthographic/perspective, other stuff)
// Playing animations
// Music effects (ME)
// Save/load?
// Display text on the screen as a sprite outside the textbox
// Math functions (abs, sin, cos, tan, etc.)
// Physics functions 
// Timers, set/reset/stop them
// Randomness/seed
// Lights and shadows?
// User-defined functions
// Visual Editor, modify cutscene as it is running, replay it
// Ending the game window / restarting the game window
// Output error logs
// Changing the file/directory where the script file is read from
// Change screen resolution / options
// Check if a file exists

CutsceneCommands::CutsceneCommands()
{
	numalias["bg"] = 0;
	numalias["l"] = 1;
	numalias["c"] = 2;
	numalias["r"] = 3;
}

CutsceneCommands::~CutsceneCommands()
{

}

void CutsceneCommands::ExecuteCommand(std::string command)
{
	// Break up the command string into a vector of strings, one word each
	//TODO: Is this the best way to do this?
	std::stringstream ss(command);
	std::istream_iterator<std::string> begin(ss);
	std::istream_iterator<std::string> end;

	std::vector<std::string> parameters(begin, end);
	std::copy(parameters.begin(), parameters.end(), std::ostream_iterator<std::string>(std::cout, "\n"));

	if (parameters.size() > 0)
	{
		bool commandFound = false;
		for (auto cmd : cmd_lut)
		{
			if (cmd.command == parameters[0])
			{
				commandFound = true;
				(this->*cmd.method)(parameters);				
				break;
			}				
		}

		if (!commandFound)
		{
			std::cout << "ERROR: Command " << parameters[0] << " not found.";
		}
	}
}

int CutsceneCommands::MusicCommand(CutsceneParameters parameters)
{
	//TODO: Deal with custom loop times

	if (parameters[1] == "play")
	{
		manager->game->soundManager->PlayBGM(parameters[2], true);
	}
	else if (parameters[1] == "once")
	{
		manager->game->soundManager->PlayBGM(parameters[2], false);
	}
	else if (parameters[1] == "stop")
	{
		manager->game->soundManager->StopBGM();
	}
	else if (parameters[1] == "fadein")
	{
		manager->game->soundManager->FadeInBGM(parameters[2], std::stoi(parameters[3]), true);
	}
	else if (parameters[1] == "fadeout")
	{
		manager->game->soundManager->FadeOutBGM(std::stoi(parameters[2]));
	}
	else if (parameters[1] == "volume")
	{
		manager->game->soundManager->SetVolumeBGM(std::stoi(parameters[2]));
	}

	return 0;
}

int CutsceneCommands::SoundCommand(CutsceneParameters parameters)
{
	if (parameters[1] == "play")
	{
		//TODO: Deal with multiple channels
		manager->game->soundManager->PlaySound(parameters[2], 1);
	}
	else if (parameters[1] == "volume")
	{
		manager->game->soundManager->SetVolumeSound(std::stoi(parameters[2]));
	}

	return 0;
}

int CutsceneCommands::IfCondition(CutsceneParameters parameters)
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

	switch (parameters[1][0])
	{
	case '$': // string variable
		leftHandIsNumber = false;
		leftValueStr = GetStringVariable(GetNumAlias(parameters[1]));
		break;
	case '%': // number variable
		leftHandIsNumber = true;
		leftValueNum = GetNumberVariable(GetNumAlias(parameters[1]));
		break;
	default:
		if (parameters[1].find_first_not_of("-0123456789") == std::string::npos)
		{
			leftValueNum = std::stoi(parameters[1]);
			leftHandIsNumber = true;
		}
		else
		{
			leftValueStr = parameters[1];
			leftHandIsNumber = false;
		}
			
		break;
	}

	switch (parameters[3][0])
	{
	case '$': // string variable
		rightHandIsNumber = false;
		rightValueStr = GetStringVariable(GetNumAlias(parameters[3]));
		break;
	case '%': // number variable
		rightHandIsNumber = true;
		rightValueNum = GetNumberVariable(GetNumAlias(parameters[3]));
		break;
	default:
		if (parameters[3].find_first_not_of("-0123456789") == std::string::npos)
		{
			rightValueNum = std::stoi(parameters[3]);
			rightHandIsNumber = true;
		}			
		else
		{
			rightValueStr = parameters[3];
			rightHandIsNumber = false;
		}
			
		break;
	}

	// Don't do any comparison if they are not the same type
	if (leftHandIsNumber != rightHandIsNumber)
		return 0;

	bool conditionIsTrue = false;

	if (parameters[2] == ">")
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
	else if (parameters[2] == ">=")
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
	else if (parameters[2] == "<")
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
	else if (parameters[2] == "<=")
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
	else if (parameters[2] == "==")
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
	else if (parameters[2] == "!=")
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

	// If the condition is true, execute the following command
	if (conditionIsTrue)
	{
		std::string nextCommand = "";

		for (int i = 4; i < parameters.size(); i++)
			nextCommand += (parameters[i] + " ");

		ExecuteCommand(nextCommand);
	}

}

int CutsceneCommands::GoToLabel(CutsceneParameters parameters)
{
	// Check the label name to see if it is a variable
	switch (parameters[1][0])
	{
	case '$': // string variable
		manager->PlayCutscene(GetStringVariable(GetNumAlias(parameters[1])).c_str());
		break;
	default:
		manager->PlayCutscene(parameters[1].c_str());
		break;
	}

	return 0;
}

int CutsceneCommands::AddNumberVariables(CutsceneParameters parameters)
{
	unsigned int number1 = GetNumAlias(parameters[1]);
	unsigned int number2 = GetNumAlias(parameters[2]);

	numberVariables[number1] = numberVariables[number1] + numberVariables[number2];

	return 0;
}

int CutsceneCommands::SubtractNumberVariables(CutsceneParameters parameters)
{
	unsigned int number1 = GetNumAlias(parameters[1]);
	unsigned int number2 = GetNumAlias(parameters[2]);

	numberVariables[number1] = numberVariables[number1] - numberVariables[number2];

	return 0;
}

int CutsceneCommands::MultiplyNumberVariables(CutsceneParameters parameters)
{
	unsigned int number1 = GetNumAlias(parameters[1]);
	unsigned int number2 = GetNumAlias(parameters[2]);

	numberVariables[number1] = numberVariables[number1] * numberVariables[number2];

	return 0;
}

int CutsceneCommands::DivideNumberVariables(CutsceneParameters parameters)
{
	unsigned int number1 = GetNumAlias(parameters[1]);
	unsigned int number2 = GetNumAlias(parameters[2]);

	numberVariables[number1] = numberVariables[number1] / numberVariables[number2];

	return 0;
}

int CutsceneCommands::ModNumberVariables(CutsceneParameters parameters)
{
	unsigned int number1 = GetNumAlias(parameters[1]);
	unsigned int number2 = GetNumAlias(parameters[2]);

	numberVariables[number1] = numberVariables[number1] % numberVariables[number2];

	return 0;
}

int CutsceneCommands::SetNumberVariable(CutsceneParameters parameters)
{
	unsigned int key = GetNumAlias(parameters[1]);
	unsigned int value = GetNumAlias(parameters[2]);

	if (GetNumberVariable(value) == 0)
		numberVariables[key] = value;
	else
		numberVariables[key] = GetNumberVariable(value);

	return 0;
}

int CutsceneCommands::SetStringVariable(CutsceneParameters parameters)
{
	unsigned int key = GetNumAlias(parameters[1]);

	std::string value = parameters[2];
	if (value[0] == '[')
	{
		unsigned int varNameIndex = 1;
		value = ParseWord(value, ']', varNameIndex);
	}
	else
	{
		value = GetStringVariable(GetNumAlias(value));
	}

	stringVariables[key] = value;
	return 0;
}

int CutsceneCommands::GetNumberVariable(const unsigned int key)
{
	if (numberVariables.find(key) == numberVariables.end())
	{
		return 0;
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
	unsigned int value = std::stoi(parameters[2]);
	//TODO: Check for errors
	numalias[key] = value;
	return 0;
}

unsigned int CutsceneCommands::GetNumAlias(const std::string& key)
{
	if (numalias.find(key) == numalias.end())
	{
		if (key.find_first_not_of("-0123456789") == std::string::npos)
			return std::stoi(key);
		else
		{
			std::cout << "ERROR: Numalias not defined for " << key << std::endl;
			return 0;
		}
	}
	else
	{
		return numalias[key];
	}
}

int CutsceneCommands::LoadBackground(CutsceneParameters parameters)
{
	std::vector<string> newParameters;
	newParameters.push_back("");
	newParameters.push_back(parameters[0]);
	newParameters.push_back(parameters[1]);
	LoadSprite(newParameters);
	return 0;
}

int CutsceneCommands::ClearSprite(CutsceneParameters parameters)
{
	unsigned int imageNumber = GetNumAlias(parameters[1]);

	if (manager->images[imageNumber] != nullptr)
		delete manager->images[imageNumber];

	manager->images[imageNumber] = nullptr;

	return 0;
}

int CutsceneCommands::LoadSprite(CutsceneParameters parameters)
{
	Vector2 pos = Vector2(0, 0);

	bool isStandingImage = parameters[1] == "l" || parameters[1] == "c" || parameters[1] == "r";

	if (!isStandingImage && parameters[1] != "bg")
	{
		const unsigned int x = std::stoi(parameters[3]);
		const unsigned int y = std::stoi(parameters[4]);

		pos = Vector2(x, y);
	}

	std::string filepath = GetStringAlias(parameters[2]);
	unsigned int imageNumber = GetNumAlias(parameters[1]);

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

int CutsceneCommands::SetSpriteProperty(CutsceneParameters parameters)
{
	unsigned int imageNumber = GetNumAlias(parameters[1]);

	Entity* entity = manager->images[imageNumber];
	if (entity == nullptr)
		return 1; //TODO: Error log

	//Sprite* sprite = manager->images[imageNumber]->GetSprite();
	if (entity->GetSprite() == nullptr)
		return 2; //TODO: Error log

	std::string spriteProperty = parameters[2];

	if (spriteProperty == "color")
	{
		Color color = { std::stoi(parameters[3]), std::stoi(parameters[4]), 
			std::stoi(parameters[5]), std::stoi(parameters[6]) };

		entity->GetSprite()->color = color;
	}
	else if (spriteProperty == "scale")
	{
		entity->GetSprite()->scale = Vector2(std::stoi(parameters[3]), std::stoi(parameters[4]));
	}
	else if (spriteProperty == "rotate")
	{
		entity->rotation = glm::vec3(std::stoi(parameters[3]), std::stoi(parameters[4]), std::stoi(parameters[5]));
	}
	else if (spriteProperty == "shader")
	{	
		//TODO: Fix this so that it works with enums
		//if (manager->game->renderer->GetShaderFromString(parameters[3]) != nullptr)
		//	sprite->shader = manager->game->renderer->GetShaderFromString(parameters[3]);
		//TODO: Log and display error if cannot find shader?
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
				float x = std::stof(parameters[2]);
				float y = std::stof(parameters[3]);
				entity->SetVelocity(Vector2(x, y));
			}
			break;
		}
	}

	return 0;
}

int CutsceneCommands::Wait(CutsceneParameters parameters)
{
	int ms = std::stoi(parameters[1]);
	manager->timer -= ms;

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

	return 0;
}