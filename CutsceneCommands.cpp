#include "CutsceneCommands.h"
#include "CutsceneManager.h"
#include "Game.h"
#include <iostream>
#include <iterator>
#include <sstream>

typedef int (CutsceneCommands::*FuncList)(CutsceneParameters parameters);
static struct FuncLUT {
	char command[30];
	FuncList method;
	int size = 12;
} cmd_lut[] = {
	{"wait", &CutsceneCommands::Wait },
	{"set_velocity", &CutsceneCommands::SetVelocity },
	{"textbox", &CutsceneCommands::Textbox },
	{"fade", &CutsceneCommands::Fade },
	{"ld", &CutsceneCommands::LoadSprite },
	{"cl", &CutsceneCommands::ClearSprite },
    {"sprite", &CutsceneCommands::SetSpriteProperty },
	{"bg", &CutsceneCommands::LoadBackground },
	{"bgm", &CutsceneCommands::MusicCommand },
	{"se", &CutsceneCommands::SoundCommand },
	{"numalias", &CutsceneCommands::SetNumAlias },
	{"stralias", &CutsceneCommands::SetStringAlias }
};

//TODO: What's the best way to deal with the parameter counts?

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
		if (key.find_first_not_of("0123456789") == std::string::npos)
			return std::stoi(key);
		else
			return 0;
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
		filepath, manager->game->renderer->shaders["default"], Vector2(0, 0)));

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
	manager->images[imageNumber]->GetSprite()->renderRelativeToCamera = true;
	manager->images[imageNumber]->GetSprite()->keepScaleRelativeToCamera = true;

	return 0;
}

int CutsceneCommands::SetSpriteProperty(CutsceneParameters parameters)
{
	unsigned int imageNumber = GetNumAlias(parameters[1]);

	if (manager->images[imageNumber] == nullptr)
		return 1; //TODO: Error log

	Sprite* sprite = manager->images[imageNumber]->GetSprite();
	if (sprite == nullptr)
		return 2; //TODO: Error log

	std::string spriteProperty = parameters[2];

	if (spriteProperty == "color")
	{
		Color color = { std::stoi(parameters[3]), std::stoi(parameters[4]), 
			std::stoi(parameters[5]), std::stoi(parameters[6]) };

		sprite->color = color;
	}
	else if (spriteProperty == "shader")
	{	
		if (manager->game->renderer->shaders[parameters[3]] != nullptr)
			sprite->shader = manager->game->renderer->shaders[parameters[3]];
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