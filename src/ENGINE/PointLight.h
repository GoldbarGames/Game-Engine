#ifndef POINTLIGHT_H
#define POINTLIGHT_H
#pragma once

#include "Light.h"

class KINJO_API PointLight : public Light
{
public:
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
	float constant, linear, exponent = 0.0f; // for attenuation

	PointLight();
	PointLight(glm::vec3 col, float ai, float di, glm::vec3 pos, glm::vec3 attenuation);
	PointLight(float red, float green, float blue, float aIntensity, float dIntensity,
		float xpos, float ypos, float zpos, float con, float lin, float exp);
	~PointLight();

	void UseLight(const ShaderProgram& shader);

};

#endif