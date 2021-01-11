#ifndef LIGHT_H
#define LIGHT_H
#pragma once

#include "leak_check.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Shader.h"

class KINJO_API Light
{
public:

	glm::vec3 color;
	float ambientIntensity = 1.0f;
	float diffuseIntensity = 0.0f;

	Light();
	Light(glm::vec3 col, float ai, float di);
	Light(float red, float green, float blue, float aIntensity,  float dIntensity);
	~Light();

	virtual void UseLight(const ShaderProgram& shader);

};

#endif