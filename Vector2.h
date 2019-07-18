#pragma once
class Vector2
{
public:
	float x = 0.0f;
	float y = 0.0f;
	Vector2(float x2, float y2);
	Vector2();
	~Vector2();
	Vector2& operator+=(const Vector2& rhs);
	Vector2& operator-=(const Vector2& rhs);
	Vector2& operator+(const Vector2& rhs);
	Vector2& operator-(const Vector2& rhs);
	Vector2& operator/(const Vector2& rhs);
	bool operator==(const Vector2& rhs);
	void RoundToInt();
};

