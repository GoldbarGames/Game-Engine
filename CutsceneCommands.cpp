#include "CutsceneCommands.h"
#include "CutsceneManager.h"
#include "Game.h"
#include <iostream>
#include <iterator>
#include <sstream>

//TODO: What's the best way to deal with the parameter counts?

CutsceneCommands::CutsceneCommands()
{

}

CutsceneCommands::~CutsceneCommands()
{

}

void CutsceneCommands::ExecuteCommand(std::string command)
{
	std::cout << command << std::endl;

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

		if (commandName == "set_velocity")
		{
			SetVelocity(parameters);
		}
		else if (commandName == "wait")
		{
			Wait(parameters);
		}
		else if (commandName == "textbox")
		{
			Textbox(parameters);
		}
		else if (commandName == "fade")
		{
			Fade(parameters);
		}
		else
		{
			// if no commands fit, use the properties code (set properties)
		}
	}
}

//TODO: Maybe put this code somewhere so it can be used
// both by the cutscene system and the level editor properties?
void CutsceneCommands::SetVelocity(const std::vector<std::string>& parameters)
{
	PhysicsEntity* entity = nullptr;

	for (int i = 0; i < manager->game->entities.size(); i++)
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
}

void CutsceneCommands::Wait(const std::vector<std::string>& parameters)
{
	int ms = std::stoi(parameters[1]);
	manager->timer -= ms;
}

void CutsceneCommands::Textbox(const std::vector<std::string>& parameters)
{
	if (parameters[1] == "on")
	{
		manager->textbox->shouldRender = true;
	}
	else if (parameters[1] == "off")
	{
		manager->textbox->shouldRender = false;
	}
}

void CutsceneCommands::Fade(const std::vector<std::string>& parameters)
{

}