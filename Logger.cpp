#include "Logger.h"
#include <chrono>
#include <ctime> 
#include <string>
#include <iostream>

Logger::Logger(const char* filename)
{
	file = std::ofstream(filename, std::ios_base::app);
	Log("Game opened");
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

void Logger::Log(const char* message)
{
	if (shouldPrintMessage)
	{
		std::cout << message << std::endl;
	}

	if (file.is_open())
	{
		std::time_t endTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		char* theTime = std::ctime(&endTime);
		theTime[strcspn(theTime, "\r\n")] = '\0';
		file << theTime << " | " << message << std::endl;
	}		
}
