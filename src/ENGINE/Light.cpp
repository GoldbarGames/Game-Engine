#include "Light.h"

Light::Light()
{
	color = glm::vec3(1.0f, 1.0f, 1.0f);
	ambientIntensity = 1.0f;
	diffuseIntensity = 0.0f;
}

Light::Light(float red, float green, float blue, float aIntensity, float dIntensity)
{
	color = glm::vec3(red, green, blue);
	ambientIntensity = aIntensity;

	diffuseIntensity = dIntensity;
}

Light::Light(glm::vec3 col, float ai, float di)
{
	color = col;
	ambientIntensity = ai;
	diffuseIntensity = di;
}

void Light::UseLight(int ambientIntensityLocation, int ambientColorLocation, int diffuseIntensityLocation)
{
	glUniform3f(ambientColorLocation, color.x, color.y, color.z);
	glUniform1f(ambientIntensityLocation, ambientIntensity);

	glUniform1f(diffuseIntensityLocation, diffuseIntensity);
}

Light::~Light()
{

}