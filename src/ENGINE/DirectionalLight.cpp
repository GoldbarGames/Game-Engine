#include "DirectionalLight.h"

DirectionalLight::DirectionalLight() : Light()
{
	direction = glm::vec3(0.0f, -1.0f, 0.0f);
}

DirectionalLight::DirectionalLight(float red, float green, float blue, float aIntensity,
	float dIntensity, float xdir, float ydir, float zdir) : Light(red, green, blue, aIntensity, dIntensity)
{
	direction = glm::vec3(xdir, ydir, zdir);
}

DirectionalLight::DirectionalLight(glm::vec3 col, float ai, float di, glm::vec3 dir) : Light(col, ai, di)
{
	direction = dir;
}

void DirectionalLight::UseLight(int ambientIntensityLocation, int ambientColorLocation, int diffuseIntensityLocation, int directionLocation)
{
	glUniform3f(ambientColorLocation, color.x, color.y, color.z);
	glUniform1f(ambientIntensityLocation, ambientIntensity);

	glUniform3f(directionLocation, direction.x, direction.y, direction.z);
	glUniform1f(diffuseIntensityLocation, diffuseIntensity);
}

DirectionalLight::~DirectionalLight()
{

}