#ifndef SPOTLIGHT_H
#define SPOTLIGHT_H
#pragma once

#include "PointLight.h"

class KINJO_API SpotLight : public PointLight
{
public:
	glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);
	float edge, procEdge;

	SpotLight();
	SpotLight(glm::vec3 col, float ai, float di, glm::vec3 pos, glm::vec3 att, glm::vec3 dir, float edg);
	SpotLight(float red, float green, float blue, float aIntensity, float dIntensity,
		float xpos, float ypos, float zpos, float xdir, float ydir, float zdir, 
		float con, float lin, float exp, float edg);
	~SpotLight();

	void UseLight(const ShaderProgram& shader);

};

#endif