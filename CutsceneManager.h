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
#include <map>

enum class SaveSections { CONFIG_OPTIONS, STORY_DATA, GOSUB_STACK, ALIAS_STRINGS, ALIAS_NUMBERS, 
	LOCAL_STRINGS, LOCAL_NUMBERS, LOCAL_OBJECTS, NAMES_TO_COLORS, OTHER_STUFF
};

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

struct BacklogData
{
	int labelIndex = 0;
	int lineIndex = 0;
	std::string text = "";

	BacklogData(int l, int i, const char* t)
	{
		labelIndex = l;
		lineIndex = i;
		text = t;
	}
};



class CutsceneManager
{
	std::string language = "english";
	std::string data = "";
	std::string currentText = "";
public:	 
	CutsceneCommands commands;
	bool useMouseControls = true;
	bool useKeyboardControls = true;
	int labelIndex = 0;
	int lineIndex = 0;
	int letterIndex = 0;
	int commandIndex = 0;
	int backlogIndex = 0;
	bool readingBacklog = false;
	Color backlogColor = { 255, 255, 0, 255 };
	std::vector<BacklogData> backlog;
	int backlogMaxSize = 3;
	unsigned int globalStart = 1000; //TODO: Should this be a config variable?
	//TODO: Move these button configurations to some place more relevant
	// This class should have a reference to the controller and get the bindings from it
	SDL_Scancode skipButton = SDL_Scancode::SDL_SCANCODE_LCTRL;
	SDL_Scancode autoButton = SDL_Scancode::SDL_SCANCODE_A;
	std::vector<SceneLabel*> labels;
	SceneLabel* currentLabel = nullptr;
	std::vector<SceneData*> gosubStack;
	const int choiceQuestionNumber = 10000;
	int buttonIndex = 0;
	int buttonResult = 0;
	bool watchingCutscene = false;
	bool waitingForButton = false;
	bool automaticallyRead = false;
	bool readingSameLine = false;
	float autoTimeToWait[3] = { 500, 2000, 8000 };
	int autoTimeIndex = 0;
	Timer autoReaderTimer;
	Timer inputTimer;
	float inputTimeToWait = 1000;
	Textbox* textbox = nullptr;
	Color currentColor = { 255, 255, 255, 255 };
	Game* game = nullptr;
	std::vector<std::string> choiceIfStatements;
	std::vector<unsigned int> activeButtons;
	std::unordered_map<unsigned int, unsigned int> spriteButtons;
	std::map<unsigned int, Entity*> images; // needs to be in order for rendering
	std::map<unsigned int, Entity*>::iterator imageIterator;
	std::unordered_map<std::string, Color> namesToColors;
	std::unordered_map<unsigned int, Timer*> timers;
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

	void CheckKeys();
	void ReadBacklog();

	void SaveGame();
	void LoadGame();

	void FlushCurrentColor();

	void LoadGlobalVariables();
	void SaveGlobalVariable(unsigned int key, unsigned int value);
	void SaveGlobalVariable(unsigned int key, const std::string& value);
	std::vector<string> GetVectorOfStringsFromFile(const char* filepath);
};
