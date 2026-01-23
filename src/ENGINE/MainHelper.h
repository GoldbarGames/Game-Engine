#ifndef MAINHELPER_H
#define MAINHELPER_H
#pragma once

#include "GUI.h"
#include "EntityFactory.h"
#include "FileManager.h"
#include "MenuManager.h"
#include "CutsceneHelper.h"
#include "NetworkManager.h"

#include <stdio.h>
#ifdef _WIN32
#include <conio.h>
#endif

class MainHelper
{
public:

	EntityFactory* e = nullptr;
	GUI* gui = nullptr;
	FileManager* f = nullptr;
	MenuManager* m = nullptr;
	CutsceneHelper* cmd = nullptr;
	NetworkManager* n = nullptr;

	MainHelper()
	{

	}

	~MainHelper()
	{
		if (gui != nullptr)
		{
			delete gui;
			gui = nullptr;
		}

		if (f != nullptr)
		{
			delete f;
			f = nullptr;
		}

		if (m != nullptr)
		{
			delete m;
			m = nullptr;
		}

		if (cmd != nullptr)
		{
			delete cmd;
			cmd = nullptr;
		}
	}
};

#endif