#pragma once
#include "Textbox.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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
	Textbox * textbox = nullptr;
	std::vector<SceneLabel*> labels;
	std::string language = "english";
	std::string data = "";
public:
	CutsceneManager(Game& game);
	void ParseScene();
	std::string ParseWord(std::string text, char limit, int& index);
	void Render(Renderer * renderer);
};
