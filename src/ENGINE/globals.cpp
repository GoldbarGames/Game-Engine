#include "globals.h"
#include <iostream>
#include <time.h>
#include "Vector2.h"
#include <fstream>
#include <glm/geometric.hpp>

int Globals::TILE_SIZE = 24;
std::string Globals::NONE_STRING = "None";
uint32_t Globals::CurrentTicks = 0;
const float Globals::TO_RADIANS = 3.14159265f / 180.0f;
std::vector<std::string> Globals::languages = { "english", "japanese" };
int Globals::currentLanguageIndex = 0;
std::unordered_map<std::string, std::unordered_map<int, std::string>> Globals::translateMaps;

// TODO: Does lerp need to use dt?

// NOTE: On Mac OSX, error: myabs(float) is ambiguous,
// so I've replaced it with a custom abs() function
// Hopefully there is a better solution than this
float myabs(float n)
{
#if _WIN32
	return std::abs(n);
#else
	return (n < 0) ? n * -1.0f : n;
#endif
}

bool LerpVector2(Vector2& current, const Vector2& target, const float maxStep, const float minStep)
{
	bool xDirectionPositive = (current.x < target.x);
	bool yDirectionPositive = (current.y < target.y);

	float xStep = std::max((myabs(target.x - current.x) / myabs(target.x)) * minStep, maxStep);
	float yStep = std::max((myabs(target.y - current.y) / myabs(target.y)) * minStep, maxStep);

	if (xDirectionPositive)
		current.x = std::min(target.x, current.x + xStep);
	else
		current.x = std::max(target.x, current.x - xStep);

	if (yDirectionPositive)
		current.y = std::min(target.y, current.y + yStep);
	else
		current.y = std::max(target.y, current.y - yStep);

	bool xFinished = myabs(current.x - target.x) < 1;
	bool yFinished = myabs(current.y - target.y) < 1;

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
	//std::cout << current.x << "," << current.y << std::endl;

	bool xFinished = LerpCoord(current.x, start.x, target.x, t);
	bool yFinished = LerpCoord(current.y, start.y, target.y, t);

	//std::cout << overlayColor.a << std::endl;
	//std::cout << timerOverlayColor.GetTicks() << std::endl;
	//timerOverlayColor.Start(1);

	return xFinished && yFinished;
}

// TODO: Actually implement this for a 3D vector (it is currently 2D)
bool LerpVector3(glm::vec3& current, const glm::vec3& target, const float maxStep, const float minStep)
{
	bool xDirectionPositive = (current.x < target.x);
	bool yDirectionPositive = (current.y < target.y);

	float xStep = std::max((myabs(target.x - current.x) / myabs(target.x)) * minStep, maxStep);
	float yStep = std::max((myabs(target.y - current.y) / myabs(target.y)) * minStep, maxStep);

	if (xDirectionPositive)
		current.x = std::min(target.x, current.x + xStep);
	else
		current.x = std::max(target.x, current.x - xStep);

	if (yDirectionPositive)
		current.y = std::min(target.y, current.y + yStep);
	else
		current.y = std::max(target.y, current.y - yStep);

	bool xFinished = myabs(current.x - target.x) < 1;
	bool yFinished = myabs(current.y - target.y) < 1;

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
	//std::cout << current.x << "," << current.y << std::endl;

	bool xFinished = LerpCoord(current.x, start.x, target.x, t);
	bool yFinished = LerpCoord(current.y, start.y, target.y, t);
	bool zFinished = LerpCoord(current.z, start.z, target.z, t);

	//std::cout << overlayColor.a << std::endl;
	//std::cout << timerOverlayColor.GetTicks() << std::endl;
	//timerOverlayColor.Start(1);

	return xFinished && yFinished && zFinished;
}

