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
	
	void SetVelocity(const std::vector<std::string>& parameters);
	void Wait(const std::vector<std::string>& parameters);
	void Textbox(const std::vector<std::string>& parameters);
	void Fade(const std::vector<std::string>& parameters);
};

