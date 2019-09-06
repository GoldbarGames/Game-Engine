#pragma once
#include "Textbox.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Timer.h"

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
		for (int i = 0; i < lines.size(); i++)
		{
			delete lines[i];
		}
		lines.clear();
	}
};


class CutsceneManager
{
	Game* game = nullptr;
	Textbox * textbox = nullptr;
	std::vector<SceneLabel*> labels;
	std::string language = "english";
	std::string data = "";
	int lineIndex = 0;
	int letterIndex = 0;
	std::string currentText = "";
	SceneLabel * currentLabel = nullptr;
public:
	float timer = 0;
	bool isReadingNextLine = false;
	Timer nextLetterTimer;
	CutsceneManager(Game& g);
	void ParseScene();
	std::string ParseWord(std::string text, char limit, int& index);
	void Update();
	void Render(Renderer * renderer);
	SceneLabel * JumpToLabel(std::string newLabelName);
	void PlayCutscene(std::string labelName);
	void ReadNextLine();
};
