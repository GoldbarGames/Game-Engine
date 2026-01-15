#ifndef GLOBALS_H
#define GLOBALS_H
#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <SDL2/SDL.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "leak_check.h"

const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

const int MAX_POINT_LIGHTS = 3;
const int MAX_SPOT_LIGHTS = 3;

class KINJO_API Globals {
public:
	static int TILE_SIZE;
	static std::string NONE_STRING;
	static uint32_t CurrentTicks;
	Globals() { TILE_SIZE = 24; };
	static const float TO_RADIANS;

	static std::vector<std::string> languages;
	static int currentLanguageIndex;

	// Map of the base language word to the languages[int] word
	static std::unordered_map<std::string, std::unordered_map<int, std::string>> translateMaps;

	static Globals* Get()
	{
		static Globals instance;
		return &instance;
	}
};

enum class DrawingLayer { BACK = 0, MIDDLE = 10, OBJECT = 20, 
	COLLISION = 30, COLLISION2 = 35, FRONT = 40, BG = 50, INVISIBLE = 99 };

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

struct ColorF {
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 0.0f;

	bool operator==(const Color& other) const
	{
		return (r == other.r && g == other.g && b == other.b && a == other.a);
	}

	bool operator!=(const Color& other) const
	{
		return !(*this == other);
	}
};

extern KINJO_API void ReadTranslationData();
extern KINJO_API std::string CurrentDate();
extern KINJO_API std::string CurrentTime();
extern KINJO_API std::string GetDrawingLayerName(DrawingLayer layer);
extern KINJO_API std::string ParseWord(const std::string& text, char limit, size_t& index);
extern KINJO_API std::vector<std::string> SplitString(const std::string& str, char delim);
extern KINJO_API Color ParseColorHexadecimal(const std::string& text);
extern KINJO_API int HexToDecimal(const char hex);

extern KINJO_API float Lerp(const float min, const float max, const float dt);

extern KINJO_API bool LerpVector2(glm::vec2& current, const glm::vec2& target, const float maxStep, const float minStep);
extern KINJO_API bool LerpVector2(glm::vec2& current, const glm::vec2& start, const glm::vec2& target,
	const uint32_t currentTime, uint32_t startTime, uint32_t endTime);

extern KINJO_API bool LerpVector3(glm::vec3& current, const glm::vec3& target, const float maxStep, const float minStep);
extern KINJO_API bool LerpVector3(glm::vec3& current, const glm::vec3& start, const glm::vec3& target,
	const uint32_t currentTime, uint32_t startTime, uint32_t endTime);
extern KINJO_API bool LerpVector4(glm::vec4& current, const glm::vec4& start, const glm::vec4& target,
	const uint32_t currentTime, uint32_t startTime, uint32_t endTime);
extern KINJO_API bool LerpCoord(float& current, const float& start, const float& target, const float& t);

extern KINJO_API const std::string& GetLanguage();

extern KINJO_API glm::vec4 ConvertColorToVec4(const Color& color);
extern KINJO_API glm::vec4 ConvertColorToVec4NoAlpha(const Color& color);

extern KINJO_API float Clamp(const float& value, const float& min, const float& max);

template<typename T>
void delete_it(T& v)
{
	delete v;
	v = nullptr;
}

// TODO: These Trim functions should either
// modify the value or return a new one,
// not do both... so pick one.

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

inline bool IsVec3Equals(const glm::vec3& lhs, const glm::vec3& rhs)
{
	bool x = static_cast<int>(lhs.x) == static_cast<int>(rhs.x);
	bool y = static_cast<int>(lhs.y) == static_cast<int>(rhs.y);
	bool z = static_cast<int>(lhs.z) == static_cast<int>(rhs.z);

	return x && y && z;
}

inline std::string Vec2ToString(const glm::vec2& v)
{
	return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + ")";
}

inline std::string Vec3ToString(const glm::vec3& v)
{
	return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + ")";
}

extern KINJO_API SDL_Rect ConvertCoordsFromCenterToTopLeft(const SDL_Rect& originalRect);

extern KINJO_API bool HasIntersection(const SDL_Rect& rect1, const SDL_Rect& rect2);
extern KINJO_API bool HasVerticalIntersection(const SDL_Rect& rect1, const SDL_Rect& rect2);
extern KINJO_API bool HasHorizontalIntersection(const SDL_Rect& rect1, const SDL_Rect& rect2);

extern KINJO_API void ReplaceAll(std::string& s, const std::string& toReplace, const std::string& replaceWith);

extern KINJO_API bool FileExists(const std::string& filepath);
extern KINJO_API std::vector<std::string> ReadStringsFromFile(const std::string& filepath, bool startAtOne=false);
extern KINJO_API std::unordered_map<std::string, std::string> GetMapStringsFromFile(const std::string& filepath, char limit=' ');
extern KINJO_API std::unordered_map<std::string, int> MapStringsToLineFromFile(const std::string& filepath, char limit=' ');

extern KINJO_API void CalcAverageNormals(unsigned int* indices, unsigned int indiceCount, float* vertices,
	unsigned int verticeCount, unsigned int vLength, unsigned int normalOffset);


#endif