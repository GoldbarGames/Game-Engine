#ifndef CUTSCENEMANAGER_H
#define CUTSCENEMANAGER_H
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

enum class SaveSections { CONFIG_OPTIONS, STORY_DATA, SEEN_LINES, GOSUB_STACK, 
	ALIAS_STRINGS, ALIAS_NUMBERS, LOCAL_STRINGS, LOCAL_NUMBERS, LOCAL_OBJECTS, 
	NAMES_TO_COLORS, OTHER_STUFF
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

	SceneLine(SceneLine* line)
	{
		if (line != nullptr)
		{
			text = line->text;
			speaker = line->speaker;
			commands = line->commands;
		}
	}
};

class SceneLabel
{
public:
	std::string name = "";
	std::vector<SceneLine> lines;

	~SceneLabel()
	{
		for (unsigned int i = 0; i < lines.size(); i++)
		{
			//delete lines[i];
		}
		lines.clear();
	}

	SceneLabel()
	{
		
	}

	SceneLabel(SceneLabel* label)
	{
		if (label != nullptr)
		{
			name = label->name;
			lines = label->lines;
		}
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
	std::string text = "";
	int labelIndex = 0;
	int lineIndex = 0;	
	float lastX = 0;
	float lastY = 0;

	BacklogData(int l, int i, const char* t, float lx = 0, float ly = 0)
	{
		labelIndex = l;
		lineIndex = i;
		text = t;
		lastX = lx;
		lastY = ly;
	}
};


struct TextTag
{
	bool active = false;
	//TODO: How will we allow for custom tags?
	//std::string name = "";
	//TextTag(std::string n) : name(n) { }
};


class CutsceneManager
{
	std::string language = "english";
	std::string data = "";
	std::string currentText = "";
public:	 
	bool rclickEnabled = true;
	bool autoreturn = false;
	bool autosave = false;
	bool autoprint = false;
	int printNumber = 0;
	bool overwriteName = true;
	std::string currentScript = "";
	CutsceneCommands commands;
	Timer printTimer;
	bool useMouseControls = true;
	bool useKeyboardControls = true;
	int labelIndex = 0;
	int lineIndex = 0;
	int letterIndex = 0;
	int commandIndex = 0;
	int backlogIndex = 0;
	bool backlogEnabled = false;
	bool readingBacklog = false;
	Color backlogColor = { 255, 255, 0, 255 };
	std::vector<std::string> unfinishedCommands;
	std::vector<BacklogData*> backlog;
	int backlogMaxSize = 3;
	unsigned int globalStart = 1000; //TODO: Should this be a config variable?
	//TODO: Move these button configurations to some place more relevant
	// This class should have a reference to the controller and get the bindings from it

	SDL_Scancode skipButton = SDL_Scancode::SDL_SCANCODE_LCTRL; //TODO: multiple buttons?
	SDL_Scancode skipButton2 = SDL_Scancode::SDL_SCANCODE_RCTRL;

	SDL_Scancode readButton = SDL_Scancode::SDL_SCANCODE_SPACE;
	SDL_Scancode readButton2 = SDL_Scancode::SDL_SCANCODE_RETURN;

	SDL_Scancode autoButton = SDL_Scancode::SDL_SCANCODE_A;

	std::vector<SceneLabel> labels;

	SceneLabel* currentLabel = nullptr;
	std::vector<SceneData*> gosubStack;

	const int choiceSpriteStartNumber = 10000;
	int buttonIndex = 0;
	int buttonResult = 0;
	bool watchingCutscene = false;
	bool waitingForButton = false;
	bool automaticallyRead = false;
	bool readingSameLine = false;
	float autoTimesToWait[3] = { 500, 2000, 8000 };
	float autoTimeToWait = 1000;
	float autoTimeToWaitPerGlyph = 100;
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

	std::map<std::string, Entity*> animatedImages;
	std::map<unsigned int, Entity*> images; // needs to be in order for rendering
	std::map<unsigned int, Entity*>::iterator imageIterator;

	std::map<unsigned int, unsigned int> seenLabelsToMostRecentLine;
	std::unordered_map<std::string, std::string> namesToNames;
	std::unordered_map<std::string, Color> namesToColors;
	std::unordered_map<std::string, TextTag*> tags;
	std::unordered_map<unsigned int, Timer*> timers;

	float msGlyphTime = 0;
	float msInitialDelayBetweenGlyphs = 10.0f;
	float msDelayBetweenGlyphs = 10.0f;
	bool isCarryingOutCommands = false;
	bool isReadingNextLine = false;
	Timer nextLetterTimer;
	Uint32 previousMouseState = 0;
	bool clickedMidPage = false;

	CutsceneManager();
	void Init(Game& g);
	void ParseScene();
	void ParseConfig(const char* configName);
	void Update();
	void Render(const Renderer& renderer);
	SceneLabel* JumpToLabel(const char* newLabelName);
	void PlayCutscene(const char* labelName);
	void EndCutscene();
	void ReadNextLine();
	void ClearAllSprites();
	void JumpForward();
	void JumpBack();
	void PushCurrentSceneDataToStack();
	bool PopSceneDataFromStack(SceneData& data);
	std::string ParseText(const std::string& originalString, int& letterIndex, Color& textColor, Text* text);
	~CutsceneManager();

	void CheckKeys();
	void CheckKeysWhileReading();
	void ReadBacklog();
	void UpdateText();

	void SaveGame(const char* filename, const char* path = "saves/");
	void LoadGame(const char* filename, const char* path = "saves/");

	void FlushCurrentColor();

	void LoadGlobalVariables();

	void SaveGlobalVariable(unsigned int key, const std::string& value, bool isNumber);

	void ModifyGlobalVariableVector(std::vector<string>& globalData, unsigned int key, const std::string& value);

	void SetSpeakerText(const std::string& name);

	void ReadCutsceneFile();

	SceneLabel* GetCurrentLabel();
	SceneLine* GetCurrentLine();
};

#endif