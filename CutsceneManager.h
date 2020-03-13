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


class CutsceneManager
{
	std::vector<SceneLabel*> labels;
	std::string language = "english";
	std::string data = "";
	unsigned int lineIndex = 0;
	unsigned int letterIndex = 0;
	unsigned int commandIndex = 0;
	std::string currentText = "";
	SceneLabel * currentLabel = nullptr;
	CutsceneCommands commands;
	std::queue<std::string> backlog;
	int backlogMaxSize = 3;
public:	 
	Textbox* textbox = nullptr;
	Game* game = nullptr;
	std::unordered_map<unsigned int, Entity*> images;
	std::unordered_map<unsigned int, Entity*>::iterator imageIterator;
	float timer = 0;
	bool isCarryingOutCommands = false;
	bool isReadingNextLine = false;
	Timer nextLetterTimer;
	CutsceneManager(Game& g);
	void ParseScene();
	std::string ParseWord(std::string text, char limit, unsigned int& index);
	void Update();
	void Render(Renderer * renderer);
	SceneLabel * JumpToLabel(std::string newLabelName);
	void PlayCutscene(std::string labelName);
	void EndCutscene();
	void ReadNextLine();
};
