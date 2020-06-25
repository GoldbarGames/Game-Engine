#ifndef CUTSCENECOMMANDS_H
#define CUTSCENECOMMANDS_H
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "Animator.h"
#include "globals.h"

class CutsceneManager;

struct UserDefinedFunction
{
	std::string functionName = "";
	std::vector<std::string> parameters;
};


typedef const std::vector<std::string>& CutsceneParameters;
class CutsceneCommands
{
public:
	std::unordered_map<std::string, std::string> stralias;
	std::unordered_map<std::string, unsigned int> numalias;
	std::unordered_map<unsigned int, std::string> stringVariables;
	std::vector<UserDefinedFunction*> userDefinedFunctions;

	//TODO: Make this only accessible to the manager
	std::unordered_map<unsigned int, int> numberVariables;

	// When you press the button, jump to the corresponding label
	std::unordered_map<unsigned int, std::string> buttonLabels;

	std::string choiceBGFilePath = "";
	std::string pathPrefix = "";
	int randomSeed = 0;

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
	int TextColor(CutsceneParameters parameters);
	int LoadTextFromSaveFile(CutsceneParameters parameters);

	// Sounds
	int MusicCommand(CutsceneParameters parameters);
	int SoundCommand(CutsceneParameters parameters);
	int MusicEffectCommand(CutsceneParameters parameters);

	// Stuff
	int SetVelocity(CutsceneParameters parameters);
	int Wait(CutsceneParameters parameters);
	int Textbox(CutsceneParameters parameters);
	int Namebox(CutsceneParameters parameters);
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

	int ConcatenateStringVariables(CutsceneParameters parameters);

	// Numeric Operations
	int AddNumberVariables(CutsceneParameters parameters);
	int SubtractNumberVariables(CutsceneParameters parameters);
	int MultiplyNumberVariables(CutsceneParameters parameters);
	int DivideNumberVariables(CutsceneParameters parameters);
	int ModNumberVariables(CutsceneParameters parameters);
	int RandomNumberVariable(CutsceneParameters parameters);
	int MoveVariables(CutsceneParameters parameters);
	int SubstringVariables(CutsceneParameters parameters);

	// Control Flow
	int GoToLabel(CutsceneParameters parameters);
	int IfCondition(CutsceneParameters parameters);
	int JumpBack(CutsceneParameters parameters);
	int JumpForward(CutsceneParameters parameters);
	int GoSubroutine(CutsceneParameters parameters);
	int ReturnFromSubroutine(CutsceneParameters parameters);
	int DisplayChoice(CutsceneParameters parameters);

	int WaitForButton(CutsceneParameters parameters);
	int SetSpriteButton(CutsceneParameters parameters);

	int EndGame(CutsceneParameters parameters);
	int ResetGame(CutsceneParameters parameters);
	int SaveGame(CutsceneParameters parameters);
	int LoadGame(CutsceneParameters parameters);

	int SetResolution(CutsceneParameters parameters);
	int DefineUserFunction(CutsceneParameters parameters);

	int DoNothing(CutsceneParameters parameters);

	unsigned int ParseNumberValue(const std::string& parameter);
	std::string ParseStringValue(const std::string& parameter);

	int SetGlobalNumber(CutsceneParameters parameters);
	int OpenBacklog(CutsceneParameters parameters);

	int TimerFunction(CutsceneParameters parameters);
	int CameraFunction(CutsceneParameters parameters);

	int WindowFunction(CutsceneParameters parameters);
	int ControlBindings(CutsceneParameters parameters);
	int BindKeyToLabel(CutsceneParameters parameters);

	int FlipSprite(CutsceneParameters parameters);

	
	Color ParseColorFromParameters(const std::vector<std::string>& parameters, const int index);

	int LuaCommand(CutsceneParameters parameters);
	int SetClickToContinue(CutsceneParameters parameters);

	int ScreenshotCommand(CutsceneParameters parameters);
	int ErrorLog(CutsceneParameters parameters);

	int FontCommand(CutsceneParameters parameters);

	int GetResourceFilename(CutsceneParameters parameters);
	int NameCommand(CutsceneParameters parameters);
	int NameDefineCommand(CutsceneParameters parameters);

	int IntToString(CutsceneParameters parameters);

	int IncrementVariable(CutsceneParameters parameters);
	int DecrementVariable(CutsceneParameters parameters);

	int Output(CutsceneParameters parameters);

	int TagCommand(CutsceneParameters parameters);
	int FileExist(CutsceneParameters parameters);
	int TextSpeed(CutsceneParameters parameters);
	int AutoMode(CutsceneParameters parameters);
	int AlignCommand(CutsceneParameters parameters);
	int InputCommand(CutsceneParameters parameters);
};

#endif