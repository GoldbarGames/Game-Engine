#pragma once
#include <string>
#include <vector>

class CutsceneManager;

class CutsceneCommands
{
public:
	CutsceneManager* manager = nullptr;
	CutsceneCommands();
	~CutsceneCommands();

	void ExecuteCommand(std::string command);
	
	int LoadSprite(const std::vector<std::string>& parameters);
	int SetSpriteProperty(const std::vector<std::string>& parameters);
	
	
	int SetVelocity(const std::vector<std::string>& parameters);
	int Wait(const std::vector<std::string>& parameters);
	int Textbox(const std::vector<std::string>& parameters);
	int Fade(const std::vector<std::string>& parameters);
};

