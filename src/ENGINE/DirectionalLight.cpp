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

void DirectionalLight::UseLight(const ShaderProgram& shader)
{
	/*
	glUniform3f(shader.GetUniformVariable(ShaderVariable::ambientColor), color.x, color.y, color.z);
	glUniform1f(shader.GetUniformVariable(ShaderVariable::ambientIntensity), ambientIntensity);
	glUniform1f(shader.GetUniformVariable(ShaderVariable::diffuseIntensity), diffuseIntensity);

	glUniform3f(shader.GetUniformVariable(ShaderVariable::lightDirection), direction.x, direction.y, direction.z);

	*/

	glUniform3f(shader.uniformDirectionalLight.uniformColor, color.x, color.y, color.z);
	glUniform1f(shader.uniformDirectionalLight.uniformAmbientIntensity, ambientIntensity);
	glUniform1f(shader.uniformDirectionalLight.uniformDiffuseIntensity, diffuseIntensity);

	glUniform3f(shader.uniformDirectionalLight.uniformDirection, direction.x, direction.y, direction.z);
}

DirectionalLight::~DirectionalLight()
{

}