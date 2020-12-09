#ifndef CUTSCENECOMMANDS_H
#define CUTSCENECOMMANDS_H
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "Animator.h"
#include "globals.h"
#include "leak_check.h"
class CutsceneManager;

struct UserDefinedFunction
{
	std::string functionName = "";
	std::vector<std::string> parameters;
};


typedef const std::vector<std::string>& CutsceneParameters;
class KINJO_API CutsceneCommands
{
public:
	std::unordered_map<std::string, std::string> stralias;
	std::unordered_map<std::string, unsigned int> numalias;

	std::unordered_map<unsigned int, std::string> stringVariables;
	std::unordered_map<unsigned int, int> numberVariables;
	std::unordered_map<unsigned int, std::vector<int>> arrayVariables;
	std::unordered_map<unsigned int, unsigned int> arrayNumbersPerSlot;

	// Imagine we want to create an array of 10 things, with a key of 123
	// Slot #123 has 10 things in it: x x x x x x x x x x

	// Imagine instead that slot #123 should have 4 slots that each contain 5 things in them.
	// Slot #123 has 20 things: a a a a a b b b b b c c c (c) c d d d d d

	// So let's pretend we want to get the value of [2][3] from slot #123
	// 5 * 2 = 10, + 3 = 13
	// numbersPerSlot * desiredSlot + desiredThing

	// TODO: N-dimensional arrays > 2 ?

	std::unordered_map<std::string, UserDefinedFunction*> userDefinedFunctions;	

	// When you press the button, jump to the corresponding label
	std::unordered_map<unsigned int, std::string> buttonLabels;
	std::unordered_map<unsigned int, bool> buttonLabelsActive;

	std::unordered_map<std::string, std::string> cacheParseStrings;
	std::unordered_map<std::string, int> cacheParseNumbers;

	std::string parseStringValue = "";
	int parseNumberValue = 0;
	int lineBreaks = 0;

	const std::string DIGITMASK = "-0123456789";

	std::string textFontKey = "";
	std::string choiceBGFilePath = "";
	std::string pathPrefix = "";
	int randomSeed = 0;
	bool shouldOutput = false;
	bool outputCommands = false;

	int arrayIndex = 0;
	int vectorIndex = 0;

	Vector2 currentQuakePosition = Vector2(0, 0);
	Timer quakeTimer;
	int quakeIntensity = 0;
	bool isQuakeHorizontal = false;
	bool isQuakeVertical = false;
	int quakeCount = 0;
	int quakeNumberOfLoops = 0;

	CutsceneManager* manager = nullptr;
	CutsceneCommands();
	~CutsceneCommands();

	//TODO: Make parameter const
	int ExecuteCommand(std::string command);
	
	// Load graphics
	int LoadSprite(CutsceneParameters parameters);
	int ClearSprite(CutsceneParameters parameters);
	int SetSpriteProperty(CutsceneParameters parameters);
	int LoadBackground(CutsceneParameters parameters);
	int AnimationCommand(CutsceneParameters parameters);
	int ParticleCommand(CutsceneParameters parameters);

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
	int GetNumAlias(const std::string& key);

	int CreateArrayVariable(CutsceneParameters parameters);
	bool GetArray(const std::string& parameter);
	std::string GetArrayName(const std::string& parameter);

	int ConcatenateStringVariables(CutsceneParameters parameters);

	// Numeric Operations
	void CacheNumberVariables(CutsceneParameters parameters);
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

	int WaitForClick(CutsceneParameters parameters);
	int WaitForButton(CutsceneParameters parameters);
	int SetSpriteButton(CutsceneParameters parameters);

	int EndGame(CutsceneParameters parameters);
	int ResetGame(CutsceneParameters parameters);
	int SaveGame(CutsceneParameters parameters);
	int LoadGame(CutsceneParameters parameters);

	int SetResolution(CutsceneParameters parameters);
	int DefineUserFunction(CutsceneParameters parameters);

	int DoNothing(CutsceneParameters parameters);

	int ParseNumberValue(const std::string& parameter);
	std::string ParseStringValue(const std::string& parameter);

	int SetGlobalNumber(CutsceneParameters parameters);
	int OpenBacklog(CutsceneParameters parameters);

	int TimerFunction(CutsceneParameters parameters);
	int CameraFunction(CutsceneParameters parameters);

	int WindowFunction(CutsceneParameters parameters);
	int ControlBindings(CutsceneParameters parameters);
	int BindKeyToLabel(CutsceneParameters parameters);

	int FlipSprite(CutsceneParameters parameters);

	int RightClickSettings(CutsceneParameters parameters);
	int Quake(CutsceneParameters parameters);
	
	Color ParseColorFromParameters(const std::vector<std::string>& parameters, const int index);

	int LuaCommand(CutsceneParameters parameters);
	int SetClickToContinue(CutsceneParameters parameters);

	int ScreenshotCommand(CutsceneParameters parameters);
	int ErrorLog(CutsceneParameters parameters);

	int FontCommand(CutsceneParameters parameters);

	int GetData(CutsceneParameters parameters);
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
	int AutoReturn(CutsceneParameters parameters);
	int AutoSave(CutsceneParameters parameters);
	int AutoChoice(CutsceneParameters parameters);
	int AlignCommand(CutsceneParameters parameters);
	int InputCommand(CutsceneParameters parameters);

	int PrintCommand(CutsceneParameters parameters);
	int EffectCommand(CutsceneParameters parameters);
	int LineBreakCommand(CutsceneParameters parameters);

	int RepeatCommand(CutsceneParameters parameters);
	int TravelCommand(CutsceneParameters parameters);

	int ToggleSkipping(CutsceneParameters parameters);
	int IsSkipping(CutsceneParameters parameters);

	int CreateShader(CutsceneParameters parameters);
	int SetShaderFilter(CutsceneParameters parameters);
	int ShellCommand(CutsceneParameters parameters);

	int SteamCommand(CutsceneParameters parameters);
	int DrawRectCommand(CutsceneParameters parameters);

	unsigned int key = 0;
	unsigned int number1 = 0;
	unsigned int number2 = 0;
	std::string word1 = "";
	std::string word2 = "";

	Timer commandTimer;

	bool leftHandIsNumber = false;
	bool rightHandIsNumber = false;

	std::string leftValueStr = "";
	std::string rightValueStr = "";
	int leftValueNum = 0;
	int rightValueNum = 0;

	std::string nextCommand = "";
	std::vector<std::string> subcommands;
	std::string shaderFilter = "";
	int filterMin = -1;
	int filterMax = 999999;

	std::unordered_map<std::string, ShaderProgram*> customShaders;

};

#endif