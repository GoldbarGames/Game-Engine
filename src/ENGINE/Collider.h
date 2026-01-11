#ifndef COLLIDER_H
#define COLLIDER_H
#pragma once
#include "leak_check.h"
#include "globals.h"
#include <glm/vec3.hpp>
#include <SDL2/SDL.h>

//TODO: This is only for a rectangle collider, maybe add more shapes later
class KINJO_API Collider
{
public:
	glm::vec3 offset = glm::vec3(0, 0, 0);
	glm::vec3 scale = glm::vec3(0, 0, 0);
	SDL_Rect bounds;

	Collider(float x, float y, float w, float h);
	~Collider();

	void CreateCollider(float x, float y, float w, float h);
	void CalculateCollider(const glm::vec3& position, const glm::vec3& rotation);
};

#endif