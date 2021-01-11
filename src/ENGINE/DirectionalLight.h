#ifndef DIRECTIONALLIGHT_H
#define DIRECTIONALLIGHT_H
#pragma once

#include "Light.h"

class KINJO_API DirectionalLight : public Light
{
public:
	glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);

	DirectionalLight();
	DirectionalLight(glm::vec3 col, float ai, float di, glm::vec3 dir);
	DirectionalLight(float red, float green, float blue, float aIntensity, float dIntensity,
		float xdir, float ydir, float zdir);
	~DirectionalLight();

	void UseLight(const ShaderProgram& shader);

};

#endif