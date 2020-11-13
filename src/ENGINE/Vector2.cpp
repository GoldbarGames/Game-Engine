#include "Vector2.h"
#include <glm/vec2.hpp>
#include <string>

Vector2::Vector2(float x2, float y2)
{
	x = x2;
	y = y2;
}

Vector2::Vector2(int x2, int y2)
{
	x = (int)x2;
	y = (int)y2;
}

Vector2::Vector2(unsigned int x2, unsigned int y2)
{
	x = (int)x2;
	y = (int)y2;
}

Vector2::Vector2(int x2, unsigned int y2)
{
	x = (int)x2;
	y = (int)y2;
}

Vector2::Vector2(unsigned int x2, int y2)
{
	x = (int)x2;
	y = (int)y2;
}

Vector2::Vector2()
{
	x = 0;
	y = 0;
}

Vector2::~Vector2()
{
}

Vector2& Vector2::operator+=(const Vector2& rhs)
{
	this->x += rhs.x;
	this->y += rhs.y;

	return *this;
}

Vector2& Vector2::operator-=(const Vector2 & rhs)
{
	this->x -= rhs.x;
	this->y -= rhs.y;

	return *this;
}

Vector2 Vector2::operator+(const Vector2& rhs)
{
	Vector2 result = Vector2(this->x, this->y);
	result.x += rhs.x;
	result.y += rhs.y;
	return result;
}

Vector2 Vector2::operator-(const Vector2 & rhs)
{
	Vector2 result = Vector2(this->x, this->y);
	result.x -= rhs.x;
	result.y -= rhs.y;
	return result;
}

Vector2 Vector2::operator*(const Vector2 & rhs)
{
	Vector2 result = Vector2(this->x, this->y);
	result.x *= rhs.x;
	result.y *= rhs.y;
	return result;
}

Vector2 Vector2::operator/(const Vector2 & rhs)
{
	Vector2 result = Vector2(this->x, this->y);
	result.x /= rhs.x;
	result.y /= rhs.y;
	return result;
}

bool Vector2::operator==(const Vector2 & rhs)
{
	return (x == rhs.x && y == rhs.y);
}

bool Vector2::operator!=(const Vector2 & rhs)
{
	return (x != rhs.x || y != rhs.y);
}

Vector2 Vector2::RoundToInt()
{
	int rx = (int)x;
	int ry = (int)y;
	return Vector2(rx, ry);
}

std::string Vector2::ToString()
{
	return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
}

//TODO: Distance function? Other calculations?