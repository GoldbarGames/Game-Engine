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
#include "leak_check.h"
#include "Timer.h"
#include "CutsceneCommands.h"
#include "CutsceneHelper.h"
#include <map>
#include <unordered_map>

enum class SaveSections { CONFIG_OPTIONS, STORY_DATA, SEEN_CHOICES, SEEN_LINES, GOSUB_STACK, 
	ALIAS_STRINGS, ALIAS_NUMBERS, LOCAL_STRINGS, LOCAL_NUMBERS, LOCAL_OBJECTS, 
	NAMES_TO_COLORS, OTHER_STUFF
};

class Game;

class SceneLine
{
public:
	int textStart = 0;
	int textEnd = 0;

	int speakerStart = -1;
	int speakerEnd = -1;

	int commandsStart = -1;
	int commandsSize = 0;

	SceneLine(int ts, int te, int ss, int se)
	{
		textStart = ts;
		textEnd = te;
		speakerStart = ss;
		speakerEnd = se;
	}

	SceneLine(SceneLine* line)
	{
		if (line != nullptr)
		{
			textStart = line->textStart;
			textEnd = line->textEnd;
			speakerStart = line->speakerStart;
			speakerEnd = line->speakerEnd;
			commandsStart = line->commandsStart;
			commandsSize = line->commandsSize;
		}
	}

	int GetTextLength()
	{
		return textEnd - textStart;
	}
};

class SceneLabel
{
public:
	int nameStart = 0;
	int nameEnd = 0;
	
	int lineStart = -1;
	int lineSize = 0;

	~SceneLabel()
	{

	}

	SceneLabel()
	{

	}

