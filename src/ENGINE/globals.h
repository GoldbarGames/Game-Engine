#ifndef GLOBALS_H
#define GLOBALS_H
#pragma once

#include <vector>
#include <string>
#include <SDL2/SDL.h>
#include <glm/vec3.hpp>
#include "Vector2.h"
#include "leak_check.h"

const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

class KINJO_API Globals {
public:
	static int TILE_SIZE;
	static std::string NONE_STRING;
	static uint32_t CurrentTicks;
	Globals() { TILE_SIZE = 24; };
	static Globals* Get()
	{
		static Globals instance;
		return &instance;
	}
};

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

extern KINJO_API std::string CurrentDate();
extern KINJO_API std::string CurrentTime();
extern KINJO_API std::string GetDrawingLayerName(DrawingLayer layer);
extern KINJO_API std::string ParseWord(const std::string& text, char limit, int& index);
extern KINJO_API std::vector<std::string> SplitString(const std::string& str, char delim);
extern KINJO_API Color ParseColorHexadecimal(const std::string& text);
extern KINJO_API int HexToDecimal(const char hex);

extern KINJO_API bool LerpVector2(Vector2& current, const Vector2& target, const float maxStep, const float minStep);
extern KINJO_API bool LerpVector2(Vector2& current, const Vector2& start, const Vector2& target,
	const uint32_t currentTime, uint32_t startTime, uint32_t endTime);

extern KINJO_API bool LerpVector3(glm::vec3& current, const glm::vec3& target, const float maxStep, const float minStep);
extern KINJO_API bool LerpVector3(glm::vec3& current, const glm::vec3& start, const glm::vec3& target,
	const uint32_t currentTime, uint32_t startTime, uint32_t endTime);
extern KINJO_API bool LerpCoord(float& current, const float& start, const float& target, const float& t);

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

extern KINJO_API SDL_Rect ConvertCoordsFromCenterToTopLeft(const SDL_Rect& originalRect);

extern KINJO_API bool HasIntersection(const SDL_Rect& rect1, const SDL_Rect& rect2);
extern KINJO_API bool HasVerticalIntersection(const SDL_Rect& rect1, const SDL_Rect& rect2);
extern KINJO_API bool HasHorizontalIntersection(const SDL_Rect& rect1, const SDL_Rect& rect2);

extern KINJO_API void ReplaceAll(std::string& s, const std::string& toReplace, const std::string& replaceWith);

#endif