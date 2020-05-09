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

int HexToDecimal(const char hex)
{
	int result = 0;
	
	switch (hex)
	{
		case 'A':
		case 'a':
			result = 10;
			break;
		case 'B':
		case 'b':
			result = 11;
			break;
		case 'C':
		case 'c':
			result = 12;
			break;
		case 'D':
		case 'd':
			result = 13;
			break;
		case 'E':
		case 'e':
			result = 14;
			break;
		case 'F':
		case 'f':
			result = 15;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			result = hex - '0';
			break;
		default:
			break;
	}

	return result;
}

Color ParseColorHexadecimal(const std::string& text)
{
	Color color = { 0, 0, 0, 0 };

	int index = 1;
	if (text[0] == '#')
	{
		// Color B
		color.b = (HexToDecimal(text[index]) * 16) + HexToDecimal(text[index+1]);
		index += 2;

		// Color G
		color.g = (HexToDecimal(text[index]) * 16) + HexToDecimal(text[index + 1]);
		index += 2;

		// Color R
		color.r = (HexToDecimal(text[index]) * 16) + HexToDecimal(text[index + 1]);
		index += 2;

		// Color A
		if (text.size() > 7)
		{
			color.a = (HexToDecimal(text[index]) * 16) + HexToDecimal(text[index + 1]);
		}
		else
		{
			color.a = 255;
		}
	}

	return color;
}

std::string ParseWord(const std::string& text, char limit, int& index)
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