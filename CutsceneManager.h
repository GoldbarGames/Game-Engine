#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#pragma once

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
	std::vector<SceneLabel*> labels;
public:
	void ParseScene(std::string data);
	std::string ParseWord(std::string text, char limit, int& index);
};
