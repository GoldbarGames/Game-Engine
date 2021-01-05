#include "Light.h"

Light::Light()
{
	color = glm::vec3(1.0f, 1.0f, 1.0f);
	ambientIntensity = 1.0f;

	direction = glm::vec3(0.0f, -1.0f, 0.0f);
	diffuseIntensity = 0.0f;
}

Light::Light(float red, float green, float blue, float aIntensity,
	float xdir, float ydir, float zdir, float dIntensity)
{
	color = glm::vec3(red, green, blue);
	ambientIntensity = aIntensity;

	direction = glm::vec3(xdir, ydir, zdir);
	diffuseIntensity = dIntensity;
}

Light::Light(glm::vec3 col, float ai, glm::vec3 dir, float di)
{
	color = col;
	ambientIntensity = ai;
	direction = dir;
	diffuseIntensity = di;
}

void Light::UseLight(int ambientIntensityLocation, int ambientColorLocation, int diffuseIntensityLocation, int directionLocation)
{
	glUniform3f(ambientColorLocation, color.x, color.y, color.z);
	glUniform1f(ambientIntensityLocation, ambientIntensity);

	glUniform3f(directionLocation, direction.x, direction.y, direction.z);
	glUniform1f(diffuseIntensityLocation, diffuseIntensity);
}

Light::~Light()
{

}