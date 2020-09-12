#ifndef GLOBALS_H
#define GLOBALS_H
#pragma once

#include <vector>
#include <string>
#include <SDL.h>
#include <glm/vec3.hpp>
#include "Vector2.h"

const int TILE_SIZE = 24;
const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

enum class DrawingLayer { BACK = 0, MIDDLE = 10, OBJECT = 20, 
	COLLISION = 30, COLLISION2 = 35, FRONT = 40, INVISIBLE = 99 };

struct Color {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 0;

	bool operator==(const Color& other) const
	{
		return (r == other.r && g == other.g && b == other.b && a == other.a);
	}

	bool operator!=(const Color& other) const
	{
		return !(*this == other);
	}
};

std::string CurrentDate();
std::string CurrentTime();
std::string GetDrawingLayerName(DrawingLayer layer);
std::string ParseWord(const std::string& text, char limit, int& index);
std::vector<std::string> SplitString(const std::string& str, char delim);
Color ParseColorHexadecimal(const std::string& text);
int HexToDecimal(const char hex);

bool LerpVector2(Vector2& current, const Vector2& target, const float maxStep, const float minStep);
bool LerpVector2(Vector2& current, const Vector2& start, const Vector2& target,
	const uint32_t currentTime, uint32_t startTime, uint32_t endTime);

bool LerpVector3(glm::vec3& current, const glm::vec3& target, const float maxStep, const float minStep);
bool LerpVector3(glm::vec3& current, const glm::vec3& start, const glm::vec3& target,
	const uint32_t currentTime, uint32_t startTime, uint32_t endTime);
bool LerpCoord(float& current, const float& start, const float& target, const float& t);

template<typename T>
void delete_it(T& v)
{
	delete v;
	v = nullptr;
}

// trim from end of string (right)
inline std::string& RTrim(std::string& s, const char* t = " \t\n\r\f\v")
{
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}

// trim from beginning of string (left)
inline std::string& LTrim(std::string& s, const char* t = " \t\n\r\f\v")
{
	s.erase(0, s.find_first_not_of(t));
	return s;
}

// trim from both ends of string (right then left)
inline std::string& Trim(std::string& s, const char* t = " \t\n\r\f\v")
{
	return LTrim(RTrim(s, t), t);
}

bool HasIntersection(const SDL_Rect& rect1, const SDL_Rect& rect2);
SDL_Rect ConvertCoordsFromCenterToTopLeft(const SDL_Rect& originalRect);

#endif