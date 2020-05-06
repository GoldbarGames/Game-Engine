#pragma once
#include "Textbox.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>

#include "Timer.h"
#include "CutsceneCommands.h"


class Game;

class SceneLine
{
public:
	std::string text = "";
	std::string speaker = "";
	std::vector<std::string> commands;

	SceneLine(std::string txt = "", std::string name = "")
	{
		text = txt;
		speaker = name;
	}
};

class SceneLabel
{
public:
	std::string name = "";
	std::vector<SceneLine*> lines;

	~SceneLabel()
	{
		for (unsigned int i = 0; i < lines.size(); i++)
		{
			delete lines[i];
		}
		lines.clear();
	}
};

// Use this for returning from a gosub or loading saved data
struct SceneData
{
public:
	std::string labelName = "";
	std::string lineText = "";
	int labelIndex = 0;
	int lineIndex = 0;
	int commandIndex = 0;
};

class CutsceneManager
{	
	std::string language = "english";
	std::string data = "";
	int labelIndex = 0;
	int lineIndex = 0;
	int letterIndex = 0;
	int commandIndex = 0;
	std::string currentText = "";
	CutsceneCommands commands;
	std::queue<std::string> backlog;
	int backlogMaxSize = 3;
public:	 
	std::vector<SceneLabel*> labels;
	SceneLabel* currentLabel = nullptr;
	std::vector<SceneData*> gosubStack;
	const int choiceQuestionNumber = 10000;
	int buttonIndex = 0;
	int buttonResult = 0;
	bool watchingCutscene = false;
	bool waitingForButton = false;
	Textbox* textbox = nullptr;
	Game* game = nullptr;
	std::vector<std::string> choiceIfStatements;
	std::vector<unsigned int> activeButtons;
	std::unordered_map<unsigned int, unsigned int> spriteButtons;
	std::unordered_map<unsigned int, Entity*> images;
	std::unordered_map<unsigned int, Entity*>::iterator imageIterator;
	float timer = 0;
	bool isCarryingOutCommands = false;
	bool isReadingNextLine = false;
	Timer nextLetterTimer;
	CutsceneManager(Game& g);
	void ParseScene();
	void Update();
	void Render(Renderer* renderer);
	SceneLabel* JumpToLabel(const char* newLabelName);
	void PlayCutscene(const char* labelName);
	void EndCutscene();
	void ReadNextLine();
	void ClearAllSprites();
	void JumpForward();
	void JumpBack();
	void PushCurrentSceneDataToStack();
	SceneData* PopSceneDataFromStack();
};
