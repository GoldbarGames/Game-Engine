#include "Vector2.h"

Vector2::Vector2(float x2, float y2)
{
	x = x2;
	y = y2;
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

Vector2 Vector2::RoundToInt()
{
	int rx = (int)x;
	int ry = (int)y;
	return Vector2(rx, ry);
}