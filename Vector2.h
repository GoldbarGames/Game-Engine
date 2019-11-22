#pragma once
#include <iostream>

#include <glm/vec2.hpp>

using Vector2 = glm::vec2;

inline glm::vec2 RoundToInt(glm::vec2 vector)
{
	glm::vec2 newVector;
	newVector.x = static_cast<float>(static_cast<int>(vector.x));
	newVector.y = static_cast<float>(static_cast<int>(vector.y));
	return newVector;
}

/*
class Vector2
{
public:
	float x = 0.0f;
	float y = 0.0f;
	Vector2(float x2, float y2);
	Vector2(int x2, int y2);
	Vector2();
	~Vector2();
	Vector2& operator+=(const Vector2& rhs);
	Vector2& operator-=(const Vector2& rhs);
	Vector2 operator+(const Vector2& rhs);
	Vector2 operator-(const Vector2& rhs);
	Vector2 operator*(const Vector2& rhs);
	Vector2 operator/(const Vector2& rhs);
	bool operator==(const Vector2& rhs);
	bool operator!=(const Vector2 & rhs);
	
	Vector2 RoundToInt();
};

*/

inline std::ostream& operator<<(std::ostream& output, Vector2 value)
{
	return output << value.x << "," << value.y;
}


