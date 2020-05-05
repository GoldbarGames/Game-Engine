#pragma once
#include <string>
#include <vector>
#include <unordered_map>

class CutsceneManager;

typedef const std::vector<std::string>& CutsceneParameters;
class CutsceneCommands
{
private:
	std::unordered_map<std::string, std::string> stralias;
	std::unordered_map<std::string, unsigned int> numalias;
	std::unordered_map<unsigned int, std::string> stringVariables;
public:
	//TODO: Make this only accessible to the manager
	std::unordered_map<unsigned int, int> numberVariables;
	CutsceneManager* manager = nullptr;
	CutsceneCommands();
	~CutsceneCommands();

	void ExecuteCommand(std::string command);
	
	// Load graphics
	int LoadSprite(CutsceneParameters parameters);
	int ClearSprite(CutsceneParameters parameters);
	int SetSpriteProperty(CutsceneParameters parameters);
	int LoadBackground(CutsceneParameters parameters);

	// Text
	int LoadText(CutsceneParameters parameters);

	// Sounds
	int MusicCommand(CutsceneParameters parameters);
	int SoundCommand(CutsceneParameters parameters);
	int MusicEffectCommand(CutsceneParameters parameters);

	// Stuff
	int SetVelocity(CutsceneParameters parameters);
	int Wait(CutsceneParameters parameters);
	int Textbox(CutsceneParameters parameters);
	int Fade(CutsceneParameters parameters);
	int SetStringAlias(CutsceneParameters parameters);
	int SetNumAlias(CutsceneParameters parameters);

	// Variables
	int SetNumberVariable(CutsceneParameters parameters);
	int GetNumberVariable(const unsigned int key);
	int SetStringVariable(CutsceneParameters parameters);
	std::string GetStringVariable(const unsigned int key);
	std::string GetStringAlias(const std::string& key);
	unsigned int GetNumAlias(const std::string& key);

	// Numeric Operations
	int AddNumberVariables(CutsceneParameters parameters);
	int SubtractNumberVariables(CutsceneParameters parameters);
	int MultiplyNumberVariables(CutsceneParameters parameters);
	int DivideNumberVariables(CutsceneParameters parameters);
	int ModNumberVariables(CutsceneParameters parameters);

	// Control Flow
	int GoToLabel(CutsceneParameters parameters);
	int IfCondition(CutsceneParameters parameters);
	int JumpBack(CutsceneParameters parameters);
	int JumpForward(CutsceneParameters parameters);
	int GoSubroutine(CutsceneParameters parameters);
	int ReturnFromSubroutine(CutsceneParameters parameters);

	int WaitForButton(CutsceneParameters parameters);
	int SetSpriteButton(CutsceneParameters parameters);

	int EndGame(CutsceneParameters parameters);
	int ResetGame(CutsceneParameters parameters);
	int SaveGame(CutsceneParameters parameters);
	int LoadGame(CutsceneParameters parameters);



	int DoNothing(CutsceneParameters parameters);
};

