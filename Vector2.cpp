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

Vector2& Vector2::operator+(const Vector2& rhs)
{
	this->x += rhs.x;
	this->y += rhs.y;
	return *this;
}

Vector2& Vector2::operator-(const Vector2 & rhs)
{
	this->x -= rhs.x;
	this->y -= rhs.y;
	return *this;
}

Vector2& Vector2::operator*(const Vector2 & rhs)
{
	this->x *= rhs.x;
	this->y *= rhs.y;
	return *this;
}

Vector2& Vector2::operator/(const Vector2 & rhs)
{
	this->x /= rhs.x;
	this->y /= rhs.y;
	return *this;
}

bool Vector2::operator==(const Vector2 & rhs)
{
	return (x == rhs.x && y == rhs.y);
}

void Vector2::RoundToInt()
{
	x = (int)x;
	y = (int)y;
}