#ifndef MATERIAL_H
#define MATERIAL_H
#pragma once

#include "leak_check.h"

class KINJO_API Material
{
public:

	float specularIntensity = 0.0f;
	float shine = 0.0f;

	Material(float si, float sh);
	Material();
	~Material();

	void UseMaterial(int specularIntensityLocation, int shineLocation);
};

#endif