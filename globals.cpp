#include "globals.h"
#include <iostream>

std::string GetDrawingLayerName(DrawingLayer layer)
{
	return "";
}

std::vector<std::string> SplitString(const std::string& str, char delim) 
{
	std::vector<std::string> strings;
	size_t start;
	size_t end = 0;
	while ((start = str.find_first_not_of(delim, end)) != std::string::npos) 
	{
		end = str.find(delim, start);
		strings.push_back(str.substr(start, end - start));
	}
	return strings;
}

std::string ParseWord(const std::string& text, char limit, unsigned int& index)
{
	std::string word = "";	
	size_t length = text.length();

	if (index >= length)
		return word;

	while (text[index] != limit)
	{
		word += text[index];
		index++;

		if (index >= length)
		{
			std::cout << "ERROR: Parsing word, index out of range: " + word << std::endl;
			break;
		}
	}

	index++; // move past the space/newline
	return word;
}