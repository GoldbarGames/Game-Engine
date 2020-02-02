#include "CutsceneCommands.h"
#include "CutsceneManager.h"
#include "Game.h"
#include <iostream>
#include <iterator>
#include <sstream>

typedef int (CutsceneCommands::*FuncList)(const std::vector<std::string>& parameters);
static struct FuncLUT {
	char command[30];
	FuncList method;
	int size = 8;
} cmd_lut[] = {
	{"wait", &CutsceneCommands::Wait },
	{"set_velocity", &CutsceneCommands::SetVelocity },
	{"textbox", &CutsceneCommands::Textbox },
	{"fade", &CutsceneCommands::Fade },
	{"ld", &CutsceneCommands::LoadSprite },
	{"cl", &CutsceneCommands::ClearSprite },
    {"sprite", &CutsceneCommands::SetSpriteProperty },
	{"stralias", &CutsceneCommands::SetStringAlias }
};

//TODO: What's the best way to deal with the parameter counts?

CutsceneCommands::CutsceneCommands()
{

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
		std::string commandName = parameters[0];

		bool commandFound = false;
		for (int i = 0; i < cmd_lut->size; i++)
		{
			if (cmd_lut[i].command == commandName)
			{
				commandFound = true;
				(this->*cmd_lut[i].method)(parameters);
				break;
			}
		}

		if (!commandFound)
		{
			// if no commands fit, use the properties code (set properties)
		}
	}
}

int CutsceneCommands::SetStringAlias(const std::vector<std::string>& parameters)
{
	std::string key = parameters[1];
	std::string value = parameters[2];
	stralias[key] = value;
	return 0;
}

std::string CutsceneCommands::GetStringAlias(std::string key)
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

int CutsceneCommands::ClearSprite(const std::vector<std::string>& parameters)
{
	char location = parameters[1][0];

	delete manager->textbox->sprites[location];
	manager->textbox->sprites[location] = nullptr;

	return 0;
}

int CutsceneCommands::LoadSprite(const std::vector<std::string>& parameters)
{
	char location = parameters[1][0];
	std::string filepath = GetStringAlias(parameters[2]);

	if (manager->textbox->sprites[location] != nullptr)
		delete manager->textbox->sprites[location];

	manager->textbox->sprites[location] = new Sprite(1, manager->game->spriteManager,
		filepath, manager->game->renderer->shaders["default"], Vector2(0, 0));

	manager->textbox->sprites[location]->renderRelativeToCamera = true;
	manager->textbox->sprites[location]->keepScaleRelativeToCamera = true;

	return 0;
}

int CutsceneCommands::SetSpriteProperty(const std::vector<std::string>& parameters)
{
	char location = parameters[1][0];
	Sprite* sprite = manager->textbox->sprites[location];
	if (sprite == nullptr)
		return 1;

	std::string spriteProperty = parameters[2];

	if (spriteProperty == "color")
	{
		Color color = { std::stoi(parameters[3]), std::stoi(parameters[4]), 
			std::stoi(parameters[5]), std::stoi(parameters[6]) };

		sprite->color = color;
	}
	else if (spriteProperty == "shader")
	{	
		sprite->shader = manager->game->renderer->shaders[parameters[3]];
	}

	return 0;
}

//TODO: Maybe put this code somewhere so it can be used
// both by the cutscene system and the level editor properties?
int CutsceneCommands::SetVelocity(const std::vector<std::string>& parameters)
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

int CutsceneCommands::Wait(const std::vector<std::string>& parameters)
{
	int ms = std::stoi(parameters[1]);
	manager->timer -= ms;

	return 0;
}

int CutsceneCommands::Textbox(const std::vector<std::string>& parameters)
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

int CutsceneCommands::Fade(const std::vector<std::string>& parameters)
{
	manager->game->changingOverlayColor = true;

	if (parameters[1] == "clear")
	{
		manager->game->targetColor = Color { 0, 0, 0, 0 };
	}
	else if (parameters[1] == "white")
	{
		manager->game->targetColor = Color{ 255, 255, 255, 255 };
	}
	else if (parameters[1] == "black")
	{
		manager->game->targetColor = Color{0, 0, 0, 255 };
	}

	return 0;
}