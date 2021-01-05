#ifndef LIGHT_H
#define LIGHT_H
#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

class Light
{
public:

	glm::vec3 color;
	float ambientIntensity;


	Light();
	Light(float red, float green, float blue, float intensity);
	~Light();

	void UseLight(int ambientIntensityLocation, int ambientColorLocation);

};

#endif