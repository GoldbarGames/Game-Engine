#include "globals.h"
#include <iostream>
#include <time.h>
#include "Vector2.h"

// TODO: Does lerp need to use dt?

bool LerpVector2(Vector2& current, const Vector2& target, const float maxStep, const float minStep)
{
	bool xDirectionPositive = (current.x < target.x);
	bool yDirectionPositive = (current.y < target.y);

	float xStep = std::max((std::abs(target.x - current.x) / std::abs(target.x)) * minStep, maxStep);
	float yStep = std::max((std::abs(target.y - current.y) / std::abs(target.y)) * minStep, maxStep);

	if (xDirectionPositive)
		current.x = std::min(target.x, current.x + xStep);
	else
		current.x = std::max(target.x, current.x - xStep);

	if (yDirectionPositive)
		current.y = std::min(target.y, current.y + yStep);
	else
		current.y = std::max(target.y, current.y - yStep);

	bool xFinished = std::abs(current.x - target.x) < 1;
	bool yFinished = std::abs(current.y - target.y) < 1;

	//std::cout << "x: " << current.x << "/" << target.x << std::endl;
	//std::cout << "y: " << current.y << "/" << target.y << std::endl;

	return xFinished && yFinished;
}

bool LerpVector2(Vector2& current, const Vector2& start, const Vector2& target,
	const uint32_t currentTime, uint32_t startTime, uint32_t endTime)
{
	float difference = endTime - startTime;
	float t = 1.0f;
	if (difference != 0)
	{
		t = (currentTime - startTime) / difference; // percentage of passed time
	}

	if (t > 1.0f)
		t = 1.0f;

	//std::cout << (currentTime - startTime) << " / " << difference << " = " << t << std::endl;

	bool xFinished = LerpCoord(current.x, start.x, target.x, t);
	bool yFinished = LerpCoord(current.y, start.y, target.y, t);

	//std::cout << overlayColor.a << std::endl;
	//std::cout << timerOverlayColor.GetTicks() << std::endl;
	//timerOverlayColor.Start(1);

	return xFinished && yFinished;
}

bool LerpVector3(glm::vec3& current, const glm::vec3& target, const float maxStep, const float minStep)
{
	bool xDirectionPositive = (current.x < target.x);
	bool yDirectionPositive = (current.y < target.y);

	float xStep = std::max((std::abs(target.x - current.x) / std::abs(target.x)) * maxStep, minStep);
	float yStep = std::max((std::abs(target.y - current.y) / std::abs(target.y)) * maxStep, minStep);

	if (xDirectionPositive)
		current.x = std::min(target.x, current.x + xStep);
	else
		current.x = std::max(target.x, current.x - xStep);

	if (yDirectionPositive)
		current.y = std::min(target.y, current.y + yStep);
	else
		current.y = std::max(target.y, current.y - yStep);

	bool xFinished = std::abs(current.x - target.x) < 1;
	bool yFinished = std::abs(current.y - target.y) < 1;

	//std::cout << "x: " << current.x << "/" << target.x << std::endl;
	//std::cout << "y: " << current.y << "/" << target.y << std::endl;

	return xFinished && yFinished;
}

// Return whether or not all three values have reached their target values
bool LerpVector3(glm::vec3& current, const glm::vec3& start, const glm::vec3& target, 
	const uint32_t currentTime, uint32_t startTime, uint32_t endTime)
{
	float difference = endTime - startTime;
	float t = 1.0f;
	if (difference != 0)
	{
		t = (currentTime - startTime) / difference; // percentage of passed time
	}

	if (t > 1.0f)
		t = 1.0f;

	//std::cout << (currentTime - startTime) << " / " << difference << " = " << t << std::endl;
	
	bool xFinished = LerpCoord(current.x, start.x, target.x, t);
	bool yFinished = LerpCoord(current.y, start.y, target.y, t);

	//std::cout << overlayColor.a << std::endl;
	//std::cout << timerOverlayColor.GetTicks() << std::endl;
	//timerOverlayColor.Start(1);

	return xFinished && yFinished;
}

bool LerpCoord(float& current, const float& start, const float& target, const float& t)
{
	if (current != target)
	{
		current = start + (t * (target - start));

		if ((current - target) * (current - target) < 1)
			current = target;
	}

	return (current == target);
}

// This is better than SDL_HasIntersection because it works with negative numbers
// NOTE: This function assumes that x and y are the top-left corners of both rectangles!
bool HasIntersection(const SDL_Rect& rect1, const SDL_Rect& rect2)
{
	bool b1 = rect1.x < (rect2.x + rect2.w);
	bool b2 = (rect1.x + rect1.w) > rect2.x;

	bool b3 = rect1.y < (rect2.y + rect2.h);
	bool b4 = (rect1.y + rect1.h) > rect2.y;

	return b1 && b2 && b3 && b4;
}

//TODO: Replace this conversion function with simply using 
SDL_Rect ConvertCoordsFromCenterToTopLeft(const SDL_Rect& originalRect)
{
	SDL_Rect result;
	result.x = originalRect.x - (originalRect.w);
	result.y = originalRect.y - (originalRect.h);
	result.w = originalRect.w * 2;
	result.h = originalRect.h * 2;
	return result;
}

std::string GetDrawingLayerName(DrawingLayer layer)
{
	return "";
}

std::string CurrentDate() {
	time_t now = time(NULL);
	struct tm tstruct;
	char buf[40];
	tstruct = *localtime(&now);
	//format: day DD-MM-YYYY
	strftime(buf, sizeof(buf), "%d-%m-%Y", &tstruct);
	return buf;
}
std::string CurrentTime() {
	time_t now = time(NULL);
	struct tm tstruct;
	char buf[40];
	tstruct = *localtime(&now);
	//format: HH:mm:ss
	strftime(buf, sizeof(buf), "%X", &tstruct);
	return buf;
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
			//std::cout << "ERROR: Parsing word, index out of range: " + word << std::endl;
			break;
		}
	}

	index++; // move past the space/newline
	return word;
}