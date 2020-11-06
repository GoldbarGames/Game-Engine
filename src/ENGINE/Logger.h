#ifndef LOGGER_H
#define LOGGER_H
#pragma once

#include <fstream>
#include "leak_check.h"
class DECLSPEC Logger
{
public:
	std::ofstream file;
	bool shouldPrintMessage = true;
	Logger(const char* filename);
	~Logger();
	void Log(const std::string& message);

	void SetOutputFile(const char* filename);
};

#endif