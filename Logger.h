#pragma once

#include <fstream>

class Logger
{
public:
	std::ofstream file;

	Logger(const char* filename);
	~Logger();
	void Log(const char* message);
};

