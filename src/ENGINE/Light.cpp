#include "Light.h"

Light::Light()
{
	color = glm::vec3(1.0f, 1.0f, 1.0f);
	ambientIntensity = 1.0f;
}

Light::Light(float red, float green, float blue, float intensity)
{
	color = glm::vec3(red, green, blue);
	ambientIntensity = intensity;
}

void Light::UseLight(int ambientIntensityLocation, int ambientColorLocation)
{
	glUniform3f(ambientColorLocation, color.x, color.y, color.z);
	glUniform1f(ambientIntensityLocation, ambientIntensity);
}

Light::~Light()
{

}