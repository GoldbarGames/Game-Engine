#include "Logger.h"
#include <chrono>
#include <ctime> 
#include <string>
#include <iostream>

Logger::Logger(const char* filename)
{
	// Can set individually whether each type should be printed
	shouldPrintLogType[(int)LogType::DEFAULT] = true;
	shouldPrintLogType[(int)LogType::ERROR] = true;
	shouldPrintLogType[(int)LogType::WARNING] = true;

	// Can set individually whether each type should be written to file
	shouldWriteLogType[(int)LogType::DEFAULT] = true;
	shouldWriteLogType[(int)LogType::ERROR] = true;
	shouldWriteLogType[(int)LogType::WARNING] = true;

	file = std::ofstream(filename, std::ios_base::app);
	Log("Game opened", LogType::DEFAULT);
}

Logger::~Logger()
{
	if (file.is_open())
	{
		Log("Game closed");
		file.close();
	}
}

void Logger::SetOutputFile(const char* filename)
{
	if (file.is_open())
	{
		Log("File closed");
		file.close();
	}

	file = std::ofstream(filename, std::ios_base::app);
	Log("File opened");
}

void Logger::Log(const std::string& message, LogType ltype)
{
#if _DEBUG
	if (shouldPrintMessage && shouldPrintLogType[(int)ltype])
	{
		std::cout << message << std::endl;
	}
#endif

	if (file.is_open() && shouldWriteLogType[(int)ltype])
	{
		std::time_t endTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::string theTime = std::ctime(&endTime);
		file << theTime << " | " << message << std::endl;
	}		
}
