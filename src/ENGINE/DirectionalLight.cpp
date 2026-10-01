#include "DirectionalLight.h"
#include "render/RenderDevice.h"

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

void DirectionalLight::UseLight(const ShaderProgram& shader, int index)
{
	/*
	Device().SetUniform((int)(shader.GetUniformVariable(ShaderVariable::ambientColor)), glm::vec3(color.x, color.y, color.z));
	Device().SetUniform((int)(shader.GetUniformVariable(ShaderVariable::ambientIntensity)), (float)(ambientIntensity));
	Device().SetUniform((int)(shader.GetUniformVariable(ShaderVariable::diffuseIntensity)), (float)(diffuseIntensity));

	Device().SetUniform((int)(shader.GetUniformVariable(ShaderVariable::lightDirection)), glm::vec3(direction.x, direction.y, direction.z));

	*/

	Device().SetUniform((int)(shader.uniformDirectionalLight.uniformColor), glm::vec3(color.x, color.y, color.z));
	Device().SetUniform((int)(shader.uniformDirectionalLight.uniformAmbientIntensity), (float)(ambientIntensity));
	Device().SetUniform((int)(shader.uniformDirectionalLight.uniformDiffuseIntensity), (float)(diffuseIntensity));

	Device().SetUniform((int)(shader.uniformDirectionalLight.uniformDirection), glm::vec3(direction.x, direction.y, direction.z));
}

DirectionalLight::~DirectionalLight()
{

}