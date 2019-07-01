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

Vector2& Vector2::operator+=(Vector2& rhs)
{
	this->x += rhs.x;
	this->y += rhs.y;

	return *this;
}

Vector2 & Vector2::operator-=(Vector2 & rhs)
{
	this->x -= rhs.x;
	this->y -= rhs.y;

	return *this;
}
