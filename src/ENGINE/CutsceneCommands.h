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
class CutsceneHelper;

struct UserDefinedFunction
{
	std::string functionName = "";
	std::vector<std::string> parameters;
};

class CutsceneCommands;

typedef const std::vector<std::string>& CutsceneParameters;
typedef int (*CmdFunc)(CutsceneParameters parameters, CutsceneCommands& c);


class KINJO_API CutsceneCommands
{
public:
	//Look Up Table
	std::unordered_map<std::string, CmdFunc> cmd_lut;

	CutsceneHelper* helper = nullptr;

	std::unordered_map<std::string, std::string> stralias;
	std::unordered_map<std::string, unsigned int> numalias;

	std::unordered_map<unsigned int, std::string> stringVariables;
	std::unordered_map<unsigned int, int> numberVariables;
	std::unordered_map<unsigned int, std::vector<int>> arrayVariables;
	std::unordered_map<unsigned int, unsigned int> arrayNumbersPerSlot;

	std::vector<std::string> includeFilepaths;

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

	std::string ParseStringValue(const std::string& parameter);
	int ParseNumberValue(const std::string& parameter);
	Color ParseColorFromParameters(const std::vector<std::string>& parameters, const int index);

	int GetNumberVariable(const unsigned int key);
	std::string GetStringVariable(const unsigned int key);
	std::string GetStringAlias(const std::string& key);
	int GetNumAlias(const std::string& key);

	bool GetArray(const std::string& parameter);
	std::string GetArrayName(const std::string& parameter);

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