#ifndef DIRECTIONALLIGHT_H
#define DIRECTIONALLIGHT_H
#pragma once

#include "Light.h"

class KINJO_API DirectionalLight : public Light
{
public:

	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
	float ambientIntensity = 1.0f;

	glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);
	float diffuseIntensity = 0.0f;


	DirectionalLight();
	DirectionalLight(glm::vec3 col, float ai, float di, glm::vec3 dir);
	DirectionalLight(float red, float green, float blue, float aIntensity,
		float xdir, float ydir, float zdir, float dIntensity);
	~DirectionalLight();

	void UseLight(int ambientIntensityLocation, int ambientColorLocation,
		int diffuseIntensityLocation, int directionLocation);

};

#endif