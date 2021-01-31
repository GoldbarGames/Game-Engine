#ifndef LOGGER_H
#define LOGGER_H
#pragma once

#include <fstream>
#include <unordered_map>
#include "leak_check.h"

enum class LogType { DEFAULT, ERROR, WARNING };

class KINJO_API Logger
{
public:
	std::ofstream file;
	bool shouldPrintMessage = true;

	std::unordered_map<int, bool> shouldPrintLogType;
	std::unordered_map<int, bool> shouldWriteLogType;

	Logger(const char* filename);
	~Logger();
	void Log(const std::string& message, LogType ltype = LogType::ERROR);

	void SetOutputFile(const char* filename);
};

#endif