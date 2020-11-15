#ifndef VECTOR2_H
#define VECTOR2_H

#include <iostream>
//using Vector2 = glm::vec2;
#include "leak_check.h"

class KINJO_API Vector2
{
public:
	float x = 0.0f;
	float y = 0.0f;
	Vector2(float x2, float y2);
	Vector2(int x2, int y2);
	Vector2(unsigned int x2, unsigned int y2);
	Vector2(int x2, unsigned int y2);
	Vector2(unsigned int x2, int y2);
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
	bool operator==(const Vector2& rhs) const;
	
	Vector2 RoundToInt();
	std::string ToString();
};

inline Vector2 RoundToInt(Vector2 vector)
{
	Vector2 newVector;
	newVector.x = static_cast<float>(static_cast<int>(vector.x));
	newVector.y = static_cast<float>(static_cast<int>(vector.y));
	return newVector;
}

inline std::ostream& operator<<(std::ostream& output, Vector2 value)
{
	return output << value.x << "," << value.y;
}

#endif