	SceneLabel(SceneLabel* label)
	{
		if (label != nullptr)
		{
			nameStart = label->nameStart;
			nameEnd = label->nameEnd;
			lineStart = label->lineStart;
			lineSize = label->lineSize;
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

	// TODO: Don't use strings here, but ok for now
	std::vector<std::string> commands;
};

struct SceneRepeatData
{
	int label = 0;
	int line = 0;
	int command = 0;
	int count = 0;
	int end = 0;
};

struct BacklogData
{
	std::string text = "";
	std::string speaker = "";

	BacklogData(const std::string& t, const std::string& s)
	{
		text = t;
		speaker = s;
	}
};


struct TextTag
{
	bool active = false;
	//TODO: How will we allow for custom tags?
	//std::string name = "";
	//TextTag(std::string n) : name(n) { }
};

struct PrintEffect
{
	int delay = 1;
	std::string mask = "";
};

// Each one of these is a unique choice within the game
struct SceneChoice
{	
	std::string label = "";
	std::string prompt = ""; 
	std::vector<std::string> responses; 
	std::vector<std::string> labels;
	std::string resultVariable = "";

	std::vector<std::string> GetCommandString()
	{
		std::vector<std::string> cmd;

		const int size = responses.size();
		cmd.reserve(4 + (size * 2));
		cmd.emplace_back("choice");
		cmd.emplace_back(std::to_string(size));
		cmd.emplace_back(resultVariable);
		cmd.emplace_back(prompt);
		for (int i = 0; i < size; i++)
		{
			cmd.emplace_back("[" + responses[i] + "]");
			cmd.emplace_back(labels[i]);
		}
		
		return cmd;
	}
};

// Represents what response the player picked for which choice
struct SelectedChoiceData
{
	int choiceNumber = -1;
	int responseNumber = -1;
};


class KINJO_API CutsceneManager
{	
	std::string data = "";
public:	 
	std::vector<std::string> languages = { "english", "japanese" };
	int currentLanguageIndex = 0;

	std::string currentText = "";
	std::string previousText = "";
	std::string beforeBacklogText = "";
	std::string beforeBacklogSpeaker = "";
	bool rclickEnabled = true;
	bool autoreturn = false;
	bool autosave = false;
	bool autoprint = false;
	bool autoskip = false;
	int printNumber = 0;
	bool overwriteName = true;
	std::string currentScript = "";

	bool playSoundsOnText = false;
	std::unordered_map<std::string, std::string> textSounds;
	
	CutsceneCommands commands;
	bool renderCutscene = true;

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
	std::vector<SceneLine> lines;
	std::vector<int> cmdStart;
	std::vector<int> cmdEnd;

	SceneLabel* currentLabel = nullptr;
	std::vector<SceneData*> gosubStack;
	std::vector<SceneRepeatData> repeatStack;

	// Checkpoint for debugging purposes
	SceneData checkpoint;

	std::unordered_map<int, PrintEffect> printEffects;

	// A list of all unique choices that is populated on game startup.
	std::unordered_map<int, SceneChoice> allChoices;

	// Represents the choices seen by the player for a single playthrough in order.
	// This should be cleared and re-loaded between save files.
	std::vector<SelectedChoiceData> selectedChoices;

	int currentChoice = -1;

	const int choiceSpriteStartNumber = 10000;
	int buttonIndex = 0;
	int buttonResult = 0;
	bool watchingCutscene = false;
	bool waitingForButton = false;
	bool waitingForClick = false;
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
	std::vector<int> activeButtons;
	std::unordered_map<int, int> spriteButtons;

	bool foundTrueConditionOnBtnWait = false;
	unsigned int textboxImageNumber = 995;

	std::map<unsigned int, Entity*> images; // needs to be in order for rendering
	std::map<unsigned int, Entity*>::iterator imageIterator;

	std::map<unsigned int, unsigned int> seenLabelsToMostRecentLine;
	std::unordered_map<std::string, std::string> namesToNames;
	std::unordered_map<std::string, Color> namesToColors;
	std::unordered_map<std::string, TextTag*> tags;
	std::unordered_map<unsigned int, Timer*> timers;
	std::unordered_map<unsigned int, std::vector<std::string>> timerCommands;

	float msGlyphTime = 0;
	float msInitialDelayBetweenGlyphs = 10.0f;
	float msDelayBetweenGlyphs = 10.0f;
	bool isCarryingOutCommands = false;
	bool isReadingNextLine = false;
	Timer nextLetterTimer;
	Uint32 mouseState = 0;
	Uint32 previousMouseState = 0;
	bool clickedMidPage = false;
	
	bool isSkipping = false;
	bool disableSkip = false;
	bool atChoice = false;
	int autoChoice = 0;

	bool isTravelling = false;
	std::string endTravelLabel = "";

	std::string backlogBtnUp = "";
	std::string backlogBtnDown = "";
	int backlogBtnUpX = 0;
	int backlogBtnUpY = 0;
	int backlogBtnDownX = 0;
	int backlogBtnDownY = 0;

	const std::string& GetLanguage() { return languages[currentLanguageIndex];  }

	CutsceneManager();
	void Init(Game& g);
	void ParseCutsceneFile();
	void ExecuteDefineBlock(const char* configName);
	void Update();
	void Render(const Renderer& renderer);
	void RenderTextbox(const Renderer& renderer);
	SceneLabel* JumpToLabel(const std::string& newLabelName);
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

	void MakeChoice();

	void SaveGame(const char* filename, const char* path = "saves/");
	void LoadGame(const char* filename, const char* path = "saves/");

	void FlushCurrentColor(const std::string& speakerName="");

	void LoadGlobalVariables();

	void SaveGlobalVariable(unsigned int key, const std::string& value, bool isNumber);

	void ModifyGlobalVariableVector(std::vector<string>& globalData, unsigned int key, const std::string& value);

	void SetSpeakerText(const std::string& name);

	void ReadCutsceneFile();
	void ClearPage();

	const std::string GetLabelName(const SceneLabel& label) const;
	const std::string GetCommand(const SceneLine& line, int index) const;
	const std::string GetLineText(const SceneLine& line) const;
	const std::string GetLineSpeaker(const SceneLine& line) const;

	void OpenBacklog();
	void CloseBacklog();

	SceneLabel* GetCurrentLabel();
	SceneLine* GetCurrentLine();
};

#endif