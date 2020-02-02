#pragma once
#include <string>
#include <vector>
#include <unordered_map>

class CutsceneManager;

class CutsceneCommands
{
private:
	std::unordered_map<std::string, std::string> stralias;
public:
	CutsceneManager* manager = nullptr;
	CutsceneCommands();
	~CutsceneCommands();

	void ExecuteCommand(std::string command);
	
	int LoadSprite(const std::vector<std::string>& parameters);
	int ClearSprite(const std::vector<std::string>& parameters);
	int SetSpriteProperty(const std::vector<std::string>& parameters);
	
	
	int SetVelocity(const std::vector<std::string>& parameters);
	int Wait(const std::vector<std::string>& parameters);
	int Textbox(const std::vector<std::string>& parameters);
	int Fade(const std::vector<std::string>& parameters);
	int SetStringAlias(const std::vector<std::string>& parameters);

	std::string GetStringAlias(std::string param);
};

