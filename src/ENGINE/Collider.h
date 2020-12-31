#ifndef COLLIDER_H
#define COLLIDER_H
#pragma once
#include "leak_check.h"
#include "globals.h"
#include "Vector2.h"
#include <glm/vec3.hpp>
#include <SDL2/SDL.h>

//TODO: This is only for a rectangle collider, maybe add more shapes later
class KINJO_API Collider
{
public:
	Vector2 offset = Vector2(0,0);
	Vector2 scale = Vector2(0, 0);
	SDL_Rect bounds;

	Collider(float x, float y, float w, float h);
	~Collider();

	void CreateCollider(float x, float y, float w, float h);
	void CalculateCollider(const Vector2& position, const glm::vec3& rotation);
};

#endif