// Return whether or not all values have reached their target values
bool LerpVector4(glm::vec4& current, const glm::vec4& start, const glm::vec4& target,
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
	//std::cout << current.x << "," << current.y << std::endl;

	bool rFinished = LerpCoord(current.r, start.r, target.r, t);
	bool gFinished = LerpCoord(current.g, start.g, target.g, t);
	bool bFinished = LerpCoord(current.b, start.b, target.b, t);
	bool aFinished = LerpCoord(current.a, start.a, target.a, t);

	//std::cout << overlayColor.a << std::endl;
	//std::cout << timerOverlayColor.GetTicks() << std::endl;
	//timerOverlayColor.Start(1);

	return rFinished && gFinished && bFinished && aFinished;
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

bool HasVerticalIntersection(const SDL_Rect& rect1, const SDL_Rect& rect2)
{
	bool b1 = rect1.x < (rect2.x + rect2.w);
	bool b2 = (rect1.x + rect1.w) > rect2.x;

	bool b3 = rect1.y < (rect2.y + rect2.h);
	bool b4 = (rect1.y + rect1.h) > rect2.y;

	return b1 && b2 && b3 && b4;
}

bool HasHorizontalIntersection(const SDL_Rect& rect1, const SDL_Rect& rect2)
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

glm::vec4 ConvertColorToVec4(const Color& color)
{
	return glm::vec4(color.r, color.g, color.b, color.a);
}

glm::vec4 ConvertColorToVec4NoAlpha(const Color& color)
{
	return glm::vec4(color.r, color.g, color.b, 0);
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

void ReplaceAll(std::string& s, const std::string& toReplace, const std::string& replaceWith)
{
	size_t pos = 0;
	while ((pos = s.find(toReplace, pos)) != std::string::npos)
	{
		s.replace(pos, toReplace.length(), replaceWith);
		pos += replaceWith.length();
	}
}

std::vector<std::string> ReadStringsFromFile(const std::string& filepath)
{
	std::vector<std::string> result;

	std::ifstream fin;
	char token[256];

	fin.open(filepath);
	if (fin.is_open())
	{
		while (!fin.eof())
		{
			fin.getline(token, 256);
			if (token[0] != '\0')
				result.emplace_back(token);
		}
	}
	else
	{
		std::cout << "ERROR: Could not open file " << filepath << std::endl;
	}

	fin.close();

	return result;
}

const std::string& GetLanguage()
{ 
	return Globals::languages[Globals::currentLanguageIndex]; 
}


void CalcAverageNormals(unsigned int* indices, unsigned int indiceCount, float* vertices,
	unsigned int verticeCount, unsigned int vLength, unsigned int normalOffset)
{

	for (size_t i = 0; i < indiceCount; i += 3)
	{
		unsigned int in0 = indices[i] * vLength;
		unsigned int in1 = indices[i+1] * vLength;
		unsigned int in2 = indices[i+2] * vLength;

		glm::vec3 v1(vertices[in1] - vertices[in0], vertices[in1 + 1] - vertices[in0 + 1], vertices[in1 + 2] - vertices[in0 + 2]);
		glm::vec3 v2(vertices[in2] - vertices[in0], vertices[in2 + 1] - vertices[in0 + 1], vertices[in2 + 2] - vertices[in0 + 2]);
		glm::vec3 normal = glm::cross(v1, v2);
		normal = glm::normalize(normal);

		in0 += normalOffset;
		in1 += normalOffset;
		in2 += normalOffset;

		vertices[in0] += normal.x;
		vertices[in0 + 1] += normal.y;
		vertices[in0 + 2] += normal.z;

		vertices[in1] += normal.x;
		vertices[in1 + 1] += normal.y;
		vertices[in1 + 2] += normal.z;

		vertices[in2] += normal.x;
		vertices[in2 + 1] += normal.y;
		vertices[in2 + 2] += normal.z;
	}

	for (size_t i = 0; i < verticeCount / vLength; i++)
	{
		unsigned int nOffset = i * vLength + normalOffset;

		glm::vec3 vec(vertices[nOffset], vertices[nOffset + 1], vertices[nOffset + 2]);
		vec = glm::normalize(vec);

		vertices[nOffset] = vec.x;
		vertices[nOffset + 1] = vec.y;
		vertices[nOffset + 2] = vec.z;
	}

}

void ReadTranslationData()
{
	std::ifstream fin;
	
	std::string baseWord = "";
	std::string newWord = "";
	int index = 0;

	for (int i = 0; i < Globals::languages.size(); i++)
	{
		fin.open("data/translations/" + Globals::languages[i] + ".txt");

		if (fin.is_open())
		{
			Globals::translateMaps[" "][i] = " ";
			for (std::string line; std::getline(fin, line); )
			{
				index = 0;
				baseWord = ParseWord(line, '`', index);
				newWord = ParseWord(line, '\n', index);

				Globals::translateMaps[baseWord][i] = newWord;
			}

			fin.close();
		}
		else
		{
			std::cout << "ERROR: Failed to open language file " << Globals::languages[i] << ".txt" << std::endl;
		}
	}
	
}