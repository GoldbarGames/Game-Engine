#ifndef LIGHT_H
#define LIGHT_H
#pragma once

#include "leak_check.h"
#include <GL/glew.h>
#include <glm/glm.hpp>

class KINJO_API Light
{
public:

	glm::vec3 color;
	float ambientIntensity = 1.0f;

	glm::vec3 direction;
	float diffuseIntensity = 0.0f;


	Light();
	Light(glm::vec3 col, float ai, glm::vec3 dir, float di);
	Light(float red, float green, float blue, float aIntensity, 
		float xdir, float ydir, float zdir, float dIntensity);
	~Light();

	void UseLight(int ambientIntensityLocation, int ambientColorLocation, 
		int diffuseIntensityLocation, int directionLocation);

};

#endif