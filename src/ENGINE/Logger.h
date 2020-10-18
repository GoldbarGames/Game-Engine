#ifndef LOGGER_H
#define LOGGER_H
#pragma once

#include <fstream>

class Logger
{
public:
	std::ofstream file;
	bool shouldPrintMessage = true;
	Logger(const char* filename);
	~Logger();
	void Log(const char* message);

	void SetOutputFile(const char* filename);
};

#endif